/*
Copyright (C) 2009, Parrot Foundation.
$Id$

=head1 NAME

src/context.c

=head1 DESCRIPTION

Parrot_Context functions.

=cut

*/

#include "parrot/parrot.h"
#include "parrot/call.h"
#include "pmc/pmc_sub.h"
#include "pmc/pmc_callcontext.h"

/*

=head2 Context and register frame layout

    +----------++----+------+------------+----+
    | context  || N  |  I   |   P        |  S +
    +----------++----+------+------------+----+
    ^          ^     ^                   ^
    |          |     ctx.bp              ctx.bp_ps
    ctx.state  opt
               padding

Registers are addressed as usual via the register base pointer ctx.bp.

The macro CONTEXT() hides these details

=cut

*/

#define ALIGNED_CTX_SIZE (((sizeof (Parrot_Context) + NUMVAL_SIZE - 1) \
        / NUMVAL_SIZE) * NUMVAL_SIZE)

/*

=head2 Allocation Size

Round register allocation size up to the nearest multiple of 8. A granularity
of 8 is arbitrary, it could have been some bigger power of 2. A "slot" is an
index into the free_list array. Each slot in free_list has a linked list of
pointers to already allocated contexts available for (re)use.  The slot where
an available context is stored corresponds to the size of the context.

=cut

*/

#define SLOT_CHUNK_SIZE 8

#define ROUND_ALLOC_SIZE(size) ((((size) + SLOT_CHUNK_SIZE - 1) \
        / SLOT_CHUNK_SIZE) * SLOT_CHUNK_SIZE)
#define CALCULATE_SLOT_NUM(size) ((size) / SLOT_CHUNK_SIZE)


/* HEADERIZER HFILE: include/parrot/call.h */

/* HEADERIZER BEGIN: static */
/* Don't modify between HEADERIZER BEGIN / HEADERIZER END.  Your changes will be lost. */

static void allocate_registers(PARROT_INTERP,
    ARGIN(PMC *pmcctx),
    ARGIN(const UINTVAL *number_regs_used))
        __attribute__nonnull__(1)
        __attribute__nonnull__(2)
        __attribute__nonnull__(3);

static size_t calculate_registers_size(SHIM_INTERP,
    ARGIN(const UINTVAL *number_regs_used))
        __attribute__nonnull__(2);

static void clear_regs(PARROT_INTERP, ARGMOD(PMC *pmcctx))
        __attribute__nonnull__(1)
        __attribute__nonnull__(2)
        FUNC_MODIFIES(*pmcctx);

PARROT_INLINE
PARROT_CANNOT_RETURN_NULL
static Parrot_Context * get_context_struct_fast(PARROT_INTERP,
    ARGIN(PMC *ctx))
        __attribute__nonnull__(1)
        __attribute__nonnull__(2);

static void init_context(PARROT_INTERP,
    ARGMOD(PMC *pmcctx),
    ARGIN_NULLOK(PMC *pmcold))
        __attribute__nonnull__(1)
        __attribute__nonnull__(2)
        FUNC_MODIFIES(*pmcctx);

static size_t Parrot_pcc_calculate_registers_size(PARROT_INTERP,
    ARGIN(const UINTVAL *number_regs_used))
        __attribute__nonnull__(1)
        __attribute__nonnull__(2);

#define ASSERT_ARGS_allocate_registers __attribute__unused__ int _ASSERT_ARGS_CHECK = (\
       PARROT_ASSERT_ARG(interp) \
    , PARROT_ASSERT_ARG(pmcctx) \
    , PARROT_ASSERT_ARG(number_regs_used))
#define ASSERT_ARGS_calculate_registers_size __attribute__unused__ int _ASSERT_ARGS_CHECK = (\
       PARROT_ASSERT_ARG(number_regs_used))
#define ASSERT_ARGS_clear_regs __attribute__unused__ int _ASSERT_ARGS_CHECK = (\
       PARROT_ASSERT_ARG(interp) \
    , PARROT_ASSERT_ARG(pmcctx))
#define ASSERT_ARGS_get_context_struct_fast __attribute__unused__ int _ASSERT_ARGS_CHECK = (\
       PARROT_ASSERT_ARG(interp) \
    , PARROT_ASSERT_ARG(ctx))
