/*
Copyright (C) 2007-2008, The Perl Foundation.
$Id$

=head1 NAME

oo.c - Class and object

=head1 DESCRIPTION

Handles class and object manipulation.

=head2 Functions

=over 4

=cut

*/

#define PARROT_IN_OO_C
#define PARROT_IN_OBJECTS_C /* To get the vtable.h imports we want. */
#include "parrot/parrot.h"
#include "parrot/oo_private.h"
#include "pmc/pmc_class.h"

#include "oo.str"

/* HEADERIZER HFILE: include/parrot/oo.h */

/* HEADERIZER BEGIN: static */
/* Don't modify between HEADERIZER BEGIN / HEADERIZER END.  Your changes will be lost. */

PARROT_WARN_UNUSED_RESULT
PARROT_CAN_RETURN_NULL
static PMC* C3_merge(PARROT_INTERP, ARGIN(PMC *merge_list))
        __attribute__nonnull__(1)
        __attribute__nonnull__(2);

static void debug_trace_find_meth(PARROT_INTERP,
    ARGIN(const PMC *_class),
    ARGIN(const STRING *name),
    ARGIN_NULLOK(const PMC *sub))
        __attribute__nonnull__(1)
        __attribute__nonnull__(2)
        __attribute__nonnull__(3);

static void fail_if_type_exists(PARROT_INTERP, ARGIN(PMC *name))
        __attribute__nonnull__(1)
        __attribute__nonnull__(2);

PARROT_WARN_UNUSED_RESULT
PARROT_CAN_RETURN_NULL
static PMC * find_method_direct_1(PARROT_INTERP,
    ARGIN(PMC *_class),
    ARGIN(STRING *method_name))
        __attribute__nonnull__(1)
        __attribute__nonnull__(2)
        __attribute__nonnull__(3);

static void invalidate_all_caches(PARROT_INTERP)
        __attribute__nonnull__(1);

static void invalidate_type_caches(PARROT_INTERP, UINTVAL type)
        __attribute__nonnull__(1);

/* Don't modify between HEADERIZER BEGIN / HEADERIZER END.  Your changes will be lost. */
/* HEADERIZER END: static */

/*

=item C<void Parrot_oo_extract_methods_from_namespace>

Extract methods and vtable overrides from the given namespace and insert them
into the class.

=cut

*/

void
Parrot_oo_extract_methods_from_namespace(PARROT_INTERP, ARGIN(PMC *self), ARGIN(PMC *ns))
{
   PMC *methods, *vtable_overrides;

    /* Pull in methods from the namespace, if any. */
   if (PMC_IS_NULL(ns))
       return;

    /* Import any methods. */
    Parrot_PCCINVOKE(interp, ns,
        CONST_STRING(interp, "get_associated_methods"), "->P", &methods);

    if (!PMC_IS_NULL(methods)) {
        PMC * const iter = VTABLE_get_iter(interp, methods);

        while (VTABLE_get_bool(interp, iter)) {
            STRING * const meth_name = VTABLE_shift_string(interp, iter);
            PMC    * const meth_sub  = VTABLE_get_pmc_keyed_str(interp, methods,
                meth_name);
            VTABLE_add_method(interp, self, meth_name, meth_sub);
        }
    }

    /* Import any vtable methods. */
    Parrot_PCCINVOKE(interp, ns,
        CONST_STRING(interp, "get_associated_vtable_methods"), "->P", &vtable_overrides);

    if (!PMC_IS_NULL(vtable_overrides)) {
        PMC * const iter = VTABLE_get_iter(interp, vtable_overrides);
        while (VTABLE_get_bool(interp, iter)) {
            STRING * const vtable_index_str = VTABLE_shift_string(interp, iter);
            PMC    * const vtable_sub       = VTABLE_get_pmc_keyed_str(interp,
                vtable_overrides, vtable_index_str);

            /* Look up the name of the vtable function from the index. */
            const INTVAL vtable_index = string_to_int(interp, vtable_index_str);
            const char * const meth_c = Parrot_vtable_slot_names[vtable_index];
            STRING     *vtable_name   = string_from_cstring(interp, meth_c, 0);

            /* Strip leading underscores in the vtable name */
            if (string_str_index(interp, vtable_name,
                CONST_STRING(interp, "__"), 0) == 0) {
                vtable_name = string_substr(interp, vtable_name, 2,
                    string_length(interp, vtable_name) - 2, NULL, 0);
            }

            VTABLE_add_vtable_override(interp, self, vtable_name, vtable_sub);
        }
    }
}


/*

=item C<PMC * Parrot_oo_get_namespace>

Lookup a namespace object from a class PMC.

=cut

*/

PARROT_CAN_RETURN_NULL
PARROT_WARN_UNUSED_RESULT
PMC *
Parrot_oo_get_namespace(SHIM_INTERP, ARGIN(const PMC *classobj))
{
    Parrot_Class_attributes * const _class     = PARROT_CLASS(classobj);
    PMC          * const _namespace = _class->_namespace;

    if (PMC_IS_NULL(_namespace))
        return PMCNULL;

    return _namespace;
}


