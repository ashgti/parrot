/* register.h
 *  Copyright (C) 2001-2008, Parrot Foundation.
 *  SVN Info
 *     $Id$
 *  Overview:
 *     Defines the register api
 *  Data Structure and Algorithms:
 *  History:
 *  Notes:
 *  References:
 */

#ifndef PARROT_REGISTER_H_GUARD
#define PARROT_REGISTER_H_GUARD

#include "parrot/core_types.h"
#include "parrot/string.h"
#include "parrot/compiler.h"

/*
 * Macros to make accessing registers more convenient/readable.
 */

/* Macros for interfacing with the Parrot_Context structure */
#define CTX_REG_NUM(ctx, x) (ctx)->bp.regs_n[-1L-(x)]
#define CTX_REG_INT(ctx, x) (ctx)->bp.regs_i[x]
#define CTX_REG_PMC(ctx, x) (ctx)->bp_ps.regs_p[-1L-(x)]
#define CTX_REG_STR(ctx, x) (ctx)->bp_ps.regs_s[x]

/* Macros for interfacing with the Context PMC */
#define REG_NUM(interp, x) (*(Parrot_ctx_FLOATVAL_reg((interp), (interp)->ctx, (x))))
#define REG_INT(interp, x) (*(Parrot_ctx_INTVAL_reg((interp), (interp)->ctx, (x))))
#define REG_PMC(interp, x) (*(Parrot_ctx_PMC_reg((interp), (interp)->ctx, (x))))
#define REG_STR(interp, x) (*(Parrot_ctx_STRING_reg((interp), (interp)->ctx, (x))))

#define PMCCTX_REG_NUM(ctx, x) PARROT_CONTEXT(ctx)->bp.regs_n[-1L-(x)]
#define PMCCTX_REG_INT(ctx, x) PARROT_CONTEXT(ctx)->bp.regs_i[x]
#define PMCCTX_REG_PMC(ctx, x) PARROT_CONTEXT(ctx)->bp_ps.regs_p[-1L-(x)]
#define PMCCTX_REG_STR(ctx, x) PARROT_CONTEXT(ctx)->bp_ps.regs_s[x]

/*
 * and a set of macros to access a register by offset, used
 * in JIT emit prederef code
 * The offsets are relative to interp->ctx.bp.
 *
 * Reg order in imcc/reg_alloc.c is "INSP"   TODO make defines
 */

#define REGNO_INT 0
#define REGNO_NUM 1
#define REGNO_STR 2
#define REGNO_PMC 3

#define __CTX interp->ctx.state
#define _SIZEOF_INTS    (sizeof (INTVAL) * __CTX->n_regs_used[REGNO_INT])
#define _SIZEOF_NUMS    (sizeof (FLOATVAL) * __CTX->n_regs_used[REGNO_NUM])
#define _SIZEOF_PMCS    (sizeof (PMC*) * __CTX->n_regs_used[REGNO_PMC])
#define _SIZEOF_STRS    (sizeof (STRING*) * __CTX->n_regs_used[REGNO_STR])

#define REG_OFFS_NUM(x) (sizeof (FLOATVAL) * (-1L - (x)))
#define REG_OFFS_INT(x) (sizeof (INTVAL) * (x))
#define REG_OFFS_PMC(x) (_SIZEOF_INTS + sizeof (PMC*) * \
        (__CTX->n_regs_used[REGNO_PMC] - 1L - (x)))
#define REG_OFFS_STR(x) (sizeof (STRING*) * (x) + _SIZEOF_INTS + _SIZEOF_PMCS)

/* Forward declaration of Context PMC attributes */
struct Parrot_Context_attributes;
//typedef struct Parrot_Context_attributes Parrot_Context_attributes;

/* HEADERIZER BEGIN: src/gc/alloc_register.c */
/* Don't modify between HEADERIZER BEGIN / HEADERIZER END.  Your changes will be lost. */

PARROT_EXPORT
void Parrot_clear_i(PARROT_INTERP)
        __attribute__nonnull__(1);

PARROT_EXPORT
void Parrot_clear_n(PARROT_INTERP)
        __attribute__nonnull__(1);

PARROT_EXPORT
void Parrot_clear_p(PARROT_INTERP)
        __attribute__nonnull__(1);

PARROT_EXPORT
void Parrot_clear_s(PARROT_INTERP)
        __attribute__nonnull__(1);

PARROT_EXPORT
void Parrot_pop_context(PARROT_INTERP)
        __attribute__nonnull__(1);

PARROT_EXPORT
PARROT_WARN_UNUSED_RESULT
PARROT_CANNOT_RETURN_NULL
PMC * Parrot_push_context(PARROT_INTERP, ARGIN(const INTVAL *n_regs_used))
        __attribute__nonnull__(1)
        __attribute__nonnull__(2);

void create_initial_context(PARROT_INTERP)
        __attribute__nonnull__(1);

PARROT_CANNOT_RETURN_NULL
PARROT_WARN_UNUSED_RESULT
PMC * Parrot_alloc_context(PARROT_INTERP,
    ARGIN(const INTVAL *number_regs_used),
    ARGIN_NULLOK(PMC *old))
        __attribute__nonnull__(1)
        __attribute__nonnull__(2);

