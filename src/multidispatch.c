/*
Copyright (C) 2003-2008, The Perl Foundation.
$Id$

=head1 NAME

src/multidispatch.c - Multimethod dispatch for binary opcode functions

=head1 SYNOPSIS

This system is set up to handle type-based dispatching for binary (two
argument) functions. This includes, though isn't necessarily limited to, binary
operators such as addition or subtraction.

=head1 DESCRIPTION

The MMD system is straightforward, and currently must be explicitly invoked,
for example by a vtable function. (We reserve the right to use MMD in all
circumstances, but currently do not).

=head2 API

For the purposes of the API, each MMD-able function is assigned a unique
number which is used to find the correct function table. This is the
C<func_num> parameter in the following functions. While Parrot isn't
restricted to a predefined set of functions, it I<does> set things up so
that all the binary vtable functions have a MMD table preinstalled for
them, with default behaviour.

=head2 Remarks

C<< binop_mmd_funcs->x >> and C<< ->y >> are table sizes
not highest type in table.

=head2 Functions

=over 4

=cut

*/

#include "parrot/compiler.h"
#include "parrot/parrot.h"
#include "parrot/multidispatch.h"
#include "parrot/oplib/ops.h"
#include "multidispatch.str"

/* HEADERIZER HFILE: include/parrot/multidispatch.h */

/* HEADERIZER BEGIN: static */
/* Don't modify between HEADERIZER BEGIN / HEADERIZER END.  Your changes will be lost. */

static INTVAL distance_cmp(SHIM_INTERP, INTVAL a, INTVAL b);
static void dump_mmd(PARROT_INTERP, INTVAL function)
        __attribute__nonnull__(1);

static void mmd_add_multi_global(PARROT_INTERP,
    ARGIN(STRING *sub_name),
    ARGIN(PMC *sub_obj))
        __attribute__nonnull__(1)
        __attribute__nonnull__(2)
        __attribute__nonnull__(3);

static void mmd_add_multi_to_namespace(PARROT_INTERP,
    ARGIN(STRING *ns_name),
    ARGIN(STRING *sub_name),
    ARGIN(PMC *sub_obj))
        __attribute__nonnull__(1)
        __attribute__nonnull__(2)
        __attribute__nonnull__(3)
        __attribute__nonnull__(4);

PARROT_CANNOT_RETURN_NULL
PARROT_WARN_UNUSED_RESULT
static PMC* mmd_build_type_tuple_from_long_sig(PARROT_INTERP,
    ARGIN(STRING *long_sig))
        __attribute__nonnull__(1)
        __attribute__nonnull__(2);

PARROT_CANNOT_RETURN_NULL
PARROT_WARN_UNUSED_RESULT
static PMC* mmd_build_type_tuple_from_type_list(PARROT_INTERP,
    ARGIN(PMC *type_list))
        __attribute__nonnull__(1)
        __attribute__nonnull__(2);

static STRING * mmd_cache_key_from_types(PARROT_INTERP,
    const char *name,
    PMC *types)
        __attribute__nonnull__(1);

static STRING * mmd_cache_key_from_values(PARROT_INTERP,
    const char *name,
    PMC *values)
        __attribute__nonnull__(1);

PARROT_CANNOT_RETURN_NULL
PARROT_WARN_UNUSED_RESULT
static PMC* mmd_cvt_to_types(PARROT_INTERP, ARGIN(PMC *multi_sig))
        __attribute__nonnull__(1)
        __attribute__nonnull__(2);

static UINTVAL mmd_distance(PARROT_INTERP,
    ARGIN(PMC *pmc),
    ARGIN(PMC *arg_tuple))
        __attribute__nonnull__(1)
        __attribute__nonnull__(2)
        __attribute__nonnull__(3);

static void mmd_expand_x(PARROT_INTERP, INTVAL func_nr, INTVAL new_x)
        __attribute__nonnull__(1);

static void mmd_expand_y(PARROT_INTERP, INTVAL func_nr, INTVAL new_y)
        __attribute__nonnull__(1);

static void mmd_search_by_sig_obj(PARROT_INTERP,
    ARGIN(STRING *name),
    ARGIN(PMC *sig_obj),
    ARGIN(PMC *candidates))
        __attribute__nonnull__(1)
        __attribute__nonnull__(2)
        __attribute__nonnull__(3)
        __attribute__nonnull__(4);

static void mmd_search_global(PARROT_INTERP,
    ARGIN(STRING *name),
    ARGIN(PMC *cl))
        __attribute__nonnull__(1)
        __attribute__nonnull__(2)
        __attribute__nonnull__(3);

static int mmd_search_local(PARROT_INTERP,
    ARGIN(STRING *name),
    ARGIN(PMC *candidates))
        __attribute__nonnull__(1)
        __attribute__nonnull__(2)
        __attribute__nonnull__(3);

PARROT_WARN_UNUSED_RESULT
PARROT_CANNOT_RETURN_NULL
static PMC* Parrot_mmd_arg_tuple_func(PARROT_INTERP)
        __attribute__nonnull__(1);

static void Parrot_mmd_ensure_writable(PARROT_INTERP,
    INTVAL function,
    ARGIN_NULLOK(const PMC *pmc))
        __attribute__nonnull__(1);

static PMC * Parrot_mmd_get_cached_multi_sig(PARROT_INTERP, ARGIN(PMC *sub))
        __attribute__nonnull__(1)
        __attribute__nonnull__(2);

static int Parrot_mmd_maybe_candidate(PARROT_INTERP,
    ARGIN(PMC *pmc),
    ARGIN(PMC *cl))
        __attribute__nonnull__(1)
        __attribute__nonnull__(2)
        __attribute__nonnull__(3);

static void Parrot_mmd_search_classes(PARROT_INTERP,
    ARGIN(STRING *meth),
    ARGIN(PMC *arg_tuple),
    ARGIN(PMC *cl),
    INTVAL start_at_parent)
        __attribute__nonnull__(1)
        __attribute__nonnull__(2)
        __attribute__nonnull__(3)
        __attribute__nonnull__(4);

PARROT_CAN_RETURN_NULL
PARROT_WARN_UNUSED_RESULT
static PMC* Parrot_mmd_search_default(PARROT_INTERP,
    ARGIN(STRING *meth),
    ARGIN(PMC *arg_tuple))
        __attribute__nonnull__(1)
        __attribute__nonnull__(2)
        __attribute__nonnull__(3);

PARROT_CANNOT_RETURN_NULL
PARROT_WARN_UNUSED_RESULT
static PMC* Parrot_mmd_search_scopes(PARROT_INTERP, ARGIN(STRING *meth))
        __attribute__nonnull__(1)
        __attribute__nonnull__(2);

static PMC * Parrot_mmd_sort_candidates(PARROT_INTERP,
    ARGIN(PMC *arg_tuple),
    ARGIN(PMC *cl))
        __attribute__nonnull__(1)
        __attribute__nonnull__(2)
        __attribute__nonnull__(3);

/* Don't modify between HEADERIZER BEGIN / HEADERIZER END.  Your changes will be lost. */
/* HEADERIZER END: static */


#define MMD_DEBUG 0

#ifndef NDEBUG
/*

=item C<static void dump_mmd>

RT #48260: Not yet documented!!!

=cut

*/

static void
dump_mmd(PARROT_INTERP, INTVAL function)
{
    UINTVAL x, y;
    UINTVAL offset;
    MMD_table * const table = interp->binop_mmd_funcs + function;
    funcptr_t func;
    const UINTVAL x_funcs = table->x;
    const UINTVAL y_funcs = table->y;

    printf("    ");
    for (x = 0; x < x_funcs; ++x) {
        if (x % 10)
            printf(" ");
        else
            printf("%d", (int) x / 10);
    }

    printf("\n");

    for (y = 0; y < y_funcs; ++y) {
        printf("%3d ", (int)y);
        for (x = 0; x < x_funcs; ++x) {
            offset = x_funcs * y + x;
            func   = table->mmd_funcs[offset];

            printf("%c",
                    (UINTVAL)func & 1 ?  'P' :
                    !func  ? '0' : 'F');
        }

        printf("\n");
    }

    for (y = 0; y < y_funcs; ++y) {
        for (x = 0; x < x_funcs; ++x) {
            offset = x_funcs * y + x;
            func   = table->mmd_funcs[offset];

            if (func && !((UINTVAL) func & 1))
                printf("%3d %3d: %p\n", (int)x, (int)y, (void*) func);
        }
    }
}
#endif