/*

=item C<PMC * Parrot_oo_get_class>

Lookup a class object from a namespace, string, or key PMC.

=cut

*/

PARROT_API
PARROT_CAN_RETURN_NULL
PARROT_WARN_UNUSED_RESULT
PMC *
Parrot_oo_get_class(PARROT_INTERP, ARGIN(PMC *key))
{
    PMC *classobj = PMCNULL;

    if (PObj_is_class_TEST(key))
        classobj = key;
    else {
        /* Fast select of behavior based on type of the lookup key */
        switch (key->vtable->base_type) {
            case enum_class_NameSpace:
                classobj = VTABLE_get_class(interp, key);
                break;

            case enum_class_String:
            case enum_class_Key:
            case enum_class_ResizableStringArray:
                {
                PMC * const hll_ns = VTABLE_get_pmc_keyed_int(interp,
                                        interp->HLL_namespace,
                                        CONTEXT(interp)->current_HLL);
                PMC * const ns     = Parrot_get_namespace_keyed(interp,
                                        hll_ns, key);

                if (!PMC_IS_NULL(ns))
                    classobj = VTABLE_get_class(interp, ns);
                }
            default:
                break;
        }
    }

    if (PMC_IS_NULL(classobj)) {
        /* Look up a low-level class and create a proxy */
        const INTVAL type = pmc_type(interp, VTABLE_get_string(interp, key));

        /* Reject invalid type numbers */
        if (type > interp->n_vtable_max || type <= 0)
            return PMCNULL;
        else {
             PMC * const type_num = pmc_new(interp, enum_class_Integer);
             VTABLE_set_integer_native(interp, type_num, type);
             classobj = pmc_new_init(interp, enum_class_PMCProxy, type_num);
        }
    }

    return classobj;
}


/*

=item C<PMC * Parrot_oo_get_class_str>

Lookup a class object from a builtin string.

=cut

*/

PARROT_API
PARROT_CAN_RETURN_NULL
PARROT_WARN_UNUSED_RESULT
PMC *
Parrot_oo_get_class_str(PARROT_INTERP, ARGIN(STRING *name))
{
    PMC * const hll_ns = VTABLE_get_pmc_keyed_int(interp, interp->HLL_namespace,
                           CONTEXT(interp)->current_HLL);
    PMC * const ns     = Parrot_get_namespace_keyed_str(interp, hll_ns, name);
    PMC * const _class = PMC_IS_NULL(ns)
                       ? PMCNULL : VTABLE_get_class(interp, ns);

    /* Look up a low-level class and create a proxy */
    if (PMC_IS_NULL(_class)) {
        const INTVAL type = pmc_type(interp, name);

        /* Reject invalid type numbers */
        if (type > interp->n_vtable_max || type <= 0)
            return PMCNULL;
        else {
            PMC * const type_num = pmc_new(interp, enum_class_Integer);
            VTABLE_set_integer_native(interp, type_num, type);
            return pmc_new_init(interp, enum_class_PMCProxy, type_num);
        }
    }

    return _class;
}


/*

=item C<PMC * Parrot_oo_newclass_from_str>

Create a new class object from a string name.

=cut

*/

PARROT_CAN_RETURN_NULL
PARROT_WARN_UNUSED_RESULT
PMC *
Parrot_oo_newclass_from_str(PARROT_INTERP, ARGIN(STRING *name))
{
    PMC * const namearg  = pmc_new(interp, enum_class_String);
    PMC        *namehash = pmc_new(interp, enum_class_Hash);
    PMC        *classobj;

    VTABLE_set_string_native(interp, namearg, name);
    VTABLE_set_pmc_keyed_str(interp, namehash, CONST_STRING(interp, "name"), namearg);

    classobj = pmc_new_init(interp, enum_class_Class, namehash);

    PARROT_ASSERT(classobj);
    return classobj;
}


/*

=item C<PMC * Parrot_oo_find_vtable_override_for_class>

Lookup a vtable override in a specific class object.

=cut

*/

PARROT_CAN_RETURN_NULL
PARROT_WARN_UNUSED_RESULT
PMC *
Parrot_oo_find_vtable_override_for_class(PARROT_INTERP,
        ARGIN(PMC *classobj), ARGIN(STRING *name))
{
    Parrot_Class_attributes *class_info;
    PARROT_ASSERT(PObj_is_class_TEST(classobj));

    class_info = PARROT_CLASS(classobj);
    return VTABLE_get_pmc_keyed_str(interp, class_info->vtable_overrides, name);
}


/*

=item C<PMC * Parrot_oo_find_vtable_override>

Lookup a vtable override in a class, including any vtable overrides inherited
from parents.

=cut

*/

