/*
 * parser_util.c
 *
 * Intermediate Code Compiler for Parrot.
 *
 * Copyright (C) 2002 Melvin Smith <melvin.smith@mindspring.com>
 * Copyright (C) 2002-2009, Parrot Foundation.
 *
 * parser support functions
 *
 * $Id$
 *
 */

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#define _PARSER

#include "imc.h"
#include "parrot/dynext.h"
#include "parrot/embed.h"
#include "pmc/pmc_sub.h"
#include "pmc/pmc_callcontext.h"
#include "pbc.h"
#include "parser.h"
#include "optimizer.h"

/*

=head1 NAME

compilers/imcc/parser_util.c

=head1 DESCRIPTION

ParserUtil - Parser support functions.

=cut

*/

/* HEADERIZER HFILE: compilers/imcc/imc.h */

/* HEADERIZER BEGIN: static */
/* Don't modify between HEADERIZER BEGIN / HEADERIZER END.  Your changes will be lost. */

PARROT_WARN_UNUSED_RESULT
static int change_op(PARROT_INTERP,
    ARGMOD(IMC_Unit *unit),
    ARGMOD(SymReg **r),
    int num,
    int emit)
        __attribute__nonnull__(1)
        __attribute__nonnull__(2)
        __attribute__nonnull__(3)
        FUNC_MODIFIES(*unit)
        FUNC_MODIFIES(*r);

PARROT_CANNOT_RETURN_NULL
static void * imcc_compile_file(PARROT_INTERP,
    ARGIN(const char *fullname),
    ARGOUT(STRING **error_message))
        __attribute__nonnull__(1)
        __attribute__nonnull__(2)
        __attribute__nonnull__(3)
        FUNC_MODIFIES(*error_message);

static void imcc_destroy_macro_values(ARGMOD(void *value))
        __attribute__nonnull__(1)
        FUNC_MODIFIES(*value);

PARROT_WARN_UNUSED_RESULT
PARROT_CAN_RETURN_NULL
static const char * try_rev_cmp(ARGIN(const char *name), ARGMOD(SymReg **r))
        __attribute__nonnull__(1)
        __attribute__nonnull__(2)
        FUNC_MODIFIES(*r);

PARROT_MALLOC
PARROT_CANNOT_RETURN_NULL
PARROT_WARN_UNUSED_RESULT
static Instruction * var_arg_ins(PARROT_INTERP,
    ARGMOD(IMC_Unit *unit),
    ARGIN(const char *name),
    ARGMOD(SymReg **r),
    int n,
    int emit)
        __attribute__nonnull__(1)
        __attribute__nonnull__(2)
        __attribute__nonnull__(3)
        __attribute__nonnull__(4)
        FUNC_MODIFIES(*unit)
        FUNC_MODIFIES(*r);

#define ASSERT_ARGS_change_op __attribute__unused__ int _ASSERT_ARGS_CHECK = (\
       PARROT_ASSERT_ARG(interp) \
    , PARROT_ASSERT_ARG(unit) \
    , PARROT_ASSERT_ARG(r))
#define ASSERT_ARGS_imcc_compile_file __attribute__unused__ int _ASSERT_ARGS_CHECK = (\
       PARROT_ASSERT_ARG(interp) \
    , PARROT_ASSERT_ARG(fullname) \
    , PARROT_ASSERT_ARG(error_message))
#define ASSERT_ARGS_imcc_destroy_macro_values __attribute__unused__ int _ASSERT_ARGS_CHECK = (\
       PARROT_ASSERT_ARG(value))
#define ASSERT_ARGS_try_rev_cmp __attribute__unused__ int _ASSERT_ARGS_CHECK = (\
       PARROT_ASSERT_ARG(name) \
    , PARROT_ASSERT_ARG(r))
#define ASSERT_ARGS_var_arg_ins __attribute__unused__ int _ASSERT_ARGS_CHECK = (\
       PARROT_ASSERT_ARG(interp) \
    , PARROT_ASSERT_ARG(unit) \
    , PARROT_ASSERT_ARG(name) \
    , PARROT_ASSERT_ARG(r))
/* Don't modify between HEADERIZER BEGIN / HEADERIZER END.  Your changes will be lost. */
/* HEADERIZER END: static */

/*
 * used in -D20 to print files with the output of every PIR compilation
 * this can't be attached to the interpreter or packfile because it has to be
 * absolutely global to prevent the files from being overwritten.
 *
 */
static Parrot_mutex eval_nr_lock;
static INTVAL       eval_nr  = 0;

/*

=head2 Functions

=over 4

=item C<Instruction * iNEW(PARROT_INTERP, IMC_Unit *unit, SymReg *r0, char
*type, SymReg *init, int emit)>

 * P = new type, [init]
 * PASM like:
 *   new P, 'SomeThing'
 * is done in the lexer, this is a mess
 * best would be to have a flag in core.ops, where a PMC type is expected

=cut

 */

