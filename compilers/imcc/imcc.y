%{
/*
 * imcc.y
 *
 * Intermediate Code Compiler for Parrot.
 *
 * Copyright (C) 2002 Melvin Smith <melvin.smith@mindspring.com>
 * Copyright (C) 2002-2008, The Perl Foundation.
 *
 * Grammar of the PIR language parser.
 *
 * $Id$
 *
 */

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#define _PARSER
#define PARSER_MAIN
#include "imc.h"
#include "parrot/dynext.h"
#include "pbc.h"
#include "parser.h"
#include "optimizer.h"

#ifndef YYENABLE_NLS
#  define YYENABLE_NLS 0
#endif

#ifndef YYLTYPE_IS_TRIVIAL
#  define YYLTYPE_IS_TRIVIAL 0
#endif

/* HEADERIZER HFILE: compilers/imcc/imc.h */

/* HEADERIZER BEGIN: static */
/* Don't modify between HEADERIZER BEGIN / HEADERIZER END.  Your changes will be lost. */

static void add_pcc_named_arg(PARROT_INTERP,
    ARGMOD(SymReg *cur_call),
    ARGIN(const char *name),
    ARGIN(SymReg *value))
        __attribute__nonnull__(1)
        __attribute__nonnull__(2)
        __attribute__nonnull__(3)
        __attribute__nonnull__(4)
        FUNC_MODIFIES(*cur_call);

static void add_pcc_named_param(PARROT_INTERP,
    ARGMOD(SymReg *cur_call),
    ARGIN(const char *name),
    ARGIN(SymReg *value))
        __attribute__nonnull__(1)
        __attribute__nonnull__(2)
        __attribute__nonnull__(3)
        __attribute__nonnull__(4)
        FUNC_MODIFIES(*cur_call);

static void add_pcc_named_result(PARROT_INTERP,
    ARGMOD(SymReg *cur_call),
    ARGIN(const char *name),
    ARGIN(SymReg *value))
        __attribute__nonnull__(1)
        __attribute__nonnull__(2)
        __attribute__nonnull__(3)
        __attribute__nonnull__(4)
        FUNC_MODIFIES(*cur_call);

static void add_pcc_named_return(PARROT_INTERP,
    ARGMOD(SymReg *cur_call),
    ARGIN(const char *name),
    ARGIN(SymReg *value))
        __attribute__nonnull__(1)
        __attribute__nonnull__(2)
        __attribute__nonnull__(3)
        __attribute__nonnull__(4)
        FUNC_MODIFIES(*cur_call);

static void adv_named_set(PARROT_INTERP, ARGIN(char *name))
        __attribute__nonnull__(1)
        __attribute__nonnull__(2);

static void begin_return_or_yield(PARROT_INTERP, int yield)
        __attribute__nonnull__(1);

static void clear_state(PARROT_INTERP)
        __attribute__nonnull__(1);

static void do_loadlib(PARROT_INTERP, ARGIN(const char *lib))
        __attribute__nonnull__(1)
        __attribute__nonnull__(2);

PARROT_WARN_UNUSED_RESULT
PARROT_CAN_RETURN_NULL
static Instruction* func_ins(PARROT_INTERP,
    ARGMOD(IMC_Unit *unit),
    ARGIN(SymReg *lhs),
    ARGIN(const char *op),
    ARGMOD(SymReg **r),
    int n,
    int keyv,
    int emit)
        __attribute__nonnull__(1)
        __attribute__nonnull__(2)
        __attribute__nonnull__(3)
        __attribute__nonnull__(4)
        __attribute__nonnull__(5)
        FUNC_MODIFIES(*unit)
        FUNC_MODIFIES(*r);

PARROT_CAN_RETURN_NULL
static Instruction * iINDEXFETCH(PARROT_INTERP,
    ARGMOD(IMC_Unit *unit),
    ARGIN(SymReg *r0),
    ARGIN(SymReg *r1),
    ARGIN(SymReg *r2))
        __attribute__nonnull__(1)
        __attribute__nonnull__(2)
        __attribute__nonnull__(3)
        __attribute__nonnull__(4)
        __attribute__nonnull__(5)
        FUNC_MODIFIES(*unit);

PARROT_CAN_RETURN_NULL
static Instruction * iINDEXSET(PARROT_INTERP,
    ARGMOD(IMC_Unit *unit),
    ARGIN(SymReg *r0),
    ARGIN(SymReg *r1),
    ARGIN(SymReg *r2))
        __attribute__nonnull__(1)
        __attribute__nonnull__(2)
        __attribute__nonnull__(3)
        __attribute__nonnull__(4)
        __attribute__nonnull__(5)
        FUNC_MODIFIES(*unit);

PARROT_CANNOT_RETURN_NULL
static Instruction * iLABEL(PARROT_INTERP,
    ARGMOD_NULLOK(IMC_Unit *unit),
    ARGMOD(SymReg *r0))
        __attribute__nonnull__(1)
        __attribute__nonnull__(3)
        FUNC_MODIFIES(*r0);

PARROT_WARN_UNUSED_RESULT
PARROT_CAN_RETURN_NULL
static const char * inv_op(ARGIN(const char *op))
        __attribute__nonnull__(1);

PARROT_CANNOT_RETURN_NULL
static Instruction * iSUBROUTINE(PARROT_INTERP,
    ARGMOD_NULLOK(IMC_Unit *unit),
    ARGMOD(SymReg *r))
        __attribute__nonnull__(1)
        __attribute__nonnull__(3)
        FUNC_MODIFIES(*r);

PARROT_IGNORABLE_RESULT
PARROT_CAN_RETURN_NULL
static Instruction * MK_I(PARROT_INTERP,
    ARGMOD(IMC_Unit *unit),
    ARGIN(const char *fmt),
    int n,
    ...)
        __attribute__nonnull__(1)
        __attribute__nonnull__(2)
        __attribute__nonnull__(3)
        FUNC_MODIFIES(*unit);

PARROT_WARN_UNUSED_RESULT
PARROT_CAN_RETURN_NULL
static Instruction* mk_pmc_const(PARROT_INTERP,
    ARGMOD(IMC_Unit *unit),
    ARGIN(const char *type),
    ARGMOD(SymReg *left),
    ARGIN(const char *constant))
        __attribute__nonnull__(1)
        __attribute__nonnull__(2)
        __attribute__nonnull__(3)
        __attribute__nonnull__(4)
        __attribute__nonnull__(5)
        FUNC_MODIFIES(*unit)
        FUNC_MODIFIES(*left);

PARROT_CANNOT_RETURN_NULL
static SymReg * mk_sub_address_fromc(PARROT_INTERP, ARGIN(const char *name))
        __attribute__nonnull__(1)
        __attribute__nonnull__(2);

PARROT_CANNOT_RETURN_NULL
static SymReg * mk_sub_address_u(PARROT_INTERP, ARGIN(const char *name))
        __attribute__nonnull__(1)
        __attribute__nonnull__(2);

static void set_lexical(PARROT_INTERP,
    ARGMOD(SymReg *r),
    ARGIN(const char *name))
        __attribute__nonnull__(1)
        __attribute__nonnull__(2)
        __attribute__nonnull__(3)
        FUNC_MODIFIES(*r);

/* Don't modify between HEADERIZER BEGIN / HEADERIZER END.  Your changes will be lost. */
/* HEADERIZER END: static */


#define YYDEBUG 1
#define YYERROR_VERBOSE 1

/*
 * We use a pure parser with the interpreter as a parameter. However this still
 * doesn't make the parser reentrant, as there are too many globals around.
 */

/*
 * Choosing instructions for Parrot is pretty easy since many are
 * polymorphic.
 */


/*
 * MK_I: build and emitb instruction by INS
 *
 * fmt may contain:
 *   op %s, %s # comment
 * or just
 *   op
 *
 *
 *  NOTE: Most usage of this function is with
 *        IMCC_INFO(interp)->cur_unit, but there are some
 *        exceptions. Thus, we can't easily factorize that piece of
 *        code.
 */

PARROT_IGNORABLE_RESULT
PARROT_CAN_RETURN_NULL
static Instruction *
MK_I(PARROT_INTERP, ARGMOD(IMC_Unit *unit), ARGIN(const char *fmt), int n, ...)
{
    char opname[64];
    char *p;
    const char *q;
    va_list ap;
    SymReg *r[IMCC_MAX_FIX_REGS];
    int i;

    for (p = opname, q = fmt; *q && *q != ' ';)
        *p++ = *q++;
    *p = 0;
    if (!*q)
        fmt = NULL;
    else
        fmt = ++q;
#ifdef OPDEBUG
    fprintf(stderr, "op '%s' format '%s' (%d)\n", opname, fmt?:"", n);
#endif
    va_start(ap, n);
    i = 0;
    for (i = 0; i < n; ++i) {
        r[i] = va_arg(ap, SymReg *);
    }
    va_end(ap);
    return INS(interp, unit, opname, fmt, r, n,
               IMCC_INFO(interp)->keyvec, 1);
}

PARROT_WARN_UNUSED_RESULT
PARROT_CAN_RETURN_NULL
static Instruction*
mk_pmc_const(PARROT_INTERP, ARGMOD(IMC_Unit *unit), ARGIN(const char *type),
        ARGMOD(SymReg *left), ARGIN(const char *constant))
{
    const int type_enum = atoi(type);
    const int ascii = (*constant == '\'' || *constant == '"');
    SymReg *rhs;
    SymReg *r[2];
    char   *name;

    if (left->type == VTADDRESS) {      /* IDENTIFIER */
        if (IMCC_INFO(interp)->state->pasm_file) {
            IMCC_fataly(interp, E_SyntaxError,
                        "Ident as PMC constant",
                        " %s\n", left->name);
        }
        left->type = VTIDENTIFIER;
        left->set = 'P';
    }
    r[0] = left;
    if (ascii) {
        /* strip delimiters */
        name                   = str_dup(constant + 1);
        name[strlen(name) - 1] = '\0';
    }
    else {
        name = str_dup(constant);
    }

    switch (type_enum) {
        case enum_class_Sub:
        case enum_class_Coroutine:
            rhs = mk_const(interp, name, 'p');

            if (!ascii)
                rhs->type |= VT_ENCODED;

            rhs->usage    = U_FIXUP;
            break;
        default:
            rhs = mk_const(interp, name, 'P');
            break;
    }

    r[1]          = rhs;
    rhs->pmc_type = type_enum;

    mem_sys_free(name);

    return INS(interp, unit, "set_p_pc", "", r, 2, 0, 1);
}

PARROT_WARN_UNUSED_RESULT
PARROT_CAN_RETURN_NULL
static Instruction*
func_ins(PARROT_INTERP, ARGMOD(IMC_Unit *unit), ARGIN(SymReg *lhs), ARGIN(const char *op),
         ARGMOD(SymReg **r), int n, int keyv, int emit)
{
    int i;
    /* shift regs up by 1 */
    for (i = n - 1; i >= 0; --i)
        r[i+1] = r[i];
    r[0] = lhs;
    /* shift keyvec */
    keyv <<= 1;
    return INS(interp, unit, op, "", r, n+1, keyv, emit);
}
/*
 * special instructions
 *
 * labels and such
 */

static void
clear_state(PARROT_INTERP)
{
    IMCC_INFO(interp) -> nargs = 0;
    IMCC_INFO(interp) -> keyvec = 0;
}

PARROT_CANNOT_RETURN_NULL
Instruction *
INS_LABEL(PARROT_INTERP, ARGMOD_NULLOK(IMC_Unit *unit), ARGMOD(SymReg *r0), int emit)
{

    Instruction * const ins = _mk_instruction("", "%s:", 1, &r0, 0);
    ins->type               = ITLABEL;
    r0->first_ins           = ins;

    if (emit)
        emitb(interp, unit, ins);

    return ins;
}

PARROT_CANNOT_RETURN_NULL
static Instruction *
iLABEL(PARROT_INTERP, ARGMOD_NULLOK(IMC_Unit *unit), ARGMOD(SymReg *r0))
{
    Instruction * const i = INS_LABEL(interp, unit, r0, 1);
    i->line               = IMCC_INFO(interp)->line;

    clear_state(interp);
    return i;
}

PARROT_CANNOT_RETURN_NULL
static Instruction *
iSUBROUTINE(PARROT_INTERP, ARGMOD_NULLOK(IMC_Unit *unit), ARGMOD(SymReg *r))
{
    Instruction * const i = iLABEL(interp, unit, r);

    r->type    = (r->type & VT_ENCODED) ? VT_PCC_SUB|VT_ENCODED : VT_PCC_SUB;
    r->pcc_sub = mem_allocate_zeroed_typed(pcc_sub_t);

    IMCC_INFO(interp)->cur_call = r;
    i->line                     = IMCC_INFO(interp)->line;

    add_namespace(interp, unit);
    return i;
}

/*
 * substr or X = P[key]
 */
PARROT_CAN_RETURN_NULL
static Instruction *
iINDEXFETCH(PARROT_INTERP, ARGMOD(IMC_Unit *unit), ARGIN(SymReg *r0), ARGIN(SymReg *r1),
        ARGIN(SymReg *r2))
{
    if (r0->set == 'S' && r1->set == 'S' && r2->set == 'I') {
        SymReg * const r3 = mk_const(interp, "1", 'I');
        return MK_I(interp, unit, "substr %s, %s, %s, 1", 4, r0, r1, r2, r3);
    }

    IMCC_INFO(interp) -> keyvec |= KEY_BIT(2);
    return MK_I(interp, unit, "set %s, %s[%s]", 3, r0, r1, r2);
}

/*
 * substr or P[key] = X
 */

PARROT_CAN_RETURN_NULL
static Instruction *
iINDEXSET(PARROT_INTERP, ARGMOD(IMC_Unit *unit), ARGIN(SymReg *r0), ARGIN(SymReg *r1),
        ARGIN(SymReg *r2))
{
    if (r0->set == 'S' && r1->set == 'I' && r2->set == 'S') {
        SymReg * const r3 = mk_const(interp, "1", 'I');
        MK_I(interp, unit, "substr %s, %s, %s, %s", 4, r0, r1, r3, r2);
    }
    else if (r0->set == 'P') {
        IMCC_INFO(interp)->keyvec |= KEY_BIT(1);
        MK_I(interp, unit, "set %s[%s], %s", 3, r0, r1, r2);
    }
    else {
        IMCC_fataly(interp, E_SyntaxError,
            "unsupported indexed set op\n");
    }
    return NULL;
}

PARROT_WARN_UNUSED_RESULT
PARROT_CAN_RETURN_NULL
static const char *
inv_op(ARGIN(const char *op))
{
    int n;
    return get_neg_op(op, &n);
}

PARROT_CANNOT_RETURN_NULL
Instruction *
IMCC_create_itcall_label(PARROT_INTERP)
{
    char         name[128];
    SymReg      *r;
    Instruction *i;

    snprintf(name, sizeof (name), "%cpcc_sub_call_%d", IMCC_INTERNAL_CHAR,
        IMCC_INFO(interp)->cnr++);

    r       = mk_pcc_sub(interp, name, 0);
    i       = iLABEL(interp, IMCC_INFO(interp)->cur_unit, r);
    i->type = ITCALL | ITPCCSUB;

    IMCC_INFO(interp)->cur_call = r;

    return i;
}

PARROT_CANNOT_RETURN_NULL
static SymReg *
mk_sub_address_fromc(PARROT_INTERP, ARGIN(const char *name))
{
    /* name is a quoted sub name */
    SymReg *r;
    char *name_copy                  = str_dup(name + 1);
    name_copy[strlen(name_copy) - 1] = '\0';

    r = mk_sub_address(interp, name_copy);
    mem_sys_free(name_copy);

    return r;
}

PARROT_CANNOT_RETURN_NULL
static SymReg *
mk_sub_address_u(PARROT_INTERP, ARGIN(const char *name))
{
    SymReg * const r = mk_sub_address(interp, name);
    r->type         |= VT_ENCODED;

    return r;
}

void
IMCC_itcall_sub(PARROT_INTERP, ARGIN(SymReg *sub))
{
    IMCC_INFO(interp)->cur_call->pcc_sub->sub = sub;
    if (IMCC_INFO(interp)->cur_obj) {
        if (IMCC_INFO(interp)->cur_obj->set != 'P')
            IMCC_fataly(interp, E_SyntaxError, "object isn't a PMC");
        IMCC_INFO(interp)->cur_call->pcc_sub->object = IMCC_INFO(interp)->cur_obj;
        IMCC_INFO(interp)->cur_obj = NULL;
    }
    if (IMCC_INFO(interp)->cur_call->pcc_sub->sub->pmc_type == enum_class_NCI)
        IMCC_INFO(interp)->cur_call->pcc_sub->flags |= isNCI;
    if (IMCC_INFO(interp)->cur_unit->type == IMC_PCCSUB)
        IMCC_INFO(interp)->cur_unit->instructions->symregs[0]->pcc_sub->calls_a_sub |= 1;
}

static void
begin_return_or_yield(PARROT_INTERP, int yield)
{
    Instruction *i;
    Instruction * const ins = IMCC_INFO(interp)->cur_unit->instructions;
    char                name[128];

    if (!ins || !ins->symregs[0] || !(ins->symregs[0]->type & VT_PCC_SUB))
        IMCC_fataly(interp, E_SyntaxError,
                    "yield or return directive outside pcc subroutine\n");
    if (yield)
       ins->symregs[0]->pcc_sub->calls_a_sub = 1 | ITPCCYIELD;
    snprintf(name, sizeof (name), yield ? "%cpcc_sub_yield_%d" : "%cpcc_sub_ret_%d",
            IMCC_INTERNAL_CHAR, IMCC_INFO(interp)->cnr++);
    interp->imc_info->sr_return = mk_pcc_sub(interp, name, 0);
    i = iLABEL(interp, IMCC_INFO(interp)->cur_unit, interp->imc_info->sr_return);
    i->type = yield ? ITPCCSUB | ITLABEL | ITPCCYIELD : ITPCCSUB | ITLABEL ;
    interp->imc_info->asm_state = yield ? AsmInYield : AsmInReturn;
}

static void
set_lexical(PARROT_INTERP, ARGMOD(SymReg *r), ARGIN(const char *name))
{
    SymReg * const n = mk_const(interp, name, 'S');

    r->usage |= U_LEXICAL;

    if (n == r->reg)
        IMCC_fataly(interp, E_SyntaxError,
            "register %s already declared as lexical %s", r->name, n->name);

    /* chain all names in r->reg */
    n->reg = r->reg;
    r->reg = n;
    r->use_count++;
}

static void
add_pcc_named_arg(PARROT_INTERP, ARGMOD(SymReg *cur_call), ARGIN(const char *name),
        ARGIN(SymReg *value))
{
    SymReg * const r = mk_const(interp, name, 'S');

    r->type  |= VT_NAMED;

    add_pcc_arg(cur_call, r);
    add_pcc_arg(cur_call, value);
}

static void
add_pcc_named_result(PARROT_INTERP, ARGMOD(SymReg *cur_call), ARGIN(const char *name),
        ARGIN(SymReg *value))
{
    SymReg * const r = mk_const(interp, name, 'S');
    r->type         |= VT_NAMED;

    add_pcc_result(cur_call, r);
    add_pcc_result(cur_call, value);
}

static void
add_pcc_named_param(PARROT_INTERP, ARGMOD(SymReg *cur_call), ARGIN(const char *name),
        ARGIN(SymReg *value))
{
    SymReg * const r = mk_const(interp, name, 'S');
    r->type         |= VT_NAMED;

    add_pcc_arg(cur_call, r);
    add_pcc_arg(cur_call, value);
}

static void
add_pcc_named_return(PARROT_INTERP, ARGMOD(SymReg *cur_call), ARGIN(const char *name),
        ARGIN(SymReg *value))
{
    SymReg * const r = mk_const(interp, name, 'S');
    r->type         |= VT_NAMED;

    add_pcc_result(cur_call, r);
    add_pcc_result(cur_call, value);
}

/* XXX Can name be consted? */
static void
adv_named_set(PARROT_INTERP, ARGIN(char *name))
{
    if (IMCC_INFO(interp)->adv_named_id) {
        IMCC_fataly(interp, E_SyntaxError,
                    "Named parameter with more than one name.\n");
    }
    IMCC_INFO(interp)->adv_named_id = name;
}

static void
do_loadlib(PARROT_INTERP, ARGIN(const char *lib))
{
    STRING * const s = string_unescape_cstring(interp, lib + 1, '"', NULL);
    PMC    *ignored  = Parrot_load_lib(interp, s, NULL);
    UNUSED(ignored);
    Parrot_register_HLL_lib(interp, s);
}

/* HEADERIZER STOP */

%}

