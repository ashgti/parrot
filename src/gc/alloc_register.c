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

static void clear_regs(PARROT_INTERP, ARGMOD(Parrot_Context *ctx))
        __attribute__nonnull__(1)
        __attribute__nonnull__(2)
        FUNC_MODIFIES(*ctx);

static void init_context(PARROT_INTERP,
    ARGMOD(Parrot_Context *ctx),
    ARGIN_NULLOK(const Parrot_Context *old))
        __attribute__nonnull__(1)
        __attribute__nonnull__(2)
        FUNC_MODIFIES(*ctx);

#define ASSERT_ARGS_clear_regs __attribute__unused__ int _ASSERT_ARGS_CHECK = \
       PARROT_ASSERT_ARG(interp) \
    || PARROT_ASSERT_ARG(ctx)
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

=item C<Parrot_Context * Parrot_ctx_get_context_struct(PARROT_INTERP, PMC * context)>

=cut

*/

PARROT_CAN_RETURN_NULL
Parrot_Context *
Parrot_ctx_get_context_struct(PARROT_INTERP, ARGIN(PMC * context))
{
    ASSERT_ARGS(Parrot_ctx_get_context_struct)
    return PARROT_CONTEXT(context);
}

/*

=item C<void destroy_context(PARROT_INTERP)>

Frees allocated context memory.

=cut

*/

void
destroy_context(PARROT_INTERP)
{
    ASSERT_ARGS(destroy_context)
    Parrot_Context *context = CONTEXT(interp);
    int             slot;

    while (context) {
        Parrot_Context * const prev = context->caller_ctx;

        /* always collect the parentmost context in the parentmost interp*/
        if (!prev && !interp->parent_interpreter)
            context->ref_count = 1;

        Parrot_free_context(interp, context, 1);

        context = prev;
    }

    /* clear freed contexts */
    for (slot = 0; slot < interp->ctx_mem.n_free_slots; ++slot) {
        void *ptr = interp->ctx_mem.free_list[slot];
        while (ptr) {
            void * const next = *(void **) ptr;
            mem_sys_free(ptr);
            ptr = next;
        }
        interp->ctx_mem.free_list[slot] = NULL;
    }
    mem_sys_free(interp->ctx_mem.free_list);
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

    /* Create some initial free_list slots. */

#define INITIAL_FREE_SLOTS 8
    interp->ctx_mem.n_free_slots = INITIAL_FREE_SLOTS;
    interp->ctx_mem.free_list    = mem_allocate_n_zeroed_typed(INITIAL_FREE_SLOTS, void *);

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

=item C<Parrot_Context * Parrot_push_context(PARROT_INTERP, const INTVAL
*n_regs_used)>

Creates and sets the current context to a new context, remembering the old
context in C<caller_ctx>.  Suitable to use with C<Parrot_pop_context>.

=cut

*/

PARROT_EXPORT
PARROT_WARN_UNUSED_RESULT
PARROT_CANNOT_RETURN_NULL
Parrot_Context *
Parrot_push_context(PARROT_INTERP, ARGIN(const INTVAL *n_regs_used))
{
    ASSERT_ARGS(Parrot_push_context)
    Parrot_Context * const old = CONTEXT(interp);
    Parrot_Context * const ctx = Parrot_set_new_context(interp, n_regs_used);

    ctx->caller_ctx  = old;

    /* doesn't change */
    ctx->current_sub = old->current_sub;

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
    Parrot_Context * const ctx = CONTEXT(interp);
    Parrot_Context * const old = ctx->caller_ctx;

#if CTX_LEAK_DEBUG
    if (ctx->ref_count > 0 &&
            Interp_debug_TEST(interp, PARROT_CTX_DESTROY_DEBUG_FLAG)) {
        fprintf(stderr, "[force recycle of context %p (%d refs)]\n",
            (void *)ctx, ctx->ref_count);
    }
#endif
    ctx->ref_count = 0;
    Parrot_free_context(interp, ctx, 0);

    /* restore old, set cached interpreter base pointers */
    CONTEXT(interp)      = old;
    interp->ctx.bp       = old->bp;
    interp->ctx.bp_ps    = old->bp_ps;
}

/*

=item C<Parrot_Context * Parrot_set_new_context(PARROT_INTERP, const INTVAL
*number_regs_used)>

Allocates and returns a new context as the current context.  Note that the
register usage C<n_regs_used> is copied.

=cut

*/

PARROT_CANNOT_RETURN_NULL
PARROT_WARN_UNUSED_RESULT
Parrot_Context *
Parrot_set_new_context(PARROT_INTERP, ARGIN(const INTVAL *number_regs_used))
{
    ASSERT_ARGS(Parrot_set_new_context)
    Parrot_Context *old = CONTEXT(interp);
    Parrot_Context *ctx = Parrot_alloc_context(interp, number_regs_used, old);

    CONTEXT(interp)          = ctx;
    interp->ctx.bp.regs_i    = ctx->bp.regs_i;
    interp->ctx.bp_ps.regs_s = ctx->bp_ps.regs_s;

    return ctx;
}




/*

=item C<Parrot_Context * Parrot_context_ref_trace(PARROT_INTERP, Parrot_Context
*ctx, const char *file, int line)>

Helper function to trace references when CTX_LEAK_DEBUG is set.

=cut

*/

PARROT_EXPORT
PARROT_CANNOT_RETURN_NULL
Parrot_Context *
Parrot_context_ref_trace(PARROT_INTERP, ARGMOD(Parrot_Context *ctx),
        ARGIN(const char *file), int line)
{
    ASSERT_ARGS(Parrot_context_ref_trace)
    if (Interp_debug_TEST(interp, PARROT_CTX_DESTROY_DEBUG_FLAG)) {
        const char *name = "unknown";

        if (ctx->current_sub) {
            Parrot_sub *sub;
            PMC_get_sub(interp, ctx->current_sub, sub);
            name = (char *)(sub->name->strstart);
        }

        fprintf(stderr, "[reference to context %p ('%s') taken at %s:%d]\n",
                (void *)ctx, name, file, line);
    }

    ctx->ref_count++;
    return ctx;
}


/*

=item C<void Parrot_set_context_threshold(PARROT_INTERP, Parrot_Context *ctx)>

Marks the context as possible threshold.

=cut

*/

PARROT_EXPORT
void
Parrot_set_context_threshold(SHIM_INTERP, SHIM(Parrot_Context *ctx))
{
    ASSERT_ARGS(Parrot_set_context_threshold)
    /* nothing to do */
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
    for (i = 0; i < CONTEXT(interp)->n_regs_used[REGNO_INT]; ++i)
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
    for (i = 0; i < CONTEXT(interp)->n_regs_used[REGNO_STR]; ++i)
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
    for (i = 0; i < CONTEXT(interp)->n_regs_used[REGNO_PMC]; ++i)
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
    for (i = 0; i < CONTEXT(interp)->n_regs_used[REGNO_NUM]; ++i)
        REG_NUM(interp, i) = 0.0;
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