#define ASSERT_ARGS_init_context __attribute__unused__ int _ASSERT_ARGS_CHECK = (\
       PARROT_ASSERT_ARG(interp) \
    , PARROT_ASSERT_ARG(pmcctx))
#define ASSERT_ARGS_Parrot_pcc_calculate_registers_size \
     __attribute__unused__ int _ASSERT_ARGS_CHECK = (\
       PARROT_ASSERT_ARG(interp) \
    , PARROT_ASSERT_ARG(number_regs_used))
/* Don't modify between HEADERIZER BEGIN / HEADERIZER END.  Your changes will be lost. */
/* HEADERIZER END: static */


/*

=head2 Context API Functions

=over 4

=item C<PMC* Parrot_pcc_get_sub(PARROT_INTERP, PMC *ctx)>

Get Sub executed inside Context.

=cut

*/

PARROT_EXPORT
PARROT_CAN_RETURN_NULL
PMC*
Parrot_pcc_get_sub(PARROT_INTERP, ARGIN(PMC *ctx))
{
    ASSERT_ARGS(Parrot_pcc_get_sub)
    Parrot_Context const *c = get_context_struct_fast(interp, ctx);
    return c->current_sub;
}


/*

=item C<void Parrot_pcc_set_sub(PARROT_INTERP, PMC *ctx, PMC *sub)>

Set Sub executed inside Context.

=cut

*/

PARROT_EXPORT
void
Parrot_pcc_set_sub(PARROT_INTERP, ARGIN(PMC *ctx), ARGIN_NULLOK(PMC *sub))
{
    ASSERT_ARGS(Parrot_pcc_set_sub)
    Parrot_Context *c = get_context_struct_fast(interp, ctx);
    c->current_sub    = sub;

    if (sub && !PMC_IS_NULL(sub)) {
        Parrot_Sub_attributes *subattr;

        PMC_get_sub(interp, sub, subattr);

        c->current_pc        = subattr->seg->base.data + subattr->start_offs;
        c->current_HLL       = subattr->HLL_id;
        c->current_namespace = subattr->namespace_stash;
    }
}


/*

=back

=head2 Context and Register Allocation Functions

=over 4

=item C<void create_initial_context(PARROT_INTERP)>

Creates the interpreter's initial context.

=cut

*/

void
create_initial_context(PARROT_INTERP)
{
    ASSERT_ARGS(create_initial_context)
    static UINTVAL   num_regs[] = {32, 32, 32, 32};
    PMC *ignored;

    /* Create some initial free_list slots. */

#define INITIAL_FREE_SLOTS 8
    /* For now create context with 32 regs each. Some src tests (and maybe
     * other extenders) assume the presence of these registers */
    ignored = Parrot_set_new_context(interp, num_regs);
    UNUSED(ignored);
}

/*

=item C<static Parrot_Context * get_context_struct_fast(PARROT_INTERP, PMC
*ctx)>

Fetches Parrot_Context from Context PMC.  This is a static, inlineable function
so it only works within this file.  It also only works if you *know* that ctx
is a valid PMC, so be careful.  This is an encapsulation-breaking optimization
that improves performance measurably.  Use responsibly.  Never export this
function.

=cut

*/


PARROT_INLINE
PARROT_CANNOT_RETURN_NULL
static Parrot_Context *
get_context_struct_fast(PARROT_INTERP, ARGIN(PMC *ctx))
{
    ASSERT_ARGS(get_context_struct_fast)

    /* temporarily violate encapsulation; big speedup here */
    return PMC_data_typed(ctx, Parrot_Context *);
}

/*

=item C<static void clear_regs(PARROT_INTERP, PMC *pmcctx)>

Clears all registers in a context.  PMC and STRING registers contain PMCNULL
and NULL, respectively.  Integer and float registers contain negative flag
values, for debugging purposes.

=cut

*/

