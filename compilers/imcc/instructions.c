/* $Id$ */

/*
 * Copyright (C) 2002-2008, The Perl Foundation.
 */

#include <stdlib.h>
#include <string.h>
#define _PARSER
#include "imc.h"
#include "pbc.h"
#include "optimizer.h"

/*

=head1 NAME

compilers/imcc/instructions.c

=head1 DESCRIPTION

When generating the code, the instructions of the program
are stored in an array.

After the register allocation is resolved, the instructions
array is flushed.

These functions operate over this array and its contents.

=head2 Functions

=over 4

=cut

*/

/* Global variables , forward def */

#if 0
static Instruction * last_ins;

int n_comp_units;
#endif

/* HEADERIZER HFILE: compilers/imcc/instructions.h */

/* HEADERIZER BEGIN: static */
/* Don't modify between HEADERIZER BEGIN / HEADERIZER END.  Your changes will be lost. */

static int e_file_close(PARROT_INTERP, SHIM(void *param))
        __attribute__nonnull__(1);

static int e_file_emit(PARROT_INTERP,
    SHIM(void *param),
    SHIM(const IMC_Unit *unit),
    ARGIN(const Instruction *ins))
        __attribute__nonnull__(1)
        __attribute__nonnull__(4);

static int e_file_open(SHIM_INTERP, ARGIN(void *param))
        __attribute__nonnull__(2);

/* Don't modify between HEADERIZER BEGIN / HEADERIZER END.  Your changes will be lost. */
/* HEADERIZER END: static */

static const char types[] = "INPS";

static const Emitter emitters[] = {
    {e_file_open,
     e_file_emit,
     NULL,
     NULL,
     e_file_close},

    {e_pbc_open,
     e_pbc_emit,
     e_pbc_new_sub,
     e_pbc_end_sub,
     e_pbc_close},
};

static int emitter;     /* XXX */

/*

=item C<Instruction * _mk_instruction>

Creates a new instruction

=cut

*/

PARROT_MALLOC
PARROT_CANNOT_RETURN_NULL
Instruction *
_mk_instruction(ARGIN(const char *op), ARGIN(const char *fmt), int n,
        ARGIN(SymReg * const *r), int flags)
{
    const size_t reg_space  = (n > 1) ? (sizeof (SymReg *) * (n - 1)) : 0;
    Instruction * const ins =
        (Instruction*)mem_sys_allocate_zeroed(sizeof (Instruction) + reg_space);
    int i;

    ins->opname       = str_dup(op);
    ins->format       = str_dup(fmt);
    ins->symreg_count = n;

    for (i = 0; i < n; i++)
        ins->symregs[i]  = r[i];

    ins->flags = flags;
    ins->opnum = -1;

    return ins;
}


/*
 * Some instructions don't have a hint in op_info that they work
 * on all registers or all registers of a given type (e.g., cleari)
 * These instructions need special handling at various points in the code.
 */

static int w_special[1+4*3];

/*

=item C<void imcc_init_tables>

RT#48260: Not yet documented!!!

=cut

*/

void
imcc_init_tables(PARROT_INTERP)
{
    const char *writes[] = {
        "cleari", "clearn", "clearp", "clears",
    };
    /* init opnums */
    if (!w_special[0]) {
        size_t i;
        for (i = 0; i < N_ELEMENTS(writes); i++) {
            const int n = interp->op_lib->op_code(writes[i], 1);
            PARROT_ASSERT(n);
            w_special[i] = n;
        }
    }
}

/*

=item C<int ins_writes2>

Returns TRUE if instruction ins writes to a register of type t

=cut

*/

int
ins_writes2(ARGIN(const Instruction *ins), int t)
{
    const char *p;

    if (ins->opnum == w_special[0])
        return 1;

    p = strchr(types, t);

    if (p) {
        const size_t idx = p - types;
        size_t i;

        for (i = 1; i < N_ELEMENTS(w_special); i += 4)
            if (ins->opnum == w_special[i + idx])
                return 1;
    }

    return 0;
}