/*

=item C<funcptr_t get_mmd_dispatch_type>

RT #48260: Not yet documented!!!

=cut

*/

PARROT_EXPORT
PARROT_WARN_UNUSED_RESULT
PARROT_CANNOT_RETURN_NULL
funcptr_t
get_mmd_dispatch_type(PARROT_INTERP, INTVAL func_nr, INTVAL left_type,
        INTVAL right_type, ARGOUT(int *is_pmc))
{
    funcptr_t         func    = NULL;
    MMD_table * const table   = interp->binop_mmd_funcs + func_nr;
    const UINTVAL     x_funcs = table->x;
    const UINTVAL     y_funcs = table->y;
    INTVAL            r       = right_type;
    funcptr_t         func_;

#if MMD_DEBUG
    fprintf(stderr, "running function %d with left type=%u, right type=%u\n",
        (int) func_nr, (unsigned) left_type, (unsigned) right_type);
#endif

    PARROT_ASSERT(left_type >= 0);
    PARROT_ASSERT(right_type >=0 ||
            (right_type >= enum_type_INTVAL && right_type <= enum_type_PMC));

    if (right_type < 0)
        right_type -= enum_type_INTVAL;
    else
        right_type += 4;

    if ((UINTVAL)left_type < x_funcs && (UINTVAL)right_type < y_funcs) {
        const UINTVAL offset = x_funcs * right_type + left_type;
        func                 = table->mmd_funcs[offset];
    }

    if (!func) {
        const char * const meth_c = Parrot_MMD_method_name(interp, func_nr);
        STRING     * const meth_s = const_string(interp, meth_c);
        PMC        * const method = Parrot_MMD_search_default_infix(interp,
                                        meth_s, left_type, r);
        if (!method)
            Parrot_ex_throw_from_c_args(interp, 0, 1, "MMD function %s not found "
                "for types (%d, %d)", meth_c, left_type, r);


        if (method->vtable->base_type == enum_class_NCI) {
            /* C function is at struct_val */
            func    = D2FPTR(PMC_struct_val(method));
            *is_pmc = 0;
            Parrot_mmd_register(interp, func_nr, left_type, r,
                    (funcptr_t)PMC_struct_val(method));
        }
        else {
            *is_pmc = 1;
            func    = D2FPTR(method);
            Parrot_mmd_register_sub(interp, func_nr, left_type, r, method);
        }

        return func;
    }

    *is_pmc = (UINTVAL)func & 3;
    func_   = (funcptr_t)((UINTVAL)func & ~3);

#ifndef PARROT_HAS_ALIGNED_FUNCPTR
    if (!*is_pmc) {
        return func;
    }
    else if (!is_pmc_ptr(interp, F2DPTR(func_))) {
        *is_pmc = 0;
        return func;
    }
#endif
    return func_;
}


/*

=item C<PMC* Parrot_mmd_find_multi_from_sig_obj>

Collect a list of possible candidates for a given sub name and call signature.
Rank the possible candidates by Manhattan Distance, and return the best
matching candidate. The candidate list is cached in the CallSignature object,
to allow for iterating through it.

Currently this only looks in the global "MULTI" namespace.

=cut

*/

PARROT_EXPORT
PARROT_WARN_UNUSED_RESULT
PARROT_CANNOT_RETURN_NULL
PMC*
Parrot_mmd_find_multi_from_sig_obj(PARROT_INTERP, ARGIN(STRING *name), ARGIN(PMC *invoke_sig))
{
    PMC *candidate_list = pmc_new(interp, enum_class_ResizablePMCArray);

    mmd_search_by_sig_obj(interp, name, invoke_sig, candidate_list);
    mmd_search_global(interp, name, candidate_list);

    return Parrot_mmd_sort_manhattan_by_sig_pmc(interp, candidate_list, invoke_sig);

    if (PMC_IS_NULL(candidate_list))
        return PMCNULL;

#if MMD_DEBUG
    fprintf(stderr, "there were '%d' elements in the candidate_list\n",
            (int) VTABLE_elements(interp, candidate_list));
#endif

    return VTABLE_get_pmc_keyed_int(interp, candidate_list, 0);

}


/*

=item C<static void Parrot_mmd_ensure_writable>

Make sure C<pmc> is writable enough for C<function>.

{{**DEPRECATE**}}

=cut

*/

static void
Parrot_mmd_ensure_writable(PARROT_INTERP, INTVAL function, ARGIN_NULLOK(const PMC *pmc))
{
    if (!PMC_IS_NULL(pmc) && (pmc->vtable->flags & VTABLE_IS_READONLY_FLAG))
        Parrot_ex_throw_from_c_args(interp, 0, 1, "%s applied to read-only argument",
            Parrot_MMD_method_name(interp, function));
}

/*

=item C<PMC* Parrot_build_sig_object_from_varargs>

Take a varargs list, and convert it into a CallSignature PMC. The CallSignature
stores the original short signature string, and an array of integer types to
pass on to the multiple dispatch search.

=cut

*/

PARROT_EXPORT
PARROT_WARN_UNUSED_RESULT
PARROT_CANNOT_RETURN_NULL
PMC*
Parrot_build_sig_object_from_varargs(PARROT_INTERP, ARGIN(const char *sig), va_list args)
{
    PMC         *type_tuple    = PMCNULL;
    PMC         *returns       = PMCNULL;
    PMC         *call_object   = pmc_new(interp, enum_class_CallSignature);
    STRING      *string_sig    = const_string(interp, sig);
    const INTVAL sig_len       = string_length(interp, string_sig);
    INTVAL       in_return_sig = 0;
    INTVAL       i;

    /* Protect call signature object from collection. */
    dod_register_pmc(interp, call_object);

    if (!sig_len)
        return call_object;

    VTABLE_set_string_native(interp, call_object, string_sig);

    for (i = 0; i < sig_len; ++i) {
        INTVAL type = string_index(interp, string_sig, i);

        /* Only create the returns array if it's needed */
        if (in_return_sig && PMC_IS_NULL(returns)) {
            returns = pmc_new(interp, enum_class_ResizablePMCArray);
            VTABLE_set_attr_str(interp, call_object, CONST_STRING(interp, "results"), returns);
        }

        if (in_return_sig) {
            STRING *signature = CONST_STRING(interp, "signature");
            /* Returns store the original passed-in pointer, so they can pass
             * the result back to the caller. */
            PMC *val_pointer = pmc_new(interp, enum_class_CPointer);
            VTABLE_push_pmc(interp, returns, val_pointer);

            switch (type) {
                case 'I':
                    VTABLE_set_pointer(interp, val_pointer, (void *) va_arg(args, INTVAL*));
                    VTABLE_set_string_keyed_str(interp, val_pointer, signature, CONST_STRING(interp, "I"));
                    break;
                case 'N':
                    VTABLE_set_pointer(interp, val_pointer, (void *) va_arg(args, FLOATVAL*));
                    VTABLE_set_string_keyed_str(interp, val_pointer, signature, CONST_STRING(interp, "N"));
                    break;
                case 'S':
                    VTABLE_set_pointer(interp, val_pointer, (void *) va_arg(args, STRING**));
                    VTABLE_set_string_keyed_str(interp, val_pointer, signature, CONST_STRING(interp, "S"));
                    break;
                case 'P':
                    VTABLE_set_pointer(interp, val_pointer, (void *) va_arg(args, PMC**));
                    VTABLE_set_string_keyed_str(interp, val_pointer, signature, CONST_STRING(interp, "P"));
                    break;
                default:
                    Parrot_ex_throw_from_c_args(interp, NULL,
                        EXCEPTION_INVALID_OPERATION,
                        "Multiple Dispatch: invalid argument type %c!", type);
             }
        }
        else {
            /* Regular arguments just set the value */
            switch (type) {
                case 'I':
                    VTABLE_push_integer(interp, call_object, va_arg(args, INTVAL));
                    break;
                case 'N':
                    VTABLE_push_float(interp, call_object, va_arg(args, FLOATVAL));
                    break;
                case 'S':
                    VTABLE_push_string(interp, call_object, va_arg(args, STRING *));
                    break;
                case 'P':
                {
                    PMC *pmc_arg = va_arg(args, PMC *);
                    VTABLE_push_pmc(interp, call_object, pmc_arg);
                    break;
                }
                case '-':
                    i++; /* skip '>' */
                    in_return_sig = 1;
                    break;
                default:
                    Parrot_ex_throw_from_c_args(interp, NULL,
                        EXCEPTION_INVALID_OPERATION,
                        "Multiple Dispatch: invalid argument type %c!", type);
            }
        }
    }

    type_tuple = Parrot_mmd_build_type_tuple_from_sig_obj(interp, call_object);
    VTABLE_set_pmc(interp, call_object, type_tuple);
    return call_object;
}

