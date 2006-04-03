/*
Copyright: 2001-2003 The Perl Foundation.  All Rights Reserved.
$Id$

=head1 NAME

src/sub.c - Subroutines

=head1 DESCRIPTION

Subroutines, continuations, co-routines and other fun stuff...

=head2 Functions

=over 4

=cut

*/

#include "parrot/parrot.h"
#include "parrot/oplib/ops.h"

/*

=item C<void
mark_context(Interp* interpreter, parrot_context_t* ctx)>

Marks the context C<*ctx>.

=cut

*/

void
mark_context(Interp* interpreter, parrot_context_t* ctx)
{
    PObj *obj;
    int i;

    mark_stack(interpreter, ctx->user_stack);
    mark_stack(interpreter, ctx->control_stack);
    mark_register_stack(interpreter, ctx->reg_stack);
    obj = (PObj*)ctx->current_sub;
    if (obj)
        pobject_lives(interpreter, obj);
    obj = (PObj*)ctx->current_object;
    if (obj)
        pobject_lives(interpreter, obj);
    /* the current continuation in the interpreter has
     * to be marked too in the call sequence currently
     * as e.g. a MMD search could need resources
     * and GC the continuation
     */
    obj = (PObj*)interpreter->current_cont;
    if (obj && obj != NEED_CONTINUATION)
        pobject_lives(interpreter, obj);
    obj = (PObj*)ctx->current_cont;
    if (obj && !PObj_live_TEST(obj))
        pobject_lives(interpreter, obj);
    obj = (PObj*)ctx->current_method;
    if (obj)
        pobject_lives(interpreter, obj);
    obj = (PObj*)ctx->current_namespace;
    if (obj)
        pobject_lives(interpreter, obj);
    obj = (PObj*)ctx->lex_pad;
    if (obj)
        pobject_lives(interpreter, obj);
    for (i = 0; i < ctx->n_regs_used[REGNO_PMC]; ++i) {
        obj = (PObj*) CTX_REG_PMC(ctx, i);
        if (obj)
            pobject_lives(interpreter, obj);
    }
    for (i = 0; i < ctx->n_regs_used[REGNO_STR]; ++i) {
        obj = (PObj*) CTX_REG_STR(ctx, i);
        if (obj)
            pobject_lives(interpreter, obj);
    }
}

/*

=item C<struct Parrot_sub *
new_sub(Interp *interp)>

Returns a new C<Parrot_sub>.

=cut

*/

struct Parrot_sub *
new_sub(Interp *interp)
{
    /* Using system memory until I figure out GC issues */
    struct Parrot_sub *newsub =
        mem_sys_allocate_zeroed(sizeof(struct Parrot_sub));
    newsub->seg = interp->code;
    return newsub;
}

/*

=item C<struct Parrot_sub *
new_closure(Interp *interp)>

Returns a new C<Parrot_sub> with its own sctatchpad.

XXX: Need to document semantics in detail.

=cut

*/

struct Parrot_sub *
new_closure(Interp *interp)
{
    struct Parrot_sub *newsub = new_sub(interp);
    return newsub;
}
/*

=item C<struct Parrot_cont *
new_continuation(Interp *interp, struct Parrot_cont *to)>

Returns a new C<Parrot_cont> to the context of C<to> with its own copy of the
current interpreter context.  If C<to> is C<NULL>, then the C<to_ctx> is set
to the current context.

=cut

*/


struct Parrot_cont *
new_continuation(Interp *interp, struct Parrot_cont *to)
{
    struct Parrot_cont *cc = mem_sys_allocate(sizeof(struct Parrot_cont));
    struct Parrot_Context *to_ctx = to ? to->to_ctx : CONTEXT(interp->ctx);

    cc->to_ctx = to_ctx;
    cc->from_ctx = CONTEXT(interp->ctx);
    CONTEXT(interp->ctx)->ref_count++;
    if (to) {
        cc->seg = to->seg;
        cc->address = to->address;
    }
    else {
        cc->seg = interp->code;
        cc->address = NULL;
    }
    cc->current_results = to_ctx->current_results;
    return cc;
}

/*

=item C<struct Parrot_cont *
new_ret_continuation(Interp *interp)>

Returns a new C<Parrot_cont> pointing to the current context.

=cut

*/

struct Parrot_cont *
new_ret_continuation(Interp *interp)
{
    struct Parrot_cont *cc = mem_sys_allocate(sizeof(struct Parrot_cont));
    cc->to_ctx = CONTEXT(interp->ctx);
    cc->from_ctx = NULL;    /* filled in during a call */
    cc->seg = interp->code;
    cc->current_results = NULL;
    cc->address = NULL;
    return cc;
}

