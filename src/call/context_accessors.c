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

/* HEADERIZER HFILE: include/parrot/context.h */

/*

=head2 Context API Functions

=over 4

=item C<Parrot_Context* Parrot_pcc_get_context_struct_func(PARROT_INTERP, PMC
*ctx)>

Fetch Parrot_Context from Context PMC.

=cut

*/

PARROT_EXPORT
PARROT_CAN_RETURN_NULL
PARROT_PURE_FUNCTION
Parrot_Context*
Parrot_pcc_get_context_struct_func(SHIM_INTERP, ARGIN_NULLOK(PMC *ctx))
{
    ASSERT_ARGS(Parrot_pcc_get_context_struct_func)

    if (PMC_IS_NULL(ctx))
        return NULL;

    return CONTEXT_STRUCT(ctx);
}

/*

=item C<struct PackFile_Constant ** Parrot_pcc_get_constants_func(PARROT_INTERP,
PMC *ctx)>

=item C<void Parrot_pcc_set_constants_func(PARROT_INTERP, PMC *ctx, struct
PackFile_Constant **constants)>

Get/set constants from context.

=cut

*/

PARROT_EXPORT
PARROT_CANNOT_RETURN_NULL
PARROT_PURE_FUNCTION
struct PackFile_Constant **
Parrot_pcc_get_constants_func(SHIM_INTERP, ARGIN(PMC *ctx))
{
    ASSERT_ARGS(Parrot_pcc_get_constants_func)
    return CONTEXT_STRUCT(ctx)->constants;
}

PARROT_EXPORT
PARROT_CAN_RETURN_NULL
void
Parrot_pcc_set_constants_func(SHIM_INTERP, ARGIN(PMC *ctx),
        ARGIN_NULLOK(struct PackFile_Constant **constants))
{
    ASSERT_ARGS(Parrot_pcc_set_constants_func)
    Parrot_Context * const c = CONTEXT_STRUCT(ctx);
    c->constants = constants;
}

/*

=item C<UINTVAL Parrot_pcc_get_recursion_depth_func(PARROT_INTERP, PMC *ctx)>

Get recursion depth from context.

=cut

*/

PARROT_EXPORT
PARROT_PURE_FUNCTION
UINTVAL
Parrot_pcc_get_recursion_depth_func(SHIM_INTERP, ARGIN(PMC *ctx))
{
    ASSERT_ARGS(Parrot_pcc_get_recursion_depth_func)
    const Parrot_Context * const c = CONTEXT_STRUCT(ctx);
    return c->recursion_depth;
}

/*

=item C<UINTVAL Parrot_pcc_inc_recursion_depth_func(PARROT_INTERP, PMC *ctx)>

Increase recursion depth. Returns previous recursion_depth value.

=cut

*/

PARROT_EXPORT
UINTVAL
Parrot_pcc_inc_recursion_depth_func(SHIM_INTERP, ARGIN(PMC *ctx))
{
    ASSERT_ARGS(Parrot_pcc_inc_recursion_depth_func)
    Parrot_Context * const c = CONTEXT_STRUCT(ctx);
    return c->recursion_depth++;
}

/*

=item C<UINTVAL Parrot_pcc_dec_recursion_depth_func(PARROT_INTERP, PMC *ctx)>

Decrease recursion depth. Returns new recursion_depth value.

=cut

*/

PARROT_EXPORT
UINTVAL
Parrot_pcc_dec_recursion_depth_func(SHIM_INTERP, ARGIN(PMC *ctx))
{
    ASSERT_ARGS(Parrot_pcc_dec_recursion_depth_func)
    Parrot_Context * const c = CONTEXT_STRUCT(ctx);
    return --c->recursion_depth;
}

/*

=item C<PMC* Parrot_pcc_get_caller_ctx_func(PARROT_INTERP, PMC *ctx)>

=item C<void Parrot_pcc_set_caller_ctx_func(PARROT_INTERP, PMC *ctx, PMC
*caller_ctx)>

Get/set caller Context.

=cut

*/

PARROT_EXPORT
PARROT_PURE_FUNCTION
PARROT_CAN_RETURN_NULL
PMC*
Parrot_pcc_get_caller_ctx_func(SHIM_INTERP, ARGIN(PMC *ctx))
{
    ASSERT_ARGS(Parrot_pcc_get_caller_ctx_func)
    const Parrot_Context * const c = CONTEXT_STRUCT(ctx);
    return c->caller_ctx;
}

