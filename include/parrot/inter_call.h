/* inter_call.h
 *  Copyright (C) 2001-2007, The Perl Foundation.
 *  SVN Info
 *  $Id$
 *  Overview:
 *  Data Structure and Algorithms:
 *     Call argument handling.
 *  History:
 *     Initial version by leo on 2005/07/22
 *     Major changes by mdiep in April 2007
 *  Notes:
 *  References:
 *     pdd03 - Calling Conventions
 */

#ifndef PARROT_INTER_CALL_H_GUARD
#define PARROT_INTER_CALL_H_GUARD

enum call_state_mode {
    /* arg processing states
     *       <src>_<dest>           sd  nibbles    */

    CALL_STATE_SIG        =  0x100,     /* runops, nci */
    CALL_STATE_OP         =  0x200,     /* get_, set_ ops */
    CALL_S_D_MASK         =  0x300,     /* src/dest mask */

    CALL_STATE_FLATTEN    =  0x400      /* flatten src */

};

typedef struct call_state_item {
    int mode;       /* from_sig, from_set_ops, flatten ...*/
    union {
        struct {
            void *ap;   /* a ptr to va_list */
            const char *sig;
        } sig;
        struct {
            opcode_t *pc;
            PMC *signature;
        } op;
    } u;
    parrot_context_t *ctx;
    INTVAL used;
    INTVAL i;
    INTVAL n;
    INTVAL sig;
    PMC *slurp;
    INTVAL slurp_i;
    INTVAL slurp_n;
} call_state_item;

typedef struct call_state {
    call_state_item src;
    call_state_item dest;
    UnionVal val;
    int n_actual_args;  /* arguments incl. flatten */
    int optionals;      /* sum of optionals */
    int params;         /* sum of params */
    int first_named;    /* param idx of 1st named */
    UINTVAL named_done; /* bit mask, 1 if named was assigned */
    STRING *name;       /* name of argument if any */
    PMC *key;           /* to iterate a flattening hash */
} call_state;

typedef enum arg_pass_t {
    PARROT_PASS_PARAMS          = 0x00,
    PARROT_PASS_RESULTS         = 0x01
} arg_pass_t;

/* HEADERIZER BEGIN: src/inter_call.c */

PARROT_API
void Parrot_convert_arg(PARROT_INTERP, ARGMOD(call_state *st))
        __attribute__nonnull__(1)
        __attribute__nonnull__(2)
        FUNC_MODIFIES(*st);

PARROT_API
int Parrot_fetch_arg(PARROT_INTERP, ARGMOD(call_state *st))
        __attribute__nonnull__(1)
        __attribute__nonnull__(2)
        FUNC_MODIFIES(*st);

PARROT_API
int Parrot_fetch_arg_nci(PARROT_INTERP, ARGMOD(call_state *st))
        __attribute__nonnull__(1)
        __attribute__nonnull__(2)
        FUNC_MODIFIES(*st);

PARROT_API
int Parrot_init_arg_indexes_and_sig_pmc(SHIM_INTERP,
    ARGIN(parrot_context_t *ctx),
    ARGIN(opcode_t *indexes),
    ARGIN_NULLOK(PMC* sig_pmc),
    ARGMOD(call_state_item *sti))
        __attribute__nonnull__(2)
        __attribute__nonnull__(3)
        __attribute__nonnull__(5)
        FUNC_MODIFIES(*sti);

PARROT_API
int Parrot_init_arg_nci(PARROT_INTERP,
    ARGOUT(call_state *st),
    ARGIN(const char *sig))
        __attribute__nonnull__(1)
        __attribute__nonnull__(2)
        __attribute__nonnull__(3)
        FUNC_MODIFIES(*st);

PARROT_API
int Parrot_init_arg_op(PARROT_INTERP,
    ARGIN(parrot_context_t *ctx),
    ARGIN_NULLOK(opcode_t *pc),
    ARGIN(call_state_item *sti))
        __attribute__nonnull__(1)
        __attribute__nonnull__(2)
        __attribute__nonnull__(4);

PARROT_API
int Parrot_init_arg_sig(SHIM_INTERP,
    ARGIN(parrot_context_t *ctx),
    ARGIN(const char *sig),
    ARGIN_NULLOK(void *ap),
    ARGMOD(call_state_item *sti))
        __attribute__nonnull__(2)
        __attribute__nonnull__(3)
        __attribute__nonnull__(5)
        FUNC_MODIFIES(*sti);

PARROT_API
int Parrot_init_ret_nci(PARROT_INTERP,
    ARGOUT(call_state *st),
    ARGIN(const char *sig))
        __attribute__nonnull__(1)
        __attribute__nonnull__(2)
        __attribute__nonnull__(3)
        FUNC_MODIFIES(*st);

