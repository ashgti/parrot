/* pmc.c
 *  Copyright: 2001-2003 The Perl Foundation.  All Rights Reserved.
 *  CVS Info
 *     $Id$
 *  Overview:
 *     The base vtable calling functions.
 *  Data Structure and Algorithms:
 *     See include/parrot/vtable.h.
 *  History:
 *     Initial version by Simon on 2001.10.20
 *  Notes:
 *  References:
 *     <5.1.0.14.2.20011008152120.02158148@pop.sidhe.org>
 */

#include "parrot/parrot.h"
static PMC* get_new_pmc_header(Parrot_Interp, INTVAL base_type, int constant);

#if PARROT_CATCH_NULL
PMC * PMCNULL;
Parrot_mutex init_null_mutex;

PMC *
pmc_init_null(struct Parrot_Interp * interpreter)
{
    LOCK(init_null_mutex);
    if(!PMCNULL)
       PMCNULL = get_new_pmc_header(interpreter, enum_class_Null, 1);
    PMCNULL->pmc_ext = NULL;
    PMCNULL->vtable = Parrot_base_vtables[enum_class_Null];
    UNLOCK(init_null_mutex);
    return PMCNULL;
}
#endif

/*

=for api pmc pmc_new

   Creates a new PMC of type C<base_type> (which is an index into
   the list of PMC types declared in C<Parrot_base_vtables> in
   F<pmc.h>). Once the PMC has been successfully created and
   its vtable pointer initialized, we call its C<init> method to
   perform any other necessary initialization.

=cut

*/

PMC *
pmc_new(struct Parrot_Interp *interpreter, INTVAL base_type)
{
    PMC *pmc = pmc_new_noinit(interpreter, base_type);
    VTABLE_init(interpreter, pmc);
    return pmc;
}

static PMC*
get_new_pmc_header(struct Parrot_Interp *interpreter, INTVAL base_type,
    int constant)
{
    struct Small_Object_Pool *pool;
    PMC *pmc;

    if (Parrot_base_vtables[base_type]->flags & VTABLE_IS_CONST_FLAG) {
        /* put the normal vtable in, so that the pmc can be initialized first
         * parrot or user code has to set the _ro property then,
         * to morph the PMC to the const variant
         */
        constant = 1;
        --base_type;
    }
    pool = constant ?
        interpreter->arena_base->constant_pmc_pool :
        interpreter->arena_base->pmc_pool;

    pmc = get_free_pmc(interpreter, pool);
    if (!pmc) {
        internal_exception(ALLOCATION_ERROR,
                "Parrot VM: PMC allocation failed!\n");
        return NULL;
    }

    if (constant)
        PObj_constant_SET(pmc);
    pmc->vtable = Parrot_base_vtables[base_type];

    if (!pmc->vtable || !pmc->vtable->init) {
        /* This is usually because you either didn't call init_world early
         * enough or you added a new PMC class without adding
         * Parrot_(classname)_class_init to init_world. */
        PANIC("Null vtable used");
        return NULL;
    }

    return pmc;
}

static void
pmc_new_ext(Parrot_Interp interpreter, PMC *pmc, INTVAL base_type)
{
    if (pmc->vtable->flags & VTABLE_PMC_NEEDS_EXT) {
        add_pmc_ext(interpreter, pmc);

        if (pmc->vtable->flags & VTABLE_IS_SHARED_FLAG) {
            PMC_sync(pmc) = mem_sys_allocate(sizeof(*pmc->synchronize));
            PMC_sync(pmc)->owner = interpreter;
            MUTEX_INIT(PMC_sync(pmc)->pmc_lock);
            PObj_is_PMC_shared_SET(pmc);
        }
    }
}

/*

=for api pmc pmc_new_noinit

   Creates a new PMC of type C<base_type> (which is an index into the
   list of PMC types declared in C<Parrot_base_vtables> in
   F<pmc.h>). Unlike C<pmc_new>, C<pmc_new_noinit> does not call its
   C<init> method.  This allows separate allocation and initialization
   for continuations.

=cut

*/

