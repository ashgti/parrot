/*
Copyright (C) 2001-2006, The Perl Foundation.
$Id$

=head1 NAME

src/exceptions.c - Exceptions

=head1 DESCRIPTION

Define the internal interpreter exceptions.

=over 4

=item * This is experimental code.

=item * The C<enum_class> of the Exception isn't fixed.

=item * The interface isn't fixed.

=item * Much of this may change in the future.

=back

=head2 Functions

=over 4

=cut

*/

#include "parrot/parrot.h"
#include "parrot/exceptions.h"

#include <stdarg.h>

/*

=item C<void
internal_exception(int exitcode, const char *format, ...)>

Signal a fatal exception.  This involves printing an error message to stderr,
and calling C<Parrot_exit> to invoke exit handlers and exit the process with the
given exitcode.  No error handlers are used, so it is not possible for Parrot
bytecode to intercept a fatal error (cf. C<real_exception>).  Furthermore, no
stack unwinding is done, so the exit handlers run in the current dynamic
environment.

=cut

*/

void
internal_exception(int exitcode, const char *format, ...)
{
    va_list arglist;
    va_start(arglist, format);
    vfprintf(stderr, format, arglist);
    fprintf(stderr, "\n");
    /* caution against output swap (with PDB_backtrace) */
    fflush(stderr);
    va_end(arglist);
/*
 * XXX TODO get rid of all the internal_exceptions or call them
 *          with an interpreter arg
    Parrot_exit(interp, exitcode);
 */
    exit(exitcode);
}

/* Panic handler */

#ifndef dumpcore
#  define dumpcore() \
     fprintf(stderr, "Sorry, coredump is not yet implemented " \
             "for this platform.\n\n"); \
             exit(1);
#endif

/*

=item C<void
do_panic(Interp *interp, const char *message,
         const char *file, int line)>

Panic handler.

=cut

*/

void
do_panic(Interp *interp, const char *message,
         const char *file, int line)
{
    /* Note: we can't format any floats in here--Parrot_sprintf
    ** may panic because of floats.
    ** and we don't use Parrot_sprintf or such, because we are
    ** already in panic --leo
    */
    fprintf(stderr, "Parrot VM: PANIC: %s!\n",
               message ? message : "(no message available)");

    fprintf(stderr, "C file %s, line %d\n",
               file ? file : "(not available)", line);

    fprintf(stderr, "Parrot file (not available), ");
    fprintf(stderr, "line (not available)\n");

    fprintf(stderr, "\n\
We highly suggest you notify the Parrot team if you have not been working on\n\
Parrot.  Use parrotbug (located in parrot's root directory) or send an\n\
e-mail to parrot-porters@perl.org.\n\
Include the entire text of this error message and the text of the script that\n\
generated the error.  If you've made any modifications to Parrot, please\n\
describe them as well.\n\n");

    fprintf(stderr, "Version     : %s\n", PARROT_VERSION);
    fprintf(stderr, "Configured  : %s\n", PARROT_CONFIG_DATE);
    fprintf(stderr, "Architecture: %s\n", PARROT_ARCHNAME);
    fprintf(stderr, "JIT Capable : %s\n", JIT_CAPABLE ? "Yes" : "No");
    if (interp)
        fprintf(stderr, "Interp Flags: %#x\n", interp->flags);
    else
        fprintf(stderr, "Interp Flags: (no interpreter)\n");
    fprintf(stderr, "Exceptions  : %s\n", "(missing from core)");
    fprintf(stderr, "\nDumping Core...\n");

    dumpcore();
}

/*

=item C<void push_exception(Interp * interp, PMC *handler)>

Add the exception handler on the stack.

=item C<void Parrot_push_action(Interp * interp, PMC *sub)>

Push an action handler onto the dynamic environment.

=item C<void Parrot_push_mark(Interp * interp, INTVAL mark)>

Push a cleanup mark onto the dynamic environment.

=item C<void Parrot_pop_mark(Interp * interp, INTVAL mark)>

Pop items off the dynamic environment up to the mark.

=cut

*/