%union {
    IdList * idlist;
    int t;
    char * s;
    SymReg * sr;
    Instruction *i;
}

/* We need precedence for a few tokens to resolve a couple of conflicts */
%nonassoc LOW_PREC
%nonassoc '\n'
%nonassoc <t> PARAM

%token <t> PRAGMA N_OPERATORS HLL HLL_MAP
%token <t> GOTO ARG IF UNLESS PNULL
%token <t> ADV_FLAT ADV_SLURPY ADV_OPTIONAL ADV_OPT_FLAG ADV_NAMED ADV_ARROW
%token <t> NEW ADV_INVOCANT
%token <t> NAMESPACE ENDNAMESPACE DOT_METHOD
%token <t> SUB SYM LOCAL LEXICAL CONST
%token <t> INC DEC GLOBAL_CONST
%token <t> PLUS_ASSIGN MINUS_ASSIGN MUL_ASSIGN DIV_ASSIGN CONCAT_ASSIGN
%token <t> BAND_ASSIGN BOR_ASSIGN BXOR_ASSIGN FDIV FDIV_ASSIGN MOD_ASSIGN
%token <t> SHR_ASSIGN SHL_ASSIGN SHR_U_ASSIGN
%token <t> SHIFT_LEFT SHIFT_RIGHT INTV FLOATV STRINGV PMCV LOG_XOR
%token <t> RELOP_EQ RELOP_NE RELOP_GT RELOP_GTE RELOP_LT RELOP_LTE
%token <t> GLOBAL GLOBALOP ADDR RESULT RETURN YIELDT GET_RESULTS
%token <t> POW SHIFT_RIGHT_U LOG_AND LOG_OR
%token <t> COMMA ESUB DOTDOT
%token <t> PCC_BEGIN PCC_END PCC_CALL PCC_SUB PCC_BEGIN_RETURN PCC_END_RETURN
%token <t> PCC_BEGIN_YIELD PCC_END_YIELD NCI_CALL METH_CALL INVOCANT
%token <t> MAIN LOAD INIT IMMEDIATE POSTCOMP METHOD ANON OUTER NEED_LEX
%token <t> MULTI VTABLE_METHOD LOADLIB SUB_INSTANCE_OF SUB_LEXID
%token <t> UNIQUE_REG
%token <s> LABEL
%token <t> EMIT EOM
%token <s> IREG NREG SREG PREG IDENTIFIER REG MACRO ENDM
%token <s> STRINGC INTC FLOATC USTRINGC
%token <s> PARROT_OP
%type <t> type pragma_1 hll_def return_or_yield comma_or_goto opt_unique_reg
%type <i> program
%type <i> class_namespace
%type <i> constdef sub emit pcc_ret pcc_yield
%type <i> compilation_units compilation_unit pmc_const pragma
%type <s> classname relop any_string assign_op  bin_op  un_op
%type <i> labels _labels label  statement sub_call
%type <i> pcc_sub_call
%type <sr> sub_param sub_params pcc_arg pcc_result pcc_args pcc_results sub_param_type_def
%type <sr> pcc_returns pcc_return pcc_call arg arglist the_sub multi_type
%type <t> argtype_list argtype paramtype_list paramtype
%type <t> pcc_return_many
%type <t> proto sub_proto sub_proto_list multi multi_types outer
%type <t> vtable instanceof lexid
%type <i> instruction assignment conditional_statement labeled_inst opt_label op_assign
%type <i> if_statement unless_statement
%type <i> func_assign get_results
%type <i> opt_invocant
%type <sr> target targetlist reg const var string result
%type <sr> keylist keylist_force _keylist key maybe_ns
%type <sr> vars _vars var_or_i _var_or_i label_op sub_label_op sub_label_op_c
%type <i> pasmcode pasmline pasm_inst
%type <sr> pasm_args
%type <i> var_returns
%token <sr> VAR

