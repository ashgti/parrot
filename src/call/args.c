/*
Copyright (C) 2001-2009, Parrot Foundation.
$Id: pcc.c 41637 2009-10-03 14:46:07Z jkeenan $

=head1 Arguments and Returns

=head1 Description

Functions in this file handle argument/return value passing to and from
subroutines following the Parrot Calling Conventions.

=head2 Functions

=over 4

=cut

*/

#include "parrot/parrot.h"
#include "parrot/oplib/ops.h"
#include "parrot/runcore_api.h"
#include "args.str"

/* HEADERIZER HFILE: include/parrot/call.h */

/* HEADERIZER BEGIN: static */
/* Don't modify between HEADERIZER BEGIN / HEADERIZER END.  Your changes will be lost. */

PARROT_CAN_RETURN_NULL
static void parse_signature_string(PARROT_INTERP,
    ARGIN(const char *signature),
    ARGMOD(PMC **arg_flags),
    ARGMOD(PMC **return_flags))
        __attribute__nonnull__(1)
        __attribute__nonnull__(2)
        __attribute__nonnull__(3)
        __attribute__nonnull__(4)
        FUNC_MODIFIES(*arg_flags)
        FUNC_MODIFIES(*return_flags);

#define ASSERT_ARGS_parse_signature_string __attribute__unused__ int _ASSERT_ARGS_CHECK = (\
       PARROT_ASSERT_ARG(interp) \
    , PARROT_ASSERT_ARG(signature) \
    , PARROT_ASSERT_ARG(arg_flags) \
    , PARROT_ASSERT_ARG(return_flags))
/* Don't modify between HEADERIZER BEGIN / HEADERIZER END.  Your changes will be lost. */
/* HEADERIZER END: static */


/*

=item C<void Parrot_pcc_fill_returns_from_op(PARROT_INTERP, PMC *call_object,
PMC *raw_sig, opcode_t *raw_returns)>

Sets return values for the current function call. First it sets the
positional returns, then the named returns.

=cut

*/