static void
clear_regs(PARROT_INTERP, ARGMOD(PMC *pmcctx))
{
    ASSERT_ARGS(clear_regs)
    UINTVAL i;
    Parrot_Context *ctx = get_context_struct_fast(interp, pmcctx);

    /* NULL out registers - P/S have to be NULL for GC
     *
     * if the architecture has 0x := NULL and 0.0 we could memset too
     */

    for (i = 0; i < ctx->n_regs_used[REGNO_PMC]; i++) {
        ctx->bp_ps.regs_p[-1L - i] = PMCNULL;
    }

    for (i = 0; i < ctx->n_regs_used[REGNO_STR]; i++) {
        ctx->bp_ps.regs_s[i] = NULL;
    }

    if (Interp_debug_TEST(interp, PARROT_REG_DEBUG_FLAG)) {
        /* depending on -D40, set int and num to identifiable garbage values */
        for (i = 0; i < ctx->n_regs_used[REGNO_INT]; i++) {
            ctx->bp.regs_i[i] = -999;
        }

        for (i = 0; i < ctx->n_regs_used[REGNO_NUM]; i++) {
            ctx->bp.regs_n[-1L - i] = -99.9;
        }
    }
}


/*

=item C<static void init_context(PARROT_INTERP, PMC *pmcctx, PMC *pmcold)>

Initializes a freshly allocated or recycled context.

=cut

*/

static void
init_context(PARROT_INTERP, ARGMOD(PMC *pmcctx), ARGIN_NULLOK(PMC *pmcold))
{
    ASSERT_ARGS(init_context)
    Parrot_Context *ctx    = get_context_struct_fast(interp, pmcctx);

    /* pmcold may be null */
    Parrot_Context *old    = PMC_IS_NULL(pmcold)
                           ? NULL
                           : get_context_struct_fast(interp, pmcold);

    PARROT_ASSERT(!PMC_IS_NULL(pmcctx) || !"Can't initialise Null CallContext");

    /*
     * FIXME Invoking corotine shouldn't initialise context. So just
     * check ctx->current_sub. If it's not null return from here
     */
    if (!PMC_IS_NULL(ctx->current_sub))
        return;

    ctx->current_results   = NULL;
    ctx->results_signature = NULL;
    ctx->lex_pad           = PMCNULL;
    ctx->outer_ctx         = NULL;
    ctx->current_cont      = NULL;
    ctx->current_object    = NULL;
    ctx->handlers          = PMCNULL;
    ctx->caller_ctx        = NULL;
    ctx->pred_offset       = 0;
    ctx->current_sig       = PMCNULL;
    ctx->current_sub       = PMCNULL;

    if (old) {
        /* some items should better be COW copied */
        ctx->constants         = old->constants;
        ctx->warns             = old->warns;
        ctx->errors            = old->errors;
        ctx->trace_flags       = old->trace_flags;
        ctx->pred_offset       = old->pred_offset;
        ctx->current_HLL       = old->current_HLL;
        ctx->current_namespace = old->current_namespace;
        /* end COW */
        ctx->recursion_depth   = old->recursion_depth;
        ctx->caller_ctx        = pmcold;
    }
    else {
        ctx->constants         = NULL;
        ctx->warns             = 0;
        ctx->errors            = 0;
        ctx->trace_flags       = 0;
        ctx->pred_offset       = 0;
        ctx->current_HLL       = 0;
        ctx->current_namespace = PMCNULL;
        ctx->recursion_depth   = 0;
    }

    /* other stuff is set inside Sub.invoke */
    clear_regs(interp, pmcctx);
}


/*

=item C<PMC * Parrot_push_context(PARROT_INTERP, const UINTVAL *n_regs_used)>

Creates and sets the current context to a new context, remembering the old
context in C<caller_ctx>.  Suitable to use with C<Parrot_pop_context>.

=cut

*/

PARROT_EXPORT
PARROT_WARN_UNUSED_RESULT
PARROT_CANNOT_RETURN_NULL
PMC *
Parrot_push_context(PARROT_INTERP, ARGIN(const UINTVAL *n_regs_used))
{
    ASSERT_ARGS(Parrot_push_context)
    PMC * const old = CURRENT_CONTEXT(interp);
    PMC * const ctx = Parrot_set_new_context(interp, n_regs_used);

    /* doesn't change */
    Parrot_pcc_set_sub(interp, ctx, Parrot_pcc_get_sub(interp, old));

    /* copy more ? */
    return ctx;
}


