/*
Copyright (C) 2001-2009, Parrot Foundation.
$Id$

=head1 NAME

src/jit.c - JIT

=head1 DESCRIPTION

JIT (Just In Time) compilation converts bytecode to native machine code
instructions and executes the generated instruction sequence directly.

Actually it's not really just in time, it's just before this piece of code is
used and not per subroutine or even opcode, it works per bytecode segment.

=head2 Functions

=over 4

=cut

*/

/* HEADERIZER HFILE: none */
/* HEADERIZER STOP */

#include <parrot/parrot.h>
#if PARROT_EXEC_CAPABLE
#  include "parrot/exec.h"
#endif
#include "jit.h"
#define JIT_EMIT 0
#include "jit_emit.h"
#include "parrot/packfile.h"
#include "parrot/oplib/ops.h"
#include "pmc/pmc_sub.h"
#include "pmc/pmc_managedstruct.h"

#define JIT_SEGS 0

extern int jit_op_count(void);
/*
 * s. jit/$jitcpuarch/jit_emit.h for the meaning of these defs
 */

#ifndef ALLOCATE_REGISTERS_PER_SECTION
#  define ALLOCATE_REGISTERS_PER_SECTION 1
#endif

#if defined __GNUC__ || defined __IBMC__
void Parrot_jit_debug(PARROT_INTERP);
#endif

/*

=item C<static void
insert_fixup_targets(PARROT_INTERP, char *branch,
        size_t limit)>

Look at fixups, mark all fixup entries as branch target.

TODO: actually this is wrong: fixups belong only to one code segment.
The code below doesn't check, for which segments the fixups are
inserted.

=cut

*/

static void
insert_fixup_targets(PARROT_INTERP, char *branch,
        size_t limit)
{
    PackFile_FixupTable *ft = interp->code->fixups;
    int i;

    if (!ft)
        return;

    for (i = 0; i < ft->fixup_count; i++) {
        if (ft->fixups[i]->type == enum_fixup_label) {
            if ((size_t)ft->fixups[i]->offset < limit)
                branch[ft->fixups[i]->offset] |= JIT_BRANCH_TARGET;
        }
    }
}

/*

=item C<static void
make_branch_list(PARROT_INTERP,
        Parrot_jit_optimizer_t * optimizer,
        opcode_t *code_start, opcode_t *code_end)>

C<< optimizer->map_branch >> parallels the opcodes with a list of
branch information and register mapping information

=over 4

=item branch instructions have C<JIT_BRANCH_SOURCE>

=item opcodes jumped to have C<JIT_BRANCH_TARGET>

=item mapped arguments have register type + 1 and finally

=item after register allocation these have the processor register that
got mapped

=back

=cut

*/

static void
make_branch_list(PARROT_INTERP,
        Parrot_jit_optimizer_t * optimizer,
         opcode_t *code_start, opcode_t *code_end)
{
    op_info_t *op_info;
    char *branch;
    opcode_t *cur_op;

    cur_op = code_start;

    /* Allocate space for the branch information and register map */
    optimizer->map_branch = branch =
        (char *)mem_sys_allocate_zeroed((size_t)(code_end - code_start + 1));

    /* Allocate space for the branch list */
    optimizer->branch_list = (opcode_t **)
        mem_sys_allocate_zeroed(
                (size_t)(code_end - code_start) * sizeof (opcode_t *));

    /* If the opcode jumps we may:
     *
     * PARROT_JUMP_RELATIVE:
     *      The op jumps to an address relative to the current position,
     *      thus we mark the branch target and the branch source.
     *
     * PARROT_JUMP_ADDRESS:
     *      The op jumps to an absolute address, thus we mark the branch
     *      target.
     *
     * PARROT_JUMP_POP:
     *      The op pops the address to jump to, thus we don't mark the
     *      branch target, anyway it may probably use expr(NEXT)
     *
     * PARROT_JUMP_ENEXT:
     *      The op does something with expr(NEXT),
     *      XXX I'll assume that it's stored in the control stack for
     *          later returning since that's the only way it's used now
     *          but this should go away by the time we add some metadata
     *          to the ops.
     *      So we will mark the branch target.
     *
     * PARROT_JUMP_GNEXT:
     *      Means the opcode does some other kind of jump, and also
     *      might goto(NEXT)
     *
     * PARROT_JUMP_UNPREDICTABLE:
     *      The branch target is unpredictable.
     *      Things get a little tricky since it's not 100% true that the
     *      target is unpredictable because of the set_addr opcode, we
     *      need to find a solution for this, in the mean time, we will
     *      make each section have its own arena and try to avoid
     *      going in and out from them as much as possible.
     *
     *  PARROT_JUMP_RESTART
     *      If the parrot program counter is zero, fall out of the
     *      run loop.
     *
     */

    while (cur_op < code_end) {
        opcode_t op = *cur_op;
        int i, n;
        size_t rel_offset;


        /* Predereference the opcode information table for this opcode
         * early since it's going to be used many times */
        op_info = &interp->op_info_table[op];

        /* if op_info->jump is not 0 this opcode may jump,
         * so mark this opcode as a branch source */
        rel_offset = cur_op - code_start;

        n = op_info->op_count;

        if (op == PARROT_OP_set_args_pc ||
                op == PARROT_OP_set_returns_pc ||
                op == PARROT_OP_get_results_pc)
            goto no_branch;
        if (op_info->jump)
            branch[rel_offset] |= JIT_BRANCH_SOURCE;
        for (i = 1; i < n; ++i) {
            /* If it's not a constant, no joy */
            if (op_info->types[i-1] == PARROT_ARG_IC && op_info->labels[i-1]) {
                /* The branch target is relative,
                 * the offset is in the i argument
                 */
                if (op_info->jump & PARROT_JUMP_RELATIVE) {
                    /* Set the branch target */
                    optimizer->branch_list[rel_offset] = cur_op + cur_op[i];
                    branch[rel_offset + cur_op[i]] |= JIT_BRANCH_TARGET;
                }
                /* The branch target is absolute,
                 * the address is in the i argument
                 */
                else if (op_info->jump & PARROT_JUMP_ADDRESS) {
                    /* Set the branch target */
                    optimizer->branch_list[rel_offset] = cur_op + cur_op[i];
                    branch[cur_op[i]] |= JIT_BRANCH_TARGET;
                }
                /* the labels of set_addr and newsub are branch targets too
                 * this is needed e.g. for JIT_CGP
                 */
                else {
                    branch[rel_offset + cur_op[i]] |= JIT_BRANCH_TARGET;
                }
            }
        }
        /* The address of the next opcode */
        if ((op_info->jump & PARROT_JUMP_ENEXT) ||
            (op_info->jump & PARROT_JUMP_GNEXT))
            branch[rel_offset + n] |= JIT_BRANCH_TARGET;
        if (op_info->jump & PARROT_JUMP_UNPREDICTABLE) {
            /*
             * TODO
             *   this flag is currently not used or set
             *   and: if we have a branch that isn't going to a constant
             *   target like a calculated branch used by rx_ opcodes
             *   we are totally lost WRT register preservation.
             *   If we don't know, that the code is a branch target, inside
             *   a JITted code section, mapped registers might be
             *   not up to date WRT Parrot registers.
             */
            optimizer->has_unpredictable_jump = 1;
        }
no_branch:
        /* Move to the next opcode */
        ADD_OP_VAR_PART(interp, interp->code, cur_op, n);
        cur_op += n;
    }
    insert_fixup_targets(interp, branch, code_end - code_start);
}

