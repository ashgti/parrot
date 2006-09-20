/* $Id$ */

/*
 * example compiler used by japh16.pasm
 *
 * $ make -C examples/compilers/
 * $ parrot examples/japh/japh16.pasm
 *
 */

#include "parrot/parrot.h"
#include "parrot/embed.h"

#define C_DEBUG 0

#if C_DEBUG
#include <stdio.h>
#  define cdebug(x) fprintf x
#else
#  define cdebug(x)
#endif

PMC* japh_compiler(Parrot_Interp interpreter, const char *s);

/*
 * loadlib calls the load and init hooks
 * we use init to register the compiler
 */

void
Parrot_lib_japhc_init(Parrot_Interp interpreter, PMC* lib)
{
    STRING *cmp;

    cdebug((stderr, "japhc_init\n"));
    cmp = const_string(interpreter, "JaPH_Compiler");
    Parrot_compreg(interpreter, cmp, japh_compiler);
}


static int
unescape(char *string)
{
    char *start, *p;

    for (start = p = string ; *string; string++) {
        if (*string == '\\' && string[1]) {
            switch (*++string) {
                case 'n':
                    *p++ = '\n';
                    break;
                default:
                    *p++ = *string;
                    break;
            }
        }
        else
            *p++ = *string;
    }
    *p = 0;
    return p - start;
}

/* add constant string to constant_table */
static int
add_const_str(Parrot_Interp interpreter,
	struct PackFile_ConstTable *consts, char *str)
{
    int k, l;
    char *o;
    char *buf = o = strdup(str);

    /*
     * TODO strip delimiters in lexer, this needs adjustment in printint strings
     */
    if (*buf == '"') {
	buf++;
	l = unescape(buf);
	if (l)
	    buf[--l] = '\0';
    }
    else if (*buf == '\'') {
	buf++;
	l = strlen(buf);
	if (l)
	    buf[--l] = '\0';
    }
    else {
	l = unescape(buf);
    }

    /* Update the constant count and reallocate */
    k = ++consts->const_count;
    if (consts->constants == NULL)
	consts->constants = mem_sys_allocate(
		k * sizeof(struct PackFile_Constant *));
    else
	consts->constants = mem_sys_realloc(consts->constants,
		k * sizeof(struct PackFile_Constant *));

    /* Allocate a new constant */
    consts->constants[--k] = PackFile_Constant_new(interpreter);
    consts->constants[k]->type = PFC_STRING;
    consts->constants[k]->u.string =
	string_make(interpreter, buf, (UINTVAL) l, "iso-8859-1", 0 );
    free(o);
    return k;
}

/*
 * simple compiler - no error checking
 */
PMC*
japh_compiler(Parrot_Interp interpreter, const char *program)
{
    struct PackFile_ByteCode *cur_cs, *old_cs;
    struct PackFile_ConstTable *consts;
    opcode_t* pc;
    const char *p;
    PMC *sub;
    parrot_sub_t sub_data;

#define CODE_SIZE 128
    cdebug((stderr, "japh_compiler '%s'\n", program));

    /*
     * need some packfile segments
     */
    cur_cs = PF_create_default_segs(interpreter, "JAPHc", 1);
    old_cs = Parrot_switch_to_cs(interpreter, cur_cs, 0);
    /*
     * alloc byte code mem
     */
    cur_cs->base.data = mem_sys_allocate(CODE_SIZE * sizeof(opcode_t));
    cur_cs->base.size = CODE_SIZE;
    consts = cur_cs->const_table;
    /*
     * now start compiling
     */
    pc = cur_cs->base.data;
    for (p = program; *p; ++p) {
	switch (*p) {
	    case 'p':	/* print_sc */
		*pc++ = interpreter->op_lib->op_code("print_sc", 1);
		/* const follows */
		++p;
		switch (*p) {
		    case 'J':
			*pc++ = add_const_str(interpreter, consts, "Just ");
			break;
		    case 'a':
			*pc++ = add_const_str(interpreter, consts, "another ");
			break;
		    case 'P':
			*pc++ = add_const_str(interpreter, consts, "Parrot ");
			break;
		    case 'H':
			*pc++ = add_const_str(interpreter, consts, "Hacker");
			break;
		    case 'n':
			*pc++ = add_const_str(interpreter, consts, "\n");
			break;
		}
		break;
	    case 'e':	/* end */
		*pc++ = interpreter->op_lib->op_code("invoke_p", 1);
		*pc++ = 1;
		break;
	}
    }
    if (old_cs) {
        /* restore old byte_code, */
        (void)Parrot_switch_to_cs(interpreter, old_cs, 0);
    }
    /*
     * create sub PMC
     */
    sub = pmc_new(interpreter, enum_class_Eval);
    sub_data = PMC_sub(sub);
    sub_data->seg = cur_cs;
    sub_data->address = cur_cs->base.data;
    sub_data->end = cur_cs->base.data + cur_cs->base.size;
    sub_data->name = string_from_cstring(interpreter, "JaPHC", 0);
    return sub;
}

/*
 * Local variables:
 *   c-file-style: "parrot"
 * End:
 * vim: expandtab shiftwidth=4:
 */