/*

=item C<void Parrot_pop_context(PARROT_INTERP)>

Frees the context created with C<Parrot_push_context> and restores the previous
context (the caller context).

=cut

*/

PARROT_EXPORT
void
Parrot_pop_context(PARROT_INTERP)
{
    ASSERT_ARGS(Parrot_pop_context)
    PMC * const ctx = CURRENT_CONTEXT(interp);
    PMC * const old = Parrot_pcc_get_caller_ctx(interp, ctx);

    /* restore old, set cached interpreter base pointers */
    CURRENT_CONTEXT(interp) = old;
}

/*

=item C<static size_t calculate_registers_size(PARROT_INTERP, const UINTVAL
*number_regs_used)>

Calculate memory size required for registers.

=cut

*/
static size_t
calculate_registers_size(SHIM_INTERP, ARGIN(const UINTVAL *number_regs_used))
{
    ASSERT_ARGS(calculate_registers_size)

    return ROUND_ALLOC_SIZE(
            sizeof (INTVAL)   * number_regs_used[REGNO_INT] +
            sizeof (FLOATVAL) * number_regs_used[REGNO_NUM] +
            sizeof (STRING *) * number_regs_used[REGNO_STR] +
            sizeof (PMC *)    * number_regs_used[REGNO_PMC]);
}


/*

=item C<static size_t Parrot_pcc_calculate_registers_size(PARROT_INTERP, const
UINTVAL *number_regs_used)>

Calculate size of Context.

=cut

*/

static size_t
Parrot_pcc_calculate_registers_size(PARROT_INTERP, ARGIN(const UINTVAL *number_regs_used))
{
    ASSERT_ARGS(Parrot_pcc_calculate_registers_size)
    return calculate_registers_size(interp, number_regs_used);
}

/*

=item C<static void allocate_registers(PARROT_INTERP, PMC *pmcctx, const UINTVAL
*number_regs_used)>

Allocate registers inside Context.

=cut

*/
static void
allocate_registers(PARROT_INTERP, ARGIN(PMC *pmcctx), ARGIN(const UINTVAL *number_regs_used))
{
    ASSERT_ARGS(allocate_registers)
    Parrot_CallContext_attributes *ctx = PARROT_CALLCONTEXT(pmcctx);

    const size_t size_i = sizeof (INTVAL)   * number_regs_used[REGNO_INT];
    const size_t size_n = sizeof (FLOATVAL) * number_regs_used[REGNO_NUM];
    const size_t size_s = sizeof (STRING *) * number_regs_used[REGNO_STR];
    const size_t size_p = sizeof (PMC *)    * number_regs_used[REGNO_PMC];

    const size_t size_nip      = size_n + size_i + size_p;
    const size_t all_regs_size = size_n + size_i + size_p + size_s;
    const size_t reg_alloc     = ROUND_ALLOC_SIZE(all_regs_size);

    /* don't allocate any storage if there are no registers */
    ctx->registers = reg_alloc
        ? (Parrot_Context *)Parrot_gc_allocate_fixed_size_storage(interp, reg_alloc)
        : NULL;

    ctx->n_regs_used[REGNO_INT] = number_regs_used[REGNO_INT];
    ctx->n_regs_used[REGNO_NUM] = number_regs_used[REGNO_NUM];
    ctx->n_regs_used[REGNO_STR] = number_regs_used[REGNO_STR];
    ctx->n_regs_used[REGNO_PMC] = number_regs_used[REGNO_PMC];

    /* ctx.bp points to I0, which has Nx on the left */
    ctx->bp.regs_i = (INTVAL *)((char *)ctx->registers + size_n);

    /* ctx.bp_ps points to S0, which has Px on the left */
    ctx->bp_ps.regs_s = (STRING **)((char *)ctx->registers + size_nip);

    clear_regs(interp, pmcctx);
}


/*

=item C<void Parrot_pcc_allocate_registers(PARROT_INTERP, PMC *pmcctx, const
UINTVAL *number_regs_used)>

Allocate registers in Context.

=cut

*/

