/*
 * $Id$
 * Copyright (C) 2008, Parrot Foundation.
 */

#ifndef PARROT_PIR_PIREMIT_H_GUARD
#define PARROT_PIR_PIREMIT_H_GUARD

/* forward declaration */
struct lexer_state;
struct constant;

void print_subs(struct lexer_state * const lexer);
void emit_pir_subs(struct lexer_state * const lexer, char const * const outfile);
void emit_pbc(struct lexer_state * const lexer, const char *outfile);


int emit_pbc_const(struct lexer_state * const lexer, struct constant * const pirconst);

#endif /* PARROT_PIR_PIREMIT_H_GUARD */

/*
 * Local variables:
 *   c-file-style: "parrot"
 * End:
 * vim: expandtab shiftwidth=4:
 */
