/*
Copyright (C) 2004-2007, The Perl Foundation.
$Id$

=head1 NAME

src/global.c - Access to global PMCs

=head1 DESCRIPTION

=head2 Functions

=over 4

=cut

*/

#include "parrot/parrot.h"
#include "global.str"

/* HEADERIZER HFILE: include/parrot/global.h */
/* HEADERIZER BEGIN: static */
/* Don't modify between HEADERIZER BEGIN / HEADERIZER END.  Your changes will be lost. */

PARROT_WARN_UNUSED_RESULT
PARROT_CAN_RETURN_NULL
static PMC * get_namespace_pmc(PARROT_INTERP, ARGIN(PMC *sub))
        __attribute__nonnull__(1)
        __attribute__nonnull__(2);

PARROT_WARN_UNUSED_RESULT
PARROT_CAN_RETURN_NULL
static PMC * internal_ns_keyed(PARROT_INTERP,
    ARGIN(PMC *base_ns),
    ARGIN_NULLOK(PMC *pmc_key),
    ARGIN_NULLOK(STRING *str_key),
    int flags)
        __attribute__nonnull__(1)
        __attribute__nonnull__(2);

static void store_sub_in_multi(PARROT_INTERP,
    ARGIN(PMC *sub),
    ARGIN(PMC *ns))
        __attribute__nonnull__(1)
        __attribute__nonnull__(2)
        __attribute__nonnull__(3);

/* Don't modify between HEADERIZER BEGIN / HEADERIZER END.  Your changes will be lost. */
/* HEADERIZER END: static */

#define DEBUG_GLOBAL 0

/* flags for internal_ns_keyed */
#define INTERN_NS_CREAT 1       /* I'm a fan of the classics */

/*

=item C<static PMC * internal_ns_keyed>

internal_ns_keyed: Internal function to do keyed namespace lookup
relative to a given namespace PMC.  Understands STRINGs, String PMCs,
Key pmcs, and array PMCs containing strings.

=cut

*/

PARROT_WARN_UNUSED_RESULT
PARROT_CAN_RETURN_NULL
static PMC *
internal_ns_keyed(PARROT_INTERP, ARGIN(PMC *base_ns), ARGIN_NULLOK(PMC *pmc_key),
        ARGIN_NULLOK(STRING *str_key), int flags)
{
    PMC *ns, *sub_ns;
    INTVAL i, n;
    static const INTVAL max_intval = (INTVAL)((~(UINTVAL)0) >> 1); /*2s comp*/

    ns = base_ns;

    if (str_key)
        n = 1;
    else if (pmc_key->vtable->base_type == enum_class_String) {
        str_key = VTABLE_get_string(interp, pmc_key);
        n = 1;
    }
    else if (pmc_key->vtable->base_type == enum_class_Key)
        n = max_intval;         /* we don't yet know how big the key is */
    else
        n = VTABLE_elements(interp, pmc_key); /* array of strings */

    for (i = 0; i < n; ++i) {
        STRING *part;

        if (str_key)
            part = str_key;
        else if (n == max_intval) {
            if (!pmc_key) {
                real_exception(interp, NULL, 1,
                        "Passing a NULL pmc_key into key_string()");
            }
            part = key_string(interp, pmc_key);
            pmc_key = key_next(interp, pmc_key);
            if (! pmc_key)
                n = i + 1;      /* now we know how big the key is */
        }
        else {
            if (!pmc_key) {
                real_exception(interp, NULL, 1,
                        "Passed a NULL pmc_key into VTABLE_get_string_keyed_int");
            }
            part = VTABLE_get_string_keyed_int(interp, pmc_key, i);
        }

        sub_ns = VTABLE_get_pmc_keyed_str(interp, ns, part);

        if (PMC_IS_NULL(sub_ns)
            /* RT#46157 - stop depending on typed namespace */
            || sub_ns->vtable->base_type != enum_class_NameSpace)
        {
            if (!(flags & INTERN_NS_CREAT))
                return PMCNULL;

            /* RT#46159 - match HLL of enclosing namespace? */
            sub_ns = pmc_new(interp,
                             Parrot_get_ctx_HLL_type(interp,
                                                     enum_class_NameSpace));
            if (PMC_IS_NULL(sub_ns))
                return PMCNULL;
            VTABLE_set_pmc_keyed_str(interp, ns, part, sub_ns);
        }
        ns = sub_ns;
    } /* for */

    return ns;
}