/*

=item C<static void
set_register_usage(PARROT_INTERP,
        Parrot_jit_info_t *jit_info,
        Parrot_jit_optimizer_section_ptr cur_section,
        op_info_t *op_info, opcode_t *cur_op, opcode_t *code_start)>

Sets the register usage counts.

=cut

*/

static void
set_register_usage(PARROT_INTERP,
        Parrot_jit_info_t *jit_info,
        Parrot_jit_optimizer_section_ptr cur_section,
        op_info_t *op_info, opcode_t *cur_op, opcode_t *code_start)
{
    int argn, args, argt;
    int typ;
    Parrot_jit_register_usage_t *ru = cur_section->ru;
    Parrot_jit_optimizer_t * optimizer = jit_info->optimizer;
    char * const map = optimizer->map_branch;

    /* For each argument that has the opcode increment the usage count,
     * We move from the end since we need to check if the first opcode
     * using the register will read or write it.
     *
     * registers are set per their type [IPSN]
     * */
    args = argt = op_info->op_count;
    ADD_OP_VAR_PART(interp, interp->code, cur_op, argt);
    for (argn = argt - 1; argn > 0; argn--) {
        /* TODO check the argn-1 entries */
        int idx = *(cur_op + argn);
        int arg_type;
        PMC *sig;
        if (argn >= args) {
            sig = Parrot_pcc_get_pmc_constant(interp, CURRENT_CONTEXT(interp), cur_op[1]);
            arg_type = VTABLE_get_integer_keyed_int(interp,
                    sig, argn - args);
            arg_type &= (PARROT_ARG_TYPE_MASK | PARROT_ARG_CONSTANT);
        }
        else
            arg_type = op_info->types[argn - 1];

        switch (arg_type) {
            case PARROT_ARG_I:
            case PARROT_ARG_KI:
                typ = 0;
                /*
                 * if the register number is negative, the register mapping
                 * was done by imcc/jit.c, which used negative numbers
                 * for allocated CPU registers. That's currently not
                 * functional because of changed register allocation
                 * strategy inside imcc.
                 * The code is still here and should probably be reactivated
                 * later, when things are stable: imcc has all the
                 * necessary information like basic blocks and loop depth
                 * calculated already. A lot is duplicated here to regain this
                 * information.
                 */
                if (idx < 0)
                    idx = -1 - idx;
                break;
            case PARROT_ARG_P:
            case PARROT_ARG_K:
                /*
                 * P and S regs aren't currently used at all. That's not
                 * really optimal. If we have plenty of mappable registers
                 * and if we can call vtables or MMD functions directly
                 * we should finally allocate P and S regs too.
                 */
                typ = 1;
                break;
            case PARROT_ARG_S:
                typ = 2;
                break;
            case PARROT_ARG_N:
                if (idx < 0)
                    idx = -1 - idx;
                typ = 3;
                break;
            default:
                typ = -1;
                break;
        }
        /*
         * JIT structures are NUM_REGISTERS big
         * we can currently allocate only that much
         */
        if (typ >= 0 && idx < NUM_REGISTERS) {
            /* remember the register typ (+1) for this op argument
             * for register allocation */
            map[cur_op + argn - code_start] = typ + 1;
            if ((!ru[typ].reg_count[idx]++) &&
                (op_info->dirs[argn-1] & PARROT_ARGDIR_IN))
                ru[typ].reg_dir[idx] |= PARROT_ARGDIR_IN;
            if (op_info->dirs[argn-1] & PARROT_ARGDIR_OUT) {
                ru[typ].reg_dir[idx] |= PARROT_ARGDIR_OUT;
            }
        }
        /* key constants may have register keys */
        else if (arg_type == PARROT_ARG_KC) {
            PMC *key = interp->code->const_table->constants[idx]->u.key;
            while (key) {
                const UINTVAL flags = PObj_get_FLAGS(key);
                if (flags & KEY_register_FLAG) {
                    INTVAL n = 0;
                    if (flags & KEY_integer_FLAG) {
                        n = VTABLE_get_integer(interp, key);
                        typ = 0;
                        if (n < 0)
                            n = -1 - n;
                    }
                    else if (flags & KEY_pmc_FLAG)
                        typ = 1;
                    else if (flags & KEY_string_FLAG)
                        typ = 2;

                    if (n < NUM_REGISTERS && !ru[typ].reg_count[n]++)
                        ru[typ].reg_dir[n] |= PARROT_ARGDIR_IN;
                }
                key = VTABLE_shift_pmc(interp, key);
            }
        }
    }
}

/*

=item C<static void
init_regusage(PARROT_INTERP,
        Parrot_jit_optimizer_section_ptr cur_section)>

Init all register usage to Parrot register usage. Used when JITting
subroutines to registers only

=cut

*/

static void
init_regusage(PARROT_INTERP, Parrot_jit_optimizer_section_ptr cur_section)
{
    int typ;

    cur_section->ru[0].registers_used = Parrot_pcc_get_regs_used(interp,
                                            CURRENT_CONTEXT(interp), REGNO_INT);
    cur_section->ru[3].registers_used = Parrot_pcc_get_regs_used(interp,
                                            CURRENT_CONTEXT(interp), REGNO_NUM);
    cur_section->ru[1].registers_used = cur_section->ru[2].registers_used = 0;

    for (typ = 0; typ < 4; typ++) {
        int j;
        for (j = 0; j < cur_section->ru[typ].registers_used; j++)
            cur_section->ru[typ].reg_usage[j] = j;
    }
}

/*

=item C<static void make_sections(PARROT_INTERP,
        Parrot_jit_info_t *jit_info,
        opcode_t *code_start, opcode_t *code_end)>

I386 has JITed vtables, which have the vtable# in extcall.

This C<Parrot_jit_vtable_n_op()> does use register mappings.

=cut

*/

#ifndef EXTCALL
#  define EXTCALL(op) (op_jit[(op)].extcall >= 1 || (op) >= jit_op_count())
#  define CALLS_C_CODE(op) (op_func[(op)].extcall == -1)
#endif

