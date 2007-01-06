/* sub.h
 *  Copyright (C) 2001-2003, The Perl Foundation.
 *  SVN Info
 *     $Id$
 *  Overview:
 *  Data Structure and Algorithms:
 *     Subroutine, coroutine, closure and continuation structures
 *     and related routines.
 *  History:
 *     Initial version by Melvin on on 2002/06/6
 *  Notes:
 *  References:
 */

#if !defined(PARROT_SUB_H_GUARD)
#define PARROT_SUB_H_GUARD

#include "parrot/parrot.h"

/*
 * Subroutine flags
 */
typedef enum {
     /* runtime usage flags */
    SUB_FLAG_CORO_FF      = PObj_private0_FLAG,
    SUB_FLAG_C_HANDLER    = PObj_private0_FLAG, /* C exceptions only */
    SUB_FLAG_TAILCALL     = PObj_private2_FLAG,
    SUB_FLAG_GENERATOR    = PObj_private3_FLAG, /* unused old python pmcs */

    /* compile/loadtime usage flags */
    /* from packfile */
    SUB_FLAG_IS_OUTER     = PObj_private1_FLAG,
    SUB_FLAG_PF_ANON      = PObj_private3_FLAG,
    SUB_FLAG_PF_MAIN      = PObj_private4_FLAG,
    SUB_FLAG_PF_LOAD      = PObj_private5_FLAG,
    SUB_FLAG_PF_IMMEDIATE = PObj_private6_FLAG,
    SUB_FLAG_PF_POSTCOMP  = PObj_private7_FLAG,

    SUB_FLAG_PF_MASK      = 0xfa   /* anon ... postcomp, is_outer*/
} sub_flags_enum;

#define SUB_FLAG_get_FLAGS(o) (PObj_get_FLAGS(o))
#define SUB_FLAG_flag_TEST(flag, o) (SUB_FLAG_get_FLAGS(o) & SUB_FLAG_ ## flag)
#define SUB_FLAG_flag_SET(flag, o) (SUB_FLAG_get_FLAGS(o) |= SUB_FLAG_ ## flag)
#define SUB_FLAG_flag_CLEAR(flag, o) (SUB_FLAG_get_FLAGS(o) &= ~(UINTVAL)(SUB_FLAG_ ## flag))

#define SUB_FLAG_flags_SETTO(o, f) SUB_FLAG_get_FLAGS(o) = (f)
#define SUB_FLAG_flags_CLEARALL(o) SUB_FLAG_flags_SETTO(o, 0)

#define SUB_FLAG_TAILCALL_TEST(o) SUB_FLAG_flag_TEST(TAILCALL, o)
#define SUB_FLAG_TAILCALL_ISSET(o) SUB_FLAG_flag_TEST(TAILCALL, o)
#define SUB_FLAG_TAILCALL_NOTSET(o) (!SUB_FLAG_flag_TEST(TAILCALL, o))
#define SUB_FLAG_TAILCALL_SET(o) SUB_FLAG_flag_SET(TAILCALL, o)
#define SUB_FLAG_TAILCALL_CLEAR(o) SUB_FLAG_flag_CLEAR(TAILCALL, o)