/*

=item C<struct Parrot_coro *
new_coroutine(Interp *interp)>

Returns a new C<Parrot_coro>.

XXX: Need to document semantics in detail.

=cut

*/

struct Parrot_coro *
new_coroutine(Interp *interp)
{
    struct Parrot_coro *co =
        mem_sys_allocate_zeroed(sizeof(struct Parrot_coro));

    co->seg = interp->code;
    co->ctx = NULL;
    return co;
}

/*

=item C<PMC *
new_ret_continuation_pmc(Interp * interp, opcode_t * address)>

Returns a new C<RetContinuation> PMC. Uses one from the cache,
if possible; otherwise, creates a new one.

=cut

*/

PMC *
new_ret_continuation_pmc(Interp * interpreter, opcode_t * address)
{
    PMC* continuation;
    continuation = pmc_new(interpreter, enum_class_RetContinuation);
    VTABLE_set_pointer(interpreter, continuation, address);
    return continuation;
}

/*

=item C< void invalidate_retc_context(Interp *, PMC *cont)>

Make true Continuation from all RetContinuations up the call chain.

=cut

*/

void
invalidate_retc_context(Interp *interpreter, PMC *cont)
{
    struct Parrot_Context *ctx = PMC_cont(cont)->from_ctx;

    Parrot_set_context_threshold(interpreter, ctx);
    while (1) {
        /*
         * We  stop if we encounter a true continuation, because
         * if one were created, everything up the chain would have been
         * invalidated earlier.
         */
        if (cont->vtable != interpreter->vtables[enum_class_RetContinuation])
            break;
        cont->vtable = interpreter->vtables[enum_class_Continuation];
        ctx->ref_count++;
        cont = ctx->current_cont;
        ctx = PMC_cont(cont)->from_ctx;
    }

}

/*

=item C<Parrot_full_sub_name>

Print name and location of subroutine, This should finally use the label
name of the frozen C<Sub> PMC image for now locate the C<Sub> name in
the globals.

=cut

*/

STRING*
Parrot_full_sub_name(Interp* interpreter, PMC* sub)
{
    struct Parrot_sub * s;
    STRING *res;

    if (!sub || !VTABLE_defined(interpreter, sub))
        return NULL;
    s = PMC_sub(sub);
    if (PMC_IS_NULL(s->namespace)) {
        return s->name;
    } else {
        Parrot_block_DOD(interpreter);
        if (s->name) {
	    STRING* ns = VTABLE_get_string(interpreter, s->namespace);

    	    ns = string_concat(interpreter, ns,
		string_from_cstring(interpreter, " :: ", 4), 0);
	    res =  string_concat(interpreter, ns, s->name, 0);
        } else {
	    STRING* ns = string_from_cstring(interpreter, "??? :: ", 7);
	    res =  string_concat(interpreter, ns, s->name, 0);
	}
        Parrot_unblock_DOD(interpreter);
        return res;
    }
    return NULL;
}

int
Parrot_Context_info(Interp *interpreter, parrot_context_t *ctx,
                    struct Parrot_Context_info *info)
{
    struct Parrot_sub *sub;

    /* set file/line/pc defaults */
    info->file = "(unknown file)";
    info->line = -1;
    info->pc = -1;
    info->nsname = NULL;
    info->subname = NULL;
    info->fullname = NULL;

    /* is the current sub of the specified context valid? */
    if (PMC_IS_NULL(ctx->current_sub)) {
        info->subname = string_from_cstring(interpreter, "???", 3);
        info->nsname = info->subname;
        info->fullname = string_from_cstring(interpreter, "??? :: ???", 10);
        info->pc = -1;
        return 0;
    }

    /* fetch struct Parrot_sub of the current sub in the given context */
    if (!VTABLE_isa(interpreter, ctx->current_sub,
                    const_string(interpreter, "Sub")))
        return 1;

    sub = PMC_sub(ctx->current_sub);
    /* set the sub name */
    info->subname = sub->name;

    /* set the namespace name and fullname of the sub */
    if (PMC_IS_NULL(sub->namespace)) {
        info->nsname = string_from_cstring(interpreter, "", 0);
        info->fullname = info->subname;
    } else {
        info->nsname = VTABLE_get_string(interpreter, sub->namespace);
        info->fullname = string_concat(interpreter, info->nsname,
        string_from_cstring(interpreter, " :: ", 4), 0);
        info->fullname = string_concat(interpreter, info->fullname,
        info->subname, 1);
    }

    /* return here if there is no current pc */
    if (ctx->current_pc == NULL)
        return 1;

    /* calculate the current pc */
    info->pc = ctx->current_pc - sub->seg->base.data;