static void
make_sections(PARROT_INTERP,
        Parrot_jit_info_t *jit_info,
         opcode_t *code_start, opcode_t *code_end)
{
    Parrot_jit_optimizer_section_ptr cur_section, t_section, prev_section;
    opcode_t *next_op;
    op_info_t *op_info;
    char *branch;
    int branched, start_new;
    opcode_t *cur_op;
    Parrot_jit_optimizer_t * optimizer;

    optimizer = jit_info->optimizer;
    branch = optimizer->map_branch;

    /* Allocate the first section */
    cur_section = optimizer->sections = (Parrot_jit_optimizer_section_t *)
        mem_sys_allocate_zeroed(sizeof (Parrot_jit_optimizer_section_t));
    cur_section->begin = code_start;
    prev_section = cur_section;

    cur_op = code_start;
    /* set all regs to Parrot's */
    if (jit_info->code_type == JIT_CODE_SUB_REGS_ONLY)
        init_regusage(interp, cur_section);
    while (cur_section) {
        opcode_t op = *cur_op;
        branched = start_new = 0;
        /* Predereference the opcode information for this opcode
         * early since it's going to be used many times */
        op_info = &interp->op_info_table[op];

        /* Calculate the next pc */
        next_op = cur_op + op_info->op_count;
        ADD_OP_VAR_PART(interp, interp->code, cur_op, next_op);

        /* Update op_count */
        cur_section->op_count++;

        /* set register usage for this section */
        set_register_usage(interp, jit_info, cur_section,
                op_info, cur_op, code_start);

        /*
         * End a section:
         * If this opcode is jitted and next is a C function */
        if (!EXTCALL(op)) {
            cur_section->jit_op_count++;

            if (next_op < code_end && EXTCALL(*next_op))
                start_new = 1;
        }
        else
            /* or if current section is not jitted, and the next opcode
             * is. */
            if (next_op < code_end && !EXTCALL(*next_op))
                start_new = 1;

        /* or when the current opcode is a branch source,
         * in other words if the opcode jumps, or if the next opcode is
         * a branch target, allocate a new section only if it's not the
         * last opcode */
        if ((branch[cur_op - code_start] & JIT_BRANCH_SOURCE)
                || (next_op < code_end &&
                    (branch[next_op - code_start] & JIT_BRANCH_TARGET))
                || (next_op >= code_end)) {
            /* remember to start a new block */
            branched = 1;
            start_new = 1;
        }

        if (start_new) {
            /* Set the type, depending on whether the current
             * instruction is external or jitted. */
            cur_section->isjit = !EXTCALL(op);

            /* Save the address where the section ends */
            cur_section->end = cur_op;

            if (next_op < code_end) {
                /* Allocate a new section */
                t_section = (Parrot_jit_optimizer_section_t *)
                    mem_sys_allocate_zeroed(
                            sizeof (Parrot_jit_optimizer_section_t));
                /* Add it to the double linked list */
                cur_section->next = t_section;
                t_section->prev = cur_section;
                /* Make the new section be the current one */
                cur_section = t_section;
                /* set all regs to Parrot's */
                if (jit_info->code_type == JIT_CODE_SUB_REGS_ONLY)
                    init_regusage(interp, cur_section);

                /* registers get either allocated per section or
                 * per basic block (i.e. one or more sections divided
                 * by branches. When allocation per block is done
                 * all sections in one block have the same block number
                 */
                if (ALLOCATE_REGISTERS_PER_SECTION || branched) {
                    cur_section->block = prev_section->block + 1;
                    prev_section = cur_section;
                }
                else
                    cur_section->block = prev_section->block;
                /* Save the address where the section begins */
                cur_section->begin = next_op;
            }
            else {
                cur_section = NULL;
            }
        }

        /* Move to the next opcode */
        cur_op = next_op;
    }
}

/*

=item C<static void
make_branch_targets(
        Parrot_jit_optimizer_t *optimizer, opcode_t * code_start)>

Makes the branch targets.

=cut

*/

static void
make_branch_targets(Parrot_jit_optimizer_t *optimizer, const opcode_t * code_start)
{
    Parrot_jit_optimizer_section_ptr cur_section, t_section;
    /* Set the branch target of this section, that is the section where
     * the program execution continues, if it ends in a branch source we
     * use the branch target and not the next section. */
    cur_section = optimizer->sections;
    while (cur_section) {
        if (optimizer->branch_list[cur_section->end - code_start]) {
            /* If the branch target is to a section before the current one
             * move from the start, otherwise from the current section */
            if (optimizer->branch_list[cur_section->end - code_start] <
                    cur_section->begin)
                t_section = optimizer->sections;
            else
                t_section = cur_section;

            while (t_section) {
                /* If we find the section attach it to the current one. */
                if (t_section->begin ==
                        optimizer->branch_list[cur_section->end - code_start]) {
                    cur_section->branch_target = t_section;
                    break;
                }
                /* If not move to the next. */
                t_section = t_section->next;
            }

        }
        /* Move to the next section */
        cur_section = cur_section->next;
    }
}

/*

=item C<static void
sort_registers(Parrot_jit_info_t *jit_info)>

Sorts the Parrot registers prior to mapping them to actual hardware registers.

=cut

*/

static void
sort_registers(Parrot_jit_info_t *jit_info)
{
    Parrot_jit_optimizer_t *optimizer;
    Parrot_jit_optimizer_section_ptr cur_section, next;
    int any, k, typ, code_type;
    int max_count, max_i = 0;
    int to_map[] = { 0, 0, 0, 0 };

    code_type = jit_info->code_type;
    to_map[0] = jit_info->arch_info->regs[code_type].n_mapped_I;
    to_map[3] = jit_info->arch_info->regs[code_type].n_mapped_F;

    optimizer = jit_info->optimizer;
    /* Start from the first section */
    cur_section = optimizer->sections;

    while (cur_section) {
        Parrot_jit_register_usage_t *ru = cur_section->ru;
        /* sum up register usage for one block, don't change
         * reg_dir. If allocation is done per section, block numbers
         * are different, so this is a nop
         */
        next = cur_section->next;
        while (next && next->block == cur_section->block) {
            const Parrot_jit_register_usage_t * const nru = next->ru;
            for (typ = 0; typ < 4; typ++) {
                int i;
                for (i = 0; i < NUM_REGISTERS; i++)
                    ru[typ].reg_count[i] += nru[typ].reg_count[i];
            }
            next = next->next;
        }

        /* now sort registers by their usage count */
        for (typ = 0; typ < 4; typ++) {
            /* find most used register */
            int i;
            for (i = max_count = 0; i < NUM_REGISTERS; i++) {
                if (cur_section->ru[typ].reg_count[i] > max_count) {
                    max_count = cur_section->ru[typ].reg_count[i];
                    max_i = i;
                }
            }
            /* start from this register and set usage */
            k = ru[typ].registers_used = 0;
            /* no usage, go on with next type */
            if (max_count == 0 || !to_map[typ])
                continue;
            /* as long as we map registers for this typ */
            while (1) {
                if (max_i >= 0)
                    ru[typ].reg_usage[k++] = max_i;
                /* all mapped? */
                if (k == to_map[typ])
                    break;
                /* now check for equal usage starting after maxi */
                for (any = 0, i = max_i + 1; i < NUM_REGISTERS; i++) {
                    if (ru[typ].reg_count[i] == max_count) {
                        max_i = i;
                        any = 1;
                        break;
                    }
                }
                /* if same usage not found, look for lower usage */
                if (any == 0) {
                    if (max_count > 1) {
                        max_count--;
                        max_i = -1;
                        continue;
                    }
                    break;
                }
            }
            ru[typ].registers_used = k;
        }
        next = cur_section->next;
        /* duplicate usage to all sections of block */
        while (next && next->block == cur_section->block) {
            Parrot_jit_register_usage_t * const nru = next->ru;
            for (typ = 0; typ < 4; typ++) {
                int i;
                for (i = 0; i < ru[typ].registers_used; i++) {
                    nru[typ].reg_count[i] = ru[typ].reg_count[i];
                    nru[typ].reg_usage[i] = ru[typ].reg_usage[i];
                }
                nru[typ].registers_used = ru[typ].registers_used;
            }
            next = next->next;
        }
        /* Move to the next section */
        cur_section = next;
    }
}

/*

=item C<static void
assign_registers(PARROT_INTERP,
        Parrot_jit_info_t *jit_info,
        Parrot_jit_optimizer_section_ptr cur_section,
        opcode_t * code_start, int from_imcc)>

Called by C<map_registers()> to actually assign the Parrot registers to
hardware registers.

TODO

Before actually assigning registers, we should optimize a bit:

1) calculate max use count of register types for all sections

2) calculate costs for register preserving and restoring
   for two different allocation strategies:

   a) allocate non-volatiles first
      overhead for jit_begin, jit_end:
      - 2 * max_used_non_volatile registers
      overhead for register preserving around non-jitted sections:
      - only used IN arguments are saved
      - only OUT non-volatile arguments are restored
   b) allocate volatiles first
      no overhead for jit_begin, jit_end
      overhead per JITed op that calls a C function:
      - 2 * n_used_volatiles_to_preserve for each call
      overhead for register preserving around non-jitted sections:
      - all volatiles are saved and restored around non-jitted sections

NB for all cost estimations size does matter: a 64bit double counts as
   two 32bit ints. Opcode count is assumed to be just one.

3) depending on costs from 2) use one of the strategies
   That does still not account for any usage patterns. Imcc has loop
   nesting depth, but that's not available here. OTOH smaller code tends
   to perform better because of better cache usage.

Usage analysis could show that a mixture of both strategies is best, e.g:
allocate 2-4 non-volatiles and the rest from volatiles. But that would
complicate the allocation code a bit.

=cut

*/