/*

=item C<PMC * Parrot_get_namespace_keyed>

Find the namespace relative to the namespace C<base_ns> with the key
C<pmc_key>, which may be a String, a Key, or an array of strings.  Return
the namespace, or NULL if not found.

=cut

*/

PARROT_API
PARROT_WARN_UNUSED_RESULT
PARROT_CAN_RETURN_NULL
PMC *
Parrot_get_namespace_keyed(PARROT_INTERP, ARGIN(PMC *base_ns), ARGIN_NULLOK(PMC *pmc_key))
{
    return internal_ns_keyed(interp, base_ns, pmc_key, NULL, 0);
}

/*

=item C<PMC * Parrot_get_namespace_keyed_str>

Find the namespace relative to the namespace C<base_ns> with the string key
C<str_key>.  Return the namespace, or NULL if not found.

=cut

*/

PARROT_API
PARROT_WARN_UNUSED_RESULT
PARROT_CAN_RETURN_NULL
PMC *
Parrot_get_namespace_keyed_str(PARROT_INTERP, ARGIN(PMC *base_ns),
        ARGIN_NULLOK(STRING *str_key))
{
    return internal_ns_keyed(interp, base_ns, PMCNULL, str_key, 0);
}

/*

=item C<PMC * Parrot_make_namespace_keyed>

Find, or create if necessary, the namespace relative to the namespace
C<base_ns> with the key C<pmc_key>, which may be a String, a Key, or an
array of strings.  Return the namespace.  Errors will result in exceptions.

=cut

*/

PARROT_API
PARROT_WARN_UNUSED_RESULT
PARROT_CAN_RETURN_NULL
PMC *
Parrot_make_namespace_keyed(PARROT_INTERP, ARGIN(PMC *base_ns),
        ARGIN_NULLOK(PMC *pmc_key))
{
    return internal_ns_keyed(interp, base_ns, pmc_key, NULL, INTERN_NS_CREAT);
}

/*

=item C<PMC * Parrot_make_namespace_keyed_str>

Find, or create if necessary, the namespace relative to the namespace
C<base_ns> with the string key C<str_key>.  Return the namespace.  Errors
will result in exceptions.

=cut

*/

PARROT_API
PARROT_WARN_UNUSED_RESULT
PARROT_CAN_RETURN_NULL
PMC *
Parrot_make_namespace_keyed_str(PARROT_INTERP, ARGIN(PMC *base_ns),
        ARGIN_NULLOK(STRING *str_key))
{
    return internal_ns_keyed(interp, base_ns, NULL, str_key, INTERN_NS_CREAT);
}


/*

=item C<PMC * Parrot_make_namespace_autobase>

Find, or create if necessary, a namespace with the key C<key>, which may be a
String, a Key, or an array of strings. If it is a String, then the lookup is
relative to the current namespace. Otherwise, it is relative to the current HLL
root namespace. Return the namespace.  Errors will result in exceptions.

=cut

*/

PARROT_API
PARROT_WARN_UNUSED_RESULT
PARROT_CAN_RETURN_NULL
PMC *
Parrot_make_namespace_autobase(PARROT_INTERP, ARGIN_NULLOK(PMC *key))
{
    PMC *base_ns;
    if (VTABLE_isa(interp, key, CONST_STRING(interp, "String")))
        base_ns = CONTEXT(interp)->current_namespace;
    else
        base_ns = VTABLE_get_pmc_keyed_int(interp, interp->HLL_namespace,
            CONTEXT(interp)->current_HLL);
    return Parrot_make_namespace_keyed(interp, base_ns, key);
}