PARROT_EXPORT
void
Parrot_pcc_set_caller_ctx_func(SHIM_INTERP, ARGIN(PMC *ctx), ARGIN(PMC *caller_ctx))
{
    ASSERT_ARGS(Parrot_pcc_set_caller_ctx_func)
    Parrot_Context * const c = CONTEXT_STRUCT(ctx);
    c->caller_ctx = caller_ctx;
}

/*

=item C<PMC* Parrot_pcc_get_outer_ctx_func(PARROT_INTERP, PMC *ctx)>

=item C<void Parrot_pcc_set_outer_ctx_func(PARROT_INTERP, PMC *ctx, PMC
*outer_ctx)>

Get/set outer Context.

=cut

*/

PARROT_EXPORT
PARROT_PURE_FUNCTION
PARROT_CAN_RETURN_NULL
PMC*
Parrot_pcc_get_outer_ctx_func(SHIM_INTERP, ARGIN(PMC *ctx))
{
    ASSERT_ARGS(Parrot_pcc_get_outer_ctx_func)
    const Parrot_Context * const c = CONTEXT_STRUCT(ctx);
    return c->outer_ctx;
}

PARROT_EXPORT
void
Parrot_pcc_set_outer_ctx_func(SHIM_INTERP, ARGIN(PMC *ctx), ARGIN(PMC *outer_ctx))
{
    ASSERT_ARGS(Parrot_pcc_set_outer_ctx_func)
    Parrot_Context * const c = CONTEXT_STRUCT(ctx);
    c->outer_ctx = outer_ctx;
}

/*

=item C<PMC* Parrot_pcc_get_lex_pad_func(PARROT_INTERP, PMC *ctx)>

=item C<void Parrot_pcc_set_lex_pad_func(PARROT_INTERP, PMC *ctx, PMC *lex_pad)>

Get/set LexPad.

=cut

*/

PARROT_EXPORT
PARROT_PURE_FUNCTION
PARROT_CANNOT_RETURN_NULL
PMC*
Parrot_pcc_get_lex_pad_func(SHIM_INTERP, ARGIN(PMC *ctx))
{
    ASSERT_ARGS(Parrot_pcc_get_lex_pad_func)
    const Parrot_Context * const c = CONTEXT_STRUCT(ctx);
    return c->lex_pad;
}

PARROT_EXPORT
void
Parrot_pcc_set_lex_pad_func(SHIM_INTERP, ARGIN(PMC *ctx), ARGIN(PMC *lex_pad))
{
    ASSERT_ARGS(Parrot_pcc_set_lex_pad_func)
    Parrot_Context * const c = CONTEXT_STRUCT(ctx);
    c->lex_pad = lex_pad;
}

/*

=item C<PMC* Parrot_pcc_get_namespace_func(PARROT_INTERP, PMC *ctx)>

=item C<void Parrot_pcc_set_namespace_func(PARROT_INTERP, PMC *ctx, PMC
*_namespace)>

Get/set namespace of Context.

=cut

*/

PARROT_EXPORT
PARROT_PURE_FUNCTION
PARROT_CAN_RETURN_NULL
PMC*
Parrot_pcc_get_namespace_func(SHIM_INTERP, ARGIN(PMC *ctx))
{
    ASSERT_ARGS(Parrot_pcc_get_namespace_func)
    const Parrot_Context * const c = CONTEXT_STRUCT(ctx);
    return c->current_namespace;
}

PARROT_EXPORT
void
Parrot_pcc_set_namespace_func(SHIM_INTERP, ARGIN(PMC *ctx), ARGIN_NULLOK(PMC *_namespace))
{
    ASSERT_ARGS(Parrot_pcc_set_namespace_func)
    Parrot_Context * const c = CONTEXT_STRUCT(ctx);
    c->current_namespace = _namespace;
}

/*

=item C<INTVAL Parrot_pcc_get_HLL_func(PARROT_INTERP, PMC *ctx)>

=item C<void Parrot_pcc_set_HLL_func(PARROT_INTERP, PMC *ctx, INTVAL hll)>

Get/set HLL of Context.

=cut

*/

PARROT_EXPORT
PARROT_PURE_FUNCTION
INTVAL
Parrot_pcc_get_HLL_func(SHIM_INTERP, ARGIN(PMC *ctx))
{
    ASSERT_ARGS(Parrot_pcc_get_HLL_func)
    const Parrot_Context * const c = CONTEXT_STRUCT(ctx);
    return c->current_HLL;
}