PARROT_WARN_UNUSED_RESULT
PARROT_CAN_RETURN_NULL
Instruction *
iNEW(PARROT_INTERP, ARGMOD(IMC_Unit *unit), ARGMOD(SymReg *r0),
        ARGMOD(char *type), ARGIN_NULLOK(SymReg *init), int emit)
{
    ASSERT_ARGS(iNEW)
    char fmt[256];
    SymReg *regs[3];
    SymReg *pmc;
    int nargs;
    const int pmc_num = pmc_type(interp,
            Parrot_str_new(interp, *type == '.' ? type + 1 : type, 0));

    snprintf(fmt, sizeof (fmt), "%d", pmc_num);
    pmc = mk_const(interp, fmt, 'I');

    if (pmc_num <= 0)
        IMCC_fataly(interp, EXCEPTION_SYNTAX_ERROR, "Unknown PMC type '%s'\n", type);

    snprintf(fmt, sizeof (fmt), "%%s, %d\t # .%s", pmc_num, type);

    r0->usage |= U_NEW;
    if (STREQ(type, "Hash"))
        r0->usage |= U_KEYED;

    regs[0] = r0;
    regs[1] = pmc;

    if (init) {
        regs[2] = init;
        nargs   = 3;
    }
    else
        nargs = 2;

    return INS(interp, unit, "new", fmt, regs, nargs, 0, emit);
}

/*

=item C<void op_fullname(char *dest, const char *name, SymReg * const *args, int
narg, int keyvec)>

Lookup the full opcode given the short name

   set I0, 5  -> set_i_ic
   set I0, I1 -> set_i_i

Obviously the registers must be examined before returning the correct
opcode.

NOTE: All this nasty IMC_TRACE is for tracking down equally nasty bugs, so
if you don't like the looks of it, stay out, but please don't remove it. :)
-Mel

=cut

 */
void
op_fullname(ARGOUT(char *dest), ARGIN(const char *name),
    ARGIN(SymReg * const *args), int narg, int keyvec)
{
    ASSERT_ARGS(op_fullname)
    int i;
    const size_t namelen = strlen(name);

#if IMC_TRACE_HIGH
    const char * const full = dest;
    Parrot_io_eprintf(NULL, "op %s", name);
#endif

    memcpy(dest, name, namelen+1);
    dest += namelen;

    for (i = 0; i < narg && args[i]; i++) {
        *dest++ = '_';
        if (args[i]->type == VTADDRESS) {
#if IMC_TRACE_HIGH
            Parrot_io_eprintf(NULL, " (address)%s", args[i]->name);
#endif
            *dest++ = 'i';
            *dest++ = 'c';
            continue;
        }
        /* if one ever wants num keys, they go with 'S' */
        if (keyvec & KEY_BIT(i)) {
#if IMC_TRACE_HIGH
            Parrot_io_eprintf(NULL, " (key)%s", args[i]->name);
#endif
            *dest++ = 'k';
            if (args[i]->set=='S' || args[i]->set=='N' || args[i]->set=='K') {
                *dest++ = 'c';
                continue;
            }
            else if (args[i]->set == 'P')
                continue;
        }

        if (args[i]->set == 'K')
            *dest++ = 'p';
        else
            *dest++ = (char)tolower((unsigned char)args[i]->set);

        if (args[i]->type & (VTCONST|VT_CONSTP)) {
#if IMC_TRACE_HIGH
            Parrot_io_eprintf(NULL, " (%cc)%s",
                    tolower((unsigned char)args[i]->set), args[i]->name);
#endif
            *dest++ = 'c';
        }
#if IMC_TRACE_HIGH
        else
            Parrot_io_eprintf(NULL, " (%c)%s",
                    tolower((unsigned char)args[i]->set), args[i]->name);
#endif
    }
    *dest = '\0';
#if IMC_TRACE_HIGH
    Parrot_io_eprintf(NULL, " -> %s\n", full);
#endif
}

/*

=item C<int check_op(PARROT_INTERP, char *fullname, const char *name, SymReg *
const * r, int narg, int keyvec)>

Return opcode value for op name

=cut

*/

PARROT_WARN_UNUSED_RESULT
int
check_op(PARROT_INTERP, ARGOUT(char *fullname), ARGIN(const char *name),
        ARGIN(SymReg * const * r), int narg, int keyvec)
{
    ASSERT_ARGS(check_op)
    op_fullname(fullname, name, r, narg, keyvec);

    return interp->op_lib->op_code(fullname, 1);
}

/*

=item C<int is_op(PARROT_INTERP, const char *name)>

Is instruction a parrot opcode?

=cut

*/

PARROT_WARN_UNUSED_RESULT
int
is_op(PARROT_INTERP, ARGIN(const char *name))
{
    ASSERT_ARGS(is_op)
    return interp->op_lib->op_code(name, 0) >= 0
        || interp->op_lib->op_code(name, 1) >= 0;
}

/*

=item C<static Instruction * var_arg_ins(PARROT_INTERP, IMC_Unit *unit, const
char *name, SymReg **r, int n, int emit)>

=cut

*/