static void
assign_registers(PARROT_INTERP,
        Parrot_jit_info_t *jit_info,
        Parrot_jit_optimizer_section_ptr cur_section,
        opcode_t * code_start, int from_imcc)
{
    char *map;
    Parrot_jit_optimizer_t *optimizer;
    int i;
    opcode_t * cur_op;
    const char * maps[] = {0, 0, 0, 0};
    const int code_type = jit_info->code_type;

    maps[0] = jit_info->arch_info->regs[code_type].map_I;
    maps[3] = jit_info->arch_info->regs[code_type].map_F;

    optimizer = jit_info->optimizer;
    map = optimizer->map_branch;
    /* For each opcode in this section */
    cur_op = cur_section->begin;
    while (cur_op <= cur_section->end) {
        const opcode_t op = *cur_op;
        const op_info_t * const op_info = &interp->op_info_table[op];
        int op_arg;
        int n;

        /* For each argument of the current opcode */
        n = op_info->op_count;
        ADD_OP_VAR_PART(interp, interp->code, cur_op, n);
        for (op_arg = 1; op_arg < n; op_arg++) {
            /* get the register typ */
            int typ = map[cur_op + op_arg - code_start];
            /* clear the register typ/map */
            map[cur_op + op_arg - code_start] = 0;
            /* if not JITted, don't map */
            if (!cur_section->isjit)
                continue;
            if (typ > 0) {
                typ--;  /* types are + 1 */
                if (!maps[typ])
                    continue;
                /* If the argument is in most used list for this typ */
                for (i = 0; i < cur_section->ru[typ].registers_used; i++) {
                    opcode_t idx = cur_op[op_arg];
                    if (from_imcc)
                        idx = -1 - idx;
                    if (idx == (opcode_t)cur_section->ru[typ].reg_usage[i]) {
                        map[cur_op + op_arg - code_start] = maps[typ][i];
                        cur_section->maps++;
                        break;
                    }
                }
            }
        }

        /* Move to the next opcode */
        cur_op += n;
    }
}

/*

=item C<static void
map_registers(PARROT_INTERP,
        Parrot_jit_info_t *jit_info, opcode_t * code_start)>

Maps the most used Parrot registers to hardware registers.

=cut

*/

static void
map_registers(PARROT_INTERP,
        Parrot_jit_info_t *jit_info,
         opcode_t * code_start)
{
    /* Start from the first section */
    Parrot_jit_optimizer_section_ptr cur_section = jit_info->optimizer->sections;

    /* While there is section */
    while (cur_section) {

        assign_registers(interp, jit_info, cur_section, code_start, 0);

        /* Move to the next section */
        cur_section = cur_section->next;
    }
}


#define JIT_DEBUG 0

#if JIT_DEBUG
/*

=item C<static void
debug_sections(PARROT_INTERP,
        Parrot_jit_optimizer_t *optimizer, opcode_t * code_start)>

Prints out debugging info.

=cut

*/

static void
debug_sections(PARROT_INTERP,
        Parrot_jit_optimizer_t *optimizer, opcode_t * code_start)
{
    Parrot_jit_optimizer_section_ptr cur_section;
#  if JIT_DEBUG > 1
    char * map = optimizer->map_branch;
#  endif
    int n;
    const char types[] = "IPSN";
    int types_to_list[] = {0, 3};

    cur_section = optimizer->sections;
    while (cur_section) {
        opcode_t * cur_op;
        unsigned int j;
        Parrot_jit_register_usage_t *ru = cur_section->ru;

        Parrot_io_eprintf(interp, "\nSection:\n");
        Parrot_io_eprintf(interp, "%s block %d\n",
                (cur_section->isjit) ? "JITTED" : "NOT JITTED",
                cur_section->block);
        for (cur_op = cur_section->begin; cur_op <= cur_section->end;) {
            char instr[256];
            const opcode_t op = *cur_op;
            const op_info_t * const op_info = &interp->op_info_table[op];

            PDB_disassemble_op(interp, instr, sizeof (instr),
                    op_info, cur_op, NULL, code_start, 0);
            Parrot_io_eprintf(interp, "\t\tOP%vu: ext %3d\t%s\n",
                    cur_op - code_start, op_jit[*cur_op].extcall, instr);
            n = op_info->op_count;
            ADD_OP_VAR_PART(interp, interp->code, cur_op, n);
#  if JIT_DEBUG > 1
            Parrot_io_eprintf(interp, "\t\t\tmap_branch: ");
            for (i = 0; i < n; i++)
                Parrot_io_eprintf(interp, "%02x ", map[cur_op-code_start+i]);
            Parrot_io_eprintf(interp, "\n");
#  endif
            cur_op += n;
        }
        Parrot_io_eprintf(interp, "\tbegin:\t%#p\t(%Ou)\n",
                cur_section->begin, *cur_section->begin);
        Parrot_io_eprintf(interp, "\tend:\t%#p\t(%Ou)\n",
                cur_section->end, *cur_section->end);

        for (j = 0; j < sizeof (types_to_list)/sizeof (int); j++) {
            const int typ = types_to_list[j];
            const char t  = types[typ];
            Parrot_io_eprintf(interp, "\t%c registers used:\t%i\n",
                    t, ru[typ].registers_used);
            if (ru[typ].registers_used) {
                int i;
                Parrot_io_eprintf(interp, "\t%c register count:\t", t);
                for (i = 0; i < NUM_REGISTERS; i++)
                    Parrot_io_eprintf(interp, "%i ", ru[typ].reg_count[i]);
                Parrot_io_eprintf(interp, "\n\t%c register usage:\t", t);
                for (i = 0; i < NUM_REGISTERS; i++)
                    Parrot_io_eprintf(interp, "%i ", ru[typ].reg_usage[i]);
                Parrot_io_eprintf(interp, "\n\t%c register direction:\t", t);
                for (i = 0; i < NUM_REGISTERS; i++)
                    Parrot_io_eprintf(interp, "%i ", (int)ru[typ].reg_dir[i]);
                Parrot_io_eprintf(interp, "\n");
            }
        }
        Parrot_io_eprintf(interp, "\tJit opcodes:\t%u\n",
                cur_section->jit_op_count);
        Parrot_io_eprintf(interp, "\tTotal opcodes:\t%u\n",
                cur_section->op_count);
        if (cur_section->branch_target)
            Parrot_io_eprintf(interp, "\tBranch target:\tOP%u\n",
                    cur_section->branch_target->begin - code_start);

        cur_section = cur_section->next;
    }
}
#endif

/*

=item C<static Parrot_jit_optimizer_t *
optimize_jit(PARROT_INTERP,
            Parrot_jit_info_t *jit_info,
             opcode_t *code_start, opcode_t *code_end)>

Called by C<parrot_build_asm()> to run the optimizer.

=cut

*/