/*

=item C<void Parrot_mmd_multi_dispatch_from_c_args>

Dispatch to a MultiSub, from a variable-sized list of C arguments. The multiple
dispatch system will figure out which sub should be called based on the types
of the arguments passed in.

Return arguments must be passed as a reference to the PMC, string, number, or
integer, so the result can be set.

=cut

*/

PARROT_EXPORT
PARROT_CAN_RETURN_NULL
void
Parrot_mmd_multi_dispatch_from_c_args(PARROT_INTERP,
        ARGIN(const char *name), ARGIN(const char *sig), ...)
{
    PMC *sig_object, *sub;

    va_list args;
    va_start(args, sig);
    sig_object = Parrot_build_sig_object_from_varargs(interp, sig, args);
    va_end(args);

    /* Check the cache. */
    sub = Parrot_mmd_cache_lookup_by_types(interp, interp->op_mmd_cache, name,
            VTABLE_get_pmc(interp, sig_object));
    if (PMC_IS_NULL(sub)) {
        sub = Parrot_mmd_find_multi_from_sig_obj(interp, const_string(interp, name), sig_object);
        if (!PMC_IS_NULL(sub))
            Parrot_mmd_cache_store_by_types(interp, interp->op_mmd_cache, name,
                    VTABLE_get_pmc(interp, sig_object), sub);
    }

    if (PMC_IS_NULL(sub))
        Parrot_ex_throw_from_c_args(interp, NULL, EXCEPTION_METH_NOT_FOUND,
                "Multiple Dispatch: No suitable candidate found for '%s',"
                " with signature '%s'", name, sig);

#if MMD_DEBUG
    fprintf(stderr, "candidate found for '%s', with signature '%s'\n", name, sig);
    fprintf(stderr, "type of candidate found: %s\n",
            string_to_cstring(interp, VTABLE_name(interp, sub)));
#endif

    Parrot_pcc_invoke_sub_from_sig_object(interp, sub, sig_object);
    dod_unregister_pmc(interp, sig_object);
}


/*

=item C<void mmd_add_function>

Add a new binary MMD function to the list of functions the MMD system knows
of. C<func_num> is the number of the new function. C<function> is ignored.

RT #45941 change this to a MMD register interface that takes a function *name*.

{{**DEPRECATE**}}

=cut

*/

PARROT_EXPORT
void
Parrot_mmd_add_function(PARROT_INTERP, INTVAL func_nr, SHIM(funcptr_t function))
{
    /* XXX Something looks wrong here.  n_binop_mmd_funcs gets incremented,
     * but the function doesn't get saved */
    if (func_nr >= (INTVAL)interp->n_binop_mmd_funcs) {
        INTVAL i;
        mem_realloc_n_typed(interp->binop_mmd_funcs, func_nr+1, MMD_table);

        for (i = interp->n_binop_mmd_funcs; i <= func_nr; ++i)  {
            MMD_table * const table = interp->binop_mmd_funcs + i;

            table->x         = 0;
            table->y         = 0;
            table->mmd_funcs = NULL;
        }

        interp->n_binop_mmd_funcs = func_nr + 1;
    }
}


/*

=item C<static void mmd_expand_x>

Expands the function table in the X dimension to include C<new_x>.

{{**DEPRECATE**}}

=cut

*/

static void
mmd_expand_x(PARROT_INTERP, INTVAL func_nr, INTVAL new_x)
{
    funcptr_t *new_table;
    UINTVAL x;
    UINTVAL y;
    UINTVAL i;
    char *src_ptr, *dest_ptr;
    size_t old_dp, new_dp;
    MMD_table * const table = interp->binop_mmd_funcs + func_nr;

    /* Is the Y 0? If so, nothing to expand, so just set the X for later use */
    if (table->y == 0) {
        table->x = new_x;
        return;
    }

    /* The Y is not zero. Bleah. This means we have to expand the
       table in an unpleasant way. */

    x = table->x;
    y = table->y;

    /* First, fill in the whole new table with the default function
       pointer. We only really need to do the new part, but... */
    new_table = mem_allocate_n_zeroed_typed(y * new_x, funcptr_t);

    /* Then copy the old table over. We have to do this row by row,
       because the rows in the old and new tables are different lengths */
    src_ptr  = (char *)table->mmd_funcs;
    dest_ptr = (char *)new_table;
    old_dp   = sizeof (funcptr_t) * x;
    new_dp   = sizeof (funcptr_t) * new_x;

    for (i = 0; i < y; i++) {
        STRUCT_COPY_N(dest_ptr, src_ptr, x);
        src_ptr  += old_dp;
        dest_ptr += new_dp;
    }

    if (table->mmd_funcs)
        mem_sys_free(table->mmd_funcs);

    table->x = new_x;

    /* Set the old table to point to the new table */
    table->mmd_funcs = new_table;
}


/*

=item C<static void mmd_expand_y>

Expands the function table in the Y direction.

{{**DEPRECATE**}}

=cut

*/

static void
mmd_expand_y(PARROT_INTERP, INTVAL func_nr, INTVAL new_y)
{
    UINTVAL           new_size, old_size;
    MMD_table * const table = interp->binop_mmd_funcs + func_nr;

    PARROT_ASSERT(table->x);

    old_size = sizeof (funcptr_t) * table->x * table->y;
    new_size = sizeof (funcptr_t) * table->x * new_y;

    if (table->mmd_funcs)
        table->mmd_funcs = (funcptr_t *)mem_sys_realloc_zeroed(
            table->mmd_funcs, new_size, old_size);
    else
        table->mmd_funcs = (funcptr_t *)mem_sys_allocate_zeroed(new_size);

    table->y = new_y;
}


/*

=item C<void Parrot_mmd_register>

Register a function C<funcptr> for MMD function table C<func_num> for classes
C<left_type> and C<right_type>. The left and right types are C<INTVAL>s that
represent the class ID numbers.

The function table must exist, but if it is too small, it will
automatically be expanded.

Adding a new function to the table can be interestingly non-trivial, so
we get to be tricky.

If the left or right types are larger than anything we've seen so far,
it means that we have to expand the table. Making Y larger is simple --
just realloc with some more rows. Making X larger is less simple. In
either case, we punt to other functions.

RT #45943 - Currently the MMD system doesn't handle inheritance and best match
searching, as it assumes that all PMC types have no parent type. This
can be considered a bug, and will be resolved at some point in the
future.

{{**DEPRECATE**}}

=cut

*/

PARROT_EXPORT
void
Parrot_mmd_register(PARROT_INTERP, INTVAL func_nr, INTVAL left_type, INTVAL right_type,
             NULLOK(funcptr_t funcptr))
{

    INTVAL     offset;
    MMD_table *table;

    PARROT_ASSERT(func_nr < (INTVAL)interp->n_binop_mmd_funcs);
    PARROT_ASSERT(left_type >= 0);
    PARROT_ASSERT(right_type >=0 ||
            (right_type >= enum_type_INTVAL && right_type <= enum_type_PMC));

    if (right_type < 0)
        right_type -= enum_type_INTVAL;
    else
        right_type += 4;

    table = interp->binop_mmd_funcs + func_nr;

    if ((INTVAL)table->x <= left_type)
        mmd_expand_x(interp, func_nr, left_type + 1);

    if ((INTVAL)table->y <= right_type)
        mmd_expand_y(interp, func_nr, right_type + 1);

    offset = table->x * right_type + left_type;
    table->mmd_funcs[offset] = funcptr;
}