PARROT_MALLOC
PARROT_CANNOT_RETURN_NULL
PARROT_WARN_UNUSED_RESULT
static Instruction *
var_arg_ins(PARROT_INTERP, ARGMOD(IMC_Unit *unit), ARGIN(const char *name),
        ARGMOD(SymReg **r), int n, int emit)
{
    ASSERT_ARGS(var_arg_ins)
    int op;
    Instruction *ins;
    char fullname[64];

    /* in constant */
    int dirs       = 1;

    /* XXX: Maybe the check for n == 0 is the only one required
     * and the other must be assertions? */
    if (n == 0 || r[0] == NULL || r[0]->name == NULL)
        IMCC_fataly(interp, EXCEPTION_SYNTAX_ERROR,
                    "The opcode '%s' needs arguments", name);

    r[0]           = mk_const(interp, r[0]->name, 'P');
    r[0]->pmc_type = enum_class_FixedIntegerArray;

    op_fullname(fullname, name, r, 1, 0);
    op = interp->op_lib->op_code(fullname, 1);

    PARROT_ASSERT(op >= 0);

    ins         = _mk_instruction(name, "", n, r, dirs);
    ins->opnum  = op;
    ins->opsize = n + 1;

    if (emit)
        emitb(interp, unit, ins);

    return ins;
}

/*

=item C<Instruction * INS(PARROT_INTERP, IMC_Unit *unit, const char *name, const
char *fmt, SymReg **r, int n, int keyvec, int emit)>

Makes an instruction.

name   ... op name
fmt    ... optional format
regs   ... SymReg **
n      ... number of params
keyvec ... see KEY_BIT()
emit   ... if true, append to instructions

see imc.c for usage

=cut

*/

PARROT_IGNORABLE_RESULT
PARROT_CAN_RETURN_NULL
Instruction *
INS(PARROT_INTERP, ARGMOD(IMC_Unit *unit), ARGIN(const char *name),
    ARGIN_NULLOK(const char *fmt), ARGIN(SymReg **r), int n, int keyvec,
    int emit)
{
    ASSERT_ARGS(INS)
    int i, op, len;
    int dirs = 0;
    Instruction *ins;
    op_info_t   *op_info;
    char fullname[64] = "", format[128] = "";

    if (STREQ(name, ".annotate")) {
        ins = _mk_instruction(name, "", n, r, 0);
        if (emit)
            return emitb(interp, unit, ins);
        else
            return ins;
    }

    if ((STREQ(name, "set_args"))
    ||  (STREQ(name, "get_results"))
    ||  (STREQ(name, "get_params"))
    ||  (STREQ(name, "set_returns")))
        return var_arg_ins(interp, unit, name, r, n, emit);

    op_fullname(fullname, name, r, n, keyvec);
    op = interp->op_lib->op_code(fullname, 1);

    /* maybe we have a fullname */
    if (op < 0)
        op = interp->op_lib->op_code(name, 1);

    /* still wrong, try reverse compare */
    if (op < 0) {
        const char * const n_name = try_rev_cmp(name, r);
        if (n_name) {
            name = n_name;
            op_fullname(fullname, name, r, n, keyvec);
            op   = interp->op_lib->op_code(fullname, 1);
        }
    }

    /* still wrong, try to find an existing op */
    if (op < 0)
        op = try_find_op(interp, unit, name, r, n, keyvec, emit);

    if (op < 0) {
        int ok = 0;

        /* check mixed constants */
        ins = IMCC_subst_constants_umix(interp, unit, name, r, n + 1);
        if (ins)
            goto found_ins;

        /* and finally multiple constants */
        ins = IMCC_subst_constants(interp, unit, name, r, n + 1, &ok);

        if (ok) {
            if (ins)
                goto found_ins;
            else
                return NULL;
        }
    }
    else
        strcpy(fullname, name);

    if (op < 0)
        IMCC_fataly(interp, EXCEPTION_SYNTAX_ERROR,
                    "The opcode '%s' (%s<%d>) was not found. "
                    "Check the type and number of the arguments",
                    fullname, name, n);

    op_info = &interp->op_info_table[op];
    *format = '\0';

    /* info->op_count is args + 1
     * build instruction format
     * set LV_in / out flags */
    if (n != op_info->op_count - 1)
        IMCC_fataly(interp, EXCEPTION_SYNTAX_ERROR,
                "arg count mismatch: op #%d '%s' needs %d given %d",
                op, fullname, op_info->op_count-1, n);

    /* XXX Speed up some by keep track of the end of format ourselves */
    for (i = 0; i < n; i++) {
        switch (op_info->dirs[i]) {
          case PARROT_ARGDIR_INOUT:
            dirs |= 1 << (16 + i);
            /* go on */
          case PARROT_ARGDIR_IN:
            dirs |= 1 << i ;
            break;

          case PARROT_ARGDIR_OUT:
            dirs |= 1 << (16 + i);
            break;

          default:
            PARROT_ASSERT(0);
        };

        if (keyvec & KEY_BIT(i)) {
            /* XXX Assert that len > 2 */
            len          = strlen(format) - 2;
            PARROT_ASSERT(len >= 0);
            format[len]  = '\0';
            strcat(format, "[%s], ");
        }
        else if (r[i]->set == 'K')
            strcat(format, "[%s], ");
        else
            strcat(format, "%s, ");
    }

    len = strlen(format);
    if (len >= 2)
        len -= 2;

    format[len] = '\0';

    if (fmt && *fmt) {
        strncpy(format, fmt, sizeof (format) - 1);
        format[sizeof (format) - 1] = '\0';
    }

    IMCC_debug(interp, DEBUG_PARSER, "%s %s\t%s\n", name, format, fullname);

    /* make the instruction */
    ins         = _mk_instruction(name, format, n, r, dirs);
    ins->keys  |= keyvec;

    /* fill in oplib's info */
    ins->opnum  = op;
    ins->opsize = n + 1;

    /* mark end as absolute branch */
    if (STREQ(name, "end") || STREQ(name, "ret")) {
        ins->type |= ITBRANCH | IF_goto;
    }
    else if (STREQ(name, "warningson")) {
        /* emit a debug seg, if this op is seen */
        PARROT_WARNINGS_on(interp, PARROT_WARNINGS_ALL_FLAG);
    }
    else if (STREQ(name, "yield")) {
        if (!IMCC_INFO(interp)->cur_unit->instructions->symregs[0])
            IMCC_fataly(interp, EXCEPTION_SYNTAX_ERROR,
                "Cannot yield from non-continuation\n");

        IMCC_INFO(interp)->cur_unit->instructions->symregs[0]->pcc_sub->calls_a_sub
            |= 1 | ITPCCYIELD;
    }
    else if ((strncmp(name, "invoke", 6) == 0) ||
             (strncmp(name, "callmethod", 10) == 0)) {
        if (IMCC_INFO(interp)->cur_unit->type & IMC_PCCSUB)
            IMCC_INFO(interp)->cur_unit->instructions->symregs[0]->pcc_sub->calls_a_sub |= 1;
    }

    /* set up branch flags
     * mark registers that are labels */
    for (i = 0; i < op_info->op_count - 1; i++) {
        if (op_info->labels[i])
            ins->type |= ITBRANCH | (1 << i);
        else {
            if (r[i]->type == VTADDRESS)
                IMCC_fataly(interp, EXCEPTION_SYNTAX_ERROR,
                        "undefined identifier '%s'\n", r[i]->name);
        }
    }

    if (op_info->jump && op_info->jump != PARROT_JUMP_ENEXT) {
        ins->type |= ITBRANCH;
        /* TODO use opnum constants */
        if (STREQ(name, "branch")
        ||  STREQ(name, "tailcall")
        ||  STREQ(name, "returncc"))
            ins->type |= IF_goto;
        else if (STREQ(fullname, "jump_i")
             ||  STREQ(fullname, "branch_i"))
            IMCC_INFO(interp)->dont_optimize = 1;
    }
    else if (STREQ(name, "set") && n == 2) {
        /* set Px, Py: both PMCs have the same address */
        if (r[0]->set == r[1]->set && REG_NEEDS_ALLOC(r[1]))
            ins->type |= ITALIAS;
    }
    else if (STREQ(name, "compile"))
        ++IMCC_INFO(interp)->has_compile;

  found_ins:
    if (emit)
        emitb(interp, unit, ins);

    return ins;
}