PARROT_CANNOT_RETURN_NULL
FLOATVAL * Parrot_ctx_FLOATVAL_reg(PARROT_INTERP,
    ARGIN(PMC *ctx),
    INTVAL idx)
        __attribute__nonnull__(1)
        __attribute__nonnull__(2);

PARROT_CANNOT_RETURN_NULL
PMC * Parrot_ctx_get_caller(PARROT_INTERP, ARGIN(PMC *ctx))
        __attribute__nonnull__(1)
        __attribute__nonnull__(2);

PARROT_CANNOT_RETURN_NULL
INTVAL * Parrot_ctx_INTVAL_reg(PARROT_INTERP, ARGIN(PMC *ctx), INTVAL idx)
        __attribute__nonnull__(1)
        __attribute__nonnull__(2);

PARROT_CANNOT_RETURN_NULL
PMC ** Parrot_ctx_PMC_reg(PARROT_INTERP, ARGIN(PMC *ctx), INTVAL idx)
        __attribute__nonnull__(1)
        __attribute__nonnull__(2);

PARROT_CANNOT_RETURN_NULL
STRING ** Parrot_ctx_STRING_reg(PARROT_INTERP, ARGIN(PMC *ctx), INTVAL idx)
        __attribute__nonnull__(1)
        __attribute__nonnull__(2);

PARROT_CANNOT_RETURN_NULL
PARROT_WARN_UNUSED_RESULT
PMC * Parrot_set_new_context(PARROT_INTERP,
    ARGIN(const INTVAL *number_regs_used))
        __attribute__nonnull__(1)
        __attribute__nonnull__(2);

#define ASSERT_ARGS_Parrot_clear_i __attribute__unused__ int _ASSERT_ARGS_CHECK = \
       PARROT_ASSERT_ARG(interp)
#define ASSERT_ARGS_Parrot_clear_n __attribute__unused__ int _ASSERT_ARGS_CHECK = \
       PARROT_ASSERT_ARG(interp)
#define ASSERT_ARGS_Parrot_clear_p __attribute__unused__ int _ASSERT_ARGS_CHECK = \
       PARROT_ASSERT_ARG(interp)
#define ASSERT_ARGS_Parrot_clear_s __attribute__unused__ int _ASSERT_ARGS_CHECK = \
       PARROT_ASSERT_ARG(interp)
#define ASSERT_ARGS_Parrot_pop_context __attribute__unused__ int _ASSERT_ARGS_CHECK = \
       PARROT_ASSERT_ARG(interp)
#define ASSERT_ARGS_Parrot_push_context __attribute__unused__ int _ASSERT_ARGS_CHECK = \
       PARROT_ASSERT_ARG(interp) \
    || PARROT_ASSERT_ARG(n_regs_used)
#define ASSERT_ARGS_create_initial_context __attribute__unused__ int _ASSERT_ARGS_CHECK = \
       PARROT_ASSERT_ARG(interp)
#define ASSERT_ARGS_Parrot_alloc_context __attribute__unused__ int _ASSERT_ARGS_CHECK = \
       PARROT_ASSERT_ARG(interp) \
    || PARROT_ASSERT_ARG(number_regs_used)
#define ASSERT_ARGS_Parrot_ctx_FLOATVAL_reg __attribute__unused__ int _ASSERT_ARGS_CHECK = \
       PARROT_ASSERT_ARG(interp) \
    || PARROT_ASSERT_ARG(ctx)
#define ASSERT_ARGS_Parrot_ctx_get_caller __attribute__unused__ int _ASSERT_ARGS_CHECK = \
       PARROT_ASSERT_ARG(interp) \
    || PARROT_ASSERT_ARG(ctx)
#define ASSERT_ARGS_Parrot_ctx_INTVAL_reg __attribute__unused__ int _ASSERT_ARGS_CHECK = \
       PARROT_ASSERT_ARG(interp) \
    || PARROT_ASSERT_ARG(ctx)
#define ASSERT_ARGS_Parrot_ctx_PMC_reg __attribute__unused__ int _ASSERT_ARGS_CHECK = \
       PARROT_ASSERT_ARG(interp) \
    || PARROT_ASSERT_ARG(ctx)
#define ASSERT_ARGS_Parrot_ctx_STRING_reg __attribute__unused__ int _ASSERT_ARGS_CHECK = \
       PARROT_ASSERT_ARG(interp) \
    || PARROT_ASSERT_ARG(ctx)
#define ASSERT_ARGS_Parrot_set_new_context __attribute__unused__ int _ASSERT_ARGS_CHECK = \
       PARROT_ASSERT_ARG(interp) \
    || PARROT_ASSERT_ARG(number_regs_used)
/* Don't modify between HEADERIZER BEGIN / HEADERIZER END.  Your changes will be lost. */
/* HEADERIZER END: src/gc/alloc_register.c */

#endif /* PARROT_REGISTER_H_GUARD */

/*
 * Local variables:
 *   c-file-style: "parrot"
 * End:
 * vim: expandtab shiftwidth=4:
 */