%token <t> LINECOMMENT
%token <s> FILECOMMENT
%type <idlist> id_list id_list_id

%nonassoc CONCAT DOT


 /* %locations */
%pure_parser

/* Note that we pass interp last, because Bison only passes
   the last param to yyerror().  (Tested on bison <= 2.3)
*/
%parse-param {void *yyscanner}
%lex-param   {void *yyscanner}
%parse-param {Parrot_Interp interp}
%lex-param   {Parrot_Interp interp}

%start program

/* In effort to make the grammar readable but not militaristic, please space indent
   code blocks on 10 col boundaries and keep indentation same for all code blocks
   in a rule. Indent rule tokens | and ; to 4th col and sub rules 6th col
 */

%%

program:
     compilation_units    { if (yynerrs) YYABORT; $$ = 0; }
   ;

compilation_units:
     compilation_unit
   | compilation_units compilation_unit
   ;

compilation_unit:
     class_namespace           { $$ = $1; }
   | constdef                  { $$ = $1; }
   | sub
         {
           $$ = $1;
           imc_close_unit(interp, IMCC_INFO(interp)->cur_unit);
           IMCC_INFO(interp)->cur_unit = 0;
         }
   | emit
         {
           $$ = $1;
           imc_close_unit(interp, IMCC_INFO(interp)->cur_unit);
           IMCC_INFO(interp)->cur_unit = 0;
         }
   | MACRO '\n'                { $$ = 0; }
   | pragma                    { $$ = 0; }
   | '\n'                      { $$ = 0; }
   ;

