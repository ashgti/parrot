/*
 * Copyright (C) 2005-2007, Parrot Foundation.
 */

/*
 * jit_emit.h
 *
 * skeleton example - a stripped down jit/ppc/jit_emit.h
 *
 * $Id$
 */

#ifndef PARROT_JIT_SKELETON_JIT_EMIT_H_GUARD
#define PARROT_JIT_SKELETON_JIT_EMIT_H_GUARD

#  include <unistd.h>
#  include <limits.h>

/*
 * define all the available cpu registers
 * reserve some for special purposes
 */
typedef enum {
    r0,       /* r0 (reg #0) can't be mapped - use it as temp if possible */
    r1,
    r2,
    ARG1 = r3,      /* return value, first arg of function call */
    r4,
    r5,
    r6,
    r7,
    r8,
    r9,
    r10,
    r11,
    ISR1 = r11,         /* temp aka intermediate scratch reg 1 */
    r12,
    ISR2 = r12,         /* temp reg 2 */
    BP   = r13,         /* register base pointer (persistent) */
    OP_MAP = r14,       /* cached op_map (persistent) */
    CODE_START = r15,   /* cached code begint (persistent) */
    INTERP = r16,       /* cached interpreter register (persistent) */
    r17,
    ...
    r31
} arch_iregister_t;

/*
 * If your arch doesn't have that much register available, you
 * don't cache OP_MAP, CODE_START, and INTERP in a cpu register.
 * Only BP (the register base pointer is needed and 1 or 2 temp regs
 */


/*
 * define, which register is the register base pointer
 */

#  define Parrot_jit_emit_get_base_reg_no(pc) BP

/*
 * define floating point register too, if there are some
 */
typedef enum {
    f0,       /* f0 (reg #0) can't be mapped - use it as temp if possible */
    FSR1 = f0,
    f1,
    ...
    f13,
    FSR2 = f13,
    ...
    f31
} arch_fregister_t;

/*
 * now define macros for all possible (and implemented) operations
 *
 * Parrot defines JIT_EMIT to 1 or 2, when this file is included in
 * exec_cpu.c or jit_cpu.c
 */

#if JIT_EMIT

/*
 * the arch might have different branch types:
 * long offset, short offset, absolute, ...
 */
enum { JIT_ARCH_CALL, JIT_ARCH_BRANCH, JIT_ARCH_UBRANCH };

/*
 * emit code for a register move, pc is the location to emit the
 * asm code
 */
#  define jit_emit_mov_rr(pc, dst, src) ...

#  define jit_emit_add_rrr(pc, D, A, B) ...

/*
 * load register from memory offset, relative to the register base pointer
 * this is used to load hardware cpu registers from parrot registers
 */
#  define jit_emit_mov_rm_i(pc, reg, offs) \
      jit_emit_lwz((pc), (reg), (offs), BP)     /* e.g. PPC */

/* load floating point register from Parrot register */
#  define jit_emit_mov_rm_n(pc, reg, offs) \
      jit_emit_lfd((pc), (reg), (offs), BP)     /* e.g. PPC */

/* Store a CPU register back to a Parrot register. */

#  define jit_emit_mov_mr_i(pc, offs, reg) \
      jit_emit_stw((pc), (reg), (offs), BP)

#  define jit_emit_mov_mr_n(pc, offs, reg) \
      jit_emit_stfd((pc), (reg), (offs), BP)

/*
 * emit a branch and remember the branch target for code fixup,
 * which id done, when all code is emitted
 */
static void
jit_emit_bc(Parrot_jit_info_t *jit_info, branch_t cond, opcode_t disp)
{
    /* see other architectures */
}


/*
 * Load a 32-bit immediate value.
 */

#  define jit_emit_mov_ri_i(pc, D, imm)   ...

/*
 * define some helper macros for code generation
 * Parrot_jit_normal_op() is: create code that does the equivalent of:
 *
 * PC = ((INTERP->op_func_table)[*PC])(PC,INTERP)
 *
 * First we need to calculate the PC at runtime by adding disp to
 * to the cached CODE_START register 'add_disp' in jit/ppc
 */

#  define add_disp(pc, D, disp) \
      jit_emit_mov_ri_i((pc), ISR1, (disp)); \
      jit_emit_add_rrr((pc), (D), CODE_START, ISR1)

/*
 * emit code that gets interp->code->jit_info->arena->op_map
 * and sets the OP_MAP register
 */
#  define jit_emit_load_op_map(pc) ...

/*
 * emit code that gets interp->code->base.data
 * and sets the CODE_START register
 */
#  define jit_emit_load_code_start(pc)  ...

/*
 * emit code that branches to the next code piece
 */
#  define jit_emit_branch_to_opcode(pc, D)  ...

/*
 * emit code that calls a Parrot opcode function
 */
#  define jit_emit_call_func(pc, addr)  ...

#endif /* JIT_EMIT */

#if JIT_EMIT == 2

/*
 * emit code that calls a core.ops function from src/core_ops.c,
 * the generated code is the translation of this:
 *
 *  PC = ((INTERP->op_func_table)[*PC])(PC,INTERP)
 */
