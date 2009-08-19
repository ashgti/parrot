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
#include "../pmc/pmc_context.h"
#include "../pmc/pmc_sub.h"


/* HEADERIZER HFILE: include/parrot/register.h */

/* HEADERIZER BEGIN: static */
/* Don't modify between HEADERIZER BEGIN / HEADERIZER END.  Your changes will be lost. */

static void init_context(PARROT_INTERP,
    ARGMOD(Parrot_Context *ctx),
    ARGIN_NULLOK(const Parrot_Context *old))
        __attribute__nonnull__(1)
        __attribute__nonnull__(2)
        FUNC_MODIFIES(*ctx);

#define ASSERT_ARGS_init_context __attribute__unused__ int _ASSERT_ARGS_CHECK = \
       PARROT_ASSERT_ARG(interp) \
    || PARROT_ASSERT_ARG(ctx)
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

/*

=head2 Context and Register Allocation Functions

=over 4

=cut

*/

/*

=item C<Parrot_Context_attributes * Parrot_ctx_get_context_struct(PARROT_INTERP,
PMC * context)>

=cut

*/

PARROT_CAN_RETURN_NULL
Parrot_Context_attributes *
Parrot_ctx_get_context_struct(PARROT_INTERP, ARGIN(PMC * context))
{
    ASSERT_ARGS(Parrot_ctx_get_context_struct)
    return PARROT_CONTEXT(context);
}

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
    Parrot_Context *ignored;

    /* For now create context with 32 regs each. Some src tests (and maybe
     * other extenders) assume the presence of these registers */
    ignored = Parrot_set_new_context(interp, num_regs);
    UNUSED(ignored);
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
    PMC * const old = interp->ctx;
    PMC * const ctx = Parrot_set_new_context(interp, n_regs_used);

    PARROT_CONTEXT(ctx)->caller_ctx  = old;

    /* doesn't change */
    PARROT_CONTEXT(ctx)->current_sub = PARROT_CONTEXT(old)->current_sub;

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
    PMC * const ctx = interp->ctx;
    PMC * const old = PARROT_CONTEXT(ctx)->caller_ctx;

    /* restore old, set cached interpreter base pointers */
    interp->ctx       = old;
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
    PMC * const old = interp->ctx;
    PMC * const ctx = Parrot_alloc_context(interp, number_regs_used, PARROT_CONTEXT(old));

    interp->ctx = ctx;

    return ctx;
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
    PMC * pmcctx;
    Parrot_Context_attributes *ctx;
    void *p;

    const size_t size_i = sizeof (INTVAL)   * number_regs_used[REGNO_INT];
    const size_t size_n = sizeof (FLOATVAL) * number_regs_used[REGNO_NUM];
    const size_t size_s = sizeof (STRING *) * number_regs_used[REGNO_STR];
    const size_t size_p = sizeof (PMC *)    * number_regs_used[REGNO_PMC];

    const size_t size_nip      = size_n + size_i + size_p;
    const size_t all_regs_size = size_n + size_i + size_p + size_s;

    pmcctx = pmc_new(interp, enum_class_Context);
    ctx = PARROT_CONTEXT(pmcctx);


    ctx->n_regs_used[REGNO_INT] = number_regs_used[REGNO_INT];
    ctx->n_regs_used[REGNO_NUM] = number_regs_used[REGNO_NUM];
    ctx->n_regs_used[REGNO_STR] = number_regs_used[REGNO_STR];
    ctx->n_regs_used[REGNO_PMC] = number_regs_used[REGNO_PMC];

#if CTX_LEAK_DEBUG
    if (Interp_debug_TEST(interp, PARROT_CTX_DESTROY_DEBUG_FLAG)) {
        fprintf(stderr, "[alloc ctx %p]\n", ctx);
    }
#endif

    ctx->regs_mem_size = all_regs_size;

    /* regs start past the context */
    p   = (void *)mem_sys_allocate(all_regs_size);

    /* ctx.bp points to I0, which has Nx on the left */
    ctx->bp.regs_i = (INTVAL *)((char *)p + size_n);

    /* ctx.bp_ps points to S0, which has Px on the left */
    ctx->bp_ps.regs_s = (STRING **)((char *)p + size_nip);

    init_context(interp, ctx, old);

    return pmcctx;
}

/*

=item C<static void init_context(PARROT_INTERP, Parrot_Context *ctx, const
Parrot_Context *old)>

Initializes a freshly allocated or recycled context.

=cut

*/

static void
init_context(PARROT_INTERP, ARGMOD(Parrot_Context *ctx),
        ARGIN_NULLOK(const Parrot_Context *old))
{
    ASSERT_ARGS(init_context)
    ctx->ref_count         = 0;
    ctx->gc_mark           = 0;
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
    clear_regs(interp, ctx);
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
    const int n_regs_used = CONTEXT(interp)->n_regs_used[REGNO_INT];
    for (i = 0; i < n_regs_used; ++i)
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
    const int n_regs_used = CONTEXT(interp)->n_regs_used[REGNO_STR];
    for (i = 0; i < n_regs_used; ++i)
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
    const int n_regs_used = CONTEXT(interp)->n_regs_used[REGNO_PMC];
    for (i = 0; i < n_regs_used; ++i)
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
    const int n_regs_used = CONTEXT(interp)->n_regs_used[REGNO_NUM];
    for (i = 0; i < nregs_used; ++i)
        REG_NUM(interp, i) = 0.0;
}

PARROT_CANNOT_RETURN_NULL
INTVAL *
Parrot_ctx_INTVAL_reg(PARROT_INTERP, ARGIN(PMC *ctx), INTVAL idx)
{
    ASSERT_ARGS(Parrot_ctx_get_INTVAL_reg)
    return &(PARROT_CONTEXT(ctx)->bp.regs_i[idx]);
}

PARROT_CANNOT_RETURN_NULL
FLOATVAL *
Parrot_ctx_FLOATVAL_reg(PARROT_INTERP, ARGIN(PMC *ctx), INTVAL idx)
{
    ASSERT_ARGS(Parrot_ctx_get_FLOATVAL_reg)
    return &(PARROT_CONTEXT(ctx)->bp.regs_n[-1L - idx]);
}

PARROT_CANNOT_RETURN_NULL
STRING **
Parrot_ctx_STRING_reg(PARROT_INTERP, ARGIN(PMC *ctx), INTVAL idx)
{
    ASSERT_ARGS(Parrot_ctx_get_FLOATVAL_reg)
    return &(PARROT_CONTEXT(ctx)->bp.regs_s[idx]);
}

PARROT_CANNOT_RETURN_NULL
PMC **
Parrot_ctx_PMC_reg(PARROT_INTERP, ARGIN(PMC *ctx), INTVAL idx)
{
    ASSERT_ARGS(Parrot_ctx_get_FLOATVAL_reg)
    return &(PARROT_CONTEXT(ctx)->bp.regs_p[-1L - idx]);
}

PARROT_CANNOT_RETURN_NULL
PMC *
Parrot_ctx_get_caller(PARROT_INTERP, ARGIN(PMC *ctx))
{
    ASSERT_ARGS(Parrot_ctx_get_caller)
    return PARROT_CONTEXT(ctx)->caller_ctx;
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