pragma:
     PRAGMA pragma_1 '\n'      { $$ = 0; }
   | hll_def         '\n'      { $$ = 0; }
   | LOADLIB STRINGC '\n'
         {
           $$ = 0;
           do_loadlib(interp, $2);
           mem_sys_free($2);
         }
   ;

pragma_1:
     N_OPERATORS INTC
         {
           if ($2)
               IMCC_INFO(interp)->state->pragmas |= PR_N_OPERATORS;
           else
               IMCC_INFO(interp)->state->pragmas &= ~PR_N_OPERATORS;
         }
   ;

hll_def:
     HLL STRINGC COMMA STRINGC
         {
            STRING * const hll_name = string_unescape_cstring(interp, $2 + 1, '"', NULL);
            CONTEXT(interp)->current_HLL =
                Parrot_register_HLL(interp, hll_name);

            /* don't bother loading the library for an empty string */
            if (strlen($4) > 2) {
                STRING * const hll_lib =
                    string_unescape_cstring(interp, $4 + 1, '"', NULL);
                PMC    *ignored        = Parrot_load_lib(interp, hll_lib, NULL);
                UNUSED(ignored);
                Parrot_register_HLL_lib(interp, hll_lib);
            }

            IMCC_INFO(interp)->cur_namespace = NULL;
            $$ = 0;
         }
   | HLL_MAP STRINGC COMMA STRINGC
         {
            Parrot_Context *ctx           = CONTEXT(interp);
            STRING * const  built_in_name =
                string_unescape_cstring(interp, $2 + 1, '"', NULL);
            STRING * const language_name  =
                string_unescape_cstring(interp, $4 + 1, '"', NULL);

            int             built_in_type = pmc_type(interp, built_in_name);
            int             language_type = pmc_type(interp, language_name);

            Parrot_register_HLL_type(interp, ctx->current_HLL,
                built_in_type, language_type);
            $$ = 0;
         }
   ;

constdef:
     CONST { is_def = 1; } type IDENTIFIER '=' const
         {
             mk_const_ident(interp, $4, $3, $6, 1);
             mem_sys_free($4);
             is_def = 0;
         }
   ;

pmc_const:
     CONST { is_def=1; } INTC var_or_i '=' any_string
         {
           $$ = mk_pmc_const(interp, IMCC_INFO(interp)->cur_unit, $3, $4, $6);
           is_def = 0;
         }
   ;
any_string:
     STRINGC
   | USTRINGC
   ;

pasmcode:
     pasmline
   | pasmcode pasmline
   ;

pasmline:
     labels  pasm_inst '\n'    { $$ = 0;  }
   | MACRO '\n'                { $$ = 0;  }
   | FILECOMMENT               { $$ = 0;  }
   | LINECOMMENT               { $$ = 0;  }
   | class_namespace           { $$ = $1; }
   | pmc_const
   | pragma
   ;

pasm_inst:                     { clear_state(interp); }
     PARROT_OP pasm_args
         {
           $$ = INS(interp, IMCC_INFO(interp)->cur_unit,
                    $2, 0, IMCC_INFO(interp)->regs,
                    IMCC_INFO(interp)->nargs, IMCC_INFO(interp) -> keyvec, 1);
           /* XXX: can't seem to mem_sys_free($1) here */
         }
   | PCC_SUB
         {
           imc_close_unit(interp, IMCC_INFO(interp)->cur_unit);
           IMCC_INFO(interp)->cur_unit = imc_open_unit(interp, IMC_PASM);
         }
     sub_proto LABEL
         {
           $$ = iSUBROUTINE(interp,
                    IMCC_INFO(interp)->cur_unit,
                    mk_sub_label(interp, $4));
           IMCC_INFO(interp)->cur_call->pcc_sub->pragma = $3;
         }
   | PNULL var
         {
           $$ = MK_I(interp, IMCC_INFO(interp)->cur_unit, "null", 1, $2);
         }
   | LEXICAL STRINGC COMMA REG
         {
           SymReg *r = mk_pasm_reg(interp, $4);
           set_lexical(interp, r, $2);
           $$ = 0;
           mem_sys_free($2);
           mem_sys_free($4);
         }
   | /* none */                { $$ = 0;}
   ;

pasm_args:
     vars
   ;

emit:                         /* EMIT and EOM tokens are used when compiling a .pasm file. */
     EMIT                     { IMCC_INFO(interp)->cur_unit = imc_open_unit(interp, IMC_PASM); }
     opt_pasmcode
     EOM
         {
           /* if (optimizer_level & OPT_PASM)
                         imc_compile_unit(interp, IMCC_INFO(interp)->cur_unit);
                         emit_flush(interp);
           */
           $$ = 0;
         }
   ;

opt_pasmcode:
     /* empty */
  | pasmcode
  ;

class_namespace:
    NAMESPACE maybe_ns '\n'
         {
           int re_open = 0;
           $$ = 0;
           if (IMCC_INFO(interp)->state->pasm_file && IMCC_INFO(interp)->cur_namespace) {
               imc_close_unit(interp, IMCC_INFO(interp)->cur_unit);
               re_open = 1;
           }
           IMCC_INFO(interp)->cur_namespace = $2;
           if (re_open)
               IMCC_INFO(interp)->cur_unit = imc_open_unit(interp, IMC_PASM);
         }
   ;

maybe_ns:
     '[' keylist ']'           { $$ = $2; }
   | '[' ']'                   { $$ = NULL; }
   |                           { $$ = NULL; }
   ;

sub:
     SUB
         {
           IMCC_INFO(interp)->cur_unit = imc_open_unit(interp, IMC_PCCSUB);
         }
     sub_label_op_c
         {
           iSUBROUTINE(interp, IMCC_INFO(interp)->cur_unit, $3);
         }
     sub_proto '\n'            { IMCC_INFO(interp)->cur_call->pcc_sub->pragma = $5; }
     sub_params
     sub_body  ESUB            { $$ = 0; IMCC_INFO(interp)->cur_call = NULL; }
   ;

sub_params:
     /* empty */               { $$ = 0; } %prec LOW_PREC
   | '\n'                               { $$ = 0; }
   | sub_params sub_param '\n'
         {
           if (IMCC_INFO(interp)->adv_named_id) {
                 add_pcc_named_param(interp, IMCC_INFO(interp)->cur_call,
                                     IMCC_INFO(interp)->adv_named_id, $2);
                 IMCC_INFO(interp)->adv_named_id = NULL;
           }
           else
               add_pcc_arg(IMCC_INFO(interp)->cur_call, $2);
         }
   ;

sub_param:
   PARAM { is_def=1; } sub_param_type_def { $$ = $3; is_def=0; }
   ;

sub_param_type_def:
     type IDENTIFIER paramtype_list
         {
           if ($3 & VT_UNIQUE_REG)
               $$ = mk_ident_ur(interp, $2, $1);
           else
               $$ = mk_ident(interp, $2, $1);
           $$->type |= $3;
           mem_sys_free($2);
          }

   /* don't free $2 here; adv_named_set uses the pointer directly */
   | type STRINGC ADV_ARROW IDENTIFIER paramtype_list
          {
            if ($5 & VT_UNIQUE_REG)
                $$ = mk_ident_ur(interp, $4, $1);
            else
                $$ = mk_ident(interp, $4, $1);
            $$->type |= $5;
            adv_named_set(interp, $2);
            mem_sys_free($4);
          }
   ;



multi:
     MULTI '(' multi_types ')' { $$ = 0; }
   ;

outer:
     OUTER '(' STRINGC ')'
         {
           $$ = 0;
           IMCC_INFO(interp)->cur_unit->outer = mk_sub_address_fromc(interp, $3);
           mem_sys_free($3);
         }
    | OUTER '(' IDENTIFIER ')'
         {
           $$ = 0;
           IMCC_INFO(interp)->cur_unit->outer = mk_const(interp, $3, 'S');
           mem_sys_free($3);
         }
   ;

vtable:
     VTABLE_METHOD
         {
           $$ = 0;
           IMCC_INFO(interp)->cur_unit->vtable_name = NULL;
           IMCC_INFO(interp)->cur_unit->is_vtable_method = 1;
         }
   | VTABLE_METHOD '(' STRINGC ')'
         {
           $$ = 0;
           IMCC_INFO(interp)->cur_unit->vtable_name = $3;
           IMCC_INFO(interp)->cur_unit->is_vtable_method = 1;
         }
   ;

instanceof:
     SUB_INSTANCE_OF '(' STRINGC ')'
         {
           $$ = 0;
           IMCC_INFO(interp)->cur_unit->instance_of = mk_const(interp, $3, 'S');
           mem_sys_free($3);
         }
   ;

lexid:
     SUB_LEXID '(' STRINGC ')'
         {
           $$ = 0;
           IMCC_INFO(interp)->cur_unit->lexid = $3;
         }
   ;