PARROT_EXPORT
void
Parrot_pcc_fill_returns_from_op(PARROT_INTERP, ARGMOD_NULLOK(PMC *call_object),
        ARGIN(PMC *raw_sig), ARGIN(opcode_t *raw_returns))
{
    ASSERT_ARGS(Parrot_pcc_fill_returns_from_op)
    INTVAL return_list_elements;
    PMC *ctx = CURRENT_CONTEXT(interp);
    PMC *return_list;
    PMC *caller_return_flags;
    INTVAL raw_return_count     = VTABLE_elements(interp, raw_sig);
    INTVAL return_index = 0;
    INTVAL return_list_index = 0;
    INTVAL err_check      = 0;

    /* Check if we should be throwing errors. This is configured separately
     * for parameters and return values. */
    if (PARROT_ERRORS_test(interp, PARROT_ERRORS_RESULT_COUNT_FLAG))
            err_check = 1;

    /* A null call object is fine if there are no arguments and no returns. */
    if (PMC_IS_NULL(call_object)) {
        if (raw_return_count > 0) {
            if (err_check)
                Parrot_ex_throw_from_c_args(interp, NULL, EXCEPTION_INVALID_OPERATION,
                        "too many return values: %d passed, 0 expected",
                        raw_return_count);
            return;
        }
    }

    return_list = VTABLE_get_attr_str(interp, call_object, CONST_STRING(interp, "returns"));
    if (PMC_IS_NULL(return_list)) {
        if (raw_return_count > 0) {
            if (err_check)
                Parrot_ex_throw_from_c_args(interp, NULL, EXCEPTION_INVALID_OPERATION,
                        "too many return values: %d passed, 0 expected",
                        raw_return_count);
            return;
        }
    }
    else
        return_list_elements = VTABLE_elements(interp, return_list);

    caller_return_flags = VTABLE_get_attr_str(interp, call_object, CONST_STRING(interp, "return_flags"));


    if (raw_return_count > return_list_elements) {
        if (err_check)
            Parrot_ex_throw_from_c_args(interp, NULL, EXCEPTION_INVALID_OPERATION,
                    "too many return values: %d passed, %d expected",
                    raw_return_count, return_list_elements);
    }

    for (return_index = 0; return_index < raw_return_count; return_index++) {
        INTVAL return_flags = VTABLE_get_integer_keyed_int(interp,
                    raw_sig, return_index);
        INTVAL result_flags;

        const INTVAL constant  = PARROT_ARG_CONSTANT_ISSET(return_flags);
        const INTVAL raw_index = raw_returns[return_index + 2];
        PMC *result_item = VTABLE_get_pmc_keyed_int(interp, return_list, return_list_index);
        STRING *item_sig;

        /* Gracefully ignore extra returns when error checking is off. */
        if (PMC_IS_NULL(result_item))
            continue; /* Go on to next return arg. */

        result_flags = VTABLE_get_integer_keyed_int(interp, caller_return_flags, return_list_index);
        item_sig = VTABLE_get_string_keyed_str(interp, result_item, CONST_STRING(interp, ''));

        switch (PARROT_ARG_TYPE_MASK_MASK(return_flags)) {
            case PARROT_ARG_INTVAL:
                if (Parrot_str_equal(interp, item_sig, CONST_STRING(interp, "P"))) {
                    VTABLE_set_pmc(interp, result_item,
                            pmc_new(interp, Parrot_get_ctx_HLL_type(interp, enum_class_Integer)));
                }
                if (constant)
                    VTABLE_set_integer_native(interp, result_item, raw_index);
                else
                    VTABLE_set_integer_native(interp, result_item, CTX_REG_INT(ctx, raw_index));
                return_list_index++;
                break;
            case PARROT_ARG_FLOATVAL:
                if (Parrot_str_equal(interp, item_sig, CONST_STRING(interp, "P"))) {
                    VTABLE_set_pmc(interp, result_item,
                        pmc_new(interp, Parrot_get_ctx_HLL_type(interp, enum_class_Float)));
                }
                if (constant)
                    VTABLE_set_number_native(interp, result_item,
                            Parrot_pcc_get_num_constant(interp, ctx, raw_index));
                else
                    VTABLE_set_number_native(interp, result_item, CTX_REG_NUM(ctx, raw_index));
                return_list_index++;
                break;
            case PARROT_ARG_STRING:
                if (Parrot_str_equal(interp, item_sig, CONST_STRING(interp, "P"))) {
                    VTABLE_set_pmc(interp, result_item,
                            pmc_new(interp, Parrot_get_ctx_HLL_type(interp, enum_class_String)));
                }
                if (constant)
                    VTABLE_set_string_native(interp, result_item, Parrot_str_new_COW(interp,
                            Parrot_pcc_get_string_constant(interp, ctx, raw_index)));
                else
                    VTABLE_set_string_native(interp, result_item, CTX_REG_STR(ctx, raw_index));
                return_list_index++;
                break;
            case PARROT_ARG_PMC:
                if (constant)
                    VTABLE_set_pmc(interp, result_item,
                            Parrot_pcc_get_pmc_constant(interp, ctx, raw_index));
                else
                    VTABLE_set_pmc(interp, result_item, CTX_REG_PMC(ctx, raw_index));
                return_list_index++;
                break;
            default:
                Parrot_ex_throw_from_c_args(interp, NULL,
                        EXCEPTION_INVALID_OPERATION, "invalid parameter type");
                break;
        }
    }
}

/*

=item C<void Parrot_pcc_fill_returns_from_c_args(PARROT_INTERP, PMC
*call_object, const char *signature, ...)>

Sets return values for the current function call. First it sets the
positional returns, then the named returns.

The signature is a string in the format used for
C<Parrot_pcc_invoke_from_sig_object>, but with only return arguments.
The parameters are passed in as a list of INTVAL, FLOATVAL, STRING *, or
PMC * variables.


=cut

*/