/*

=item C<void Parrot_mmd_register_sub>

RT #48260: Not yet documented!!!

{{**DEPRECATE**}}

=cut

*/

PARROT_EXPORT
void
Parrot_mmd_register_sub(PARROT_INTERP, INTVAL func_nr,
             INTVAL left_type, INTVAL right_type, ARGIN(const PMC *sub))
{
    if (sub->vtable->base_type == enum_class_NCI) {
        Parrot_mmd_register(interp, func_nr, left_type, right_type,
                D2FPTR(PMC_struct_val(sub)));
    }
    else {
        PMC * const fake = (PMC *)((UINTVAL) sub | 1);
        Parrot_mmd_register(interp, func_nr, left_type, right_type, D2FPTR(fake));
    }
}


/*

=item C<void mmd_destroy>

Frees all the memory allocated used the MMD subsystem.

{{**DEPRECATE**}}

=cut

*/

PARROT_EXPORT
void
Parrot_mmd_destroy(PARROT_INTERP)
{
    if (interp->n_binop_mmd_funcs) {
        UINTVAL i;
        for (i = 0; i <interp->n_binop_mmd_funcs; ++i) {
            if (interp->binop_mmd_funcs[i].mmd_funcs) {
                mem_sys_free(interp->binop_mmd_funcs[i].mmd_funcs);
                interp->binop_mmd_funcs[i].mmd_funcs = NULL;
            }
        }
    }
    mem_sys_free(interp->binop_mmd_funcs);
    interp->binop_mmd_funcs = NULL;
}


/*

=item C<PMC * Parrot_mmd_find_multi_from_long_sig>

Find the best candidate multi for a given sub name and signature. The signature
is a string containing a comma-delimited list of type names.

Currently only searches the global MULTI namespace.

=cut

*/

PARROT_EXPORT
PARROT_CANNOT_RETURN_NULL
PARROT_WARN_UNUSED_RESULT
PMC *
Parrot_mmd_find_multi_from_long_sig(PARROT_INTERP, ARGIN(STRING *name),
        ARGIN(STRING *long_sig))
{
    PMC *type_tuple, *candidate_list;
    PMC * const namespace_ = Parrot_make_namespace_keyed_str(
            interp, interp->root_namespace,
            CONST_STRING(interp, "MULTI"));
    PMC *multi_sub = Parrot_get_global(interp, namespace_, name);

    if (PMC_IS_NULL(multi_sub))
        return PMCNULL;

    type_tuple = mmd_build_type_tuple_from_long_sig(interp, long_sig);

    candidate_list = VTABLE_clone(interp, multi_sub);

    return Parrot_mmd_sort_candidates(interp, type_tuple, candidate_list);
}


/*

=item C<PMC * Parrot_MMD_search_default_infix>

RT #48260: Not yet documented!!!

{{**DEPRECATE**}}

=cut

*/

PARROT_EXPORT
PARROT_CANNOT_RETURN_NULL
PARROT_WARN_UNUSED_RESULT
PMC *
Parrot_MMD_search_default_infix(PARROT_INTERP, ARGIN(STRING *meth),
        INTVAL left_type, INTVAL right_type)
{
    PMC* const arg_tuple = pmc_new(interp, enum_class_FixedIntegerArray);

    VTABLE_set_integer_native(interp,    arg_tuple, 2);
    VTABLE_set_integer_keyed_int(interp, arg_tuple, 0, left_type);
    VTABLE_set_integer_keyed_int(interp, arg_tuple, 1, right_type);

    return Parrot_mmd_search_default(interp, meth, arg_tuple);
}


/*

=item C<PMC * Parrot_mmd_sort_manhattan_by_sig_pmc>

Given an array PMC (usually a MultiSub) and a CallSignature PMC, sorts the mmd
candidates by their manhattan distance to the signature args and returns the
best one.

=cut

*/

PARROT_EXPORT
PARROT_CAN_RETURN_NULL
PARROT_WARN_UNUSED_RESULT
PMC *
Parrot_mmd_sort_manhattan_by_sig_pmc(PARROT_INTERP, ARGIN(PMC *candidates),
        ARGIN(PMC* invoke_sig))
{
    INTVAL n         = VTABLE_elements(interp, candidates);
    PMC   *arg_tuple = VTABLE_get_pmc(interp, invoke_sig);

    if (!n)
        return PMCNULL;

    return Parrot_mmd_sort_candidates(interp, arg_tuple, candidates);
}

/*

=item C<PMC * Parrot_mmd_sort_manhattan>

Given an array PMC (usually a MultiSub) sorts the mmd candidates by their
manhattan distance to the current args and returns the best one.

=cut

*/
PARROT_EXPORT
PARROT_CAN_RETURN_NULL
PARROT_WARN_UNUSED_RESULT
PMC *
Parrot_mmd_sort_manhattan(PARROT_INTERP, ARGIN(PMC *candidates))
{
    PMC   *arg_tuple;
    INTVAL n = VTABLE_elements(interp, candidates);

    if (!n)
        return PMCNULL;

    arg_tuple  = Parrot_mmd_arg_tuple_func(interp);
    candidates = VTABLE_clone(interp, candidates);

    return Parrot_mmd_sort_candidates(interp, arg_tuple, candidates);
}


/*

=item C<static PMC* Parrot_mmd_arg_tuple_func>

Return a list of argument types. PMC arguments are taken from registers
according to calling conventions.

=cut

*/

PARROT_WARN_UNUSED_RESULT
PARROT_CANNOT_RETURN_NULL
static PMC*
Parrot_mmd_arg_tuple_func(PARROT_INTERP)
{
    INTVAL sig_len, i, type;
    PMC                *arg;
    PMC                *args_array;    /* from recent set_args opcode */
    PackFile_Constant **constants;

    /*
     * if there is no signature e.g. because of
     *      m = getattribute l, "__add"
     * - we have to return the MultiSub
     * - create a BoundMulti
     * - dispatch in invoke - yeah ugly
     */

    PMC * const arg_tuple = pmc_new(interp, enum_class_ResizableIntegerArray);
    opcode_t   *args_op   = interp->current_args;

    if (!args_op)
        return arg_tuple;

    PARROT_ASSERT(*args_op == PARROT_OP_set_args_pc);
    constants  = interp->code->const_table->constants;
    ++args_op;
    args_array = constants[*args_op]->u.key;

    ASSERT_SIG_PMC(args_array);

    sig_len = SIG_ELEMS(args_array);
    if (!sig_len)
        return arg_tuple;

    ++args_op;

    for (i = 0; i < sig_len; ++i, ++args_op) {
        type = SIG_ITEM(args_array, i);

        /* named don't MMD */
        if (type & PARROT_ARG_NAME)
            break;
        switch (type & (PARROT_ARG_TYPE_MASK | PARROT_ARG_FLATTEN)) {
            case PARROT_ARG_INTVAL:
                VTABLE_push_integer(interp, arg_tuple, enum_type_INTVAL);
                break;
            case PARROT_ARG_FLOATVAL:
                VTABLE_push_integer(interp, arg_tuple, enum_type_FLOATVAL);
                break;
            case PARROT_ARG_STRING:
                VTABLE_push_integer(interp, arg_tuple, enum_type_STRING);
                break;
            case PARROT_ARG_PMC:
                {
                const int idx = *args_op;
                if ((type & PARROT_ARG_CONSTANT))
                    arg = constants[idx]->u.key;
                else
                    arg = REG_PMC(interp, idx);

                if (PMC_IS_NULL(arg))
                    type = enum_type_PMC;
                else
                    type = VTABLE_type(interp, arg);

                VTABLE_push_integer(interp, arg_tuple, type);
                }
                break;
            case PARROT_ARG_FLATTEN | PARROT_ARG_PMC:  {
                /* expand flattening args */
                int j, n;

                const int idx = *args_op;
                arg           = REG_PMC(interp, idx);
                n             = VTABLE_elements(interp, arg);

                for (j = 0; j < n; ++j)  {
                    PMC * const elem = VTABLE_get_pmc_keyed_int(interp, arg, j);
                    type             = VTABLE_type(interp, elem);
                    VTABLE_push_integer(interp, arg_tuple, type);
                }
                break;
            }
            default:
                Parrot_ex_throw_from_c_args(interp, NULL, 1,
                    "Unknown signature type %d in mmd_arg_tuple", type);
                break;
        }
    }


    return arg_tuple;
}


