/*
Copyright (C) 2001-2008, Parrot Foundation.
$Id$

=head1 NAME

src/jit_debug.c - Write stabs file for JIT code

=head1 SYNOPSIS

When debugging JIT code with C<gdb>, do:

    add-symbol-file <file.o> 0

=head1 DESCRIPTION

Stabs is a file format for information that describes a program to a
debugger.

For more information see the stabs documentation at
http://sources.redhat.com/gdb/current/onlinedocs/stabs_toc.html.

=head2 Functions

=over 4

=cut

*/

/* HEADERIZER HFILE: none */
/* HEADERIZER STOP */

#include <parrot/parrot.h>
#include "parrot/exec.h"
#include "jit.h"

#define N_GSYM "32"     /* global variable */
#define N_FNAME "34"
#define N_FUN "36"
#define N_STSYM "38"    /* variable in data section */
#define N_LCSYM "40"    /* bss section */
#define N_MAIN "42"
#define N_ROSYM "44"
#define N_PC "48"
#define N_NSYMS "50"
#define N_NOMAP "52"
#define N_OBJ "56"
#define N_OPT "60"
#define N_RSYM "64"     /* register variable */
#define N_M2C "66"
#define N_SLINE "68"
#define N_DSLINE "70"
#define N_BSLINE "72"
#define N_BROWS "72"
#define N_DEFD "74"
#define N_FLINE "76"
#define N_EHDECL "80"
#define N_MOD2 "80"
#define N_CATCH "84"
#define N_SSYM "96"
#define N_ENDM "98"
#define N_SO "100"     /* filename */
#define N_LSYM "128"   /* stack variable */
#define N_BINCL "130"
#define N_SOL "132"
#define N_PSYM "160"   /* parameter */
#define N_EINCL "162"
#define N_ENTRY "164"
#define N_LBRAC "192"
#define N_EXCL "194"
#define N_SCOPE "196"
#define N_RBRAC "224"
#define N_BCOMM "226"
#define N_ECOMM "228"
#define N_ECOML "232"
#define N_WITH "234"
#define N_NBTEXT "240"
#define N_NBDATA "242"
#define N_NBBSS "244"
#define N_NBSTS "246"
#define N_NBLCS "248"

#ifdef __GNUC__
void Parrot_jit_debug(PARROT_INTERP);

#  define BIT_SIZE(t) ((int)(sizeof (t)*8))
#  define BYTE_SIZE(t) ((int)sizeof (t))
#  define BIT_OFFSET(str, field) ((int)(offsetof(str, field) * 8))

typedef struct BaseTypes {
    const char *name;
    const char *spec;
} BaseTypes;

/*

=item C<static void write_types(FILE *stabs, PARROT_INTERP)>

Writes the types to C<stabs>.

=cut

*/