PARROT_EXPORT
void
Parrot_pcc_set_HLL_func(SHIM_INTERP, ARGIN(PMC *ctx), INTVAL hll)
{
    ASSERT_ARGS(Parrot_pcc_set_HLL_func)
    Parrot_Context * const c = CONTEXT_STRUCT(ctx);
    c->current_HLL = hll;
}

/*

=item C<PMC* Parrot_pcc_get_handlers_func(PARROT_INTERP, PMC *ctx)>

=item C<void Parrot_pcc_set_handlers_func(PARROT_INTERP, PMC *ctx, PMC
*handlers)>

Get/set scheduler handlers.

=cut

*/

PARROT_EXPORT
PARROT_PURE_FUNCTION
PARROT_CAN_RETURN_NULL
PMC*
Parrot_pcc_get_handlers_func(SHIM_INTERP, ARGIN(PMC *ctx))
{
    ASSERT_ARGS(Parrot_pcc_get_handlers_func)
    const Parrot_Context * const c = CONTEXT_STRUCT(ctx);
    return c->handlers;
}


PARROT_EXPORT
void
Parrot_pcc_set_handlers_func(SHIM_INTERP, ARGIN(PMC *ctx), ARGIN(PMC *handlers))
{
    ASSERT_ARGS(Parrot_pcc_set_handlers_func)
    Parrot_Context * const c = CONTEXT_STRUCT(ctx);
    c->handlers = handlers;
}

/*

=item C<PMC* Parrot_pcc_get_continuation_func(PARROT_INTERP, PMC *ctx)>

=item C<void Parrot_pcc_set_continuation_func(PARROT_INTERP, PMC *ctx, PMC
*_continuation)>

Get/set continuation of Context.

=cut

*/

PARROT_EXPORT
PARROT_PURE_FUNCTION
PARROT_CAN_RETURN_NULL
PMC*
Parrot_pcc_get_continuation_func(SHIM_INTERP, ARGIN(PMC *ctx))
{
    ASSERT_ARGS(Parrot_pcc_get_continuation_func)
    const Parrot_Context * const c = CONTEXT_STRUCT(ctx);
    return c->current_cont;
}

PARROT_EXPORT
void
Parrot_pcc_set_continuation_func(SHIM_INTERP, ARGIN(PMC *ctx), ARGIN_NULLOK(PMC *_continuation))
{
    ASSERT_ARGS(Parrot_pcc_set_continuation_func)
    Parrot_Context * const c = CONTEXT_STRUCT(ctx);
    c->current_cont = _continuation;
}

/*

=item C<PMC* Parrot_pcc_get_signature_func(PARROT_INTERP, PMC *ctx)>

=item C<void Parrot_pcc_set_signature_func(PARROT_INTERP, PMC *ctx, PMC
*sig_object)>

Get/set call signature object of Context (in sub/method call).

=cut

*/

PARROT_EXPORT
PARROT_PURE_FUNCTION
PARROT_CAN_RETURN_NULL
PMC*
Parrot_pcc_get_signature_func(SHIM_INTERP, ARGIN(PMC *ctx))
{
    ASSERT_ARGS(Parrot_pcc_get_signature_func)
    const Parrot_Context * const c = CONTEXT_STRUCT(ctx);
    return c->current_sig;
}

PARROT_EXPORT
void
Parrot_pcc_set_signature_func(SHIM_INTERP, ARGIN(PMC *ctx), ARGIN_NULLOK(PMC *sig_object))
{
    ASSERT_ARGS(Parrot_pcc_set_signature_func)
    Parrot_Context * const c = CONTEXT_STRUCT(ctx);
    c->current_sig = sig_object;
}

/*

=item C<PMC* Parrot_pcc_get_object_func(PARROT_INTERP, PMC *ctx)>

=item C<void Parrot_pcc_set_object_func(PARROT_INTERP, PMC *ctx, PMC *object)>

Get object of Context (in method call).

=cut

*/

PARROT_EXPORT
PARROT_PURE_FUNCTION
PARROT_CAN_RETURN_NULL
PMC*
Parrot_pcc_get_object_func(SHIM_INTERP, ARGIN(PMC *ctx))
{
    ASSERT_ARGS(Parrot_pcc_get_object_func)
    const Parrot_Context * const c = CONTEXT_STRUCT(ctx);
    return c->current_object;
}

PARROT_EXPORT
void
Parrot_pcc_set_object_func(SHIM_INTERP, ARGIN(PMC *ctx), ARGIN_NULLOK(PMC *object))
{
    ASSERT_ARGS(Parrot_pcc_set_object_func)
    Parrot_Context * const c = CONTEXT_STRUCT(ctx);
    c->current_object = object;
}