/*

=item C<static PMC* Parrot_mmd_search_default>

Default implementation of MMD search. Search scopes for candidates, walk the
class hierarchy, sort all candidates by their Manhattan distance, and return
result

{{**DEPRECATE?? **}}

=cut

*/

PARROT_CAN_RETURN_NULL
PARROT_WARN_UNUSED_RESULT
static PMC*
Parrot_mmd_search_default(PARROT_INTERP, ARGIN(STRING *meth), ARGIN(PMC *arg_tuple))
{
    /* 2) create a list of matching functions */
    PMC * const candidate_list = Parrot_mmd_search_scopes(interp, meth);

    /*
     * 3) if list is empty fail
     *    if the first found function is a plain Sub: finito
     */
    INTVAL n = VTABLE_elements(interp, candidate_list);

    if (n == 1) {
        PMC    * const pmc  = VTABLE_get_pmc_keyed_int(interp, candidate_list, 0);
        STRING * const _sub = CONST_STRING(interp, "Sub");

        if (VTABLE_isa(interp, pmc, _sub))
            return pmc;
    }

    /*
     * 4) first was a MultiSub - go through all found MultiSubs and check
     *    the first arguments MRO, add all MultiSubs and plain methods,
     *    where the first argument matches
     */
    Parrot_mmd_search_classes(interp, meth, arg_tuple, candidate_list, 0);
    n = VTABLE_elements(interp, candidate_list);
    if (!n)
        return NULL;

    /* 5) sort the list */

    if (n > 1)
        return Parrot_mmd_sort_candidates(interp, arg_tuple, candidate_list);

    return VTABLE_get_pmc_keyed_int(interp, candidate_list, 0);
}


/*

=item C<static void Parrot_mmd_search_classes>

Search all the classes in all MultiSubs of the candidates C<cl> and return
a list of all candidates. C<start_at_parent> is 0 to start at the class itself
or 1 to search from the first parent class.

=cut

*/

static void
Parrot_mmd_search_classes(PARROT_INTERP, ARGIN(STRING *meth),
        ARGIN(PMC *arg_tuple), ARGIN(PMC *cl), INTVAL start_at_parent)
{
    INTVAL type1;

    /* get the class of the first argument */
    if (!VTABLE_elements(interp, arg_tuple))
        return;

    type1 = VTABLE_get_integer_keyed_int(interp, arg_tuple, 0);

    if (type1 < 0) {
        return;
        /* RT #45947 create some class namespace */
    }
    else {
        PMC * const  mro = interp->vtables[type1]->mro;
        const INTVAL n   = VTABLE_elements(interp, mro);
        INTVAL       i;

        for (i = start_at_parent; i < n; ++i) {
            PMC * const _class = VTABLE_get_pmc_keyed_int(interp, mro, i);
            PMC *ns, *methodobj;

            if (PObj_is_class_TEST(_class))
                ns = Parrot_oo_get_namespace(interp, _class);
            else
                ns = VTABLE_get_namespace(interp, _class);

            methodobj = VTABLE_get_pmc_keyed_str(interp, ns, meth);

            if (!PMC_IS_NULL(methodobj)) {
                /*
                 * RT #45949 pass current n so that only candidates from this
                 *     mro are used?
                 */
                if (Parrot_mmd_maybe_candidate(interp, methodobj, cl))
                    break;
            }
        }
    }
}


/*

=item C<static INTVAL distance_cmp>

RT #48260: Not yet documented!!!

=cut

*/

static INTVAL
distance_cmp(SHIM_INTERP, INTVAL a, INTVAL b)
{
    short da = (short)(a & 0xffff);
    short db = (short)(b & 0xffff);

    /* sort first by distance */
    if (da > db)
        return 1;

    if (da < db)
        return -1;

    /* end then by index in candidate list */
    da = (short)(a >> 16);
    db = (short)(b >> 16);

    return da > db ? 1 : da < db ? -1 : 0;
}

/*

=item C<static PMC* mmd_build_type_tuple_from_type_list>

Construct a FixedIntegerArray of type numbers from an array of
type names. Used for multiple dispatch.

=cut

*/

PARROT_CANNOT_RETURN_NULL
PARROT_WARN_UNUSED_RESULT
static PMC*
mmd_build_type_tuple_from_type_list(PARROT_INTERP, ARGIN(PMC *type_list))
{
    INTVAL i, param_count;
    PMC *multi_sig = constant_pmc_new(interp, enum_class_FixedIntegerArray);

    param_count = VTABLE_elements(interp, type_list);
    VTABLE_set_integer_native(interp, multi_sig, param_count);

    for (i = 0; i < param_count; i++) {
        INTVAL type;
        STRING *type_name = VTABLE_get_string_keyed_int(interp, type_list, i);
        if (string_equal(interp, type_name, CONST_STRING(interp, "DEFAULT"))==0)
            type = enum_type_PMC;
        else if (string_equal(interp, type_name, CONST_STRING(interp, "STRING"))==0)
            type = enum_type_STRING;
        else if (string_equal(interp, type_name, CONST_STRING(interp, "INTVAL"))==0)
            type = enum_type_INTVAL;
        else if (string_equal(interp, type_name, CONST_STRING(interp, "FLOATVAL"))==0)
            type = enum_type_FLOATVAL;
        else
            type = pmc_type(interp, type_name);

        VTABLE_set_integer_keyed_int(interp, multi_sig, i, type);
    }
    return multi_sig;
}

/*

=item C<static PMC* mmd_build_type_tuple_from_long_sig>

Construct a FixedIntegerArray of type numbers from a comma-delimited string of
type names. Used for multiple dispatch.

=cut

*/

PARROT_CANNOT_RETURN_NULL
PARROT_WARN_UNUSED_RESULT
static PMC*
mmd_build_type_tuple_from_long_sig(PARROT_INTERP, ARGIN(STRING *long_sig))
{
    PMC *type_list = string_split(interp, CONST_STRING(interp, ","), long_sig);

    return mmd_build_type_tuple_from_type_list(interp, type_list);
}

/*

=item C<PMC* Parrot_mmd_build_type_tuple_from_sig_obj>

Construct a FixedIntegerArray of type numbers from the arguments of a Call
Signature object. Used for multiple dispatch.

=cut

*/

PARROT_EXPORT
PARROT_CANNOT_RETURN_NULL
PARROT_WARN_UNUSED_RESULT
PMC*
Parrot_mmd_build_type_tuple_from_sig_obj(PARROT_INTERP, ARGIN(PMC *sig_obj))
{
    PMC * const type_tuple = pmc_new(interp, enum_class_FixedIntegerArray);
    INTVAL i;
    INTVAL tuple_size = 0;
    INTVAL args_ended = 0;
    STRING *string_sig = VTABLE_get_string(interp, sig_obj);
    const INTVAL sig_len = string_length(interp, string_sig);

    /* First calculate the number of arguments participating in MMD */
    for (i = 0; i < sig_len; ++i) {
        INTVAL type = string_index(interp, string_sig, i);
        if (type == '-')
            break;

        tuple_size++;
    }

    VTABLE_set_integer_native(interp, type_tuple, tuple_size);

    for (i = 0; i < sig_len; ++i) {
        INTVAL type = string_index(interp, string_sig, i);
        if (args_ended)
            break;

        /* Regular arguments just set the value */
        switch (type) {
            case 'I':
                VTABLE_set_integer_keyed_int(interp, type_tuple,
                        i, enum_type_INTVAL);
                break;
            case 'N':
                VTABLE_set_integer_keyed_int(interp, type_tuple,
                        i, enum_type_FLOATVAL);
                break;
            case 'S':
                VTABLE_set_integer_keyed_int(interp, type_tuple,
                        i, enum_type_STRING);
                break;
            case 'P':
            {
                PMC *pmc_arg = VTABLE_get_pmc_keyed_int(interp, sig_obj, i);
                if (PMC_IS_NULL(pmc_arg))
                    VTABLE_set_integer_keyed_int(interp, type_tuple,
                            i, enum_type_PMC);
                else
                    VTABLE_set_integer_keyed_int(interp, type_tuple, i,
                            VTABLE_type(interp, pmc_arg));

                break;
            }
            case '-':
                args_ended = 1;
                break;
            default:
                Parrot_ex_throw_from_c_args(interp, NULL,
                    EXCEPTION_INVALID_OPERATION,
                    "Multiple Dispatch: invalid argument type %c!", type);
        }
    }

    return type_tuple;
}

