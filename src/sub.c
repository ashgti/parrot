/*
Copyright (C) 2001-2009, Parrot Foundation.
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
#include "sub.str"
#include "pmc/pmc_sub.h"
#include "pmc/pmc_continuation.h"

/* HEADERIZER HFILE: include/parrot/sub.h */


/*

=item C<void mark_context_start(void)>

Indicate that a new round of context marking is about to take place.

=cut

*/

static int context_gc_mark = 0;

void
mark_context_start(void)
{
    ASSERT_ARGS(mark_context_start)
    if (++context_gc_mark == 0) context_gc_mark = 1;
}


/*

=item C<Parrot_cont * new_continuation(PARROT_INTERP, const Parrot_cont *to)>

Returns a new C<Parrot_cont> to the context of C<to> with its own copy of the
current interpreter context.  If C<to> is C<NULL>, then the C<to_ctx> is set
to the current context.

=cut

*/

PARROT_MALLOC
PARROT_CANNOT_RETURN_NULL
Parrot_cont *
new_continuation(PARROT_INTERP, ARGIN_NULLOK(const Parrot_cont *to))
{
    ASSERT_ARGS(new_continuation)
    Parrot_cont    * const cc     = mem_allocate_typed(Parrot_cont);
    PMC            * const to_ctx = to ? to->to_ctx : CONTEXT(interp);

    cc->to_ctx        = to_ctx;
    cc->from_ctx      = CONTEXT(interp);
    cc->runloop_id    = 0;
    if (to) {
        cc->seg       = to->seg;
        cc->address   = to->address;
    }
    else {
        cc->seg       = interp->code;
        cc->address   = NULL;
    }

    cc->current_results = CONTEXT_FIELD(to_ctx, current_results);
    return cc;
}

/*

=item C<Parrot_cont * new_ret_continuation(PARROT_INTERP)>

Returns a new C<Parrot_cont> pointing to the current context.

=cut

*/

PARROT_MALLOC
PARROT_CANNOT_RETURN_NULL
Parrot_cont *
new_ret_continuation(PARROT_INTERP)
{
    ASSERT_ARGS(new_ret_continuation)
    Parrot_cont * const cc = mem_allocate_typed(Parrot_cont);

    cc->to_ctx          = CONTEXT(interp);
    cc->from_ctx        = CONTEXT(interp);    /* filled in during a call */
    cc->runloop_id      = 0;
    cc->seg             = interp->code;
    cc->current_results = NULL;
    cc->address         = NULL;
    return cc;
}

/*

=item C<PMC * new_ret_continuation_pmc(PARROT_INTERP, opcode_t *address)>

Returns a new C<RetContinuation> PMC. Uses one from the cache,
if possible; otherwise, creates a new one.

=cut

*/

PARROT_EXPORT
PARROT_MALLOC
PARROT_CANNOT_RETURN_NULL
PMC *
new_ret_continuation_pmc(PARROT_INTERP, ARGIN_NULLOK(opcode_t *address))
{
    ASSERT_ARGS(new_ret_continuation_pmc)
    PMC* const continuation = pmc_new(interp, enum_class_RetContinuation);
    VTABLE_set_pointer(interp, continuation, address);
    return continuation;
}

/*

=item C<void invalidate_retc_context(PARROT_INTERP, PMC *cont)>

Make true Continuations from all RetContinuations up the call chain.

=cut

*/

void
invalidate_retc_context(PARROT_INTERP, ARGMOD(PMC *cont))
{
    ASSERT_ARGS(invalidate_retc_context)
    PMC *ctx = PMC_cont(cont)->from_ctx;
    cont = CONTEXT_FIELD(ctx, current_cont);

    while (1) {
        /*
         * We  stop if we encounter a true continuation, because
         * if one were created, everything up the chain would have been
         * invalidated earlier.
         */
        if (!cont || cont->vtable != interp->vtables[enum_class_RetContinuation])
            break;
        cont->vtable = interp->vtables[enum_class_Continuation];
        ctx  = CONTEXT_FIELD(ctx, caller_ctx);
        cont = CONTEXT_FIELD(ctx, current_cont);
    }

}

/*

=item C<STRING* Parrot_full_sub_name(PARROT_INTERP, PMC* sub_pmc)>

Return namespace, name, and location of subroutine.

=cut

*/

