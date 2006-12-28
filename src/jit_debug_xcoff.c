/*
Copyright (C) 2001-2006, The Perl Foundation.
$Id$

=head1 NAME

src/jit_debug_xcoff.c - XCOFF stabs for JIT

=head1 DESCRIPTION

Write an XCOFF stabs file for JIT code. This file is based on
F<src/jit_debug.c>.

Stabs is a file format for information that describes a program to a
debugger.

For more information see the stabs documentation at
http://sources.redhat.com/gdb/current/onlinedocs/stabs_toc.html.

=head2 Functions

=over 4

=cut

*/

#include <parrot/parrot.h>
#include "parrot/exec.h"
#include "jit.h"

#ifdef __IBMC__

/* following from /usr/include/dbxstclass.h */
#  define C_GSYM        "0x80" /* global variable */
#  define C_LSYM        "0x81" /* stack variable */
#  define C_PSYM        "0x82" /* parameter */
#  define C_RSYM        "0x83" /* register variable */
#  define C_RPSYM       "0x84"
#  define C_STSYM       "0x85" /* variable in data section */
#  define C_TCSYM       "0x86"
#  define C_BCOMM       "0x87"
#  define C_ECOML       "0x88"
#  define C_ECOMM       "0x89"
#  define C_DECL        "0x8c" /* type declaration */
#  define C_ENTRY       "0x8d"
#  define C_FUN         "0x8e"
#  define C_BSTAT       "0x8f"
#  define C_ESTAT       "0x90"

void Parrot_jit_debug(Interp *interp);

#    define BIT_SIZE(t) ((int)(sizeof (t)*8))
#    define BYTE_SIZE(t) ((int)sizeof (t))
#    define BIT_OFFSET(str, field) ((int)(offsetof(str, field) * 8))

typedef struct {
    const char *name;
    const char *spec;
} BaseTypes;

/*

=item C<static void write_types(FILE *stabs, Interp *interp)>

Writes the types to C<stabs>.

=cut

*/