/*

=item C<static PMC* mmd_cvt_to_types>

Given a ResizablePMCArray PMC containing some form of type identifier (either
the string name of a class or a PMC representing the type), resolves all type
references to type IDs, if possible.  If that's not possible, returns PMCNULL.
In that case you can't dispatch to the multi variant with this type signature,
as Parrot doesn't yet know about the respective types requested -- you have to
register them first.

Otherwise, returns a ResizableIntegerArray PMC full of type IDs representing
the signature of a multi variant to which you may be able to dispatch.

{{**DEPRECATE**}}

=cut

*/

PARROT_CANNOT_RETURN_NULL
PARROT_WARN_UNUSED_RESULT
static PMC*
mmd_cvt_to_types(PARROT_INTERP, ARGIN(PMC *multi_sig))
{
    PMC        *ar = PMCNULL;
    const INTVAL n = VTABLE_elements(interp, multi_sig);
    INTVAL       i;

    for (i = 0; i < n; ++i) {
        PMC * const sig_elem = VTABLE_get_pmc_keyed_int(interp, multi_sig, i);
        INTVAL type;

        if (sig_elem->vtable->base_type == enum_class_String) {
            STRING * const sig = VTABLE_get_string(interp, sig_elem);

            if (!sig)
                return PMCNULL;

            /* yes, this is horrible and ugly */
            if (memcmp(sig->strstart, "__VOID", 6) == 0) {
                if (PMC_IS_NULL(ar)) {
                    ar = pmc_new(interp, enum_class_FixedIntegerArray);
                    VTABLE_set_integer_native(interp, ar, n);
                }
                PMC_int_val(ar)--;  /* RT #45951 */
                break;
            }

            type = pmc_type(interp, sig);

            if (type == enum_type_undef)
                return PMCNULL;
        }
        else
            type = pmc_type_p(interp, sig_elem);

        /* create destination PMC only as necessary */
        if (PMC_IS_NULL(ar)) {
            ar = pmc_new(interp, enum_class_FixedIntegerArray);
            VTABLE_set_integer_native(interp, ar, n);
        }

        VTABLE_set_integer_keyed_int(interp, ar, i, type);
    }

    return ar;
}

static PMC *
Parrot_mmd_get_cached_multi_sig(PARROT_INTERP, ARGIN(PMC *sub))
{
    if (VTABLE_isa(interp, sub, CONST_STRING(interp, "Sub"))) {
        PMC *multi_sig = PMC_sub(sub)->multi_signature;

        if (multi_sig->vtable->base_type == enum_class_FixedPMCArray) {
            PMC *converted_sig = mmd_cvt_to_types(interp, multi_sig);

            if (PMC_IS_NULL(converted_sig))
                return PMCNULL;

            multi_sig = PMC_sub(sub)->multi_signature = converted_sig;
        }

        return multi_sig;
    }

    return PMCNULL;
}


#define MMD_BIG_DISTANCE 0x7fff

/*

=item C<static UINTVAL mmd_distance>

Create Manhattan Distance of sub C<pmc> against given argument types.
0xffff is the maximum distance

=cut

*/

static UINTVAL
mmd_distance(PARROT_INTERP, ARGIN(PMC *pmc), ARGIN(PMC *arg_tuple))
{
    PMC *multi_sig, *mro;
    INTVAL i, n, args, dist, j, m;

    /* has to be a builtin multi method */
    if (pmc->vtable->base_type == enum_class_NCI)
        multi_sig = PMC_pmc_val(pmc);
    else {
        /* not a multi; no distance */
        if (!PMC_sub(pmc)->multi_signature)
            return 0;

        multi_sig = Parrot_mmd_get_cached_multi_sig(interp, pmc);
    }

    if (PMC_IS_NULL(multi_sig))
        return MMD_BIG_DISTANCE;

    n    = VTABLE_elements(interp, multi_sig);
    args = VTABLE_elements(interp, arg_tuple);

    /*
     * arg_tuple may have more arguments - only the
     * n multi_sig invocants are counted
     */
    if (args < n)
        return MMD_BIG_DISTANCE;

    dist = 0;

    if (args > n)
        dist = PARROT_MMD_MAX_CLASS_DEPTH;

    /* now go through args */
    for (i = 0; i < n; ++i) {
        const INTVAL type_sig  = VTABLE_get_integer_keyed_int(interp, multi_sig, i);
        const INTVAL type_call = VTABLE_get_integer_keyed_int(interp, arg_tuple, i);
        if (type_sig == type_call)
            continue;

        /* promote primitives to their PMC equivalents, as PCC will autobox
         * the distance penalty makes primitive variants look cheaper */
        switch (type_call) {
            case enum_type_INTVAL:
                if (type_sig == enum_class_Integer) { dist++; continue; }
                break;
            case enum_type_FLOATVAL:
                if (type_sig == enum_class_Float)   { dist++; continue; }
                break;
            case enum_type_STRING:
                if (type_sig == enum_class_String)  { dist++; continue; }
                break;
            default:
                break;
        }

        /*
         * different native types are very different, except a PMC
         * which matches any PMC
         */
        if (type_call <= 0 && type_sig == enum_type_PMC) {
            dist++;
            continue;
        }

        if ((type_sig <= 0 && type_sig != enum_type_PMC) || type_call <= 0) {
            dist = MMD_BIG_DISTANCE;
            break;
        }

        /*
         * now consider MRO of types the signature type has to be somewhere
         * in the MRO of the type_call
         */
        mro = interp->vtables[type_call]->mro;
        m   = VTABLE_elements(interp, mro);

        for (j = 0; j < m; ++j) {
            PMC * const cl = VTABLE_get_pmc_keyed_int(interp, mro, j);

            if (cl->vtable->base_type == type_sig)
                break;
            if (VTABLE_type(interp, cl) == type_sig)
                break;

            ++dist;
        }

        /*
         * if the type wasn't in MRO check, if any PMC matches
         * in that case use the distance + 1 (of an any PMC parent)
         */
        if (j == m && type_sig != enum_type_PMC) {
            dist = MMD_BIG_DISTANCE;
            break;
        }

        ++dist;
#if MMD_DEBUG
        {
            STRING *s1, *s2;
            if (type_sig < 0)
                s1 = Parrot_get_datatype_name(interp, type_sig);
            else {
                s1 = interp->vtables[type_sig]->whoami;
            }
            if (type_call < 0)
                s2 = Parrot_get_datatype_name(interp, type_call);
            else {
                s2 = interp->vtables[type_call]->whoami;
            }
            PIO_eprintf(interp, "arg %d: dist %d sig %Ss arg %Ss\n",
                i, dist, s1, s2);
        }
#endif
    }

    return dist;
}


/*

=item C<static PMC * Parrot_mmd_sort_candidates>

Sort the candidate list C<cl> by Manhattan Distance, returning the best
candidate.

=cut

*/