void
push_exception(Interp *interp, PMC *handler)
{
    if (handler->vtable->base_type != enum_class_Exception_Handler)
        PANIC("Tried to set_eh a non Exception_Handler");
    stack_push(interp, &interp->dynamic_env, handler,
               STACK_ENTRY_PMC, STACK_CLEANUP_NULL);
}

static void
run_cleanup_action(Interp *interp, Stack_Entry_t *e)
{
    /*
     * this is called during normal stack_pop of the control
     * stack - run the action subroutine with an INTVAL arg of 0
     */
    PMC * const sub = UVal_pmc(e->entry);
    Parrot_runops_fromc_args(interp, sub, "vI", 0);
}

void
Parrot_push_action(Interp *interp, PMC *sub)
{
    if (!VTABLE_isa(interp, sub,
                const_string(interp, "Sub"))) {
        internal_exception(1, "Tried to push a non Sub PMC action");
    }
    stack_push(interp, &interp->dynamic_env, sub,
               STACK_ENTRY_ACTION, run_cleanup_action);
}

void
Parrot_push_mark(Interp *interp, INTVAL mark)
{
    stack_push(interp, &interp->dynamic_env, &mark,
               STACK_ENTRY_MARK, STACK_CLEANUP_NULL);
}

void
Parrot_pop_mark(Interp *interp, INTVAL mark)
{
    do {
        const Stack_Entry_t * const e
            = stack_entry(interp, interp->dynamic_env, 0);
        if (!e)
            real_exception(interp, NULL, 1,
                           "Mark %d not found.", mark);
        (void)stack_pop(interp, &interp->dynamic_env,
                        NULL, e->entry_type);
        if (e->entry_type == STACK_ENTRY_MARK) {
            if (UVal_int(e->entry) == mark)
                return;
        }
    } while (1);
}

/*

=item C<static PMC *
find_exception_handler(Interp * interp, PMC *exception)>

Find the exception handler for C<exception>.

=cut

*/

static PMC *
find_exception_handler(Interp *interp, PMC *exception)
{
    PMC *handler;
    STRING *message;
    char *m;
    int exit_status, print_location;
    int depth = 0;
    Stack_Entry_t *e;

    /* for now, we don't check the exception class and we don't
     * look for matching handlers.  [this is being redesigned anyway.]
     */
    message = VTABLE_get_string_keyed_int(interp, exception, 0);
    /* [TODO: replace quadratic search with something linear, hopefully without
       trashing abstraction layers.  -- rgr, 17-Sep-06.] */
    while ((e = stack_entry(interp, interp->dynamic_env, depth))) {
        if (e->entry_type == STACK_ENTRY_PMC) {
            handler = UVal_pmc(e->entry);
            if (handler && handler->vtable->base_type ==
                    enum_class_Exception_Handler) {
                return handler;
            }
        }
        depth++;
    }

    /* flush interpreter output to get things printed in order */
    PIO_flush(interp, PIO_STDOUT(interp));
    PIO_flush(interp, PIO_STDERR(interp));
    if (interp->debugger) {
        PIO_flush(interp->debugger, PIO_STDOUT(interp->debugger));
        PIO_flush(interp->debugger, PIO_STDERR(interp->debugger));
    }

    m = string_to_cstring(interp, message);
    exit_status = print_location = 1;
    if (m && *m) {
        fputs(m, stderr);
        if (m[strlen(m)-1] != '\n')
            fprintf(stderr, "%c", '\n');
    }
    else {
        const INTVAL severity =
            VTABLE_get_integer_keyed_int(interp, exception, 2);
        if (severity == EXCEPT_exit) {
            print_location = 0;
            exit_status =
                (int)VTABLE_get_integer_keyed_int(interp, exception, 1);
        }
        else
            fprintf(stderr, "No exception handler and no message\n");
    }
    if (m)
        string_cstring_free(m);
    /* caution against output swap (with PDB_backtrace) */
    fflush(stderr);
    if (print_location)
        PDB_backtrace(interp);
    /*
     * returning NULL from here returns resume address NULL to the
     * runloop, which will terminate the thread function finally
     *
     * XXX this check should better be in Parrot_exit
     */
    if (interp->thread_data && interp->thread_data->tid) {
        /*
         * we should probably detach the thread here
         */
        return NULL;
    }
    /*
     * only main should run the destroy functions - exit handler chain
     * is freed during Parrot_exit
     */
    Parrot_exit(interp, exit_status);

    return NULL;
}