PMC *
pmc_new_noinit(struct Parrot_Interp *interpreter, INTVAL base_type)
{
    PMC *pmc;
    /* we only have one global Env object, livin in the interpreter */
    if (Parrot_base_vtables[base_type]->flags & VTABLE_PMC_IS_SINGLETON) {
        if (base_type == enum_class_Env) {
            /* XXX need probably a lock around this code
             */
            pmc = VTABLE_get_pmc_keyed_int(interpreter, interpreter->iglobals,
                    (INTVAL)IGLOBALS_ENV_HASH);
            if (!pmc) {
                pmc = get_new_pmc_header(interpreter, base_type, 0);
                VTABLE_set_pmc_keyed_int(interpreter, interpreter->iglobals,
                        (INTVAL)IGLOBALS_ENV_HASH, pmc);
            /* UNLOCK */}
            return pmc;
        }
        /*
         * singletons (monadic objects) exist only once, the interface
         * with the class is:
         * - get_pointer: return NULL or a pointer to the single instance
         * - set_pointer: set the only instance once
         */
        pmc = (Parrot_base_vtables[base_type]->get_pointer)(interpreter, NULL);
        /* LOCK */
        if (!pmc) {
            pmc = get_new_pmc_header(interpreter, base_type, 0);
            VTABLE_set_pointer(interpreter, pmc, pmc);
        }
        return pmc;
    }
    pmc = get_new_pmc_header(interpreter, base_type, 0);
    pmc_new_ext(interpreter, pmc, base_type);
    return pmc;
}

/*

=for api pmc constant_pmc_new_noinit

   Creates a new constant PMC of type C<base_type>

=cut

*/

PMC *
constant_pmc_new_noinit(struct Parrot_Interp *interpreter, INTVAL base_type)
{
    PMC *pmc = get_new_pmc_header(interpreter, base_type, 1);
    pmc_new_ext(interpreter, pmc, base_type);
    return pmc;
}

/*

=for api pmc constant_pmc_new

   Creates a new constant PMC of type C<base_type>, the call C<init>.

=cut

*/

PMC *
constant_pmc_new(struct Parrot_Interp *interpreter, INTVAL base_type)
{
    PMC *pmc = get_new_pmc_header(interpreter, base_type, 1);
    pmc_new_ext(interpreter, pmc, base_type);
    VTABLE_init(interpreter, pmc);
    return pmc;
}

/*

=for api pmc pmc_new_init

   As C<pmc_new>, but passes C<init> to the PMC's C<init_pmc> method.

=cut

*/

PMC *
pmc_new_init(struct Parrot_Interp *interpreter, INTVAL base_type, PMC *init)
{
    PMC *pmc = pmc_new_noinit(interpreter, base_type);

    VTABLE_init_pmc(interpreter, pmc, init);

    return pmc;
}

/*

=for api pmc constant_pmc_new_init

   As C<constant_pmc_new>, but passes C<init> to the PMC's C<init_pmc> method.

=cut

*/

PMC *
constant_pmc_new_init(struct Parrot_Interp *interpreter, INTVAL base_type,
        PMC *init)
{
    PMC *pmc = get_new_pmc_header(interpreter, base_type, 1);
    pmc_new_ext(interpreter, pmc, base_type);
    VTABLE_init_pmc(interpreter, pmc, init);
    return pmc;
}

/* This segment handles PMC registration and such */

INTVAL
pmc_register(Parrot_Interp interp, STRING *name)
{
    INTVAL type;
    PMC *classname_hash;
    /* If they're looking to register an existing class, return that
       class' type number */
    if ((type = pmc_type(interp, name)) > enum_type_undef) {
        return type;
    }
    if (type < enum_type_undef) {
        internal_exception(1, "native type with name '%s' already exists - "
                "can't register PMC", data_types[type].name);
        return 0;
    }

    /* We don't have one, so lets add one. We need to get a lock for
       this, though */
    LOCK(class_count_mutex);
    /* Try again, just in case */
    if ((type = pmc_type(interp, name)) != enum_type_undef) {
        UNLOCK(class_count_mutex);
        return type;
    }

    classname_hash = VTABLE_get_pmc_keyed_int(interp, interp->iglobals,
                                              IGLOBALS_CLASSNAME_HASH);
    type = enum_class_max++;
    /* Have we overflowed the table? */
    if (enum_class_max > class_table_size - 1) {
        VTABLE **new_vtable_table;
        /* 10 bigger seems reasonable, though it's only a pointer
           table and we could get bigger without blowing much memory
        */
        INTVAL new_max = class_table_size + 10;
        INTVAL new_size = new_max * sizeof(VTABLE *);
        new_vtable_table = mem_sys_realloc(Parrot_base_vtables, new_size);
        /* XXX Should set all the empty slots to the null PMC's
           vtable pointer */
        Parrot_base_vtables = new_vtable_table;
        class_table_size = new_max;
    }

    VTABLE_set_integer_keyed_str(interp, classname_hash, name, type);

    UNLOCK(class_count_mutex);
    return type;
}