extern void* yy_scan_string(const char *);

/*

=item C<int do_yylex_init(PARROT_INTERP, yyscan_t* yyscanner)>

=cut

*/

PARROT_EXPORT
int
do_yylex_init(PARROT_INTERP, ARGOUT(yyscan_t* yyscanner))
{
    ASSERT_ARGS(do_yylex_init)
    const int retval = yylex_init(yyscanner);

    /* This way we can get the interpreter via yyscanner */
    if (!retval)
        yyset_extra(interp, *yyscanner);

    return retval;
}

/*

=item C<PMC * imcc_compile(PARROT_INTERP, const char *s, int pasm_file, STRING
**error_message)>

Compile a pasm or imcc string

FIXME as we have separate constants, the old constants in ghash must be deleted.

=cut

*/

PARROT_WARN_UNUSED_RESULT
PARROT_CANNOT_RETURN_NULL
PMC *
imcc_compile(PARROT_INTERP, ARGIN(const char *s), int pasm_file,
        ARGOUT(STRING **error_message))
{
    ASSERT_ARGS(imcc_compile)
    /* imcc always compiles to interp->code
     * save old cs, make new
     */
    STRING                *name;
    PackFile_ByteCode     *old_cs, *new_cs;
    PMC                   *sub      = NULL;
    struct _imc_info_t    *imc_info = NULL;
    struct parser_state_t *next;
    void                  *yyscanner;
    PMC                   *ignored;
    UINTVAL regs_used[4] = {3, 3, 3, 3};
    INTVAL eval_number;

    do_yylex_init(interp, &yyscanner);

    /* we create not yet anchored PMCs - e.g. Subs: turn off GC */
    Parrot_block_GC_mark(interp);

    if (IMCC_INFO(interp)->last_unit) {
        /* a reentrant compile */
        imc_info          = mem_allocate_zeroed_typed(imc_info_t);
        imc_info->ghash   = IMCC_INFO(interp)->ghash;
        imc_info->prev    = IMCC_INFO(interp);
        IMCC_INFO(interp) = imc_info;
    }

    ignored = Parrot_push_context(interp, regs_used);
    UNUSED(ignored);

    if (eval_nr == 0)
        MUTEX_INIT(eval_nr_lock);

    LOCK(eval_nr_lock);
    eval_number = ++eval_nr;
    UNLOCK(eval_nr_lock);

    name = Parrot_sprintf_c(interp, "EVAL_" INTVAL_FMT, eval_number);
    new_cs = PF_create_default_segs(interp, name, 0);
    old_cs = Parrot_switch_to_cs(interp, new_cs, 0);

    IMCC_INFO(interp)->cur_namespace = NULL;

    /* spit out the sourcefile */
    if (Interp_debug_TEST(interp, PARROT_EVAL_DEBUG_FLAG)) {
        char *buf = Parrot_str_to_cstring(interp, name);
        FILE * const fp = fopen(buf, "w");
        Parrot_str_free_cstring(buf);
        if (fp) {
            fputs(s, fp);
            fclose(fp);
        }
    }

    IMCC_push_parser_state(interp);
    next = IMCC_INFO(interp)->state->next;

    if (imc_info)
        IMCC_INFO(interp)->state->next = NULL;

    IMCC_INFO(interp)->state->pasm_file = pasm_file;
    IMCC_INFO(interp)->state->file      = Parrot_str_to_cstring(interp, name);
    IMCC_INFO(interp)->expect_pasm      = 0;

    compile_string(interp, s, yyscanner);

    Parrot_pop_context(interp);

    /*
     * compile_string NULLifies frames->next, so that yywrap
     * doesn't try to continue compiling the previous buffer
     * This OTOH prevents pop_parser-state ->
     *
     * set next here and pop
     */
    IMCC_INFO(interp)->state->next = next;
    IMCC_pop_parser_state(interp, yyscanner);

    if (!IMCC_INFO(interp)->error_code) {
        Parrot_Sub_attributes *sub_data;

        /*
         * create sub PMC
         *
         * TODO if a sub was denoted :main return that instead
         */
        sub                  = pmc_new(interp, enum_class_Eval);
        PMC_get_sub(interp, sub, sub_data);
        sub_data->seg        = new_cs;
        sub_data->start_offs = 0;
        sub_data->end_offs   = new_cs->base.size;
        sub_data->name       = Parrot_str_copy(interp, name);

        *error_message = NULL;
    }
    else {
        *error_message = IMCC_INFO(interp)->error_message;
    }

    if (imc_info) {
        SymReg *ns                  = IMCC_INFO(interp)->cur_namespace;
        IMCC_INFO(interp)           = imc_info->prev;
        mem_sys_free(imc_info);
        imc_info                    = IMCC_INFO(interp);
        IMCC_INFO(interp)->cur_unit = imc_info->last_unit;

        if (ns && ns != imc_info->cur_namespace)
            free_sym(ns);

        IMCC_INFO(interp)->cur_namespace = imc_info->cur_namespace;
    }
    else
        imc_cleanup(interp, yyscanner);

    Parrot_unblock_GC_mark(interp);

    yylex_destroy(yyscanner);

    /* Now run any :load/:init subs. */
    if (!*error_message)
        PackFile_fixup_subs(interp, PBC_MAIN, sub);

    /* restore old byte_code, */
    if (old_cs)
        (void)Parrot_switch_to_cs(interp, old_cs, 0);

    return sub;
}