static void
optimize_jit(PARROT_INTERP,
            Parrot_jit_info_t *jit_info,
            opcode_t *code_start, opcode_t *code_end)
{
    Parrot_jit_optimizer_t *optimizer;

    /* Allocate space for the optimizer */
    jit_info->optimizer =
        optimizer = (Parrot_jit_optimizer_t *)
            mem_sys_allocate_zeroed(sizeof (Parrot_jit_optimizer_t));

    /* Look, which opcodes might branch */
    make_branch_list(interp, optimizer, code_start, code_end);

    /* ok, let's loop again and generate the sections */
    make_sections(interp, jit_info, code_start, code_end);

    /* look where a section jumps to */
    make_branch_targets(optimizer, code_start);

    /* This is where we start deciding which Parrot registers get
     * mapped to a hardware one in each different section. */

#if JIT_DEBUG > 2
    debug_sections(interp, optimizer, code_start);
#endif
    if (jit_info->code_type != JIT_CODE_SUB_REGS_ONLY)
        sort_registers(jit_info);
    map_registers(interp, jit_info, code_start);

#if JIT_DEBUG
    debug_sections(interp, optimizer, code_start);
#endif
}

/*

=item C<static Parrot_jit_optimizer_t *
optimize_imcc_jit(PARROT_INTERP,
            Parrot_jit_info_t *jit_info,
            opcode_t *code_start, opcode_t *code_end,
            PackFile_Segment *jit_seg)>

Generate optimizer stuff from the C<_JIT> section in the packfile.

=cut

*/

static void
optimize_imcc_jit(PARROT_INTERP,
            Parrot_jit_info_t *jit_info,
             opcode_t *code_start, opcode_t *code_end,
             PackFile_Segment *jit_seg)
{
    Parrot_jit_optimizer_t *optimizer;
    size_t size, i, typ, n;
    int j;
    opcode_t *ptr, offs;
    Parrot_jit_optimizer_section_ptr section, prev;
    char *branch;
    opcode_t *cur_op;

    /* Allocate space for the optimizer */
    jit_info->optimizer =
        optimizer = (Parrot_jit_optimizer_t *)
            mem_sys_allocate_zeroed(sizeof (Parrot_jit_optimizer_t));
    /*
     * TODO: pass the whole map_branch in the PBC
     *       this would save two runs through all the opcode
     */
    optimizer->map_branch = branch =
        (char *)mem_sys_allocate_zeroed((size_t)(code_end - code_start));
    ptr = jit_seg->data;
    size = jit_seg->size;
    PARROT_ASSERT(jit_seg->itype == 0);
    PARROT_ASSERT((size % 6) == 0);
    cur_op = code_start;
    for (prev = NULL, i = 0; i < size/6; i++, prev = section) {
        section = (Parrot_jit_optimizer_section_t *)
            mem_sys_allocate_zeroed(sizeof (Parrot_jit_optimizer_section_t));
        if (prev)
            prev->next = section;
        else
            optimizer->sections = section;
        section->prev = prev;
        section->block = i;
        offs = *ptr++;
        if (offs & 0x80000000) {
            offs &= ~0x80000000;
            branch[offs] = JIT_BRANCH_TARGET;
        }
        section->begin = code_start + offs;
        section->end = code_start + *ptr++;
        section->isjit = 1;
        for (typ = 0; typ < 4; typ++) {
            section->ru[typ].registers_used = *ptr++;
            for (j = 0; j < section->ru[typ].registers_used; j++)
                section->ru[typ].reg_usage[j] = j;

        }
        while (cur_op <= section->end) {
            const opcode_t op = *cur_op;
            op_info_t * const op_info = &interp->op_info_table[op];
            set_register_usage(interp, jit_info, section,
                    op_info, cur_op, code_start);
            section->op_count++;
            n = op_info->op_count;
            ADD_OP_VAR_PART(interp, interp->code, cur_op, n);
            cur_op += n;
        }
        assign_registers(interp, jit_info, section, code_start, 1);
    }
    insert_fixup_targets(interp, branch, code_end - code_start);
#if JIT_DEBUG
    debug_sections(interp, optimizer, code_start);
#endif
}

/*

=item C<size_t reg_offs(int typ, int i)>

Returns the offset of register C<typ[i]>.

F<src/jit/arch/jit_emit.h> has to define C<Parrot_jit_emit_get_base_reg_no(pc)>

=cut

*/

/* we always are using offsets */


static size_t
reg_offs(int typ, int i)
{
    switch (typ) {
        case 0:
            return REG_OFFS_INT(i);
        case 3:
            return REG_OFFS_NUM(i);
        default:
            return 0;
    }
}

/*

=item C<static void
Parrot_jit_load_registers(Parrot_jit_info_t *jit_info,
                          PARROT_INTERP, int volatiles)>

Load registers for the current section from parrot to processor registers.
If C<volatiles> is true, this code is used to restore these registers in
JITted code that calls out to Parrot.

=cut

*/

static void
Parrot_jit_load_registers(Parrot_jit_info_t *jit_info,
                          PARROT_INTERP, int volatiles)
{
    Parrot_jit_optimizer_section_t *sect = jit_info->optimizer->cur_section;
    Parrot_jit_register_usage_t *ru = sect->ru;
    int typ;
    size_t offs;
    int base_reg = 0;   /* -O3 warning */
    int lasts[] = { 0, 0, 0, 0 };
    const char * maps[] = {0, 0, 0, 0};
    int first = 1;
    const int code_type                   = jit_info->code_type;
    const jit_arch_info * const arch_info = jit_info->arch_info;
    const jit_arch_regs * const reg_info  = arch_info->regs + code_type;

    maps[0]  = reg_info->map_I;
    maps[3]  = reg_info->map_F;
    lasts[0] = reg_info->n_preserved_I;
    lasts[3] = reg_info->n_preserved_F;

    for (typ = 0; typ < 4; typ++) {
        if (maps[typ]) {
            int i;
            for (i = ru[typ].registers_used-1; i >= 0; --i) {
                const int us      = ru[typ].reg_usage[i];
                const int is_used = i >= lasts[typ] && ru[typ].reg_dir[us];
                if ((is_used && volatiles) ||
                        (!volatiles &&
                         ((ru[typ].reg_dir[us] & PARROT_ARGDIR_IN)))) {
                    if (first) {
                        base_reg = Parrot_jit_emit_get_base_reg_no(
                                jit_info->native_ptr);
                        first = 0;
                    }
                    offs = reg_offs(typ, us);
                    if (typ == 3)
                        (arch_info->mov_RM_n)(interp, jit_info,
                                              maps[typ][i], base_reg, offs);
                    else
                        (arch_info->mov_RM_i)(interp, jit_info,
                                              maps[typ][i], base_reg, offs);

                }
            }
        }
    }

    /* The total size of the loads. This is used for branches to
     * the same section - these skip the load asm bytes */
    sect->load_size = jit_info->native_ptr -
        (jit_info->arena.start +
         jit_info->arena.op_map[jit_info->op_i].offset);
}

/*

=item C<static void
Parrot_jit_save_registers(Parrot_jit_info_t *jit_info,
                          PARROT_INTERP, int volatiles)>

Save registers for the current section.
If C<volatiles> is true, this code is used to preserve these registers in
JITted code that calls out to Parrot.

=cut

*/

