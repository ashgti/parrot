/*
Copyright (C) 2001-2009, Parrot Foundation.
$Id$

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
#include "../pmc/pmc_key.h"

/* HEADERIZER HFILE: include/parrot/call.h */

/*
Set of functions used in generic versions of fill_params and fill_returns.
*/
typedef INTVAL*   (*intval_ptr_func_t)(PARROT_INTERP, void *arg_info, INTVAL index);
typedef FLOATVAL* (*numval_ptr_func_t)(PARROT_INTERP, void *arg_info, INTVAL index);
typedef STRING**  (*string_ptr_func_t)(PARROT_INTERP, void *arg_info, INTVAL index);
typedef PMC**     (*pmc_ptr_func_t)   (PARROT_INTERP, void *arg_info, INTVAL index);

typedef INTVAL    (*intval_func_t)(PARROT_INTERP, void *arg_info, INTVAL index);
typedef FLOATVAL  (*numval_func_t)(PARROT_INTERP, void *arg_info, INTVAL index);
typedef STRING*   (*string_func_t)(PARROT_INTERP, void *arg_info, INTVAL index);
typedef PMC*      (*pmc_func_t)   (PARROT_INTERP, void *arg_info, INTVAL index);

typedef struct pcc_set_funcs {
    intval_ptr_func_t   intval;
    numval_ptr_func_t   numval;
    string_ptr_func_t   string;
    pmc_ptr_func_t      pmc;

    intval_func_t   intval_constant;
    numval_func_t   numval_constant;
    string_func_t   string_constant;
    pmc_func_t      pmc_constant;
} pcc_set_funcs;

typedef struct pcc_get_funcs {
    intval_func_t   intval;
    numval_func_t   numval;
    string_func_t   string;
    pmc_func_t      pmc;

    intval_func_t   intval_constant;
    numval_func_t   numval_constant;
    string_func_t   string_constant;
    pmc_func_t      pmc_constant;
} pcc_get_funcs;

/* HEADERIZER BEGIN: static */
/* Don't modify between HEADERIZER BEGIN / HEADERIZER END.  Your changes will be lost. */

static void assign_default_param_value(PARROT_INTERP,
    INTVAL param_index,
    INTVAL param_flags,
    ARGIN(void *arg_info),
    ARGIN(struct pcc_set_funcs *accessor))
        __attribute__nonnull__(1)
        __attribute__nonnull__(4)
        __attribute__nonnull__(5);

static void assign_default_result_value(PARROT_INTERP,
    ARGMOD(PMC *result),
    INTVAL result_flags)
        __attribute__nonnull__(1)
        __attribute__nonnull__(2)
        FUNC_MODIFIES(*result);

PARROT_CAN_RETURN_NULL
static PMC* clone_key_arg(PARROT_INTERP, ARGIN(PMC *key))
        __attribute__nonnull__(1)
        __attribute__nonnull__(2);

PARROT_CANNOT_RETURN_NULL
static STRING * dissect_aggregate_arg(PARROT_INTERP,
    ARGMOD(PMC *call_object),
    ARGIN(PMC *aggregate))
        __attribute__nonnull__(1)
        __attribute__nonnull__(2)
        __attribute__nonnull__(3)
        FUNC_MODIFIES(*call_object);

static void extract_named_arg_from_op(PARROT_INTERP,
    ARGMOD(PMC *call_object),
    ARGIN(STRING *name),
    ARGIN(PMC * const raw_sig),
    ARGIN(opcode_t * const raw_args),
    INTVAL arg_index)
        __attribute__nonnull__(1)
        __attribute__nonnull__(2)
        __attribute__nonnull__(3)
        __attribute__nonnull__(4)
        __attribute__nonnull__(5)
        FUNC_MODIFIES(*call_object);

static void fill_params(PARROT_INTERP,
    ARGMOD_NULLOK(PMC *call_object),
    ARGIN(PMC *raw_sig),
    ARGIN(void *arg_info),
    ARGIN(struct pcc_set_funcs *accessor))
        __attribute__nonnull__(1)
        __attribute__nonnull__(3)
        __attribute__nonnull__(4)
        __attribute__nonnull__(5)
        FUNC_MODIFIES(*call_object);

static void fill_results(PARROT_INTERP,
    ARGMOD_NULLOK(PMC *call_object),
    ARGIN(PMC *raw_sig),
    ARGIN(void *return_info),
    ARGIN(struct pcc_get_funcs *accessor))
        __attribute__nonnull__(1)
        __attribute__nonnull__(3)
        __attribute__nonnull__(4)
        __attribute__nonnull__(5)
        FUNC_MODIFIES(*call_object);

PARROT_CANNOT_RETURN_NULL
static INTVAL intval_arg_from_c_args(PARROT_INTERP,
    ARGIN(va_list *args),
    SHIM(INTVAL param_index))
        __attribute__nonnull__(1)
        __attribute__nonnull__(2);

PARROT_CANNOT_RETURN_NULL
static INTVAL intval_arg_from_continuation(PARROT_INTERP,
    ARGIN(PMC *cs),
    INTVAL arg_index)
        __attribute__nonnull__(1)
        __attribute__nonnull__(2);

PARROT_CANNOT_RETURN_NULL
static INTVAL intval_arg_from_op(PARROT_INTERP,
    ARGIN(opcode_t *raw_args),
    INTVAL arg_index)
        __attribute__nonnull__(1)
        __attribute__nonnull__(2);

static INTVAL intval_constant_from_op(PARROT_INTERP,
    ARGIN(opcode_t *raw_params),
    INTVAL param_index)
        __attribute__nonnull__(1)
        __attribute__nonnull__(2);

static INTVAL intval_constant_from_varargs(PARROT_INTERP,
    ARGIN(void *data),
    INTVAL index)
        __attribute__nonnull__(1)
        __attribute__nonnull__(2);

PARROT_CANNOT_RETURN_NULL
static INTVAL* intval_param_from_c_args(PARROT_INTERP,
    ARGIN(va_list *args),
    SHIM(INTVAL param_index))
        __attribute__nonnull__(1)
        __attribute__nonnull__(2);

PARROT_CANNOT_RETURN_NULL
static INTVAL* intval_param_from_op(PARROT_INTERP,
    ARGIN(opcode_t *raw_params),
    INTVAL param_index)
        __attribute__nonnull__(1)
        __attribute__nonnull__(2);

PARROT_CANNOT_RETURN_NULL
static FLOATVAL numval_arg_from_c_args(PARROT_INTERP,
    ARGIN(va_list *args),
    SHIM(INTVAL param_index))
        __attribute__nonnull__(1)
        __attribute__nonnull__(2);

PARROT_CANNOT_RETURN_NULL
static FLOATVAL numval_arg_from_continuation(PARROT_INTERP,
    ARGIN(PMC *cs),
    INTVAL arg_index)
        __attribute__nonnull__(1)
        __attribute__nonnull__(2);

PARROT_CANNOT_RETURN_NULL
static FLOATVAL numval_arg_from_op(PARROT_INTERP,
    ARGIN(opcode_t *raw_args),
    INTVAL arg_index)
        __attribute__nonnull__(1)
        __attribute__nonnull__(2);

static FLOATVAL numval_constant_from_op(PARROT_INTERP,
    ARGIN(opcode_t *raw_params),
    INTVAL param_index)
        __attribute__nonnull__(1)
        __attribute__nonnull__(2);

static FLOATVAL numval_constant_from_varargs(PARROT_INTERP,
    ARGIN(void *data),
    INTVAL index)
        __attribute__nonnull__(1)
        __attribute__nonnull__(2);

PARROT_CANNOT_RETURN_NULL
static FLOATVAL* numval_param_from_c_args(PARROT_INTERP,
    ARGIN(va_list *args),
    SHIM(INTVAL param_index))
        __attribute__nonnull__(1)
        __attribute__nonnull__(2);

PARROT_CANNOT_RETURN_NULL
static FLOATVAL* numval_param_from_op(PARROT_INTERP,
    ARGIN(opcode_t *raw_params),
    INTVAL param_index)
        __attribute__nonnull__(1)
        __attribute__nonnull__(2);

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

PARROT_CANNOT_RETURN_NULL
static PMC* pmc_arg_from_c_args(PARROT_INTERP,
    ARGIN(va_list *args),
    SHIM(INTVAL param_index))
        __attribute__nonnull__(1)
        __attribute__nonnull__(2);

PARROT_CANNOT_RETURN_NULL
static PMC* pmc_arg_from_continuation(PARROT_INTERP,
    ARGIN(PMC *cs),
    INTVAL arg_index)
        __attribute__nonnull__(1)
        __attribute__nonnull__(2);

PARROT_CANNOT_RETURN_NULL
static PMC* pmc_arg_from_op(PARROT_INTERP,
    ARGIN(opcode_t *raw_args),
    INTVAL arg_index)
        __attribute__nonnull__(1)
        __attribute__nonnull__(2);

PARROT_CAN_RETURN_NULL
static PMC* pmc_constant_from_op(PARROT_INTERP,
    ARGIN(opcode_t *raw_params),
    INTVAL param_index)
        __attribute__nonnull__(1)
        __attribute__nonnull__(2);

PARROT_CAN_RETURN_NULL
static PMC* pmc_constant_from_varargs(PARROT_INTERP,
    ARGIN(void *data),
    INTVAL index)
        __attribute__nonnull__(1)
        __attribute__nonnull__(2);

PARROT_CANNOT_RETURN_NULL
static PMC** pmc_param_from_c_args(PARROT_INTERP,
    ARGIN(va_list *args),
    SHIM(INTVAL param_index))
        __attribute__nonnull__(1)
        __attribute__nonnull__(2);

PARROT_CANNOT_RETURN_NULL
static PMC** pmc_param_from_op(PARROT_INTERP,
    ARGIN(opcode_t *raw_params),
    INTVAL param_index)
        __attribute__nonnull__(1)
        __attribute__nonnull__(2);

PARROT_CANNOT_RETURN_NULL
static STRING* string_arg_from_c_args(PARROT_INTERP,
    ARGIN(va_list *args),
    SHIM(INTVAL param_index))
        __attribute__nonnull__(1)
        __attribute__nonnull__(2);

PARROT_CANNOT_RETURN_NULL
static STRING* string_arg_from_continuation(PARROT_INTERP,
    ARGIN(PMC *cs),
    INTVAL arg_index)
        __attribute__nonnull__(1)
        __attribute__nonnull__(2);

PARROT_CANNOT_RETURN_NULL
static STRING* string_arg_from_op(PARROT_INTERP,
    ARGIN(opcode_t *raw_args),
    INTVAL arg_index)
        __attribute__nonnull__(1)
        __attribute__nonnull__(2);

PARROT_CAN_RETURN_NULL
static STRING* string_constant_from_op(PARROT_INTERP,
    ARGIN(opcode_t *raw_params),
    INTVAL param_index)
        __attribute__nonnull__(1)
        __attribute__nonnull__(2);

PARROT_CAN_RETURN_NULL
static STRING* string_constant_from_varargs(PARROT_INTERP,
    ARGIN(void *data),
    INTVAL index)
        __attribute__nonnull__(1)
        __attribute__nonnull__(2);

PARROT_CANNOT_RETURN_NULL
static STRING** string_param_from_c_args(PARROT_INTERP,
    ARGIN(va_list *args),
    SHIM(INTVAL param_index))
        __attribute__nonnull__(1)
        __attribute__nonnull__(2);

PARROT_CANNOT_RETURN_NULL
static STRING** string_param_from_op(PARROT_INTERP,
    ARGIN(opcode_t *raw_params),
    INTVAL param_index)
        __attribute__nonnull__(1)
        __attribute__nonnull__(2);

#define ASSERT_ARGS_assign_default_param_value __attribute__unused__ int _ASSERT_ARGS_CHECK = (\
       PARROT_ASSERT_ARG(interp) \
    , PARROT_ASSERT_ARG(arg_info) \
    , PARROT_ASSERT_ARG(accessor))
#define ASSERT_ARGS_assign_default_result_value __attribute__unused__ int _ASSERT_ARGS_CHECK = (\
       PARROT_ASSERT_ARG(interp) \
    , PARROT_ASSERT_ARG(result))
#define ASSERT_ARGS_clone_key_arg __attribute__unused__ int _ASSERT_ARGS_CHECK = (\
       PARROT_ASSERT_ARG(interp) \
    , PARROT_ASSERT_ARG(key))
#define ASSERT_ARGS_dissect_aggregate_arg __attribute__unused__ int _ASSERT_ARGS_CHECK = (\
       PARROT_ASSERT_ARG(interp) \
    , PARROT_ASSERT_ARG(call_object) \
    , PARROT_ASSERT_ARG(aggregate))
