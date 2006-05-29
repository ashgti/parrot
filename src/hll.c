/*
Copyright: 2005 The Perl Foundation.  All Rights Reserved.
$Id$

=head1 NAME

src/hll.c - High Level Language support

=head1 DESCRIPTION

Parrot core PMCs have to create sometimes new PMCs, which should map
to current HLLs defaults. The current language and a typemap provides
this feature.

=head1 DATA

   interpreter->HLL_info

   @HLL_info = [
     [ hll_name, hll_lib, { core_type => HLL_type, ... }, namespace ],
     ...
     ]

=head2 Functions

=over 4

=item C<INTVAL Parrot_register_HLL(Interp*, STRING *hll_name, STRING *hll_lib)>

Register HLL C<hll_name> within Parrot core. If C<hll_lib> isn't a NULL STRING,
load the shared language support library. Returns a type id for this HLL or 0
on error.

=item C<INTVAL Parrot_get_HLL_id(Interp*, STRING *hll_name)>

Return the id of the given HLL name or -1 on error. "parrot" has id 0.

=item C<void Parrot_register_HLL_type(Interp *, INTVAL hll_id,
	INTVAL core_type, INTVAL hll_type)>

Register a type mapping	C<core_type => hll_type> for the given HLL.

=item C<INTVAL Parrot_get_HLL_type(Interp *, INTVAL hll_id, INTVAL core_type)>

Get an equivalent HLL type number for the language C<hll_id> or return
C<core_type> if the HLL doesn't remap the type.

=item C<INTVAL Parrot_get_ctx_HLL_type(Interp *, INTVAL core_type)>

Return an equivalent PMC type number according to the current HLL setings
in the context. If no type is registered just return C<core_type>.

=cut

*/

#include "parrot/parrot.h"
#include "parrot/dynext.h"
#include <assert.h>

enum {
    e_HLL_name,
    e_HLL_lib,
    e_HLL_typemap,
    e_HLL_MAX
} HLL_enum_t;

static STRING*
string_as_const_string(Interp* interpreter, STRING *src)
{
    if (PObj_constant_TEST(src))
        return src;
    assert(0);
    return NULL;
}

INTVAL
Parrot_register_HLL(Interp *interpreter,
	STRING *hll_name, STRING *hll_lib)
{
    PMC *hll_info, *entry, *name, *type_hash, *ns_hash;
    INTVAL idx;

    idx = Parrot_get_HLL_id(interpreter, hll_name);
    if (idx >= 0)
        return idx;
    /* TODO LOCK or disallow in threads */
    hll_info = interpreter->HLL_info;
    idx = VTABLE_elements(interpreter, hll_info);
    /*
     * ATT: all items that are owned by the HLL_info structure
     *      have to be created as constant objects, because
     *      this structure isn't marked by DOD/GC
     */ 
    entry = constant_pmc_new(interpreter, enum_class_FixedPMCArray);
    VTABLE_push_pmc(interpreter, hll_info, entry);

    VTABLE_set_integer_native(interpreter, entry, e_HLL_MAX);

    /* register HLL name */
    name = constant_pmc_new_noinit(interpreter, enum_class_String);
    hll_name = string_as_const_string(interpreter, hll_name);
    VTABLE_set_string_native(interpreter, name, hll_name);
    VTABLE_set_pmc_keyed_int(interpreter, entry, e_HLL_name, name);

    /* create HLL namespace */
    hll_name = string_downcase(interpreter, hll_name);

    /* HLL type mappings aren't yet created, we can't create
     * a namespace in HLL's flavor yet - mabe promote the
     * ns_hash to another type, if mappings provide one
     *
     * XXX always try to fetch namespace first?
     */
    ns_hash  = pmc_new(interpreter, enum_class_NameSpace);
    VTABLE_set_pmc_keyed_str(interpreter, interpreter->root_namespace,
            hll_name, ns_hash);
    /* cache HLLs toplevel namespace */
    VTABLE_set_pmc_keyed_int(interpreter, interpreter->HLL_namespace, 
            idx, ns_hash);

    /* register HLL lib */
    name = constant_pmc_new_noinit(interpreter, enum_class_String);
    if (!hll_lib)
        hll_lib = const_string(interpreter, "");
    hll_lib = string_as_const_string(interpreter, hll_lib);
    VTABLE_set_string_native(interpreter, name, hll_lib);
    VTABLE_set_pmc_keyed_int(interpreter, entry, e_HLL_lib, name);

    /* create HLL typemap hash */
    type_hash = Parrot_new_INTVAL_hash(interpreter, PObj_constant_FLAG);
    VTABLE_set_pmc_keyed_int(interpreter, entry, e_HLL_typemap, type_hash);
    if (string_length(interpreter, hll_lib)) {
        /* load lib */
        Parrot_load_lib(interpreter, hll_lib, NULL);
    }

    /* UNLOCK */

    return idx;
}

