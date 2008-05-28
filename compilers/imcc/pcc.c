/*
 * $Id$
 * Copyright (C) 2003-2008, The Perl Foundation.
 *
 * pcc.c
 *
 */

/*

=head1 NAME

compilers/imcc/pcc.c

=head1 DESCRIPTION

Parrot calling convention implementation.

see: docs/pdds/pdd03_calling_conventions.pod

PCC Implementation by Leopold Toetsch

=head2 Functions

=over 4

=cut

*/

#include <stdlib.h>
#include <string.h>
#include "imc.h"
#include "parser.h"

/* HEADERIZER HFILE: compilers/imcc/imc.h */

/* HEADERIZER BEGIN: static */
/* Don't modify between HEADERIZER BEGIN / HEADERIZER END.  Your changes will be lost. */

static void insert_tail_call(PARROT_INTERP,
    ARGIN(IMC_Unit *unit),
    ARGMOD(Instruction *ins),
    ARGMOD(SymReg *sub),
    ARGIN(SymReg *meth))
        __attribute__nonnull__(1)
        __attribute__nonnull__(2)
        __attribute__nonnull__(3)
        __attribute__nonnull__(4)
        __attribute__nonnull__(5)
        FUNC_MODIFIES(*ins)
        FUNC_MODIFIES(*sub);

PARROT_WARN_UNUSED_RESULT
PARROT_CAN_RETURN_NULL
static Instruction * insINS(PARROT_INTERP,
    ARGMOD(IMC_Unit *unit),
    ARGIN(Instruction *ins),
    ARGIN(const char *name),
    ARGIN(SymReg **regs),
    int n)
        __attribute__nonnull__(1)
        __attribute__nonnull__(2)
        __attribute__nonnull__(3)
        __attribute__nonnull__(4)
        __attribute__nonnull__(5)
        FUNC_MODIFIES(*unit);

PARROT_WARN_UNUSED_RESULT
PARROT_CANNOT_RETURN_NULL
static Instruction * move_regs(PARROT_INTERP,
    ARGIN(IMC_Unit *unit),
    ARGIN(Instruction *ins),
    int n,
    ARGIN(SymReg **dest),
    ARGIN(SymReg **src))
        __attribute__nonnull__(1)
        __attribute__nonnull__(2)
        __attribute__nonnull__(3)
        __attribute__nonnull__(5)
        __attribute__nonnull__(6);

PARROT_WARN_UNUSED_RESULT
PARROT_CAN_RETURN_NULL
static Instruction* pcc_get_args(PARROT_INTERP,
    ARGMOD(IMC_Unit *unit),
    ARGIN(Instruction *ins),
    ARGIN(const char *op_name),
    int n,
    ARGIN_NULLOK(SymReg * const *args),
    ARGIN_NULLOK(const int *arg_flags))
        __attribute__nonnull__(1)
        __attribute__nonnull__(2)
        __attribute__nonnull__(3)
        __attribute__nonnull__(4)
        FUNC_MODIFIES(*unit);

static int pcc_reg_mov(PARROT_INTERP,
    unsigned char d,
    unsigned char s,
    ARGMOD(void *vinfo))
        __attribute__nonnull__(1)
        __attribute__nonnull__(4)
        FUNC_MODIFIES(*vinfo);

static int recursive_tail_call(PARROT_INTERP,
    ARGIN(IMC_Unit *unit),
    ARGIN(Instruction *ins),
    ARGIN(SymReg *sub))
        __attribute__nonnull__(1)
        __attribute__nonnull__(2)
        __attribute__nonnull__(3)
        __attribute__nonnull__(4);

static void unshift_self(ARGIN(SymReg *sub), ARGIN(SymReg *obj))
        __attribute__nonnull__(1)
        __attribute__nonnull__(2);

/* Don't modify between HEADERIZER BEGIN / HEADERIZER END.  Your changes will be lost. */
/* HEADERIZER END: static */