static void
write_types(FILE *stabs, Interp *interp)
{
    int i, j;
    /* borrowed from mono */
    static BaseTypes base_types[] = {
            {"Char", "-6"},
            {"Byte", "-5"},
            {"Int16", "-3"},
            {"UInt16", "-7"},
            {"Int32", "-1"}, /* 5 */
            {"UInt32", "-8"},
            {"Int64", ";01000000000000000000000;0777777777777777777777;"},
            {"UInt64", ";0000000000000;01777777777777777777777;"},
            {"Single", "-12"},
            {"Double", "-13"},  /* 10 */
            {"LongDouble", "-14"},
#  if INTVAL_SIZE == 4
            {"INTVAL", "5;"},       /* 12 */
#  else
            {"INTVAL", "7;"},
#  endif
#  if NUMVAL_SIZE == 8
            {"FLOATVAL", "10;"},    /* 13 */
#  else
            {"FLOATVAL", "11;"},
#  endif
            {"Ptr", "*0;"},
            {"CharPtr", "*1;"},     /* 15 */
            {0, 0}
        };
    for (i = 0; base_types[i].name; ++i) {
        if (! base_types[i].spec)
            continue;
        fprintf(stabs, ".stabx \"%s:t%d=", base_types[i].name, i);
        if (base_types[i].spec [0] == ';') {
            fprintf(stabs, "r%d%s\"", i, base_types[i].spec);
        }
        else {
            fprintf(stabs, "%s\"", base_types[i].spec);
        }
        fprintf(stabs, ",0," C_DECL ",0\n");
    }
    fprintf(stabs, ".stabx \"STRING:t%d=*%d\""
                ",0," C_DECL ",0\n", i, i+1);
    ++i;
    fprintf(stabs, ".stabx \"Parrot_String:T%d=s%d"
                "bufstart:14,%d,%d;"
                "buflen:6,%d,%d;"   /* XXX type */
                "flags:12,%d,%d;"
                "bufused:12,%d,%d;"
                "strstart:15,%d,%d;"        /* fake a char* */
                ";\""
                ",0," C_DECL ",0\n", i++, BYTE_SIZE(STRING),
                BIT_OFFSET(STRING, obj.u._b._bufstart), BIT_SIZE(void*),
                BIT_OFFSET(STRING, obj.u._b._buflen), BIT_SIZE(size_t),
                BIT_OFFSET(STRING, obj.flags), BIT_SIZE(UINTVAL),
                BIT_OFFSET(STRING, bufused), BIT_SIZE(UINTVAL),
                BIT_OFFSET(STRING, strstart), BIT_SIZE(void*)
                );

    fprintf(stabs, ".stabx \"PMCType:T%d=e", i++);
    for (j = 0; j < interp->n_vtable_max; ++j) {
        if (interp->vtables[j] && interp->vtables[j]->whoami) {
            STRING* name = interp->vtables[j]->whoami;
            fwrite(name->strstart, name->strlen, 1, stabs);
            fprintf(stabs, ":%d,", j);
        }
    }
    fprintf(stabs, ";\",0," C_DECL ",0\n");

    /* PMC type */
    fprintf(stabs, ".stabx \"PMC:T%d=s%d", i, BYTE_SIZE(PMC));
    fprintf(stabs, "obj:%d,%d,%d;",
            i + 1, BIT_OFFSET(PMC, obj), BIT_SIZE(pobj_t));
    fprintf(stabs, "vtable:*%d,%d,%d;",
            i + 3, BIT_OFFSET(PMC, vtable), BIT_SIZE(void*));
#  if ! PMC_DATA_IN_EXT
    fprintf(stabs, "data:14,%d,%d;",
            BIT_OFFSET(PMC, data), BIT_SIZE(void*));
#  endif
    fprintf(stabs, "pmc_ext:*%d,%d,%d;",
            i, BIT_OFFSET(PMC, pmc_ext), BIT_SIZE(void*));
    fprintf(stabs, ";\"");
    fprintf(stabs, ",0," C_DECL ",0\n");

    fprintf(stabs, ".stabx \"pobj_t:T%d=s%d"
                "u:%d,%d,%d;"
                "flags:12,%d,%d;"
                ";\""
                ",0," C_DECL ",0\n", i + 1, (int)(sizeof (pobj_t)),
                i + 2, BIT_OFFSET(pobj_t, u), BIT_SIZE(UnionVal),
                BIT_OFFSET(pobj_t, flags), BIT_SIZE(Parrot_UInt)
                );
    fprintf(stabs, ".stabx \"UnionVal:T%d=u%d"
                "int_val:12,%d,%d;"
                "pmc_val:*%d,%d,%d;"
                ";\""
                ",0," C_DECL ",0\n", i + 2, BYTE_SIZE(UnionVal),
                BIT_OFFSET(UnionVal, int_val), BIT_SIZE(INTVAL),
                i, BIT_OFFSET(UnionVal, pmc_val), BIT_SIZE(void*)
                );
    fprintf(stabs, ".stabx \"VTABLE:T%d=s%d"
                "base_type:%d,%d,%d;"
                ";\""
                ",0," C_DECL ",0\n", i + 3, BYTE_SIZE(UnionVal),
                i - 1, BIT_OFFSET(VTABLE, base_type), BIT_SIZE(INTVAL)
                );
    i += 4;

}

/*

=item C<static void
write_vars(FILE *stabs, Interp *interp)>

Writes the contents of the registers to C<stabs>.

=cut

*/

static void
write_vars(FILE *stabs, Interp *interp)
{
    int i;
    /* fake static var stabs */
    fprintf(stabs, ".bs parrot_jit_vars\n");
    for (i = 0; i < NUM_REGISTERS; i++) {
        fprintf(stabs, ".stabx \"I%d:V12\",0x%p," C_STSYM ",0\n", i,
                (char*)&REG_INT(i));
        fprintf(stabs, ".stabx \"N%d:V13\",0x%p," C_STSYM ",0\n", i,
                (char*)&REG_NUM(i));
        fprintf(stabs, ".stabx \"S%d:V16\",0x%p," C_STSYM ",0\n", i,
                (char*)&REG_STR(i));
        fprintf(stabs, ".stabx \"P%d:V19\",0x%p," C_STSYM ",0\n", i,
                (char*)&REG_PMC(i));
    }
    fprintf(stabs, ".es\n");
}

/*

=item C<static STRING *
debug_file(Interp *interp, STRING *file, const char *ext)>

Returns C<file> with C<ext> appended.

=cut

*/

