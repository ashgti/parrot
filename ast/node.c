/*
Copyright: 2001-2004 The Perl Foundation.  All Rights Reserved.
$Id$

=head1 NAME

ast/node.c - AST node handling

=head1 DESCRIPTION

The AST (Abstract Syntax Tree) represents the code of a HLL source module.

=head2 Functions

=over 4

=cut

*/

#include <stdlib.h>
#include <stdio.h>

#include "ast.h"
#include "../imcc/imc.h"

static nodeType* create_0(int nr, nodeType *self, nodeType *p);
static nodeType* create_1(int nr, nodeType *self, nodeType *p);

static void
pr(nodeType *p)
{
    if (!p)
	return;
    printf("%s", p->u.r->name);
}

static void
dump_Const(nodeType *p, int l)
{
    pr(p);
}

static void
dump_Op(nodeType *p, int l)
{
    pr(NODE0(p));
    printf("'");
}

static void
dump_Var(nodeType *p, int l)
{
    pr(p);
}

static nodeType*
exp_Const(Interp* interpreter, nodeType *p)
{
    return p;
}

static void
set_const(nodeType *p)
{
    p->expand = exp_Const;
    /* p->context = const_context; */
    p->dump = dump_Const;
    p->opt = NULL;
    p->d = "Const";
}


static nodeType*
create_Op(int nr, nodeType *self, nodeType *p)
{
    char *s;
    s = p->u.r->name;
    p->u.r->set = 'o';  /* don't create const segmenet entries */
    s[strlen(s) - 1] = '\0';
    create_1(nr, self, p);
    return self;
}

static void
dump(nodeType *p, int l)
{
    nodeType *child;
    printf("\n%*s", l*2, "");
    printf("%s(", p->d);
    if (p->dump)
	p->dump(p, l);
    else {
	child = NODE0(p);
	if (child)
	    dump(child, l + 1);
    }
    printf(")");
    if (p->next )
	dump(p->next, l);
}

static nodeType *
new_node(YYLTYPE *loc)
{
    nodeType *p = mem_sys_allocate_zeroed(sizeof(nodeType));
    p->loc = *loc;
    return p;
}

static nodeType *
new_con(YYLTYPE *loc)
{
    nodeType *p = new_node(loc);
    set_const(p);
    return p;
}

static nodeType*
exp_Temp(Interp* interpreter, nodeType *p)
{
    return p;
}

static nodeType *
new_temp(YYLTYPE *loc)
{
    nodeType *p = new_node(loc);
    p->expand = exp_Temp;
    p->dump = dump_Var;
    p->d = "Temp";
    return p;
}

static Instruction *
insINS(Parrot_Interp interpreter, IMC_Unit * unit, Instruction *ins,
        const char *name, SymReg **regs, int n)
{
    Instruction *tmp;
    union {
        const void * __c_ptr;
        void * __ptr;
    } __ptr_u;
    char *s = const_cast(name);
    tmp = INS(interpreter, unit, s, NULL, regs, n, 0, 0);
    insert_ins(unit, ins, tmp);
    return tmp;
}

static Instruction *
insert_new(Interp* interpreter, nodeType *var, const char *name)
{
    Instruction *ins;
    SymReg *regs[IMCC_MAX_REGS], *r;
    int type = pmc_type(interpreter, const_string(interpreter, name));
    char ireg[8];

    cur_unit = interpreter->imc_info->last_unit;
    ins = cur_unit->last_ins;
    sprintf(ireg, "%d", type);
    r = mk_const(str_dup(ireg), 'I');

    regs[0] = var->u.r;
    regs[1] = r;
    return insINS(interpreter, cur_unit, ins, "new", regs, 2);
}

/*
 * node expand aka code creation functions
 */

static nodeType*
exp_Var(Interp* interpreter, nodeType *p)
{
    return p;
}

static nodeType*
exp_Name(Interp* interpreter, nodeType *p)
{
    return NODE0(p);
}

static nodeType*
exp_default(Interp* interpreter, nodeType *p)
{
    nodeType *child;
    nodeType *next = p->next;
    child = NODE0(p);
    if (child)
        child->expand(interpreter, child);
    if (next)
        next->expand(interpreter, next);
    return p;
}

static nodeType*
exp_next(Interp* interpreter, nodeType *p)
{
    nodeType *next = p->next;
    if (next)
        next->expand(interpreter, next);
    return p;
}

static nodeType*
exp_Assign(Interp* interpreter, nodeType *p)
{
    IMC_Unit *unit;
    Instruction *ins;
    SymReg *regs[IMCC_MAX_REGS], *r;
    nodeType *var = NODE0(p);
    nodeType *rhs = var->next;

    rhs = rhs->expand(interpreter, rhs);
    unit = interpreter->imc_info->last_unit;
    ins = unit->last_ins;
    regs[0] = NODE0(var)->u.r;
    regs[1] = rhs->u.r;
    insINS(interpreter, unit, ins, "assign", regs, 2);
    return exp_next(interpreter, p);
}