static void
Parrot_jit_save_registers(Parrot_jit_info_t *jit_info,
                          PARROT_INTERP, int volatiles)
{
    Parrot_jit_optimizer_section_t *sect = jit_info->optimizer->cur_section;
    Parrot_jit_register_usage_t *ru = sect->ru;
    int i, typ;
    size_t offs;
    int base_reg = 0; /* -O3 warning */
    int lasts[] = { 0, 0, 0, 0 };
    const char * maps[] = {0, 0, 0, 0};
    int first = 1;
    int code_type;
    const jit_arch_info *arch_info;
    const jit_arch_regs *reg_info;

    arch_info = jit_info->arch_info;
    code_type = jit_info->code_type;
    reg_info = arch_info->regs + code_type;
    maps[0]  = reg_info->map_I;
    maps[3]  = reg_info->map_F;
    lasts[0] = reg_info->n_preserved_I;
    lasts[3] = reg_info->n_preserved_F;

    for (typ = 0; typ < 4; typ++) {
        if (maps[typ])
            for (i = 0; i < ru[typ].registers_used; ++i) {
                const int us = ru[typ].reg_usage[i];
                const int is_used = i >= lasts[typ] && ru[typ].reg_dir[us];
                if ((is_used && volatiles) ||
                    (!volatiles &&
                     (ru[typ].reg_dir[us] & PARROT_ARGDIR_OUT))) {
                    if (first) {
                        base_reg = Parrot_jit_emit_get_base_reg_no(
                                jit_info->native_ptr);
                        first = 0;
                    }

                    offs = reg_offs(typ, us);
                    if (typ == 3)
                        (arch_info->mov_MR_n)(interp, jit_info,
                                 base_reg, offs, maps[typ][i]);
                    else
                        (arch_info->mov_MR_i)(interp, jit_info,
                                 base_reg, offs, maps[typ][i]);

                }
            }
    }
}

/*

=item C<void
Parrot_destroy_jit(void *ptr)>

Frees the memory used by the JIT subsystem.

=cut

*/

void
Parrot_destroy_jit(void *ptr)
{
    Parrot_jit_optimizer_t *optimizer;
    Parrot_jit_optimizer_section_ptr cur_section, next;
    Parrot_jit_fixup_t *fixup, *next_f;
    Parrot_jit_info_t *jit_info = (Parrot_jit_info_t *)ptr;

    if (!jit_info)
        return;
    /* delete sections */
    optimizer = jit_info->optimizer;
    cur_section = optimizer->sections;
    while (cur_section) {
        next = cur_section->next;
        mem_sys_free(cur_section);
        cur_section = next;
    }
    /* arena stuff */
    mem_sys_free(jit_info->arena.op_map);
    mem_free_executable(jit_info->arena.start, jit_info->arena.size);
    fixup = jit_info->arena.fixups;
    while (fixup) {
        next_f = fixup->next;
        mem_sys_free(fixup);
        fixup = next_f;
    }
    /* optimizer stuff */
    mem_sys_free(optimizer->map_branch);
    mem_sys_free(optimizer->branch_list);
    mem_sys_free(optimizer);

    free(jit_info);
}

/*
 * see TODO below
 * - locate Sub according to pc
 * - set register usage in context
 */
static void
set_reg_usage(PARROT_INTERP, const opcode_t *pc)
{
    PackFile_ByteCode   * const seg = interp->code;
    PackFile_FixupTable * const ft  = seg->fixups;
    PackFile_ConstTable * const ct  = seg->const_table;

    int i;

    if (!ft)
        return;

    if (!ct)
        return;

    for (i = 0; i < ft->fixup_count; i++) {
        if (ft->fixups[i]->type == enum_fixup_sub) {
            const int ci               = ft->fixups[i]->offset;
            PMC           * const sub_pmc = ct->constants[ci]->u.key;
            Parrot_Sub_attributes *sub;
            size_t                 offs;
            int                    i;

            PMC_get_sub(interp, sub_pmc, sub);
            offs = pc - sub->seg->base.data;

            if (offs >= sub->start_offs && offs < sub->end_offs) {
                for (i = 0; i < 4; i++)
                    Parrot_pcc_set_regs_used(interp, CURRENT_CONTEXT(interp),
                            i, sub->n_regs_used[i]);

                return;
            }
        }
    }
}

/*

=item C<Parrot_jit_info_t *
parrot_build_asm(PARROT_INTERP,
          opcode_t *code_start, opcode_t *code_end,
          void *objfile, enum_jit_code_type)>

This is the main function of the JIT code generator.

It loops over the bytecode, calling the code generating routines for
each opcode.

The information obtained is used to perform certain types of fixups on
native code, as well as by the native code itself to convert bytecode
program counters values to hardware program counter values.

Finally this code here is used to generate native executables (or better
object files that are linked to executables), if EXEC_CAPABLE is defined.
This functionality is triggered by

  parrot -o foo.o foo.pir

which uses the JIT engine to translate to native code inside the object
file.

=cut

*/