static STRING *
debug_file(Interp *interp, STRING *file, const char *ext)
{
    STRING *ret;
    ret = string_copy(interp, file);
    ret = string_append(interp, ret,
            string_make(interp, ext, strlen(ext), NULL,
                PObj_external_FLAG));
    return ret;
}

/*

=item C<static void
Parrot_jit_debug_stabs(Interp *interp)>

Writes the JIT debugging stabs.

=cut

*/

static void
Parrot_jit_debug_stabs(Interp *interp)
{
    Parrot_jit_info_t *jit_info = interp->jit_info;
    STRING *file = NULL;
    STRING *pasmfile, *stabsfile, *ofile, *cmd;
    FILE *stabs;
    size_t i;
    int line;
    opcode_t lc;

    if (interp->code->debugs) {
        char *ext;
        char *src = string_to_cstring(interp,
            Parrot_debug_pc_to_filename(interp,
            interp->code->debugs, 0));
        pasmfile = string_make(interp, src, strlen(src), NULL,
                PObj_external_FLAG);
        file = string_copy(interp, pasmfile);
        /* chop pasm/pir */

        ext = strrchr(src, '.');
        if (ext && strcmp(ext, ".pasm") == 0)
            file = string_chopn(interp, file, 4, 1);
        else if (ext && strcmp(ext, ".pir") == 0)
            file = string_chopn(interp, file, 3, 1);
        else if (!ext) /* EVAL_n */
            file = string_append(interp, file,
                    string_make(interp, ".", 1, NULL, PObj_external_FLAG));
    }
    else {
        /* chop pbc */
        file = string_chopn(interp, file, 3, 1);
        pasmfile = debug_file(interp, file, "pasm");
    }
    stabsfile = debug_file(interp, file, "stabs.s");
    ofile = debug_file(interp, file, "o");
    {
        char *temp = string_to_cstring(interp,stabsfile);
        stabs = fopen(temp, "w");
        free(temp);
    }
    if (stabs == NULL)
        return;

    {
        char *temp = string_to_cstring(interp, pasmfile);
        /* filename info */
        fprintf(stabs, ".file \"%s\"\n",temp);
        free(temp);
    }
    /* declare function name */
    fprintf(stabs, ".jit_func:\n" );
    /* jit_func start addr */
    fprintf(stabs, ".stabx \"jit_func:F1\",0x%p," C_FUN ",0\n",
            jit_info->arena.start);
    fprintf(stabs, ".function .jit_func,.jit_func,0,0\n" );
    fprintf(stabs, ".bf 1\n" );

    write_types(stabs, interp);
    write_vars(stabs, interp);
    /* if we don't have line numbers, emit dummys, assuming there are
     * no comments and spaces in source for testing
     */

    /* jit_begin */
    fprintf(stabs, ".line 1\n");
    line = 1;
    lc = 0;
    for (i = 0; i < interp->code->base.size; i++) {
        if (jit_info->arena.op_map[i].ptr) {
            op_info_t* op = &interp->op_info_table[
                interp->code->base.data[i]];
            if (interp->code->debugs) {
                line = (int)interp->code->debugs->base.data[lc++];
            }
            fprintf(stabs, ".line %d # 0x%p %s\n", line,
                    (int)((char *)jit_info->arena.op_map[i].ptr -
                    (char *)jit_info->arena.start),
                    op->full_name);
            line++;
        }
    }
    /* eof */
    fprintf(stabs, ".stabx \"\",0x%p," C_FUN ",0\n",
            (char *) jit_info->arena.size);
    fprintf(stabs, ".ef %d\n",line);
    fclose(stabs);
    /* run the stabs file through C<as> generating file.o */
    cmd = Parrot_sprintf_c(interp, "as %Ss -o %Ss", stabsfile, ofile);

    {
        char *temp = string_to_cstring(interp, cmd);
        system(temp);
        free(temp);
    }
}

/*

=item C<void
Parrot_jit_debug(Interp* interp)>

Writes the JIT debugging stabs. Just calls C<Parrot_jit_debug_stabs()>.

=cut

*/

void
Parrot_jit_debug(Interp* interp)
{
    Parrot_jit_debug_stabs(interp);
}

#endif


/*

=back

=head1 SEE ALSO

F<src/jit_debug.c>, F<src/jit.c>, F<src/jit.h>.

=cut

*/


/*
 * Local variables:
 *   c-file-style: "parrot"
 * End:
 * vim: expandtab shiftwidth=4:
 */