/*

=item C<opcode_t* Parrot_pcc_get_pc_func(PARROT_INTERP, PMC *ctx)>

=item C<void Parrot_pcc_set_pc_func(PARROT_INTERP, PMC *ctx, opcode_t *pc)>

Get/set program counter of Sub invocation.

=cut

*/

PARROT_EXPORT
PARROT_PURE_FUNCTION
PARROT_CAN_RETURN_NULL
opcode_t*
Parrot_pcc_get_pc_func(SHIM_INTERP, ARGIN(PMC *ctx))
{
    ASSERT_ARGS(Parrot_pcc_get_pc_func)
    const Parrot_Context * const c = CONTEXT_STRUCT(ctx);
    return c->current_pc;
}

PARROT_EXPORT
void
Parrot_pcc_set_pc_func(SHIM_INTERP, ARGIN(PMC *ctx), ARGIN_NULLOK(opcode_t *pc))
{
    ASSERT_ARGS(Parrot_pcc_set_pc_func)
    Parrot_Context * const c = CONTEXT_STRUCT(ctx);
    c->current_pc = pc;
}

/*

=item C<UINTVAL Parrot_pcc_warnings_on_func(PARROT_INTERP, PMC *ctx, UINTVAL
flags)>

Set warnings flags.

=cut

*/

PARROT_EXPORT
UINTVAL
Parrot_pcc_warnings_on_func(SHIM_INTERP, ARGIN(PMC *ctx), UINTVAL flags)
{
    ASSERT_ARGS(Parrot_pcc_warnings_on_func)
    Parrot_Context * const c = CONTEXT_STRUCT(ctx);
    c->warns |= flags;
    return c->warns;
}


/*

=item C<void Parrot_pcc_warnings_off_func(PARROT_INTERP, PMC *ctx, UINTVAL
flags)>

Clear warnings flags.

=cut

*/

PARROT_EXPORT
void
Parrot_pcc_warnings_off_func(SHIM_INTERP, ARGIN(PMC *ctx), UINTVAL flags)
{
    ASSERT_ARGS(Parrot_pcc_warnings_off_func)
    Parrot_Context * const c = CONTEXT_STRUCT(ctx);
    c->warns &= ~flags;
}


/*

=item C<UINTVAL Parrot_pcc_warnings_test_func(PARROT_INTERP, PMC *ctx, UINTVAL
flags)>

Test warnings flags.

=cut

*/

PARROT_EXPORT
PARROT_PURE_FUNCTION
UINTVAL
Parrot_pcc_warnings_test_func(SHIM_INTERP, ARGIN(PMC *ctx), UINTVAL flags)
{
    ASSERT_ARGS(Parrot_pcc_warnings_test_func)
    const Parrot_Context * const c = CONTEXT_STRUCT(ctx);
    return c->warns & flags;
}

/*

=item C<void Parrot_pcc_errors_on_func(PARROT_INTERP, PMC *ctx, UINTVAL flags)>

Set errors flags.

=cut

*/

PARROT_EXPORT
void
Parrot_pcc_errors_on_func(SHIM_INTERP, ARGIN(PMC *ctx), UINTVAL flags)
{
    ASSERT_ARGS(Parrot_pcc_errors_on_func)
    Parrot_Context * const c = CONTEXT_STRUCT(ctx);
    c->errors |= flags;
}


/*

=item C<void Parrot_pcc_errors_off_func(PARROT_INTERP, PMC *ctx, UINTVAL flags)>

Clear errors flags.

=cut

*/

PARROT_EXPORT
void
Parrot_pcc_errors_off_func(SHIM_INTERP, ARGIN(PMC *ctx), UINTVAL flags)
{
    ASSERT_ARGS(Parrot_pcc_errors_off_func)
    Parrot_Context * const c = CONTEXT_STRUCT(ctx);
    c->errors &= ~flags;
}

/*

=item C<UINTVAL Parrot_pcc_errors_test_func(PARROT_INTERP, PMC *ctx, UINTVAL
flags)>

Test errors flags.

=cut

*/

PARROT_EXPORT
PARROT_PURE_FUNCTION
UINTVAL
Parrot_pcc_errors_test_func(SHIM_INTERP, ARGIN(PMC *ctx), UINTVAL flags)
{
    ASSERT_ARGS(Parrot_pcc_errors_test_func)
    const Parrot_Context * const c = CONTEXT_STRUCT(ctx);
    return c->errors & flags;
}