/*

=item C<static Instruction * insINS>

Utility instruction routine. Creates and inserts an instruction
into the current block in one call.

=cut

*/

PARROT_WARN_UNUSED_RESULT
PARROT_CAN_RETURN_NULL
static Instruction *
insINS(PARROT_INTERP, ARGMOD(IMC_Unit *unit), ARGIN(Instruction *ins),
        ARGIN(const char *name), ARGIN(SymReg **regs), int n)
{
    /* INS can return NULL, but insert_ins() cannot take one */
    Instruction * const tmp = INS(interp, unit, name, NULL, regs, n, 0, 0);
    if (tmp)
        insert_ins(unit, ins, tmp);
    return tmp;
}

/*

=item C<SymReg* get_pasm_reg>

get or create the SymReg

=cut

*/

PARROT_WARN_UNUSED_RESULT
PARROT_CANNOT_RETURN_NULL
SymReg*
get_pasm_reg(PARROT_INTERP, ARGIN(const char *name))
{
    SymReg * const r = _get_sym(&IMCC_INFO(interp)->cur_unit->hash, name);

    if (r)
        return r;

    return mk_pasm_reg(interp, name);
}

/*

=item C<SymReg* get_const>

get or create a constant

=cut

*/

PARROT_WARN_UNUSED_RESULT
PARROT_CANNOT_RETURN_NULL
SymReg*
get_const(PARROT_INTERP, ARGIN(const char *name), int type)
{
    SymReg * const r = _get_sym(&IMCC_INFO(interp)->ghash, name);

    if (r && r->set == type)
        return r;

    return mk_const(interp, name, type);
}

/*

=item C<static Instruction* pcc_get_args>

set arguments or return values
get params or results
used by expand_pcc_sub_call and expand_pcc_sub

=cut

*/

PARROT_WARN_UNUSED_RESULT
PARROT_CAN_RETURN_NULL
static Instruction*
pcc_get_args(PARROT_INTERP, ARGMOD(IMC_Unit *unit), ARGIN(Instruction *ins),
        ARGIN(const char *op_name), int n,
        ARGIN_NULLOK(SymReg * const *args), ARGIN_NULLOK(const int *arg_flags))
{
    int i, flags;
    char buf[1024], s[16];
    SymReg ** const regs  = mem_allocate_n_zeroed_typed(n + 1, SymReg *);

    strcpy(buf, "\"(");
    for (i = 0; i < n; i++) {
        SymReg *arg = args[i];

        if (arg->type & VT_CONSTP)
            arg = arg->reg;

        regs[i + 1] = arg;
        flags       = 0;

        if (arg_flags[i] & VT_FLAT)
            flags |= PARROT_ARG_FLATTEN;

        if (arg_flags[i] & VT_OPTIONAL)
            flags |= PARROT_ARG_OPTIONAL;
        else if (arg_flags[i] & VT_OPT_FLAG)
            flags |= PARROT_ARG_OPT_FLAG;

        if (arg_flags[i] & VT_NAMED)
            flags |= PARROT_ARG_NAME;

        /* add argument type bits */
        if (arg->type & VTCONST)
            flags |= PARROT_ARG_CONSTANT;

        /* TODO verify if const is allowed */
        switch (arg->set) {
            case 'I':                               break;
            case 'S': flags |= PARROT_ARG_STRING;   break;
            case 'N': flags |= PARROT_ARG_FLOATVAL; break;
            case 'K':
            case 'P': flags |= PARROT_ARG_PMC;      break;
            default :                               break;
        }

        snprintf(s, sizeof (s), "0x%x", flags);

        if (i < n - 1)
            strcat(s, ",");
        strcat(buf, s);         /* XXX check avail len */
    }

    strcat(buf, ")\"");

    regs[0] = mk_const(interp, buf, 'S');
    ins     = insINS(interp, unit, ins, op_name, regs, n + 1);

    mem_sys_free(regs);
    return ins;
}