PARROT_CAN_RETURN_NULL
PARROT_WARN_UNUSED_RESULT
PMC *
Parrot_oo_find_vtable_override(PARROT_INTERP,
        ARGIN(PMC *classobj), ARGIN(STRING *name))
{
    Parrot_Class_attributes * const _class = PARROT_CLASS(classobj);

    if (VTABLE_exists_keyed_str(interp, _class->parent_overrides, name))
        return VTABLE_get_pmc_keyed_str(interp, _class->parent_overrides, name);
    else {
        /* Walk and search for the vtable method. */
        const INTVAL num_classes = VTABLE_elements(interp, _class->all_parents);
        PMC         *result      = PMCNULL;
        INTVAL       i;

        for (i = 0; i < num_classes; i++) {
            /* Get the class. */
            PMC * const cur_class =
                VTABLE_get_pmc_keyed_int(interp, _class->all_parents, i);

            result = Parrot_oo_find_vtable_override_for_class(interp,
                        cur_class, name);

            if (!PMC_IS_NULL(result))
                break;
        }

        VTABLE_set_pmc_keyed_str(interp, _class->parent_overrides, name, result);

        return result;
    }
}


/*

=item C<INTVAL Parrot_get_vtable_index>

Return index if C<name> is a valid vtable slot name.

=cut

*/

PARROT_API
INTVAL
Parrot_get_vtable_index(PARROT_INTERP, ARGIN(const STRING *name))
{
    char * const name_c      = string_to_cstring(interp, name);

    /* some of the first "slots" don't have names. skip 'em. */
    INTVAL low               = PARROT_VTABLE_LOW;
    INTVAL high              = NUM_VTABLE_FUNCTIONS + PARROT_VTABLE_LOW;

    while (low < high) {
        const INTVAL       mid    = (low + high) / 2;
        const char * const meth_c = Parrot_vtable_slot_names[mid];

        /* RT #45965 slot_names still have __ in front */
        const INTVAL cmp = strcmp(name_c, meth_c + 2);

        if (cmp == 0) {
            string_cstring_free(name_c);
            return mid;
        }
        else if (cmp > 0)
            low  = mid + 1;
        else
            high = mid;
    }

    string_cstring_free(name_c);

    return -1;
}


/*

=item C<STRING* readable_name>

Given a String or Key PMC return the STRING* representation

RT #45967 this function, key_set_to_string, and the key PMC get_repr should be
consolidated

=cut

*/

PARROT_API
PARROT_WARN_UNUSED_RESULT
PARROT_CANNOT_RETURN_NULL
STRING*
readable_name(PARROT_INTERP, ARGIN(PMC *name))
{
    PMC    *array;

    if (name->vtable->base_type == enum_class_String)
        return VTABLE_get_string(interp, name);

    array   = pmc_new(interp, enum_class_ResizableStringArray);

    PARROT_ASSERT(name->vtable->base_type == enum_class_Key);

    while (name) {
        VTABLE_push_string(interp, array, VTABLE_get_string(interp, name));
        name = key_next(interp, name);
    }

    return string_join(interp, CONST_STRING(interp, ";"), array);
}


/*

=item C<const char* Parrot_MMD_method_name>

Return the method name for the given MMD enum.

{{**DEPRECATE**}}

=cut

*/

PARROT_API
PARROT_PURE_FUNCTION
PARROT_CAN_RETURN_NULL
const char*
Parrot_MMD_method_name(SHIM_INTERP, INTVAL idx)
{
    PARROT_ASSERT(idx >= 0);

    if (idx >= MMD_USER_FIRST)
        return NULL;

    return Parrot_mmd_func_names[idx];
}


/*

=item C<PMC * Parrot_class_lookup>

Looks for the class named C<class_name> and returns it if it exists.
Otherwise it returns C<PMCNULL>.

=cut

*/

PARROT_API
PARROT_CAN_RETURN_NULL
PARROT_WARN_UNUSED_RESULT
PMC *
Parrot_class_lookup(PARROT_INTERP, ARGIN(STRING *class_name))
{
    const INTVAL type = pmc_type(interp, class_name);
    PMC         *pmc;

    if (type <= 0)
        return PMCNULL;

    pmc = interp->vtables[type]->pmc_class;
    PARROT_ASSERT(pmc);
    return pmc;
}


/*

=item C<PMC * Parrot_class_lookup_p>

Looks for the class named C<class_name> and returns it if it exists.
Otherwise it returns C<PMCNULL>.

=cut

*/

PARROT_CAN_RETURN_NULL
PARROT_WARN_UNUSED_RESULT
PMC *
Parrot_class_lookup_p(PARROT_INTERP, ARGIN(PMC *class_name))
{
    const INTVAL type = pmc_type_p(interp, class_name);
    PMC         *pmc;

    if (type <= 0)
        return PMCNULL;

    pmc = interp->vtables[type]->pmc_class;
    PARROT_ASSERT(pmc);
    return pmc;
}


/*

=item C<static void fail_if_type_exists>

This function throws an exception if a PMC or class with the same name *
already exists in the global type registry. The global type registry
will go away eventually, but this allows the new object metamodel to
interact with the old one until it does.

=cut

*/