static PMC *
Parrot_mmd_sort_candidates(PARROT_INTERP, ARGIN(PMC *arg_tuple), ARGIN(PMC *cl))
{
    INTVAL       i;
    const INTVAL n              = VTABLE_elements(interp, cl);
    INTVAL       best_distance  = MMD_BIG_DISTANCE;
    PMC         *best_candidate = PMCNULL;

    for (i = 0; i < n; ++i) {
        PMC * const  pmc = VTABLE_get_pmc_keyed_int(interp, cl, i);
        const INTVAL d   = mmd_distance(interp, pmc, arg_tuple);
        if (d < best_distance) {
            best_candidate = pmc;
            best_distance  = d;
        }
    }

    return best_candidate;
}


/*

=item C<static PMC* Parrot_mmd_search_scopes>

Search all scopes for MMD candidates matching the arguments given in
C<arg_tuple>.

=cut

*/

PARROT_CANNOT_RETURN_NULL
PARROT_WARN_UNUSED_RESULT
static PMC*
Parrot_mmd_search_scopes(PARROT_INTERP, ARGIN(STRING *meth))
{
    PMC * const candidates = pmc_new(interp, enum_class_ResizablePMCArray);

    const int stop         = mmd_search_local(interp, meth, candidates);

    if (!stop)
        mmd_search_global(interp, meth, candidates);

    return candidates;
}


/*

=item C<static int mmd_maybe_candidate>

If the candidate C<pmc> is a Sub PMC, push it on the candidate list and
return TRUE to stop further search.

If the candidate is a MultiSub remember all matching Subs and return FALSE
to continue searching outer scopes.

=cut

*/

static int
Parrot_mmd_maybe_candidate(PARROT_INTERP, ARGIN(PMC *pmc), ARGIN(PMC *cl))
{
    INTVAL i, n;

    STRING * const _sub       = CONST_STRING(interp, "Sub");
    STRING * const _multi_sub = CONST_STRING(interp, "MultiSub");

    if (VTABLE_isa(interp, pmc, _sub)) {
        /* a plain sub stops outer searches */
        /* RT #45959 check arity of sub */

        VTABLE_push_pmc(interp, cl, pmc);
        return 1;
    }

    /* not a Sub or MultiSub - ignore */
    if (!VTABLE_isa(interp, pmc, _multi_sub))
        return 0;

    /* ok we have a multi sub pmc, which is an array of candidates */
    n = VTABLE_elements(interp, pmc);

    for (i = 0; i < n; ++i) {
        PMC * const multi_sub = VTABLE_get_pmc_keyed_int(interp, pmc, i);
        VTABLE_push_pmc(interp, cl, multi_sub);
    }

    return 0;
}


/*

=item C<static int mmd_search_local>

Search the current package namespace for matching candidates. Return
TRUE if the MMD search should stop.

=cut

*/

static int
mmd_search_local(PARROT_INTERP, ARGIN(STRING *name), ARGIN(PMC *candidates))
{
    PMC * const multi_sub = Parrot_find_global_cur(interp, name);

    return multi_sub && Parrot_mmd_maybe_candidate(interp, multi_sub, candidates);
}

/*

=item C<static void mmd_search_by_sig_obj>

Search the namespace of the first argument to the sub call for matching
candidates.

=cut

*/

static void
mmd_search_by_sig_obj(PARROT_INTERP, ARGIN(STRING *name),
        ARGIN(PMC *sig_obj), ARGIN(PMC *candidates))
{
    PMC *namespace_, *multi_sub;
    PMC *first_arg = VTABLE_get_pmc_keyed_int(interp, sig_obj, 0);

    if (PMC_IS_NULL(first_arg))
        return;

    namespace_ = VTABLE_get_namespace(interp, first_arg);

    if (PMC_IS_NULL(namespace_))
        return;

    multi_sub = Parrot_get_global(interp, namespace_, name);

    if (PMC_IS_NULL(multi_sub))
        return;

    Parrot_mmd_maybe_candidate(interp, multi_sub, candidates);
}


/*

=item C<static void mmd_search_global>

Search the builtin namespace for matching candidates.

=cut

*/

static void
mmd_search_global(PARROT_INTERP, ARGIN(STRING *name), ARGIN(PMC *cl))
{
    PMC *multi_sub;
    PMC * const namespace_ = Parrot_get_namespace_keyed_str(
            interp, interp->root_namespace,
            CONST_STRING(interp, "MULTI"));

    multi_sub = Parrot_get_global(interp, namespace_, name);

    if (PMC_IS_NULL(multi_sub))
        return;

    Parrot_mmd_maybe_candidate(interp, multi_sub, cl);
}


/*

=item C<static void mmd_add_multi_global>

Create a MultiSub, or add a variant to an existing MultiSub. The MultiSub is
stored in the global MULTI namespace.

=cut

*/

static void
mmd_add_multi_global(PARROT_INTERP, ARGIN(STRING *sub_name), ARGIN(PMC *sub_obj))
{
        PMC * const namespace_ = Parrot_make_namespace_keyed_str(
            interp, interp->root_namespace,
            CONST_STRING(interp, "MULTI"));
        PMC *multi_sub = Parrot_get_global(interp, namespace_, sub_name);

        if (PMC_IS_NULL(multi_sub)) {
            multi_sub = constant_pmc_new(interp, enum_class_MultiSub);
            Parrot_set_global(interp, namespace_, sub_name, multi_sub);
        }

        PARROT_ASSERT(multi_sub->vtable->base_type == enum_class_MultiSub);
        VTABLE_push_pmc(interp, multi_sub, sub_obj);
}

/*

=item C<static void mmd_add_multi_to_namespace>

Create a MultiSub, or add a variant to an existing MultiSub. The MultiSub is
added as a method to a class.

=cut

*/

static void
mmd_add_multi_to_namespace(PARROT_INTERP, ARGIN(STRING *ns_name),
            ARGIN(STRING *sub_name), ARGIN(PMC *sub_obj))
{
        PMC * const hll_ns    = VTABLE_get_pmc_keyed_int(interp,
                interp->HLL_namespace, CONTEXT(interp)->current_HLL);
        PMC * const namespace_ = Parrot_make_namespace_keyed_str(interp, hll_ns, ns_name);
        PMC *multi_sub = Parrot_get_global(interp, namespace_, sub_name);

        if (PMC_IS_NULL(multi_sub)) {
            multi_sub = constant_pmc_new(interp, enum_class_MultiSub);
            Parrot_set_global(interp, namespace_, sub_name, multi_sub);
        }

        PARROT_ASSERT(multi_sub->vtable->base_type == enum_class_MultiSub);
        VTABLE_push_pmc(interp, multi_sub, sub_obj);
}

/*

=item C<void Parrot_mmd_add_multi_from_long_sig>

Create a MultiSub, or add a variant to an existing MultiSub. The MultiSub is
stored in the global MULTI namespace.

=cut

*/

PARROT_EXPORT
void
Parrot_mmd_add_multi_from_long_sig(PARROT_INTERP,
        ARGIN(STRING *sub_name), ARGIN(STRING *long_sig),
        ARGIN(PMC *sub_obj))
{
        PMC *type_list = string_split(interp, CONST_STRING(interp, ","), long_sig);
        STRING *namespace_name = VTABLE_get_string_keyed_int(interp, type_list, 0);

        /* Attach a type tuple array to the sub for multi dispatch */
        PMC *multi_sig = mmd_build_type_tuple_from_type_list(interp, type_list);

        if (sub_obj->vtable->base_type == enum_class_NCI) {
            PMC_pmc_val(sub_obj) = multi_sig;
        }
        else if (VTABLE_isa(interp, sub_obj, CONST_STRING(interp, "Sub"))
             ||  VTABLE_isa(interp, sub_obj, CONST_STRING(interp, "Closure"))) {
            PMC_sub(sub_obj)->multi_signature = multi_sig;
        }

        mmd_add_multi_to_namespace(interp, namespace_name, sub_name, sub_obj);
        mmd_add_multi_global(interp, sub_name, sub_obj);
}

/*

=item C<void Parrot_mmd_add_multi_from_c_args>

Create a MultiSub, or add a variant to an existing MultiSub. The MultiSub is
stored in the specified namespace.

=cut

*/