/*

=item C<static void unshift_self>

prepend the object to args or self to params

=cut

*/

static void
unshift_self(ARGIN(SymReg *sub), ARGIN(SymReg *obj))
{
    struct pcc_sub_t * const pcc_sub = sub->pcc_sub;
    const int                n       = pcc_sub->nargs;
    int                      i;

    mem_realloc_n_typed(pcc_sub->args,      n + 1, SymReg *);
    mem_realloc_n_typed(pcc_sub->arg_flags, n + 1, int);

    for (i = n; i; --i) {
        pcc_sub->args[i]      = pcc_sub->args[i - 1];
        pcc_sub->arg_flags[i] = pcc_sub->arg_flags[i - 1];
    }

    pcc_sub->args[0]      = obj;
    pcc_sub->arg_flags[0] = 0;
    pcc_sub->nargs++;
}

/*

=item C<void expand_pcc_sub>

Expand a PCC (Parrot Calling Convention) subroutine
by generating the appropriate prologue and epilogue
for parameter passing/returning.

=cut

*/

void
expand_pcc_sub(PARROT_INTERP, ARGMOD(IMC_Unit *unit), ARGIN(Instruction *ins))
{
    int          nargs;
    SymReg      *sub = ins->symregs[0];
    SymReg      *regs[2];

    /* if this sub is a method, unshift 'self' as first param */
    if (sub->pcc_sub->pragma & P_METHOD) {
        SymReg *self = get_sym(interp, "self");
        if (!self) {
            self       = mk_symreg(interp, "self", 'P');
            self->type = VTIDENTIFIER;
        }
        unshift_self(sub, self);
    }

    /* Don't generate any parameter checking code if there
     * are no named arguments.  */
    nargs = sub->pcc_sub->nargs;

    if (nargs)
        ins = pcc_get_args(interp, unit, ins, "get_params", nargs,
                sub->pcc_sub->args, sub->pcc_sub->arg_flags);

    /* check if there is a return */
    if (unit->last_ins->type          & (ITPCCSUB)
    &&  unit->last_ins->symreg_count == 1
    && (sub = unit->last_ins->symregs[0])
    &&  sub->pcc_sub
    && !sub->pcc_sub->object
       /* s. src/inter_call.c:119 */
    && (sub->pcc_sub->flags & isTAIL_CALL))
        return;

    if (unit->last_ins->type != (ITPCCSUB|ITLABEL)
    && STRNEQ(unit->last_ins->opname, "ret")
    && STRNEQ(unit->last_ins->opname, "exit")
    && STRNEQ(unit->last_ins->opname, "end")
    && STRNEQ(unit->last_ins->opname, "branch")
       /* was adding rets multiple times... */
    && STRNEQ(unit->last_ins->opname, "returncc")) {
        Instruction *tmp;

        /* check to make sure the sub is ok before we try to use it */
        if (!sub)
            real_exception(interp, NULL, 1, "NULL sub detected");

        if (!sub->pcc_sub)
            real_exception(interp, NULL, 1, "NULL sub->pcc_sub detected");

        if (sub->pcc_sub->pragma & P_MAIN)
            tmp = INS(interp, unit, "end", NULL, regs, 0, 0, 0);
        else {
            Instruction *unused_ins = pcc_get_args(interp, unit, unit->last_ins,
                    "set_returns", 0, NULL, NULL);
            UNUSED(unused_ins);
            tmp = INS(interp, unit, "returncc", NULL, regs, 0, 0, 0);
        }

        IMCC_debug(interp, DEBUG_IMC, "add sub ret - %I\n", tmp);
        insert_ins(unit, unit->last_ins, tmp);
    }
}

/*

=item C<void expand_pcc_sub_ret>

Expand a PCC sub return directive into its PASM instructions

=cut

*/

