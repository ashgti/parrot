/*
 * jit_debug.c
 *
 * $Id$
 *
 * write stabs file for jit code
 * when debugging jit code with gdb, do:
 *
 * add-symbol-file <file.o> 0
 *
 */

#include <parrot/parrot.h>
#include "parrot/jit.h"

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
void Parrot_jit_debug(struct Parrot_Interp* interpreter);

#define BIT_SIZE(t) ((int)(sizeof(t)*8))
#define BYTE_SIZE(t) ((int)sizeof(t))
#define BIT_OFFSET(str, field) ((int)(offsetof(str, field) * 8))

typedef struct {
	const char *name;
	const char *spec;
} BaseTypes;
static void
write_types(FILE *stabs)
{
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
#if INTVAL_SIZE == 4
            {"INTVAL", "(0,5);"},       /* 12 */
#else
            {"INTVAL", "(0,7);"},
#endif
#if NUMVAL_SIZE == 8
            {"FLOATVAL", "(0,10);"},    /* 13 */
#else
            {"FLOATVAL", "(0,11);"},
#endif
            {"Ptr", "*(0,0);"},
            {"CharPtr", "*(0,1);"},     /* 15 */
            {0, 0}
        };
    for (i = 0; base_types[i].name; ++i) {
        if (! base_types[i].spec)
            continue;
        fprintf (stabs, ".stabs \"%s:t(0,%d)=", base_types[i].name, i);
        if (base_types[i].spec [0] == ';') {
            fprintf (stabs, "r(0,%d)%s\"", i, base_types[i].spec);
        } else {
            fprintf (stabs, "%s\"", base_types[i].spec);
        }
        fprintf (stabs, "," N_LSYM ",0,0,0\n");
    }
    fprintf(stabs, ".stabs \"STRING:t(0,%d)=*(0,%d)\""
                "," N_LSYM ",0,0,0\n", i, i+1);
    fprintf(stabs, ".stabs \"Parrot_String:T(0,%d)=s%d"
                "bufstart:(0,14),%d,%d;"
                "buflen:(0,6),%d,%d;"   /* XXX type */
                "flags:(0,12),%d,%d;"
                "bufused:(0,12),%d,%d;"
                "strstart:(0,15),%d,%d;"        /* fake a char* */
                ";\""
                "," N_LSYM ",0,0,0\n", i++, BYTE_SIZE(STRING),
                BIT_OFFSET(STRING, bufstart), BIT_SIZE(void*),
                BIT_OFFSET(STRING, buflen), BIT_SIZE(size_t),
                BIT_OFFSET(STRING, obj.flags), BIT_SIZE(UINTVAL),
                BIT_OFFSET(STRING, bufused), BIT_SIZE(UINTVAL),
                BIT_OFFSET(STRING, strstart), BIT_SIZE(void*)
                );

    fprintf(stabs, ".stabs \"PMCType:T(0,%d)=e", i++);
    for (j = 0; j < enum_class_max; ++j) {
        if (Parrot_base_vtables[j].name) {
            STRING* name = (*Parrot_base_vtables[j].name)(NULL, NULL);
            fwrite(name->strstart, name->strlen, 1, stabs);
            fprintf(stabs, ":%d,", j);
        }
    }
    fprintf(stabs, ";\"," N_LSYM ",0,0,0\n");

    /* PMC type */
    fprintf(stabs, ".stabs \"PMC:T(0,%d)=s%d", i, BYTE_SIZE(PMC));
    fprintf(stabs, "obj:(0,%d),%d,%d;",
            i + 1, BIT_OFFSET(PMC, obj), BIT_SIZE(pobj_t));
    fprintf(stabs, "vtable:*(0,%d),%d,%d;",
            i + 3, BIT_OFFSET(PMC, vtable), BIT_SIZE(void*));
    fprintf(stabs, "data:(0,14),%d,%d;",
            BIT_OFFSET(PMC, data), BIT_SIZE(void*));
    fprintf(stabs, "metadata:*(0,%d),%d,%d;",
            i, BIT_OFFSET(PMC, metadata), BIT_SIZE(void*));
    fprintf(stabs, ";\"");
    fprintf(stabs, "," N_LSYM ",0,0,0\n");

    fprintf(stabs, ".stabs \"pobj_t:T(0,%d)=s%d"
                "u:(0,%d),%d,%d;"
                "flags:(0,12),%d,%d;"
                ";\""
                "," N_LSYM ",0,0,0\n", i + 1, (int)(sizeof(pobj_t)),
                i + 2, BIT_OFFSET(pobj_t, u), BIT_SIZE(UnionVal),
                BIT_OFFSET(pobj_t, flags), BIT_SIZE(Parrot_UInt)
                );
    fprintf(stabs, ".stabs \"UnionVal:T(0,%d)=u%d"
                "int_val:(0,12),%d,%d;"
                "pmc_val:*(0,%d),%d,%d;"
                ";\""
                "," N_LSYM ",0,0,0\n", i + 2, BYTE_SIZE(UnionVal),
                BIT_OFFSET(UnionVal, int_val), BIT_SIZE(INTVAL),
                i, BIT_OFFSET(UnionVal, pmc_val), BIT_SIZE(void*)
                );
    fprintf(stabs, ".stabs \"VTABLE:T(0,%d)=s%d"
                "base_type:(0,%d),%d,%d;"
                ";\""
                "," N_LSYM ",0,0,0\n", i + 3, BYTE_SIZE(UnionVal),
                i - 1, BIT_OFFSET(VTABLE, base_type), BIT_SIZE(INTVAL)
                );
    i += 4;

}