/*

=item C<PMC * imcc_compile_pasm(PARROT_INTERP, const char *s)>

Note: This function is provided for backward compatibility. This
function can go away in future.

=cut

*/

PARROT_WARN_UNUSED_RESULT
PARROT_CANNOT_RETURN_NULL
PMC *
imcc_compile_pasm(PARROT_INTERP, ARGIN(const char *s))
{
    ASSERT_ARGS(imcc_compile_pasm)
    STRING *error_message;
    return imcc_compile(interp, s, 1, &error_message);
}

/*

=item C<PMC * imcc_compile_pir(PARROT_INTERP, const char *s)>

Note: This function is provided for backward compatibility. This
function can go away in future.

=cut

*/

PARROT_WARN_UNUSED_RESULT
PARROT_CANNOT_RETURN_NULL
PMC *
imcc_compile_pir(PARROT_INTERP, ARGIN(const char *s))
{
    ASSERT_ARGS(imcc_compile_pir)
    STRING *error_message;
    return imcc_compile(interp, s, 0, &error_message);
}

/*

=item C<PMC * IMCC_compile_pir_s(PARROT_INTERP, const char *s, STRING
**error_message)>

=cut

*/

PARROT_WARN_UNUSED_RESULT
PARROT_CANNOT_RETURN_NULL
PMC *
IMCC_compile_pir_s(PARROT_INTERP, ARGIN(const char *s),
        ARGOUT(STRING **error_message))
{
    ASSERT_ARGS(IMCC_compile_pir_s)
    return imcc_compile(interp, s, 0, error_message);
}

/*

=item C<PMC * IMCC_compile_pasm_s(PARROT_INTERP, const char *s, STRING
**error_message)>

=cut

*/

PARROT_WARN_UNUSED_RESULT
PARROT_CANNOT_RETURN_NULL
PMC *
IMCC_compile_pasm_s(PARROT_INTERP, ARGIN(const char *s),
        ARGOUT(STRING **error_message))
{
    ASSERT_ARGS(IMCC_compile_pasm_s)
    return imcc_compile(interp, s, 1, error_message);
}