void
Parrot_pcc_allocate_registers(PARROT_INTERP, ARGIN(PMC *pmcctx),
        ARGIN(const UINTVAL *number_regs_used))
{
    ASSERT_ARGS(Parrot_pcc_allocate_registers)
    allocate_registers(interp, pmcctx, number_regs_used);
}


/*

=item C<void Parrot_pcc_free_registers(PARROT_INTERP, PMC *pmcctx)>

Free memory allocated for registers in Context.

=cut

*/

void
Parrot_pcc_free_registers(PARROT_INTERP, ARGIN(PMC *pmcctx))
{
    ASSERT_ARGS(Parrot_pcc_free_registers)
    Parrot_CallContext_attributes * const ctx = PARROT_CALLCONTEXT(pmcctx);
    size_t reg_size;

    if (!ctx)
        return;

    reg_size = Parrot_pcc_calculate_registers_size(interp, ctx->n_regs_used);
    if (!reg_size)
        return;

    /* Free registers */
    Parrot_gc_free_fixed_size_storage(interp, reg_size, ctx->registers);

}


/*

=item C<PMC * Parrot_alloc_context(PARROT_INTERP, const UINTVAL
*number_regs_used, PMC *old)>

Allocates and returns a new context.  Does not set this new context as the
current context. Note that the register usage C<n_regs_used> is copied.  Use
the init flag to indicate whether you want to initialize the new context
(setting its default values and clearing its registers).

TODO: Remove this function!

=cut

*/

PARROT_CANNOT_RETURN_NULL
PARROT_WARN_UNUSED_RESULT
PMC *
Parrot_alloc_context(PARROT_INTERP, ARGIN(const UINTVAL *number_regs_used),
    ARGIN_NULLOK(PMC *old))
{
    ASSERT_ARGS(Parrot_alloc_context)
    PMC            *pmcctx = pmc_new(interp, enum_class_CallContext);

    allocate_registers(interp, pmcctx, number_regs_used);
    init_context(interp, pmcctx, old);

    return pmcctx;
}


/*

=item C<PMC * Parrot_pcc_allocate_empty_context(PARROT_INTERP, PMC *old)>

Allocates and returns a new context.  Does not set this new context as the
current context.

=cut

*/

PARROT_CANNOT_RETURN_NULL
PARROT_WARN_UNUSED_RESULT
PMC *
Parrot_pcc_allocate_empty_context(PARROT_INTERP, ARGIN_NULLOK(PMC *old))
{
    ASSERT_ARGS(Parrot_pcc_allocate_empty_context)
    PMC            *pmcctx = pmc_new(interp, enum_class_CallContext);

    init_context(interp, pmcctx, old);

    return pmcctx;
}

/*

=item C<PMC * Parrot_pcc_init_context(PARROT_INTERP, PMC *ctx, PMC *old)>

Initialise new context from old.

=cut

*/

PARROT_CANNOT_RETURN_NULL
PMC *
Parrot_pcc_init_context(PARROT_INTERP, ARGIN(PMC *ctx), ARGIN_NULLOK(PMC *old))
{
    ASSERT_ARGS(Parrot_pcc_init_context)

    init_context(interp, ctx, old);

    return ctx;
}

/*

=item C<PMC * Parrot_set_new_context(PARROT_INTERP, const UINTVAL
*number_regs_used)>

Allocates and returns a new context as the current context.  Note that the
register usage C<n_regs_used> is copied.

=cut

*/

PARROT_CANNOT_RETURN_NULL
PARROT_WARN_UNUSED_RESULT
PMC *
Parrot_set_new_context(PARROT_INTERP, ARGIN(const UINTVAL *number_regs_used))
{
    ASSERT_ARGS(Parrot_set_new_context)
    PMC *old = CURRENT_CONTEXT(interp);
    PMC *ctx = Parrot_alloc_context(interp, number_regs_used, old);

    CURRENT_CONTEXT(interp) = ctx;

    return ctx;
}


/*

=back

=head2 Register Stack Functions

=over 4

=cut

=item C<void Parrot_clear_i(PARROT_INTERP)>

Sets all integer registers in the current context to 0.

=cut

*/

