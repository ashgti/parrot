/*
Copyright (C) 2001-2009, Parrot Foundation.
$Id$

=head1 NAME

src/gc/register.c - Register handling routines

=head1 DESCRIPTION

Parrot has 4 register sets, one for each of its basic types. The number of
registers in each set varies depending on the use counts of the subroutine and
is determined by the PASM/PIR compiler in the register allocation pass
(F<imcc/reg_alloc.c>).

=cut

*/

#include "parrot/parrot.h"
#include "parrot/register.h"
#include "../pmc/pmc_sub.h"


/* set CTX_LEAK_DEBUG_FULL to 1 for enhanced context debugging.
 * When set (1) freed contexts are "poisoned" so that any dangling
 * references produce segfaults, and (2) contexts are not recycled
 * so that later allocations don't suddenly restore a dangling
 * reference to a "working" condition.
 */
#define CTX_LEAK_DEBUG_FULL 0



/* HEADERIZER HFILE: include/parrot/register.h */

/* HEADERIZER BEGIN: static */
/* Don't modify between HEADERIZER BEGIN / HEADERIZER END.  Your changes will be lost. */

static void clear_regs(PARROT_INTERP, ARGMOD(PMC *pmcctx))
        __attribute__nonnull__(1)
        __attribute__nonnull__(2)
        FUNC_MODIFIES(*pmcctx);

static void init_context(PARROT_INTERP,
    ARGMOD(PMC *pmcctx),
    ARGIN_NULLOK(PMC *pmcold))
        __attribute__nonnull__(1)
        __attribute__nonnull__(2)
        FUNC_MODIFIES(*pmcctx);

#define ASSERT_ARGS_clear_regs __attribute__unused__ int _ASSERT_ARGS_CHECK = \
       PARROT_ASSERT_ARG(interp) \
    || PARROT_ASSERT_ARG(pmcctx)
#define ASSERT_ARGS_init_context __attribute__unused__ int _ASSERT_ARGS_CHECK = \
       PARROT_ASSERT_ARG(interp) \
    || PARROT_ASSERT_ARG(pmcctx)
/* Don't modify between HEADERIZER BEGIN / HEADERIZER END.  Your changes will be lost. */
/* HEADERIZER END: static */


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

/*
=head2 Context and register frame allocation

There are two allocation strategies: chunked memory and malloced with a free
list.

 CHUNKED_CTX_MEM = 1

C<ctx_mem.data> is a pointer to an allocated chunk of memory.  The pointer
C<ctx_mem.free> holds the next usable location. With (full) continuations the
C<ctx_mem.free> pointer can't be moved below the C<ctx_mem.threshold>, which is
the highest context pointer of all active continuations.

[the code for this is incomplete; it had suffered some bit-rot and was
getting in the way of maintaining the other case.  -- rgr, 4-Feb-06.]

RT #46177 GC has to lower this threshold when collecting continuations.

 CHUNKED_CTX_MEM = 0

Context/register memory is malloced. C<ctx_mem.free> is used as a free list of
reusable items.

=cut

*/

#define CTX_ALLOC_SIZE 0x20000

#define ALIGNED_CTX_SIZE (((sizeof (Parrot_Context) + NUMVAL_SIZE - 1) \
        / NUMVAL_SIZE) * NUMVAL_SIZE)

/*

=pod

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

#if CHUNKED_CTX_MEM
 #  error "Non-working code removed."
#endif

/*

=head2 Context and Register Allocation Functions

=over 4

=cut

*/


/*

=item C<void create_initial_context(PARROT_INTERP)>

Creates the interpreter's initial context.

=cut

*/

void
create_initial_context(PARROT_INTERP)
{
    ASSERT_ARGS(create_initial_context)
    static INTVAL   num_regs[] = {32, 32, 32, 32};
    PMC *ignored;

    /* Create some initial free_list slots. */

#define INITIAL_FREE_SLOTS 8
    /* For now create context with 32 regs each. Some src tests (and maybe
     * other extenders) assume the presence of these registers */
    ignored = Parrot_set_new_context(interp, num_regs);
    UNUSED(ignored);
}


/*

=item C<void parrot_gc_context(PARROT_INTERP)>

Cleans up dead context memory; called by the garbage collector.  This only
applies in the chunked context memory scheme.

=cut

*/