PARROT_EXPORT
PARROT_CAN_RETURN_NULL
PARROT_WARN_UNUSED_RESULT
STRING*
Parrot_full_sub_name(PARROT_INTERP, ARGIN_NULLOK(PMC* sub_pmc))
{
    ASSERT_ARGS(Parrot_full_sub_name)
    if (sub_pmc && VTABLE_defined(interp, sub_pmc)) {
        Parrot_Sub_attributes *sub;

        PMC_get_sub(interp, sub_pmc, sub);

        if (PMC_IS_NULL(sub->namespace_stash)) {
            return sub->name;
        }
        else {
            PMC    *ns_array;
            STRING * const semicolon = CONST_STRING(interp, ";");
            STRING *res;

            /*
             * When running with -t4, the invoke done in
             * Parrot_ns_get_name stomps on settings in interp; we
             * have to save these and restore them to avoid affecting
             * the running program.
             */
            PMC      * const saved_ccont       = interp->current_cont;
            opcode_t * const current_args      = interp->current_args;
            opcode_t * const current_params    = interp->current_params;
            opcode_t * const current_returns   = interp->current_returns;
            PMC      * const args_signature    = interp->args_signature;
            PMC      * const params_signature  = interp->params_signature;
            PMC      * const returns_signature = interp->returns_signature;

            Parrot_block_GC_mark(interp);

            ns_array = Parrot_ns_get_name(interp, sub->namespace_stash);

            /* Restore stuff that might have got overwritten */
            interp->current_cont      = saved_ccont;
            interp->current_args      = current_args;
            interp->current_params    = current_params;
            interp->current_returns   = current_returns;
            interp->args_signature    = args_signature;
            interp->params_signature  = params_signature;
            interp->returns_signature = returns_signature;

            if (sub->name)
                VTABLE_push_string(interp, ns_array, sub->name);

            res = Parrot_str_join(interp, semicolon, ns_array);
            Parrot_unblock_GC_mark(interp);
            return res;
        }
    }
    return NULL;
}

/*

=item C<int Parrot_Context_get_info(PARROT_INTERP, PMC *ctx, Parrot_Context_info
*info)>

Takes pointers to a context and its information table.
Populates the table and returns 0 or 1. XXX needs explanation
Used by Parrot_Context_infostr.

=cut

*/

PARROT_EXPORT
int
Parrot_Context_get_info(PARROT_INTERP, ARGIN(PMC *ctx),
                    ARGOUT(Parrot_Context_info *info))
{
    ASSERT_ARGS(Parrot_Context_get_info)
    Parrot_Sub_attributes *sub;

    /* set file/line/pc defaults */
    info->file     = CONST_STRING(interp, "(unknown file)");
    info->line     = -1;
    info->pc       = -1;
    info->nsname   = NULL;
    info->subname  = NULL;
    info->fullname = NULL;

    /* is the current sub of the specified context valid? */
    if (PMC_IS_NULL(CONTEXT_FIELD(ctx, current_sub))) {
        info->subname  = Parrot_str_new(interp, "???", 3);
        info->nsname   = info->subname;
        info->fullname = Parrot_str_new(interp, "??? :: ???", 10);
        info->pc       = -1;
        return 0;
    }

    /* fetch Parrot_sub of the current sub in the given context */
    if (!VTABLE_isa(interp, CONTEXT_FIELD(ctx, current_sub), CONST_STRING(interp, "Sub")))
        return 1;

    PMC_get_sub(interp, CONTEXT_FIELD(ctx, current_sub), sub);
    /* set the sub name */
    info->subname = sub->name;

    /* set the namespace name and fullname of the sub */
    if (PMC_IS_NULL(sub->namespace_name)) {
        info->nsname   = CONST_STRING(interp, "");
        info->fullname = info->subname;
    }
    else {
        info->nsname   = VTABLE_get_string(interp, sub->namespace_name);
        info->fullname = Parrot_full_sub_name(interp, CONTEXT_FIELD(ctx, current_sub));
    }

    /* return here if there is no current pc */
    if (CONTEXT_FIELD(ctx, current_pc) == NULL)
        return 1;

    /* calculate the current pc */
    info->pc = CONTEXT_FIELD(ctx, current_pc) - sub->seg->base.data;

    /* determine the current source file/line */
    if (CONTEXT_FIELD(ctx, current_pc)) {
        const size_t offs = info->pc;
        size_t i, n;
        opcode_t *pc = sub->seg->base.data;
        PackFile_Debug * const debug = sub->seg->debugs;

        if (!debug)
            return 0;
        for (i = n = 0; n < sub->seg->base.size; i++) {
            op_info_t * const op_info = &interp->op_info_table[*pc];
            opcode_t var_args = 0;

            if (i >= debug->base.size)
                return 0;
            if (n >= offs) {
                /* set source line and file */
                info->line = debug->base.data[i];
                info->file = Parrot_debug_pc_to_filename(interp, debug, i);
                break;
            }
            ADD_OP_VAR_PART(interp, sub->seg, pc, var_args);
            n  += op_info->op_count + var_args;
            pc += op_info->op_count + var_args;
        }
    }
    return 1;
}