multi_types:
     /* empty */
         {
           add_pcc_multi(IMCC_INFO(interp)->cur_call, NULL);
         }
   | multi_types COMMA multi_type
         {
           $$ = 0;
           add_pcc_multi(IMCC_INFO(interp)->cur_call, $3);
         }
   | multi_type
         {
           $$ = 0;
           add_pcc_multi(IMCC_INFO(interp)->cur_call, $1);
         }
   ;

multi_type:
     INTV                      { $$ = mk_const(interp, "INTVAL",   'S'); }
   | FLOATV                    { $$ = mk_const(interp, "FLOATVAL", 'S'); }
   | PMCV                      { $$ = mk_const(interp, "PMC",      'S'); }
   | STRINGV                   { $$ = mk_const(interp, "STRING",   'S'); }
   | IDENTIFIER
         {
           SymReg *r;
           if (strcmp($1, "_") != 0)
               r = mk_const(interp, $1, 'S');
           else {
               r = mk_const(interp, "PMC", 'S');
           }
           mem_sys_free($1);
           $$ = r;
         }
   | STRINGC
         {
           SymReg *r;
           if (strcmp($1, "_") != 0)
               r = mk_const(interp, $1, 'S');
           else {
               r = mk_const(interp, "PMC", 'S');
           }
           mem_sys_free($1);
           $$ = r;
         }
   | '[' keylist ']'           { $$ = $2; }
   ;

sub_body:
     /* empty */
   |  statements
   ;

pcc_sub_call:
     PCC_BEGIN '\n'
         {
           char name[128];
           SymReg *r, *r1;
           Instruction *i;

           snprintf(name, sizeof (name), "%cpcc_sub_call_%d",
                   IMCC_INTERNAL_CHAR, IMCC_INFO(interp)->cnr++);
           $<sr>$ = r = mk_pcc_sub(interp, name, 0);
           /* this mid rule action has the semantic value of the
            * sub SymReg.
            * This is used below to append args & results
            */
           i = iLABEL(interp, IMCC_INFO(interp)->cur_unit, r);
           IMCC_INFO(interp)->cur_call = r;
           i->type = ITPCCSUB;
           /*
            * if we are inside a pcc_sub mark the sub as doing a
            * sub call; the sub is in r[0] of the first ins
            */
           r1 = IMCC_INFO(interp)->cur_unit->instructions->symregs[0];
           if (r1 && r1->pcc_sub)
               r1->pcc_sub->calls_a_sub |= 1;
         }
     pcc_args
     opt_invocant
     pcc_call
     opt_label
     pcc_results
     PCC_END                   { $$ = 0; IMCC_INFO(interp)->cur_call = NULL; }
   ;

opt_label:
     /* empty */               { $$ = NULL; IMCC_INFO(interp)->cur_call->pcc_sub->label = 0; }
   | label '\n'                { $$ = NULL; IMCC_INFO(interp)->cur_call->pcc_sub->label = 1; }
   ;

opt_invocant:
     /* empty */               { $$ = NULL; }
   | INVOCANT var '\n'         { $$ = NULL;  IMCC_INFO(interp)->cur_call->pcc_sub->object = $2; }
   ;

sub_proto:
     /* empty */               { $$ = 0; }
   | sub_proto_list
   ;

sub_proto_list:
     proto                     { $$ = $1; }
   | sub_proto_list proto      { $$ = $1 | $2; }
   ;

proto:
     LOAD                      { $$ = P_LOAD; }
   | INIT                      { $$ = P_INIT; }
   | MAIN                      { $$ = P_MAIN; }
   | IMMEDIATE                 { $$ = P_IMMEDIATE; }
   | POSTCOMP                  { $$ = P_POSTCOMP; }
   | ANON                      { $$ = P_ANON; }
   | METHOD                    { $$ = P_METHOD; }
   | NEED_LEX                  { $$ = P_NEED_LEX; }
   | multi
   | outer
   | vtable
   | instanceof
   | lexid
   ;

pcc_call:
     PCC_CALL var COMMA var '\n'
         {
           add_pcc_sub(IMCC_INFO(interp)->cur_call, $2);
           add_pcc_cc(IMCC_INFO(interp)->cur_call, $4);
         }
   | PCC_CALL var '\n'
         {
           add_pcc_sub(IMCC_INFO(interp)->cur_call, $2);
         }
   | NCI_CALL var '\n'
         {
           add_pcc_sub(IMCC_INFO(interp)->cur_call, $2);
           IMCC_INFO(interp)->cur_call->pcc_sub->flags |= isNCI;
         }
   | METH_CALL target '\n'
         {
           add_pcc_sub(IMCC_INFO(interp)->cur_call, $2);
         }
   | METH_CALL STRINGC '\n'
         {
           add_pcc_sub(IMCC_INFO(interp)->cur_call, mk_const(interp, $2, 'S'));
         }
   | METH_CALL target COMMA var '\n'
         {
           add_pcc_sub(IMCC_INFO(interp)->cur_call, $2);
           add_pcc_cc(IMCC_INFO(interp)->cur_call, $4);
         }
   | METH_CALL STRINGC COMMA var '\n'
         {
           add_pcc_sub(IMCC_INFO(interp)->cur_call, mk_const(interp, $2, 'S'));
           add_pcc_cc(IMCC_INFO(interp)->cur_call, $4);
         }
   ;


pcc_args:
     /* empty */               { $$ = 0; }
   | pcc_args pcc_arg '\n'     { add_pcc_arg(IMCC_INFO(interp)->cur_call, $2); }
   ;

pcc_arg:
     ARG arg                   { $$ = $2; }
   ;


pcc_results:
     /* empty */               { $$ = 0; }
   | pcc_results pcc_result '\n'
         {
           if ($2)
               add_pcc_result(IMCC_INFO(interp)->cur_call, $2);
         }
   ;

pcc_result:
     RESULT target paramtype_list { $$ = $2; $$->type |= $3; }
   | LOCAL { is_def=1; } type id_list_id
         {
           IdList *l = $4;
           SymReg *ignored;
           if (l->unique_reg)
               ignored = mk_ident_ur(interp, l->id, $3);
           else
               ignored = mk_ident(interp, l->id, $3);
           UNUSED(ignored);
           is_def=0;
           $$=0;
         }
   ;

paramtype_list:
     /* empty */               { $$ = 0; }
   | paramtype_list paramtype  { $$ = $1 | $2; }
   ;

paramtype:
     ADV_SLURPY                { $$ = VT_FLAT;   }
   | ADV_OPTIONAL              { $$ = VT_OPTIONAL; }
   | ADV_OPT_FLAG              { $$ = VT_OPT_FLAG; }
   | ADV_NAMED                 { $$ = VT_NAMED; }
   | ADV_NAMED '(' STRINGC ')' { adv_named_set(interp, $3); $$ = 0; }
   | UNIQUE_REG                { $$ = VT_UNIQUE_REG; }
   ;


pcc_ret:
     PCC_BEGIN_RETURN '\n'
         {
           begin_return_or_yield(interp, 0);
         }
     pcc_returns PCC_END_RETURN
         {
           $$ = 0;
           IMCC_INFO(interp)->asm_state = AsmDefault;
         }
   | pcc_return_many
         {
           IMCC_INFO(interp)->asm_state = AsmDefault;
           $$ = 0;
         }
   ;

pcc_yield:
     PCC_BEGIN_YIELD '\n'      { begin_return_or_yield(interp, 1); }
     pcc_returns
     PCC_END_YIELD             { $$ = 0; IMCC_INFO(interp)->asm_state = AsmDefault; }
   ;

pcc_returns:
     /* empty */               { $$ = 0; }
   | pcc_returns '\n'
         {
           if ($1)
               add_pcc_result(IMCC_INFO(interp)->sr_return, $1);
         }
   | pcc_returns pcc_return '\n'
         {
           if ($2)
               add_pcc_result(IMCC_INFO(interp)->sr_return, $2);
         }
   ;

pcc_return:
     RETURN var argtype_list   { $$ = $2; $$->type |= $3; }
   ;

pcc_return_many:
    return_or_yield  '('
        {
          if (IMCC_INFO(interp)->asm_state == AsmDefault)
              begin_return_or_yield(interp, $1);
        }
    var_returns  ')'
        {
          IMCC_INFO(interp)->asm_state = AsmDefault;
          $$ = 0;
        }
  ;

return_or_yield:
     RETURN                    { $$ = 0; }
   | YIELDT                    { $$ = 1; }
   ;