PARROT_EXPORT
void
parrot_gc_context(PARROT_INTERP)
{
    ASSERT_ARGS(parrot_gc_context)
#if CHUNKED_CTX_MEM
    Parrot_Context ctx;
    ASSERT_ARGS(parrot_gc_context)

    if (!interp->ctx_mem.threshold)
        return;
    LVALUE_CAST(char *, ctx.bp) = interp->ctx_mem.threshold
                                - sizeof (parrot_regs_t);
#else
    UNUSED(interp);
#endif
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
    int i;
    Parrot_Context *ctx = Parrot_pcc_get_context_struct(interp, pmcctx);

    /* NULL out registers - P/S have to be NULL for GC
     *
     * if the architecture has 0x := NULL and 0.0 we could memset too
     */

    for (i = 0; i < ctx->n_regs_used[REGNO_PMC]; i++) {
        CTX_REG_PMC(pmcctx, i) = PMCNULL;
    }

    for (i = 0; i < ctx->n_regs_used[REGNO_STR]; i++) {
        CTX_REG_STR(pmcctx, i) = NULL;
    }

    if (Interp_debug_TEST(interp, PARROT_REG_DEBUG_FLAG)) {
        /* depending on -D40 we set int and num to be identifiable garbage values */
        for (i = 0; i < ctx->n_regs_used[REGNO_INT]; i++) {
            CTX_REG_INT(pmcctx, i) = -999;
        }
        for (i = 0; i < ctx->n_regs_used[REGNO_NUM]; i++) {
            CTX_REG_NUM(pmcctx, i) = -99.9;
        }
    }
}


/*

=item C<static void init_context(PARROT_INTERP, PMC *pmcctx, PMC *pmcold)>

Initializes a freshly allocated or recycled context.

=cut

*/

