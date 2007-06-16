/*
Copyright (C) 2005-2007, The Perl Foundation.
$Id$

=head1 NAME

src/hll.c - High Level Language support

=head1 DESCRIPTION

Parrot core PMCs have to create sometimes new PMCs, which should map
to current HLLs defaults. The current language and a typemap provides
this feature.

=head1 DATA

   interp->HLL_info

   @HLL_info = [
     [ hll_name, hll_lib, { core_type => HLL_type, ... }, namespace ],
     ...
     ]

=head2 Functions


FUNCDOC: Parrot_register_HLL

Register HLL C<hll_name> within Parrot core.  If C<hll_lib> isn't a NULL
STRING, load the shared language support library.  Creates a root namespace for
the HLL named C<hll_name>.  Returns a type id for this HLL or 0 on error.

If C<hll_name> is NULL, only the library is loaded. This is used from the
C<.loadlib> pragma.

FUNCDOC: Parrot_get_HLL_id

Return the id of the given HLL name or -1 on error. "parrot" has id 0.

FUNCDOC: Parrot_register_HLL_type

Register a type mapping        C<core_type => hll_type> for the given HLL.

FUNCDOC: Parrot_get_HLL_type

Get an equivalent HLL type number for the language C<hll_id>.  If the given HLL
doesn't remap the given type, or if C<hll_id> is the special value
C<PARROT_HLL_NONE>, just return C<core_type> unchanged.

FUNCDOC: Parrot_get_ctx_HLL_type

Return an equivalent PMC type number according to the current HLL setings
in the context.  If no type is registered just return C<core_type>.

*/

#include "parrot/parrot.h"
#include "parrot/dynext.h"
#include <assert.h>

/* HEADER: include/parrot/hll.h */

enum {
    e_HLL_name,
    e_HLL_lib,
    e_HLL_typemap,
    e_HLL_MAX
} HLL_enum_t;

/* for shared HLL data, do COW stuff */
#define START_READ_HLL_INFO(interp, hll_info)
#define END_READ_HLL_INFO(interp, hll_info)
#define START_WRITE_HLL_INFO(interp, hll_info) \
    do { \
        if (PMC_sync((interp)->HLL_info)) { \
            hll_info = (interp)->HLL_info = \
                Parrot_clone((interp), (interp)->HLL_info); \
            if (PMC_sync((interp)->HLL_info)) \
                mem_internal_free(PMC_sync((interp)->HLL_info)); \
        } \
    } while (0)
#define END_WRITE_HLL_INFO(interp, hll_info)


static STRING*
string_as_const_string(Interp* interp, STRING *src)
{
    if (PObj_constant_TEST(src))
        return src;
    assert(0);
    return NULL;
}

static PMC*
new_hll_entry(Interp *interp /*NN*/)
{
    PMC * const hll_info = interp->HLL_info;
    /*
     * ATT: all items that are owned by the HLL_info structure
     *      have to be created as constant objects, because
     *      this structure isn't marked by DOD/GC
     */
    PMC * const entry = constant_pmc_new(interp, enum_class_FixedPMCArray);
    VTABLE_push_pmc(interp, hll_info, entry);

    VTABLE_set_integer_native(interp, entry, e_HLL_MAX);
    return entry;
}