static void
fail_if_type_exists(PARROT_INTERP, ARGIN(PMC *name))
{
    INTVAL      type;

    PMC * const classname_hash = interp->class_hash;
    PMC * const type_pmc       = (PMC *)VTABLE_get_pointer_keyed(interp,
                                        classname_hash, name);

    if (PMC_IS_NULL(type_pmc)
    ||  type_pmc->vtable->base_type == enum_class_NameSpace)
        type = 0;
    else
        type = VTABLE_get_integer(interp, type_pmc);

    if (type > enum_type_undef) {
        STRING *classname;

        if (VTABLE_isa(interp, name, CONST_STRING(interp, "ResizableStringArray"))) {
            PMC * const base_ns = VTABLE_get_pmc_keyed_int(interp,
                                    interp->HLL_namespace,
                                    CONTEXT(interp)->current_HLL);
            PMC             *ns = Parrot_get_namespace_keyed(interp,
                                    base_ns, name);

            if (!PMC_IS_NULL(ns))
                classname = VTABLE_get_string(interp, ns);
            else
                classname = CONST_STRING(interp, "");
        }
        else
            classname = VTABLE_get_string(interp, name);

        Parrot_ex_throw_from_c_args(interp, NULL, EXCEPTION_INVALID_OPERATION,
            "Class %Ss already registered!\n",
            string_escape_string(interp, classname));
    }

    if (type < enum_type_undef)
        Parrot_ex_throw_from_c_args(interp, NULL, EXCEPTION_INVALID_OPERATION,
            "native type with name '%s' already exists - "
            "can't register Class", data_types[type].name);
}


/*

=item C<INTVAL Parrot_oo_register_type>

This function registers a type in the global registry, first checking if it
already exists. The global type registry will go away eventually, but this
allows the new object metamodel to interact with the old one until it does.

=cut

*/

PARROT_WARN_UNUSED_RESULT
INTVAL
Parrot_oo_register_type(PARROT_INTERP, ARGIN(PMC *name))
{
    INTVAL type;
    PMC   *classname_hash, *item;

    fail_if_type_exists(interp, name);

    /* Type doesn't exist, so go ahead and register it. Lock interpreter so
     * pt_shared_fixup() can safely do a type lookup. */
    LOCK_INTERPRETER(interp);
    classname_hash = interp->class_hash;

    type = interp->n_vtable_max++;

    /* Have we overflowed the table? */
    if (type >= interp->n_vtable_alloced)
        parrot_realloc_vtables(interp);

    /* set entry in name->type hash */
    item              = pmc_new(interp, enum_class_Integer);
    PMC_int_val(item) = type;

    VTABLE_set_pmc_keyed(interp, classname_hash, name, item);
    UNLOCK_INTERPRETER(interp);

    return type;
}

/*

=item C<PMC * Parrot_remove_parent>

This currently does nothing but return C<PMCNULL>.
RT #50646

=cut

*/

PARROT_API
PARROT_IGNORABLE_RESULT
PARROT_CAN_RETURN_NULL
PMC *
Parrot_remove_parent(PARROT_INTERP, ARGIN(PMC *removed_class),
        ARGIN(PMC *existing_class))
{
    UNUSED(interp);
    UNUSED(removed_class);
    UNUSED(existing_class);

    return PMCNULL;
}


/*

=item C<void mark_object_cache>

Marks all PMCs in the object method cache as live.  This shouldn't strictly be
necessary, as they're likely all reachable from namespaces and classes, but
it's unlikely to hurt anything except mark phase performance.

=cut

*/

#define TBL_SIZE_MASK 0x1ff   /* x bits 2..10 */
#define TBL_SIZE (1 + TBL_SIZE_MASK)

void
mark_object_cache(PARROT_INTERP)
{
    Caches * const mc = interp->caches;
    UINTVAL type, entry;

    if (!mc)
        return;

    for (type = 0; type < mc->mc_size; type++) {
        if (!mc->idx[type])
            continue;

        for (entry = 0; entry < TBL_SIZE; ++entry) {
            Meth_cache_entry *e = mc->idx[type][entry];
            while (e) {
                pobject_lives(interp, (PObj *)e->pmc);
                e = e->next;
            }
        }
    }
}


/*

=item C<void init_object_cache>

Allocate memory for object cache.

=cut

*/

void
init_object_cache(PARROT_INTERP)
{
    Caches * const mc = interp->caches = mem_allocate_zeroed_typed(Caches);
    mc->idx = NULL;
}


/*

=item C<void destroy_object_cache>

RT #48260: Not yet documented!!!

=cut

*/

void
destroy_object_cache(PARROT_INTERP)
{
    UINTVAL i;
    Caches * const mc = interp->caches;

    /* mc->idx[type][bits] = e; */
    for (i = 0; i < mc->mc_size; i++) {
        if (mc->idx[i])
            invalidate_type_caches(interp, i);
    }

    mem_sys_free(mc->idx);
    mem_sys_free(mc);
}