PARROT_EXPORT
void
Parrot_clear_i(PARROT_INTERP)
{
    ASSERT_ARGS(Parrot_clear_i)
    UINTVAL i;
    for (i = 0; i < Parrot_pcc_get_regs_used(interp, CURRENT_CONTEXT(interp), REGNO_INT); ++i)
        REG_INT(interp, i) = 0;
}


/*

=item C<void Parrot_clear_s(PARROT_INTERP)>

Sets all STRING registers in the current context to NULL.

=cut

*/

PARROT_EXPORT
void
Parrot_clear_s(PARROT_INTERP)
{
    ASSERT_ARGS(Parrot_clear_s)
    UINTVAL i;
    for (i = 0; i < Parrot_pcc_get_regs_used(interp, CURRENT_CONTEXT(interp), REGNO_STR); ++i)
        REG_STR(interp, i) = NULL;
}


/*

=item C<void Parrot_clear_p(PARROT_INTERP)>

Sets all PMC registers in the current context to NULL.

=cut

*/

PARROT_EXPORT
void
Parrot_clear_p(PARROT_INTERP)
{
    ASSERT_ARGS(Parrot_clear_p)
    UINTVAL i;
    for (i = 0; i < Parrot_pcc_get_regs_used(interp, CURRENT_CONTEXT(interp), REGNO_PMC); ++i)
        REG_PMC(interp, i) = PMCNULL;
}


/*

=item C<void Parrot_clear_n(PARROT_INTERP)>

Sets all number registers in the current context to 0.0.

=cut

*/

PARROT_EXPORT
void
Parrot_clear_n(PARROT_INTERP)
{
    ASSERT_ARGS(Parrot_clear_n)
    UINTVAL i;
    for (i = 0; i < Parrot_pcc_get_regs_used(interp, CURRENT_CONTEXT(interp), REGNO_NUM); ++i)
        REG_NUM(interp, i) = 0.0;
}

/*

=item C<INTVAL * Parrot_pcc_get_INTVAL_reg(PARROT_INTERP, PMC *ctx, UINTVAL
idx)>

Get pointer to INTVAL register.

=cut

*/

PARROT_EXPORT
PARROT_CANNOT_RETURN_NULL
INTVAL *
Parrot_pcc_get_INTVAL_reg(PARROT_INTERP, ARGIN(PMC *ctx), UINTVAL idx)
{
    ASSERT_ARGS(Parrot_pcc_get_INTVAL_reg)
    PARROT_ASSERT(Parrot_pcc_get_regs_used(interp, ctx, REGNO_INT) > idx);
    return &(get_context_struct_fast(interp, ctx)->bp.regs_i[idx]);
}

/*

=item C<FLOATVAL * Parrot_pcc_get_FLOATVAL_reg(PARROT_INTERP, PMC *ctx, UINTVAL
idx)>

Get pointer to FLOATVAL register.

=cut

*/

PARROT_EXPORT
PARROT_CANNOT_RETURN_NULL
FLOATVAL *
Parrot_pcc_get_FLOATVAL_reg(PARROT_INTERP, ARGIN(PMC *ctx), UINTVAL idx)
{
    ASSERT_ARGS(Parrot_pcc_get_FLOATVAL_reg)
    PARROT_ASSERT(Parrot_pcc_get_regs_used(interp, ctx, REGNO_NUM) > idx);
    return &(get_context_struct_fast(interp, ctx)->bp.regs_n[-1L - idx]);
}

/*

=item C<STRING ** Parrot_pcc_get_STRING_reg(PARROT_INTERP, PMC *ctx, UINTVAL
idx)>

Get pointer to STRING register.

=cut

*/

PARROT_EXPORT
PARROT_CANNOT_RETURN_NULL
STRING **
Parrot_pcc_get_STRING_reg(PARROT_INTERP, ARGIN(PMC *ctx), UINTVAL idx)
{
    ASSERT_ARGS(Parrot_pcc_get_STRING_reg)
    PARROT_ASSERT(Parrot_pcc_get_regs_used(interp, ctx, REGNO_STR) > idx);
    return &(get_context_struct_fast(interp, ctx)->bp_ps.regs_s[idx]);
}