/*

=item C<int instruction_reads>

next two functions are called very often, says gprof
they should be fast

=cut

*/

int
instruction_reads(ARGIN(const Instruction *ins), ARGIN(const SymReg *r))
{
    int f, i;

    if (ins->opnum == PARROT_OP_set_args_pc
    ||  ins->opnum == PARROT_OP_set_returns_pc) {

        for (i = ins->symreg_count - 1; i >= 0; --i)
            if (r == ins->symregs[i])
                return 1;

        return 0;
    }
    else if (ins->opnum == PARROT_OP_get_params_pc ||
             ins->opnum == PARROT_OP_get_results_pc) {
        return 0;
    }

    f = ins->flags;

    for (i = ins->symreg_count - 1; i >= 0; --i) {
        if (f & (1 << i)) {
            const SymReg * const ri = ins->symregs[i];

            if (ri == r)
                return 1;

            /* this additional test for _kc ops seems to slow
             * down instruction_reads by a huge amount compared to the
             * _writes below
             */
            if (ri->set == 'K') {
                const SymReg *key;
                for (key = ri->nextkey; key; key = key->nextkey)
                    if (key->reg == r)
                        return 1;
            }
        }
    }

    /* a sub call reads the previous args */
    if (ins->type & ITPCCSUB) {
        while (ins && ins->opnum != PARROT_OP_set_args_pc)
            ins = ins->prev;

        if (!ins)
            return 0;

        for (i = ins->symreg_count - 1; i >= 0; --i) {
            if (ins->symregs[i] == r)
                return 1;
        }
    }

    return 0;
}

/*

=item C<int instruction_writes>

RT#48260: Not yet documented!!!

=cut

*/

int
instruction_writes(ARGIN(const Instruction *ins), ARGIN(const SymReg *r))
{
    const int f = ins->flags;
    int i;

    /*
     * a get_results opcode is before the actual sub call
     * but for the register allocator, the effect matters, thus
     * postpone the effect after the invoke
     */
    if (ins->opnum == PARROT_OP_get_results_pc) {
        int i;

        /* but only if it isn't the get_results opcode of
         * an ExceptionHandler, which doesn't have
         * a call next
         */
        if (ins->next && (ins->next->type & ITPCCSUB))
            return 0;

        for (i = ins->symreg_count - 1; i >= 0; --i) {
            if (ins->symregs[i] == r)
                return 1;
        }

        return 0;
    }
    else if (ins->type & ITPCCSUB) {
        int i;
        ins = ins->prev;
        /* can't used pcc_sub->ret due to bug #38406
         * it seems that all sub SymRegs are shared
         * and point to the most recemt pcc_sub
         * structure
         */
        while (ins && ins->opnum != PARROT_OP_get_results_pc)
            ins = ins->prev;

        if (!ins)
            return 0;

        for (i = ins->symreg_count - 1; i >= 0; --i) {
            if (ins->symregs[i] == r)
                return 1;
        }

        return 0;
    }

    if (ins->opnum == PARROT_OP_get_params_pc) {
        int i;

        for (i = ins->symreg_count - 1; i >= 0; --i) {
            if (ins->symregs[i] == r)
                return 1;
        }

        return 0;
    }
    else if (ins->opnum == PARROT_OP_set_args_pc
         ||  ins->opnum == PARROT_OP_set_returns_pc) {
        return 0;
    }

    for (i = 0; i < ins->symreg_count; i++)
        if (f & (1 << (16 + i)))
            if (ins->symregs[i] == r)
                return 1;

    return 0;
}

/*

=item C<int get_branch_regno>

Get the register number of an address which is a branch target

=cut

*/

int
get_branch_regno(ARGIN(const Instruction *ins))
{
    int j;

    for (j = ins->opsize - 2; j >= 0 && ins->symregs[j] ; --j)
        if (ins->type & (1 << j))
            return j;

    return -1;
}

/*

=item C<SymReg * get_branch_reg>

Get the register corresponding to an address which is a branch target

=cut

*/