/*

=item C<void
pop_exception(Interp * interp)>

Pops the topmost exception handler off the stack.

=cut

*/

void
pop_exception(Interp *interp)
{
    Stack_entry_type type;
    struct Parrot_cont * cc;

    PMC * const handler
        = stack_peek(interp, interp->dynamic_env, &type);

    if (! handler
            || type != STACK_ENTRY_PMC
            || handler->vtable->base_type != enum_class_Exception_Handler) {
        real_exception(interp, NULL, E_RuntimeError,
                "No exception to pop.");
    }
    cc = PMC_cont(handler);
    if (cc->to_ctx != CONTEXT(interp->ctx)) {
        real_exception(interp, NULL, E_RuntimeError,
                "No exception to pop.");
    }
    (void)stack_pop(interp, &interp->dynamic_env,
                    NULL, STACK_ENTRY_PMC);
}

/*

=item C<PMC*
new_c_exception_handler(Interp *interp, Parrot_exception *jb)>

Generate an exception handler, that catches PASM level exceptions inside
a C function. This could be a separate class too, for now just a private
flag bit is set.

=cut

*/

PMC*
new_c_exception_handler(Interp *interp, Parrot_exception *jb)
{
    PMC * const handler = pmc_new(interp, enum_class_Exception_Handler);
    /*
     * this flag denotes a C exception handler
     */
    PObj_get_FLAGS(handler) |= SUB_FLAG_C_HANDLER;
    VTABLE_set_pointer(interp, handler, jb);
    return handler;
}

/*

=item C<void
push_new_c_exception_handler(Interp *interp, Parrot_exception *jb)>

Pushes an new C exception handler onto the stack.

=cut

*/

void
push_new_c_exception_handler(Interp *interp, Parrot_exception *jb)
{
    push_exception(interp, new_c_exception_handler(interp, jb));
}

/*

=item C<void *
throw_exception(Interp *interp, PMC *exception, void *dest)>

Throw the exception.

=cut

*/

void *
throw_exception(Interp *interp, PMC *exception, void *dest)
{
    void *address;

    PMC * const handler = find_exception_handler(interp, exception);
    if (!handler)
        return NULL;
    /* put the handler aka continuation ctx in the interpreter */
    address = VTABLE_invoke(interp, handler, exception);
    /* address = VTABLE_get_pointer(interp, handler); */
    if (PObj_get_FLAGS(handler) & SUB_FLAG_C_HANDLER) {
        /* its a C exception handler */
        Parrot_exception *jb = (Parrot_exception *) address;
        longjmp(jb->destination, 1);
    }
    /* return the address of the handler */
    return address;
}

/*

=item C<void *
rethrow_exception(Interp *interp, PMC *exception)>

Rethrow the exception.

=cut

*/

void *
rethrow_exception(Interp *interp, PMC *exception)
{
    PMC *handler;
    void *address;

    if (exception->vtable->base_type != enum_class_Exception)
        PANIC("Illegal rethrow");
    handler = find_exception_handler(interp, exception);
    address = VTABLE_invoke(interp, handler, exception);
    /* return the address of the handler */
    return address;
}

/*

=item C<void
rethrow_c_exception(Interp *interp)>

Return back to runloop, assumes exception is still in C<TODO> and
that this is called from within a handler setup with C<new_c_exception>.

=cut

*/