#define ASSERT_ARGS_extract_named_arg_from_op __attribute__unused__ int _ASSERT_ARGS_CHECK = (\
       PARROT_ASSERT_ARG(interp) \
    , PARROT_ASSERT_ARG(call_object) \
    , PARROT_ASSERT_ARG(name) \
    , PARROT_ASSERT_ARG(raw_sig) \
    , PARROT_ASSERT_ARG(raw_args))
#define ASSERT_ARGS_fill_params __attribute__unused__ int _ASSERT_ARGS_CHECK = (\
       PARROT_ASSERT_ARG(interp) \
    , PARROT_ASSERT_ARG(raw_sig) \
    , PARROT_ASSERT_ARG(arg_info) \
    , PARROT_ASSERT_ARG(accessor))
#define ASSERT_ARGS_fill_results __attribute__unused__ int _ASSERT_ARGS_CHECK = (\
       PARROT_ASSERT_ARG(interp) \
    , PARROT_ASSERT_ARG(raw_sig) \
    , PARROT_ASSERT_ARG(return_info) \
    , PARROT_ASSERT_ARG(accessor))
#define ASSERT_ARGS_intval_arg_from_c_args __attribute__unused__ int _ASSERT_ARGS_CHECK = (\
       PARROT_ASSERT_ARG(interp) \
    , PARROT_ASSERT_ARG(args))
#define ASSERT_ARGS_intval_arg_from_continuation __attribute__unused__ int _ASSERT_ARGS_CHECK = (\
       PARROT_ASSERT_ARG(interp) \
    , PARROT_ASSERT_ARG(cs))
#define ASSERT_ARGS_intval_arg_from_op __attribute__unused__ int _ASSERT_ARGS_CHECK = (\
       PARROT_ASSERT_ARG(interp) \
    , PARROT_ASSERT_ARG(raw_args))
#define ASSERT_ARGS_intval_constant_from_op __attribute__unused__ int _ASSERT_ARGS_CHECK = (\
       PARROT_ASSERT_ARG(interp) \
    , PARROT_ASSERT_ARG(raw_params))
#define ASSERT_ARGS_intval_constant_from_varargs __attribute__unused__ int _ASSERT_ARGS_CHECK = (\
       PARROT_ASSERT_ARG(interp) \
    , PARROT_ASSERT_ARG(data))
#define ASSERT_ARGS_intval_param_from_c_args __attribute__unused__ int _ASSERT_ARGS_CHECK = (\
       PARROT_ASSERT_ARG(interp) \
    , PARROT_ASSERT_ARG(args))
#define ASSERT_ARGS_intval_param_from_op __attribute__unused__ int _ASSERT_ARGS_CHECK = (\
       PARROT_ASSERT_ARG(interp) \
    , PARROT_ASSERT_ARG(raw_params))
#define ASSERT_ARGS_numval_arg_from_c_args __attribute__unused__ int _ASSERT_ARGS_CHECK = (\
       PARROT_ASSERT_ARG(interp) \
    , PARROT_ASSERT_ARG(args))
#define ASSERT_ARGS_numval_arg_from_continuation __attribute__unused__ int _ASSERT_ARGS_CHECK = (\
       PARROT_ASSERT_ARG(interp) \
    , PARROT_ASSERT_ARG(cs))
#define ASSERT_ARGS_numval_arg_from_op __attribute__unused__ int _ASSERT_ARGS_CHECK = (\
       PARROT_ASSERT_ARG(interp) \
    , PARROT_ASSERT_ARG(raw_args))
#define ASSERT_ARGS_numval_constant_from_op __attribute__unused__ int _ASSERT_ARGS_CHECK = (\
       PARROT_ASSERT_ARG(interp) \
    , PARROT_ASSERT_ARG(raw_params))
#define ASSERT_ARGS_numval_constant_from_varargs __attribute__unused__ int _ASSERT_ARGS_CHECK = (\
       PARROT_ASSERT_ARG(interp) \
    , PARROT_ASSERT_ARG(data))
#define ASSERT_ARGS_numval_param_from_c_args __attribute__unused__ int _ASSERT_ARGS_CHECK = (\
       PARROT_ASSERT_ARG(interp) \
    , PARROT_ASSERT_ARG(args))
#define ASSERT_ARGS_numval_param_from_op __attribute__unused__ int _ASSERT_ARGS_CHECK = (\
       PARROT_ASSERT_ARG(interp) \
    , PARROT_ASSERT_ARG(raw_params))
#define ASSERT_ARGS_parse_signature_string __attribute__unused__ int _ASSERT_ARGS_CHECK = (\
       PARROT_ASSERT_ARG(interp) \
    , PARROT_ASSERT_ARG(signature) \
    , PARROT_ASSERT_ARG(arg_flags) \
    , PARROT_ASSERT_ARG(return_flags))
#define ASSERT_ARGS_pmc_arg_from_c_args __attribute__unused__ int _ASSERT_ARGS_CHECK = (\
       PARROT_ASSERT_ARG(interp) \
    , PARROT_ASSERT_ARG(args))
#define ASSERT_ARGS_pmc_arg_from_continuation __attribute__unused__ int _ASSERT_ARGS_CHECK = (\
       PARROT_ASSERT_ARG(interp) \
    , PARROT_ASSERT_ARG(cs))
#define ASSERT_ARGS_pmc_arg_from_op __attribute__unused__ int _ASSERT_ARGS_CHECK = (\
       PARROT_ASSERT_ARG(interp) \
    , PARROT_ASSERT_ARG(raw_args))
#define ASSERT_ARGS_pmc_constant_from_op __attribute__unused__ int _ASSERT_ARGS_CHECK = (\
       PARROT_ASSERT_ARG(interp) \
    , PARROT_ASSERT_ARG(raw_params))
#define ASSERT_ARGS_pmc_constant_from_varargs __attribute__unused__ int _ASSERT_ARGS_CHECK = (\
       PARROT_ASSERT_ARG(interp) \
    , PARROT_ASSERT_ARG(data))
#define ASSERT_ARGS_pmc_param_from_c_args __attribute__unused__ int _ASSERT_ARGS_CHECK = (\
       PARROT_ASSERT_ARG(interp) \
    , PARROT_ASSERT_ARG(args))
#define ASSERT_ARGS_pmc_param_from_op __attribute__unused__ int _ASSERT_ARGS_CHECK = (\
       PARROT_ASSERT_ARG(interp) \
    , PARROT_ASSERT_ARG(raw_params))
#define ASSERT_ARGS_string_arg_from_c_args __attribute__unused__ int _ASSERT_ARGS_CHECK = (\
       PARROT_ASSERT_ARG(interp) \
    , PARROT_ASSERT_ARG(args))
#define ASSERT_ARGS_string_arg_from_continuation __attribute__unused__ int _ASSERT_ARGS_CHECK = (\
       PARROT_ASSERT_ARG(interp) \
    , PARROT_ASSERT_ARG(cs))
#define ASSERT_ARGS_string_arg_from_op __attribute__unused__ int _ASSERT_ARGS_CHECK = (\
       PARROT_ASSERT_ARG(interp) \
    , PARROT_ASSERT_ARG(raw_args))
#define ASSERT_ARGS_string_constant_from_op __attribute__unused__ int _ASSERT_ARGS_CHECK = (\
       PARROT_ASSERT_ARG(interp) \
    , PARROT_ASSERT_ARG(raw_params))
#define ASSERT_ARGS_string_constant_from_varargs __attribute__unused__ int _ASSERT_ARGS_CHECK = (\
       PARROT_ASSERT_ARG(interp) \
    , PARROT_ASSERT_ARG(data))
#define ASSERT_ARGS_string_param_from_c_args __attribute__unused__ int _ASSERT_ARGS_CHECK = (\
       PARROT_ASSERT_ARG(interp) \
    , PARROT_ASSERT_ARG(args))
#define ASSERT_ARGS_string_param_from_op __attribute__unused__ int _ASSERT_ARGS_CHECK = (\
       PARROT_ASSERT_ARG(interp) \
    , PARROT_ASSERT_ARG(raw_params))
/* Don't modify between HEADERIZER BEGIN / HEADERIZER END.  Your changes will be lost. */
/* HEADERIZER END: static */

/*

=item C<PMC* Parrot_pcc_build_sig_object_from_op(PARROT_INTERP, PMC *signature,
PMC * const raw_sig, opcode_t * const raw_args)>

Take a raw signature and argument list from a set_args opcode and
convert it to a CallSignature PMC.

=cut

*/

PARROT_EXPORT
PARROT_WARN_UNUSED_RESULT
PARROT_CANNOT_RETURN_NULL
PMC*
Parrot_pcc_build_sig_object_from_op(PARROT_INTERP, ARGIN_NULLOK(PMC *signature),
        ARGIN(PMC * const raw_sig), ARGIN(opcode_t * const raw_args))
{
    ASSERT_ARGS(Parrot_pcc_build_sig_object_from_op)
    PMC            *call_object;
    INTVAL          arg_index;
    INTVAL          arg_count  = VTABLE_elements(interp, raw_sig);
    PMC            *ctx        = CURRENT_CONTEXT(interp);
    STRING         *string_sig = Parrot_str_new(interp, "", 0);

    if (PMC_IS_NULL(signature)) {
        call_object = pmc_new(interp, enum_class_CallSignature);
        gc_register_pmc(interp, call_object);
    }
    else
        call_object = signature;

    VTABLE_set_attr_str(interp, call_object, CONST_STRING(interp, "arg_flags"), raw_sig);

    for (arg_index = 0; arg_index < arg_count; arg_index++) {
        INTVAL arg_flags = VTABLE_get_integer_keyed_int(interp,
                    raw_sig, arg_index);

        const INTVAL constant  = PARROT_ARG_CONSTANT_ISSET(arg_flags);
        const INTVAL raw_index = raw_args[arg_index + 2];

        switch (PARROT_ARG_TYPE_MASK_MASK(arg_flags)) {
            case PARROT_ARG_INTVAL:
                string_sig = Parrot_str_append(interp, string_sig, CONST_STRING(interp, "I"));
                if (constant)
                    VTABLE_push_integer(interp, call_object, raw_index);
                else {
                    const INTVAL int_value = CTX_REG_INT(ctx, raw_index);
                    VTABLE_push_integer(interp, call_object, int_value);
                }
                break;
            case PARROT_ARG_FLOATVAL:
                string_sig = Parrot_str_append(interp, string_sig, CONST_STRING(interp, "N"));
                if (constant)
                    VTABLE_push_float(interp, call_object,
                            Parrot_pcc_get_num_constant(interp, ctx, raw_index));
                else {
                    const FLOATVAL float_value = CTX_REG_NUM(ctx, raw_index);
                    VTABLE_push_float(interp, call_object, float_value);
                }
                break;
            case PARROT_ARG_STRING:
            {
                STRING *string_value;
                string_sig = Parrot_str_append(interp, string_sig, CONST_STRING(interp, "S"));
                if (constant)
                    /* ensure that callees don't modify constant caller strings */
                    string_value = Parrot_str_new_COW(interp,
                            Parrot_pcc_get_string_constant(interp, ctx, raw_index));
                else
                    string_value = CTX_REG_STR(ctx, raw_index);

                if (arg_flags & PARROT_ARG_NAME) {
                    string_sig = Parrot_str_append(interp, string_sig, CONST_STRING(interp, "n"));
                    arg_index++;
                    if (!PMC_IS_NULL(call_object)
                         && VTABLE_exists_keyed_str(interp, call_object, string_value)) {
                        Parrot_ex_throw_from_c_args(interp, NULL, EXCEPTION_INVALID_OPERATION,
                                "duplicate named argument in call");

                    }
                    extract_named_arg_from_op(interp, call_object, string_value,
                            raw_sig, raw_args, arg_index);
                }
                else
                    VTABLE_push_string(interp, call_object, string_value);

                break;
            }
            case PARROT_ARG_PMC:
            {
                PMC *pmc_value;
                if (constant)
                    pmc_value = Parrot_pcc_get_pmc_constant(interp, ctx, raw_index);
                else
                    pmc_value = CTX_REG_PMC(ctx, raw_index);

                if (arg_flags & PARROT_ARG_FLATTEN) {
                    STRING * const flat_list = dissect_aggregate_arg(
                                                   interp, call_object, pmc_value);
                    string_sig = Parrot_str_append(interp, string_sig, flat_list);
                }
                else {
                    string_sig = Parrot_str_append(interp, string_sig, CONST_STRING(interp, "P"));
                    VTABLE_push_pmc(interp, call_object, clone_key_arg(interp, pmc_value));
                }

                break;
            }
            default:
                break;
        }

    }

    VTABLE_set_string_native(interp, call_object, string_sig);
    return call_object;
}