/*

=item C<PMC * imcc_compile_pasm_ex(PARROT_INTERP, const char *s)>

=cut

*/

PARROT_WARN_UNUSED_RESULT
PARROT_CANNOT_RETURN_NULL
PMC *
imcc_compile_pasm_ex(PARROT_INTERP, ARGIN(const char *s))
{
    ASSERT_ARGS(imcc_compile_pasm_ex)
    STRING *error_message;

    PMC * const sub = imcc_compile(interp, s, 1, &error_message);

    if (sub)
        return sub;

    Parrot_ex_throw_from_c_args(interp, NULL, EXCEPTION_SYNTAX_ERROR, "%Ss",
        error_message);
}

/*

=item C<PMC * imcc_compile_pir_ex(PARROT_INTERP, const char *s)>

=cut

*/

PARROT_WARN_UNUSED_RESULT
PARROT_CANNOT_RETURN_NULL
PMC *
imcc_compile_pir_ex(PARROT_INTERP, ARGIN(const char *s))
{
    ASSERT_ARGS(imcc_compile_pir_ex)
    STRING *error_message;
    PMC *sub;

    /* We need to clear the current_results from the current context. This is
     * in order to prevent any RetContinuations that get promoted to full
     * Continuations (this happens when something is the target of a :outer)
     * trying to return values using them when invoked. (See TT #500 for the
     * report of the bug this fixes). */
    opcode_t *save_results = Parrot_pcc_get_results(interp, CURRENT_CONTEXT(interp));
    Parrot_pcc_set_results(interp, CURRENT_CONTEXT(interp), NULL);
    sub = imcc_compile(interp, s, 0, &error_message);
    Parrot_pcc_set_results(interp, CURRENT_CONTEXT(interp), save_results);

    if (sub)
        return sub;

    Parrot_ex_throw_from_c_args(interp, NULL, EXCEPTION_SYNTAX_ERROR, "%Ss",
        error_message);
}

/*

=item C<static void * imcc_compile_file(PARROT_INTERP, const char *fullname,
STRING **error_message)>

Compile a file by filename (can be either PASM or IMCC code)

=cut

*/

PARROT_CANNOT_RETURN_NULL
static void *
imcc_compile_file(PARROT_INTERP, ARGIN(const char *fullname),
        ARGOUT(STRING **error_message))
{
    ASSERT_ARGS(imcc_compile_file)
    PackFile_ByteCode  * const cs_save  = interp->code;
    PackFile_ByteCode         *cs       = NULL;
    struct _imc_info_t        *imc_info = NULL;
    const char                *ext;
    FILE                      *fp;
    STRING                    *fs;
    PMC                       *ignored;

    /* need at least 3 regs for compilation of constant math e.g.
     * add_i_ic_ic - see also IMCC_subst_constants() */
    UINTVAL regs_used[4] = {3, 3, 3, 3};

    if (IMCC_INFO(interp)->last_unit) {
        /* a reentrant compile */
        imc_info          = mem_allocate_zeroed_typed(imc_info_t);
        imc_info->prev    = IMCC_INFO(interp);
        imc_info->ghash   = IMCC_INFO(interp)->ghash;
        IMCC_INFO(interp) = imc_info;
    }

    fs = string_make(interp, fullname, strlen(fullname), NULL, 0);

    if (Parrot_stat_info_intval(interp, fs, STAT_ISDIR))
        Parrot_ex_throw_from_c_args(interp, NULL, EXCEPTION_EXTERNAL_ERROR,
            "imcc_compile_file: '%s' is a directory\n", fullname);

    fp = fopen(fullname, "r");
    if (!fp)
        IMCC_fatal(interp, EXCEPTION_EXTERNAL_ERROR,
                "imcc_compile_file: couldn't open '%s'\n", fullname);

#if IMC_TRACE
    fprintf(stderr, "parser_util.c: imcc_compile_file '%s'\n", fullname);
#endif

    IMCC_INFO(interp)->cur_namespace = NULL;
    interp->code                     = NULL;

    IMCC_push_parser_state(interp);
    {
        /* Store a copy, in order to know how to free it later */
        char *copyname = mem_sys_strdup(fullname);
        IMCC_INFO(interp)->state->file = copyname;
        ext                            = strrchr(copyname, '.');
    }
    IMCC_INFO(interp)->line        = 1;

    /*
     * the Parrot_str_compare() called from pmc_type() triggers GC
     * which can destroy packfiles under construction
     */
    Parrot_block_GC_mark(interp);
    ignored = Parrot_push_context(interp, regs_used);
    UNUSED(ignored);

    if (ext && STREQ(ext, ".pasm")) {
        void *yyscanner;
        do_yylex_init(interp, &yyscanner);

        IMCC_INFO(interp)->state->pasm_file = 1;
        /* see imcc.l */
        compile_file(interp, fp, yyscanner);

        yylex_destroy(yyscanner);
    }
    else {
        void *yyscanner;
        do_yylex_init(interp, &yyscanner);

        IMCC_INFO(interp)->state->pasm_file = 0;
        compile_file(interp, fp, yyscanner);

        yylex_destroy(yyscanner);
    }

    Parrot_unblock_GC_mark(interp);
    Parrot_pop_context(interp);

    imc_cleanup(interp, NULL);
    fclose(fp);

    if (!IMCC_INFO(interp)->error_code)
        cs = interp->code;
    else
        *error_message = IMCC_INFO(interp)->error_message;

    if (cs_save)
        (void)Parrot_switch_to_cs(interp, cs_save, 0);

    if (imc_info) {
        IMCC_INFO(interp) = imc_info->prev;
        if (imc_info->globals)
            mem_sys_free(imc_info->globals);

        mem_sys_free(imc_info);
    }

    return cs;
}