void
rethrow_c_exception(Interp *interp)
{
    Parrot_exception * const the_exception = interp->exceptions;

    PMC * const exception = NULL;   /* TODO */
    PMC * const handler = find_exception_handler(interp, exception);

    /* XXX we should only peek for the next handler */
    push_exception(interp, handler);
    /*
     * if there was no user handler, interpreter is already shutdown
     */
    the_exception->resume = VTABLE_get_pointer(interp, handler);
    the_exception->error = VTABLE_get_integer_keyed_int(interp,
            exception, 1);
    the_exception->severity = VTABLE_get_integer_keyed_int(interp,
            exception, 2);
    the_exception->msg = VTABLE_get_string_keyed_int(interp, exception, 0);
    longjmp(the_exception->destination, 1);
}

/*

=item C<static size_t
dest2offset(Interp *interp, const opcode_t *dest)>

Translate an absolute bytecode location to an offset used for resuming
after an exception had occurred.

=cut

*/

static size_t
dest2offset(Interp *interp, const opcode_t *dest)
{
    size_t offset;
    /* translate an absolute location in byte_code to an offset
     * used for resuming after an exception had occurred
     */
    switch (interp->run_core) {
        case PARROT_SWITCH_CORE:
        case PARROT_SWITCH_JIT_CORE:
        case PARROT_CGP_CORE:
        case PARROT_CGP_JIT_CORE:
            offset = dest - (const opcode_t *)interp->code->prederef.code;
        default:
            offset = dest - interp->code->base.data;
    }
    return offset;
}

/*

=item C<static opcode_t *
create_exception(Interp *interp)>

Create an exception.

=cut

*/

static opcode_t *
create_exception(Interp *interp)
{
    PMC *exception;     /* exception object */
    opcode_t *dest;     /* absolute address of handler */
    Parrot_exception * const the_exception = interp->exceptions;

    /*
     * if the exception number is in the range of our known exceptions
     * use the precreated exception
     */
    if (the_exception->error <= E_LAST_PYTHON_E &&
            the_exception->error >= 0) {
        exception = interp->exception_list[the_exception->error];
    }
    else {
        /* create an exception object */
        exception = pmc_new(interp, enum_class_Exception);
        /* exception type */
        VTABLE_set_integer_keyed_int(interp, exception, 1,
                the_exception->error);
    }
    /* exception severity */
    VTABLE_set_integer_keyed_int(interp, exception, 2,
            (INTVAL)the_exception->severity);
    if (the_exception->msg) {
        VTABLE_set_string_keyed_int(interp, exception, 0,
                the_exception->msg);
    }
    /* now fill rest of exception, locate handler and get
     * destination of handler
     */
    dest = throw_exception(interp, exception, the_exception->resume);
    return dest;
}

/*

=item C<size_t handle_exception(Interp *interp)>

Handle an exception.

=cut

*/

size_t
handle_exception(Interp *interp)
{
    /* absolute address of handler */
    const opcode_t * const dest = create_exception(interp);

    return dest2offset(interp, dest);
}

/*

=item C<void
new_internal_exception(Interp *interp)>

Create a new internal exception buffer, either by allocating it or by
getting one from the free list.

=cut

*/

void
new_internal_exception(Interp *interp)
{
    Parrot_exception *the_exception;

    if (interp->exc_free_list) {
        the_exception = interp->exc_free_list;
        interp->exc_free_list = the_exception->prev;
    }
    else
        the_exception = mem_sys_allocate(sizeof (*the_exception));
    the_exception->prev = interp->exceptions;
    the_exception->resume = NULL;
    the_exception->msg = NULL;
    interp->exceptions = the_exception;
}

/*

=item C<void
free_internal_exception(Interp *interp)>

Place internal exception buffer back on the free list.

=cut

*/

void
free_internal_exception(Interp *interp)
{
    Parrot_exception * const e = interp->exceptions;
    interp->exceptions = e->prev;
    e->prev = interp->exc_free_list;
    interp->exc_free_list = e;
}

void
destroy_exception_list(Interp *interp)
{
    really_destroy_exception_list(interp->exceptions);
    really_destroy_exception_list(interp->exc_free_list);
}