static nodeType*
exp_Binary(Interp* interpreter, nodeType *p)
{
    nodeType *op, *left, *right, *dest;
    IMC_Unit *unit;
    Instruction *ins;
    SymReg *regs[IMCC_MAX_REGS];

    op = NODE0(p);
    left = op->next;
    right = left->next;
    left = left->expand(interpreter, left);
    right = right->expand(interpreter, right);
    unit = interpreter->imc_info->last_unit;
    ins = unit->last_ins;
    if (!p->dest) {
        dest = IMCC_new_temp_node(interpreter, left->u.r->set, &p->loc);
        if (dest->u.r->set == 'P')
            ins = insert_new(interpreter, dest, "Undef");
    }
    p->dest = dest;
    regs[0] = dest->u.r;
    regs[1] = left->u.r;
    regs[2] = right->u.r;
    insINS(interpreter, unit, ins, NODE0(op)->u.r->name + 1, regs, 3);
    return dest;
}


static nodeType*
exp_Py_Local(Interp* interpreter, nodeType *p)
{
    nodeType *var = NODE0(p);

    insert_new(interpreter, var, "Undef");
    return exp_next(interpreter, p);
}

static nodeType*
exp_Py_Module(Interp* interpreter, nodeType *p)
{
    nodeType *doc;
    SymReg *sub;
    Instruction *i;
    /*
     * this is the main init code
     * (Py_doc, Py_local, Stmts)
     */
    cur_unit = interpreter->imc_info->last_unit;
    if (!cur_unit)
        cur_unit = imc_open_unit(interpreter, IMC_PCCSUB);
    sub = mk_sub_address(str_dup("__main__"));
    i = INS_LABEL(cur_unit, sub, 1);

    i->r[1] = mk_pcc_sub(str_dup(i->r[0]->name), 0);
    add_namespace(interpreter, i->r[1]);
    i->r[1]->pcc_sub->pragma = P_MAIN|P_PROTOTYPED ;
    doc = NODE0(p);      /* TODO */
    return exp_default(interpreter, doc);
}

static nodeType*
exp_Py_Print(Interp* interpreter, nodeType *p)
{
    IMC_Unit *unit;
    Instruction *ins;
    SymReg *regs[IMCC_MAX_REGS];
    nodeType * child = NODE0(p), *d;
    if (!child)
        fatal(1, "exp_Py_Print", "nothing to print");
    for (; child; child = child->next) {
        d = child->expand(interpreter, child);
        unit = interpreter->imc_info->last_unit;
        ins = unit->last_ins;
        /* TODO file handle node */
        if (d->expand == exp_Const || d->expand == exp_Var ||
                d->expand == exp_Temp)
            regs[0] = d->u.r;
        else
            fatal(1, "exp_Py_Print", "unknown node to print: '%s'", d->d);
        ins = insINS(interpreter, unit, ins, "print_item", regs, 1);
    }
    return exp_next(interpreter, p);
}

static nodeType*
exp_Py_Print_nl(Interp* interpreter, nodeType *p)
{
    IMC_Unit *unit = interpreter->imc_info->last_unit;
    Instruction *ins = unit->last_ins;
    SymReg *regs[IMCC_MAX_REGS];
    insINS(interpreter, unit, ins, "print_newline", regs, 0);
    return exp_next(interpreter, p);
}

static nodeType*
exp_Src_File(Interp* interpreter, nodeType *p)
{
    return exp_default(interpreter, p);
}

static nodeType*
exp_Src_Lines(Interp* interpreter, nodeType *p)
{
    return exp_default(interpreter, p);
}



typedef struct {
    const char *name;
    node_create_t create;
    node_expand_t expand;
    node_opt_t opt;
    node_dump_t   dump;
} node_names;

/*
 * keep this list sorted
 * TODO create an include file from this list with node numbers
 */

static node_names ast_list[] = {
    { "-no-node-", 	NULL, NULL, NULL, NULL },
    { "AssName", 	create_1, exp_Name, NULL, NULL },
    { "Assign", 	create_1, exp_Assign, NULL, NULL },
    { "Binary", 	create_1, exp_Binary, NULL, NULL },
    { "Const", 		NULL,     exp_Const, NULL, dump_Const },
    { "Name",           create_1, exp_Name, NULL, NULL },
    { "Op",             create_Op, NULL, NULL, dump_Op },
    { "Py_Local", 	create_1, exp_Py_Local, NULL, NULL },
    { "Py_Module", 	create_1, exp_Py_Module, NULL, NULL },
    { "Py_Print" , 	create_1, exp_Py_Print, NULL, NULL },
    { "Py_Print_nl",	create_0, exp_Py_Print_nl, NULL, NULL },
    { "Src_File",    	create_1, exp_Src_File, NULL, NULL },
    { "Src_Line",    	create_1, exp_Src_Lines, NULL, NULL },
    { "Stmts",          create_1, exp_default, NULL, NULL },
    { "_",              create_0, exp_next, NULL, NULL }
#define CONST_NODE 4
};