static void
write_types(FILE *stabs, PARROT_INTERP)
{
    /* It would be create if this function would be auto generated */
    int i, j;
    /* borrowed from mono */
    static BaseTypes base_types[] = {
            {"Void", "(0,1)"},
            {"Char", ";-128;127;"},
            {"Byte", ";0;255;"},
            {"Int16", ";-32768;32767;"},
            {"UInt16", ";0;65535;"},
            {"Int32", ";0020000000000;0017777777777;"}, /* 5 */
            {"UInt32", ";0000000000000;0037777777777;"},
            {"Int64", ";01000000000000000000000;0777777777777777777777;"},
            {"UInt64", ";0000000000000;01777777777777777777777;"},
            {"Single", "r(0,8);4;0;"},
            {"Double", "r(0,8);8;0;"},  /* 10 */
            {"LongDouble", "r(0,8);12;0;"},
#  if INTVAL_SIZE == 4
            {"INTVAL", "(0,5);"},       /* 12 */
#  else
            {"INTVAL", "(0,7);"},
#  endif
#  if NUMVAL_SIZE == 8
            {"FLOATVAL", "(0,10);"},    /* 13 */
#  else
            {"FLOATVAL", "(0,11);"},
#  endif
            {"Ptr", "*(0,0);"},
            {"CharPtr", "*(0,1);"},     /* 15 */
            {0, 0}
        };
    for (i = 0; base_types[i].name; ++i) {
        if (! base_types[i].spec)
            continue;
        fprintf(stabs, ".stabs \"%s:t(0,%d)=", base_types[i].name, i);
        if (base_types[i].spec [0] == ';') {
            fprintf(stabs, "r(0,%d)%s\"", i, base_types[i].spec);
        }
        else {
            fprintf(stabs, "%s\"", base_types[i].spec);
        }
        fprintf(stabs, "," N_LSYM ",0,0,0\n");
    }
    fprintf(stabs, ".stabs \"STRING:t(0,%d)=*(0,%d)\""
                "," N_LSYM ",0,0,0\n", i, i+1);
    ++i;
    fprintf(stabs, ".stabs \"Parrot_String:T(0,%d)=s%d"
                "bufstart:(0,14),%d,%d;"
                "buflen:(0,6),%d,%d;"
                "flags:(0,12),%d,%d;"
                "bufused:(0,12),%d,%d;"
                "strstart:(0,15),%d,%d;"
                ";\""
                "," N_LSYM ",0,0,0\n", i++, BYTE_SIZE(STRING),
                BIT_OFFSET(STRING, _bufstart), BIT_SIZE(void*),
                BIT_OFFSET(STRING, _buflen), BIT_SIZE(size_t),
                BIT_OFFSET(STRING, flags), BIT_SIZE(UINTVAL),
                BIT_OFFSET(STRING, bufused), BIT_SIZE(UINTVAL),
                BIT_OFFSET(STRING, strstart), BIT_SIZE(void*));

    fprintf(stabs, ".stabs \"PMCType:T(0,%d)=e", i++);
    for (j = 0; j < interp->n_vtable_max; ++j) {
        if (interp->vtables[j] && interp->vtables[j]->whoami) {
            STRING *name  = interp->vtables[j]->whoami;
            size_t  items = fwrite(name->strstart, name->strlen, 1, stabs);
            if (!items)
                fprintf(stderr, "Error writing stabs!\n");
            fprintf(stabs, ":%d,", j);
        }
    }

    fprintf(stabs, ";\"," N_LSYM ",0,0,0\n");

    fprintf(stabs, ".stabs \"PMC:T(0,%d)=s%d"
                "flags:(0,12),%d,%d;"
                "vtable:*(0,%d),%d,%d;"
                "data:(0,14),%d,%d;"
                "_metadata:*(0,%d),%d,%d;"
                "_next_for_GC:*(0,%d),%d,%d;"
                ";\""
                "," N_LSYM ",0,0,0\n", i, BYTE_SIZE(PMC),
                BIT_OFFSET(PMC, flags), BIT_SIZE(UINTVAL),
                i + 1, BIT_OFFSET(PMC, vtable), BIT_SIZE(void*),
                BIT_OFFSET(PMC, data), BIT_SIZE(void*),
                i, BIT_OFFSET(PMC, _metadata), BIT_SIZE(void*),
                i, BIT_OFFSET(PMC, _next_for_GC), BIT_SIZE(void*));

    i++;

    /* someone can add some field to this one */
    fprintf(stabs, ".stabs \"VTABLE:T(0,%d)=s%d"
                "base_type:(0,12),%d,%d;"
                ";\""
                "," N_LSYM ",0,0,0\n", i, BYTE_SIZE(_vtable),
                BIT_OFFSET(VTABLE, base_type), BIT_SIZE(INTVAL));

    i++;
}

/*

=item C<static void
write_vars(FILE *stabs, PARROT_INTERP)>

Writes the contents of the registers to C<stabs>.

=cut

*/

static void
write_vars(FILE *stabs, PARROT_INTERP)
{
    int i;
    /* fake static var stabs */
    for (i = 0; i < NUM_REGISTERS; i++) {
        fprintf(stabs, ".stabs \"I%d:S(0,12)\"," N_STSYM ",0,0,%p\n", i,
                (char*)&REG_INT(interp, i));
        fprintf(stabs, ".stabs \"N%d:S(0,13)\"," N_STSYM ",0,0,%p\n", i,
                (char*)&REG_NUM(interp, i));
        fprintf(stabs, ".stabs \"S%d:S(0,16)\"," N_STSYM ",0,0,%p\n", i,
                (char*)&REG_STR(interp, i));
        fprintf(stabs, ".stabs \"P%d:S*(0,19)\"," N_STSYM ",0,0,%p\n", i,
                (char*)&REG_PMC(interp, i));
    }
}

/*

=item C<static STRING *
debug_file(PARROT_INTERP, STRING *file, const char *ext)>

Returns C<file> with C<ext> appended.

=cut

*/

static STRING *
debug_file(PARROT_INTERP, STRING *file, const char *ext)
{
    STRING *ret;
    ret = Parrot_str_copy(interp, file);
    ret = Parrot_str_append(interp, ret,
            string_make(interp, ext, strlen(ext), NULL,
                PObj_external_FLAG));
    return ret;
}

