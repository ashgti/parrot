/*
Copyright: 2001-2004 The Perl Foundation.  All Rights Reserved.
$Id$

=head1 NAME

ast/ast_main.c - AST compiler

=head1 DESCRIPTION

The AST (Abstract Syntax Tree) represents the code of a HLL source module.

=head2 Functions

=over 4

=cut

*/

#include "../imcc/imc.h"
#include "parrot/method_util.h"
#include "ast.h"

extern FILE* ASTin;
extern void ASTparse(Interp *);

PMC * ast_compile_past(Interp *interpreter, const char *src_string);
PMC *
ast_compile_past(Interp *interpreter, const char *src_string)
{
    IMCC_fatal(interpreter, 1, "ast_compile_past: unimplemented");
    return NULL;
}

/*

=item C<IMCC_ast_compile(Interp *, FILE *fp)>

Compile AST source file to bytecode

=cut

*/


void
IMCC_ast_compile(Interp *interpreter, FILE *fp)
{
    nodeType *top_node;
    SymReg *sym;

    ASTin = fp;
    ASTparse(interpreter);

    top_node = interpreter->imc_info->top_node;
    if (top_node) {
        sym = IMCC_expand_nodes(interpreter, top_node);
        if (interpreter->imc_info->debug & DEBUG_AST) {
            IMCC_dump_nodes(interpreter, top_node);
        }
        IMCC_free_nodes(interpreter, top_node);
    }
    interpreter->imc_info->top_node = NULL;
}

static void
register_ast_compiler(Interp* interpreter)
{
    STRING *past = const_string(interpreter, "PAST");
    Parrot_compreg(interpreter, past, ast_compile_past);
}
/*

=item C<IMCC_ast_init(Interp*)>

Initialize the AST compiler

=cut

*/

void
IMCC_ast_init(Interp* interpreter)
{
    register_ast_compiler(interpreter);
}

/*

=back

=cut

*/

/*
 * Local variables:
 * c-indentation-style: bsd
 * c-basic-offset: 4
 * indent-tabs-mode: nil
 * End:
 *
 * vim: expandtab shiftwidth=4:
*/