PARROT_WARN_UNUSED_RESULT
PARROT_CAN_RETURN_NULL
SymReg *
get_branch_reg(ARGIN(const Instruction *ins))
{
    const int r = get_branch_regno(ins);

    if (r >= 0)
        return ins->symregs[r];

    return NULL;
}

/* some useful instruction routines */

/*

=item C<Instruction * _delete_ins>

Delete instruction ins. It's up to the caller to actually free the memory
of ins, if appropriate.

The instruction following ins is returned.

=cut

*/

PARROT_WARN_UNUSED_RESULT
PARROT_CAN_RETURN_NULL
Instruction *
_delete_ins(ARGMOD(IMC_Unit *unit), ARGIN(Instruction *ins))
{
    Instruction * const next = ins->next;
    Instruction * const prev = ins->prev;

    if (prev)
        prev->next = next;
    else
        unit->instructions = next;

    if (next)
        next->prev = prev;
    else
        unit->last_ins = prev;

    return next;
}

/*

=item C<Instruction * delete_ins>

Delete instruction ins, and then free it.

The instruction following ins is returned.

=cut

*/

PARROT_WARN_UNUSED_RESULT
PARROT_CAN_RETURN_NULL
Instruction *
delete_ins(ARGMOD(IMC_Unit *unit), ARGMOD(Instruction *ins))
{
    Instruction * next = _delete_ins(unit, ins);

    free_ins(ins);

    return next;
}

/*

=item C<void insert_ins>

insert tmp after ins

=cut

*/

void
insert_ins(ARGMOD(IMC_Unit *unit), ARGMOD_NULLOK(Instruction *ins),
        ARGMOD(Instruction *tmp))
{
    if (!ins) {
        Instruction * const next = unit->instructions;

        unit->instructions = tmp;
        tmp->next          = next;

        if (next) {
            next->prev = tmp;
            tmp->line  = next->line;
        }
        else {
            unit->last_ins = tmp;
        }
    }
    else {
        Instruction * const next = ins->next;

        ins->next = tmp;
        tmp->prev = ins;
        tmp->next = next;

        if (next)
            next->prev = tmp;
        else
            unit->last_ins = tmp;

        if (!tmp->line)
            tmp->line = ins->line;
    }
}

/*

=item C<void prepend_ins>

insert tmp before ins

=cut

*/

void
prepend_ins(ARGMOD(IMC_Unit *unit), ARGMOD_NULLOK(Instruction *ins),
        ARGMOD(Instruction *tmp))
{
    if (!ins) {
        Instruction * const next = unit->instructions;

        unit->instructions = tmp;
        tmp->next          = next;
        next->prev         = tmp;
        tmp->line          = next->line;
    }
    else {
        Instruction * const prev = ins->prev;

        ins->prev = tmp;
        tmp->next = ins;
        tmp->prev = prev;

        if (prev)
            prev->next = tmp;

        if (!tmp->line)
            tmp->line = ins->line;
    }
}

/*

=item C<void subst_ins>

Substitute tmp for ins. Free ins if needs_freeing is true.

=cut

*/

void
subst_ins(ARGMOD(IMC_Unit *unit), ARGMOD(Instruction *ins),
          ARGMOD(Instruction *tmp), int needs_freeing)
{
    Instruction * const prev = ins->prev;

    if (prev)
        prev->next = tmp;
    else
        unit->instructions = tmp;

    tmp->prev = prev;
    tmp->next = ins->next;

    if (ins->next)
        ins->next->prev = tmp;
    else
        unit->last_ins = tmp;

    if (tmp->line == 0)
        tmp->line = ins->line;

    if (needs_freeing)
        free_ins(ins);
}

/*

=item C<Instruction * move_ins>

Move instruction ins from its current position to the position
following instruction to. Returns the instruction following the
initial position of ins.

=cut

*/

PARROT_CAN_RETURN_NULL
Instruction *
move_ins(ARGMOD(IMC_Unit *unit), ARGMOD(Instruction *ins), ARGMOD(Instruction *to))
{
    Instruction * const next = _delete_ins(unit, ins);
    insert_ins(unit, to, ins);
    return next;
}