PARROT_API
INTVAL
Parrot_register_HLL(Interp *interp /*NN*/, STRING *hll_name /*NULLOK*/, STRING *hll_lib)
{
    PMC *entry, *name, *type_hash, *ns_hash, *hll_info;
    INTVAL idx;

    /* TODO LOCK or disallow in threads */

    if (!hll_name) {
        /* .loadlib pragma */
        hll_info = interp->HLL_info;
        START_WRITE_HLL_INFO(interp, hll_info);
        entry = new_hll_entry(interp);
        VTABLE_set_pmc_keyed_int(interp, entry, e_HLL_name, PMCNULL);
        /* register  dynlib */
        name = constant_pmc_new_noinit(interp, enum_class_String);
        hll_lib = string_as_const_string(interp, hll_lib);
        VTABLE_set_string_native(interp, name, hll_lib);
        VTABLE_set_pmc_keyed_int(interp, entry, e_HLL_lib, name);
        END_WRITE_HLL_INFO(interp, hll_info);
        return 0;
    }
    idx = Parrot_get_HLL_id(interp, hll_name);
    if (idx >= 0)
        return idx;

    hll_info = interp->HLL_info;
    START_WRITE_HLL_INFO(interp, hll_info);
    idx = VTABLE_elements(interp, hll_info);
    entry = new_hll_entry(interp);
    /* register HLL name */
    name = constant_pmc_new_noinit(interp, enum_class_String);
    hll_name = string_as_const_string(interp, hll_name);
    VTABLE_set_string_native(interp, name, hll_name);
    VTABLE_set_pmc_keyed_int(interp, entry, e_HLL_name, name);

    /* create HLL namespace */
    hll_name = string_downcase(interp, hll_name);

    /* HLL type mappings aren't yet created, we can't create
     * a namespace in HLL's flavor yet - mabe promote the
     * ns_hash to another type, if mappings provide one
     * XXX - FIXME
     */
    ns_hash = Parrot_make_namespace_keyed_str(interp,
                                              interp->root_namespace,
                                              hll_name);

    /* cache HLLs toplevel namespace */
    VTABLE_set_pmc_keyed_int(interp, interp->HLL_namespace,
                             idx, ns_hash);

    /* register HLL lib */
    name = constant_pmc_new_noinit(interp, enum_class_String);
    if (!hll_lib)
        hll_lib = const_string(interp, "");
    hll_lib = string_as_const_string(interp, hll_lib);
    VTABLE_set_string_native(interp, name, hll_lib);
    VTABLE_set_pmc_keyed_int(interp, entry, e_HLL_lib, name);

    /* create HLL typemap hash */
    type_hash = Parrot_new_INTVAL_hash(interp, PObj_constant_FLAG);
    VTABLE_set_pmc_keyed_int(interp, entry, e_HLL_typemap, type_hash);
    if (string_length(interp, hll_lib)) {
        /* load lib */
        Parrot_load_lib(interp, hll_lib, NULL);
    }

    /* UNLOCK */
    END_WRITE_HLL_INFO(interp, hll_info);

    return idx;
}

PARROT_API
INTVAL
Parrot_get_HLL_id(Interp *interp /*NN*/, STRING *hll_name /*NULLOK*/)
    /* WARN_UNUSED */
{
    INTVAL i;

    PMC * const hll_info = interp->HLL_info;
    INTVAL nelements;

    START_READ_HLL_INFO(interp, hll_info);

    nelements = VTABLE_elements(interp, hll_info);

    for (i = 0; i < nelements; ++i) {
        PMC * const entry = VTABLE_get_pmc_keyed_int(interp, hll_info, i);
        PMC * const name_pmc = VTABLE_get_pmc_keyed_int(interp, entry,
                e_HLL_name);
        STRING * name;

        if (PMC_IS_NULL(name_pmc))
            continue;
        name = VTABLE_get_string(interp, name_pmc);
        if (!string_equal(interp, name, hll_name))
            break;
    }
    END_READ_HLL_INFO(interp, hll_info);

    return i < nelements ? i : -1;
}

PARROT_API
STRING *
Parrot_get_HLL_name(Interp *interp /*NN*/, INTVAL id)
{
    PMC * const hll_info = interp->HLL_info;
    STRING *ret;

    const INTVAL nelements = VTABLE_elements(interp, hll_info);

    if (id < 0 || id >= nelements) {
        ret = NULL;
    }
    else {
        PMC *entry;
        PMC *name_pmc;
        START_READ_HLL_INFO(interp, hll_info);
        entry = VTABLE_get_pmc_keyed_int(interp, hll_info, id);
        name_pmc = VTABLE_get_pmc_keyed_int(interp, entry, e_HLL_name);
        if (PMC_IS_NULL(name_pmc)) /* loadlib-created 'HLL's are nameless */
            ret = NULL;
        else
            ret = VTABLE_get_string(interp, name_pmc);
        END_READ_HLL_INFO(interp, hll_info);
    }

    return ret;
}

PARROT_API
void
Parrot_register_HLL_type(Interp *interp /*NN*/, INTVAL hll_id,
        INTVAL core_type, INTVAL hll_type)
{
    PMC *entry, *type_hash;
    Hash *hash;

    PMC *hll_info = interp->HLL_info;
    const INTVAL n = VTABLE_elements(interp, hll_info);
    if (hll_id >= n) {
        real_exception(interp, NULL, E_ValueError,
                "no such HLL id (%vd)", hll_id);
    }

    if (PMC_sync(hll_info)) {
        /* the type might already be registered in a non-conflicting way,
         * in which case we can avoid copying
         */
        if (hll_type == Parrot_get_HLL_type(interp, hll_id, core_type))
            return;
    }
    START_WRITE_HLL_INFO(interp, hll_info);
    entry = VTABLE_get_pmc_keyed_int(interp, hll_info, hll_id);
    assert(!PMC_IS_NULL(entry));
    type_hash = VTABLE_get_pmc_keyed_int(interp, entry, e_HLL_typemap);
    assert(!PMC_IS_NULL(type_hash));
    hash = (Hash *)PMC_struct_val(type_hash);
    parrot_hash_put(interp, hash, (void*)core_type, (void*)hll_type);
    END_WRITE_HLL_INFO(interp, hll_info);
}