void
expand_pcc_sub_ret(PARROT_INTERP, ARGMOD(IMC_Unit *unit), ARGIN(Instruction *ins))
{
    const int is_yield = ins->type & ITPCCYIELD;
    SymReg * const sub = ins->symregs[0];
    const int n        = sub->pcc_sub->nret;

    /* TODO implement return conventions */
    ins = pcc_get_args(interp, unit, ins, "set_returns", n,
                       sub->pcc_sub->ret, sub->pcc_sub->ret_flags);

    /* we have a pcc_begin_yield */
    if (is_yield) {
        SymReg *regs[2];
        ins        = insINS(interp, unit, ins, "yield", regs, 0);
        ins->type |= ITPCCYIELD;
    }
    else {
        SymReg *regs[2];
        /* insert return invoke */
        ins        = insINS(interp, unit, ins, "returncc", regs, 0);
        ins->type |= ITPCCRET;
    }
}

typedef struct move_info_t {
    IMC_Unit    *unit;
    NOTNULL(Instruction *ins);
    int          n;
    SymReg      **dest;
    SymReg      **src;
} move_info_t;

/*

=item C<static int pcc_reg_mov>

RT #48260: Not yet documented!!!

=cut

*/

static int
pcc_reg_mov(PARROT_INTERP, unsigned char d, unsigned char s, ARGMOD(void *vinfo))
{
    static const char types[] = "INSP";
    static SymReg    *temps[4];
    move_info_t      *info    = (move_info_t *)vinfo;
    SymReg           *src     = NULL;
    SymReg           *dest    = NULL;
    SymReg           *regs[2];

    if (d == 255) {
        int t;

        /* handle temp use/create temp of src type */
        PARROT_ASSERT(s != 255);
        PARROT_ASSERT(s < 2 * info->n);

        src = s < info->n ? info->dest[(int)s] : info->src[(int)s - info->n];

        for (t = 0; t < 4; ++t) {
            if (types[t] == src->set) {
                if (temps[t])
                    dest = temps[t];
                else {
                    dest = temps[t] = mk_temp_reg(interp, src->set);
                }
                break;
            }

        }
    }
    else if (s == 255) {
        int t;
        /* handle temp use/create temp of dest type */
        PARROT_ASSERT(d < 2 * info->n);

        dest = d < info->n ? info->dest[(int)d] : info->src[(int)d - info->n];

        for (t = 0; t < 4; ++t) {
            if (types[t] == dest->set) {
                if (!temps[t])
                    temps[t] = mk_temp_reg(interp, dest->set);
                src = temps[t];
                break;
            }
        }
    }

    if (!dest)
        dest = d < info->n ? info->dest[(int)d] : info->src[(int)d - info->n];

    if (!src)
        src = s < info->n ? info->dest[(int)s] : info->src[(int)s - info->n];

    regs[0]   = dest;
    regs[1]   = src;
    info->ins = insINS(interp, info->unit, info->ins, "set", regs, 2);

    return 1;
}

/*

=item C<static Instruction * move_regs>

RT #48260: Not yet documented!!!

=cut

*/

PARROT_WARN_UNUSED_RESULT
PARROT_CANNOT_RETURN_NULL
static Instruction *
move_regs(PARROT_INTERP, ARGIN(IMC_Unit *unit), ARGIN(Instruction *ins),
        int n, ARGIN(SymReg **dest), ARGIN(SymReg **src))
{
    unsigned char *move_list;
    move_info_t    move_info;
    int            i;

    if (!n)
        return ins;

    move_list      = (unsigned char *)mem_sys_allocate(2 * n);
    move_info.unit = unit;
    move_info.ins  = ins;
    move_info.n    = n;
    move_info.dest = dest;
    move_info.src  = src;

    memset(move_list, -1, 2 * n);

    for (i = 0; i < 2 * n; ++i) {
        const SymReg * const ri = i < n ? dest[i] : src[i - n];
        int j;
        for (j = 0; j < i; ++j) {
            const SymReg * const rj = j < n ? dest[j] : src[j - n];
            if (ri == rj) {
                PARROT_ASSERT(j < 255);
                move_list[i] = (unsigned char)j;
                goto done;
            }
        }
        PARROT_ASSERT(i < 255);
        move_list[i] = (unsigned char)i;
done:
        ;
    }

    Parrot_register_move(interp, n, move_list, move_list + n, 255,
        pcc_reg_mov, NULL, &move_info);

    mem_sys_free(move_list);
    return move_info.ins;
}