/*

=item C<static void invalidate_type_caches>

RT #48260: Not yet documented!!!

=cut

*/

static void
invalidate_type_caches(PARROT_INTERP, UINTVAL type)
{
    Caches * const mc = interp->caches;
    INTVAL i;

    if (!mc)
        return;

    /* is it a valid entry */
    if (type >= mc->mc_size || !mc->idx[type])
        return;

    for (i = 0; i < TBL_SIZE; ++i) {
        Meth_cache_entry *e = mc->idx[type][i];
        while (e) {
            Meth_cache_entry * const next = e->next;
            mem_sys_free(e);
            e = next;
        }
    }

    mem_sys_free(mc->idx[type]);
    mc->idx[type] = NULL;
}


/*

=item C<static void invalidate_all_caches>

RT #48260: Not yet documented!!!

=cut

*/

static void
invalidate_all_caches(PARROT_INTERP)
{
    UINTVAL i;
    for (i = 1; i < (UINTVAL)interp->n_vtable_max; ++i)
        invalidate_type_caches(interp, i);
}


/*

=item C<void Parrot_invalidate_method_cache>

Clear method cache for the given class. If class is NULL, caches for
all classes are invalidated.

=cut

*/

PARROT_API
void
Parrot_invalidate_method_cache(PARROT_INTERP, ARGIN_NULLOK(STRING *_class), ARGIN(STRING *meth))
{
    INTVAL type;

    /* during interp creation and NCI registration the class_hash
     * isn't yet up */
    if (!interp->class_hash)
        return;

    if (interp->resume_flag & RESUME_INITIAL)
        return;

    if (!_class) {
        invalidate_all_caches(interp);
        return;
    }

    type = pmc_type(interp, _class);

    if (type == 0)
        invalidate_all_caches(interp);
    else if (type > 0)
        invalidate_type_caches(interp, (UINTVAL)type);
}


/*
 * quick'n'dirty method cache
 * RT #45987: use a hash if method_name is not constant
 *       i.e. from obj.$Sreg(args)
 *       If this hash is implemented mark it during DOD
 */

/*

=item C<PMC * Parrot_find_method_direct>

Find a method PMC for a named method, given the class PMC, current
interpreter, and name of the method. Don't use a possible method cache.

=cut

*/

PARROT_API
PARROT_CAN_RETURN_NULL
PARROT_WARN_UNUSED_RESULT
PMC *
Parrot_find_method_direct(PARROT_INTERP, ARGIN(PMC *_class), ARGIN(STRING *method_name))
{
    PMC * const found = find_method_direct_1(interp, _class, method_name);

    if (!PMC_IS_NULL(found))
        return found;


    if (!string_equal(interp, method_name, CONST_STRING(interp, "__get_string")))
        return find_method_direct_1(interp, _class,
            CONST_STRING(interp, "__get_repr"));

    return PMCNULL;
}


/*

=item C<PMC * Parrot_find_method_with_cache>

Find a method PMC for a named method, given the class PMC, current
interp, and name of the method.

This routine should use the current scope's method cache, if there is
one. If not, it creates a new method cache. Or, rather, it will when
we've got that bit working. For now it unconditionally goes and looks up
the name in the global stash.

=cut

*/

PARROT_API
PARROT_CAN_RETURN_NULL
PARROT_WARN_UNUSED_RESULT
PMC *
Parrot_find_method_with_cache(PARROT_INTERP, ARGIN(PMC *_class), ARGIN(STRING *method_name))
{
    UINTVAL type, bits;

    Caches           *mc;
    Meth_cache_entry *e, *old;

    PARROT_ASSERT(method_name != 0);

#if DISABLE_METH_CACHE
    return Parrot_find_method_direct(interp, _class, method_name);
#endif

    if (! PObj_constant_TEST(method_name))
        return Parrot_find_method_direct(interp, _class, method_name);

    mc   = interp->caches;
    type = _class->vtable->base_type;
    bits = (((UINTVAL) method_name->strstart) >> 2) & TBL_SIZE_MASK;

    if (type >= mc->mc_size) {
        if (mc->idx) {
            mc->idx = (Meth_cache_entry ***)mem_sys_realloc_zeroed(mc->idx,
                sizeof (Meth_cache_entry ***) * (type + 1),
                sizeof (Meth_cache_entry ***) * mc->mc_size);
        }
        else {
            mc->idx = (Meth_cache_entry ***)mem_sys_allocate_zeroed(
                sizeof (Meth_cache_entry ***) * (type + 1));
        }
        mc->mc_size = type + 1;
    }

    if (!mc->idx[type]) {
        mc->idx[type] = (Meth_cache_entry **)mem_sys_allocate_zeroed(
            sizeof (Meth_cache_entry *) * TBL_SIZE);
    }

    e   = mc->idx[type][bits];
    old = NULL;

    while (e && e->strstart != method_name->strstart) {
        old = e;
        e   = e->next;
    }

    if (!e) {
        /* when here no or no correct entry was at [bits] */
        e     = mem_allocate_typed(Meth_cache_entry);

        if (old)
            old->next = e;
        else
            mc->idx[type][bits] = e;

        e->pmc      = Parrot_find_method_direct(interp, _class, method_name);
        e->next     = NULL;
        e->strstart = method_name->strstart;
    }

    return e->pmc;
}