PARROT_EXPORT
void
Parrot_pcc_fill_returns_from_c_args(PARROT_INTERP, ARGMOD(PMC *call_object),
        ARGIN(const char *signature), ...)
{
    ASSERT_ARGS(Parrot_pcc_fill_returns_from_c_args)
    va_list args;
    INTVAL return_list_elements;
    PMC *ctx = CURRENT_CONTEXT(interp);
    PMC * const return_list = VTABLE_get_attr_str(interp, call_object, CONST_STRING(interp, "returns"));
    INTVAL raw_return_count = 0;
    INTVAL return_index = 0;
    INTVAL return_list_index = 0;
    INTVAL err_check      = 0;

    PMC *raw_sig = PMCNULL;
    PMC *invalid_sig = PMCNULL;

    parse_signature_string(interp, signature, &raw_sig, &invalid_sig);
    if (!PMC_IS_NULL(invalid_sig))
        Parrot_ex_throw_from_c_args(interp, NULL, EXCEPTION_INVALID_OPERATION,
                "parameters should not be included in the return signature");
    raw_return_count = VTABLE_elements(interp, raw_sig);

    /* Check if we should be throwing errors. This is configured separately
     * for parameters and return values. */
    if (PARROT_ERRORS_test(interp, PARROT_ERRORS_RESULT_COUNT_FLAG))
            err_check = 1;

    if (PMC_IS_NULL(return_list)) {
        if (err_check)
            Parrot_ex_throw_from_c_args(interp, NULL, EXCEPTION_INVALID_OPERATION,
                    "too many return values: %d passed, 0 expected",
                    raw_return_count, return_list_elements);
        return;
    }
    else
        return_list_elements = VTABLE_elements(interp, return_list);


    if (raw_return_count > return_list_elements) {
        if (err_check)
            Parrot_ex_throw_from_c_args(interp, NULL, EXCEPTION_INVALID_OPERATION,
                    "too many return values: %d passed, %d expected",
                    raw_return_count, return_list_elements);
    }

    va_start(args, signature);
    for (return_index = 0; return_index < raw_return_count; return_index++) {
        STRING *item_sig;
        INTVAL return_flags = VTABLE_get_integer_keyed_int(interp,
                    raw_sig, return_index);

        PMC *result_item = VTABLE_get_pmc_keyed_int(interp, return_list, return_list_index);

        /* Gracefully ignore extra returns when error checking is off. */
        if (PMC_IS_NULL(result_item))
            continue; /* Go on to next return arg. */

        item_sig = VTABLE_get_string_keyed_str(interp, result_item, CONST_STRING(interp, ''));

        switch (PARROT_ARG_TYPE_MASK_MASK(return_flags)) {
            case PARROT_ARG_INTVAL:
                if (Parrot_str_equal(interp, item_sig, CONST_STRING(interp, "P"))) {
                    VTABLE_set_pmc(interp, result_item, pmc_new(
                        interp, Parrot_get_ctx_HLL_type(interp, enum_class_Integer)));
                }
                VTABLE_set_integer_native(interp, result_item, va_arg(args, INTVAL));
                return_list_index++;
                break;
            case PARROT_ARG_FLOATVAL:
                if (Parrot_str_equal(interp, item_sig, CONST_STRING(interp, "P"))) {
                    VTABLE_set_pmc(interp, result_item, pmc_new(
                        interp, Parrot_get_ctx_HLL_type(interp, enum_class_Float)));
                }
                VTABLE_set_number_native(interp, result_item, va_arg(args, FLOATVAL));
                return_list_index++;
                break;
            case PARROT_ARG_STRING:
                if (Parrot_str_equal(interp, item_sig, CONST_STRING(interp, "P"))) {
                    VTABLE_set_pmc(interp, result_item, pmc_new(
                        interp, Parrot_get_ctx_HLL_type(interp, enum_class_String)));
                }
                VTABLE_set_string_native(interp, result_item,
                        Parrot_str_new_COW(interp, va_arg(args, STRING *)));
                return_list_index++;
                break;
            case PARROT_ARG_PMC:
                VTABLE_set_pmc(interp, result_item, va_arg(args, PMC *));
                return_list_index++;
                break;
            default:
                Parrot_ex_throw_from_c_args(interp, NULL,
                        EXCEPTION_INVALID_OPERATION, "invalid parameter type");
                break;
        }
    }
    va_end(args);
}