/*

=item C<static void extract_named_arg_from_op(PARROT_INTERP, PMC *call_object,
STRING *name, PMC * const raw_sig, opcode_t * const raw_args, INTVAL arg_index)>

Pulls in the next argument from a set_args opcode, and sets it as the
value of a named argument in the CallSignature PMC.

=cut

*/

static void
extract_named_arg_from_op(PARROT_INTERP, ARGMOD(PMC *call_object), ARGIN(STRING *name),
        ARGIN(PMC * const raw_sig), ARGIN(opcode_t * const raw_args),
        INTVAL arg_index)
{
    ASSERT_ARGS(extract_named_arg_from_op)
    PMC   *ctx       = CURRENT_CONTEXT(interp);
    INTVAL arg_flags = VTABLE_get_integer_keyed_int(interp,
                    raw_sig, arg_index);

    const INTVAL constant  = PARROT_ARG_CONSTANT_ISSET(arg_flags);
    const INTVAL raw_index = raw_args[arg_index + 2];
    STRING      *string_sig = VTABLE_get_string(interp, call_object);

    switch (PARROT_ARG_TYPE_MASK_MASK(arg_flags)) {
        case PARROT_ARG_INTVAL:
            string_sig = Parrot_str_append(interp, string_sig, CONST_STRING(interp, "I"));
            if (constant)
                VTABLE_set_integer_keyed_str(interp, call_object, name, raw_index);
            else
                VTABLE_set_integer_keyed_str(interp, call_object, name,
                        CTX_REG_INT(ctx, raw_index));
            break;
        case PARROT_ARG_FLOATVAL:
            string_sig = Parrot_str_append(interp, string_sig, CONST_STRING(interp, "N"));
            if (constant)
                VTABLE_set_number_keyed_str(interp, call_object, name,
                        Parrot_pcc_get_num_constant(interp, ctx, raw_index));
            else
                VTABLE_set_number_keyed_str(interp, call_object, name,
                        CTX_REG_NUM(ctx, raw_index));
            break;
        case PARROT_ARG_STRING:
            string_sig = Parrot_str_append(interp, string_sig, CONST_STRING(interp, "S"));
            if (constant)
                /* ensure that callees don't modify constant caller strings */
                VTABLE_set_string_keyed_str(interp, call_object, name,
                        Parrot_str_new_COW(interp,
                                Parrot_pcc_get_string_constant(interp, ctx, raw_index)));
            else
                VTABLE_set_string_keyed_str(interp, call_object, name,
                        CTX_REG_STR(ctx, raw_index));
            break;
        case PARROT_ARG_PMC:
            string_sig = Parrot_str_append(interp, string_sig, CONST_STRING(interp, "P"));
            if (constant)
                VTABLE_set_pmc_keyed_str(interp, call_object, name,
                        Parrot_pcc_get_pmc_constant(interp, ctx, raw_index));
            else
                VTABLE_set_pmc_keyed_str(interp, call_object, name,
                        CTX_REG_PMC(ctx, raw_index));
            break;
        default:
            break;
    }

    VTABLE_set_string_native(interp, call_object, string_sig);
}

/*

=item C<static STRING * dissect_aggregate_arg(PARROT_INTERP, PMC *call_object,
PMC *aggregate)>

Takes an aggregate PMC and splits it up into individual arguments,
adding each one to the CallSignature PMC. If the aggregate is an array,
its elements are added as positional arguments. If the aggregate is a
hash, its key/value pairs are added as named arguments.

=cut

*/

PARROT_CANNOT_RETURN_NULL
static STRING *
dissect_aggregate_arg(PARROT_INTERP, ARGMOD(PMC *call_object), ARGIN(PMC *aggregate))
{
    ASSERT_ARGS(dissect_aggregate_arg)
    STRING * sub_string = Parrot_str_new(interp, NULL, 0);
    if (VTABLE_does(interp, aggregate, CONST_STRING(interp, "array"))) {
        INTVAL elements = VTABLE_elements(interp, aggregate);
        INTVAL index;
        for (index = 0; index < elements; index++) {
            VTABLE_push_pmc(interp, call_object,
                    VTABLE_get_pmc_keyed_int(interp, aggregate, index));
            sub_string = Parrot_str_append(interp, sub_string, CONST_STRING(interp, "P"));
        }
    }
    else if (VTABLE_does(interp, aggregate, CONST_STRING(interp, "hash"))) {
        INTVAL elements = VTABLE_elements(interp, aggregate);
        INTVAL index;
        PMC *key = pmc_new(interp, enum_class_Key);
        VTABLE_set_integer_native(interp, key, 0);
        SETATTR_Key_next_key(interp, key, (PMC *)INITBucketIndex);

        /* Low-level hash iteration. */
        for (index = 0; index < elements; index++) {
            if (!PMC_IS_NULL(key)) {
                STRING *name = (STRING *)parrot_hash_get_idx(interp,
                                (Hash *)VTABLE_get_pointer(interp, aggregate), key);
                PARROT_ASSERT(name);
                VTABLE_set_pmc_keyed_str(interp, call_object, name,
                    VTABLE_get_pmc_keyed_str(interp, aggregate, name));
                sub_string = Parrot_str_append(interp, sub_string, CONST_STRING(interp, "SnP"));
            }
        }
    }
    else {
        Parrot_ex_throw_from_c_args(interp, NULL, EXCEPTION_INVALID_OPERATION,
                "flattened parameters must be a hash or array");
    }
    return sub_string;
}

/*

=item C<PMC* Parrot_pcc_build_sig_object_returns_from_op(PARROT_INTERP, PMC
*signature, PMC *raw_sig, opcode_t *raw_args)>

Take a raw signature and argument list from a set_results opcode and
convert it to a CallSignature PMC. Uses an existing CallSignature PMC if
one was already created for set_args. Otherwise, creates a new one.

=cut

*/

PARROT_EXPORT
PARROT_WARN_UNUSED_RESULT
PARROT_CAN_RETURN_NULL
PMC*
Parrot_pcc_build_sig_object_returns_from_op(PARROT_INTERP, ARGIN_NULLOK(PMC *signature),
        ARGIN(PMC *raw_sig), ARGIN(opcode_t *raw_args))
{
    ASSERT_ARGS(Parrot_pcc_build_sig_object_returns_from_op)
    PMC            *call_object;
    STRING         *string_sig;
    INTVAL          arg_index;
    INTVAL          arg_count = VTABLE_elements(interp, raw_sig);
    PMC            *ctx       = CURRENT_CONTEXT(interp);
    PMC            *returns   = pmc_new(interp, enum_class_ResizablePMCArray);

    if (PMC_IS_NULL(signature)) {
        call_object = pmc_new(interp, enum_class_CallSignature);
        gc_register_pmc(interp, call_object);
    }
    else
        call_object = signature;

    string_sig = VTABLE_get_string(interp, call_object);

    /* A hack to support 'get_results' as the way of fetching the
     * exception object inside an exception handler. The first argument
     * in the call object is the exception, stick it directly into the
     * destination register. */
    if (CALLSIGNATURE_is_exception_TEST(call_object)) {
        const INTVAL raw_index = raw_args[2];
        CTX_REG_PMC(ctx, raw_index) =
                VTABLE_get_pmc_keyed_int(interp, call_object, 0);
        return NULL;
    }

    VTABLE_set_attr_str(interp, call_object, CONST_STRING(interp, "return_flags"), raw_sig);
    VTABLE_set_attr_str(interp, call_object, CONST_STRING(interp, "returns"), returns);
    string_sig = Parrot_str_append(interp, string_sig, CONST_STRING(interp, "->"));

    for (arg_index = 0; arg_index < arg_count; arg_index++) {
        STRING * const signature = CONST_STRING(interp, "signature");
        INTVAL arg_flags = VTABLE_get_integer_keyed_int(interp,
                    raw_sig, arg_index);
        const INTVAL raw_index = raw_args[arg_index + 2];

        /* Returns store a pointer to the register, so they can pass
         * the result back to the caller. */
        PMC * const val_pointer = pmc_new(interp, enum_class_CPointer);

        switch (PARROT_ARG_TYPE_MASK_MASK(arg_flags)) {
            case PARROT_ARG_INTVAL:
                string_sig = Parrot_str_append(interp, string_sig, CONST_STRING(interp, "I"));
                VTABLE_set_pointer(interp, val_pointer, (void *) &(CTX_REG_INT(ctx, raw_index)));
                VTABLE_set_string_keyed_str(interp, val_pointer, signature, CONST_STRING(interp, "I"));
                VTABLE_push_pmc(interp, returns, val_pointer);
                break;
            case PARROT_ARG_FLOATVAL:
                string_sig = Parrot_str_append(interp, string_sig, CONST_STRING(interp, "N"));
                VTABLE_set_pointer(interp, val_pointer, (void *) &(CTX_REG_NUM(ctx, raw_index)));
                VTABLE_set_string_keyed_str(interp, val_pointer, signature, CONST_STRING(interp, "N"));
                VTABLE_push_pmc(interp, returns, val_pointer);
                break;
            case PARROT_ARG_STRING:
                if (arg_flags & PARROT_ARG_NAME) {
                    PMC *name_string = pmc_new(interp, enum_class_String);
                    STRING * string_val = arg_flags & PARROT_ARG_CONSTANT
                                          ? Parrot_pcc_get_string_constant(interp, ctx, raw_index)
                                          : CTX_REG_STR(ctx, raw_index);
                    VTABLE_set_string_native(interp, name_string, string_val);
                    VTABLE_push_pmc(interp, returns, name_string);
                }
                else {
                    string_sig = Parrot_str_append(interp, string_sig, CONST_STRING(interp, "S"));
                    VTABLE_set_pointer(interp, val_pointer,
                                       (void *) &(CTX_REG_STR(ctx, raw_index)));
                    VTABLE_set_string_keyed_str(interp, val_pointer, signature, CONST_STRING(interp, "S"));
                    VTABLE_push_pmc(interp, returns, val_pointer);
                }
                break;
            case PARROT_ARG_PMC:
                string_sig = Parrot_str_append(interp, string_sig, CONST_STRING(interp, "P"));
                VTABLE_set_pointer(interp, val_pointer, (void *) &(CTX_REG_PMC(ctx, raw_index)));
                VTABLE_set_string_keyed_str(interp, val_pointer, signature, CONST_STRING(interp, "P"));
                VTABLE_push_pmc(interp, returns, val_pointer);
                break;
            default:
                break;
        }

    }

    VTABLE_set_string_native(interp, call_object, string_sig);
    return call_object;
}

/*

=item C<PMC* Parrot_pcc_build_sig_object_from_varargs(PARROT_INTERP, PMC *obj,
const char *sig, va_list args)>

Converts a varargs list into a CallSignature PMC. The CallSignature stores the
original short signature string and an array of integer types to pass on to the
multiple dispatch search.

=cut

*/