void
really_destroy_exception_list(Parrot_exception *e)
{
    Parrot_exception *prev;

    while (e != NULL)
    {
        prev = e->prev;
        mem_sys_free(e);
        e    = prev;
    }
}

/*

=item C<void
do_exception(Interp *interp,
        exception_severity severity, long error)>

Called from interrupt code. Does a C<longjmp> in front of the runloop,
which calls C<handle_exception()>, returning the handler address where
execution then resumes.

=cut

*/
void
do_exception(Interp *interp,
        exception_severity severity, long error)
{
    Parrot_exception * const the_exception = interp->exceptions;

    the_exception->error = error;
    the_exception->severity = severity;
    the_exception->msg = NULL;
    the_exception->resume = NULL;
    longjmp(the_exception->destination, 1);
}

/*

=item C<void
real_exception(Interp *interp, void *ret_addr,
        int exitcode, const char *format, ...)>

Throws a real exception, with an error message constructed from the format
string and arguments.  C<ret_addr> is the address from which to resume, if some
handler decides that is appropriate, or zero to make the error non-resumable.
C<exitcode> is a C<exception_type_enum> value.

See also C<internal_exception()>, which signals fatal errors, and
C<throw_exception>, which calls the handler.

=cut

*/

void
real_exception(Interp *interp, void *ret_addr,
        int exitcode,  const char *format, ...)
{
    STRING *msg;
    Parrot_exception * const the_exception = interp->exceptions;

    /*
     * if profiling remember end time of lastop and
     * generate entry for exception
     */
    if (interp->profile &&
            Interp_flags_TEST(interp, PARROT_PROFILE_FLAG)) {
        RunProfile * const profile = interp->profile;
        const FLOATVAL now = Parrot_floatval_time();
        profile->data[profile->cur_op].time += now - profile->starttime;
        profile->cur_op = PARROT_PROF_EXCEPTION;
        profile->starttime = now;
        profile->data[PARROT_PROF_EXCEPTION].numcalls++;
    }

    /*
     * make exception message
     */
    if (strchr(format, '%')) {
        va_list arglist;
        va_start(arglist, format);
        msg = Parrot_vsprintf_c(interp, format, arglist);
        va_end(arglist);
    }
    else
        msg = string_make(interp, format, strlen(format),
                NULL, PObj_external_FLAG);
    /* string_from_cstring(interp, format, strlen(format)); */
    /*
     * FIXME classify errors
     */
    if (!the_exception) {
        PIO_eprintf(interp,
                "real_exception (severity:%d error:%d): %Ss\n"
                "likely reason: argument count mismatch in main "
                "(more than 1 param)\n",
                EXCEPT_error, exitcode, msg);
        /* [what if exitcode is a multiple of 256?] */
        exit(exitcode);
    }
    the_exception->severity = EXCEPT_error;
    the_exception->error = exitcode;
    the_exception->msg = msg;
    the_exception->resume = ret_addr;
    if (Interp_debug_TEST(interp, PARROT_BACKTRACE_DEBUG_FLAG)) {
        PIO_eprintf(interp, "real_exception (severity:%d error:%d): %Ss\n",
            EXCEPT_error, exitcode, msg);
        PDB_backtrace(interp);
    }
    /*
     * reenter runloop
     */
    longjmp(the_exception->destination, 1);
}

/*

=item C<void Parrot_init_exceptions(Interp *interp)>

Create exception objects.

*/

void
Parrot_init_exceptions(Interp *interp) {
    int i;

    interp->exception_list = mem_sys_allocate(
            sizeof (PMC*) * (E_LAST_PYTHON_E + 1));
    for (i = 0; i <= E_LAST_PYTHON_E; ++i) {
        PMC * const ex = pmc_new(interp, enum_class_Exception);
        interp->exception_list[i] = ex;
        VTABLE_set_integer_keyed_int(interp, ex, 1, i);
    }
}

/*

=back

=head1 SEE ALSO

F<include/parrot/exceptions.h>.

=cut

*/


/*
 * Local variables:
 *   c-file-style: "parrot"
 * End:
 * vim: expandtab shiftwidth=4:
 */