PARROT_API
INTVAL
Parrot_get_HLL_type(Interp *interp /*NN*/, INTVAL hll_id, INTVAL core_type)
{
    PMC *entry, *type_hash, *hll_info;
    Hash *hash;
    HashBucket *b;
    INTVAL n;

    if (hll_id == PARROT_HLL_NONE || hll_id == 0)
        return core_type;
    if (hll_id < 0)
        real_exception(interp, NULL, E_ValueError,
                "no such HLL id (%vd)", hll_id);

    hll_info = interp->HLL_info;
    n = VTABLE_elements(interp, hll_info);
    if (hll_id >= n) {
        real_exception(interp, NULL, E_ValueError,
                "no such HLL id (%vd)", hll_id);
    }
    START_READ_HLL_INFO(interp, hll_info);
    entry = VTABLE_get_pmc_keyed_int(interp, hll_info, hll_id);
    END_READ_HLL_INFO(interp, hll_info);
    type_hash = VTABLE_get_pmc_keyed_int(interp, entry, e_HLL_typemap);
    if (PMC_IS_NULL(type_hash))
        return core_type;
    hash = (Hash *)PMC_struct_val(type_hash);
    if (!hash->entries)
        return core_type;
    b = parrot_hash_get_bucket(interp, hash, (void*)core_type);
    if (b)
        return (INTVAL) b->value;
    return core_type;
}

PARROT_API
INTVAL
Parrot_get_ctx_HLL_type(Interp *interp /*NN*/, INTVAL core_type)
{
    const INTVAL hll_id = CONTEXT(interp->ctx)->current_HLL;

    return Parrot_get_HLL_type(interp, hll_id, core_type);
}

/*

FUNCDOC: Parrot_get_ctx_HLL_namespace

Return root namespace of the current HLL.

*/

PARROT_API
PMC*
Parrot_get_ctx_HLL_namespace(Interp *interp /*NN*/)
{
    const parrot_context_t * const ctx = CONTEXT(interp->ctx);

    return Parrot_get_HLL_namespace(interp, ctx->current_HLL);
}

/*

FUNCDOC: Parrot_get_HLL_namespace

Return root namespace of the HLL with the id of I<hll_id>.  If C<hll_id> is the
special value C<PARROT_HLL_NONE>, return the global root namespace.

*/

PARROT_API
PMC*
Parrot_get_HLL_namespace(Interp *interp /*NN*/, int hll_id)
{
    PMC *ns_hash;

    if (hll_id == PARROT_HLL_NONE)
        return interp->root_namespace;

    ns_hash = PMCNULL;
    if (PMC_int_val(interp->HLL_namespace) >= hll_id) {
        ns_hash = VTABLE_get_pmc_keyed_int(interp,
                                interp->HLL_namespace,
                                hll_id);
    }
    return ns_hash;
}

/*

FUNCDOC: Parrot_regenerate_HLL_namespaces

Create all HLL namespaces that don't already exist. This is necessary
when creating a new interpreter which is sharing an old interpreter's
HLL_info.

*/

PARROT_API
void
Parrot_regenerate_HLL_namespaces(Interp *interp /*NN*/)
{
    const INTVAL n = VTABLE_elements(interp, interp->HLL_info);
    INTVAL hll_id;

    /* start at one since the 'parrot' namespace should already have
     * been created */
    for (hll_id = 1; hll_id < n; ++hll_id) {
        PMC *ns_hash =
            VTABLE_get_pmc_keyed_int(interp, interp->HLL_namespace, hll_id);
        if (PMC_IS_NULL(ns_hash) ||
            ns_hash->vtable->base_type == enum_class_Undef)
        {
            STRING *hll_name;
            hll_name = Parrot_get_HLL_name(interp, hll_id);
            if (!hll_name)
                continue;
            hll_name = string_downcase(interp, hll_name);
            /* XXX as in Parrot_register_HLL() this needs to be fixed
             * to use the correct type of namespace. Its relatively
             * easy to do that here since the typemap already exists,
             * but it is not currently done for consistency.
             */
            ns_hash = Parrot_make_namespace_keyed_str(interp,
                interp->root_namespace, hll_name);
            VTABLE_set_pmc_keyed_int(interp, interp->HLL_namespace,
                                     hll_id, ns_hash);
        }
    }
}
/*

=head1 AUTHOR

Leopold Toetsch

*/


/*
 * Local variables:
 *   c-file-style: "parrot"
 * End:
 * vim: expandtab shiftwidth=4:
 */