INTVAL
pmc_type(Parrot_Interp interp, STRING *name)
{
    INTVAL return_val;
    PMC *classname_hash = VTABLE_get_pmc_keyed_int(interp,
                            interp->iglobals, IGLOBALS_CLASSNAME_HASH);

    return_val = VTABLE_get_integer_keyed_str(interp, classname_hash, name);
    if (return_val == enum_type_undef) {
	return_val = Parrot_get_datatype_enum(interp, name);
    }
    return return_val;

}

/* This segment is made up of the 'fallback' functions -- the
   functions we call if we have no clue as to how to do MMD for a
   vtable function */

void
register_fallback_methods(Parrot_Interp interp) {
    /* Yeah, this first one's out of order logically, but it means
       the table doesn't have to keep being re-malloc'd */
    mmd_add_function(interp, MMD_SXOR, (funcptr_t)mmd_fallback_stringxor_pmc);
    mmd_add_function(interp, MMD_ADD, (funcptr_t)mmd_fallback_add_pmc);
    mmd_add_function(interp, MMD_SUBTRACT, (funcptr_t)mmd_fallback_subtract_pmc);
    mmd_add_function(interp, MMD_SUBTRACT, (funcptr_t)mmd_fallback_subtract_pmc);
    mmd_add_function(interp, MMD_MULTIPLY, (funcptr_t)mmd_fallback_multiply_pmc);
    mmd_add_function(interp, MMD_DIVIDE, (funcptr_t)mmd_fallback_divide_pmc);
    mmd_add_function(interp, MMD_MOD, (funcptr_t)mmd_fallback_mod_pmc);
    mmd_add_function(interp, MMD_CMOD, (funcptr_t)mmd_fallback_cmod_pmc);
    mmd_add_function(interp, MMD_BAND, (funcptr_t)mmd_fallback_band_pmc);
    mmd_add_function(interp, MMD_BOR, (funcptr_t)mmd_fallback_bor_pmc);
    mmd_add_function(interp, MMD_BXOR, (funcptr_t)mmd_fallback_bxor_pmc);
    mmd_add_function(interp, MMD_BSL, (funcptr_t)mmd_fallback_bsl_pmc);
    mmd_add_function(interp, MMD_BSR, (funcptr_t)mmd_fallback_bsr_pmc);
    mmd_add_function(interp, MMD_CONCAT, (funcptr_t)mmd_fallback_concat_pmc);
    mmd_add_function(interp, MMD_LAND, (funcptr_t)mmd_fallback_land_pmc);
    mmd_add_function(interp, MMD_LOR, (funcptr_t)mmd_fallback_lor_pmc);
    mmd_add_function(interp, MMD_LXOR, (funcptr_t)mmd_fallback_lxor_pmc);
    mmd_add_function(interp, MMD_REPEAT, (funcptr_t)mmd_fallback_repeat_pmc);
    mmd_add_function(interp, MMD_NUMEQ, (funcptr_t)mmd_fallback_numeq_pmc);
    mmd_add_function(interp, MMD_STREQ, (funcptr_t)mmd_fallback_streq_pmc);
    mmd_add_function(interp, MMD_NUMCMP, (funcptr_t)mmd_fallback_numcmp_pmc);
    mmd_add_function(interp, MMD_STRCMP, (funcptr_t)mmd_fallback_strcmp_pmc);
    mmd_add_function(interp, MMD_SOR, (funcptr_t)mmd_fallback_stringor_pmc);
    mmd_add_function(interp, MMD_SAND, (funcptr_t)mmd_fallback_stringand_pmc);
}

void
mmd_fallback_add_pmc(Parrot_Interp interp, PMC *left, PMC *right, PMC *dest)
{
    FLOATVAL result = (VTABLE_get_number(interp, left) +
                       VTABLE_get_number(interp, right));
    VTABLE_set_number_native(interp, dest, result);
}

void
mmd_fallback_subtract_pmc(Parrot_Interp interp,
        PMC *left, PMC *right, PMC *dest)
{
    FLOATVAL result = (VTABLE_get_number(interp, left) -
                       VTABLE_get_number(interp, right));
    VTABLE_set_number_native(interp, dest, result);

}

