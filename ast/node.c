#include <stdlib.h>
#include <stdio.h>

#include "ast.h"
#ifndef AST_TEST
#include "../imcc/imc.h"
#endif

static void
pr(nodeType *p)
{
    if (!p)
	return;
    printf("%s", p->u.r->name);
}

static void
const_dump(nodeType *p, int l)
{
    pr(p);
}

static nodeType*
const_expand(nodeType *p)
{
    return p;
}

static void
set_const(nodeType *p)
{
    p->expand = const_expand;
    /* p->context = const_context; */
    p->dump = const_dump;
    p->opt = NULL;
    p->d = "Const";
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
    if (!p->dump && p->next )
	dump(p->next, l);
}

void
dump_nodes(nodeType *p)
{
    dump(p, 0);
    printf("\n");
}

static nodeType *
new_node(YYLTYPE *loc)
{
    nodeType *p;
    if ((p = malloc(sizeof(nodeType))) == NULL) {
        fprintf(stderr, "out of memory");
	exit(1);
    }
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
var_expand(nodeType *p)
{
    return p;
}

static nodeType* create_0(int nr, nodeType *self, nodeType *p);
static nodeType* create_1(int nr, nodeType *self, nodeType *p);

typedef struct {
    const char *name;
    node_create_t create;
    node_expand_t expand;
    node_opt_t opt;
    node_dump_t   dump;
} node_names;

/* keep this list sorted */
static node_names ast_list[] = {
    { "--no-node", 	NULL, NULL, NULL, NULL },
    { "Const", 		NULL, NULL, NULL, const_dump },
#define CONST_NODE 1
    { "Py_Module", 	create_1, NULL, NULL, NULL },
    { "Py_Print" , 	create_1, NULL, NULL, NULL },
    { "Py_Print_nl",	create_0, NULL, NULL, NULL },
    { "Src_File",    	create_1, NULL, NULL, NULL },
    { "Src_Line",    	create_1, NULL, NULL, NULL },
    { "Stmts",          create_1, NULL, NULL, NULL }
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

int find_node_type(const char *name)
{
    node_names search, *r;

    search.name = name;
    r = bsearch(&search, ast_list, sizeof(ast_list) / sizeof(ast_list[0]),
		sizeof(ast_list[0]), ast_comp);

    if (!r) {
	fprintf(stderr, "Unknown astnode '%s'\n", name);
	exit(1);
    }
    return r - ast_list;
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
nodeType *
IMCC_new_const_node(char *name, int set, YYLTYPE *loc)
{
    nodeType *p = new_con(loc);
    SymReg *r = mk_const(name, set);
    p->u.r = r;
    return p;
}

nodeType *
IMCC_new_node(int nr, nodeType *child, YYLTYPE *loc)
{
    nodeType * n;
    if (nr == CONST_NODE) {
	return child;
    }
    n = new_node(loc);
    return ast_list[nr].create(nr, n, child);
}

nodeType *
IMCC_append_node(nodeType *a, nodeType *b, YYLTYPE *loc)
{
    nodeType *last = a;
    while (last->next)
	last = last->next;
    last->next = b;
    return a;
}

/*
 * Local variables:
 * c-indentation-style: bsd
 * c-basic-offset: 4
 * indent-tabs-mode: nil
 * End:
 *
 * vim: expandtab shiftwidth=4:
*/