static void
init_context(PARROT_INTERP, ARGMOD(PMC *pmcctx),
        ARGIN_NULLOK(PMC *pmcold))
{
    ASSERT_ARGS(init_context)
    Parrot_Context *ctx = Parrot_pcc_get_context_struct(interp, pmcctx);
    Parrot_Context *old = Parrot_pcc_get_context_struct(interp, pmcold);

    ctx->current_results   = NULL;
    ctx->results_signature = NULL;
    ctx->lex_pad           = PMCNULL;
    ctx->outer_ctx         = NULL;
    ctx->current_cont      = NULL;
    ctx->current_object    = NULL;
    ctx->handlers          = PMCNULL;
    ctx->caller_ctx        = NULL;

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

=item C<PMC * Parrot_push_context(PARROT_INTERP, const INTVAL *n_regs_used)>

Creates and sets the current context to a new context, remembering the old
context in C<caller_ctx>.  Suitable to use with C<Parrot_pop_context>.

=cut

*/

PARROT_EXPORT
PARROT_WARN_UNUSED_RESULT
PARROT_CANNOT_RETURN_NULL
PMC *
Parrot_push_context(PARROT_INTERP, ARGIN(const INTVAL *n_regs_used))
{
    ASSERT_ARGS(Parrot_push_context)
    PMC * const old = CONTEXT(interp);
    PMC * const ctx = Parrot_set_new_context(interp, n_regs_used);

    Parrot_pcc_set_caller_ctx(interp, ctx, old);

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
    PMC * const ctx = CONTEXT(interp);
    PMC * const old = Parrot_pcc_get_caller_ctx(interp, ctx);

    /* restore old, set cached interpreter base pointers */
    CONTEXT(interp) = old;
}


/*

=item C<PMC * Parrot_alloc_context(PARROT_INTERP, const INTVAL
*number_regs_used, PMC *old)>

Allocates and returns a new context.  Does not set this new context as the
current context. Note that the register usage C<n_regs_used> is copied.  Use
the init flag to indicate whether you want to initialize the new context
(setting its default values and clearing its registers).

=cut

*/

PARROT_CANNOT_RETURN_NULL
PARROT_WARN_UNUSED_RESULT
PMC *
Parrot_alloc_context(PARROT_INTERP, ARGIN(const INTVAL *number_regs_used),
    ARGIN_NULLOK(PMC *old))
{
    ASSERT_ARGS(Parrot_alloc_context)
    PMC            *pmcctx;
    Parrot_Context *ctx;
    void *p;

    const size_t size_i = sizeof (INTVAL)   * number_regs_used[REGNO_INT];
    const size_t size_n = sizeof (FLOATVAL) * number_regs_used[REGNO_NUM];
    const size_t size_s = sizeof (STRING *) * number_regs_used[REGNO_STR];
    const size_t size_p = sizeof (PMC *)    * number_regs_used[REGNO_PMC];

    const size_t size_nip      = size_n + size_i + size_p;
    const size_t all_regs_size = size_n + size_i + size_p + size_s;
    const size_t reg_alloc     = ROUND_ALLOC_SIZE(all_regs_size);

    const size_t to_alloc = reg_alloc + ALIGNED_CTX_SIZE;
    ctx                   = (Parrot_Context *)mem_sys_allocate(to_alloc);

    ctx->n_regs_used[REGNO_INT] = number_regs_used[REGNO_INT];
    ctx->n_regs_used[REGNO_NUM] = number_regs_used[REGNO_NUM];
    ctx->n_regs_used[REGNO_STR] = number_regs_used[REGNO_STR];
    ctx->n_regs_used[REGNO_PMC] = number_regs_used[REGNO_PMC];

    /* regs start past the context */
    p   = (void *) ((char *)ctx + ALIGNED_CTX_SIZE);

    /* ctx.bp points to I0, which has Nx on the left */
    ctx->bp.regs_i = (INTVAL *)((char *)p + size_n);

    /* ctx.bp_ps points to S0, which has Px on the left */
    ctx->bp_ps.regs_s = (STRING **)((char *)p + size_nip);

    pmcctx = pmc_new(interp, enum_class_Context);
    VTABLE_set_pointer(interp, pmcctx, ctx);

    init_context(interp, pmcctx, old);

    return pmcctx;
}


/*

=item C<PMC * Parrot_set_new_context(PARROT_INTERP, const INTVAL
*number_regs_used)>

Allocates and returns a new context as the current context.  Note that the
register usage C<n_regs_used> is copied.

=cut

*/

PARROT_CANNOT_RETURN_NULL
PARROT_WARN_UNUSED_RESULT
PMC *
Parrot_set_new_context(PARROT_INTERP, ARGIN(const INTVAL *number_regs_used))
{
    ASSERT_ARGS(Parrot_set_new_context)
    PMC *old = CONTEXT(interp);
    PMC *ctx = Parrot_alloc_context(interp, number_regs_used, old);

    CONTEXT(interp) = ctx;

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
    int i;
    for (i = 0; i < CURRENT_CONTEXT_FIELD(interp, n_regs_used[REGNO_INT]); ++i)
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
    int i;
    for (i = 0; i < CURRENT_CONTEXT_FIELD(interp, n_regs_used[REGNO_STR]); ++i)
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
    int i;
    for (i = 0; i < CURRENT_CONTEXT_FIELD(interp, n_regs_used[REGNO_PMC]); ++i)
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
    int i;
    for (i = 0; i < CURRENT_CONTEXT_FIELD(interp, n_regs_used[REGNO_NUM]); ++i)
        REG_NUM(interp, i) = 0.0;
}

/*

=item C<INTVAL * Parrot_pcc_get_INTVAL_reg(PARROT_INTERP, PMC *ctx, INTVAL idx)>

Get pointer to INTVAL register.

=cut

*/

PARROT_EXPORT
PARROT_CANNOT_RETURN_NULL
INTVAL *
Parrot_pcc_get_INTVAL_reg(PARROT_INTERP, ARGIN(PMC *ctx), INTVAL idx)
{
    ASSERT_ARGS(Parrot_pcc_get_INTVAL_reg)
    return &(Parrot_pcc_get_context_struct(interp, ctx)->bp.regs_i[idx]);
}

/*

=item C<FLOATVAL * Parrot_pcc_get_FLOATVAL_reg(PARROT_INTERP, PMC *ctx, INTVAL
idx)>

Get pointer to FLOATVAL register.

=cut

*/

PARROT_EXPORT
PARROT_CANNOT_RETURN_NULL
FLOATVAL *
Parrot_pcc_get_FLOATVAL_reg(PARROT_INTERP, ARGIN(PMC *ctx), INTVAL idx)
{
    ASSERT_ARGS(Parrot_pcc_get_FLOATVAL_reg)
    return &(Parrot_pcc_get_context_struct(interp, ctx)->bp.regs_n[-1L - idx]);
}

/*

=item C<STRING ** Parrot_pcc_get_STRING_reg(PARROT_INTERP, PMC *ctx, INTVAL
idx)>

Get pointer to STRING register.

=cut

*/

PARROT_EXPORT
PARROT_CANNOT_RETURN_NULL
STRING **
Parrot_pcc_get_STRING_reg(PARROT_INTERP, ARGIN(PMC *ctx), INTVAL idx)
{
    ASSERT_ARGS(Parrot_pcc_get_STRING_reg)
    return &(Parrot_pcc_get_context_struct(interp, ctx)->bp_ps.regs_s[idx]);
}

/*

=item C<PMC ** Parrot_pcc_get_PMC_reg(PARROT_INTERP, PMC *ctx, INTVAL idx)>

Get pointer to PMC register.

=cut

*/

PARROT_EXPORT
PARROT_CANNOT_RETURN_NULL
PMC **
Parrot_pcc_get_PMC_reg(PARROT_INTERP, ARGIN(PMC *ctx), INTVAL idx)
{
    ASSERT_ARGS(Parrot_pcc_get_PMC_reg)
    return &(Parrot_pcc_get_context_struct(interp, ctx)->bp_ps.regs_p[-1L - idx]);
}



/*

=back

=head1 SEE ALSO

F<include/parrot/register.h> and F<src/stacks.c>.

=cut

*/


/*
 * Local variables:
 *   c-file-style: "parrot"
 * End:
 * vim: expandtab shiftwidth=4:
 */
