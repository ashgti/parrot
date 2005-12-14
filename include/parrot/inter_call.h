/* inter_call.h
 *  Copyright: 2001-2003 The Perl Foundation.  All Rights Reserved.
 *  CVS Info
 *     $Id$
 *  Overview:
 *  Data Structure and Algorithms:
 *     Call argument handling.
 *  History:
 *     Initial version by leo on on 2005/07/22
 *  Notes:
 *  References:
 */

#if !defined(PARROT_INTER_CALL_H_GUARD)
#define PARROT_INTER_CALL_H_GUARD

enum call_state_mode {
    CALL_STATE_SIG        =  0x001,     /* runops, nci */
    CALL_STATE_OP         =  0x002,     /* get_, set_ ops */
    CALL_STATE_MASK       =  0x003,

    CALL_STATE_FLATTEN    =  0x010,

    CALL_STATE_NEXT_ARG   =  0x100
};

struct call_state_1 {
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
    INTVAL i;
    INTVAL n;
    INTVAL sig;
    PMC *slurp;
    INTVAL slurp_i;
    INTVAL slurp_n;
};

struct call_state {
    struct call_state_1 src;
    struct call_state_1 dest;
    UnionVal val;
    int opt_so_far;
    int n_actual_args;
};

int Parrot_init_arg_sig(Interp *, parrot_context_t *ctx,
        const char *sig, void *ap, struct call_state_1 *st);

int Parrot_init_arg_op(Interp *, parrot_context_t *ctx,
        opcode_t *pc, struct call_state_1 *st);

int Parrot_init_arg_nci(Interp *, struct call_state *st, const char *sig);
int Parrot_init_ret_nci(Interp *, struct call_state *st, const char *sig);

int Parrot_fetch_arg(Interp *, struct call_state *st);
int Parrot_fetch_arg_nci(Interp *, struct call_state *st);
int Parrot_convert_arg(Interp *, struct call_state *st);
int Parrot_store_arg(Interp *, struct call_state *st);

opcode_t * parrot_pass_args(Interp *, parrot_context_t *src_ctx,
        parrot_context_t *dest_ctx, int what);
opcode_t * parrot_pass_args_fromc(Interp *, const char *sig,
        opcode_t *dest, parrot_context_t * ctx, va_list ap);
FLOATVAL set_retval_f(Interp*, int sig_ret, parrot_context_t *ctx);
void* set_retval(Interp*, int sig_ret, parrot_context_t *ctx);
INTVAL set_retval_i(Interp*, int sig_ret, parrot_context_t *ctx);

#define ADD_OP_VAR_PART(interpreter, seg, pc, n) do { \
    if (*pc == PARROT_OP_set_args_pc || \
            *pc == PARROT_OP_get_results_pc || \
            *pc == PARROT_OP_get_params_pc || \
            *pc == PARROT_OP_set_returns_pc) { \
        PMC *sig; \
        sig = seg->const_table->constants[pc[1]]->u.key; \
        n += VTABLE_elements(interpreter, sig); \
    } \
} while (0)

#endif /* PARROT_INTER_CALL_H_GUARD */

/*
 * Local variables:
 * c-indentation-style: bsd
 * c-basic-offset: 4
 * indent-tabs-mode: nil
 * End:
 *
 * vim: expandtab shiftwidth=4:
 */
