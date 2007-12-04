/*
Copyright (C) 2001-2007, The Perl Foundation.
$Id$

=head1 NAME

src/exec.c - Generate an object file

=head1 DESCRIPTION

=head2 Functions

=over 4

=cut

*/

#include <parrot/parrot.h>
#if HAVE_COMPUTED_GOTO
#  include <parrot/oplib/core_ops_cgp.h>
#endif /* HAVE_COMPUTED_GOTO */
#include "parrot/exec.h"
#include "jit.h"
#define JIT_EMIT 1
#include "jit_emit.h"
#include "exec_dep.h"
#include "exec_save.h"
#include "parrot/compiler.h"

/* HEADERIZER HFILE: include/parrot/exec.h */

/* HEADERIZER BEGIN: static */

static void add_data_member(
    NOTNULL(Parrot_exec_objfile_t *obj),
    NULLOK(void *src),
    size_t len)
        __attribute__nonnull__(1);

static void exec_init(NOTNULL(Parrot_exec_objfile_t *obj))
        __attribute__nonnull__(1);

static int symbol_list_find(
    NOTNULL(Parrot_exec_objfile_t *obj),
    NOTNULL(const char *symbol))
        __attribute__nonnull__(1)
        __attribute__nonnull__(2);

/* HEADERIZER END: static */

/*
 * Parrot_exec_run must be 0 while the program runs.
 * It will be set to 2 inside eval (s. eval.pmc)
 * to switch to runops_jit (s. interpreter.c:runops_exec()).
 * Must be 1 while starting the compiled code to have Parrot_new()
 * return the address of the global interpreter (s. interpreter.c)
 * and PackFile_ConstTable_unpack use the global const_table (s. packfile.c).
 * Must also be 1 while generating the executable.
 */

int Parrot_exec_run = 0;
extern PARROT_API char **Parrot_exec_rel_addr;
extern PARROT_API int Parrot_exec_rel_count;

/*

=item C<void
Parrot_exec(PARROT_INTERP, opcode_t *pc,
        opcode_t *code_start, opcode_t *code_end)>

Call the jit to get the program code. Adds the members of the data
section. And emits the executable.

=cut

*/

void
Parrot_exec(PARROT_INTERP, NOTNULL(opcode_t *pc),
        NOTNULL(opcode_t *code_start), NOTNULL(opcode_t *code_end))
{
#ifdef JIT_CGP
    int i, j *k;
#endif
    const char *output;
    long bhs;
    Parrot_jit_info_t *jit_info;

    Parrot_exec_objfile_t * const obj =
        mem_allocate_zeroed_typed(Parrot_exec_objfile_t);
    exec_init(obj);
    Parrot_exec_rel_addr = (char **)mem_sys_allocate_zeroed(4 * sizeof (char *));
    obj->bytecode_header_size =
        (interp->code->base.file_offset + 4) * sizeof (opcode_t);
    jit_info = parrot_build_asm(interp, code_start, code_end,
            obj, JIT_CODE_FILE);

    /* TODO Go zero the calls to jited opcodes. */
    /* Place the program code in the data section. */
    /* program_code */
    add_data_member(obj, interp->code->base.pf->src,
            interp->code->base.pf->size);
    /* opcode_map */
    add_data_member(obj, jit_info->arena.op_map, (jit_info->arena.map_size+1) *
        sizeof (opcode_t *));
    /* const_table */
    add_data_member(obj, NULL, interp->code->const_table->const_count *
        sizeof (PackFile_Constant));
#ifdef JIT_CGP
    /* prederef_code */
    j = (int)cgp_core;
    j = (int)((op_func_t*)interp->op_lib->op_func_table)[2] - j;
    k = (int *)interp->code->prederef.code;
    for (i = 0; i < (int)interp->code->base.size; i++) {
        if (k[i] != j)
            k[i] = 0;
    }
    add_data_member(obj, interp->code->prederef.code,
        interp->code->base.size * sizeof (void *));
#endif /* JIT_CGP */
    /* bytecode_offset */
    bhs = obj->bytecode_header_size / sizeof (opcode_t);
    add_data_member(obj, &bhs, 4);

    obj->text.code = jit_info->arena.start;
    obj->text.size = jit_info->arena.size;

    /* PAD */
    obj->text.size += (4 - obj->text.size % 4);
    obj->data.size += (4 - obj->data.size % 4);
    offset_fixup(obj);
    output = interp->output_file ?
        interp->output_file : "exec_output.o";
    Parrot_exec_save(interp, obj, output);
}