Parrot_jit_info_t *
parrot_build_asm(PARROT_INTERP, ARGIN(opcode_t *code_start), ARGIN(opcode_t *code_end),
          ARGIN(void *objfile), INTVAL jit_type)
{
    int                   n;
    UINTVAL               i;
    char                 *new_arena;
    Parrot_jit_info_t    *jit_info = NULL;
    opcode_t              cur_opcode_byte;
    opcode_t             *cur_op;
    PackFile_Segment     *jit_seg;
    char                 *map;
    Parrot_jit_fn_info_t *op_func;
    INTVAL                n_regs_used[4];        /* INSP in PBC */
    op_info_t            *op_info;
    const jit_arch_info  *arch_info;
    int                   needs_fs;       /* fetch/store */

    /* XXX
     * no longer referenced due to disabled code below
    char *name;
    */

    Parrot_jit_optimizer_section_ptr cur_section;

#if EXEC_CAPABLE
    Parrot_exec_objfile_t *obj = (Parrot_exec_objfile_t *)objfile;
#endif

    jit_info = interp->code->jit_info = mem_allocate_typed(Parrot_jit_info_t);

    jit_info->flags      = jit_type & JIT_CODE_RECURSIVE;
    jit_type            &= ~ JIT_CODE_RECURSIVE;
    jit_info->code_type  = jit_type;
    needs_fs             = jit_type
                        != JIT_CODE_SUB_REGS_ONLY;

#if EXEC_CAPABLE
    if (objfile) {
        op_func           = op_exec;
        jit_info->objfile = obj;
    }
    else
#endif
    {
        op_func           = op_jit;
        jit_info->objfile = NULL;
    }

    /* reset some extcall bits - all is JITed
     */
    if (jit_type == JIT_CODE_SUB_REGS_ONLY) {
        op_func[PARROT_OP_set_returns_pc].extcall = 0;
        op_func[PARROT_OP_get_params_pc].extcall  = 0;
        op_func[PARROT_OP_get_params_pc].extcall  = 0;
        op_func[PARROT_OP_invokecc_p].extcall     = 0;
    }

    /* get register mappings and such */
    arch_info = jit_info->arch_info = Parrot_jit_init(interp);

    /*
     * check if IMCC did all the work. If yes, we have a PF segment with
     * register allocation information inside.
     * See imcc/jit.c for more
     */
#if JIT_SEGS
    /* RT #45055
     * JIT segs are currently not built
     * the find_segments also segfaults on PPC eval_2
     * maybe something not initialized correctly
     * - disabled --leo
     */
    name    = mem_sys_allocate(strlen(interp->code->base.name) + 5);
    sprintf(name, "%s_JIT", interp->code->base.name);
    jit_seg = PackFile_find_segment(interp,
            interp->code->base.dir, name, 0);
    mem_sys_free(name);
#else
    jit_seg = NULL;
#endif

    /* remember register usage */
    for (i = 0; i < 4; i++)
        n_regs_used[i] = Parrot_pcc_get_regs_used(interp, CURRENT_CONTEXT(interp), i);

    set_reg_usage(interp, code_start);

#if JIT_SEGS
    optimize_imcc_jit(interp, jit_info, code_start, code_end, jit_seg);
#else
    optimize_jit(interp, jit_info, code_start, code_end);
#endif

    /* Byte code size in opcode_t's */
    jit_info->arena.map_size = (code_end - code_start) + 1;
    jit_info->arena.op_map   =
        (Parrot_jit_opmap_t *)mem_sys_allocate_zeroed(
            jit_info->arena.map_size * sizeof (* (jit_info->arena.op_map)));

#if REQUIRES_CONSTANT_POOL
    Parrot_jit_init_arenas(jit_info);
#else
    jit_info->arena.size = 1024;

    /* estimate size needed
     * 10 times pbc code size seems to be enough for i386
     */
    if ((size_t)jit_info->arena.map_size * 20 > (size_t)jit_info->arena.size)
        jit_info->arena.size = jit_info->arena.map_size * 20;
    jit_info->native_ptr     = jit_info->arena.start =
        (char *)mem_alloc_executable((size_t)jit_info->arena.size);
    if (! jit_info->native_ptr)
        Parrot_ex_throw_from_c_args(interp, NULL, EXCEPTION_JIT_ERROR,
                "Cannot allocate executable memory");

#  if EXEC_CAPABLE
    if (obj)
        jit_info->objfile->text.code = jit_info->arena.start;
#  endif
#endif

    jit_info->op_i         = 0;
    jit_info->arena.fixups = NULL;

    /* The first section */
    cur_section = jit_info->optimizer->cur_section
                = jit_info->optimizer->sections;
    map         = jit_info->optimizer->map_branch;

    /*
     * from C's ABI all the emitted code here is one (probably big)
     * function. So we have to generate an appropriate function
     * prologue, that makes all this look like a normal function ;)
     */
    jit_info->cur_op = cur_section->begin;
    (arch_info->regs[jit_type].jit_begin)(jit_info, interp);

    /*
     *   op_map holds the offset from arena.start
     *   of the parrot op at the given opcode index
     *
     * Set the offset of the first opcode
     */
    jit_info->arena.op_map[jit_info->op_i].offset =
        jit_info->native_ptr - jit_info->arena.start;

    /*
     * the function epilog can basically be anywhere, that's done
     * by the Parrot_end opcode somewhere in core.jit
     */

    while (jit_info->optimizer->cur_section) {
        /* the code emitting functions need cur_op and cur_section
         * so these vars are in jit_info too
         */

        /*
         * TODO
         *
         * Register offsets depend on n_regs_used, which is per
         * subroutine. JIT code is currently generated for a whole
         * PBC. We can either:
         * 1) create JIT per subroutine or
         * 2) track the sub we are currently in, set register usage
         *    in the interpreter context and restore it at end
         *
         * for now we use 2) - longterm plan is 1)
         */

        /* The first opcode for this section */
        cur_op = jit_info->cur_op = cur_section->begin;

        set_reg_usage(interp, cur_op);

        /* Load mapped registers for this section, if JIT */
        if (!jit_seg && cur_section->isjit && needs_fs)
            Parrot_jit_load_registers(jit_info, interp, 0);

        /* The first opcode of each section doesn't have a previous one since
         * it's impossible to be sure which was it */
        jit_info->prev_op = NULL;

        while (cur_op <= cur_section->end) {
            /* Grow the arena early */
            if (jit_info->arena.size <
                    (jit_info->arena.op_map[jit_info->op_i].offset + 200)) {
#if REQUIRES_CONSTANT_POOL
                Parrot_jit_extend_arena(jit_info);
#else
                new_arena = (char *)mem_realloc_executable(jit_info->arena.start,
                        jit_info->arena.size, (size_t)jit_info->arena.size * 2);
                jit_info->arena.size *= 2;
                jit_info->native_ptr  = new_arena +
                    (jit_info->native_ptr - jit_info->arena.start);
                jit_info->arena.start = new_arena;
#  if EXEC_CAPABLE
                if (obj)
                    obj->text.code = new_arena;
#  endif
#endif
            }

            cur_opcode_byte = *cur_op;

            /* Need to save the registers if there is a branch and is not to
             * the same section, I admit I don't like this, and it should be
             * really checking if the target section has the same registers
             * mapped too.
             *
             * Yep so: TODO
             * during register allocation try to use the same registers, if
             * its a loop or a plain branch and if register usage doesn't
             * differ too much. This could save a lot of register reloads.
             *
             * --
             *
             * save also, if we have a jitted sections and encounter
             * an "end" opcode, e.g. in evaled code
             */
            if (needs_fs) {
                if ((((map[cur_op - code_start] == JIT_BRANCH_SOURCE) &&
                            (cur_section->branch_target != cur_section)) ||
                            !cur_opcode_byte) &&
                        cur_section->isjit &&
                        !jit_seg) {
                    Parrot_jit_save_registers(jit_info, interp, 0);
                }
                else if (CALLS_C_CODE(cur_opcode_byte)) {
                    /*
                     * a JITted function with a function call, we have to
                     * save volatile registers but
                     * TODO not if the previous opcode was also one
                     *      that called C code
                     */
                    Parrot_jit_save_registers(jit_info, interp, 1);
                }
            }

            /*
             * for all opcodes that are dynamically loaded, we can't have
             * a JITted version, so we execute the function incarnation
             * via the wrapper opcode, which just runs one opcode
             * see ops/core.ops for more
             */
            if (cur_opcode_byte >= jit_op_count())
                cur_opcode_byte = CORE_OPS_wrapper__;

            (op_func[cur_opcode_byte].fn) (jit_info, interp);

            if (CALLS_C_CODE(cur_opcode_byte) && needs_fs) {
                /*
                 * restore volatiles only - and TODO only if next
                 *      wouldn't load registers anyway
                 */
                Parrot_jit_load_registers(jit_info, interp, 1);
            }
            /* Update the previous opcode */
            jit_info->prev_op = cur_op;

            op_info = &interp->op_info_table[*cur_op];
            n       = op_info->op_count;
            ADD_OP_VAR_PART(interp, interp->code, cur_op, n);
            cur_op += n;

            /* update op_i and cur_op accordingly */
            jit_info->op_i   += n;
            jit_info->cur_op += n;

            if (*jit_info->prev_op == PARROT_OP_set_args_pc &&
                    jit_type == JIT_CODE_SUB_REGS_ONLY) {
                PARROT_ASSERT(*cur_op == PARROT_OP_set_p_pc);

                /* skip it */
                cur_op           += 3;
                jit_info->op_i   += 3;
                jit_info->cur_op += 3;

                jit_info->arena.op_map[jit_info->op_i].offset =
                    jit_info->native_ptr - jit_info->arena.start;

                PARROT_ASSERT(*cur_op == PARROT_OP_get_results_pc);

                /* now emit the call - use special op for this */
                /* Don't want to fix it. JIT on chopping block.
                 * (op_func[PARROT_OP_pic_callr___pc].fn)(jit_info, interp);
                 */

                /* and the get_results */
                (op_func[*cur_op].fn)(jit_info, interp);

                op_info = &interp->op_info_table[*cur_op];
                n       = op_info->op_count;
                ADD_OP_VAR_PART(interp, interp->code, cur_op, n);

                cur_op += n;

                /* update op_i and cur_op accordingly */
                jit_info->op_i   += n;
                jit_info->cur_op += n;

                jit_info->arena.op_map[jit_info->op_i].offset =
                    jit_info->native_ptr - jit_info->arena.start;

                /* now at invoke */
                PARROT_ASSERT(*cur_op == PARROT_OP_invokecc_p);

                cur_op            += 2;    /* skip it */
                jit_info->op_i    += 2;
                jit_info->cur_op  += 2;
            }

            /* if this is a branch target, align it */
#ifdef jit_emit_noop
#  if JUMP_ALIGN
            if (((!cur_section->next && cur_op <= cur_section->end) ||
                        cur_section->next) &&
                    map[cur_op - code_start] == JIT_BRANCH_TARGET) {
                while ((long)jit_info->native_ptr & ((1<<JUMP_ALIGN) - 1))
                    jit_emit_noop(jit_info->native_ptr);
            }
#  endif
#endif
            /* set the offset */
            jit_info->arena.op_map[jit_info->op_i].offset =
                jit_info->native_ptr - jit_info->arena.start;
        }

        /* Save mapped registers back to the Parrot registers */
        if (!jit_seg && cur_section->isjit && needs_fs)
            Parrot_jit_save_registers(jit_info, interp, 0);

        /* update the offset for saved registers */
        jit_info->arena.op_map[jit_info->op_i].offset =
            jit_info->native_ptr - jit_info->arena.start;

        /* Move to the next section */
        cur_section = jit_info->optimizer->cur_section =
            cur_section->next;
    }

    /* restore register usage */
    for (i = 0; i < 4; i++)
        Parrot_pcc_set_regs_used(interp, CURRENT_CONTEXT(interp), i, n_regs_used[i]);

    /* Do fixups before converting offsets */
    (arch_info->jit_dofixup)(jit_info, interp);

    /* Convert offsets to pointers */
    if (!objfile)
        for (i = 0; i < jit_info->arena.map_size; i++) {

            /* Assuming native code chunks contain some initialization code,
             * the first op (and every other op) is at an offset > 0
             */
            if (jit_info->arena.op_map[i].offset) {
                jit_info->arena.op_map[i].ptr = (char *)jit_info->arena.start +
                    jit_info->arena.op_map[i].offset;
            }
        }

    jit_info->arena.size =
        (ptrdiff_t)(jit_info->native_ptr - jit_info->arena.start);
#if JIT_DEBUG
    Parrot_io_eprintf(interp, "\nTotal size %u bytes\n",
            (unsigned int)(jit_info->native_ptr - jit_info->arena.start));
#endif

    /*
     * sync data cache if needed - we are executing it as code in some usecs
     */
    if (arch_info->jit_flush_cache)
        (arch_info->jit_flush_cache)(jit_info, interp);

    /* assume gdb is available: generate symbol information  */
#if defined __GNUC__ || defined __IBMC__
    if (Interp_debug_TEST(interp, PARROT_JIT_DEBUG_FLAG)) {
        /*
         * TODO same like above here e.g. create ASM listing of code
         *      if real debug support isn't available
         */
        if (jit_type == JIT_CODE_FILE) {
            interp->code->jit_info = jit_info;
            Parrot_jit_debug(interp);
        }
    }
#endif

    return jit_info;
}