/*

=item C<static void debug_trace_find_meth>

RT #48260: Not yet documented!!!

=cut

*/

#ifdef NDEBUG
#  define TRACE_FM(i, c, m, sub)
#else
#  define TRACE_FM(i, c, m, sub) \
    debug_trace_find_meth((i), (c), (m), (sub))

static void
debug_trace_find_meth(PARROT_INTERP, ARGIN(const PMC *_class),
        ARGIN(const STRING *name), ARGIN_NULLOK(const PMC *sub))
{
    STRING *class_name;
    const char *result;
    Interp *tracer;

    if (!Interp_trace_TEST(interp, PARROT_TRACE_FIND_METH_FLAG))
        return;

    if (PObj_is_class_TEST(_class)) {
        SLOTTYPE * const class_array = PMC_data_typed(_class, SLOTTYPE *);
        PMC *const class_name_pmc = get_attrib_num(class_array, PCD_CLASS_NAME);
        class_name = PMC_str_val(class_name_pmc);
    }
    else
        class_name = _class->vtable->whoami;

    if (sub) {
        if (sub->vtable->base_type == enum_class_NCI)
            result = "NCI";
        else
            result = "Sub";
    }
    else
        result = "no";

    tracer = interp->debugger ? interp->debugger : interp;
    PIO_eprintf(tracer, "# find_method class '%Ss' method '%Ss': %s\n",
            class_name, name, result);
}

#endif


/*

=item C<static PMC * find_method_direct_1>

RT #48260: Not yet documented!!!

=cut

*/

PARROT_WARN_UNUSED_RESULT
PARROT_CAN_RETURN_NULL
static PMC *
find_method_direct_1(PARROT_INTERP, ARGIN(PMC *_class),
                              ARGIN(STRING *method_name))
{
    INTVAL i;

    PMC * const  mro = _class->vtable->mro;
    const INTVAL n   = VTABLE_elements(interp, mro);

    for (i = 0; i < n; ++i) {
        PMC *method, *ns;

        _class = VTABLE_get_pmc_keyed_int(interp, mro, i);
        ns     = VTABLE_get_namespace(interp, _class);
        method = VTABLE_get_pmc_keyed_str(interp, ns, method_name);

        TRACE_FM(interp, _class, method_name, method);

        if (!PMC_IS_NULL(method))
            return method;
    }

    TRACE_FM(interp, _class, method_name, NULL);
    return PMCNULL;
}


/* ************************************************************************ */
/* ********* BELOW HERE IS NEW PPD15 IMPLEMENTATION RELATED STUFF ********* */
/* ************************************************************************ */

/*

=item C<static PMC* C3_merge>

RT #48260: Not yet documented!!!

=cut

*/

PARROT_WARN_UNUSED_RESULT
PARROT_CAN_RETURN_NULL
static PMC*
C3_merge(PARROT_INTERP, ARGIN(PMC *merge_list))
{
    PMC      *accepted   = PMCNULL;
    PMC      *result     = pmc_new(interp, enum_class_ResizablePMCArray);
    const int list_count = VTABLE_elements(interp, merge_list);
    int       cand_count = 0;
    int       i;

    /* Try and find something appropriate to add to the MRO - basically, the
     * first list head that is not in the tail of all the other lists. */
    for (i = 0; i < list_count; i++) {
        PMC * const cand_list = VTABLE_get_pmc_keyed_int(interp, merge_list, i);

        PMC *cand_class;
        int reject = 0;
        int j;

        if (VTABLE_elements(interp, cand_list) == 0)
            continue;

        cand_class = VTABLE_get_pmc_keyed_int(interp, cand_list, 0);
        cand_count++;

        for (j = 0; j < list_count; j++) {
            /* Skip the current list. */
            if (j != i) {
                /* Is it in the tail? If so, reject. */
                PMC * const check_list =
                        VTABLE_get_pmc_keyed_int(interp, merge_list, j);

                const int check_length = VTABLE_elements(interp, check_list);
                int k;

                for (k = 1; k < check_length; k++) {
                    if (VTABLE_get_pmc_keyed_int(interp, check_list, k) ==
                        cand_class) {
                        reject = 1;
                        break;
                    }
                }
            }
        }

        /* If we didn't reject it, this candidate will do. */
        if (!reject) {
            accepted = cand_class;
            break;
        }
    }

    /* If we never found any candidates, return an empty list. */
    if (cand_count == 0)
        return pmc_new(interp, enum_class_ResizablePMCArray);

    /* If we didn't find anything to accept, error. */
    if (PMC_IS_NULL(accepted))
        Parrot_ex_throw_from_c_args(interp, NULL, EXCEPTION_ILL_INHERIT,
            "Could not build C3 linearization: ambiguous hierarchy");

    /* Otherwise, remove what was accepted from the merge lists. */
    for (i = 0; i < list_count; i++) {
        int j;

        PMC * const list       = VTABLE_get_pmc_keyed_int(interp, merge_list, i);
        const int   list_count = VTABLE_elements(interp, list);

        for (j = 0; j < list_count; j++) {
            if (VTABLE_get_pmc_keyed_int(interp, list, j) == accepted) {
                VTABLE_delete_keyed_int(interp, list, j);
                break;
            }
        }
    }

    /* Need to merge what remains of the list, then put what was accepted on
     * the start of the list, and we're done. */
    result = C3_merge(interp, merge_list);
    VTABLE_unshift_pmc(interp, result, accepted);

    return result;
}