/*

=item C<STRING* Parrot_Context_infostr(PARROT_INTERP, PMC *ctx)>

Formats context information for display.  Takes a context pointer and
returns a pointer to the text.  Used in debug.c and warnings.c

=cut

*/

PARROT_EXPORT
PARROT_CAN_RETURN_NULL
PARROT_WARN_UNUSED_RESULT
STRING*
Parrot_Context_infostr(PARROT_INTERP, ARGIN(PMC *ctx))
{
    ASSERT_ARGS(Parrot_Context_infostr)
    Parrot_Context_info info;
    STRING             *res = NULL;
    const char * const  msg = (CONTEXT(interp) == ctx)
        ? "current instr.:"
        : "called from Sub";

    Parrot_block_GC_mark(interp);
    if (Parrot_Context_get_info(interp, ctx, &info)) {

        res = Parrot_sprintf_c(interp,
            "%s '%Ss' pc %d (%Ss:%d)", msg,
            info.fullname, info.pc, info.file, info.line);
    }

    Parrot_unblock_GC_mark(interp);
    return res;
}

/*

=item C<PMC* Parrot_find_pad(PARROT_INTERP, STRING *lex_name, PMC *ctx)>

Locate the LexPad containing the given name. Return NULL on failure.

=cut

*/

PARROT_CAN_RETURN_NULL
PARROT_WARN_UNUSED_RESULT
PMC*
Parrot_find_pad(PARROT_INTERP, ARGIN(STRING *lex_name), ARGIN(PMC *ctx))
{
    ASSERT_ARGS(Parrot_find_pad)
    while (1) {
        PMC * const lex_pad = CONTEXT_FIELD(ctx, lex_pad);
        PMC        *outer   = CONTEXT_FIELD(ctx, outer_ctx);

        if (!outer)
            return lex_pad;

        if (!PMC_IS_NULL(lex_pad))
            if (VTABLE_exists_keyed_str(interp, lex_pad, lex_name))
                return lex_pad;

        ctx = outer;
    }
}


/*

=item C<void Parrot_capture_lex(PARROT_INTERP, PMC *sub_pmc)>

Capture the current lexical environment of a sub.

=cut

*/

void
Parrot_capture_lex(PARROT_INTERP, ARGMOD(PMC *sub_pmc))
{
    ASSERT_ARGS(Parrot_capture_lex)
    PMC            * const ctx          = CONTEXT(interp);
    Parrot_Sub_attributes *current_sub;
    Parrot_Sub_attributes *sub;

    PMC_get_sub(interp, CONTEXT_FIELD(ctx, current_sub), current_sub);

    /* MultiSub gets special treatment */
    if (VTABLE_isa(interp, sub_pmc, CONST_STRING(interp, "MultiSub"))) {

        PMC * const iter = VTABLE_get_iter(interp, sub_pmc);

        while (VTABLE_get_bool(interp, iter)) {

            PMC        * const child_pmc = VTABLE_shift_pmc(interp, iter);
            Parrot_Sub_attributes *child_sub, *child_outer_sub;

            PMC_get_sub(interp, child_pmc, child_sub);

            if (!PMC_IS_NULL(child_sub->outer_sub)) {
                PMC_get_sub(interp, child_sub->outer_sub, child_outer_sub);
                if (Parrot_str_equal(interp, current_sub->subid,
                                      child_outer_sub->subid)) {
                    child_sub->outer_ctx = ctx;
                }
            }
        }
        return;
    }

    /* the sub_pmc has to have an outer_sub that is the caller */
    PMC_get_sub(interp, sub_pmc, sub);
    if (PMC_IS_NULL(sub->outer_sub))
        return;

#if 0
    /* verify that the current sub is sub_pmc's :outer */
    PMC_get_sub(interp, sub->outer_sub, outer_sub);
    if (Parrot_str_not_equal(interp, current_sub->subid,
                         outer_sub->subid)) {
        Parrot_ex_throw_from_c_args(interp, NULL,
            EXCEPTION_INVALID_OPERATION, "'%Ss' isn't the :outer of '%Ss'",
            current_sub->name, sub->name);
        return;
    }
#endif

    /* set the sub's outer context to the current context */
    sub->outer_ctx = ctx;
}