/*

=item C<Instruction * emitb>

Emit a single instruction into the current unit buffer.

=cut

*/

PARROT_CAN_RETURN_NULL
Instruction *
emitb(PARROT_INTERP, ARGMOD_NULLOK(IMC_Unit *unit), ARGIN_NULLOK(Instruction *i))
{

    if (!unit || !i)
        return NULL;

    if (!unit->instructions)
        unit->last_ins = unit->instructions = i;
    else {
        unit->last_ins->next = i;
        i->prev              = unit->last_ins;
        unit->last_ins       = i;
    }

    /* lexer is in next line already */
    i->line = IMCC_INFO(interp)->line - 1;

    return i;
}

/*

=item C<void free_ins>

Free the Instruction structure ins.

=cut

*/

void
free_ins(ARGMOD(Instruction *ins))
{
    free(ins->format);
    free(ins->opname);
    free(ins);
}

/*

=item C<int ins_print>

Print details of instruction ins in file fd.

=cut

*/

#define REGB_SIZE 256
int
ins_print(PARROT_INTERP, ARGMOD(FILE *fd), ARGIN(const Instruction *ins))
{
    char regb[IMCC_MAX_FIX_REGS][REGB_SIZE];
    /* only long key constants can overflow */
    char *regstr[IMCC_MAX_FIX_REGS];
    int i;
    int len;

#if IMC_TRACE
    PIO_eprintf(NULL, "ins_print\n");
#endif

    /* comments, labels and such */
    if (!ins->symregs[0] || !strchr(ins->format, '%'))
        return fprintf(fd, "%s", ins->format);

    for (i = 0; i < ins->symreg_count; i++) {
        const SymReg *p = ins->symregs[i];
        if (!p)
            continue;

        if (p->type & VT_CONSTP)
            p = p->reg;

        if (p->color >= 0 && REG_NEEDS_ALLOC(p)) {
            snprintf(regb[i], REGB_SIZE, "%c%d", p->set, (int)p->color);
            regstr[i] = regb[i];
        }
        else if (IMCC_INFO(interp)->allocated
             && (IMCC_INFO(interp)->optimizer_level & OPT_J)
             &&  p->set != 'K'
             &&  p->color < 0
             && REG_NEEDS_ALLOC(p)) {
                    snprintf(regb[i], REGB_SIZE,
                        "r%c%d", tolower((unsigned char)p->set),
                        -1 -(int)p->color);
                    regstr[i] = regb[i];
        }
        else if (p->type & VTREGKEY) {
            const SymReg *k = p;

            *regb[i] = '\0';

            while ((k = k->nextkey) != NULL) {
                const size_t used = strlen(regb[i]);

                if (k->reg && k->reg->color >= 0)
                    snprintf(regb[i]+used, REGB_SIZE - used, "%c%d",
                            k->reg->set, (int)k->reg->color);
                else if (IMCC_INFO(interp)->allocated
                     && (IMCC_INFO(interp)->optimizer_level & OPT_J)
                     && k->reg
                     && k->reg->color < 0)
                        snprintf(regb[i]+used, REGB_SIZE - used, "r%c%d",
                            tolower((unsigned char)k->reg->set),
                            -1 -(int)k->reg->color);
                else
                    strncat(regb[i], k->name, REGB_SIZE - used - 1);

                if (k->nextkey)
                    strncat(regb[i], ";", REGB_SIZE - strlen(regb[i]) - 1);
            }

            regstr[i] = regb[i];
        }
        else if (p->type == VTCONST
             &&  p->set  == 'S'
             && *p->name != '"'
             && *p->name != '\'') {
            /* unquoted string const */
            snprintf(regb[i], REGB_SIZE, "\"%s\"", p->name);
            regstr[i] = regb[i];
        }
        else
            regstr[i] = p->name;
    }

    switch (ins->opsize-1) {
        case -1:        /* labels */
        case 1:
            len = fprintf(fd, ins->format, regstr[0]);
            break;
        case 2:
            len = fprintf(fd, ins->format, regstr[0], regstr[1]);
            break;
        case 3:
            len = fprintf(fd, ins->format, regstr[0], regstr[1], regstr[2]);
            break;
        case 4:
            len = fprintf(fd, ins->format, regstr[0], regstr[1], regstr[2],
                    regstr[3]);
            break;
        case 5:
            len = fprintf(fd, ins->format, regstr[0], regstr[1], regstr[2],
                    regstr[3], regstr[4]);
            break;
        case 6:
            len = fprintf(fd, ins->format, regstr[0], regstr[1], regstr[2],
                    regstr[3], regstr[4], regstr[5]);
            break;
        default:
            fprintf(stderr, "unhandled: opsize (%d), op %s, fmt %s\n",
                    ins->opsize, ins->opname, ins->format);
            exit(EXIT_FAILURE);
            break;
    }

    return len;
}