/*

=item C<void * IMCC_compile_file(PARROT_INTERP, const char *s)>

Note: This function is provided for backward compatibility. This
function can go away in future.

=cut

*/

PARROT_CANNOT_RETURN_NULL
void *
IMCC_compile_file(PARROT_INTERP, ARGIN(const char *s))
{
    ASSERT_ARGS(IMCC_compile_file)
    STRING *error_message;
    return imcc_compile_file(interp, s, &error_message);
}

/*

=item C<void * IMCC_compile_file_s(PARROT_INTERP, const char *s, STRING
**error_message)>

=cut

*/

PARROT_CANNOT_RETURN_NULL
void *
IMCC_compile_file_s(PARROT_INTERP, ARGIN(const char *s),
        ARGOUT(STRING **error_message))
{
    ASSERT_ARGS(IMCC_compile_file_s)
    return imcc_compile_file(interp, s, error_message);
}

/*

=item C<void register_compilers(PARROT_INTERP)>

Register additional compilers with the interpreter

=cut

*/

void
register_compilers(PARROT_INTERP)
{
    ASSERT_ARGS(register_compilers)
    Parrot_compreg(interp, Parrot_str_new_constant(interp, "PASM"), imcc_compile_pasm_ex);
    Parrot_compreg(interp, Parrot_str_new_constant(interp, "PIR"),  imcc_compile_pir_ex);

    /* It looks like this isn't used anywhere yet */
    /* TODO: return a Eval PMC, instead of a packfile */
    /* Parrot_compreg(interp,
                      Parrot_str_new_constant(interp, "FILE"),
                      imcc_compile_file ); */
}

/*

=item C<static int change_op(PARROT_INTERP, IMC_Unit *unit, SymReg **r, int num,
int emit)>

=cut

*/

PARROT_WARN_UNUSED_RESULT
static int
change_op(PARROT_INTERP, ARGMOD(IMC_Unit *unit), ARGMOD(SymReg **r), int num, int emit)
{
    ASSERT_ARGS(change_op)
    int changed = 0;

    if (r[num]->type & (VTCONST|VT_CONSTP)) {
        /* make a number const */
        const SymReg *c = r[num];

        if (c->type & VT_CONSTP)
            c = c->reg;

        r[num]  = mk_const(interp, c->name, 'N');
        changed = 1;
    }
    else if (emit) {
        /* emit
        *   set $N0, Iy
        *   op  Nx, $N0
        * or
        *   op  Nx, ..., $N0
        */
        SymReg *rr[2];

        rr[0]   = mk_temp_reg(interp, 'N');
        rr[1]   = r[num];

        INS(interp, unit, "set", NULL, rr, 2, 0, 1);

        r[num]  = rr[0];
        changed = 1;

        /* need to allocate the temp - run reg_alloc */
        IMCC_INFO(interp)->optimizer_level |= OPT_PASM;
    }

    return changed;
}

/*

=item C<int try_find_op(PARROT_INTERP, IMC_Unit *unit, const char *name, SymReg
**r, int n, int keyvec, int emit)>

Try to find valid op doing the same operation e.g.

   add_n_i_n  => add_n_n_i
   div_n_ic_n => div_n_nc_n
   div_n_i_n  => set_n_i ; div_n_n_n
   ge_n_ic_ic => ge_n_nc_ic
   acos_n_i   => acos_n_n

=cut

*/

