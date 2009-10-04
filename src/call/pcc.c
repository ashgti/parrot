/*
Copyright (C) 2001-2009, Parrot Foundation.
$Id$

=head1 Parrot Calling Conventions

=head1 Description

Functions in this file handle argument/return value passing to and from
subroutines.

=head2 Functions

=over 4

=cut

*/

#include "parrot/parrot.h"
#include "parrot/oplib/ops.h"
#include "parrot/runcore_api.h"
#include "pcc.str"
#include "../pmc/pmc_key.h"
#include "../pmc/pmc_continuation.h"

/* HEADERIZER HFILE: include/parrot/call.h */

/* HEADERIZER BEGIN: static */
/* Don't modify between HEADERIZER BEGIN / HEADERIZER END.  Your changes will be lost. */

/* Don't modify between HEADERIZER BEGIN / HEADERIZER END.  Your changes will be lost. */
/* HEADERIZER END: static */


/* Make sure we don't conflict with any other MAX() macros defined elsewhere */
#define PARROT_MAX(a, b) (((a)) > (b) ? (a) : (b))

/*

=item C<void Parrot_pcc_invoke_sub_from_c_args(PARROT_INTERP, PMC *sub_obj,
const char *sig, ...)>

Follows the same conventions as C<Parrot_PCCINVOKE>, but the subroutine object
to invoke is passed as an argument rather than looked up by name. The signature
string and call arguments are converted to a CallSignature PMC.

=cut

*/

PARROT_EXPORT
void
Parrot_pcc_invoke_sub_from_c_args(PARROT_INTERP, ARGIN(PMC *sub_obj),
        ARGIN(const char *sig), ...)
{
    ASSERT_ARGS(Parrot_pcc_invoke_sub_from_c_args)
    PMC    *sig_obj;
    va_list args;

    va_start(args, sig);
    sig_obj = Parrot_pcc_build_sig_object_from_varargs(interp, PMCNULL,
         sig, args);
    va_end(args);

    Parrot_pcc_invoke_from_sig_object(interp, sub_obj, sig_obj);
}


/*

=item C<void Parrot_PCCINVOKE(PARROT_INTERP, PMC* pmc, STRING *method_name,
const char *signature, ...)>

C<pmc> is the invocant.

C<method_name> is the same C<method_name> used in the C<find_method>
VTABLE call

C<signature> is a C string describing the Parrot calling conventions for
Parrot_PCCINVOKE.  ... variable args contains the IN arguments followed
by the OUT results variables.  You must pass the address_of(&) the OUT
results, of course.

Signatures:
  uppercase letters repesent each arg and denote its types

  I INTVAL
  N FLOATVAL
  S STRING *
  P PMC *

  lowercase letters are adverb modifiers to the preceeding uppercase arg
  identifier

  f flatten
  n named
  s slurpy
  o optional
  p opt flag
  l lookahead parameter (next positional, or next named if no positionals)
  i invocant

  -> is the separator between args and results, similar to type theory
  notation.

  Named args require two arg slots. The first is the name, the second the arg.

Example signature:

  "SnIPf->INPs"

  The args to the method invocation are
    a named INTVAL:  SnI
    a flattened PMC:  Pf

  The results of the method invocation are
    a INTVAL: I
    a FLOATVAL: N
    a slurpy PMC: Ps

invokes a PMC method

=cut

*/

PARROT_EXPORT
void
Parrot_PCCINVOKE(PARROT_INTERP, ARGIN(PMC* pmc), ARGMOD(STRING *method_name),
        ARGIN(const char *signature), ...)
{
    ASSERT_ARGS(Parrot_PCCINVOKE)
    PMC *sig_obj;
    PMC *sub_obj;
    va_list args;
    va_start(args, signature);
    sig_obj = Parrot_pcc_build_sig_object_from_varargs(interp, pmc, signature, args);
    va_end(args);

    /* Find the subroutine object as a named method on pmc */
    sub_obj = VTABLE_find_method(interp, pmc, method_name);
    if (PMC_IS_NULL(sub_obj))
         Parrot_ex_throw_from_c_args(interp, NULL, EXCEPTION_METHOD_NOT_FOUND,
             "Method '%Ss' not found", method_name);

    /* Invoke the subroutine object with the given CallSignature object */
    interp->current_object = pmc;
    Parrot_pcc_invoke_from_sig_object(interp, sub_obj, sig_obj);
    gc_unregister_pmc(interp, sig_obj);
}