/*

=item C<PMC* parrot_new_closure(PARROT_INTERP, PMC *sub_pmc)>

Used where? XXX

Creates a new closure, saving the context information.  Takes a pointer
to a subroutine.

Returns a pointer to the closure, (or throws exceptions if invalid).

=cut

*/

PARROT_EXPORT
PARROT_CANNOT_RETURN_NULL
PARROT_WARN_UNUSED_RESULT
PMC*
parrot_new_closure(PARROT_INTERP, ARGIN(PMC *sub_pmc))
{
    ASSERT_ARGS(parrot_new_closure)
    PMC        * const clos_pmc = VTABLE_clone(interp, sub_pmc);
    Parrot_capture_lex(interp, clos_pmc);
    return clos_pmc;
}


/*

=item C<void Parrot_continuation_check(PARROT_INTERP, const PMC *pmc, const
Parrot_cont *cc)>

Verifies that the provided continuation is sane.

=cut

*/

void
Parrot_continuation_check(PARROT_INTERP, ARGIN(const PMC *pmc),
    ARGIN(const Parrot_cont *cc))
{
    ASSERT_ARGS(Parrot_continuation_check)
    PMC *to_ctx       = cc->to_ctx;
    PMC *from_ctx     = CONTEXT(interp);

    if (PMC_IS_NULL(to_ctx))
        Parrot_ex_throw_from_c_args(interp, NULL, EXCEPTION_INVALID_OPERATION,
                       "Continuation invoked after deactivation.");
}


/*

=item C<void Parrot_continuation_rewind_environment(PARROT_INTERP, PMC *pmc,
Parrot_cont *cc)>

Restores the appropriate context for the continuation.

=cut

*/

void
Parrot_continuation_rewind_environment(PARROT_INTERP, SHIM(PMC *pmc),
        ARGIN(Parrot_cont *cc))
{
    ASSERT_ARGS(Parrot_continuation_rewind_environment)
    PMC * const to_ctx = cc->to_ctx;

    /* debug print before context is switched */
    if (Interp_trace_TEST(interp, PARROT_TRACE_SUB_CALL_FLAG)) {
        PMC * const sub = CONTEXT_FIELD(to_ctx, current_sub);

        Parrot_io_eprintf(interp, "# Back in sub '%Ss', env %p\n",
                    Parrot_full_sub_name(interp, sub),
                    interp->dynamic_env);
    }

    /* set context */
    CONTEXT(interp)      = to_ctx;
}


/*

=item C<void * Parrot_get_sub_pmc_from_subclass(PARROT_INTERP, PMC *subclass)>

Gets a Parrot_sub structure from something that isn't a Sub PMC, but rather a
subclass.

=cut

*/

PARROT_EXPORT
PARROT_CANNOT_RETURN_NULL
void *
Parrot_get_sub_pmc_from_subclass(PARROT_INTERP, ARGIN(PMC *subclass)) {
    ASSERT_ARGS(Parrot_get_sub_pmc_from_subclass)
    PMC        *key, *sub_pmc;
    Parrot_Sub_attributes *sub;

    /* Ensure we really do have a subclass of sub. */
    if (VTABLE_isa(interp, subclass, CONST_STRING(interp, "Sub"))) {
        /* If it's actually a PMC still, probably does the same structure
         * underneath. */
        if (!PObj_is_object_TEST(subclass)) {
            return PARROT_SUB(subclass);
        }

        /* Get the Sub PMC itself. */
        key = pmc_new(interp, enum_class_String);
        VTABLE_set_string_native(interp, key, CONST_STRING(interp, "Sub"));
        sub_pmc = VTABLE_get_attr_keyed(interp, subclass, key, CONST_STRING(interp, "proxy"));
        if (sub_pmc->vtable->base_type == enum_class_Sub) {
            return PARROT_SUB(sub_pmc);
        }
    }
    Parrot_ex_throw_from_c_args(interp, NULL, EXCEPTION_INVALID_OPERATION,
            "Attempting to do sub operation on non-Sub.");
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
 *   c-file-style: "parrot"
 * End:
 * vim: expandtab shiftwidth=4:
 */