/*

=item C<PMC ** Parrot_pcc_get_PMC_reg(PARROT_INTERP, PMC *ctx, UINTVAL idx)>

Get pointer to PMC register.

=cut

*/

PARROT_EXPORT
PARROT_CANNOT_RETURN_NULL
PMC **
Parrot_pcc_get_PMC_reg(PARROT_INTERP, ARGIN(PMC *ctx), UINTVAL idx)
{
    ASSERT_ARGS(Parrot_pcc_get_PMC_reg)
    PARROT_ASSERT(Parrot_pcc_get_regs_used(interp, ctx, REGNO_PMC) > idx);
    return &(get_context_struct_fast(interp, ctx)->bp_ps.regs_p[-1L - idx]);
}

/*

=item C<UINTVAL Parrot_pcc_get_regs_used(PARROT_INTERP, PMC *ctx, int type)>

Return number of used registers of particular type.

=cut

*/
PARROT_EXPORT
UINTVAL
Parrot_pcc_get_regs_used(PARROT_INTERP, ARGIN(PMC *ctx), int type)
{
    ASSERT_ARGS(Parrot_pcc_get_regs_used)
    return get_context_struct_fast(interp, ctx)->n_regs_used[type];
}

/*

=item C<void Parrot_pcc_set_regs_used(PARROT_INTERP, PMC *ctx, int type, INTVAL
num)>

Set number of used registers of particular type.

=cut

*/
PARROT_EXPORT
void
Parrot_pcc_set_regs_used(PARROT_INTERP, ARGIN(PMC *ctx), int type, INTVAL num)
{
    ASSERT_ARGS(Parrot_pcc_set_regs_used)
    get_context_struct_fast(interp, ctx)->n_regs_used[type] = num;
}

/*

=item C<Regs_ni* Parrot_pcc_get_regs_ni(PARROT_INTERP, PMC *ctx)>

Get pointer to FLOANFAL and INTVAL registers.

=cut

*/
PARROT_EXPORT
PARROT_CANNOT_RETURN_NULL
Regs_ni*
Parrot_pcc_get_regs_ni(PARROT_INTERP, ARGIN(PMC *ctx))
{
    ASSERT_ARGS(Parrot_pcc_get_regs_ni)
    return &(get_context_struct_fast(interp, ctx)->bp);
}

/*

=item C<void Parrot_pcc_set_regs_ni(PARROT_INTERP, PMC *ctx, Regs_ni *bp)>

Copy Regs_ni into Context.

=cut

*/
PARROT_EXPORT
PARROT_CANNOT_RETURN_NULL
void
Parrot_pcc_set_regs_ni(PARROT_INTERP, ARGIN(PMC *ctx), ARGIN(Regs_ni *bp))
{
    ASSERT_ARGS(Parrot_pcc_set_regs_ni)
    get_context_struct_fast(interp, ctx)->bp = *bp;
}

/*

=item C<Regs_ps* Parrot_pcc_get_regs_ps(PARROT_INTERP, PMC *ctx)>

Get pointer to PMC and STRING registers.

=cut

*/
PARROT_EXPORT
PARROT_CANNOT_RETURN_NULL
Regs_ps*
Parrot_pcc_get_regs_ps(PARROT_INTERP, ARGIN(PMC *ctx))
{
    ASSERT_ARGS(Parrot_pcc_get_regs_ps)
    return &(get_context_struct_fast(interp, ctx)->bp_ps);
}

/*

=item C<void Parrot_pcc_set_regs_ps(PARROT_INTERP, PMC *ctx, Regs_ps *bp_ps)>

Copy Regs_ps into Context.

=cut

*/
PARROT_EXPORT
PARROT_CANNOT_RETURN_NULL
void
Parrot_pcc_set_regs_ps(PARROT_INTERP, ARGIN(PMC *ctx), ARGIN(Regs_ps *bp_ps))
{
    ASSERT_ARGS(Parrot_pcc_set_regs_ps)
    get_context_struct_fast(interp, ctx)->bp_ps = *bp_ps;
}


/*

=back

*/


/*
 * Local variables:
 *   c-file-style: "parrot"
 * End:
 * vim: expandtab shiftwidth=4:
 */