PARROT_EXPORT
PARROT_WARN_UNUSED_RESULT
PARROT_CANNOT_RETURN_NULL
PMC*
Parrot_pcc_build_sig_object_from_varargs(PARROT_INTERP, ARGIN_NULLOK(PMC *obj),
        ARGIN(const char *sig), va_list args)
{
    ASSERT_ARGS(Parrot_pcc_build_sig_object_from_varargs)
    PMC         *type_tuple         = PMCNULL;
    PMC         *returns            = PMCNULL;
    PMC         *arg_flags     = PMCNULL;
    PMC         *return_flags  = PMCNULL;
    PMC         * const call_object = pmc_new(interp, enum_class_CallSignature);
    STRING      *string_sig         = Parrot_str_new_constant(interp, sig);
    const INTVAL sig_len            = Parrot_str_byte_length(interp, string_sig);
    INTVAL       in_return_sig      = 0;
    INTVAL       i;

    if (!sig_len)
        return call_object;

    VTABLE_set_string_native(interp, call_object, string_sig);
    parse_signature_string(interp, sig, &arg_flags, &return_flags);
    VTABLE_set_attr_str(interp, call_object, CONST_STRING(interp, "arg_flags"), arg_flags);
    VTABLE_set_attr_str(interp, call_object, CONST_STRING(interp, "return_flags"), return_flags);

    /* Process the varargs list */
    for (i = 0; i < sig_len; ++i) {
        const INTVAL type = Parrot_str_indexed(interp, string_sig, i);

        /* Only create the returns array if it's needed */
        if (in_return_sig && PMC_IS_NULL(returns)) {
            returns = pmc_new(interp, enum_class_ResizablePMCArray);
            VTABLE_set_attr_str(interp, call_object, CONST_STRING(interp, "returns"), returns);
        }

        if (in_return_sig) {
            STRING * const signature = CONST_STRING(interp, "signature");
            /* Returns store the original passed-in pointer so they can pass
             * the result back to the caller. */
            PMC * const val_pointer = pmc_new(interp, enum_class_CPointer);
            VTABLE_push_pmc(interp, returns, val_pointer);

            switch (type) {
                case 'I':
                    VTABLE_set_pointer(interp, val_pointer, (void *)va_arg(args, INTVAL *));
                    VTABLE_set_string_keyed_str(interp, val_pointer, signature, CONST_STRING(interp, "I"));
                    break;
                case 'N':
                    VTABLE_set_pointer(interp, val_pointer, (void *)va_arg(args, FLOATVAL *));
                    VTABLE_set_string_keyed_str(interp, val_pointer, signature, CONST_STRING(interp, "N"));
                    break;
                case 'S':
                    VTABLE_set_pointer(interp, val_pointer, (void *)va_arg(args, STRING **));
                    VTABLE_set_string_keyed_str(interp, val_pointer, signature, CONST_STRING(interp, "S"));
                    break;
                case 'P':
                    VTABLE_set_pointer(interp, val_pointer, (void *)va_arg(args, PMC **));
                    VTABLE_set_string_keyed_str(interp, val_pointer, signature, CONST_STRING(interp, "P"));
                    break;
                default:
                    Parrot_ex_throw_from_c_args(interp, NULL,
                        EXCEPTION_INVALID_OPERATION,
                        "Dispatch: invalid argument type %c!", type);
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
                    INTVAL type_lookahead = Parrot_str_indexed(interp, string_sig, (i + 1));
                    PMC * const pmc_arg = va_arg(args, PMC *);
                    VTABLE_push_pmc(interp, call_object, clone_key_arg(interp, pmc_arg));
                    if (type_lookahead == 'i') {
                        if (i != 0)
                            Parrot_ex_throw_from_c_args(interp, NULL,
                                EXCEPTION_INVALID_OPERATION,
                                "Dispatch: only the first argument can be an invocant");
                        i++; /* skip 'i' */
                    }
                    break;
                }
                case '-':
                    i++; /* skip '>' */
                    in_return_sig = 1;
                    break;
                default:
                    Parrot_ex_throw_from_c_args(interp, NULL,
                        EXCEPTION_INVALID_OPERATION,
                        "Dispatch: invalid argument type %c!", type);
            }
        }
    }

    /* Check if we have an invocant, and add it to the front of the arguments */
    if (!PMC_IS_NULL(obj)) {
        string_sig = Parrot_str_concat(interp, CONST_STRING(interp, "Pi"), string_sig, 0);
        VTABLE_set_string_native(interp, call_object, string_sig);
        VTABLE_unshift_pmc(interp, call_object, obj);
    }

    /* Build a type_tuple for multiple dispatch */
    type_tuple = Parrot_mmd_build_type_tuple_from_sig_obj(interp, call_object);
    VTABLE_set_pmc(interp, call_object, type_tuple);

    return call_object;
}

/*

=item C<static void fill_params(PARROT_INTERP, PMC *call_object, PMC *raw_sig,
void *arg_info, struct pcc_set_funcs *accessor)>

Gets args for the current function call and puts them into position.
First it gets the positional non-slurpy parameters, then the positional
slurpy parameters, then the named parameters, and finally the named
slurpy parameters.

=cut

*/

static void
fill_params(PARROT_INTERP, ARGMOD_NULLOK(PMC *call_object),
        ARGIN(PMC *raw_sig), ARGIN(void *arg_info), ARGIN(struct pcc_set_funcs *accessor))
{
    ASSERT_ARGS(fill_params)
    PMC    *named_used_list = PMCNULL;
    PMC    *arg_sig;
    INTVAL  param_count     = VTABLE_elements(interp, raw_sig);
    INTVAL  positional_args;
    INTVAL  param_index     = 0;
    INTVAL  arg_index       = 0;
    INTVAL  named_count     = 0;
    INTVAL  err_check       = 0;

    /* Check if we should be throwing errors. This is configured separately
     * for parameters and return values. */
    if (PARROT_ERRORS_test(interp, PARROT_ERRORS_PARAM_COUNT_FLAG))
            err_check = 1;

    /* A null call object is fine if there are no arguments and no returns. */
    if (PMC_IS_NULL(call_object)) {
        if (param_count > 0) {
            if (err_check)
                Parrot_ex_throw_from_c_args(interp, NULL, EXCEPTION_INVALID_OPERATION,
                        "too few arguments: 0 passed, %d expected",
                        param_count);
        }
        return;
    }

    positional_args = VTABLE_elements(interp, call_object);
    arg_sig   = VTABLE_get_attr_str(interp, call_object, CONST_STRING(interp, "arg_flags"));

    /* First iterate over positional args and positional parameters. */
    arg_index = 0;
    param_index = 0;
    while (1) {
        INTVAL param_flags;

        /* Check if we've used up all the parameters. */
        if (param_index >= param_count) {
            if (arg_index >= positional_args) {
                /* We've used up all the arguments and parameters, we're done. */
                break;
            }
            else if (err_check) {
                /* We've used up all the parameters, but have extra positional
                 * args left over. */
                Parrot_ex_throw_from_c_args(interp, NULL, EXCEPTION_INVALID_OPERATION,
                        "too many positional arguments: %d passed, %d expected",
                        positional_args, param_index);
            }
            return;
        }

        param_flags = VTABLE_get_integer_keyed_int(interp, raw_sig, param_index);

        /* If the parameter is slurpy, collect all remaining positional
         * arguments into an array.*/
        if (param_flags & PARROT_ARG_SLURPY_ARRAY) {
            PMC *collect_positional;

            /* Can't handle named slurpy here, go on to named argument handling. */
            if (param_flags & PARROT_ARG_NAME)
                break;

            if (named_count > 0)
                Parrot_ex_throw_from_c_args(interp, NULL,
                        EXCEPTION_INVALID_OPERATION,
                        "named parameters must follow all positional parameters");

            collect_positional = pmc_new(interp,
                    Parrot_get_ctx_HLL_type(interp, enum_class_ResizablePMCArray));
            for (; arg_index < positional_args; arg_index++) {
                VTABLE_push_pmc(interp, collect_positional,
                        VTABLE_get_pmc_keyed_int(interp, call_object, arg_index));
            }
            *accessor->pmc(interp, arg_info, param_index) = collect_positional;
            param_index++;
            break; /* Terminate the positional arg loop. */
        }

        /* We have a positional argument, fill the parameter with it. */
        if (arg_index < positional_args) {

            /* Fill a named parameter with a positional argument. */
            if (param_flags & PARROT_ARG_NAME) {
                STRING *param_name;
                if (!(param_flags & PARROT_ARG_STRING))
                    Parrot_ex_throw_from_c_args(interp, NULL, EXCEPTION_INVALID_OPERATION,
                            "named parameters must have a name specified");
                param_name = PARROT_ARG_CONSTANT_ISSET(param_flags)
                                   ? accessor->string_constant(interp, arg_info, param_index)
                                   : *accessor->string(interp, arg_info, param_index);
                named_count++;
                param_index++;
                if (param_index >= param_count)
                    continue;
                param_flags = VTABLE_get_integer_keyed_int(interp,
                            raw_sig, param_index);

                /* Mark the name as used, cannot be filled again. */
                if (PMC_IS_NULL(named_used_list)) /* Only created if needed. */
                    named_used_list = pmc_new(interp, enum_class_Hash);
                VTABLE_set_integer_keyed_str(interp, named_used_list, param_name, 1);
            }
            else if (named_count > 0) {
                Parrot_ex_throw_from_c_args(interp, NULL, EXCEPTION_INVALID_OPERATION,
                        "named parameters must follow all positional parameters");
            }

            /* Check for :lookahead parameter goes here. */

            /* Go ahead and fill the parameter with a positional argument. */
            switch (PARROT_ARG_TYPE_MASK_MASK(param_flags)) {
                case PARROT_ARG_INTVAL:
                    *accessor->intval(interp, arg_info, param_index) =
                            VTABLE_get_integer_keyed_int(interp, call_object, arg_index);
                    break;
                case PARROT_ARG_FLOATVAL:
                    *accessor->numval(interp, arg_info, param_index) =
                            VTABLE_get_number_keyed_int(interp, call_object, arg_index);
                    break;
                case PARROT_ARG_STRING:
                    *accessor->string(interp, arg_info, param_index) =
                            VTABLE_get_string_keyed_int(interp, call_object, arg_index);
                    break;
                case PARROT_ARG_PMC:
                    *accessor->pmc(interp, arg_info, param_index) =
                            VTABLE_get_pmc_keyed_int(interp, call_object, arg_index);
                    break;
                default:
                    Parrot_ex_throw_from_c_args(interp, NULL,
                            EXCEPTION_INVALID_OPERATION, "invalid parameter type");
                    break;
            }

            /* Mark the option flag for the filled parameter. */
            if (param_flags & PARROT_ARG_OPTIONAL) {
                INTVAL next_param_flags;

                if (param_index + 1 < param_count) {
                    next_param_flags = VTABLE_get_integer_keyed_int(interp,
                            raw_sig, param_index + 1);
                    if (next_param_flags & PARROT_ARG_OPT_FLAG) {
                        param_index++;
                        *accessor->intval(interp, arg_info, param_index) = 1;
                    }
                }
            }
        }
        /* We have no more positional arguments, fill the optional parameter
         * with a default value. */
        else if (param_flags & PARROT_ARG_OPTIONAL) {
            INTVAL next_param_flags;

            /* We don't handle optional named params here, handle them in the
             * next loop. */
            if (param_flags & PARROT_ARG_NAME)
                break;

            assign_default_param_value(interp, param_index, param_flags,
                    arg_info, accessor);

            /* Mark the option flag for the parameter to FALSE, it was filled
             * with a default value. */
            if (param_index + 1 < param_count) {
                next_param_flags = VTABLE_get_integer_keyed_int(interp,
                        raw_sig, param_index + 1);
                if (next_param_flags & PARROT_ARG_OPT_FLAG) {
                    param_index++;
                    *accessor->intval(interp, arg_info, param_index) = 0;
                }
            }
        }
        /* We don't have an argument for the parameter, and it's not optional,
         * so it's an error. */
        else {
            /* We don't handle named params here, go to the next loop. */
            if (param_flags & PARROT_ARG_NAME)
                break;

            if (err_check)
                Parrot_ex_throw_from_c_args(interp, NULL, EXCEPTION_INVALID_OPERATION,
                        "too few positional arguments: %d passed, %d (or more) expected",
                        positional_args, param_index + 1);
        }

        /* Go on to next argument and parameter. */
        arg_index++;
        param_index++;
    }

    /* Now iterate over the named arguments and parameters. */
    while (1) {
        STRING *param_name    = NULL;
        INTVAL param_flags;

        /* Check if we've used up all the parameters. We'll check for leftover
         * named args after the loop. */
        if (param_index >= param_count)
            break;

        param_flags = VTABLE_get_integer_keyed_int(interp, raw_sig, param_index);

        /* All remaining parameters must be named. */
        if (!(param_flags & PARROT_ARG_NAME)) {
            Parrot_ex_throw_from_c_args(interp, NULL, EXCEPTION_INVALID_OPERATION,
                    "named parameters must follow all positional parameters");
        }

        if (arg_index < positional_args) {
            /* We've used up all the positional parameters, but have extra
             * positional args left over. */
            if (VTABLE_get_integer_keyed_int(interp, arg_sig, arg_index) & PARROT_ARG_NAME) {
                Parrot_ex_throw_from_c_args(interp, NULL, EXCEPTION_INVALID_OPERATION,
                        "named arguments must follow all positional arguments");
            }
            Parrot_ex_throw_from_c_args(interp, NULL, EXCEPTION_INVALID_OPERATION,
                    "too many positional arguments: %d passed, %d expected",
                    positional_args, param_index);
        }

        /* Collected ("slurpy") named parameter */
        if (param_flags & PARROT_ARG_SLURPY_ARRAY) {
            PMC * const collect_named = pmc_new(interp,
                    Parrot_get_ctx_HLL_type(interp, enum_class_Hash));
            PMC *named_arg_list = VTABLE_get_attr_str(interp, call_object, CONST_STRING(interp, "named"));
            if (!PMC_IS_NULL(named_arg_list)) {
                INTVAL named_arg_count = VTABLE_elements(interp, named_arg_list);
                INTVAL named_arg_index;
                PMC *named_key = pmc_new(interp, enum_class_Key);
                VTABLE_set_integer_native(interp, named_key, 0);
                SETATTR_Key_next_key(interp, named_key, (PMC *)INITBucketIndex);

                /* Low-level hash iteration. */
                for (named_arg_index = 0; named_arg_index < named_arg_count; named_arg_index++) {
                    if (!PMC_IS_NULL(named_key)) {
                        STRING *name = (STRING *)parrot_hash_get_idx(interp,
                                    (Hash *)VTABLE_get_pointer(interp, named_arg_list), named_key);
                        PARROT_ASSERT(name);
                        if ((PMC_IS_NULL(named_used_list)) ||
                                !VTABLE_exists_keyed_str(interp, named_used_list, name)) {
                            VTABLE_set_pmc_keyed_str(interp, collect_named, name,
                                    VTABLE_get_pmc_keyed_str(interp, call_object, name));
                            /* Mark the name as used, cannot be filled again. */
                            if (PMC_IS_NULL(named_used_list)) /* Only created if needed. */
                                named_used_list = pmc_new(interp, enum_class_Hash);
                            VTABLE_set_integer_keyed_str(interp, named_used_list, name, 1);
                            named_count++;
                        }
                    }
                }
            }

            *accessor->pmc(interp, arg_info, param_index) = collect_named;
            break; /* End of named parameters. */
        }

        /* Store the name. */
       if (!(param_flags & PARROT_ARG_STRING))
            Parrot_ex_throw_from_c_args(interp, NULL, EXCEPTION_INVALID_OPERATION,
                    "named parameters must have a name specified");
        param_name = PARROT_ARG_CONSTANT_ISSET(param_flags)
                               ? accessor->string_constant(interp, arg_info, param_index)
                               : *accessor->string(interp, arg_info, param_index);

        if (!STRING_IS_NULL(param_name)) {
            /* The next parameter is the actual value. */
            param_index++;
            if (param_index >= param_count)
                continue;
            param_flags = VTABLE_get_integer_keyed_int(interp, raw_sig, param_index);

            if (VTABLE_exists_keyed_str(interp, call_object, param_name)) {

                /* Mark the name as used, cannot be filled again. */
                if (PMC_IS_NULL(named_used_list)) /* Only created if needed. */
                    named_used_list = pmc_new(interp, enum_class_Hash);
                VTABLE_set_integer_keyed_str(interp, named_used_list, param_name, 1);
                named_count++;

                /* Fill the named parameter. */
                switch (PARROT_ARG_TYPE_MASK_MASK(param_flags)) {
                    case PARROT_ARG_INTVAL:
                        *accessor->intval(interp, arg_info, param_index) =
                                VTABLE_get_integer_keyed_str(interp, call_object, param_name);
                        break;
                    case PARROT_ARG_FLOATVAL:
                        *accessor->numval(interp, arg_info, param_index) =
                                VTABLE_get_number_keyed_str(interp, call_object, param_name);
                        break;
                    case PARROT_ARG_STRING:
                        *accessor->string(interp, arg_info, param_index) =
                                VTABLE_get_string_keyed_str(interp, call_object, param_name);
                        break;
                    case PARROT_ARG_PMC:
                        *accessor->pmc(interp, arg_info, param_index) =
                                VTABLE_get_pmc_keyed_str(interp, call_object, param_name);
                        break;
                    default:
                        Parrot_ex_throw_from_c_args(interp, NULL,
                                EXCEPTION_INVALID_OPERATION, "invalid parameter type");
                        break;
                }

                /* Mark the option flag for the filled parameter. */
                if (param_flags & PARROT_ARG_OPTIONAL) {
                    INTVAL next_param_flags;

                    if (param_index + 1 < param_count) {
                        next_param_flags = VTABLE_get_integer_keyed_int(interp,
                                raw_sig, param_index + 1);
                        if (next_param_flags & PARROT_ARG_OPT_FLAG) {
                            param_index++;
                            *accessor->intval(interp, arg_info, param_index) = 1;
                        }
                    }
                }
            }
            else if (param_flags & PARROT_ARG_OPTIONAL) {
                INTVAL next_param_flags;

                assign_default_param_value(interp, param_index, param_flags,
                        arg_info, accessor);

                /* Mark the option flag for the parameter to FALSE, it was filled
                 * with a default value. */
                if (param_index + 1 < param_count) {
                    next_param_flags = VTABLE_get_integer_keyed_int(interp,
                            raw_sig, param_index + 1);
                    if (next_param_flags & PARROT_ARG_OPT_FLAG) {
                        param_index++;
                        *accessor->intval(interp, arg_info, param_index) = 0;
                    }
                }
            }
            /* We don't have an argument for the parameter, and it's not optional,
             * so it's an error. */
            else {
                if (err_check)
                    Parrot_ex_throw_from_c_args(interp, NULL, EXCEPTION_INVALID_OPERATION,
                            "too few named arguments: no argument for required parameter '%S'",
                            param_name);
            }
        }

        param_index++;
    }

    /* Double check that all named arguments were assigned to parameters. */
    if (err_check) {
        PMC *named_arg_list = VTABLE_get_attr_str(interp, call_object, CONST_STRING(interp, "named"));
        if (!PMC_IS_NULL(named_arg_list)) {
            INTVAL named_arg_count = VTABLE_elements(interp, named_arg_list);
            if (PMC_IS_NULL(named_used_list))
                return;
                /* The 'return' above is a temporary hack to duplicate an old bug,
                 * and will be replaced by the exception below at the next
                 * deprecation point, see TT #1103

                Parrot_ex_throw_from_c_args(interp, NULL, EXCEPTION_INVALID_OPERATION,
                        "too many named arguments: %d passed, 0 used",
                        named_arg_count);
                 */
            if (named_arg_count > named_count) {
                /* At this point we know we have named arguments that weren't
                 * assigned to parameters. We're going to throw an exception
                 * anyway, so spend a little extra effort to tell the user *which*
                 * named argument is extra. */
                INTVAL named_arg_index;
                PMC *named_key = pmc_new(interp, enum_class_Key);
                VTABLE_set_integer_native(interp, named_key, 0);
                SETATTR_Key_next_key(interp, named_key, (PMC *)INITBucketIndex);

                /* Low-level hash iteration. */
                for (named_arg_index = 0; named_arg_index < named_arg_count; named_arg_index++) {
                    if (!PMC_IS_NULL(named_key)) {
                        STRING *name = (STRING *)parrot_hash_get_idx(interp,
                                    (Hash *)VTABLE_get_pointer(interp, named_arg_list), named_key);
                        PARROT_ASSERT(name);
                        if (!VTABLE_exists_keyed_str(interp, named_used_list, name)) {
                            Parrot_ex_throw_from_c_args(interp, NULL,
                                    EXCEPTION_INVALID_OPERATION,
                                    "too many named arguments: '%S' not used",
                                    name);
                        }
                    }
                }
            }
        }
    }
}