PARROT_API
void parrot_pass_args(PARROT_INTERP,
    ARGMOD(parrot_context_t *src_ctx),
    ARGMOD(parrot_context_t *dest_ctx),
    ARGMOD(opcode_t *src_indexes),
    ARGMOD(opcode_t *dest_indexes),
    arg_pass_t param_or_result)
        __attribute__nonnull__(1)
        __attribute__nonnull__(2)
        __attribute__nonnull__(3)
        __attribute__nonnull__(4)
        __attribute__nonnull__(5)
        FUNC_MODIFIES(*src_ctx)
        FUNC_MODIFIES(*dest_ctx)
        FUNC_MODIFIES(*src_indexes)
        FUNC_MODIFIES(*dest_indexes);

PARROT_API
void Parrot_PCCINVOKE(PARROT_INTERP,
    ARGIN(PMC* pmc),
    ARGMOD(STRING *method_name),
    ARGIN(const char *signature),
    ...)
        __attribute__nonnull__(1)
        __attribute__nonnull__(2)
        __attribute__nonnull__(3)
        __attribute__nonnull__(4)
        FUNC_MODIFIES(*method_name);

PARROT_API
void Parrot_process_args(PARROT_INTERP,
    ARGMOD(call_state *st),
    arg_pass_t param_or_result)
        __attribute__nonnull__(1)
        __attribute__nonnull__(2)
        FUNC_MODIFIES(*st);

PARROT_CANNOT_RETURN_NULL
PARROT_WARN_UNUSED_RESULT
opcode_t * parrot_pass_args_fromc(PARROT_INTERP,
    ARGIN(const char *sig),
    ARGMOD(opcode_t *dest),
    ARGIN(parrot_context_t *old_ctxp),
    va_list ap)
        __attribute__nonnull__(1)
        __attribute__nonnull__(2)
        __attribute__nonnull__(3)
        __attribute__nonnull__(4)
        FUNC_MODIFIES(*dest);

int Parrot_store_arg(SHIM_INTERP, ARGIN(const call_state *st))
        __attribute__nonnull__(2);

PARROT_WARN_UNUSED_RESULT
PARROT_CAN_RETURN_NULL
void* set_retval(PARROT_INTERP, int sig_ret, ARGIN(parrot_context_t *ctx))
        __attribute__nonnull__(1)
        __attribute__nonnull__(3);

FLOATVAL set_retval_f(PARROT_INTERP,
    int sig_ret,
    ARGIN(parrot_context_t *ctx))
        __attribute__nonnull__(1)
        __attribute__nonnull__(3);

INTVAL set_retval_i(PARROT_INTERP,
    int sig_ret,
    ARGIN(parrot_context_t *ctx))
        __attribute__nonnull__(1)
        __attribute__nonnull__(3);

PARROT_CAN_RETURN_NULL
PARROT_WARN_UNUSED_RESULT
PMC* set_retval_p(PARROT_INTERP, int sig_ret, ARGIN(parrot_context_t *ctx))
        __attribute__nonnull__(1)
        __attribute__nonnull__(3);

PARROT_CAN_RETURN_NULL
PARROT_WARN_UNUSED_RESULT
STRING* set_retval_s(PARROT_INTERP,
    int sig_ret,
    ARGIN(parrot_context_t *ctx))
        __attribute__nonnull__(1)
        __attribute__nonnull__(3);

/* HEADERIZER END: src/inter_call.c */

#define ASSERT_SIG_PMC(sig) \
    PARROT_ASSERT(PObj_is_PMC_TEST(sig)); \
    PARROT_ASSERT(sig->vtable->base_type == enum_class_FixedIntegerArray)

#define SIG_ELEMS(sig) PMC_int_val(sig)
#define SIG_ARRAY(sig) (INTVAL*)PMC_data(sig)
#define SIG_ITEM(sig, idx) (SIG_ARRAY(sig))[idx]

/* XXX Remove interp from this */
#define ADD_OP_VAR_PART(interp, seg, pc, n) do { \
    if (*(pc) == PARROT_OP_set_args_pc || \
            *(pc) == PARROT_OP_get_results_pc || \
            *(pc) == PARROT_OP_get_params_pc || \
            *(pc) == PARROT_OP_set_returns_pc) { \
        PMC * const sig = seg->const_table->constants[pc[1]]->u.key; \
        n += SIG_ELEMS(sig); \
    } \
} while (0)


#endif /* PARROT_INTER_CALL_H_GUARD */

/*
 * Local variables:
 *   c-file-style: "parrot"
 * End:
 * vim: expandtab shiftwidth=4:
 */