/*

=item C<static void
add_data_member(NOTNULL(Parrot_exec_objfile_t *obj), NULLOK(void *src), size_t len)>

Adds a member to the data section, storing the size of it at
C<< obj->data_size[N] >>.

=cut

*/

static void
add_data_member(NOTNULL(Parrot_exec_objfile_t *obj), NULLOK(void *src), size_t len)
{
    char *cp;

    if (obj->data.size == 0) {
        obj->data.code = (char *)mem_sys_allocate(len);
        obj->data_size = (int *)mem_sys_allocate(sizeof (int));
    }
    else {
        int *nds;
        obj->data.code = (char *)mem_sys_realloc(obj->data.code,
                                                 obj->data.size + len);
        nds = (int *)mem_sys_realloc(obj->data_size, (obj->data_count + 2) *
            sizeof (int));
        obj->data_size = nds;
    }

    cp = obj->data.code + obj->data.size;
    if (src)
        memcpy(cp, src, len);
    else
        memset(cp, 0, len);
    obj->data_size[obj->data_count++] = len;
    obj->data.size += len;
}

/*

=item C<static void
exec_init(NOTNULL(Parrot_exec_objfile_t *obj))>

Initialize the obj structure.

=cut

*/

static void
exec_init(NOTNULL(Parrot_exec_objfile_t *obj))
{
    obj->text_rellocation_table = (Parrot_exec_rellocation_t *)
        mem_sys_allocate_zeroed(sizeof (Parrot_exec_rellocation_t));
    obj->data_rellocation_table = (Parrot_exec_rellocation_t *)
        mem_sys_allocate_zeroed(sizeof (Parrot_exec_rellocation_t));
    obj->symbol_table = (Parrot_exec_symbol_t *)
        mem_sys_allocate_zeroed(sizeof (Parrot_exec_symbol_t));
    /* size of table */
#ifdef PARROT_I386
    obj->symbol_list_size = 4;
#else
    obj->symbol_list_size = 0;
#endif
#ifdef EXEC_ELF
    obj->symbol_list_size = 0;
#endif
    Parrot_exec_add_symbol(obj, "program_code", STYPE_GDATA);
    Parrot_exec_add_symbol(obj, "opcode_map", STYPE_GDATA);
    Parrot_exec_add_symbol(obj, "const_table", STYPE_GDATA);
#ifdef JIT_CGP
    Parrot_exec_add_symbol(obj, "exec_prederef_code", STYPE_GDATA);
#endif /* JIT_CGP */
    Parrot_exec_add_symbol(obj, "bytecode_offset", STYPE_GDATA);
    Parrot_exec_add_symbol(obj, "run_compiled", STYPE_FUNC);
}

/*

=item C<int
Parrot_exec_add_symbol(NOTNULL(Parrot_exec_objfile_t *obj),
                       NOTNULL(const char *symbol), int stype)>

Adds a symbol to the object file.

=cut

*/

int
Parrot_exec_add_symbol(NOTNULL(Parrot_exec_objfile_t *obj),
                       NOTNULL(const char *symbol), int stype)
{

    int symbol_number = symbol_list_find(obj, symbol);
    if (symbol_number == -1) {
        Parrot_exec_symbol_t *new_symbol;

        symbol_number = obj->symbol_count;
        new_symbol = (Parrot_exec_symbol_t *)mem_sys_realloc(obj->symbol_table,
            (size_t)(obj->symbol_count + 1) * sizeof (Parrot_exec_symbol_t));
        obj->symbol_table = new_symbol;

        new_symbol = &obj->symbol_table[obj->symbol_count++];
        new_symbol->offset_list = obj->symbol_list_size;
        new_symbol->symbol = symbol;
        obj->symbol_list_size += strlen(symbol);
#if defined(EXEC_A_OUT) || defined(EXEC_COFF)
        /* for the trailing "_" */
        obj->symbol_list_size++;
#endif
        if (stype != STYPE_GCC)
            obj->symbol_list_size++;
        if (stype == STYPE_COM) {
            new_symbol->type = STYPE_COM;
            new_symbol->value = sizeof (Interp);
        }
        else {
            new_symbol->type = stype;
            new_symbol->value = 0;
        }
    }
    return symbol_number;
}