/*

=item C<void
Parrot_jit_newfixup(Parrot_jit_info_t *jit_info)>

Remember the current position in the native code for later update.

=cut

*/

void
Parrot_jit_newfixup(Parrot_jit_info_t *jit_info)
{
    Parrot_jit_fixup_t *fixup;

    fixup = mem_allocate_zeroed_typed(Parrot_jit_fixup_t);

    /* Insert fixup at the head of the list */
    fixup->next = jit_info->arena.fixups;
    jit_info->arena.fixups = fixup;

    /* Fill in the native code offset */
    fixup->native_offset =
        (ptrdiff_t)(jit_info->native_ptr - jit_info->arena.start);
}

/*

=item C<void Parrot_jit_free_buffer(PARROT_INTERP, void *ptr, void *priv)>

This is a callback to implement the proper freeing semantics.  It is called by
the ManagedStruct PMC as it is garbage collected.

=cut

*/

void
Parrot_jit_free_buffer(PARROT_INTERP, void *ptr, void *priv)
{
    const struct jit_buffer_private_data * const jit = (struct jit_buffer_private_data*)priv;
    mem_free_executable(ptr, jit->size);
    free(priv);
}

/*

=item C<PMC *Parrot_jit_clone_buffer(PARROT_INTERP, PMC *pmc, void *priv)>

This is a callback to implement the proper cloning semantics for jit buffers.
It is called by the ManagedStruct PMC's clone() function.

=cut

*/

PMC *
Parrot_jit_clone_buffer(PARROT_INTERP, PMC *pmc, void *priv)
{
    PMC * const rv = pmc_new(interp, pmc->vtable->base_type);

    VTABLE_init(interp, rv);
    /* copy the attributes */
    {
        void (*tmpfreefunc)(PARROT_INTERP, void*, void*);
        GETATTR_ManagedStruct_custom_free_func(interp, pmc, tmpfreefunc);
        SETATTR_ManagedStruct_custom_free_func(interp, rv , tmpfreefunc);
    }
    {
        PMC* (*tmpclonefunc)(PARROT_INTERP, PMC*, void*);
        GETATTR_ManagedStruct_custom_clone_func(interp, pmc, tmpclonefunc);
        SETATTR_ManagedStruct_custom_clone_func(interp, rv , tmpclonefunc);
    }

    {
        void *freepriv, *clonepriv;
        GETATTR_ManagedStruct_custom_free_priv(interp , pmc, freepriv);
        GETATTR_ManagedStruct_custom_clone_priv(interp, pmc, clonepriv);
        if (freepriv) {
            void *tmp = mem_sys_allocate(sizeof (struct jit_buffer_private_data));
            memcpy(tmp, freepriv, sizeof (struct jit_buffer_private_data));
            SETATTR_ManagedStruct_custom_free_priv(interp, rv , tmp);
            if (clonepriv == freepriv) {
                /* clonepriv is a copy of freepriv, make it a copy in the clone too. */
                SETATTR_ManagedStruct_custom_clone_priv(interp, rv , tmp);
                clonepriv = NULL; /* disable the clonepriv copying below */
            }
        }
        if (clonepriv) {
            void *tmp = mem_sys_allocate(sizeof (struct jit_buffer_private_data));
            memcpy(tmp, clonepriv, sizeof (struct jit_buffer_private_data));
            SETATTR_ManagedStruct_custom_clone_priv(interp, rv , tmp);
        }
    }

    /* copy the execmem buffer */
    if (PARROT_MANAGEDSTRUCT(pmc)->ptr) {
        struct jit_buffer_private_data *jit = (struct jit_buffer_private_data*)priv;
        void *ptr = PARROT_MANAGEDSTRUCT(pmc)->ptr;
        void *newptr = mem_alloc_executable(jit->size);
        if (!newptr)
            Parrot_ex_throw_from_c_args(interp, NULL, EXCEPTION_JIT_ERROR,
                    "Cannot allocate executable memory");
        memcpy(newptr, ptr, jit->size);
        PARROT_MANAGEDSTRUCT(rv)->ptr = newptr;
    }

    return rv;
}


/*

=back

=head1 SEE ALSO

F<src/jit.h>, F<docs/jit.pod>, F<src/jit_debug.c>,
F<src/jit/$jitcpuarch/jit_emit.h>, F<jit/$jitcpuarch/core.jit>.

=cut

*/


/*
 * Local variables:
 *   c-file-style: "parrot"
 * End:
 * vim: expandtab shiftwidth=4:
 */