/*

=item C<void Parrot_pcc_trace_flags_on_func(PARROT_INTERP, PMC *ctx, UINTVAL
flags)>

Set trace flags.

=cut

*/

PARROT_EXPORT
void
Parrot_pcc_trace_flags_on_func(SHIM_INTERP, ARGIN(PMC *ctx), UINTVAL flags)
{
    ASSERT_ARGS(Parrot_pcc_trace_flags_on_func)
    Parrot_Context * const c = CONTEXT_STRUCT(ctx);
    c->trace_flags |= flags;
}


/*

=item C<void Parrot_pcc_trace_flags_off_func(PARROT_INTERP, PMC *ctx, UINTVAL
flags)>

Clear trace flags.

=cut

*/

PARROT_EXPORT
void
Parrot_pcc_trace_flags_off_func(SHIM_INTERP, ARGIN(PMC *ctx), UINTVAL flags)
{
    ASSERT_ARGS(Parrot_pcc_trace_flags_off_func)
    Parrot_Context * const c = CONTEXT_STRUCT(ctx);
    c->trace_flags &= ~flags;
}

/*

=item C<UINTVAL Parrot_pcc_trace_flags_test_func(PARROT_INTERP, PMC *ctx,
UINTVAL flags)>

Test trace flags.

=cut

*/

PARROT_EXPORT
PARROT_PURE_FUNCTION
UINTVAL
Parrot_pcc_trace_flags_test_func(SHIM_INTERP, ARGIN(PMC *ctx), UINTVAL flags)
{
    ASSERT_ARGS(Parrot_pcc_trace_flags_test_func)
    const Parrot_Context * const c = CONTEXT_STRUCT(ctx);
    return c->trace_flags & flags;
}


/*

=item C<INTVAL Parrot_pcc_get_int_constant_func(PARROT_INTERP, PMC *ctx, INTVAL
idx)>

=item C<FLOATVAL Parrot_pcc_get_num_constant_func(PARROT_INTERP, PMC *ctx,
INTVAL idx)>

=item C<STRING* Parrot_pcc_get_string_constant_func(PARROT_INTERP, PMC *ctx,
INTVAL idx)>

=item C<PMC* Parrot_pcc_get_pmc_constant_func(PARROT_INTERP, PMC *ctx, INTVAL
idx)>

Get typed constant from context.

=cut

*/

PARROT_EXPORT
PARROT_PURE_FUNCTION
PARROT_CAN_RETURN_NULL
INTVAL
Parrot_pcc_get_int_constant_func(SHIM_INTERP, ARGIN(PMC *ctx), INTVAL idx)
{
    ASSERT_ARGS(Parrot_pcc_get_int_constant_func)
    const Parrot_Context * const c = CONTEXT_STRUCT(ctx);
    PARROT_ASSERT(c->constants[idx]->type == 'i');
    return c->constants[idx]->u.integer;
}

PARROT_EXPORT
PARROT_PURE_FUNCTION
PARROT_CAN_RETURN_NULL
FLOATVAL
Parrot_pcc_get_num_constant_func(SHIM_INTERP, ARGIN(PMC *ctx), INTVAL idx)
{
    ASSERT_ARGS(Parrot_pcc_get_num_constant_func)
    const Parrot_Context * const c = CONTEXT_STRUCT(ctx);
    PARROT_ASSERT(c->constants[idx]->type == 'n');
    return c->constants[idx]->u.number;
}

PARROT_EXPORT
PARROT_PURE_FUNCTION
PARROT_CAN_RETURN_NULL
STRING*
Parrot_pcc_get_string_constant_func(SHIM_INTERP, ARGIN(PMC *ctx), INTVAL idx)
{
    ASSERT_ARGS(Parrot_pcc_get_string_constant_func)
    const Parrot_Context * const c = CONTEXT_STRUCT(ctx);
    PARROT_ASSERT(c->constants[idx]->type == 's');
    return c->constants[idx]->u.string;
}

PARROT_EXPORT
PARROT_PURE_FUNCTION
PARROT_CAN_RETURN_NULL
PMC*
Parrot_pcc_get_pmc_constant_func(SHIM_INTERP, ARGIN(PMC *ctx), INTVAL idx)
{
    ASSERT_ARGS(Parrot_pcc_get_pmc_constant_func)
    const Parrot_Context * const c = CONTEXT_STRUCT(ctx);
    PARROT_ASSERT((c->constants[idx]->type == 'k')
            || (c->constants[idx]->type == 'p'));
    return c->constants[idx]->u.key;
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