PARROT_EXPORT
void
Parrot_mmd_add_multi_from_c_args(PARROT_INTERP,
        ARGIN(const char *sub_name),
        ARGIN(const char *short_sig),
        ARGIN(const char *long_sig),
        ARGIN(funcptr_t multi_func_ptr))
{
        STRING *comma          = CONST_STRING(interp, ",");
        STRING *sub_name_str   = string_from_cstring(interp, sub_name, 0);
        STRING *long_sig_str   = string_from_cstring(interp, long_sig, 0);
        STRING *short_sig_str  = string_from_cstring(interp, short_sig, 0);
        PMC    *type_list      = string_split(interp, comma, long_sig_str);
        STRING *namespace_name = VTABLE_get_string_keyed_int(interp, type_list, 0);
        /* Create an NCI sub for the C function */
        PMC    *sub_obj        = constant_pmc_new(interp, enum_class_NCI);
        PMC    *multi_sig      = mmd_build_type_tuple_from_long_sig(interp,
                                    long_sig_str);

        VTABLE_set_pointer_keyed_str(interp, sub_obj, short_sig_str,
                                        F2DPTR(multi_func_ptr));

        /* Attach a type tuple array to the NCI sub for multi dispatch */
        PMC_pmc_val(sub_obj) = multi_sig;

        mmd_add_multi_to_namespace(interp, namespace_name, sub_name_str, sub_obj);
        mmd_add_multi_global(interp, sub_name_str, sub_obj);
}

/*

=item C<void Parrot_mmd_add_multi_list_from_c_args>

Create a collection of multiple dispatch subs from a C structure of
information. Iterate through the list of details passed in. For each entry
create a MultiSub or add a variant to an existing MultiSub. MultiSubs are
created in the global 'MULTI' namespace in the Parrot HLL.

Typically used to create all the multiple dispatch routines
declared in a PMC from the PMC's class initialization function.

=cut

*/

PARROT_EXPORT
void
Parrot_mmd_add_multi_list_from_c_args(PARROT_INTERP,
        ARGIN(const multi_func_list *mmd_info), INTVAL elements)
{
    INTVAL i;
    for (i = 0; i < elements; ++i) {
#ifdef PARROT_HAS_ALIGNED_FUNCPTR
        PARROT_ASSERT((PTR2UINTVAL(mmd_info[i].func_ptr) & 3) == 0);
#endif
        Parrot_mmd_add_multi_from_c_args(interp,
                mmd_info[i].multi_name,
                mmd_info[i].short_sig,
                mmd_info[i].full_sig,
                mmd_info[i].func_ptr);
    }
}


/*

=item C<MMD_Cache * Parrot_mmd_cache_create>

Creates and returns a new MMD cache.

=cut

*/

PARROT_EXPORT
MMD_Cache *
Parrot_mmd_cache_create(PARROT_INTERP) {
    /* String hash. */
    Hash* cache;
    parrot_new_hash(interp, &cache);
    return cache;
}


/*

=item C<static STRING * mmd_cache_key_from_values>

Generates an MMD cache key from an array of values.

=cut

*/

static STRING *
mmd_cache_key_from_values(PARROT_INTERP, const char *name, PMC *values)
{
    /* Build array of type IDs, which we'll then use as a string to key into
     * the hash. */
    INTVAL i;
    INTVAL num_values = VTABLE_elements(interp, values);
    INTVAL name_len   = name ? strlen(name) + 1: 0;
    INTVAL *type_ids  = (INTVAL *)mem_sys_allocate(num_values
                            * sizeof(INTVAL) + name_len);
    STRING *key;

    for (i = 0; i < num_values; i++) {
        INTVAL id = VTABLE_type(interp, VTABLE_get_pmc_keyed_int(interp, values, i));
        if (id == 0)
            return NULL;
        type_ids[i] = id;
    }

    if (name)
        strcpy((char *)(type_ids + num_values), name);

    key = string_from_cstring(interp, (char *)type_ids,
                             num_values * sizeof(INTVAL) + name_len);
    mem_sys_free(type_ids);

    return key;
}


/*

=item C<PMC * Parrot_mmd_cache_lookup_by_values>

Takes an array of values for the call and does a lookup in the MMD cache.

=cut

*/

PARROT_EXPORT
PMC *
Parrot_mmd_cache_lookup_by_values(PARROT_INTERP, MMD_Cache *cache, const char *name, PMC *values)
{
    STRING *key = mmd_cache_key_from_values(interp, name, values);

    if (key)
        return (PMC *)parrot_hash_get(interp, cache, key);

    return PMCNULL;
}


/*

=item C<void Parrot_mmd_cache_store_by_values>

Takes an array of values for the call along with a chosen candidate and puts
it into the cache.

=cut

*/

PARROT_EXPORT
void
Parrot_mmd_cache_store_by_values(PARROT_INTERP, MMD_Cache *cache, const char *name, PMC *values, PMC *chosen)
{
    STRING *key = mmd_cache_key_from_values(interp, name, values);

    if (key)
        parrot_hash_put(interp, cache, key, chosen);
}


/*

=item C<static STRING * mmd_cache_key_from_values>

Generates an MMD cache key from an array of values.

=cut

*/

static STRING *
mmd_cache_key_from_types(PARROT_INTERP, const char *name, PMC *types)
{
    /* Build array of type IDs, which we'll then use as a string to key into
     * the hash. */
    STRING *key;
    INTVAL  num_types = VTABLE_elements(interp, types);
    INTVAL  name_len  = name ? strlen(name) + 1: 0;
    INTVAL *type_ids  = (INTVAL *)mem_sys_allocate(num_types
                                                   * sizeof(INTVAL) + name_len);
    INTVAL  i;

    for (i = 0; i < num_types; i++) {
        INTVAL id   = VTABLE_get_integer_keyed_int(interp, types, i);

        if (id == 0)
            return NULL;

        type_ids[i] = id;
    }
    if (name)
        strcpy((char*)(type_ids + num_types), name);
    key = string_from_cstring(interp, (char*)type_ids, num_types * sizeof(INTVAL) + name_len);
    mem_sys_free(type_ids);
    return key;
}


/*

=item C<PMC * Parrot_mmd_cache_lookup_by_types>

Takes an array of types for the call and does a lookup in the MMD cache.

=cut

*/

PARROT_EXPORT
PMC *
Parrot_mmd_cache_lookup_by_types(PARROT_INTERP, MMD_Cache *cache,
    const char *name, PMC *types)
{
    STRING *key = mmd_cache_key_from_types(interp, name, types);

    if (key)
        return (PMC *)parrot_hash_get(interp, cache, key);

    return PMCNULL;
}


/*

=item C<void Parrot_mmd_cache_store_by_types>

Takes an array of types for the call along with a chosen candidate and puts
it into the cache. The name parameter is optional, and if the cache is already
tied to an individual multi can be null.

=cut

*/

PARROT_EXPORT
void
Parrot_mmd_cache_store_by_types(PARROT_INTERP, MMD_Cache *cache, const char *name, PMC *types, PMC *chosen)
{
    STRING *key = mmd_cache_key_from_types(interp, name, types);
    if (key != NULL)
        parrot_hash_put(interp, cache, key, chosen);
}


/*

=item C<void Parrot_mmd_cache_mark>

GC-marks an MMD cache.

=cut

*/

PARROT_EXPORT
void
Parrot_mmd_cache_mark(PARROT_INTERP, MMD_Cache *cache)
{
    /* As a small future optimization, note that we only *really* need to mark keys -
     * the candidates will be referenced outside the cache, provided it's invalidated
     * properly. */
    parrot_mark_hash(interp, cache);
}


/*

=item C<void Parrot_mmd_cache_destroy>

Destroys an MMD cache.

=cut

*/

PARROT_EXPORT
void
Parrot_mmd_cache_destroy(PARROT_INTERP, MMD_Cache *cache)
{
    parrot_hash_destroy(interp, cache);
}


/*

=back

=head1 SEE ALSO

F<include/parrot/multidispatch.h>,
F<http://svn.perl.org/perl6/doc/trunk/design/apo/A12.pod>,
F<http://svn.perl.org/perl6/doc/trunk/design/syn/S12.pod>

=cut

*/


/*
 * Local variables:
 *   c-file-style: "parrot"
 * End:
 * vim: expandtab shiftwidth=4:
 */