PARROT_WARN_UNUSED_RESULT
int
try_find_op(PARROT_INTERP, ARGMOD(IMC_Unit *unit), ARGIN(const char *name),
        ARGMOD(SymReg **r), int n, int keyvec, int emit)
{
    ASSERT_ARGS(try_find_op)
    char fullname[64];
    int changed = 0;
    /*
     * eq_str, eq_num => eq
     * ...
     */
    if (n == 3 && r[2]->type == VTADDRESS) {
        if (STREQ(name, "eq_str") || STREQ(name, "eq_num")) {
            name    = "eq";
            changed = 1;
        }
        else if (STREQ(name, "ne_str") || STREQ(name, "ne_num")) {
            name    = "ne";
            changed = 1;
        }
        else if (STREQ(name, "le_str") || STREQ(name, "le_num")) {
            name    = "le";
            changed = 1;
        }
        else if (STREQ(name, "lt_str") || STREQ(name, "lt_num")) {
            name    = "lt";
            changed = 1;
        }
        else if (STREQ(name, "ge_str") || STREQ(name, "ge_num")) {
            name    = "ge";
            changed = 1;
        }
        else if (STREQ(name, "gt_str") || STREQ(name, "gt_num")) {
            name    = "gt";
            changed = 1;
        }
    }
    else if (n == 3 && (STREQ(name, "cmp_str") || STREQ(name, "cmp_num"))) {
        name     = "cmp";
        changed = 1;
    }
    if (n == 3 && r[0]->set == 'N') {
        if (r[1]->set == 'I') {
            const SymReg * const r1 = r[1];
            changed |= change_op(interp, unit, r, 1, emit);

            /* op Nx, Iy, Iy: reuse generated temp Nz */
            if (r[2]->set == 'I' && r[2]->type != VTADDRESS && r[2] == r1)
                r[2] = r[1];
        }

        if (r[2]->set == 'I' && r[2]->type != VTADDRESS)
            changed |= change_op(interp, unit, r, 2, emit);
    }

    /* handle eq_i_n_ic */
    else if (n == 3 && r[1]->set == 'N' && r[0]->set == 'I' &&
            r[2]->type == VTADDRESS) {
        changed |= change_op(interp, unit, r, 0, emit);
    }
    else if (n == 2 && r[0]->set == 'N' && r[1]->set == 'I') {
        /*
         * transcendentals  e.g. acos N, I
         */
        if (!STREQ(name, "fact"))
            changed = change_op(interp, unit, r, 1, emit);
    }

    if (changed) {
        op_fullname(fullname, name, r, n, keyvec);
        return interp->op_lib->op_code(fullname, 1);
    }

    return -1;
}

/*

=item C<static const char * try_rev_cmp(const char *name, SymReg **r)>

=cut

*/

PARROT_WARN_UNUSED_RESULT
PARROT_CAN_RETURN_NULL
static const char *
try_rev_cmp(ARGIN(const char *name), ARGMOD(SymReg **r))
{
    ASSERT_ARGS(try_rev_cmp)
    static struct br_pairs {
        PARROT_OBSERVER const char * const op;
        PARROT_OBSERVER const char * const nop;
        const int to_swap;
    } br_pairs[] = {
        { "gt",   "lt",   0 },
        { "ge",   "le",   0 },
        { "isgt", "islt", 1 },
        { "isge", "isle", 1 },
    };

    unsigned int i;

    for (i = 0; i < N_ELEMENTS(br_pairs); i++) {
        if (STREQ(name, br_pairs[i].op)) {
            const int to_swap = br_pairs[i].to_swap;
            SymReg *t;

            if (r[to_swap + 1]->set == 'P')
                return NULL;

            t              = r[to_swap];
            r[to_swap]     = r[to_swap + 1];
            r[to_swap + 1] = t;

            return br_pairs[i].nop;
        }
    }

    return NULL;
}


/*

=item C<int imcc_vfprintf(PARROT_INTERP, PMC *io, const char *format, va_list
ap)>

Formats a given series of arguments per a given format string and prints it to
the given Parrot IO PMC.

=cut

*/

PARROT_IGNORABLE_RESULT
int
imcc_vfprintf(PARROT_INTERP, ARGMOD(PMC *io), ARGIN(const char *format), va_list ap)
{
    ASSERT_ARGS(imcc_vfprintf)
    return Parrot_io_putps(interp, io, Parrot_vsprintf_c(interp, format, ap));
}

/* Utility functions */

/*

=item C<void imcc_init(PARROT_INTERP)>

=cut

*/

PARROT_EXPORT
void
imcc_init(PARROT_INTERP)
{
    ASSERT_ARGS(imcc_init)
    PARROT_ASSERT(IMCC_INFO(interp) == NULL);

    IMCC_INFO(interp) = mem_allocate_zeroed_typed(imc_info_t);
    /* register PASM and PIR compilers to parrot core */
    register_compilers(interp);
}

/*

=item C<static void imcc_destroy_macro_values(void *value)>

A callback for parrot_chash_destroy_values() to free all macro-allocated memory.

=cut

*/

static void
imcc_destroy_macro_values(ARGMOD(void *value))
{
    ASSERT_ARGS(imcc_destroy_macro_values)
    macro_t *  const m      = (macro_t *)value;
    params_t * const params = &m->params;

    int i;

    for (i = 0; i < params->num_param; ++i) {
        char * const name = params->name[i];
        if (name)
            mem_sys_free(name);
    }

    mem_sys_free(m->expansion);
    mem_sys_free(m);
}


/*

=item C<void imcc_destroy(PARROT_INTERP)>

=cut

*/

PARROT_EXPORT
void
imcc_destroy(PARROT_INTERP)
{
    ASSERT_ARGS(imcc_destroy)
    Hash * const macros = IMCC_INFO(interp)->macros;

    if (macros)
        parrot_chash_destroy_values(interp, macros, imcc_destroy_macro_values);

    if (IMCC_INFO(interp)->globals)
        mem_sys_free(IMCC_INFO(interp)->globals);

    mem_sys_free(IMCC_INFO(interp));
    IMCC_INFO(interp) = NULL;

    if (eval_nr != 0)
        MUTEX_DESTROY(eval_nr_lock);
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