var_returns:
     /* empty */               { $$ = 0; }
   | arg
         {
           if (IMCC_INFO(interp)->adv_named_id) {
               add_pcc_named_return(interp, IMCC_INFO(interp)->sr_return,
                                    IMCC_INFO(interp)->adv_named_id, $1);
               IMCC_INFO(interp)->adv_named_id = NULL;
           }
           else
               add_pcc_result(IMCC_INFO(interp)->sr_return, $1);
         }
   | STRINGC ADV_ARROW var
         {
           add_pcc_named_return(interp, IMCC_INFO(interp)->sr_return, $1, $3);
         }
   | var_returns COMMA arg
         {
           if (IMCC_INFO(interp)->adv_named_id) {
               add_pcc_named_return(interp, IMCC_INFO(interp)->sr_return,
                                    IMCC_INFO(interp)->adv_named_id, $3);
               IMCC_INFO(interp)->adv_named_id = NULL;
             }
             else
                 add_pcc_result(IMCC_INFO(interp)->sr_return, $3);
         }
   | var_returns COMMA STRINGC ADV_ARROW var
         {
           add_pcc_named_return(interp, IMCC_INFO(interp)->sr_return, $3, $5);
         }
   ;


statements:
     statement
   | statements statement
   ;

/* This is ugly. Because 'instruction' can start with PARAM and in the
 * 'pcc_sub' rule, 'pcc_params' is followed by 'statement', we get a
 * shift/reduce conflict on PARAM between reducing to the dummy
 * { clear_state(); } rule and shifting the PARAM to be used as part
 * of the 'pcc_params' (which is what we want). However, yacc syntax
 * doesn't propagate precedence to the dummy rules, so we have to
 * split out the action just so that we can assign it a precedence. */

helper_clear_state:
                               { clear_state(interp); } %prec LOW_PREC
   ;

statement:
     helper_clear_state
     instruction               { $$ = $2; }
   | MACRO '\n'                { $$ = 0; }
   | FILECOMMENT               { $$ = 0; }
   | LINECOMMENT               { $$ = 0; }
   ;

labels:
     /* none */                { $$ = NULL; }
   |  _labels
   ;

_labels:
     _labels label
   | label
   ;

label:
     LABEL
         {
           $$ = iLABEL(interp, IMCC_INFO(interp)->cur_unit, mk_local_label(interp, $1));
         }
   ;



instruction:
     labels labeled_inst '\n'  { $$ = $2; }
   | error '\n'
         {
           if (yynerrs >= PARROT_MAX_RECOVER_ERRORS) {
               IMCC_warning(interp, "Too many errors. Correct some first.\n");
               YYABORT;
           }
           yyerrok;
         }
   ;

id_list :
     id_list_id
         {
           IdList* l = $1;
           l->next = NULL;
           $$ = l;
         }

   | id_list COMMA id_list_id
         {
           IdList* l = $3;
           l->next = $1;
           $$ = l;
         }
   ;

id_list_id :
     IDENTIFIER opt_unique_reg
         {
           IdList* const l = mem_allocate_n_zeroed_typed(1, IdList);
           l->id           = $1;
           l->unique_reg   = $2;
           $$ = l;
         }
   ;

opt_unique_reg:
     /* empty */               { $$ = 0; }
   | UNIQUE_REG                { $$ = 1; }
   ;


labeled_inst:
     assignment
   | conditional_statement
   | NAMESPACE IDENTIFIER      { push_namespace(interp, $2); mem_sys_free($2); }
   | ENDNAMESPACE IDENTIFIER   { pop_namespace(interp, $2); mem_sys_free($2); }
   | LOCAL { is_def=1; } type id_list
         {
           IdList *l = $4;
           while (l) {
               IdList *l1;
               if (l->unique_reg)
                   mk_ident_ur(interp, l->id, $3);
               else
                   mk_ident(interp, l->id, $3);
               l1 = l;
               l  = l->next;
               mem_sys_free(l1->id);
               mem_sys_free(l1);
           }
           is_def=0; $$=0;
         }
   | LEXICAL STRINGC COMMA target
         {
           set_lexical(interp, $4, $2); $$ = 0;
         }
   | CONST { is_def=1; } type IDENTIFIER '=' const
         {
           mk_const_ident(interp, $4, $3, $6, 0);
           is_def=0;
           mem_sys_free($4);
         }

   | pmc_const
   | GLOBAL_CONST { is_def=1; } type IDENTIFIER '=' const
         {
           mk_const_ident(interp, $4, $3, $6, 1);
           is_def=0;
           mem_sys_free($4);
         }
   | RETURN  sub_call
         {
           $$ = NULL;
           IMCC_INFO(interp)->cur_call->pcc_sub->flags |= isTAIL_CALL;
           IMCC_INFO(interp)->cur_call = NULL;
         }
   | GOTO label_op { $$ = MK_I(interp, IMCC_INFO(interp)->cur_unit, "branch", 1, $2); }
   | PARROT_OP vars
         {
           $$ = INS(interp,
                    IMCC_INFO(interp)->cur_unit,
                    $1,
                    0,
                    IMCC_INFO(interp)->regs,
                    IMCC_INFO(interp)->nargs,
                    IMCC_INFO(interp)->keyvec,
                    1);
           mem_sys_free($1);
         }
   | PNULL var                 { $$ = MK_I(interp, IMCC_INFO(interp)->cur_unit, "null", 1, $2); }
   | sub_call                  { $$ = 0; IMCC_INFO(interp)->cur_call = NULL; }
   | pcc_sub_call              { $$ = 0; }
   | pcc_ret
   | pcc_yield
   | /* none */                { $$ = 0;}
   ;

type:
     INTV                      { $$ = 'I'; }
   | FLOATV                    { $$ = 'N'; }
   | STRINGV                   { $$ = 'S'; }
   | PMCV                      { $$ = 'P'; }
   ;

classname:
     IDENTIFIER
         {
           /* there'd normally be a str_dup() here, but the lexer already
            * copied the string, so it's safe to use directly */
           if ((IMCC_INFO(interp)->cur_pmc_type = pmc_type(interp,
               string_from_cstring(interp, $1, 0))) <= 0) {
               IMCC_fataly(interp, E_SyntaxError, "Unknown PMC type '%s'\n", $1);
           }
         }
   ;

assignment:
     target '=' var
            { $$ = MK_I(interp, IMCC_INFO(interp)->cur_unit, "set", 2, $1, $3);  }
   | target '=' un_op var
            { $$ = MK_I(interp, IMCC_INFO(interp)->cur_unit, $3, 2, $1, $4);  }
   | target '=' var bin_op var
            { $$ = MK_I(interp, IMCC_INFO(interp)->cur_unit, $4, 3, $1, $3, $5);  }
   | target '=' var '[' keylist ']'
            { $$ = iINDEXFETCH(interp, IMCC_INFO(interp)->cur_unit, $1, $3, $5); }
   | target '[' keylist ']' '=' var
            { $$ = iINDEXSET(interp, IMCC_INFO(interp)->cur_unit, $1, $3, $6); }
   | target '=' NEW classname COMMA var
            { $$ = iNEW(interp, IMCC_INFO(interp)->cur_unit, $1, $4, $6, 1); }
   | target '=' NEW classname '[' keylist ']'
            { $$ = iNEW(interp, IMCC_INFO(interp)->cur_unit, $1, $4, $6, 1); }
   | target '=' NEW classname
            { $$ = iNEW(interp, IMCC_INFO(interp)->cur_unit, $1, $4, NULL, 1); }
   | target '=' NEW var
            { $$ = MK_I(interp, IMCC_INFO(interp)->cur_unit, "new", 2, $1, $4); }
   | target '=' NEW '[' keylist ']'
            { $$ = MK_I(interp, IMCC_INFO(interp)->cur_unit, "new", 2, $1, $5); }
   | target '=' NEW '[' keylist ']' COMMA var
            { $$ = MK_I(interp, IMCC_INFO(interp)->cur_unit, "new", 3, $1, $5, $8); }
   | target '=' NEW var COMMA var
            { $$ = MK_I(interp, IMCC_INFO(interp)->cur_unit, "new", 3, $1, $4, $6); }
   | target '=' NEW var '[' keylist ']'
            { $$ = MK_I(interp, IMCC_INFO(interp)->cur_unit, "new", 3, $1, $4, $6); }
   | target '=' ADDR IDENTIFIER
            { $$ = MK_I(interp, IMCC_INFO(interp)->cur_unit, "set_addr",
                        2, $1, mk_label_address(interp, $4)); mem_sys_free($4); }
   | target '=' GLOBALOP string
            { $$ = MK_I(interp, IMCC_INFO(interp)->cur_unit, "find_global", 2, $1, $4);}
   | GLOBALOP string '=' var
            { $$ = MK_I(interp, IMCC_INFO(interp)->cur_unit, "store_global", 2, $2, $4); }
       /* NEW is here because it is both PIR and PASM keywords so we
        * have to handle the token here (or badly hack the lexer). */
   | NEW target COMMA var
            { $$ = MK_I(interp, IMCC_INFO(interp)->cur_unit, "new", 2, $2, $4); }
   | NEW target COMMA var COMMA var
            { $$ = MK_I(interp, IMCC_INFO(interp)->cur_unit, "new", 3, $2, $4, $6); }
   | NEW target COMMA var '[' keylist ']'
            { $$ = MK_I(interp, IMCC_INFO(interp)->cur_unit, "new", 3, $2, $4, $6); }
     /* Subroutine call the short way */
   | target  '=' sub_call
         {
           add_pcc_result($3->symregs[0], $1);
           IMCC_INFO(interp)->cur_call = NULL;
           $$ = 0;
         }
   | '('
         {
           $<i>$ = IMCC_create_itcall_label(interp);
         }
     targetlist  ')' '=' the_sub '(' arglist ')'
         {
           IMCC_itcall_sub(interp, $6);
           IMCC_INFO(interp)->cur_call = NULL;
         }
   | get_results
   | op_assign
   | func_assign
   | target '=' PNULL
         {
           $$ = MK_I(interp, IMCC_INFO(interp)->cur_unit, "null", 1, $1);
         }
   ;

