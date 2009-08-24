/* context.h
 *  Copyright (C) 2001-2009, Parrot Foundation.
 *  SVN Info
 *     $Id$
 *  Overview:
 *     Context
 */

#ifndef PARROT_CONTEXT_H_GUARD
#define PARROT_CONTEXT_H_GUARD

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
    /* common header with Interp_Context */
    struct PMC            *caller_ctx;      /* caller context */
    Regs_ni                bp;              /* pointers to FLOATVAL & INTVAL */
    Regs_ps                bp_ps;           /* pointers to PMC & STR */

    /* end common header */
    INTVAL                n_regs_used[4];   /* INSP in PBC points to Sub */
    PMC                   *lex_pad;         /* LexPad PMC */
    struct PMC            *outer_ctx;       /* outer context, if a closure */

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
    size_t                 regs_mem_size;   /* memory occupied by registers */
    int                    gc_mark;         /* marked in gc run */

    UINTVAL                warns;           /* Keeps track of what warnings
                                             * have been activated */
    UINTVAL                errors;          /* fatals that can be turned off */
    UINTVAL                trace_flags;
    UINTVAL                recursion_depth; /* Sub call recursion depth */

    /* code->prederefed.code - code->base.data in opcodes
     * to simplify conversion between code ptrs in e.g. invoke */
    size_t pred_offset;
};

#define PMC_context(pmc) Parrot_cx_get_context(interp, (pmc))

/* HEADERIZER BEGIN: src/context.c */
/* Don't modify between HEADERIZER BEGIN / HEADERIZER END.  Your changes will be lost. */

PARROT_EXPORT
PARROT_CANNOT_RETURN_NULL
struct PackFile_Constant ** Parrot_cx_constants(PARROT_INTERP,
    ARGIN(PMC *ctx))
        __attribute__nonnull__(1)
        __attribute__nonnull__(2);

PARROT_EXPORT
UINTVAL Parrot_cx_dec_recursion_depth(PARROT_INTERP, ARGIN(PMC *ctx))
        __attribute__nonnull__(1)
        __attribute__nonnull__(2);

PARROT_EXPORT
PARROT_CAN_RETURN_NULL
Parrot_Context* Parrot_cx_get_context(PARROT_INTERP, ARGIN_NULLOK(PMC *ctx))
        __attribute__nonnull__(1);

PARROT_EXPORT
PARROT_CAN_RETURN_NULL
PMC* Parrot_cx_get_pmc_constant(PARROT_INTERP, ARGIN(PMC *ctx), INTVAL idx)
        __attribute__nonnull__(1)
        __attribute__nonnull__(2);

PARROT_EXPORT
UINTVAL Parrot_cx_get_recursion_depth(PARROT_INTERP, ARGIN(PMC *ctx))
        __attribute__nonnull__(1)
        __attribute__nonnull__(2);

PARROT_EXPORT
PARROT_CAN_RETURN_NULL
STRING* Parrot_cx_get_string_constant(PARROT_INTERP,
    ARGIN(PMC *ctx),
    INTVAL idx)
        __attribute__nonnull__(1)
        __attribute__nonnull__(2);

PARROT_EXPORT
UINTVAL Parrot_cx_inc_recursion_depth(PARROT_INTERP, ARGIN(PMC *ctx))
        __attribute__nonnull__(1)
        __attribute__nonnull__(2);

#define ASSERT_ARGS_Parrot_cx_constants __attribute__unused__ int _ASSERT_ARGS_CHECK = \
       PARROT_ASSERT_ARG(interp) \
    || PARROT_ASSERT_ARG(ctx)
#define ASSERT_ARGS_Parrot_cx_dec_recursion_depth __attribute__unused__ int _ASSERT_ARGS_CHECK = \
       PARROT_ASSERT_ARG(interp) \
    || PARROT_ASSERT_ARG(ctx)
#define ASSERT_ARGS_Parrot_cx_get_context __attribute__unused__ int _ASSERT_ARGS_CHECK = \
       PARROT_ASSERT_ARG(interp)
#define ASSERT_ARGS_Parrot_cx_get_pmc_constant __attribute__unused__ int _ASSERT_ARGS_CHECK = \
       PARROT_ASSERT_ARG(interp) \
    || PARROT_ASSERT_ARG(ctx)
#define ASSERT_ARGS_Parrot_cx_get_recursion_depth __attribute__unused__ int _ASSERT_ARGS_CHECK = \
       PARROT_ASSERT_ARG(interp) \
    || PARROT_ASSERT_ARG(ctx)
#define ASSERT_ARGS_Parrot_cx_get_string_constant __attribute__unused__ int _ASSERT_ARGS_CHECK = \
       PARROT_ASSERT_ARG(interp) \
    || PARROT_ASSERT_ARG(ctx)
#define ASSERT_ARGS_Parrot_cx_inc_recursion_depth __attribute__unused__ int _ASSERT_ARGS_CHECK = \
       PARROT_ASSERT_ARG(interp) \
    || PARROT_ASSERT_ARG(ctx)
/* Don't modify between HEADERIZER BEGIN / HEADERIZER END.  Your changes will be lost. */
/* HEADERIZER END: src/context.c */

#endif