/*

=item C<PMC * Parrot_get_namespace_autobase>

Find a namespace with the key C<key>, which may be a String, a Key, or an
array of strings. If it is a String, then the lookup is relative to the
current namespace. Otherwise, it is relative to the current HLL root
namespace. Return the namespace, or NULL if not found.

=cut

*/

PARROT_API
PARROT_WARN_UNUSED_RESULT
PARROT_CAN_RETURN_NULL
PMC *
Parrot_get_namespace_autobase(PARROT_INTERP, ARGIN_NULLOK(PMC *key))
{
    PMC *base_ns;
    if (VTABLE_isa(interp, key, CONST_STRING(interp, "String")))
        base_ns = CONTEXT(interp)->current_namespace;
    else
        base_ns = VTABLE_get_pmc_keyed_int(interp, interp->HLL_namespace,
            CONTEXT(interp)->current_HLL);
    return Parrot_get_namespace_keyed(interp, base_ns, key);
}

/*

=item C<PMC * Parrot_ns_get_name>

Retrieve an array of names from a namespace object.

=cut

*/

PARROT_API
PARROT_WARN_UNUSED_RESULT
PARROT_CAN_RETURN_NULL
PMC *
Parrot_ns_get_name(PARROT_INTERP, ARGIN(PMC *_namespace))
{
    PMC *names;
    Parrot_PCCINVOKE(interp, _namespace,
            CONST_STRING(interp, "get_name"), "->P", &names);
    return names;
}

/*

=item C<PMC * Parrot_get_global>

Parrot_get_global allows a null namespace without throwing an exception; it
simply returns PMCNULL in that case.

NOTE: At present the use of the {get, set}_global functions is mandatory due to the
      wacky namespace typing of the default Parrot namespace.  Eventually it will be
      safe to just use the standard hash interface (if desired).

Look up the global named C<globalname> in the namespace C<ns>.  Return the
global, or return PMCNULL if C<ns> is null or if the global is not found.

KLUDGE ALERT: Currently prefers non-namespaces in case of collision.

=cut

*/


/*
 * {get, set}_global.
 *
 * Parrot_get_global allows a null namespace without throwing an exception; it
 * simply returns PMCNULL in that case.
 *
 * NOTE: At present the use of the {get, set}_global functions is mandatory due to the
 *       wacky namespace typing of the default Parrot namespace.  Eventually it will be
 *       safe to just use the standard hash interface (if desired).
 */

PARROT_API
PARROT_WARN_UNUSED_RESULT
PARROT_CAN_RETURN_NULL
PMC *
Parrot_get_global(PARROT_INTERP, ARGIN_NULLOK(PMC *ns), ARGIN_NULLOK(STRING *globalname))
{
    if (PMC_IS_NULL(ns))
        return PMCNULL;

    return (PMC *)VTABLE_get_pointer_keyed_str(interp, ns, globalname);
}

/*

=item C<void Parrot_set_global>

Set the global named C<globalname> in the namespace C<ns> to the value C<val>.

=cut

*/

PARROT_API
void
Parrot_set_global(PARROT_INTERP, ARGIN_NULLOK(PMC *ns),
        ARGIN_NULLOK(STRING *globalname), ARGIN_NULLOK(PMC *val))
{
    VTABLE_set_pmc_keyed_str(interp, ns, globalname, val);
}


/*

=item C<PMC * Parrot_find_global_n>

Search the namespace PMC C<ns> for an object with name C<globalname>.
Return the object, or NULL if not found.

RT#46161 - For now this function prefers non-namespaces, it will eventually
entirely use the untyped interface.

=cut

*/

PARROT_API
PARROT_WARN_UNUSED_RESULT
PARROT_CAN_RETURN_NULL
PMC *
Parrot_find_global_n(PARROT_INTERP, ARGIN_NULLOK(PMC *ns), ARGIN_NULLOK(STRING *globalname))
{
    PMC *res;

#if DEBUG_GLOBAL
    if (globalname)
        PIO_printf(interp, "find_global name '%Ss'\n", globalname);
#endif

    if (PMC_IS_NULL(ns))
        res = PMCNULL;
    else {
        /*
         * RT#46163 - we should be able to use 'get_pmc_keyed' here,
         * but we can't because Parrot's default namespaces are not
         * fully typed and there's a pseudo-typed interface that
         * distinguishes 'get_pmc_keyed' from 'get_pointer_keyed';
         * the former is for NS and the latter is for non-NS.
         */
        res = (PMC *)VTABLE_get_pointer_keyed_str(interp, ns, globalname);
    }

    return PMC_IS_NULL(res) ? NULL : res;
}