/*

=item C<static void
Parrot_jit_debug_stabs(PARROT_INTERP)>

Writes the JIT debugging stabs.

=cut

*/

static void
Parrot_jit_debug_stabs(PARROT_INTERP)
{
    Parrot_jit_info_t *jit_info = interp->code->jit_info;
    STRING *file = NULL;
    STRING *pasmfile, *stabsfile, *ofile, *cmd;
    FILE *stabs;
    size_t i;
    int line;
    opcode_t lc;
    PackFile_Debug *debug;

    if (interp->code->debugs) {
        char *ext;
        char * const src = Parrot_str_to_cstring(interp,
            Parrot_debug_pc_to_filename(interp,
            interp->code->debugs, 0));
        pasmfile = string_make(interp, src, strlen(src), NULL,
                PObj_external_FLAG);
        file = Parrot_str_copy(interp, pasmfile);
        /* chop pasm/pir */

        ext = strrchr(src, '.');
        if (ext && STREQ(ext, ".pasm"))
            Parrot_str_chopn_inplace(interp, file, 4);
        else if (ext && STREQ(ext, ".pir"))
            Parrot_str_chopn_inplace(interp, file, 3);
        else if (!ext) /* EVAL_n */
            file = Parrot_str_append(interp, file,
                    string_make(interp, ".", 1, NULL, PObj_external_FLAG));

        Parrot_str_free_cstring(src);
    }
    else {
        /* chop pbc */
        Parrot_str_chopn_inplace(interp, file, 3);
        pasmfile = debug_file(interp, file, "pasm");
    }
    stabsfile = debug_file(interp, file, "stabs.s");
    ofile     = debug_file(interp, file, "o");
    {
        char *const temp = Parrot_str_to_cstring(interp, stabsfile);
        stabs            = fopen(temp, "w");
        Parrot_str_free_cstring(temp);
    }
    if (stabs == NULL)
        return;

    {
        char * const temp = Parrot_str_to_cstring(interp, pasmfile);
        /* filename info */
        fprintf(stabs, ".data\n.text\n");       /* darwin wants it */
        fprintf(stabs, ".stabs \"%s\"," N_SO ",0,0,0\n", temp);
        Parrot_str_free_cstring(temp);
    }
    /* jit_func start addr */
    fprintf(stabs, ".stabs \"jit_func:F(0,1)\"," N_FUN ",0,1,%p\n",
            jit_info->arena.start);

    write_types(stabs, interp);
    write_vars(stabs, interp);
    /* if we don't have line numbers, emit dummys, assuming there are
     * no comments and spaces in source for testing
     */

    /* jit_begin */
    fprintf(stabs, ".stabn " N_SLINE ",0,1,0\n");
    line = 1;
    lc = 0;
    debug = interp->code->debugs;
    for (i = 0; i < interp->code->base.size; i++) {
        if (jit_info->arena.op_map[i].ptr) {
            op_info_t* op = &interp->op_info_table[
                interp->code->base.data[i]];
            if (interp->code->debugs) {
                if (lc >= (int)(debug->base.size))
                    break;
                line = (int)debug->base.data[lc++];
            }
            fprintf(stabs, ".stabn " N_SLINE ",0,%d,%d /* %s */\n", line,
                    (int)((char *)jit_info->arena.op_map[i].ptr -
                    (char *)jit_info->arena.start),
                    op->full_name);
            line++;
        }
    }
    /* eof */
    fprintf(stabs, ".stabs \"\"," N_FUN ",0,1,%p\n",
            (char *) jit_info->arena.size);
    fclose(stabs);

    /* run the stabs file through C<as> generating file.o */
    cmd = Parrot_sprintf_c(interp, "as %Ss -o %Ss", stabsfile, ofile);

    {
        char * const temp   = Parrot_str_to_cstring(interp, cmd);
        int          status = system(temp);
        if (status)
            fprintf(stderr, "Assembly failed: %d\n%s\n", status, temp);
        Parrot_str_free_cstring(temp);
    }
}

/*

=item C<void
Parrot_jit_debug(PARROT_INTERP)>

Writes the JIT debugging stabs. Just calls C<Parrot_jit_debug_stabs()>.

=cut

*/

void
Parrot_jit_debug(PARROT_INTERP)
{
    Parrot_jit_debug_stabs(interp);
}


#endif

/*

=back

=head1 SEE ALSO

F<src/jit.c>, F<include/parrot/jit.h>, F<docs/jit.pod>.

=cut

*/


/*
 * Local variables:
 *   c-file-style: "parrot"
 * End:
 * vim: expandtab shiftwidth=4:
 */