/* for debug */
static char *output;

/*

=item C<static int e_file_open>

RT#48260: Not yet documented!!!

=cut

*/

static int
e_file_open(SHIM_INTERP, ARGIN(void *param))
{
    char * const file = (char *) param;

    if (!STREQ(file, "-"))
        freopen(file, "w", stdout);
    output = file;
    printf("# IMCC does produce b0rken PASM files\n");
    printf("# see http://guest@rt.perl.org/rt3/Ticket/Display.html?id=32392\n");
    return 1;
}

/*

=item C<static int e_file_close>

RT#48260: Not yet documented!!!

=cut

*/

static int
e_file_close(PARROT_INTERP, SHIM(void *param))
{
    printf("\n\n");
    fclose(stdout);
    IMCC_info(interp, 1, "assembly module %s written.\n", output);
    return 0;
}

/*

=item C<static int e_file_emit>

RT#48260: Not yet documented!!!

=cut

*/

static int
e_file_emit(PARROT_INTERP,
        SHIM(void *param),
        SHIM(const IMC_Unit *unit),
        ARGIN(const Instruction *ins))
{
#if IMC_TRACE
    PIO_eprintf(NULL, "e_file_emit\n");
#endif
    if ((ins->type & ITLABEL) || ! *ins->opname)
        ins_print(interp, stdout, ins);
    else {
        imcc_fprintf(interp, stdout, "\t%I ", ins);
    }
    printf("\n");
    return 0;
}

/*

=item C<int emit_open>

RT#48260: Not yet documented!!!

=cut

*/

PARROT_API
int
emit_open(PARROT_INTERP, int type, ARGIN_NULLOK(void *param))
{
    emitter                          = type;
    IMCC_INFO(interp)->has_compile   = 0;
    IMCC_INFO(interp)->dont_optimize = 0;

    return (emitters[emitter]).open(interp, param);
}

/*

=item C<int emit_flush>

RT#48260: Not yet documented!!!

=cut

*/

PARROT_API
int
emit_flush(PARROT_INTERP, ARGIN_NULLOK(void *param), ARGIN(IMC_Unit *unit))
{
    Instruction * ins;

    if (emitters[emitter].new_sub)
        (emitters[emitter]).new_sub(interp, param, unit);

    for (ins = unit->instructions; ins; ins = ins->next) {
        IMCC_debug(interp, DEBUG_IMC, "emit %I\n", ins);
        (emitters[emitter]).emit(interp, param, unit, ins);
    }

    if (emitters[emitter].end_sub)
        (emitters[emitter]).end_sub(interp, param, unit);

    return 0;
}

/*

=item C<int emit_close>

RT#48260: Not yet documented!!!

=cut

*/

PARROT_API
int
emit_close(PARROT_INTERP, ARGIN_NULLOK(void *param))
{
    return (emitters[emitter]).close(interp, param);
}

/*

=back

=cut

*/

/*
 * Local variables:
 *   c-file-style: "parrot"
 * End:
 * vim: expandtab shiftwidth=4:
 */