/*

=item C<static void assign_default_param_value(PARROT_INTERP, INTVAL
param_index, INTVAL param_flags, void *arg_info, struct pcc_set_funcs
*accessor)>

Assign an appropriate default value to the parameter depending on its type

=cut

*/

static void
assign_default_param_value(PARROT_INTERP, INTVAL param_index, INTVAL param_flags,
        ARGIN(void *arg_info), ARGIN(struct pcc_set_funcs *accessor))
{
    ASSERT_ARGS(assign_default_param_value)
    switch (PARROT_ARG_TYPE_MASK_MASK(param_flags)) {
        case PARROT_ARG_INTVAL:
            *accessor->intval(interp, arg_info, param_index) = 0;
            break;
        case PARROT_ARG_FLOATVAL:
            *accessor->numval(interp, arg_info, param_index) = 0.0;
            break;
        case PARROT_ARG_STRING:
            *accessor->string(interp, arg_info, param_index) = NULL;
            break;
        case PARROT_ARG_PMC:
            *accessor->pmc(interp, arg_info, param_index) = PMCNULL;
            break;
        default:
            Parrot_ex_throw_from_c_args(interp, NULL,
                    EXCEPTION_INVALID_OPERATION, "invalid parameter type");
            break;
    }
}

/*

=item C<static void assign_default_result_value(PARROT_INTERP, PMC *result,
INTVAL result_flags)>

Assign an appropriate default value to the result depending on its type

=cut

*/

static void
assign_default_result_value(PARROT_INTERP, ARGMOD(PMC *result), INTVAL result_flags)
{
    ASSERT_ARGS(assign_default_result_value)
    switch (PARROT_ARG_TYPE_MASK_MASK(result_flags)) {
        case PARROT_ARG_INTVAL:
            VTABLE_set_integer_native(interp, result, 0);
            break;
        case PARROT_ARG_FLOATVAL:
            VTABLE_set_number_native(interp, result, 0.0);
            break;
        case PARROT_ARG_STRING:
            VTABLE_set_string_native(interp, result, NULL);
            break;
        case PARROT_ARG_PMC:
            VTABLE_set_pmc(interp, result, PMCNULL);
            break;
        default:
            Parrot_ex_throw_from_c_args(interp, NULL,
                    EXCEPTION_INVALID_OPERATION, "invalid parameter type");
            break;
    }
}

/*

=item C<void Parrot_pcc_fill_params_from_op(PARROT_INTERP, PMC *call_object, PMC
*raw_sig, opcode_t *raw_params)>

Gets args for the current function call and puts them into position.
First it gets the positional non-slurpy parameters, then the positional
slurpy parameters, then the named parameters, and finally the named
slurpy parameters.

=cut

*/

PARROT_EXPORT
void
Parrot_pcc_fill_params_from_op(PARROT_INTERP, ARGMOD_NULLOK(PMC *call_object),
        ARGIN(PMC *raw_sig), ARGIN(opcode_t *raw_params))
{
    ASSERT_ARGS(Parrot_pcc_fill_params_from_op)

    static pcc_set_funcs function_pointers = {
        (intval_ptr_func_t)intval_param_from_op,
        (numval_ptr_func_t)numval_param_from_op,
        (string_ptr_func_t)string_param_from_op,
        (pmc_ptr_func_t)pmc_param_from_op,

        (intval_func_t)intval_constant_from_op,
        (numval_func_t)numval_constant_from_op,
        (string_func_t)string_constant_from_op,
        (pmc_func_t)pmc_constant_from_op,
    };

    fill_params(interp, call_object, raw_sig, raw_params, &function_pointers);
}

/*

=item C<void Parrot_pcc_fill_params_from_c_args(PARROT_INTERP, PMC *call_object,
const char *signature, ...)>

Gets args for the current function call and puts them into position.
First it gets the positional non-slurpy parameters, then the positional
slurpy parameters, then the named parameters, and finally the named
slurpy parameters.

The signature is a string in the format used for
C<Parrot_pcc_invoke_from_sig_object>, but with no return arguments. The
parameters are passed in as a list of references to the destination
variables.

=cut

*/

PARROT_EXPORT
void
Parrot_pcc_fill_params_from_c_args(PARROT_INTERP, ARGMOD(PMC *call_object),
        ARGIN(const char *signature), ...)
{
    ASSERT_ARGS(Parrot_pcc_fill_params_from_c_args)
    va_list args;
    PMC    *raw_sig  = PMCNULL;
    PMC    *invalid_sig = PMCNULL;
    static pcc_set_funcs function_pointers = {
        (intval_ptr_func_t)intval_param_from_c_args,
        (numval_ptr_func_t)numval_param_from_c_args,
        (string_ptr_func_t)string_param_from_c_args,
        (pmc_ptr_func_t)pmc_param_from_c_args,

        (intval_func_t)intval_constant_from_varargs,
        (numval_func_t)numval_constant_from_varargs,
        (string_func_t)string_constant_from_varargs,
        (pmc_func_t)pmc_constant_from_varargs,
    };

    parse_signature_string(interp, signature, &raw_sig, &invalid_sig);
    if (!PMC_IS_NULL(invalid_sig))
        Parrot_ex_throw_from_c_args(interp, NULL, EXCEPTION_INVALID_OPERATION,
                "returns should not be included in the parameter list");

    va_start(args, signature);
    fill_params(interp, call_object, raw_sig, &args, &function_pointers);
    va_end(args);
}

/*

=item C<static void fill_results(PARROT_INTERP, PMC *call_object, PMC *raw_sig,
void *return_info, struct pcc_get_funcs *accessor)>

Gets return values for the current return and puts them into position.
First it gets the positional non-slurpy returns, then the positional
slurpy returns, then the named returns, and finally the named
slurpy returns.

=cut

*/