static void
write_vars(FILE *stabs, struct Parrot_Interp *interpreter)
{
    int i;
    /* fake static var stabs */
    for (i = 0; i < NUM_REGISTERS; i++) {
        fprintf(stabs, ".stabs \"I%d:S(0,12)\"," N_STSYM ",0,0,%p\n", i,
                (char*)&interpreter->ctx.int_reg.registers[i]);
        fprintf(stabs, ".stabs \"N%d:S(0,13)\"," N_STSYM ",0,0,%p\n", i,
                (char*)&interpreter->ctx.num_reg.registers[i]);
        fprintf(stabs, ".stabs \"S%d:S(0,16)\"," N_STSYM ",0,0,%p\n", i,
                (char*)&interpreter->ctx.string_reg.registers[i]);
        fprintf(stabs, ".stabs \"P%d:S*(0,18)\"," N_STSYM ",0,0,%p\n", i,
                (char*)&interpreter->ctx.pmc_reg.registers[i]);
    }
}

static STRING *
debug_file(struct Parrot_Interp *interpreter, STRING *file, const char *ext)
{
    STRING *ret;
    ret = string_copy(interpreter, file);
    ret = string_append(interpreter, ret,
            string_make(interpreter, ext, strlen(ext), 0,
                PObj_external_FLAG, 0),
            0);
    return ret;
}

static void
Parrot_jit_debug_stabs(struct Parrot_Interp *interpreter)
{
    Parrot_jit_info_t *jit_info = interpreter->jit_info;
    STRING *file = interpreter->current_file;
    STRING *pasmfile, *stabsfile, *ofile, *cmd;
    FILE *stabs;
    size_t i;
    int line;
    opcode_t lc;

    if (interpreter->code->cur_cs->debug) {
        char *ext;
        char *src = interpreter->code->cur_cs->debug->filename;
        pasmfile = string_make(interpreter, src, strlen(src), NULL,
                PObj_external_FLAG, NULL);
        file = string_copy(interpreter, pasmfile);
        /* chop pasm/imc */

        ext = strrchr(src, '.');
        if (ext && strcmp (ext, ".pasm") == 0)
            file = string_chopn(file, 4);
        else if (ext && strcmp (ext, ".imc") == 0)
            file = string_chopn(file, 3);
        else if (!ext) /* EVAL_n */
            file = string_append(interpreter, file,
                    string_make(interpreter, ".", 1, 0, PObj_external_FLAG, 0),
                    0);
    }
    else {
        /* chop pbc */
        file = string_chopn(file, 3);
        pasmfile = debug_file(interpreter, file, "pasm");
    }
    stabsfile = debug_file(interpreter, file, "stabs.s");
    ofile = debug_file(interpreter, file, "o");
    stabs = fopen(string_to_cstring(interpreter,stabsfile), "w");
    if (stabs == NULL)
        return;

    /* filename info */
    fprintf(stabs, ".stabs \"%s\"," N_SO ",0,0,0\n",
            string_to_cstring(interpreter, pasmfile));
    /* jit_func start addr */
    fprintf(stabs, ".stabs \"jit_func:F(0,1)\"," N_FUN ",0,1,%p\n",
            jit_info->arena.start);

    write_types(stabs);
    write_vars(stabs, interpreter);
    /* if we don't have line numbers, emit dummys, assuming there are
     * no comments and spaces in source for testing
     */

    /* jit_begin */
    fprintf(stabs, ".stabn " N_SLINE ",0,1,0\n");
    line = 1;
    lc = 0;
    for (i = 0; i < jit_info->arena.map_size; i++) {
        if (jit_info->arena.op_map[i].ptr) {
            op_info_t* op = &interpreter->op_info_table[interpreter->code->byte_code[i]];
            if (interpreter->code->cur_cs->debug) {
                line = (int)interpreter->code->cur_cs->debug->base.data[lc++];
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
    cmd = Parrot_sprintf_c(interpreter, "as %Ss -o %Ss", stabsfile, ofile);
    system(string_to_cstring(interpreter, cmd));
}

void
Parrot_jit_debug(struct Parrot_Interp* interpreter)
{

    Parrot_jit_debug_stabs(interpreter);
}


#endif

/*
 * Local variables:
 * c-indentation-style: bsd
 * c-basic-offset: 4
 * indent-tabs-mode: nil
 * End:
 *
 * vim: expandtab shiftwidth=4:
*/