void
mmd_fallback_multiply_pmc(Parrot_Interp interp,
        PMC *left, PMC *right, PMC *dest)
{
    FLOATVAL result = (VTABLE_get_number(interp, left) *
                       VTABLE_get_number(interp, right));
    VTABLE_set_number_native(interp, dest, result);
}

void
mmd_fallback_divide_pmc(Parrot_Interp interp, PMC *left, PMC *right, PMC *dest) {
    FLOATVAL result = (VTABLE_get_number(interp, left) /
                       VTABLE_get_number(interp, right));
    VTABLE_set_number_native(interp, dest, result);

}

void
mmd_fallback_cmod_pmc(Parrot_Interp interp, PMC *left, PMC *right, PMC *dest)
{
    VTABLE_set_number_native(interp, dest,
                             VTABLE_get_integer(interp, left) %
                             VTABLE_get_integer(interp, right));
}

void
mmd_fallback_mod_pmc(Parrot_Interp interp, PMC *left, PMC *right, PMC *dest)
{
    FLOATVAL result = floatval_mod(VTABLE_get_number(interp, left),
                                   VTABLE_get_number(interp, right));
    VTABLE_set_number_native(interp, dest, result);
}

void
mmd_fallback_band_pmc(Parrot_Interp interp, PMC *left, PMC *right, PMC *dest)
{
    VTABLE_set_integer_native(interp, dest,
                              VTABLE_get_integer(interp, left) &
                              VTABLE_get_integer(interp, right));
}

void
mmd_fallback_bor_pmc(Parrot_Interp interp, PMC *left, PMC *right, PMC *dest)
{
    VTABLE_set_integer_native(interp, dest,
                              VTABLE_get_integer(interp, left) |
                              VTABLE_get_integer(interp, right));
}

void
mmd_fallback_bxor_pmc(Parrot_Interp interp, PMC *left, PMC *right, PMC *dest)
{
    VTABLE_set_integer_native(interp, dest,
                              VTABLE_get_integer(interp, left) ^
                              VTABLE_get_integer(interp, right));
}

void
mmd_fallback_bsl_pmc(Parrot_Interp interp, PMC *left, PMC *right, PMC *dest)
{
    VTABLE_set_integer_native(interp, dest,
                              VTABLE_get_integer(interp, left) <<
                              VTABLE_get_integer(interp, right));
}

void
mmd_fallback_bsr_pmc(Parrot_Interp interp, PMC *left, PMC *right, PMC *dest)
{
    VTABLE_set_integer_native(interp, dest,
                              VTABLE_get_integer(interp, left) >>
                              VTABLE_get_integer(interp, right));
}

void
mmd_fallback_concat_pmc(Parrot_Interp interp, PMC *left, PMC *right, PMC *dest)
{
    STRING *left_str, *right_str, *total_string;
    left_str = VTABLE_get_string(interp, left);
    right_str = VTABLE_get_string(interp, right);
    total_string = string_concat(interp, left_str, right_str, 0);
    VTABLE_set_string_native(interp, dest, total_string);

}

void
mmd_fallback_land_pmc(Parrot_Interp interp, PMC *left, PMC *right, PMC *dest)
{
    PMC *truth;
    if (!VTABLE_get_bool(interp, left)) {
        truth = left;
    } else {
        truth = right;
    }
    VTABLE_set_pmc(interp, dest, truth);
}

void
mmd_fallback_lor_pmc(Parrot_Interp interp, PMC *left, PMC *right, PMC *dest)
{
    PMC *truth;
    if (VTABLE_get_bool(interp, left)) {
        truth = left;
    } else {
        truth = right;
    }
    VTABLE_set_pmc(interp, dest, truth);
}

void
mmd_fallback_lxor_pmc(Parrot_Interp interp, PMC *left, PMC *right, PMC *dest)
{
    INTVAL left_truth, right_truth;
    PMC *true;
    left_truth = VTABLE_get_bool(interp, left);
    right_truth = VTABLE_get_bool(interp, left);
    /* Are they both false? That's the easy case */
    if ((left_truth && right_truth) || (!left_truth && !right_truth)) {
        true = constant_pmc_new_noinit(interp, enum_class_PerlUndef);
    } else {
        if (left_truth) {
            true = left;
        } else {
            true = right;
        }
    }
    VTABLE_set_pmc(interp, dest, true);
}