void
Parrot_jit_normal_op(Parrot_jit_info_t *jit_info,
                     PARROT_INTERP)
{
}

/*
 * emit code for a branching parrot opcode. All cached registers
 * need recalculation, as a branch can go into different code segments
 * with different code start and different jit_info
 */
void
Parrot_jit_cpcf_op(Parrot_jit_info_t *jit_info,
                   PARROT_INTERP)
{
    Parrot_jit_normal_op(jit_info, interp);

    /* fix our reserved registers,
     * in case we are branching to a new segment
     */
    jit_emit_load_op_map(jit_info->native_ptr);
    jit_emit_load_code_start(jit_info->native_ptr);

    /* branch to the opcode just returned from the normal_op call ARG1 */
    jit_emit_branch_to_opcode(jit_info->native_ptr, ARG1);
}

/*
 * release stack frame end exit see core.jit
 */
static void Parrot_end_jit(Parrot_jit_info_t *, Interp *);

#  undef Parrot_jit_restart_op
/*
 * emit code that might leave the JIT runcore
 * see  ppc or i386
 */
void
Parrot_jit_restart_op(Parrot_jit_info_t *jit_info,
                      PARROT_INTERP)
{
}

#endif /* JIT_EMIT == 2 */

#if JIT_EMIT == 0

/*
 * emit stack frame according to ABI
 * preserve mapped registers according to ABI
 * load INTERP, OP_MAP, CODE_START, BP registers
 * then run the code at pc
 *
 * the function is called as
 *   runops(interp, pc)
 *
 * at runtime
 */
void
Parrot_jit_begin(Parrot_jit_info_t *jit_info,
                 PARROT_INTERP)
{
    ...
}
/*
 * fix up all emitted branches
 * see  ppc or i386
 */
static void
Parrot_jit_dofixup(Parrot_jit_info_t *jit_info,
                   PARROT_INTERP)
{
}

/*
 * define interface functions for register -> parrot register moves
 * and v.v.
 *
 * see ppc or x86
 */

static void
jit_mov_mr_offs(...) {}
static void
jit_mov_rm_offs(...) {}

static void
jit_mov_mr_n_offs(...) {}
static void
jit_mov_rm_n_offs(...) {}

/*
 * define how many int and float registers can be used by the
 * jit core
 */

#  define INT_REGISTERS_TO_MAP 14
#  define FLOAT_REGISTERS_TO_MAP 2

/*
 * enumerate these mapped registers
 * please note that you have to preserve registers in
 * Parrot_jit_begin according to the ABI of the architecture
 */

static const char intval_map[INT_REGISTERS_TO_MAP] =

    { r17, r18, r19, r20, r21, r22, r23,
      r24, r25, r26, r27, r28, r29, r30
    };

static const char floatval_map[FLOAT_REGISTERS_TO_MAP] =
    {

      f4, f5
    };

/*
 * you might need a function that flushes hardware caches after
 * JIT compilation is done
 */
static void
ppc_flush_cache(Parrot_jit_info_t * jit_info, Interp *i)
{
    ...
}

/*
 * define arch specific details in jit_arch_info
 */

static const jit_arch_info arch_info = {
    jit_mov_rm_offs,
    jit_mov_rm_n_offs,
    jit_mov_mr_offs,
    jit_mov_mr_n_offs,
    Parrot_jit_dofixup,
    ppc_flush_cache,
    {
        /* JIT_CODE_FILE */
        {
            Parrot_jit_begin,   /* emit code prologue */
            INT_REGISTERS_TO_MAP,   /* mapped ints */
            INT_REGISTERS_TO_MAP,   /* all are preserved */
            intval_map,
            FLOAT_REGISTERS_TO_MAP,    /* mapped float regs */
            FLOAT_REGISTERS_TO_MAP,    /* all preserved */
            floatval_map
        },
        /* JIT_CODE_SUB */
        {
            Parrot_jit_begin_sub,   /* emit code prologue */
            7,                  /* 7 mapped ints */
            7,                  /* all volatile */
            intval_map_sub,
            12,                  /* mapped float regs */
            12,                  /* all volatile */
            floatval_map_sub
        },
        /* JIT_CODE_SUB_REGS_ONLY */
        {
            Parrot_jit_begin_sub_regs,  /* emit code prologue */
            7,                  /* 7 mapped ints */
            7,                  /* all volatile */
            intval_map_sub,
            12,                  /* 12 mapped float regs */
            12,                  /* all volatile */
            floatval_map_sub
        }
    }
};

/*
 * and finally you need an interface function to return above structure
 */

PARROT_WARN_UNUSED_RESULT
PARROT_CANNOT_RETURN_NULL
const jit_arch_info *
Parrot_jit_init(PARROT_INTERP)
{
    return &arch_info;
}


#endif /* JIT_EMIT == 0 */
#endif /* PARROT_JIT_SKELETON_JIT_EMIT_H_GUARD */


/*
 * Local variables:
 *   c-file-style: "parrot"
 * End:
 * vim: expandtab shiftwidth=4:
 */