INTVAL
Parrot_get_HLL_id(Interp* interpreter, STRING *hll_name)
{
    INTVAL i;

    PMC * const hll_info = interpreter->HLL_info;
    const INTVAL nelements = VTABLE_elements(interpreter, hll_info);

    for (i = 0; i < nelements; ++i) {
        PMC * const entry = VTABLE_get_pmc_keyed_int(interpreter, hll_info, i);
        PMC * const name_pmc = VTABLE_get_pmc_keyed_int(interpreter, entry, e_HLL_name);
        STRING * const name = VTABLE_get_string(interpreter, name_pmc);
        if (!string_equal(interpreter, name, hll_name))
            return i;
    }
    return -1;
}

void
Parrot_register_HLL_type(Interp *interpreter, INTVAL hll_id,
	INTVAL core_type, INTVAL hll_type)
{
    PMC *entry, *type_hash;
    Hash *hash;

    PMC * const hll_info = interpreter->HLL_info;
    const INTVAL n = VTABLE_elements(interpreter, hll_info);
    if (hll_id >= n) {
        real_exception(interpreter, NULL, E_ValueError,
                "no such HLL id (%vd)", hll_id);
    }
    entry = VTABLE_get_pmc_keyed_int(interpreter, hll_info, hll_id);
    assert(!PMC_IS_NULL(entry));
    type_hash = VTABLE_get_pmc_keyed_int(interpreter, entry, e_HLL_typemap);
    assert(!PMC_IS_NULL(type_hash));
    hash = PMC_struct_val(type_hash);
    hash_put(interpreter, hash, (void*)core_type, (void*)hll_type);
}

INTVAL
Parrot_get_HLL_type(Interp *interpreter, INTVAL hll_id, INTVAL core_type)
{
    PMC *entry, *type_hash, *hll_info;
    Hash *hash;
    HashBucket *b;
    INTVAL n;

    if (hll_id < 0) {
        real_exception(interpreter, NULL, E_ValueError,
                "no such HLL id (%vd)", hll_id);
    }
    if (hll_id == 0)
        return core_type;
    hll_info = interpreter->HLL_info;
    n = VTABLE_elements(interpreter, hll_info);
    if (hll_id >= n) {
        real_exception(interpreter, NULL, E_ValueError,
                "no such HLL id (%vd)", hll_id);
    }
    entry = VTABLE_get_pmc_keyed_int(interpreter, hll_info, hll_id);
    type_hash = VTABLE_get_pmc_keyed_int(interpreter, entry, e_HLL_typemap);
    if (PMC_IS_NULL(type_hash))
        return core_type;
    hash = PMC_struct_val(type_hash);
    if (!hash->entries)
        return core_type;
    b = hash_get_bucket(interpreter, hash, (void*)core_type);
    if (b)
        return (INTVAL) b->value;
    return core_type;
}

INTVAL
Parrot_get_ctx_HLL_type(Interp *interpreter, INTVAL core_type)
{
    INTVAL hll_id;

    hll_id = CONTEXT(interpreter->ctx)->current_HLL;
    return Parrot_get_HLL_type(interpreter, hll_id, core_type);
}
/*

=back

=head1 AUTHOR

Leopold Toetsch

=head1 SEE ALSO

=cut

*/

/*
 * Local variables:
 * c-indentation-style: bsd
 * c-basic-offset: 4
 * indent-tabs-mode: nil
 * End:
 *
 * vim: expandtab shiftwidth=4:
*/