void
mmd_fallback_repeat_pmc(Parrot_Interp interp, PMC *left, PMC *right, PMC *dest)
{
    STRING *base;
    INTVAL count;

    base = VTABLE_get_string(interp, left);
    count = VTABLE_get_integer(interp, right);

    VTABLE_set_string_native(interp, dest,
                             string_repeat(interp, base, count, NULL));
}

INTVAL
mmd_fallback_numeq_pmc(Parrot_Interp interp, PMC *left, PMC *right)
{
    if (VTABLE_get_number(interp, left) == VTABLE_get_number(interp, right)) {
        return 1;
    } else {
        return 0;
    }
}

INTVAL
mmd_fallback_streq_pmc(Parrot_Interp interp, PMC *left, PMC *right)
{
    if (string_compare(interp, VTABLE_get_string(interp, left), VTABLE_get_string(interp, right))) {
        return 1;
    } else {
        return 0;
    }
}

INTVAL
mmd_fallback_numcmp_pmc(Parrot_Interp interp, PMC *left, PMC *right)
{
    FLOATVAL left_float, right_float;
    INTVAL cmp_val;

    left_float = VTABLE_get_number(interp, left);
    right_float = VTABLE_get_number(interp, right);

    if (left_float == right_float) {
        cmp_val = 0;
    } else {
        if (left_float > right_float) {
            cmp_val = 1;
        } else {
            cmp_val = -1;
        }
    }

    return cmp_val;
}

INTVAL
mmd_fallback_strcmp_pmc(Parrot_Interp interp, PMC *left, PMC *right)
{
    return string_compare(interp, VTABLE_get_string(interp, left), VTABLE_get_string(interp, right));
}

void
mmd_fallback_stringor_pmc(Parrot_Interp interp, PMC *left, PMC *right, PMC *dest)
{
    VTABLE_set_string_native(interp, dest,
                              string_bitwise_or(interp, VTABLE_get_string(interp, left), VTABLE_get_string(interp, right), NULL));
}

void
mmd_fallback_stringand_pmc(Parrot_Interp interp, PMC *left, PMC *right, PMC *dest)
{
    VTABLE_set_string_native(interp, dest,
                              string_bitwise_and(interp, VTABLE_get_string(interp, left), VTABLE_get_string(interp, right), NULL));
}

void
mmd_fallback_stringxor_pmc(Parrot_Interp interp, PMC *left, PMC *right, PMC *dest)
{
    VTABLE_set_string_native(interp, dest,
                              string_bitwise_xor(interp, VTABLE_get_string(interp, left), VTABLE_get_string(interp, right), NULL));
}



static size_t
key_hash_int(Interp *interp, Hash *hash, void *value)
{
    UNUSED(interp);
    UNUSED(hash);
    return (size_t) value;
}

static int
int_compare(Parrot_Interp interp, void *a, void *b)
{
    UNUSED(interp);
    return a != b;
}
/*
 * DOD registry interface
 */
void
dod_register_pmc(Parrot_Interp interpreter, PMC* pmc)
{
    Hash *hash;
    HashBucket *bucket;

    if (!interpreter->DOD_registry) {
        PMC *registry;
        registry = interpreter->DOD_registry = pmc_new_noinit(interpreter,
                enum_class_PerlHash);
        new_hash_x(interpreter, &hash, enum_type_int, 0, Hash_key_type_int,
                int_compare, key_hash_int, pobject_lives);
        PObj_custom_mark_SET(registry);
        PMC_ptr1v(registry) = hash;
    }
    else
        hash = PMC_ptr1v(interpreter->DOD_registry);

    bucket = hash_get_bucket(interpreter, hash, pmc);
    if (bucket)
        LVALUE_CAST(int, bucket->value) ++;
    else
        hash_put(interpreter, hash, pmc, (void *) 1);
}

void
dod_unregister_pmc(Parrot_Interp interpreter, PMC* pmc)
{
    Hash *hash;
    HashBucket *bucket;

    if (!interpreter->DOD_registry)
        return; /* XXX or signal exception? */
    hash = PMC_ptr1v(interpreter->DOD_registry);

    bucket = hash_get_bucket(interpreter, hash, pmc);
    if (bucket) {
        if ((int) bucket->value == 1)
            hash_delete(interpreter, hash, pmc);
        else
            LVALUE_CAST(int, bucket->value) --;
    }
}

/*
 * Local variables:
 * c-indentation-style: bsd
 * c-basic-offset: 4
 * indent-tabs-mode: nil
 * End:
 *
 * vim: expandtab shiftwidth=4:
 */