/*

=item C<static int recursive_tail_call>

convert a recursive tailcall into a loop

=cut

*/

static int
recursive_tail_call(PARROT_INTERP, ARGIN(IMC_Unit *unit),
        ARGIN(Instruction *ins), ARGIN(SymReg *sub))
{
    SymReg *called_sub, *this_sub, *label;
    SymReg *regs[2];
    Instruction *get_params, *tmp_ins, *unused_ins;
    char *buf;

    if (!(unit->instructions->type & ITLABEL))
        return 0;

    this_sub = unit->instructions->symregs[0];

    if (!this_sub)
        return 0;

    called_sub = sub->pcc_sub->sub;

    if (STRNEQ(this_sub->name, called_sub->name))
        return 0;

    if (sub->pcc_sub->nargs != this_sub->pcc_sub->nargs)
        return 0;

    /* TODO check if we have only positional args */

    get_params = unit->instructions->next;

    if (get_params->opnum != PARROT_OP_get_params_pc)
        return 0;

    buf = (char *)malloc(strlen(this_sub->name) + 3);
    sprintf(buf, "%s@0", this_sub->name);

    if (!(label = find_sym(interp, buf))) {
        label   = mk_local_label(interp, buf);
        tmp_ins = INS_LABEL(interp, unit, label, 0);
        insert_ins(unit, get_params, tmp_ins);
    }

    free(buf);

    ins = move_regs(interp, unit, ins, sub->pcc_sub->nargs,
            this_sub->pcc_sub->args, sub->pcc_sub->args);

    regs[0]    = label;
    unused_ins = insINS(interp, unit, ins, "branch", regs, 1);
    UNUSED(unused_ins);

    return 1;
}

/*

=item C<static void insert_tail_call>

RT #48260: Not yet documented!!!

=cut

*/

static void
insert_tail_call(PARROT_INTERP, ARGIN(IMC_Unit *unit), ARGMOD(Instruction *ins),
        ARGMOD(SymReg *sub), ARGIN(SymReg *meth))
{
    SymReg *regs[2];

    if (meth) {
        regs[0] = sub->pcc_sub->object;
        regs[1] = meth;
        ins     = insINS(interp, unit, ins, "tailcallmethod", regs, 2);
    }
    else {
        regs[0] = sub->pcc_sub->sub;
        ins     = insINS(interp, unit, ins, "tailcall", regs, 1);
    }

    regs[0]->pcc_sub  = sub->pcc_sub;
    sub->pcc_sub      = NULL;
    ins->type        |= ITPCCSUB;
}

/*

=item C<void expand_pcc_sub_call>

Expand a PCC subroutine call (IMC) into its PASM instructions
This is the nuts and bolts of pdd03 routine call style

=cut

*/

