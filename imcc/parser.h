#ifndef __PARSER_H
#define __PARSER_H

#ifdef MAIN
#define EXTERN
#else
#define EXTERN extern
#endif

#include "imcparser.h"

EXTERN SymReg *regs[IMCC_MAX_REGS];
EXTERN int keyvec;
EXTERN int nargs;

EXTERN int expect_pasm;
EXTERN int pasm_file;
EXTERN int is_def;
EXTERN int IMCC_DEBUG;
EXTERN int gc_off;
EXTERN char* output;
EXTERN int yydebug;

#define KEY_BIT(argnum) (1 << (argnum))


SymReg * macro(struct Parrot_Interp *, char *name);
int yyerror(char *);
int yyparse(void *param);
int yylex(YYSTYPE *valp, struct Parrot_Interp *);
void op_fullname(char * dest, const char * name, SymReg * args[], int);
void open_comp_unit(void);
void register_compilers(Parrot_Interp);
void *imcc_eval_pasm(Parrot_Interp, const char *s);
void *imcc_eval_pir (Parrot_Interp, const char *s);

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