/*

=item C<PMC * Parrot_find_global_cur>

RT#48260: Not yet documented!!!

=cut

*/

PARROT_API
PARROT_WARN_UNUSED_RESULT
PARROT_CAN_RETURN_NULL
PMC *
Parrot_find_global_cur(PARROT_INTERP, ARGIN_NULLOK(STRING *globalname))
{
    PMC * const ns = CONTEXT(interp)->current_namespace;
    return Parrot_find_global_n(interp, ns, globalname);
}

/*

=item C<PMC * Parrot_find_global_k>

Search the namespace designated by C<pmc_key>, which may be a key PMC,
an array of namespace name strings, or a string PMC, for an object
with name C<globalname>.  Return the object, or NULL if not found.

RT#46161 - For now this function prefers non-namespaces, it will eventually
entirely use the untyped interface.

=cut

*/

PARROT_API
PARROT_WARN_UNUSED_RESULT
PARROT_CAN_RETURN_NULL
PMC *
Parrot_find_global_k(PARROT_INTERP, ARGIN_NULLOK(PMC *pmc_key), ARGIN(STRING *globalname))
{
    PMC * const ns =
        Parrot_get_namespace_keyed(interp,
                                   Parrot_get_ctx_HLL_namespace(interp),
                                   pmc_key);
    return Parrot_find_global_n(interp, ns, globalname);
}

/*

=item C<PMC * Parrot_find_global_s>

Search the namespace designated by C<str_key>, or the HLL root if
C<str_key> is NULL, for an object with name C<globalname>.  Return the
object, or NULL if not found.

RT#46161 - For now this function prefers non-namespaces, it will eventually
entirely use the untyped interface.

=cut

*/

PARROT_API
PARROT_WARN_UNUSED_RESULT
PARROT_CAN_RETURN_NULL
PMC *
Parrot_find_global_s(PARROT_INTERP, ARGIN_NULLOK(STRING *str_key),
        ARGIN_NULLOK(STRING *globalname))
{
    PMC *const ns =
        Parrot_get_namespace_keyed_str(interp,
                                       Parrot_get_ctx_HLL_namespace(interp),
                                       str_key);
    return Parrot_find_global_n(interp, ns, globalname);
}

/*

=item C<void Parrot_store_global_n>

Store the PMC C<val> into the namespace PMC C<ns> with name C<globalname>.

=cut

*/

PARROT_API
void
Parrot_store_global_n(PARROT_INTERP, ARGIN_NULLOK(PMC *ns),
        ARGIN_NULLOK(STRING *globalname), ARGIN_NULLOK(PMC *val))
{
#if DEBUG_GLOBAL
    if (globalname)
        PIO_printf(interp, "store_global name '%Ss'\n", globalname);
#endif

    if (PMC_IS_NULL(ns))
        return;

    VTABLE_set_pmc_keyed_str(interp, ns, globalname, val);
}

/*

=item C<void Parrot_store_global_cur>

RT#48260: Not yet documented!!!

=cut

*/

PARROT_API
void
Parrot_store_global_cur(PARROT_INTERP, ARGIN_NULLOK(STRING *globalname),
        ARGIN_NULLOK(PMC *val))
{
    Parrot_store_global_n(interp,
                          CONTEXT(interp)->current_namespace,
                          globalname, val);

    /* RT#46165 - method cache invalidation should occur */
}

/*

=item C<void Parrot_store_global_k>

Store the PMC C<val> into the namespace designated by C<pmc_key>,
which may be a key PMC, an array of namespace name strings, or a
string PMC, with name C<globalname>.

RT#46161 - For now this function prefers non-namespaces, it will eventually
entirely use the untyped interface.

=cut

*/