    /* determine the current source file/line */
    if (ctx->current_pc) {
        size_t offs = info->pc;
        size_t i, n;
        opcode_t *pc = sub->seg->base.data;
        struct PackFile_Debug *debug = sub->seg->debugs;
        if (!debug)
            return 0;
        for (i = n = 0; n < sub->seg->base.size; i++) {
            op_info_t *op_info = &interpreter->op_info_table[*pc];
            opcode_t var_args = 0;
            if (i >= debug->base.size)
                return 0;
            if (n >= offs) {
                /* set source line and file */
                info->line = debug->base.data[i];
                info->file = string_to_cstring(interpreter,
                Parrot_debug_pc_to_filename(interpreter, debug, i));
                break;
            }
            ADD_OP_VAR_PART(interpreter, sub->seg, pc, var_args);
            n += op_info->op_count + var_args;
            pc += op_info->op_count + var_args;
        }
    }
    return 1;
}

STRING*
Parrot_Context_infostr(Interp *interpreter, parrot_context_t *ctx)
{
    struct Parrot_Context_info info;
    const char* msg = (CONTEXT(interpreter->ctx) == ctx) ?
        "current instr.:":
        "called from Sub";
    STRING *res = NULL;

    Parrot_block_DOD(interpreter);
    if (Parrot_Context_info(interpreter, ctx, &info)) {
        res = Parrot_sprintf_c(interpreter,
            "%s '%Ss' pc %d (%s:%d)", msg,
            info.fullname, info.pc, info.file, info.line);
    }
    Parrot_unblock_DOD(interpreter);
    return res;
}

/*

=item C<PMC* Parrot_find_pad(Interp*, STRING *lex_name)>

Locate the LexPad containing the given name. Return NULL on failure.

=cut

*/

PMC*
Parrot_find_pad(Interp* interpreter, STRING *lex_name, parrot_context_t *ctx)
{
    PMC *lex_pad;
    parrot_context_t *outer;

    while (1) {
        lex_pad = ctx->lex_pad;
        outer = ctx->outer_ctx;
        if (!outer)
            return lex_pad;
        if (!PMC_IS_NULL(lex_pad)) {
            if (VTABLE_exists_keyed_str(interpreter, lex_pad, lex_name))
                return lex_pad;
        }
#if CTX_LEAK_DEBUG
        if (outer == ctx) {
            /* This is a bug; a context can never be its own :outer context.
             * Detecting it avoids an unbounded loop, which is difficult to
             * debug, though we'd rather not pay the cost of detection in a
             * production release.
             */
            real_exception(interpreter, NULL, INVALID_OPERATION,
                           "Bug:  Context %p :outer points back to itself.",
                           ctx);
        }
#endif
        ctx = outer;
    }
    return NULL;
}

PMC*
parrot_new_closure(Interp *interpreter, PMC *sub_pmc)
{
    PMC *clos_pmc;
    struct Parrot_sub *clos, *sub;
    PMC *cont;
    parrot_context_t *ctx;

    clos_pmc = VTABLE_clone(interpreter, sub_pmc);
    sub = PMC_sub(sub_pmc);
    clos = PMC_sub(clos_pmc);
    /* 
     * the given sub_pmc has to have an :outer(sub) that is
     * this subroutine
     */
    ctx = CONTEXT(interpreter->ctx);
    if (PMC_IS_NULL(sub->outer_sub)) {
        real_exception(interpreter, NULL, INVALID_OPERATION,
                "'%Ss' isn't a closure (no :outer)", sub->name);
    }
    /* if (sub->outer_sub != ctx->current_sub) - fails if outer
     * is a closure too e.g. test 'closure 4'
     */
    if (0 == string_equal(interpreter,
                (PMC_sub(ctx->current_sub))->name,
                sub->name)) {
        real_exception(interpreter, NULL, INVALID_OPERATION,
                "'%Ss' isn't the :outer of '%Ss')",
                (PMC_sub(ctx->current_sub))->name,
                sub->name);
    }
    cont = ctx->current_cont;
    /* preserve this frame by converting the continuation */
    cont->vtable = interpreter->vtables[enum_class_Continuation];
    /* remember this (the :outer) ctx in the closure */
    clos->outer_ctx = ctx;
    /* the closure refs now this context too */
    ctx->ref_count++;
#if CTX_LEAK_DEBUG
    if (Interp_debug_TEST(interpreter, PARROT_CTX_DESTROY_DEBUG_FLAG)) {
        fprintf(stderr, "[alloc closure  %p, outer_ctx %p, ref_count=%d]\n",
                clos_pmc, ctx, (int) ctx->ref_count);
    }
#endif
    return clos_pmc;
}
/*

=back

=head1 SEE ALSO

F<include/parrot/sub.h>.

=head1 HISTORY

Initial version by Melvin on 2002/06/6.

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