/*

=item C<PMC* Parrot_ComputeMRO_C3>

Computes the C3 linearization for the given class.

=cut

*/

PARROT_API
PARROT_WARN_UNUSED_RESULT
PARROT_CAN_RETURN_NULL
PMC*
Parrot_ComputeMRO_C3(PARROT_INTERP, ARGIN(PMC *_class))
{
    PMC *result;
    PMC * const merge_list = pmc_new(interp, enum_class_ResizablePMCArray);
    PMC *immediate_parents;
    int i, parent_count;

    /* Now get immediate parents list. */
    Parrot_PCCINVOKE(interp, _class, CONST_STRING(interp, "parents"),
        "->P", &immediate_parents);

    if (!immediate_parents)
        Parrot_ex_throw_from_c_args(interp, NULL, EXCEPTION_METH_NOT_FOUND,
            "Failed to get parents list from class!");

    parent_count = VTABLE_elements(interp, immediate_parents);

    if (parent_count == 0) {
        /* No parents - MRO just contains this class. */
        result = pmc_new(interp, enum_class_ResizablePMCArray);
        VTABLE_push_pmc(interp, result, _class);
        return result;
    }

    /* Otherwise, need to do merge. For that, need linearizations of all of
     * our parents added to the merge list. */
    for (i = 0; i < parent_count; i++) {
        PMC * const lin = Parrot_ComputeMRO_C3(interp,
            VTABLE_get_pmc_keyed_int(interp, immediate_parents, i));

        if (PMC_IS_NULL(lin))
            return PMCNULL;

        VTABLE_push_pmc(interp, merge_list, lin);
    }

    /* Finally, need list of direct parents on the end of the merge list, then
     * we can merge. */
    VTABLE_push_pmc(interp, merge_list, immediate_parents);
    result = C3_merge(interp, merge_list);

    if (PMC_IS_NULL(result))
        return PMCNULL;

    /* Merged result needs this class on the start, and then we're done. */
    VTABLE_unshift_pmc(interp, result, _class);

    return result;
}


/*

=item C<void Parrot_ComposeRole>

Used by the Class and Object PMCs internally to compose a role into either of
them. The C<role> parameter is the role that we are composing into the class
or role. C<methods_hash> is the hash of method names to invokable PMCs that
contains the methods the class or role has. C<roles_list> is the list of roles
the the class or method does.

The C<role> parameter is only dealt with by its external interface. Whether
this routine is usable by any other object system implemented in Parrot very
much depends on how closely the role composition semantics they want are to
the default implementation.

=cut

*/