PARROT_API
void
Parrot_store_global_k(PARROT_INTERP, ARGIN(PMC *pmc_key),
        ARGIN_NULLOK(STRING *globalname), ARGIN_NULLOK(PMC *val))
{
    PMC *ns;

    /*
     * RT#46167 - temporary hack to notice when key is actually a string, so that
     * the legacy logic for invalidating method cache will be called; this is
     * not good enough but it avoids regressesions for now
     */
    if (pmc_key->vtable->base_type == enum_class_String) {
        Parrot_store_global_s(interp, PMC_str_val(pmc_key),
                              globalname, val);
        return;
    }

    ns = Parrot_make_namespace_keyed(interp,
                                     Parrot_get_ctx_HLL_namespace(interp),
                                     pmc_key);

    Parrot_store_global_n(interp, ns, globalname, val);

    /* RT#46165 - method cache invalidation should occur */
}

/*

=item C<void Parrot_store_global_s>

Store the PMC C<val> into the namespace designated by C<str_key>, or
the HLL root if C<str_key> is NULL, with the name C<globalname>.

=cut

*/

PARROT_API
void
Parrot_store_global_s(PARROT_INTERP, ARGIN_NULLOK(STRING *str_key),
        ARGIN_NULLOK(STRING *globalname), ARGIN_NULLOK(PMC *val))
{
    PMC * const ns = Parrot_make_namespace_keyed_str(interp,
                                         Parrot_get_ctx_HLL_namespace(interp),
                                         str_key);

    Parrot_store_global_n(interp, ns, globalname, val);

    /* RT#46169 - method cache invalidation should be a namespace function */
    Parrot_invalidate_method_cache(interp, str_key, globalname);
}


/*

=item C<PMC * Parrot_find_global_op>

If the global exists in the given namespace PMC, return it.  If not, return
PMCNULL.

=cut

*/

PARROT_API
PARROT_WARN_UNUSED_RESULT
PARROT_CANNOT_RETURN_NULL
PMC *
Parrot_find_global_op(PARROT_INTERP, ARGIN(PMC *ns),
        ARGIN_NULLOK(STRING *globalname), ARGIN_NULLOK(void *next))
{
    PMC *res;

    if (!globalname)
        real_exception(interp, next, E_NameError,
                       "Tried to get null global");

    res = Parrot_find_global_n(interp, ns, globalname);
    if (!res)
        res = PMCNULL;

    return res;
}


/*

=item C<PMC * Parrot_find_name_op>

RT#46171 - THIS IS BROKEN - it doesn't walk up the scopes yet

Find the given C<name> in lexicals, then the current namespace, then the HLL
root namespace, and finally Parrot builtins.  If the name isn't found
anywhere, return PMCNULL.

=cut

*/

PARROT_API
PARROT_WARN_UNUSED_RESULT
PARROT_CAN_RETURN_NULL
PMC *
Parrot_find_name_op(PARROT_INTERP, ARGIN(STRING *name), SHIM(void *next))
{
    parrot_context_t * const ctx = CONTEXT(interp);
    PMC * const lex_pad = Parrot_find_pad(interp, name, ctx);
    PMC *g;

    if (PMC_IS_NULL(lex_pad))
        g = PMCNULL;
    else
        g = VTABLE_get_pmc_keyed_str(interp, lex_pad, name);

    /* RT#46171 - walk up the scopes!  duh!! */

    if (PMC_IS_NULL(g)) {
        g = Parrot_find_global_cur(interp, name);

        if (PMC_IS_NULL(g)) {
            g = Parrot_find_global_n(interp,
                    Parrot_get_ctx_HLL_namespace(interp), name);

            if (PMC_IS_NULL(g)) {
                g = Parrot_find_builtin(interp, name);
            }
        }
    }

    if (PMC_IS_NULL(g))
        return PMCNULL;
    else
        return g;
}