static void
fill_results(PARROT_INTERP, ARGMOD_NULLOK(PMC *call_object),
        ARGIN(PMC *raw_sig), ARGIN(void *return_info), ARGIN(struct pcc_get_funcs *accessor))
{
    ASSERT_ARGS(fill_results)
    PMC    *ctx = CURRENT_CONTEXT(interp);
    PMC    *named_used_list = PMCNULL;
    PMC    *named_return_list = PMCNULL;
    INTVAL  return_count    = VTABLE_elements(interp, raw_sig);
    INTVAL  result_count;
    INTVAL  positional_returns = 0; /* initialized by a loop later */
    INTVAL  i = 0;                  /* used by the initialization loop */
    INTVAL  return_index    = 0;
    INTVAL  return_subindex = 0;
    INTVAL  result_index    = 0;
    INTVAL  positional_index = 0;
    INTVAL  named_count    = 0;
    INTVAL  err_check      = 0;
    PMC    *result_list;
    PMC    *result_sig;

    /* Check if we should be throwing errors. This is configured separately
     * for parameters and return values. */
    if (PARROT_ERRORS_test(interp, PARROT_ERRORS_RESULT_COUNT_FLAG))
            err_check = 1;

    /* A null call object is fine if there are no returns and no results. */
    if (PMC_IS_NULL(call_object)) { /* No results to be filled. */
        /* If the return_count is 0, then there are no return values waiting to
         * fill the results, so no error. */
        if (return_count > 0) {
            if (err_check)
                Parrot_ex_throw_from_c_args(interp, NULL, EXCEPTION_INVALID_OPERATION,
                        "too few returns: 0 passed, %d expected",
                        return_count);
        }
        return;
    }

    result_list = VTABLE_get_attr_str(interp, call_object, CONST_STRING(interp, "returns"));
    result_sig   = VTABLE_get_attr_str(interp, call_object, CONST_STRING(interp, "return_flags"));
    result_count = PMC_IS_NULL(result_list) ? 0 : VTABLE_elements(interp, result_list);
    PARROT_ASSERT(PMC_IS_NULL(result_list) || !PMC_IS_NULL(result_sig));

    /* the call obj doesn't have the returns as positionals, so instead we loop
     * over raw_sig and count the number of returns before the first named return.
     */
    for (i = 0; i < return_count; i++) {
        INTVAL flags = VTABLE_get_integer_keyed_int(interp, raw_sig, i);
        if (flags & PARROT_ARG_NAME)
            break;

        positional_returns++;
    }

    /*
    Parrot_io_eprintf(interp,
     "return_count: %d\nresult_count: %d\npositional_returns: %d\nraw_sig: %S\nresult_sig: %S\n",
         return_count, result_count, positional_returns, VTABLE_get_repr(interp, raw_sig),
         VTABLE_get_repr(interp, result_sig));
    */

    while (1) {
        INTVAL result_flags;
        PMC *result_item;

        /* Check if we've used up all the results. */
        if (result_index >= result_count) {
            if (return_index >= return_count) {
                /* We've used up all returns and results, we're
                 * done with the whole process. */
                return;
            }
            else if (err_check) {
                /* We've used up all the results, but have extra positional
                 * returns left over. */
                Parrot_ex_throw_from_c_args(interp, NULL, EXCEPTION_INVALID_OPERATION,
                        "too many positional returns: %d passed, %d expected",
                        return_index, result_count);
            }
            return;
        }

        result_flags = VTABLE_get_integer_keyed_int(interp, result_sig, result_index);
        result_item  = VTABLE_get_pmc_keyed_int(interp, result_list, result_index);

        /* If the result is slurpy, collect all remaining positional
         * returns into an array.*/
        if (result_flags & PARROT_ARG_SLURPY_ARRAY) {
            PMC *collect_positional;

            /* Can't handle named slurpy here, go on to named return handling. */
            if (result_flags & PARROT_ARG_NAME)
                break;

            if (named_count > 0)
                Parrot_ex_throw_from_c_args(interp, NULL,
                        EXCEPTION_INVALID_OPERATION,
                        "named results must follow all positional results");

            collect_positional = pmc_new(interp,
                    Parrot_get_ctx_HLL_type(interp, enum_class_ResizablePMCArray));

            /* Iterate over all positional returns in the list. */
            while (1) {
                INTVAL constant;
                INTVAL return_flags;
                if (return_index >= return_count)
                    break; /* no more returns */

                return_flags = VTABLE_get_integer_keyed_int(interp, raw_sig, return_index);

                if (return_flags & PARROT_ARG_NAME)
                    break; /* stop at named returns */

                constant = PARROT_ARG_CONSTANT_ISSET(return_flags);
                switch (PARROT_ARG_TYPE_MASK_MASK(return_flags)) {
                    case PARROT_ARG_INTVAL:
                        VTABLE_push_integer(interp, collect_positional, constant?
                                accessor->intval_constant(interp, return_info, return_index)
                                :accessor->intval(interp, return_info, return_index));
                        break;
                    case PARROT_ARG_FLOATVAL:
                        VTABLE_push_float(interp, collect_positional, constant?
                                accessor->numval_constant(interp, return_info, return_index)
                                :accessor->numval(interp, return_info, return_index));
                        break;
                    case PARROT_ARG_STRING:
                        VTABLE_push_string(interp, collect_positional, constant?
                                accessor->string_constant(interp, return_info, return_index)
                                :accessor->string(interp, return_info, return_index));
                        break;
                    case PARROT_ARG_PMC:
                        if (return_flags & PARROT_ARG_FLATTEN) {
                            Parrot_ex_throw_from_c_args(interp, NULL,
                                    EXCEPTION_INVALID_OPERATION,
                                    "We don't handle :flat returns into a :slurpy result yet\n");
                        }
                        VTABLE_push_pmc(interp, collect_positional, constant?
                                accessor->pmc_constant(interp, return_info, return_index)
                                :accessor->pmc(interp, return_info, return_index));
                        break;
                    default:
                        Parrot_ex_throw_from_c_args(interp, NULL,
                                EXCEPTION_INVALID_OPERATION, "invalid return type");
                        break;
                }
                return_index++;
            }
            VTABLE_set_pmc(interp, result_item, collect_positional);
            result_index++;
            break; /* Terminate the positional return loop. */
        }

        /* We have a positional return, fill the result with it. */
        if (return_index < positional_returns) {
            INTVAL return_flags = VTABLE_get_integer_keyed_int(interp, raw_sig, return_index);
            INTVAL constant     = PARROT_ARG_CONSTANT_ISSET(return_flags);

            /* Fill a named result with a positional return. */
            if (result_flags & PARROT_ARG_NAME) {
                STRING *result_name;
                if (!(result_flags & PARROT_ARG_STRING))
                    Parrot_ex_throw_from_c_args(interp, NULL, EXCEPTION_INVALID_OPERATION,
                            "named results must have a name specified");
                result_name = VTABLE_get_string(interp, result_item);
                named_count++;
                result_index++;
                if (result_index >= result_count)
                    continue;
                result_flags = VTABLE_get_integer_keyed_int(interp, result_sig, result_index);
                result_item  = VTABLE_get_pmc_keyed_int(interp, result_list, result_index);

                /* Mark the name as used, cannot be filled again. */
                if (PMC_IS_NULL(named_used_list)) /* Only created if needed. */
                    named_used_list = pmc_new(interp,
                            Parrot_get_ctx_HLL_type(interp, enum_class_Hash));
                VTABLE_set_integer_keyed_str(interp, named_used_list, result_name, 1);
            }
            else if (named_count > 0) {
                Parrot_ex_throw_from_c_args(interp, NULL, EXCEPTION_INVALID_OPERATION,
                        "named results must follow all positional results");
            }

            /* Check for :lookahead result goes here. */

            /* Go ahead and fill the result with a positional return. */
            switch (PARROT_ARG_TYPE_MASK_MASK(return_flags)) {
                case PARROT_ARG_INTVAL:
                    if (constant)
                        VTABLE_set_integer_native(interp, result_item,
                            accessor->intval_constant(interp, return_info, return_index));
                    else
                        VTABLE_set_integer_native(interp, result_item,
                            accessor->intval(interp, return_info, return_index));
                    break;
                case PARROT_ARG_FLOATVAL:
                    if (constant)
                        VTABLE_set_number_native(interp, result_item,
                            accessor->numval_constant(interp, return_info, return_index));
                    else
                        VTABLE_set_number_native(interp, result_item,
                            accessor->numval(interp, return_info, return_index));
                    break;
                case PARROT_ARG_STRING:
                    if (constant)
                        VTABLE_set_string_native(interp, result_item,
                            accessor->string_constant(interp, return_info, return_index));
                    else
                        VTABLE_set_string_native(interp, result_item,
                            accessor->string(interp, return_info, return_index));
                    break;
                case PARROT_ARG_PMC:
                    {
                        PMC *return_item = (constant)
                                         ? accessor->pmc_constant(interp, return_info, return_index)
                                         : accessor->pmc(interp, return_info, return_index);
                        if (return_flags & PARROT_ARG_FLATTEN) {
                            INTVAL flat_elems;
                            if (!VTABLE_does(interp, return_item, CONST_STRING(interp, "array"))) {
                                Parrot_ex_throw_from_c_args(interp, NULL,
                                                            EXCEPTION_INVALID_OPERATION,
                                                            "flattened return on a non-array");
                            }
                            flat_elems = VTABLE_elements(interp, return_item);
                            if (return_subindex < flat_elems) {
                                /* fetch an item out of the aggregate */
                                return_item = VTABLE_get_pmc_keyed_int(interp, return_item,
                                                                       return_subindex);
                                return_subindex++;
                            }
                            if (return_subindex >= flat_elems) {
                                return_subindex = 0; /* reset */
                            }
                            else {
                                return_index--; /* we want to stay on the same item */
                            }
                        }
                        VTABLE_set_pmc(interp, result_item, return_item);
                        break;
                    }
                default:
                    Parrot_ex_throw_from_c_args(interp, NULL,
                            EXCEPTION_INVALID_OPERATION, "invalid return type");
                    break;
            }

            /* Mark the option flag for the filled result. */
            if (result_flags & PARROT_ARG_OPTIONAL) {
                INTVAL next_result_flags;

                if (result_index + 1 < result_count) {
                    next_result_flags = VTABLE_get_integer_keyed_int(interp,
                            result_sig, result_index + 1);
                    if (next_result_flags & PARROT_ARG_OPT_FLAG) {
                        result_index++;
                        result_item = VTABLE_get_pmc_keyed_int(interp, result_list,
                                result_index);
                        VTABLE_set_integer_native(interp, result_item, 1);
                    }
                }
            }
        }
        /* We have no more positional returns, fill the optional result
         * with a default value. */
        else if (result_flags & PARROT_ARG_OPTIONAL) {
            INTVAL next_result_flags;

            /* We don't handle optional named results here, handle them in the
             * next loop. */
            if (result_flags & PARROT_ARG_NAME)
                break;

            assign_default_result_value(interp, result_item, result_flags);

            /* Mark the option flag for the result to FALSE, it was filled
             * with a default value. */
            if (result_index + 1 < result_count) {
                next_result_flags = VTABLE_get_integer_keyed_int(interp,
                        result_sig, result_index + 1);
                if (next_result_flags & PARROT_ARG_OPT_FLAG) {
                    result_index++;
                    result_item = VTABLE_get_pmc_keyed_int(interp, result_list,
                            result_index);
                    VTABLE_set_integer_native(interp, result_item, 0);
                }
            }
        }
        /* We don't have an return for the result, and it's not optional,
         * so it's an error. */
        else {
            /* We don't handle named results here, go to the next loop. */
            if (result_flags & PARROT_ARG_NAME)
                break;

            if (err_check)
                Parrot_ex_throw_from_c_args(interp, NULL, EXCEPTION_INVALID_OPERATION,
                        "too few positional returns: %d passed, %d (or more) expected",
                        positional_returns, result_index + 1);
        }

        /* Go on to next return and result. */
        return_index++;
        result_index++;
    }

    for (; return_index < return_count; return_index++) {
        STRING *return_name;
        INTVAL  return_flags;
        INTVAL  constant;

        return_flags = VTABLE_get_integer_keyed_int(interp, raw_sig, return_index);

        /* All remaining returns must be named. */
        if (!(return_flags & PARROT_ARG_NAME))
            Parrot_ex_throw_from_c_args(interp, NULL, EXCEPTION_INVALID_OPERATION,
                    "named returns must follow all positional returns");

        if (!(return_flags & PARROT_ARG_STRING))
            Parrot_ex_throw_from_c_args(interp, NULL, EXCEPTION_INVALID_OPERATION,
                    "named results must have a name specified");

        return_name = PARROT_ARG_CONSTANT_ISSET(return_flags)
                           ? accessor->string_constant(interp, return_info, return_index)
                           : accessor->string(interp, return_info, return_index);
        named_count++;
        return_index++;
        if (result_index >= result_count)
            Parrot_ex_throw_from_c_args(interp, NULL, EXCEPTION_INVALID_OPERATION,
                    "named returns must have a value");

        return_flags = VTABLE_get_integer_keyed_int(interp, raw_sig, return_index);

        if (PMC_IS_NULL(named_return_list)) /* Only created if needed. */
            named_return_list = pmc_new(interp,
                    Parrot_get_ctx_HLL_type(interp, enum_class_Hash));

        if (VTABLE_exists_keyed_str(interp, named_return_list, return_name))
            continue;

        constant = PARROT_ARG_CONSTANT_ISSET(return_flags);
        switch (PARROT_ARG_TYPE_MASK_MASK(return_flags)) {
            case PARROT_ARG_INTVAL:
                VTABLE_set_integer_keyed_str(interp, named_return_list, return_name,
                        constant
                        ? accessor->intval_constant(interp, return_info, return_index)
                        : accessor->intval(interp, return_info, return_index));
                break;
            case PARROT_ARG_FLOATVAL:
                VTABLE_set_number_keyed_str(interp, named_return_list, return_name,
                        constant
                        ? accessor->numval_constant(interp, return_info, return_index)
                        : accessor->numval(interp, return_info, return_index));
                break;
            case PARROT_ARG_STRING:
                VTABLE_set_string_keyed_str(interp, named_return_list, return_name,
                        constant
                        ? accessor->string_constant(interp, return_info, return_index)
                        : accessor->string(interp, return_info, return_index));
                break;
            case PARROT_ARG_PMC:
                if (0) {
                    PMC *return_item = (constant)
                                     ? accessor->pmc_constant(interp, return_info, return_index)
                                     : accessor->pmc(interp, return_info, return_index);
                    if (return_flags & PARROT_ARG_FLATTEN) {
                        Parrot_ex_throw_from_c_args(interp, NULL, EXCEPTION_INVALID_OPERATION,
                                                    "named flattened returns not yet implemented");
                    }
                    VTABLE_set_pmc_keyed_str(interp, named_return_list, return_name, return_item);
                    break;
                }
            default:
                Parrot_ex_throw_from_c_args(interp, NULL,
                        EXCEPTION_INVALID_OPERATION, "invalid return type");
                break;
        }
    }

    /* Now iterate over the named results, filling them from the
     * temporary hash of named returns. */
    while (1) {
        STRING *result_name    = NULL;
        PMC *result_item;
        INTVAL result_flags;

        /* Check if we've used up all the results. We'll check for leftover
         * named returns after the loop. */
        if (result_index >= result_count)
            break;

        result_flags = VTABLE_get_integer_keyed_int(interp, result_sig, result_index);
        result_item  = VTABLE_get_pmc_keyed_int(interp, result_list, result_index);

        /* All remaining results must be named. */
        if (!(result_flags & PARROT_ARG_NAME))
            Parrot_ex_throw_from_c_args(interp, NULL, EXCEPTION_INVALID_OPERATION,
                    "named results must follow all positional results");

        /* Collected ("slurpy") named result */
        if (result_flags & PARROT_ARG_SLURPY_ARRAY) {
            if (PMC_IS_NULL(named_return_list))
                named_return_list = pmc_new(interp,
                        Parrot_get_ctx_HLL_type(interp, enum_class_Hash));

            VTABLE_set_pmc(interp, result_item, named_return_list);
            break; /* End of named results. */
        }

        /* Store the name. */
        if (!(result_flags & PARROT_ARG_STRING))
            Parrot_ex_throw_from_c_args(interp, NULL, EXCEPTION_INVALID_OPERATION,
                    "named results must have a name specified");
        result_name = VTABLE_get_string(interp, result_item);

        if (!STRING_IS_NULL(result_name)) {
            /* The next result is the actual value. */
            result_index++;
            if (result_index >= result_count)
                continue;
            result_flags = VTABLE_get_integer_keyed_int(interp, result_sig, result_index);
            result_item  = VTABLE_get_pmc_keyed_int(interp, result_list, result_index);

            if (VTABLE_exists_keyed_str(interp, named_return_list, result_name)) {

                named_count++;

                /* Fill the named result. */
                switch (PARROT_ARG_TYPE_MASK_MASK(result_flags)) {
                    case PARROT_ARG_INTVAL:
                        VTABLE_set_integer_native(interp, result_item,
                            VTABLE_get_integer_keyed_str(interp, named_return_list, result_name));
                        break;
                    case PARROT_ARG_FLOATVAL:
                        VTABLE_set_number_native(interp, result_item,
                            VTABLE_get_number_keyed_str(interp, named_return_list, result_name));
                        break;
                    case PARROT_ARG_STRING:
                        VTABLE_set_string_native(interp, result_item,
                            VTABLE_get_string_keyed_str(interp, named_return_list, result_name));
                        break;
                    case PARROT_ARG_PMC:
                        VTABLE_set_pmc(interp, result_item,
                            VTABLE_get_pmc_keyed_str(interp, named_return_list, result_name));
                        break;
                    default:
                        Parrot_ex_throw_from_c_args(interp, NULL,
                                EXCEPTION_INVALID_OPERATION, "invalid result type");
                        break;
                }
                VTABLE_delete_keyed_str(interp, named_return_list, result_name);

                /* Mark the option flag for the filled result. */
                if (result_flags & PARROT_ARG_OPTIONAL) {
                    INTVAL next_result_flags;

                    if (result_index + 1 < result_count) {
                        next_result_flags = VTABLE_get_integer_keyed_int(interp,
                                raw_sig, result_index + 1);
                        if (next_result_flags & PARROT_ARG_OPT_FLAG) {
                            result_index++;
                            result_item = VTABLE_get_pmc_keyed_int(interp,
                                    result_list, result_index);
                            VTABLE_set_integer_native(interp, result_item, 1);
                        }
                    }
                }
            }
            else if (result_flags & PARROT_ARG_OPTIONAL) {
                INTVAL next_result_flags;

                assign_default_result_value(interp, result_item, result_flags);

                /* Mark the option flag for the result to FALSE, it was filled
                 * with a default value. */
                if (result_index + 1 < result_count) {
                    next_result_flags = VTABLE_get_integer_keyed_int(interp,
                            result_sig, result_index + 1);
                    if (next_result_flags & PARROT_ARG_OPT_FLAG) {
                        result_index++;
                        result_item = VTABLE_get_pmc_keyed_int(interp,
                                          result_list, result_index);
                        VTABLE_set_integer_native(interp, result_item, 1);
                    }
                }
            }
            /* We don't have a return for the result, and it's not optional,
             * so it's an error. */
            else {
                if (err_check)
                    Parrot_ex_throw_from_c_args(interp, NULL, EXCEPTION_INVALID_OPERATION,
                            "too few named returns: no return for required result '%S'",
                            result_name);
            }
        }

        result_index++;
    }

    /* Double check that all named returns were assigned to results. */
    if (err_check) {
        if (!PMC_IS_NULL(named_return_list)) {
            INTVAL named_return_count = VTABLE_elements(interp, named_return_list);
            if (named_return_count > 0)
                    Parrot_ex_throw_from_c_args(interp, NULL, EXCEPTION_INVALID_OPERATION,
                            "too many named returns: %d passed, %d used",
                            named_return_count + named_count, named_count);
        }
    }

}

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
    INTVAL raw_return_count     = VTABLE_elements(interp, raw_sig);
    INTVAL err_check      = 0;
    static pcc_get_funcs function_pointers = {
        (intval_func_t)intval_arg_from_op,
        (numval_func_t)numval_arg_from_op,
        (string_func_t)string_arg_from_op,
        (pmc_func_t)pmc_arg_from_op,

        (intval_func_t)intval_constant_from_op,
        (numval_func_t)numval_constant_from_op,
        (string_func_t)string_constant_from_op,
        (pmc_func_t)pmc_constant_from_op,
    };


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
        }
        return;
    }

    fill_results(interp, call_object, raw_sig, raw_returns, &function_pointers);

    return;
}
/*

=item C<void Parrot_pcc_fill_returns_from_continuation(PARROT_INTERP, PMC
*call_object, PMC *raw_sig, PMC *from_call_obj)>

Evil function.  Fill results from arguments passed to a continuation.
Only works for positional arguments.

=cut

*/