/*

=item C<static void parse_signature_string(PARROT_INTERP, const char *signature,
PMC **arg_flags, PMC **return_flags)>

Parses a signature string and creates call and return signature integer
arrays. The two integer arrays should be passed in as references to a
PMC.

=cut

*/

PARROT_CAN_RETURN_NULL
static void
parse_signature_string(PARROT_INTERP, ARGIN(const char *signature),
        ARGMOD(PMC **arg_flags), ARGMOD(PMC **return_flags))
{
    ASSERT_ARGS(parse_signature_string)
    PMC *current_array;
    const char *x;
    INTVAL flags = 0;
    INTVAL set = 0;

    if (PMC_IS_NULL(*arg_flags))
        *arg_flags = pmc_new(interp, enum_class_ResizableIntegerArray);
    current_array = *arg_flags;

    for (x = signature; *x != '\0'; x++) {

        /* detect -> separator */
        if (*x == '-') {
            /* skip '>' */
            x++;
            /* Switch to the return argument flags. */
            if (PMC_IS_NULL(*return_flags))
                *return_flags = pmc_new(interp, enum_class_ResizableIntegerArray);
            current_array = *return_flags;
        }
        /* parse arg type */
        else if (isupper((unsigned char)*x)) {
            /* Starting a new argument, so store the previous argument,
             * if there was one. */
            if (set) {
                VTABLE_push_integer(interp, current_array, flags);
                set = 0;
            }

            switch (*x) {
                case 'I': flags = PARROT_ARG_INTVAL;   set++; break;
                case 'N': flags = PARROT_ARG_FLOATVAL; set++; break;
                case 'S': flags = PARROT_ARG_STRING;   set++; break;
                case 'P': flags = PARROT_ARG_PMC;      set++; break;
                default:
                  Parrot_ex_throw_from_c_args(interp, NULL,
                    EXCEPTION_INVALID_OPERATION,
                    "invalid signature string element %c!", *x);
            }

        }
        /* parse arg adverbs */
        else if (islower((unsigned char)*x)) {
            switch (*x) {
                case 'c': flags |= PARROT_ARG_CONSTANT;     break;
                case 'f': flags |= PARROT_ARG_FLATTEN;      break;
                case 'i': flags |= PARROT_ARG_INVOCANT;     break;
                case 'l': flags |= PARROT_ARG_LOOKAHEAD;    break;
                case 'n': flags |= PARROT_ARG_NAME;         break;
                case 'o': flags |= PARROT_ARG_OPTIONAL;     break;
                case 'p': flags |= PARROT_ARG_OPT_FLAG;     break;
                case 's': flags |= PARROT_ARG_SLURPY_ARRAY; break;
                default:
                    Parrot_ex_throw_from_c_args(interp, NULL,
                        EXCEPTION_INVALID_OPERATION,
                        "invalid signature string element %c!", *x);
            }
        }
    }

    /* Store the final argument, if there was one. */
    if (set)
        VTABLE_push_integer(interp, current_array, flags);
}

/*

=back

=head1 SEE ALSO

F<include/parrot/call.h>, F<src/call/ops.c>, F<src/call/pcc.c>.

=cut

*/

/*
 * Local variables:
 *   c-file-style: "parrot"
 * End:
 * vim: expandtab shiftwidth=4:
 */