PARROT_API
void
Parrot_ComposeRole(PARROT_INTERP, ARGIN(PMC *role),
                        ARGIN(PMC *exclude), int got_exclude,
                        ARGIN(PMC *alias),   int got_alias,
                        ARGIN(PMC *methods_hash), ARGIN(PMC *roles_list))
{
    PMC *methods;
    PMC *methods_iter;
    PMC *roles_of_role;
    PMC *proposed_add_methods;

    int i, roles_of_role_count;

    /* Check we have not already composed the role; if so, just ignore it. */
    int roles_count = VTABLE_elements(interp, roles_list);

    for (i = 0; i < roles_count; i++)
        if (VTABLE_get_pmc_keyed_int(interp, roles_list, i) == role)
            return;

    /* Get the methods from the role. */
    Parrot_PCCINVOKE(interp, role,
        CONST_STRING(interp, "methods"), "->P", &methods);

    if (PMC_IS_NULL(methods))
        return;

    /* We need to check for conflicts before we do the composition. We
     * put each method that would be OK to add into a proposal list, and
     * bail out right away if we find a problem. */
    proposed_add_methods = pmc_new(interp, enum_class_Hash);
    methods_iter         = VTABLE_get_iter(interp, methods);

    while (VTABLE_get_bool(interp, methods_iter)) {
        STRING * const method_name = VTABLE_shift_string(interp, methods_iter);
        PMC    * const cur_method  = VTABLE_get_pmc_keyed_str(interp, methods,
                                        method_name);

        /* Need to find the name we'll check for a conflict on. */
        int excluded = 0;

        /* Check if it's in the exclude list. */
        if (got_exclude) {
            const int exclude_count = VTABLE_elements(interp, exclude);

            for (i = 0; i < exclude_count; i++) {
                const STRING * const check =
                    VTABLE_get_string_keyed_int(interp, exclude, i);

                if (string_equal(interp, check, method_name) == 0) {
                    excluded = 1;
                    break;
                }
            }
        }

        /* If we weren't excluded... */
        if (!excluded) {
            /* Is there a method with this name already in the class?
             * RT #45999 multi-method handling. */

            if (VTABLE_exists_keyed_str(interp, methods_hash, method_name))
                /* Conflicts with something already in the class. */
                Parrot_ex_throw_from_c_args(interp, NULL,
                    EXCEPTION_ROLE_COMPOSITION_METH_CONFLICT,
                    "A conflict occurred during role composition "
                    "due to method '%S'.", method_name);

            /* What about a conflict with ourslef? */
            if (VTABLE_exists_keyed_str(interp, proposed_add_methods,
                method_name))
                /* Something very weird is going on. */
                Parrot_ex_throw_from_c_args(interp, NULL,
                    EXCEPTION_ROLE_COMPOSITION_METH_CONFLICT,
                    "A conflict occurred during role composition;"
                    " the method '%S' from the role managed to conflict "
                    "with itself somehow.", method_name);

            /* If we got here, no conflicts! Add method to the "to compose"
             * list. */
            VTABLE_set_pmc_keyed_str(interp, proposed_add_methods,
                method_name, cur_method);
        }

        /* Now see if we've got an alias. */
        if (got_alias && VTABLE_exists_keyed_str(interp, alias, method_name)) {
            /* Got one. Get name to alias it to. */
            STRING * const alias_name = VTABLE_get_string_keyed_str(interp,
                alias, method_name);

            /* Is there a method with this name already in the class?
             * RT #45999: multi-method handling. */
            if (VTABLE_exists_keyed_str(interp, methods_hash, alias_name))
                /* Conflicts with something already in the class. */
                Parrot_ex_throw_from_c_args(interp, NULL,
                    EXCEPTION_ROLE_COMPOSITION_METH_CONFLICT,
                    "A conflict occurred during role composition"
                    " due to the aliasing of '%S' to '%S'.",
                    method_name, alias_name);

            /* What about a conflict with ourslef? */
            if (VTABLE_exists_keyed_str(interp, proposed_add_methods,
                    alias_name))
                Parrot_ex_throw_from_c_args(interp, NULL,
                    EXCEPTION_ROLE_COMPOSITION_METH_CONFLICT,
                    "A conflict occurred during role composition"
                    " due to the aliasing of '%S' to '%S' (role already has"
                    " a method '%S').", method_name, alias_name, alias_name);

            /* If we get here, no conflicts! Add method to the "to compose"
             * list with its alias. */
            VTABLE_set_pmc_keyed_str(interp, proposed_add_methods,
                alias_name, cur_method);
        }
    }

    /* If we get here, we detected no conflicts. Go ahead and compose the
     * methods. */
    methods_iter = VTABLE_get_iter(interp, proposed_add_methods);

    while (VTABLE_get_bool(interp, methods_iter)) {
        /* Get current method and its name. */
        STRING * const method_name = VTABLE_shift_string(interp, methods_iter);
        PMC *    const cur_method  = VTABLE_get_pmc_keyed_str(interp,
                                        proposed_add_methods, method_name);

        /* Add it to the methods of the class. */
        VTABLE_set_pmc_keyed_str(interp, methods_hash, method_name, cur_method);
    }

    /* Add this role to the roles list. */
    VTABLE_push_pmc(interp, roles_list, role);
    roles_count++;

    /* As a result of composing this role, we will also now do the roles
     * that it did itself. Note that we already have the correct methods
     * as roles "flatten" the methods they get from other roles into their
     * own method list. */
    Parrot_PCCINVOKE(interp, role,
        CONST_STRING(interp, "roles"), "->P", &roles_of_role);
    roles_of_role_count = VTABLE_elements(interp, roles_of_role);

    for (i = 0; i < roles_of_role_count; i++) {
        /* Only add if we don't already have it in the list. */
        PMC * const cur_role = VTABLE_get_pmc_keyed_int(interp,
                                    roles_of_role, i);
        int j;

        for (j = 0; j < roles_count; j++) {
            if (VTABLE_get_pmc_keyed_int(interp, roles_list, j) == cur_role) {
                /* We ain't be havin' it. */
                VTABLE_push_pmc(interp, roles_list, cur_role);
            }
        }
    }
}


/*

=back

=head1 SEE ALSO

F<include/parrot/oo.h>, F<include/parrot/oo_private.h>,
F<docs/pdds/pdd15_objects.pod>.

=cut

*/

/*
 * Local variables:
 *   c-file-style: "parrot"
 * End:
 * vim: expandtab shiftwidth=4:
 */
