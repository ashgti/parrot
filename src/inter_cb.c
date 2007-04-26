/*
Copyright (C) 2001-2003, The Perl Foundation.
$Id$

=head1 NAME

src/inter_cb.c - Parrot Interpreter - Callback Function Handling

=head1 DESCRIPTION

NCI callback functions may run whenever the C code executes the callback.
To be prepared for asynchronous callbacks these are converted to callback
events.

Often callbacks should run synchronously. This can only happen when
the C-library calls the callback, because Parrot called a function in
the C-library.

=head2 Functions

=over 4

=cut

*/

#include <assert.h>
#include "parrot/parrot.h"
#include "inter_cb.str"


/*

=item C<PMC* Parrot_make_cb(Parrot_Interp interp, PMC* sub, PMC* user
        STRING* cb_signature)>

Create a callback function according to pdd16.

=cut

*/

PMC*
Parrot_make_cb(Parrot_Interp interp, PMC* sub, PMC* user_data,
        STRING *cb_signature)
{
    PMC* interp_pmc, *cb, *cb_sig;
    int type = '?';     /* avoid -Ox warning */
    char * sig_str;
    STRING *sc;
    /*
     * we stuff all the information into the user_data PMC and pass that
     * on to the external sub
     */
    interp_pmc = VTABLE_get_pmc_keyed_int(interp, interp->iglobals,
            (INTVAL) IGLOBALS_INTERPRETER);
    /* be sure __LINE__ is consistent */
    sc = CONST_STRING(interp, "_interpreter");
    VTABLE_setprop(interp, user_data, sc, interp_pmc);
    sc = CONST_STRING(interp, "_sub");
    VTABLE_setprop(interp, user_data, sc, sub);
    /* only ASCII signatures are supported */
    sig_str = cb_signature->strstart;

    if (strlen(sig_str) != 3) {
        internal_exception(1, "unhandled signature '%s' in make_cb",
              cb_signature->strstart);
    }

    ++sig_str;     /* Skip callback return type */

    if (*sig_str == 'U') {
        type = 'D';
    }
    else {
        ++sig_str;
        if (*sig_str == 'U') {
            type = 'C';
        }
        else {
            internal_exception(1, "unhandled signature '%s' in make_cb",
                    cb_signature->strstart);
        }
    }

    cb_sig = pmc_new(interp, enum_class_String);
    VTABLE_set_string_native(interp, cb_sig, cb_signature);
    sc = CONST_STRING(interp, "_signature");
    VTABLE_setprop(interp, user_data, sc, cb_sig);
    /*
     * We are going to be passing the user_data PMC to external code, but
     * it may go out of scope until the callback is called -- we don't know
     * for certain as we don't know when the callback will be called.
     * Therefore, to prevent the PMC from being destroyed by a DOD sweep,
     * we need to anchor it.
     *
     */
    dod_register_pmc(interp, user_data);

    /*
     * Finally, the external lib awaits a function pointer.
     * Create a PMC that points to Parrot_callback_C (or _D);
     * it can be passed on with signature 'p'.
     */
    cb = pmc_new(interp, enum_class_UnManagedStruct);
    /*
     * Currently, we handle only 2 types:
     * _C ... user_data is 2nd parameter
     * _D ... user_data is 1st parameter
     */
    if (type == 'C')
        PMC_data(cb) = F2DPTR(Parrot_callback_C);
    else
        PMC_data(cb) = F2DPTR(Parrot_callback_D);
    dod_register_pmc(interp, cb);

    return cb;
}

/*

=item C<static void verify_CD(char *external_data, PMC *user_data)>

Verify user_data PMC then continue with callback_CD

=cut

*/

static void callback_CD(Parrot_Interp, char *, PMC *user_data);

static void
verify_CD(char *external_data, PMC *user_data)
{
    Parrot_Interp interp = NULL;
    size_t i;

    /*
     * 1.) user_data is from external code so:
     *     verify that we get a PMC that is one that we have passed in
     *     as user data, when we prepared the callback
     */

    /* a NULL pointer or a pointer not aligned is very likely wrong */
    if (!user_data || ((UINTVAL)user_data & 3))
        PANIC("user_data doesn't look like a pointer");

    /*
     * We don't yet know which interpreter this PMC is from, so run
     * through all of the existing interpreters and check their PMC
     * pools
     */
    LOCK(interpreter_array_mutex);
    for (i = 0; i < n_interpreters; ++i) {
        if (interpreter_array[i] == NULL)
            continue;
        interp = interpreter_array[i];
        if (interp)
            if (contained_in_pool(interp,
                        interp->arena_base->pmc_pool, user_data))
                break;
    }
    UNLOCK(interpreter_array_mutex);
    if (!interp)
        PANIC("interpreter not found for callback");

    /*
     * 2) some more checks
     * now we should have the interpreter where that callback
     * did originate - do some further checks on the PMC
     */

    /* if that doesn't look like a PMC we are still lost */
    if (!PObj_is_PMC_TEST(user_data))
        PANIC("user_data isn't a PMC");

    if (!user_data->vtable)
        PANIC("user_data hasn't a vtable");
    /*
     * ok fine till here
     */
    callback_CD(interp, external_data, user_data);
}