/*

=item C<PARROT_API
int *
Parrot_exec_add_text_rellocation_reg(NOTNULL(Parrot_exec_objfile_t *obj),
                                     char *nptr, const char *var, int offset,
                                     int disp)>

TODO: Not yet documented!!!

=cut

*/

PARROT_API
PARROT_CAN_RETURN_NULL
int *
Parrot_exec_add_text_rellocation_reg(NOTNULL(Parrot_exec_objfile_t *obj),
                                     NOTNULL(char *nptr), NOTNULL(const char *var),
                                     int offset, int disp)
{
    Parrot_exec_add_text_rellocation(obj, nptr, RTYPE_COM, var, disp);
    return (int *)offset;
}

/*

=item C<PARROT_API
void
Parrot_exec_add_text_rellocation_func(NOTNULL(Parrot_exec_objfile_t *obj),
                                      char *nptr, const char *func_name)>

TODO: Not yet documented!!!

=cut

*/

PARROT_API
void
Parrot_exec_add_text_rellocation_func(NOTNULL(Parrot_exec_objfile_t *obj),
                                      NOTNULL(char *nptr), NOTNULL(const char *func_name))
{
    Parrot_exec_add_text_rellocation(obj, nptr, RTYPE_FUNC, func_name, 1);
}

/*

=item C<PARROT_API
void
Parrot_exec_add_text_rellocation(NOTNULL(Parrot_exec_objfile_t *obj), char *nptr,
    int type, const char *symbol, int disp)>

Adds a text rellocation to the object file.

=cut

*/

PARROT_API
void
Parrot_exec_add_text_rellocation(NOTNULL(Parrot_exec_objfile_t *obj), NOTNULL(char *nptr),
    int type, NOTNULL(const char *symbol), int disp)
{
    int symbol_number;
    char *addr;
    Parrot_exec_rellocation_t * const new_relloc = (Parrot_exec_rellocation_t *)
        mem_sys_realloc(obj->text_rellocation_table,
            (size_t)(obj->text_rellocation_count + 1) *
            sizeof (Parrot_exec_rellocation_t));

    obj->text_rellocation_table = new_relloc;
    new_relloc = &obj->text_rellocation_table[obj->text_rellocation_count++];

    switch (type) {
        case RTYPE_FUNC:
            symbol_number = Parrot_exec_add_symbol(obj, symbol, STYPE_UND);
            break;
        case RTYPE_COM:
            symbol_number = Parrot_exec_add_symbol(obj, symbol, STYPE_COM);
            break;
        case RTYPE_DATA:
        case RTYPE_DATA1:
            symbol_number = Parrot_exec_add_symbol(obj, symbol, STYPE_GDATA);
            break;
        default:
            symbol_number = 0;
            break;
    }

    if (Parrot_exec_rel_count)
        addr = Parrot_exec_rel_addr[--Parrot_exec_rel_count];
    else
        addr = nptr + disp;
    new_relloc->offset = (int)(addr - obj->text.code);
    new_relloc->symbol_number = symbol_number;
    new_relloc->type = type;
}

/*

=item C<static int
symbol_list_find(NOTNULL(Parrot_exec_objfile_t *obj), NOTNULL(const char *symbol))>

Returns the index of C<symbol> in the symbol list. Returns -1 if it is
not in the list.

Used by C<Parrot_exec_add_symbol()>.

=cut

*/

static int
symbol_list_find(NOTNULL(Parrot_exec_objfile_t *obj), NOTNULL(const char *symbol))
{
    int i;

    for (i = 0; i < obj->symbol_count; i++)
        if (strcmp(symbol, obj->symbol_table[i].symbol) == 0)
            return i;
    return -1;
}

/*

=back

=head1 SEE ALSO

F<include/parrot/exec.h>, F<src/exec_cpu.c>, F<src/exec_save.h>
and F<src/exec_start.c>.

=head1 HISTORY

Initial version by Daniel Grunblatt on 2003.6.9.

=cut

*/


/*
 * Local variables:
 *   c-file-style: "parrot"
 * End:
 * vim: expandtab shiftwidth=4:
 */
