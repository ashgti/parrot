/* context.h
 *  Copyright (C) 2009, Parrot Foundation.
 *  SVN Info
 *     $Id$
 *  Overview:
 *     Context
 */

#ifndef PARROT_CONTEXT_H_GUARD
#define PARROT_CONTEXT_H_GUARD

#include "parrot/string.h"
#include "parrot/compiler.h"

/*
 * Macros to make accessing registers more convenient/readable.
 */

#ifndef NDEBUG

#  define CTX_REG_NUM(p, x) (*Parrot_pcc_get_FLOATVAL_reg(interp, (p), (x)))
#  define CTX_REG_INT(p, x) (*Parrot_pcc_get_INTVAL_reg(interp, (p), (x)))
#  define CTX_REG_PMC(p, x) (*Parrot_pcc_get_PMC_reg(interp, (p), (x)))
#  define CTX_REG_STR(p, x) (*Parrot_pcc_get_STRING_reg(interp, (p), (x)))

#  define REG_NUM(interp, x) (*Parrot_pcc_get_FLOATVAL_reg((interp), (interp)->ctx, (x)))
#  define REG_INT(interp, x) (*Parrot_pcc_get_INTVAL_reg((interp), (interp)->ctx, (x)))
#  define REG_PMC(interp, x) (*Parrot_pcc_get_PMC_reg((interp), (interp)->ctx, (x)))
#  define REG_STR(interp, x) (*Parrot_pcc_get_STRING_reg((interp), (interp)->ctx, (x)))

#else /* NDEBUG */

/* Manually inlined macros. Used in optimised builds */

#  define __C(c) (PMC_data_typed((c), Parrot_Context*))

#  define CTX_REG_NUM(p, x) (__C(p)->bp.regs_n[-1L - (x)])
#  define CTX_REG_INT(p, x) (__C(p)->bp.regs_i[(x)])
#  define CTX_REG_PMC(p, x) (__C(p)->bp_ps.regs_p[-1L - (x)])
#  define CTX_REG_STR(p, x) (__C(p)->bp_ps.regs_s[(x)])

#  define REG_NUM(interp, x) CTX_REG_NUM((interp)->ctx, (x))
#  define REG_INT(interp, x) CTX_REG_INT((interp)->ctx, (x))
#  define REG_PMC(interp, x) CTX_REG_PMC((interp)->ctx, (x))
#  define REG_STR(interp, x) CTX_REG_STR((interp)->ctx, (x))

#endif

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

#define __CTX Parrot_pcc_get_context_struct(interp, interp->ctx)
#define _SIZEOF_INTS    (sizeof (INTVAL) * __CTX->n_regs_used[REGNO_INT])
#define _SIZEOF_NUMS    (sizeof (FLOATVAL) * __CTX->n_regs_used[REGNO_NUM])
#define _SIZEOF_PMCS    (sizeof (PMC*) * __CTX->n_regs_used[REGNO_PMC])
#define _SIZEOF_STRS    (sizeof (STRING*) * __CTX->n_regs_used[REGNO_STR])

#define REG_OFFS_NUM(x) (sizeof (FLOATVAL) * (-1L - (x)))
#define REG_OFFS_INT(x) (sizeof (INTVAL) * (x))
#define REG_OFFS_PMC(x) (_SIZEOF_INTS + sizeof (PMC*) * \
        (__CTX->n_regs_used[REGNO_PMC] - 1L - (x)))
#define REG_OFFS_STR(x) (sizeof (STRING*) * (x) + _SIZEOF_INTS + _SIZEOF_PMCS)


struct PackFile_Constant;

typedef union {
    PMC         **regs_p;
    STRING      **regs_s;
} Regs_ps;

typedef union {
    FLOATVAL     *regs_n;
    INTVAL       *regs_i;
} Regs_ni;

struct Parrot_Context {
    PMC     *caller_ctx;      /* caller context */
    Regs_ni  bp;              /* pointers to FLOATVAL & INTVAL */
    Regs_ps  bp_ps;           /* pointers to PMC & STR */

    /* end common header */
    UINTVAL  n_regs_used[4];   /* INSP in PBC points to Sub */
    PMC      *lex_pad;         /* LexPad PMC */
    PMC      *outer_ctx;       /* outer context, if a closure */

    /* new call scheme and introspective variables */
    PMC      *current_sub;           /* the Sub we are executing */

    /* for now use a return continuation PMC */
    PMC      *handlers;              /* local handlers for the context */
    PMC      *current_cont;          /* the return continuation PMC */
    PMC      *current_object;        /* current object if a method call */
    PMC      *current_namespace;     /* The namespace we're currently in */
    PMC      *results_signature;     /* non-const results signature PMC */
    opcode_t *current_pc;            /* program counter of Sub invocation */
    opcode_t *current_results;       /* ptr into code with get_results opcode */

    /* deref the constants - we need it all the time */
    struct PackFile_Constant **constants;

    INTVAL                 current_HLL;     /* see also src/hll.c */

    UINTVAL                warns;           /* Keeps track of what warnings
                                             * have been activated */
    UINTVAL                errors;          /* fatals that can be turned off */
    UINTVAL                trace_flags;
    UINTVAL                recursion_depth; /* Sub call recursion depth */

    /* code->prederefed.code - code->base.data in opcodes
     * to simplify conversion between code ptrs in e.g. invoke */
    size_t pred_offset;
};

typedef struct Parrot_Context Parrot_Context;

#define PMC_context(pmc) Parrot_cx_get_context(interp, (pmc))


#endif /* PARROT_CONTEXT_H_GUARD */

/*
 * Local variables:
 *   c-file-style: "parrot"
 * End:
 * vim: expandtab shiftwidth=4:
 */