/*

=item C<static void
callback_CD(Parrot_Interp, void *external_data, PMC *user_data)>

Common callback function handler. See pdd16.

=cut

*/

static void
callback_CD(Parrot_Interp interp, char *external_data, PMC *user_data)
{

    PMC *passed_interp;       /* the interp that originated the CB */
    PMC *passed_synchronous;  /* flagging synchronous execution */
    int synchronous = 0;      /* cb is hitting this sub somewhen
                               * inmidst, or not */
    STRING *sc;
    /*
     * 3) check interpreter ...
     */
    sc = CONST_STRING(interp, "_interpreter");
    passed_interp = VTABLE_getprop(interp, user_data, sc);
    if (PMC_data(passed_interp) != interp)
        PANIC("callback gone to wrong interpreter");

    sc = CONST_STRING(interp, "_synchronous");
    passed_synchronous = VTABLE_getprop(interp, user_data, sc);
    if (!PMC_IS_NULL(passed_synchronous) &&
            VTABLE_get_bool(interp, passed_synchronous))
        synchronous = 1;

    /*
     * 4) check if the call_back is synchronous:
     *    - if yes we are inside the NCI call
     *      we could run the Sub immediately now (I think)
     *    - if no, and that's always safe, post a CALLBACK_EVENT
     */

    if (synchronous) {
        /*
         * just call the sub
         */
        Parrot_run_callback(interp, user_data, external_data);
    }
    else {
        /*
         * create a CB_EVENT, put user_data and data inside and finito
         *
         * *if* this function is finally no void, i.e. the calling
         * C program awaits a return result from the callback,
         * then wait for the CB_EVENT_xx to finish and return the
         * result
         */
        Parrot_new_cb_event(interp, user_data, external_data);
    }
}

/*

=item C<void
Parrot_run_callback(Parrot_Interp interp,
                    PMC* user_data, void* external_data)>

Run a callback function. The PMC* user_data holds all
necessary items in its properties.

=cut

*/

void
Parrot_run_callback(Parrot_Interp interp,
                    PMC* user_data, char* external_data)
{
    PMC *    signature;
    PMC *    sub;
    STRING * sig_str;
    char *   p;
    char     pasm_sig[4];
    INTVAL   i_param;
    PMC *    p_param;
    void*    param = NULL;      /* avoid -Ox warning */
    STRING * sc;

    sc = CONST_STRING(interp, "_sub");
    sub = VTABLE_getprop(interp, user_data, sc);
    sc = CONST_STRING(interp, "_signature");
    signature = VTABLE_getprop(interp, user_data, sc);

    sig_str = VTABLE_get_string(interp, signature);
    p = sig_str->strstart;
    ++p;     /* Skip return type */

    pasm_sig[0] = 'v';  /* no return value supported yet */
    pasm_sig[1] = 'P';
    if (*p == 'U') /* user_data Z in pdd16 */
        ++p;       /* p is now type of external data */
    switch (*p) {
        case 'v':
            pasm_sig[2] = 'v';
            break;
#if 0
        case '2':
        case '3':
        case '4':
#endif
        case 'l':
            i_param = (INTVAL)(long) external_data;
            goto case_I;
        case 'i':
            i_param = (INTVAL)(int)(long) external_data;
            goto case_I;
        case 's':
            i_param = (INTVAL)(short)(long) external_data;
            goto case_I;
        case 'c':
            i_param = (INTVAL)(char)(long)external_data;
case_I:
            pasm_sig[2] = 'I';
            param = (void*) i_param;
            break;
#if 0
        case 'f':
        case 'd':
            /* these types don't fit into a pointer, they will not
             * work
             */
            break;
#endif
        case 'p':
            /* created a UnManagedStruct */
            p_param = pmc_new(interp, enum_class_UnManagedStruct);
            PMC_data(p_param) = external_data;
            pasm_sig[2] = 'P';
            param = (void*) p_param;
            break;
#if 0
        case 'P':
            pasm_sig[2] = 'P';
            break;
#endif
        case 't':
            pasm_sig[2] = 'S';
            param = string_from_cstring(interp, external_data, 0);
            break;
        default:
            internal_exception(1, "unhandled signature char '%c' in run_cb",
                               *p);
    }
    pasm_sig[3] = '\0';
    Parrot_runops_fromc_args_event(interp, sub, pasm_sig,
            user_data, param);
}
/*

=item C<void Parrot_callback_C(void *external_data, PMC *user_data)>

=item C<void Parrot_callback_D(PMC *user_data, void *external_data)>

NCI callback functions. See pdd16.

=cut

*/

void
Parrot_callback_C(char *external_data, PMC *user_data)
{
    verify_CD(external_data, user_data);
}

void
Parrot_callback_D(PMC *user_data, char *external_data)
{
    verify_CD(external_data, user_data);
}

/*

=back

=head1 SEE ALSO

F<include/parrot/interpreter.h>, F<docs/pdds/pdd16_native_call.pod>.

=cut

*/


/*
 * Local variables:
 *   c-file-style: "parrot"
 * End:
 * vim: expandtab shiftwidth=4:
 */