/* C++ hates implicit casts from string constants to char *, so be explicit */
un_op:
     '!'                       { $$ = (char *)"not"; }
   | '~'                       { $$ = (char *)"bnot"; }
   | '-'                       { $$ = (char *)"neg"; }
   ;

bin_op:
     '-'                       { $$ = (char *)"sub"; }
   | '+'                       { $$ = (char *)"add"; }
   | '*'                       { $$ = (char *)"mul"; }
   | '/'                       { $$ = (char *)"div"; }
   | '%'                       { $$ = (char *)"mod"; }
   | FDIV                      { $$ = (char *)"fdiv"; }
   | POW                       { $$ = (char *)"pow"; }
   | CONCAT                    { $$ = (char *)"concat"; }
   | RELOP_EQ                  { $$ = (char *)"iseq"; }
   | RELOP_NE                  { $$ = (char *)"isne"; }
   | RELOP_GT                  { $$ = (char *)"isgt"; }
   | RELOP_GTE                 { $$ = (char *)"isge"; }
   | RELOP_LT                  { $$ = (char *)"islt"; }
   | RELOP_LTE                 { $$ = (char *)"isle"; }
   | SHIFT_LEFT                { $$ = (char *)"shl"; }
   | SHIFT_RIGHT               { $$ = (char *)"shr"; }
   | SHIFT_RIGHT_U             { $$ = (char *)"lsr"; }
   | LOG_AND                   { $$ = (char *)"and"; }
   | LOG_OR                    { $$ = (char *)"or"; }
   | LOG_XOR                   { $$ = (char *)"xor"; }
   | '&'                       { $$ = (char *)"band"; }
   | '|'                       { $$ = (char *)"bor"; }
   | '~'                       { $$ = (char *)"bxor"; }
   ;


get_results:
     GET_RESULTS
         {
           $$ = IMCC_create_itcall_label(interp);
           $$->type &= ~ITCALL;
           $$->type |= ITRESULT;
         }
     '(' targetlist  ')'       {  $$ = 0; }
   ;



op_assign:
     target assign_op var
         {
           $$ = MK_I(interp, IMCC_INFO(interp)->cur_unit, $2, 2, $1, $3);
         }
   ;

assign_op:
     PLUS_ASSIGN               { $$ = (char *)"add"; }
   | MINUS_ASSIGN              { $$ = (char *)"sub"; }
   | MUL_ASSIGN                { $$ = (char *)"mul"; }
   | DIV_ASSIGN                { $$ = (char *)"div"; }
   | MOD_ASSIGN                { $$ = (char *)"mod"; }
   | FDIV_ASSIGN               { $$ = (char *)"fdiv"; }
   | CONCAT_ASSIGN             { $$ = (char *)"concat"; }
   | BAND_ASSIGN               { $$ = (char *)"band"; }
   | BOR_ASSIGN                { $$ = (char *)"bor"; }
   | BXOR_ASSIGN               { $$ = (char *)"bxor"; }
   | SHR_ASSIGN                { $$ = (char *)"shr"; }
   | SHL_ASSIGN                { $$ = (char *)"shl"; }
   | SHR_U_ASSIGN              { $$ = (char *)"lsr"; }
   ;


func_assign:
   target '=' PARROT_OP pasm_args
       {
        $$ = func_ins(interp, IMCC_INFO(interp)->cur_unit, $1, $3,
                      IMCC_INFO(interp) -> regs,
                      IMCC_INFO(interp) -> nargs,
                      IMCC_INFO(interp) -> keyvec, 1);
         mem_sys_free($3);
       }
   ;

the_sub:
     IDENTIFIER                { $$ = mk_sub_address(interp, $1);  mem_sys_free($1); }
   | STRINGC                   { $$ = mk_sub_address_fromc(interp, $1); mem_sys_free($1); }
   | USTRINGC                  { $$ = mk_sub_address_u(interp, $1); mem_sys_free($1); }
   | target
         {
           $$ = $1;
           if ($1->set != 'P')
               IMCC_fataly(interp, E_SyntaxError, "Sub isn't a PMC");
         }
   | target DOT sub_label_op   { IMCC_INFO(interp)->cur_obj = $1; $$ = $3; }
   | target DOT STRINGC
         {
           IMCC_INFO(interp)->cur_obj = $1; $$ = mk_const(interp, $3, 'S');
           mem_sys_free($3);
         }
   | target DOT target         { IMCC_INFO(interp)->cur_obj = $1; $$ = $3; }
   ;


sub_call:
     the_sub
         {
           $$ = IMCC_create_itcall_label(interp);
           IMCC_itcall_sub(interp, $1);
         }
     '(' arglist ')'           { $$ = $<i>2; }
   ;

arglist:
     /* empty */               { $$ = 0; }
   | arglist COMMA arg
         {
           $$ = 0;
           if (IMCC_INFO(interp)->adv_named_id) {
               add_pcc_named_arg(interp, IMCC_INFO(interp)->cur_call, IMCC_INFO(interp)->adv_named_id, $3);
               IMCC_INFO(interp)->adv_named_id = NULL;
           }
           else
               add_pcc_arg(IMCC_INFO(interp)->cur_call, $3);
         }
   | arg
         {
           $$ = 0;
           if (IMCC_INFO(interp)->adv_named_id) {
               add_pcc_named_arg(interp, IMCC_INFO(interp)->cur_call, IMCC_INFO(interp)->adv_named_id, $1);
               IMCC_INFO(interp)->adv_named_id = NULL;
           }
           else
               add_pcc_arg(IMCC_INFO(interp)->cur_call, $1);
         }
   | arglist COMMA STRINGC ADV_ARROW var
         {
           $$ = 0;
           add_pcc_named_arg(interp, IMCC_INFO(interp)->cur_call, $3, $5);
           mem_sys_free($3);
         }
   | STRINGC ADV_ARROW var
         {
           $$ = 0;
           add_pcc_named_arg(interp, IMCC_INFO(interp)->cur_call, $1, $3);
           mem_sys_free($1);
         }
   ;

arg:
     var argtype_list          { $$ = $1; $$->type |= $2; }
   ;

argtype_list:
     /* empty */               { $$ = 0; }
   | argtype_list argtype      { $$ = $1 | $2; }
   ;

argtype:
     ADV_FLAT                  { $$ = VT_FLAT; }
   | ADV_NAMED                 { $$ = VT_NAMED; }

   /* don't free $3 here; adv_named_set uses the pointer directly */
   | ADV_NAMED '(' STRINGC ')' { adv_named_set(interp, $3); $$ = 0; }
   ;

result:
     target paramtype_list     { $$ = $1; $$->type |= $2; }
   ;

targetlist:
     targetlist COMMA result
         {
           $$ = 0;
           if (IMCC_INFO(interp)->adv_named_id) {
               add_pcc_named_result(interp, IMCC_INFO(interp)->cur_call, IMCC_INFO(interp)->adv_named_id, $3);
               IMCC_INFO(interp)->adv_named_id = NULL;
           }
           else
               add_pcc_result(IMCC_INFO(interp)->cur_call, $3);
         }
   | targetlist COMMA STRINGC ADV_ARROW target
         {
           add_pcc_named_result(interp, IMCC_INFO(interp)->cur_call, $3, $5);
           mem_sys_free($3);
         }
   | result
         {
           $$ = 0;
           if (IMCC_INFO(interp)->adv_named_id) {
               add_pcc_named_result(interp, IMCC_INFO(interp)->cur_call, IMCC_INFO(interp)->adv_named_id, $1);
               IMCC_INFO(interp)->adv_named_id = NULL;
           }
           else
               add_pcc_result(IMCC_INFO(interp)->cur_call, $1);
         }
   | STRINGC ADV_ARROW target
         {
           add_pcc_named_result(interp, IMCC_INFO(interp)->cur_call, $1, $3);
           mem_sys_free($1);
         }
   | /* empty */                { $$ = 0; }
   ;