static int
ast_comp(const void *a, const void *b)
{
    const node_names *pa = (const node_names *) a;
    const node_names *pb = (const node_names *) b;
    return strcmp(pa->name, pb->name);
}

static void
print_node_name(int i) {
    printf("%s", ast_list[i].name);
}

static void
set_fptrs(nodeType *self, int nr)
{
    self->d = ast_list[nr].name;
    self->expand = ast_list[nr].expand;
    self->opt    = ast_list[nr].opt;
    self->dump   = ast_list[nr].dump;
}

static nodeType*
create_0(int nr, nodeType *self, nodeType *p)
{
    set_fptrs(self, nr);
    return self;
}

static nodeType*
create_1(int nr, nodeType *self, nodeType *p)
{
    p->up = self;
    NODE0(self) = p;
    set_fptrs(self, nr);
    return self;
}

/*
 * API
 */

/*

=item C<int IMCC_find_node_type(const char *name)>

Returns the index in C<ast_list> of the given node name. The node must exist.

=cut

*/

int
IMCC_find_node_type(const char *name)
{
    node_names search, *r;

    search.name = name;
    r = bsearch(&search, ast_list, sizeof(ast_list) / sizeof(ast_list[0]),
		sizeof(ast_list[0]), ast_comp);

    if (!r) {
	return 0;
    }
    return r - ast_list;
}

/*

=item C<nodeType *IMCC_new_const_node(Interp*, char *name, int set, YYLTYPE *loc)>

Create a new Const node for the given constant C<name> and Parrot register
set C<set>. Set may be:

  'I' ... Integer
  'N' ... Number
  'S' ... String
  'U' ... String treated as unicode string

=item C<nodeType *IMCC_new_temp_node(Interp*, int set, YYLTYPE *loc)>

Create a new temporary node.

=cut

*/

nodeType *
IMCC_new_const_node(Interp* interp, char *name, int set, YYLTYPE *loc)
{
    nodeType *p = new_con(loc);
    SymReg *r = mk_const(name, set);
    p->u.r = r;
    return p;
}

nodeType *
IMCC_new_var_node(Interp* interpreter, char *name, int set, YYLTYPE *loc)
{
    nodeType *p = new_node(loc);
    cur_unit = interpreter->imc_info->last_unit;
    if (!cur_unit)
        cur_unit = imc_open_unit(interpreter, IMC_PCCSUB);
    p->u.r = mk_ident(name, set);
    p->expand = exp_Var;
    p->dump   = dump_Var;
    return p;
}

nodeType *
IMCC_new_temp_node(Interp* interp, int set, YYLTYPE *loc)
{
    nodeType *p = new_temp(loc);
    SymReg *r;
    char buf[128];
    static int temp;
    sprintf(buf, "$%c%d", set, ++temp);
    r = mk_symreg(str_dup(buf), set);
    p->u.r = r;
    return p;
}

/*

=item C<nodeType *IMCC_new_node(Interp*, int nr, nodeType *child, YYLTYPE *loc)>

Create a new node with node type number C<nr> and the given child.

=cut

*/

nodeType *
IMCC_new_node(Interp* interp, int nr, nodeType *child, YYLTYPE *loc)
{
    nodeType * n;
    if (nr == CONST_NODE) {
	return child;
    }
    n = new_node(loc);
    return ast_list[nr].create(nr, n, child);
}

/*

=item C<nodeType *
IMCC_append_node(Interp *, nodeType *a, nodeType *b, YYLTYPE *loc)>

Append node C<b> add the end of C<a> and return C<a>.

=cut

*/


nodeType *
IMCC_append_node(Interp *interpreter, nodeType *a, nodeType *b, YYLTYPE *loc)
{
    nodeType *last = a;
    /*
     * TODO if we have long sequences of e.g. statement nodes
     *      a last pointer in the node structure could be ok
     */
    while (last->next)
	last = last->next;
    last->next = b;
    return a;
}

/*

=item C<IMCC_dump_nodes(nodeType *p)>

Dump nodes to C<stdout>

=item C<void IMCC_expand_nodes(Interp* interpreter, nodeType *p)>

Create code for the passed node. The code is appended to the current
unit's instructions.

=item C<void IMCC_free_nodes(Interp* interpreter, nodeType *p)>

Free the nodes.

=cut

*/

void
IMCC_dump_nodes(nodeType *p)
{
    dump(p, 0);
    printf("\n");
}

nodeType *
IMCC_expand_nodes(Interp* interpreter, nodeType *p)
{
    return p->expand(interpreter, p);
}

void
IMCC_free_nodes(Interp* interpreter, nodeType *p)
{
    nodeType *child, *next, *dest;
    if (p->expand == exp_Const ||
            p->expand == exp_Temp ||
            p->expand == exp_Var)
        ;
    else {
        child = NODE0(p);
        if (child)
            IMCC_free_nodes(interpreter, child);
    }
    dest = p->dest;
    if (dest && dest->expand == exp_Temp)
        mem_sys_free(dest);
    next = p->next;
    mem_sys_free(p);
    if (next)
        IMCC_free_nodes(interpreter, next);
}

/*

=back

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