void
expand_pcc_sub_call(PARROT_INTERP, ARGMOD(IMC_Unit *unit), ARGMOD(Instruction *ins))
{
    SymReg *arg, *reg, *regs[3];
    int          n;
    int          tail_call;
    int          meth_call = 0;
    SymReg      *meth      = NULL;
    Instruction *get_name;

    SymReg * const sub = ins->symregs[0];

    if (ins->type & ITRESULT) {
        const int n = sub->pcc_sub->nret;
        ins         = pcc_get_args(interp, unit, ins, "get_results", n,
                        sub->pcc_sub->ret, sub->pcc_sub->ret_flags);
        return;
    }

    tail_call = (sub->pcc_sub->flags & isTAIL_CALL);

    if (tail_call && IMCC_INFO(interp)->optimizer_level & OPT_SUB)
        if (recursive_tail_call(interp, unit, ins, sub))
            return;

    if (sub->pcc_sub->object) {
        meth_call = 1;
        if (sub->pcc_sub->object->set == 'S') {
            regs[0] = mk_temp_reg(interp, 'P');
            regs[1] = sub->pcc_sub->object;
            ins     = insINS(interp, unit, ins, "getclass", regs, 2);

            sub->pcc_sub->object = regs[0];
        }
    }

    /*
     * See if we need to create a temporary sub object for the short
     * function call syntax _f()
     */
    get_name = NULL;

    if (ins->type & ITCALL) {
        SymReg * const the_sub = sub->pcc_sub->sub;

        if (!meth_call && (the_sub->type & VTADDRESS)) {
            /* sub->pcc_sub->sub is an actual subroutine name, not a variable */
            reg = mk_temp_reg(interp, 'P');
            add_pcc_sub(sub, reg);

            /* insert set_p_pc with the sub as constant */
            the_sub->set    = 'p';
            the_sub->usage  = U_FIXUP;
            the_sub->type  &= ~VTADDRESS;
            the_sub->type  |= VTCONST;   /* preserve VT_ENCODED */
            regs[0]         = reg;
            regs[1]         = the_sub;

            /*
             * set_p_pc gets replaced in imcc/pbc.c, if the
             * function can't located in the current namespace
             */
            get_name   = INS(interp, unit, "set_p_pc", "", regs, 2, 0, 0);
            ins->type &= ~ITCALL;
        }
    }

    if (sub->pcc_sub->object)
        unshift_self(sub, sub->pcc_sub->object);

    /* insert arguments */
    n   = sub->pcc_sub->nargs;
    ins = pcc_get_args(interp, unit, ins, "set_args", n,
            sub->pcc_sub->args, sub->pcc_sub->arg_flags);

    /*
     * insert get_name after args have been setup, so that
     * a possible MMD call can inspect the passed arguments
     */
    if (get_name) {
        insert_ins(unit, ins, get_name);
        ins = get_name;
    }

    arg = sub->pcc_sub->sub;

    if (meth_call) {
        meth = arg;

        if (arg->set != 'P') {
            if (!(arg->type == VTIDENTIFIER
            ||    arg->type == VTPASM
            ||    arg->type == VTREG))
                meth = mk_const(interp, arg->name, 'S');
        }
    }

    /* if we have a tail call then insert a tailcall opcode */
    if (tail_call) {
        insert_tail_call(interp, unit, ins, sub, meth);
        return;
    }

    /* handle return results */
    n   = sub->pcc_sub->nret;
    ins = pcc_get_args(interp, unit, ins, "get_results", n,
            sub->pcc_sub->ret, sub->pcc_sub->ret_flags);

    /* insert the call */
    if (meth_call) {
        regs[0] = sub->pcc_sub->object;
        regs[1] = meth;
        arg     = sub->pcc_sub->cc;

        if (arg) {
            regs[2] = arg;
            ins     = insINS(interp, unit, ins, "callmethod" , regs, 3);
        }
        else {
            ins = insINS(interp, unit, ins, "callmethodcc" , regs, 2);
        }
    }
    else {
        regs[0] = sub->pcc_sub->sub;
        arg     = sub->pcc_sub->cc;

        if (arg) {
            regs[1] = arg;
            ins     = insINS(interp, unit, ins, "invoke" , regs, 2);
        }
        else {
            ins = insINS(interp, unit, ins, "invokecc" , regs, 1);
        }
    }

    ins->type |= ITPCCSUB;
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