conditional_statement:
     if_statement               { $$ = $1; }
   | unless_statement           { $$ = $1; }
   ;

unless_statement:
     UNLESS var relop var GOTO label_op
         {
           $$ =MK_I(interp, IMCC_INFO(interp)->cur_unit, inv_op($3), 3, $2, $4, $6);
         }
   | UNLESS PNULL var GOTO label_op
         {
           $$ = MK_I(interp, IMCC_INFO(interp)->cur_unit, "unless_null", 2, $3, $5);
         }
   | UNLESS var comma_or_goto label_op
         {
           $$ = MK_I(interp, IMCC_INFO(interp)->cur_unit, "unless", 2, $2, $4);
         }
   ;

if_statement:
     IF var comma_or_goto label_op
         {
           $$ = MK_I(interp, IMCC_INFO(interp)->cur_unit, "if", 2, $2, $4);
         }
   | IF var relop var GOTO label_op
         {
           $$ =MK_I(interp, IMCC_INFO(interp)->cur_unit, $3, 3, $2, $4, $6);
         }
   | IF PNULL var GOTO label_op
         {
           $$ = MK_I(interp, IMCC_INFO(interp)->cur_unit, "if_null", 2, $3, $5);
         }
   ;

comma_or_goto:
     COMMA                     { $$ = 0; }
   | GOTO                      { $$ = 0; }
   ;

relop:
     RELOP_EQ                  { $$ = (char *)"eq"; }
   | RELOP_NE                  { $$ = (char *)"ne"; }
   | RELOP_GT                  { $$ = (char *)"gt"; }
   | RELOP_GTE                 { $$ = (char *)"ge"; }
   | RELOP_LT                  { $$ = (char *)"lt"; }
   | RELOP_LTE                 { $$ = (char *)"le"; }
   ;

target:
     VAR
   | reg
   ;

vars:
     /* empty */               { $$ = NULL; }
   | _vars                     { $$ = $1; }
   ;

_vars:
     _vars COMMA _var_or_i     { $$ = IMCC_INFO(interp)->regs[0]; }
   | _var_or_i
   ;

_var_or_i:
     var_or_i                  { IMCC_INFO(interp)->regs[IMCC_INFO(interp)->nargs++] = $1; }
   | target '[' keylist ']'
         {
           IMCC_INFO(interp) -> regs[IMCC_INFO(interp)->nargs++] = $1;
           IMCC_INFO(interp) -> keyvec |= KEY_BIT(IMCC_INFO(interp)->nargs);
           IMCC_INFO(interp) -> regs[IMCC_INFO(interp)->nargs++] = $3;
           $$ = $1;
         }
   | '[' keylist_force ']'
         {
           IMCC_INFO(interp) -> regs[IMCC_INFO(interp)->nargs++] = $2;
           $$ = $2;
         }
   ;
sub_label_op_c:
     sub_label_op
   | STRINGC                   { $$ = mk_sub_address_fromc(interp, $1); mem_sys_free($1); }
   | USTRINGC                  { $$ = mk_sub_address_u(interp, $1);  mem_sys_free($1); }
   ;

sub_label_op:
     IDENTIFIER                { $$ = mk_sub_address(interp, $1); mem_sys_free($1); }
   | PARROT_OP                 { $$ = mk_sub_address(interp, $1); mem_sys_free($1); }
   ;

label_op:
     IDENTIFIER                { $$ = mk_label_address(interp, $1); mem_sys_free($1); }
   | PARROT_OP                 { $$ = mk_label_address(interp, $1); mem_sys_free($1); }
   ;

var_or_i:
     label_op
   | var
   ;

var:
     target
   | const
   ;

keylist:
         {
           IMCC_INFO(interp)->nkeys    = 0;
           IMCC_INFO(interp)->in_slice = 0;
         }
     _keylist
         {
           $$ = link_keys(interp,
                          IMCC_INFO(interp)->nkeys,
                          IMCC_INFO(interp)->keys, 0);
         }
   ;

keylist_force:
         {
           IMCC_INFO(interp)->nkeys = 0;
           IMCC_INFO(interp)->in_slice = 0;
         }
     _keylist
         {
           $$ = link_keys(interp,
                          IMCC_INFO(interp)->nkeys,
                          IMCC_INFO(interp)->keys, 1);
         }
   ;

_keylist:
     key                       { IMCC_INFO(interp)->keys[IMCC_INFO(interp)->nkeys++] = $1; }
   | _keylist ';' key
         {
           IMCC_INFO(interp)->keys[IMCC_INFO(interp)->nkeys++] = $3;
           $$ = IMCC_INFO(interp)->keys[0];
         }
   | _keylist COMMA            { IMCC_INFO(interp)->in_slice = 1; }
     key
         {
           IMCC_INFO(interp)->keys[IMCC_INFO(interp)->nkeys++] = $4;
           $$ = IMCC_INFO(interp)->keys[0];
         }
   ;

key:
     var
         {
           if (IMCC_INFO(interp)->in_slice)
               $1->type |= VT_START_SLICE | VT_END_SLICE;
           $$ = $1;
         }
   | var DOTDOT var
         {
           $1->type |= VT_START_SLICE;
           $3->type |= VT_END_SLICE;
           IMCC_INFO(interp)->keys[IMCC_INFO(interp)->nkeys++] = $1;
           $$ = $3;
         }
   | DOTDOT var                { $2->type |= VT_START_ZERO | VT_END_SLICE; $$ = $2; }
   | var DOTDOT                { $1->type |= VT_START_SLICE | VT_END_INF; $$ = $1; }
   ;

reg:
     IREG                      { $$ = mk_symreg(interp, $1, 'I'); }
   | NREG                      { $$ = mk_symreg(interp, $1, 'N'); }
   | SREG                      { $$ = mk_symreg(interp, $1, 'S'); }
   | PREG                      { $$ = mk_symreg(interp, $1, 'P'); }
   | REG                       { $$ = mk_pasm_reg(interp, $1); mem_sys_free($1); }
   ;

const:
     INTC                      { $$ = mk_const(interp, $1, 'I'); mem_sys_free($1); }
   | FLOATC                    { $$ = mk_const(interp, $1, 'N'); mem_sys_free($1); }
   | STRINGC                   { $$ = mk_const(interp, $1, 'S'); mem_sys_free($1); }
   | USTRINGC                  { $$ = mk_const(interp, $1, 'U'); mem_sys_free($1); }
   ;

string:
     SREG                      { $$ = mk_symreg(interp, $1, 'S'); mem_sys_free($1); }
   | STRINGC                   { $$ = mk_const(interp, $1, 'S');  mem_sys_free($1); }
   ;


/* The End */
%%

/* I need this prototype somewhere... */
char *yyget_text(yyscan_t yyscanner);

/* I do not like this function, but, atm, it is the only way I can
 * make the code in yyerror work without segfault on some specific
 * cases.
 */
/* int yyholds_char(yyscan_t yyscanner); */

int yyerror(void *yyscanner, PARROT_INTERP, const char *s)
{
    /* If the error occurr in the end of the buffer (I mean, the last
     * token was already read), yyget_text will return a pointer
     * outside the bison buffer, and thus, not "accessible" by
     * us. This means it may segfault. */
    const char * const chr = yyget_text((yyscan_t)yyscanner);

    /* IMCC_fataly(interp, E_SyntaxError, s); */
    /* --- This was called before, not sure if I should call some
           similar function that does not die like this one. */

    /* Basically, if current token is a newline, it mean the error was
     * before the newline, and thus, line is the line *after* the
     * error. Instead of duplicating code for both cases (the 'newline' and
     * non-newline case, do the test twice; efficiency is not important when
     * we have an error anyway.
     */
    if (!at_eof(yyscanner)) {
        if (*chr == '\n') {
            IMCC_INFO(interp)->line--;
        }

        IMCC_warning(interp, "error:imcc:%s", s);
        /* don't print the current token if it is a newline */
        if (*chr != '\n') {
            IMCC_warning(interp, " ('%s')", chr);
        }
        IMCC_print_inc(interp);

        if (*chr == '\n') {
            IMCC_INFO(interp)->line++;
        }

    }
    else { /* scanner is at end of file; just to be sure, do not print "current" token. */
        IMCC_warning(interp, "error:imcc:%s", s);
        IMCC_print_inc(interp);
    }

    return 0;
}

/*
 * Local variables:
 *   c-file-style: "parrot"
 * End:
 * vim: expandtab shiftwidth=4:
 */