PARROT_EXPORT
void
Parrot_pcc_fill_returns_from_continuation(PARROT_INTERP, ARGMOD_NULLOK(PMC *call_object),
        ARGIN(PMC *raw_sig), ARGIN(PMC *from_call_obj))
{
    ASSERT_ARGS(Parrot_pcc_fill_returns_from_continuation)
    INTVAL raw_return_count     = VTABLE_elements(interp, raw_sig);
    INTVAL err_check      = 0;
    static pcc_get_funcs function_pointers = {
        (intval_func_t)intval_arg_from_continuation,
        (numval_func_t)numval_arg_from_continuation,
        (string_func_t)string_arg_from_continuation,
        (pmc_func_t)pmc_arg_from_continuation,

        (intval_func_t)intval_arg_from_continuation,
        (numval_func_t)numval_arg_from_continuation,
        (string_func_t)string_arg_from_continuation,
        (pmc_func_t)pmc_arg_from_continuation,
    };


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
        }
        return;
    }

    fill_results(interp, call_object, raw_sig, from_call_obj, &function_pointers);

    return;
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
Parrot_pcc_fill_returns_from_c_args(PARROT_INTERP, ARGMOD_NULLOK(PMC *call_object),
        ARGIN(const char *signature), ...)
{
    ASSERT_ARGS(Parrot_pcc_fill_returns_from_c_args)
    va_list args;
    INTVAL raw_return_count = 0;
    INTVAL err_check        = 0;
    PMC    *raw_sig         = PMCNULL;
    PMC    *invalid_sig     = PMCNULL;

    static pcc_get_funcs function_pointers = {
        (intval_func_t)intval_arg_from_c_args,
        (numval_func_t)numval_arg_from_c_args,
        (string_func_t)string_arg_from_c_args,
        (pmc_func_t)pmc_arg_from_c_args,

        (intval_func_t)intval_constant_from_varargs,
        (numval_func_t)numval_constant_from_varargs,
        (string_func_t)string_constant_from_varargs,
        (pmc_func_t)pmc_constant_from_varargs,
    };

    parse_signature_string(interp, signature, &raw_sig, &invalid_sig);
    if (!PMC_IS_NULL(invalid_sig))
        Parrot_ex_throw_from_c_args(interp, NULL, EXCEPTION_INVALID_OPERATION,
                "parameters should not be included in the return signature");

    raw_return_count = VTABLE_elements(interp, raw_sig);

    /* Check if we should be throwing errors. This is configured separately
     * for parameters and return values. */
    if (PARROT_ERRORS_test(interp, PARROT_ERRORS_RESULT_COUNT_FLAG))
            err_check = 1;

    /* A null call object is fine if there are no arguments and no returns. */
    if (PMC_IS_NULL(call_object)) {
        if (raw_return_count > 0)
            if (err_check)
                Parrot_ex_throw_from_c_args(interp, NULL,
                    EXCEPTION_INVALID_OPERATION,
                    "too many return values: %d passed, 0 expected",
                    raw_return_count);
        return;
    }

    va_start(args, signature);
    fill_results(interp, call_object, raw_sig, &args, &function_pointers);
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

Get the appropriate argument value from the continuation

=item C<static INTVAL intval_arg_from_continuation(PARROT_INTERP, PMC *cs,
INTVAL arg_index)>

=item C<static FLOATVAL numval_arg_from_continuation(PARROT_INTERP, PMC *cs,
INTVAL arg_index)>

=item C<static STRING* string_arg_from_continuation(PARROT_INTERP, PMC *cs,
INTVAL arg_index)>

=item C<static PMC* pmc_arg_from_continuation(PARROT_INTERP, PMC *cs, INTVAL
arg_index)>


Get the appropriate argument value from the op.

=item C<static INTVAL intval_arg_from_op(PARROT_INTERP, opcode_t *raw_args,
INTVAL arg_index)>

=item C<static FLOATVAL numval_arg_from_op(PARROT_INTERP, opcode_t *raw_args,
INTVAL arg_index)>

=item C<static STRING* string_arg_from_op(PARROT_INTERP, opcode_t *raw_args,
INTVAL arg_index)>

=item C<static PMC* pmc_arg_from_op(PARROT_INTERP, opcode_t *raw_args, INTVAL
arg_index)>

Get the appropriate parameter value from the op (these are pointers, so the
argument value can be stored into them.)

=item C<static INTVAL* intval_param_from_op(PARROT_INTERP, opcode_t *raw_params,
INTVAL param_index)>

=item C<static FLOATVAL* numval_param_from_op(PARROT_INTERP, opcode_t
*raw_params, INTVAL param_index)>

=item C<static STRING** string_param_from_op(PARROT_INTERP, opcode_t
*raw_params, INTVAL param_index)>

=item C<static PMC** pmc_param_from_op(PARROT_INTERP, opcode_t *raw_params,
INTVAL param_index)>

=item C<static INTVAL intval_constant_from_op(PARROT_INTERP, opcode_t
*raw_params, INTVAL param_index)>

=item C<static FLOATVAL numval_constant_from_op(PARROT_INTERP, opcode_t
*raw_params, INTVAL param_index)>

=item C<static STRING* string_constant_from_op(PARROT_INTERP, opcode_t
*raw_params, INTVAL param_index)>

=item C<static PMC* pmc_constant_from_op(PARROT_INTERP, opcode_t *raw_params,
INTVAL param_index)>

Get the appropriate argument value from varargs.

=item C<static INTVAL intval_arg_from_c_args(PARROT_INTERP, va_list *args,
INTVAL param_index)>

=item C<static FLOATVAL numval_arg_from_c_args(PARROT_INTERP, va_list *args,
INTVAL param_index)>

=item C<static STRING* string_arg_from_c_args(PARROT_INTERP, va_list *args,
INTVAL param_index)>

=item C<static PMC* pmc_arg_from_c_args(PARROT_INTERP, va_list *args, INTVAL
param_index)>

Get the appropriate parameter value from varargs (these are pointers, so they
can be set with the argument value).

=item C<static INTVAL* intval_param_from_c_args(PARROT_INTERP, va_list *args,
INTVAL param_index)>

=item C<static FLOATVAL* numval_param_from_c_args(PARROT_INTERP, va_list *args,
INTVAL param_index)>

=item C<static STRING** string_param_from_c_args(PARROT_INTERP, va_list *args,
INTVAL param_index)>

=item C<static PMC** pmc_param_from_c_args(PARROT_INTERP, va_list *args, INTVAL
param_index)>

Parrot constants cannot be passed from varargs, so these functions are dummies
that throw exceptions.

=item C<static INTVAL intval_constant_from_varargs(PARROT_INTERP, void *data,
INTVAL index)>

=item C<static FLOATVAL numval_constant_from_varargs(PARROT_INTERP, void *data,
INTVAL index)>

=item C<static STRING* string_constant_from_varargs(PARROT_INTERP, void *data,
INTVAL index)>

=item C<static PMC* pmc_constant_from_varargs(PARROT_INTERP, void *data, INTVAL
index)>

 - More specific comments can be added later

=cut

*/

PARROT_CANNOT_RETURN_NULL
static INTVAL
intval_arg_from_continuation(PARROT_INTERP, ARGIN(PMC *cs), INTVAL arg_index)
{
    ASSERT_ARGS(intval_arg_from_continuation)
    const INTVAL ret      = VTABLE_get_integer_keyed_int(interp, cs, arg_index);
    return ret;
}

PARROT_CANNOT_RETURN_NULL
static FLOATVAL
numval_arg_from_continuation(PARROT_INTERP, ARGIN(PMC *cs), INTVAL arg_index)
{
    ASSERT_ARGS(numval_arg_from_continuation)
    const FLOATVAL ret      = VTABLE_get_number_keyed_int(interp, cs, arg_index);
    return ret;
}

PARROT_CANNOT_RETURN_NULL
static STRING*
string_arg_from_continuation(PARROT_INTERP, ARGIN(PMC *cs), INTVAL arg_index)
{
    ASSERT_ARGS(string_arg_from_continuation)
    STRING *ret      = VTABLE_get_string_keyed_int(interp, cs, arg_index);
    return ret;
}

PARROT_CANNOT_RETURN_NULL
static PMC*
pmc_arg_from_continuation(PARROT_INTERP, ARGIN(PMC *cs), INTVAL arg_index)
{
    ASSERT_ARGS(pmc_arg_from_continuation)
    PMC *ret      = VTABLE_get_pmc_keyed_int(interp, cs, arg_index);
    return ret;
}

PARROT_CANNOT_RETURN_NULL
static INTVAL
intval_arg_from_op(PARROT_INTERP, ARGIN(opcode_t *raw_args), INTVAL arg_index)
{
    ASSERT_ARGS(intval_arg_from_op)
    const INTVAL raw_index      = raw_args[arg_index + 2];
    return REG_INT(interp, raw_index);
}

PARROT_CANNOT_RETURN_NULL
static FLOATVAL
numval_arg_from_op(PARROT_INTERP, ARGIN(opcode_t *raw_args), INTVAL arg_index)
{
    ASSERT_ARGS(numval_arg_from_op)
    const INTVAL raw_index      = raw_args[arg_index + 2];
    return REG_NUM(interp, raw_index);
}

PARROT_CANNOT_RETURN_NULL
static STRING*
string_arg_from_op(PARROT_INTERP, ARGIN(opcode_t *raw_args), INTVAL arg_index)
{
    ASSERT_ARGS(string_arg_from_op)
    const INTVAL raw_index      = raw_args[arg_index + 2];
    return REG_STR(interp, raw_index);
}

PARROT_CANNOT_RETURN_NULL
static PMC*
pmc_arg_from_op(PARROT_INTERP, ARGIN(opcode_t *raw_args), INTVAL arg_index)
{
    ASSERT_ARGS(pmc_arg_from_op)
    const INTVAL raw_index      = raw_args[arg_index + 2];
    return REG_PMC(interp, raw_index);
}

PARROT_CANNOT_RETURN_NULL
static INTVAL*
intval_param_from_op(PARROT_INTERP, ARGIN(opcode_t *raw_params), INTVAL param_index)
{
    ASSERT_ARGS(intval_param_from_op)
    const INTVAL raw_index      = raw_params[param_index + 2];
    return &REG_INT(interp, raw_index);
}

PARROT_CANNOT_RETURN_NULL
static FLOATVAL*
numval_param_from_op(PARROT_INTERP, ARGIN(opcode_t *raw_params), INTVAL param_index)
{
    ASSERT_ARGS(numval_param_from_op)
    const INTVAL raw_index      = raw_params[param_index + 2];
    return &REG_NUM(interp, raw_index);
}

PARROT_CANNOT_RETURN_NULL
static STRING**
string_param_from_op(PARROT_INTERP, ARGIN(opcode_t *raw_params), INTVAL param_index)
{
    ASSERT_ARGS(string_param_from_op)
    const INTVAL raw_index      = raw_params[param_index + 2];
    return &REG_STR(interp, raw_index);
}

PARROT_CANNOT_RETURN_NULL
static PMC**
pmc_param_from_op(PARROT_INTERP, ARGIN(opcode_t *raw_params), INTVAL param_index)
{
    ASSERT_ARGS(pmc_param_from_op)
    const INTVAL raw_index      = raw_params[param_index + 2];
    return &REG_PMC(interp, raw_index);
}

static INTVAL
intval_constant_from_op(PARROT_INTERP, ARGIN(opcode_t *raw_params), INTVAL param_index)
{
    ASSERT_ARGS(intval_constant_from_op)
    const INTVAL raw_index      = raw_params[param_index + 2];
    return raw_index;
}

static FLOATVAL
numval_constant_from_op(PARROT_INTERP, ARGIN(opcode_t *raw_params), INTVAL param_index)
{
    ASSERT_ARGS(numval_constant_from_op)
    const INTVAL raw_index      = raw_params[param_index + 2];
    return Parrot_pcc_get_num_constant(interp, CURRENT_CONTEXT(interp), raw_index);
}

PARROT_CAN_RETURN_NULL
static STRING*
string_constant_from_op(PARROT_INTERP, ARGIN(opcode_t *raw_params), INTVAL param_index)
{
    ASSERT_ARGS(string_constant_from_op)
    const INTVAL raw_index      = raw_params[param_index + 2];
    return Parrot_pcc_get_string_constant(interp, CURRENT_CONTEXT(interp), raw_index);
}

PARROT_CAN_RETURN_NULL
static PMC*
pmc_constant_from_op(PARROT_INTERP, ARGIN(opcode_t *raw_params), INTVAL param_index)
{
    ASSERT_ARGS(pmc_constant_from_op)
    const INTVAL raw_index      = raw_params[param_index + 2];
    return Parrot_pcc_get_pmc_constant(interp, CURRENT_CONTEXT(interp), raw_index);
}

PARROT_CANNOT_RETURN_NULL
static INTVAL
intval_arg_from_c_args(PARROT_INTERP, ARGIN(va_list *args), SHIM(INTVAL param_index))
{
    ASSERT_ARGS(intval_arg_from_c_args)
    return va_arg(*args, INTVAL);
}

PARROT_CANNOT_RETURN_NULL
static FLOATVAL
numval_arg_from_c_args(PARROT_INTERP, ARGIN(va_list *args), SHIM(INTVAL param_index))
{
    ASSERT_ARGS(numval_arg_from_c_args)
    return va_arg(*args, FLOATVAL);
}

PARROT_CANNOT_RETURN_NULL
static STRING*
string_arg_from_c_args(PARROT_INTERP, ARGIN(va_list *args), SHIM(INTVAL param_index))
{
    ASSERT_ARGS(string_arg_from_c_args)
    return va_arg(*args, STRING*);
}

PARROT_CANNOT_RETURN_NULL
static PMC*
pmc_arg_from_c_args(PARROT_INTERP, ARGIN(va_list *args), SHIM(INTVAL param_index))
{
    ASSERT_ARGS(pmc_arg_from_c_args)
    return va_arg(*args, PMC*);
}

PARROT_CANNOT_RETURN_NULL
static INTVAL*
intval_param_from_c_args(PARROT_INTERP, ARGIN(va_list *args), SHIM(INTVAL param_index))
{
    ASSERT_ARGS(intval_param_from_c_args)
    return va_arg(*args, INTVAL*);
}

PARROT_CANNOT_RETURN_NULL
static FLOATVAL*
numval_param_from_c_args(PARROT_INTERP, ARGIN(va_list *args), SHIM(INTVAL param_index))
{
    ASSERT_ARGS(numval_param_from_c_args)
    return va_arg(*args, FLOATVAL*);
}

PARROT_CANNOT_RETURN_NULL
static STRING**
string_param_from_c_args(PARROT_INTERP, ARGIN(va_list *args), SHIM(INTVAL param_index))
{
    ASSERT_ARGS(string_param_from_c_args)
    return va_arg(*args, STRING**);
}

PARROT_CANNOT_RETURN_NULL
static PMC**
pmc_param_from_c_args(PARROT_INTERP, ARGIN(va_list *args), SHIM(INTVAL param_index))
{
    ASSERT_ARGS(pmc_param_from_c_args)
    return va_arg(*args, PMC**);
}

static INTVAL
intval_constant_from_varargs(PARROT_INTERP, ARGIN(void *data), INTVAL index)
{
    ASSERT_ARGS(intval_constant_from_varargs)
    PARROT_ASSERT(!"Wrong call");
    return 0;
}

static FLOATVAL
numval_constant_from_varargs(PARROT_INTERP, ARGIN(void *data), INTVAL index)
{
    ASSERT_ARGS(numval_constant_from_varargs)
    PARROT_ASSERT(!"Wrong call");
    return 0.0;
}

PARROT_CAN_RETURN_NULL
static STRING*
string_constant_from_varargs(PARROT_INTERP, ARGIN(void *data), INTVAL index)
{
    ASSERT_ARGS(string_constant_from_varargs)
    PARROT_ASSERT(!"Wrong call");
    return NULL;
}

PARROT_CAN_RETURN_NULL
static PMC*
pmc_constant_from_varargs(PARROT_INTERP, ARGIN(void *data), INTVAL index)
{
    ASSERT_ARGS(pmc_constant_from_varargs)
    PARROT_ASSERT(!"Wrong call");
    return PMCNULL;
}

/*

=item C<static PMC* clone_key_arg(PARROT_INTERP, PMC *key)>

Replaces any src registers by their values (done inside clone).  This needs a
test for tailcalls too, but I think there is no syntax to pass a key to a
tailcalled function or method.

=cut

*/

PARROT_CAN_RETURN_NULL
static PMC*
clone_key_arg(PARROT_INTERP, ARGIN(PMC *key))
{
    ASSERT_ARGS(clone_key_arg)
    PMC *t;

    if (PMC_IS_NULL(key))
        return key;

    if (key->vtable->base_type != enum_class_Key)
        return key;

    for (t = key; t; t=key_next(interp, t)) {
        /* register keys have to be cloned */
        if (PObj_get_FLAGS(key) & KEY_register_FLAG) {
            return VTABLE_clone(interp, key);
        }
    }

    return key;
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