typedef enum {
    SUB_COMP_FLAG_BIT_0 = 1 << 0,
    SUB_COMP_FLAG_BIT_1 = 1 << 1,
    SUB_COMP_FLAG_BIT_2 = 1 << 2,
    SUB_COMP_FLAG_BIT_3 = 1 << 3,
    SUB_COMP_FLAG_BIT_4 = 1 << 4,
    SUB_COMP_FLAG_BIT_5 = 1 << 5,
    SUB_COMP_FLAG_BIT_6 = 1 << 6,
    SUB_COMP_FLAG_BIT_7 = 1 << 7,
    SUB_COMP_FLAG_BIT_8 = 1 << 8,
    SUB_COMP_FLAG_BIT_9 = 1 << 9,
    SUB_COMP_FLAG_PF_INIT = 1 << 10,
    SUB_COMP_FLAG_BIT_10 = 1 << 10,
    SUB_COMP_FLAG_BIT_11 = 1 << 11,
    SUB_COMP_FLAG_BIT_12 = 1 << 12,
    SUB_COMP_FLAG_BIT_13 = 1 << 13,
    SUB_COMP_FLAG_BIT_14 = 1 << 14,
    SUB_COMP_FLAG_BIT_15 = 1 << 15,
    SUB_COMP_FLAG_BIT_16 = 1 << 16,
    SUB_COMP_FLAG_BIT_17 = 1 << 17,
    SUB_COMP_FLAG_BIT_18 = 1 << 18,
    SUB_COMP_FLAG_BIT_19 = 1 << 19,
    SUB_COMP_FLAG_BIT_20 = 1 << 20,
    SUB_COMP_FLAG_BIT_21 = 1 << 21,
    SUB_COMP_FLAG_BIT_22 = 1 << 22,
    SUB_COMP_FLAG_BIT_23 = 1 << 23,
    SUB_COMP_FLAG_BIT_24 = 1 << 24,
    SUB_COMP_FLAG_BIT_25 = 1 << 25,
    SUB_COMP_FLAG_BIT_26 = 1 << 26,
    SUB_COMP_FLAG_BIT_27 = 1 << 27,
    SUB_COMP_FLAG_BIT_28 = 1 << 28,
    SUB_COMP_FLAG_BIT_29 = 1 << 29,
    SUB_COMP_FLAG_BIT_30 = 1 << 30,
    SUB_COMP_FLAG_BIT_31 = 1 << 31,
    SUB_COMP_FLAG_MASK   = 0x00000400,
} sub_comp_flags_enum;

#define Sub_comp_get_FLAGS(o) ((PMC_sub(o))->comp_flags)
#define Sub_comp_flag_TEST(flag, o) (Sub_comp_get_FLAGS(o) & SUB_COMP_FLAG_ ## flag)
#define Sub_comp_flag_SET(flag, o) (Sub_comp_get_FLAGS(o) |= SUB_COMP_FLAG_ ## flag)
#define Sub_comp_flag_CLEAR(flag, o) (Sub_comp_get_FLAGS(o) &= ~(UINTVAL)(SUB_COMP_FLAG_ ## flag))

#define Sub_comp_flags_SETTO(o, f) Sub_comp_get_FLAGS(o) = (f)
#define Sub_comp_flags_CLEARALL(o) Sub_comp_flags_SETTO(o, 0)

#define Sub_comp_INIT_TEST(o) Sub_comp_flag_TEST(PF_INIT, o)
#define Sub_comp_INIT_SET(o) Sub_comp_flag_SET(PF_INIT, o)
#define Sub_comp_INIT_CLEAR(o) Sub_comp_flag_CLEAR(PF_INIT, o)

union parrot_context_t;

/*
 * a flag to signal a Sub that a new RetContinuation should be created
 */

#define NEED_CONTINUATION ((void*)1)

/*
 * Sub and Closure share a Parrot_sub structure.
 * Closures have additionally an 'outer_ctx'
 */
typedef struct Parrot_sub {
    struct PackFile_ByteCode *seg;      /* bytecode segment */
    size_t   start_offs;        /* sub entry in ops from seg->base.data */
    size_t   end_offs;

    INTVAL   HLL_id;             /* see src/hll.c XXX or per segment? */
    PMC      *namespace;         /* where this Sub is in - this is either
                                  * a String or a [Key] and describes
                                  * the relative path in the NameSpace
                                  */
    PMC      *namespace_stash;   /* the actual hash, HLL::namespace */
    STRING   *name;              /* name of the sub */
    PMC      *multi_signature;   /* list of types for MMD */
    INTVAL   n_regs_used[4];     /* INSP in PBC */

    PMC      *lex_info;          /* LexInfo PMC */
    PMC      *outer_sub;         /* :outer for closures */
    PMC      *eval_pmc;          /* eval container / NULL */
    parrot_context_t *ctx;       /* the context this sub is in */
    UINTVAL  comp_flags;         /* compile time and additional flags */

    /* - end common */
    struct Parrot_Context *outer_ctx;   /* outer context, if a closure */
} * parrot_sub_t;