/*

=item C<void Parrot_pcc_invoke_method_from_c_args(PARROT_INTERP, PMC* pmc,
STRING *method_name, const char *signature, ...)>

Makes a method call given the name of the method and the arguments as a C
variadic argument list. C<pmc> is the invocant, C<method_name> is the string
name of the method, C<signature> is a C string describing the signature of the
invocation, according to the Parrot calling conventions.  The variadic argument
list contains the input arguments followed by the output results in the same
order that they appear in the function signature.

=cut

*/

PARROT_EXPORT
void
Parrot_pcc_invoke_method_from_c_args(PARROT_INTERP, ARGIN(PMC* pmc),
        ARGMOD(STRING *method_name),
        ARGIN(const char *signature), ...)
{
    ASSERT_ARGS(Parrot_pcc_invoke_method_from_c_args)
    PMC    *sig_obj;
    PMC    *sub_obj;
    va_list args;

    va_start(args, signature);
    sig_obj = Parrot_pcc_build_sig_object_from_varargs(interp, pmc,
                 signature, args);
    va_end(args);

    /* Find the subroutine object as a named method on pmc */
    sub_obj = VTABLE_find_method(interp, pmc, method_name);

    if (PMC_IS_NULL(sub_obj))
         Parrot_ex_throw_from_c_args(interp, NULL, EXCEPTION_METHOD_NOT_FOUND,
             "Method '%Ss' not found", method_name);

    /* Invoke the subroutine object with the given CallSignature object */
    interp->current_object = pmc;
    Parrot_pcc_invoke_from_sig_object(interp, sub_obj, sig_obj);
    gc_unregister_pmc(interp, sig_obj);

}


/*

=item C<void Parrot_pcc_invoke_from_sig_object(PARROT_INTERP, PMC *sub_obj, PMC
*call_object)>

Follows the same conventions as C<Parrot_PCCINVOKE>, but the subroutine object
to invoke is passed as an argument rather than looked up by name, and the
signature string and call arguments are passed in a CallSignature PMC.

=cut

*/

PARROT_EXPORT
void
Parrot_pcc_invoke_from_sig_object(PARROT_INTERP, ARGIN(PMC *sub_obj),
        ARGIN(PMC *call_object))
{
    ASSERT_ARGS(Parrot_pcc_invoke_from_sig_object)

    opcode_t *dest;
    INTVAL n_regs_used[] = { 0, 0, 0, 0, 0, 0, 0, 0 };
    PMC *ctx  = Parrot_push_context(interp, n_regs_used);
    PMC * const ret_cont = new_ret_continuation_pmc(interp, NULL);

    Parrot_pcc_set_signature(interp, ctx, call_object);
    Parrot_pcc_set_continuation(interp, ctx, ret_cont);
    interp->current_cont         = NEED_CONTINUATION;
    PARROT_CONTINUATION(ret_cont)->from_ctx = ctx;

    /* Invoke the function */
    dest = VTABLE_invoke(interp, sub_obj, NULL);

    /* PIR Subs need runops to run their opcodes. Methods and NCI subs
     * don't. */
    if (sub_obj->vtable->base_type == enum_class_Sub
            && PMC_IS_NULL(interp->current_object)) {
        Parrot_runcore_t *old_core = interp->run_core;
        const opcode_t offset = dest - interp->code->base.data;

        /* can't re-enter the runloop from here with PIC cores: RT #60048 */
        if (PARROT_RUNCORE_PREDEREF_OPS_TEST(interp->run_core))
            Parrot_runcore_switch(interp, CONST_STRING(interp, "slow"));

        runops(interp, offset);
        Interp_core_SET(interp, old_core);
    }
    gc_unregister_pmc(interp, call_object);
    Parrot_pcc_set_signature(interp, ctx, NULL);
    Parrot_pop_context(interp);
}

/*

=back

=head1 SEE ALSO

F<include/parrot/interpreter.h>, F<src/call/ops.c>, F<src/pmc/sub.pmc>.

=cut

*/

/*
 * Local variables:
 *   c-file-style: "parrot"
 * End:
 * vim: expandtab shiftwidth=4:
 */