/*

=item C<static PMC * get_namespace_pmc>

RT#48260: Not yet documented!!!

=cut

*/

PARROT_WARN_UNUSED_RESULT
PARROT_CAN_RETURN_NULL
static PMC *
get_namespace_pmc(PARROT_INTERP, ARGIN(PMC *sub))
{
    PMC * const nsname = PMC_sub(sub)->namespace_name;
    PMC * const nsroot = Parrot_get_HLL_namespace(interp, PMC_sub(sub)->HLL_id);

    /* If we have a NULL, return the HLL namespace */
    if (PMC_IS_NULL(nsname))
        return nsroot;
    /* If we have a String, do a string lookup */
    else if (nsname->vtable->base_type == enum_class_String)
        return Parrot_make_namespace_keyed_str(interp, nsroot, PMC_str_val(nsname));
    /* Otherwise, do a PMC lookup */
    else
        return Parrot_make_namespace_keyed(interp, nsroot, nsname);
}

/*

=item C<static void store_sub_in_multi>

RT#48260: Not yet documented!!!

=cut

*/

static void
store_sub_in_multi(PARROT_INTERP, ARGIN(PMC *sub), ARGIN(PMC *ns))
{
    INTVAL func_nr;
    char   *c_meth;
    STRING * const subname = PMC_sub(sub)->name;
    PMC    *multisub = VTABLE_get_pmc_keyed_str(interp, ns, subname);

    /* is there an existing MultiSub PMC? or do we need to create one? */
    if (PMC_IS_NULL(multisub)) {
        multisub = pmc_new(interp, enum_class_MultiSub);
        /* we have to push the sub onto the MultiSub before we try to store
        it because storing requires information from the sub */
        VTABLE_push_pmc(interp, multisub, sub);
        VTABLE_set_pmc_keyed_str(interp, ns, subname, multisub);
    }
    else
        VTABLE_push_pmc(interp, multisub, sub);

    c_meth  = string_to_cstring(interp, subname);
    func_nr = Parrot_MMD_method_idx(interp, c_meth);
    if (func_nr >= 0)
        Parrot_mmd_rebuild_table(interp, -1, func_nr);
    string_cstring_free(c_meth);
}

/*

=item C<void Parrot_store_sub_in_namespace>

RT#48260: Not yet documented!!!

=cut

*/

PARROT_API
void
Parrot_store_sub_in_namespace(PARROT_INTERP, ARGIN(PMC *sub))
{
    const INTVAL cur_id = CONTEXT(interp)->current_HLL;

    PMC *ns;
    /* PF structures aren't fully constructed yet */
    Parrot_block_DOD(interp);
    /* store relative to HLL namespace */
    CONTEXT(interp)->current_HLL = PMC_sub(sub)->HLL_id;

    ns = get_namespace_pmc(interp, sub);

    /* attach a namespace to the sub for lookups */
    PMC_sub(sub)->namespace_stash = ns;

    /* store a :multi sub */
    if (!PMC_IS_NULL(PMC_sub(sub)->multi_signature))
        store_sub_in_multi(interp, sub, ns);
    /* store other subs (as long as they're not :anon) */
    else if (!(PObj_get_FLAGS(sub) & SUB_FLAG_PF_ANON)) {
        STRING * const name   = PMC_sub(sub)->name;
        PMC    * const nsname = PMC_sub(sub)->namespace_name;

        Parrot_store_global_n(interp, ns, name, sub);

        /* TEMPORARY HACK - cache invalidation should be a namespace function */
        if (!PMC_IS_NULL(nsname)) {
            STRING * const nsname_s = VTABLE_get_string(interp, nsname);
            Parrot_invalidate_method_cache(interp, nsname_s, name);
        }
    }

    /* restore HLL_id */
    CONTEXT(interp)->current_HLL = cur_id;
    Parrot_unblock_DOD(interp);
}

/*

=back

=head1 SEE ALSO

F<include/parrot/global.h>

=cut

*/


/*
 * Local variables:
 *   c-file-style: "parrot"
 * End:
 * vim: expandtab shiftwidth=4:
 */