#define PMC_sub(pmc) ((parrot_sub_t)PMC_struct_val(pmc))

/* the first entries must match Parrot_sub, so we can cast
 * these two to the other type
 */
typedef struct Parrot_coro {
    struct PackFile_ByteCode *seg;      /* bytecode segment */
    size_t   start_offs;         /* sub entry in ops from seg->base.data */
    size_t   end_offs;

    INTVAL   HLL_id;             /* see src/hll.c XXX or per segment? */
    PMC      *namespace;         /* where this Sub is in - this is either
                                  * a String or a [Key] and describes
                                  * the relative path in the NameSpace
                                  */
    PMC      *namespace_stash;   /* the actual hash, HLL::namespace */
    STRING   *name;              /* name of the sub */
    PMC      *multi_signature;   /* list of types for MMD */
    INTVAL   n_regs_used[4];     /* INSP in PBC */

    PMC      *lex_info;          /* LexInfo PMC */
    PMC      *outer_sub;         /* :outer for closures */
    PMC      *eval_pmc;          /* eval container / NULL */
    struct Parrot_Context  *ctx; /* coroutine context */
    UINTVAL  comp_flags;         /* compile time and additional flags */

    /* - end common */

    struct PackFile_ByteCode *caller_seg;  /* bytecode segment */
    opcode_t *address;           /* next address to run - toggled each time */
    struct Stack_Chunk *dynamic_state; /* next dynamic state */
} * parrot_coro_t;

#define PMC_coro(pmc) ((parrot_coro_t)PMC_struct_val(pmc))

typedef struct Parrot_cont {
    /* continuation destination */
    struct PackFile_ByteCode *seg;   /* bytecode segment */
    opcode_t *address;               /* start of bytecode, addr to continue */
    struct Parrot_Context *to_ctx;   /* pointer to dest context */
    struct Stack_Chunk *dynamic_state; /* dest dynamic state */
    /* a Continuation keeps the from_ctx alive */
    struct Parrot_Context *from_ctx; /* sub, this cont is returning from */
    opcode_t *current_results;       /* ptr into code with get_results opcode
                                        full continuation only */
    int runloop_id;                  /* id of the creating runloop. */
} * parrot_cont_t;

#define PMC_cont(pmc) ((parrot_cont_t)PMC_struct_val(pmc))

struct Parrot_Context_info {
    STRING* subname;
    STRING* nsname;
    STRING* fullname;
    int pc;
    const char *file;
    int line;
    opcode_t *address;
};

struct Parrot_sub * new_sub(Interp * interp);
struct Parrot_sub * new_closure(Interp * interp);
struct Parrot_coro * new_coroutine(Interp * interp);
struct Parrot_cont * new_continuation(Interp * interp, struct Parrot_cont *to);
struct Parrot_cont * new_ret_continuation(Interp * interp);

PMC * new_ret_continuation_pmc(Interp *, opcode_t * address);

void mark_context(Interp *, parrot_context_t *);

void invalidate_retc_context(Interp *interp, PMC *cont);

STRING* Parrot_full_sub_name(Interp *interp, PMC* sub);
int Parrot_Context_info(Interp *interp, parrot_context_t *, struct Parrot_Context_info *);
STRING* Parrot_Context_infostr(Interp *interp, parrot_context_t *);

PMC* Parrot_find_pad(Interp*, STRING *lex_name, parrot_context_t *);
PMC* parrot_new_closure(Interp*, PMC*);

#endif /* PARROT_SUB_H_GUARD */

/*
 * Local variables:
 *   c-file-style: "parrot"
 * End:
 * vim: expandtab shiftwidth=4:
 */
