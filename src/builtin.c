/*
Copyright: 2003 The Perl Foundation.  All Rights Reserved.
$Id$

=head1 NAME

src/builtin.c - Builtin Methods

=head1 SYNOPSIS

=head1 DESCRIPTION

=head2 Functions

=over 4

=cut

*/

#include "parrot/parrot.h"
#include "builtin.str"
#include <assert.h>


typedef struct _builtin {
    const char *c_name;		/* short name 'cos' */
    const char *signature;	/* e.g. PP */
    const char *c_ns;		/* namespace */
    STRING *meth_name;	/* internal name e.g. '__cos' */
    STRING *namespace;	/* same */
} Builtins;

static Builtins builtins[] = {
    { "acos", 	"PJO", 		"Float", 	0, 0 },
    { "asec", 	"PJO", 		"Float", 	0, 0 },
    { "asin", 	"PJO", 		"Float", 	0, 0 },
    { "atan", 	"PJO", 		"Float", 	0, 0 },
    { "atan2", 	"PJOP",		"Float", 	0, 0 },
    { "cos", 	"PJO", 		"Float", 	0, 0 },
    { "cosh", 	"PJO", 		"Float", 	0, 0 },
    { "cd",     "vJOS",         "OS",           0, 0 },
    { "cwd",    "SJO",          "OS",           0, 0 },
    { "exp", 	"PJO", 		"Float", 	0, 0 },
    { "mkdir",  "vJOSI",        "OS",           0, 0 },
    { "ln", 	"PJO", 		"Float", 	0, 0 },
    { "log10",  "PJO",          "Float", 	0, 0 },
    { "log2", 	"PJO", 		"Float", 	0, 0 },
    { "sec", 	"PJO", 		"Float", 	0, 0 },
    { "sech", 	"PJO", 		"Float", 	0, 0 },
    { "sin", 	"PJO", 		"Float", 	0, 0 },
    { "sinh", 	"PJO", 		"Float", 	0, 0 },
    { "tan", 	"PJO", 		"Float", 	0, 0 },
    { "tanh", 	"PJO", 		"Float", 	0, 0 },
    { "index",  "IJSS.I",       "String",       0, 0 },
    { "is_integer","IJS",       "String",       0, 0 },
    { "link",   "vJOSS",        "OS",           0, 0 },
    { "lower", 	"PJO",	        "String", 	0, 0 },
    { "open", 	"PJS.S",	"ParrotIO", 	0, 0 },
    { "puts", 	"IJOS",         "ParrotIO", 	0, 0 },
    { "reverse","vJS",          "String",       0, 0 },
    { "rm",     "vJOS",         "OS",           0, 0 },
    { "say", 	"IJS",          "ParrotIO", 	0, 0 },
    { "say", 	"IJOS",         "ParrotIO", 	0, 0 },
    { "say", 	"vJS",          "ParrotIO", 	0, 0 },
    { "say", 	"vJOS",         "ParrotIO", 	0, 0 },
    { "symlink","vJOSS",        "OS",           0, 0 },
    { "trans",  "vJSP",         "String",       0, 0 },
    { "umask",  "IJOI",         "OS",           0, 0 },
};

/*

=item C<void Parrot_init_builtins(Interp *)>

Initialize the builtins structure.

=item C<int Parrot_is_builtin(Interp *, char *func, char *sig)>

Return the index of the builtin or -1 on failure.

=item C<PMC* Parrot_find_builtin(Interp *, STRING *func)>

Return the NCI PMC of the builtin or NULL.

=cut

*/

void
Parrot_init_builtins(Interp *interpreter)
{
    size_t i, n;
    char buffer[128];

    n = sizeof(builtins) / sizeof(builtins[0]);
    buffer[0] = buffer[1] = '_';
    for (i = 0; i < n; ++i) {
        /* XXX mangle yes or no */
#ifdef MANGLE_BUILTINS
	strcpy(buffer + 2, builtins[i].c_name);
	builtins[i].meth_name = const_string(interpreter, buffer);
#else
	builtins[i].meth_name = const_string(interpreter,
                builtins[i].c_name);
#endif
        builtins[i].namespace = const_string(interpreter,
                builtins[i].c_ns);
    }
    /*
     * TODO create a hash
     */
}

static int
find_builtin(Interp *interpreter, char *func)
{
    size_t i, n;

    n = sizeof(builtins) / sizeof(builtins[0]);
    /* TODO either hash or use binsearch */
    for (i = 0; i < n; ++i) {
        if (strcmp(func, builtins[i].c_name) == 0)
            return i;
    }
    return -1;
}

static int
find_builtin_s(Interp *interpreter, STRING *func)
{
    size_t i, n;

    n = sizeof(builtins) / sizeof(builtins[0]);
    /* TODO either hash or use binsearch */
    for (i = 0; i < n; ++i) {
        if (string_equal(interpreter, func, builtins[i].meth_name) == 0)
            return i;
    }
    return -1;
}

static int
check_builtin_sig(Interp *interpreter, size_t i, char *sig, int pass)
{
    Builtins *b = builtins + i;
    const char *p;
    int opt = 0;

    p = b->signature;
    if (*p != *sig && *p == 'v')
        ++p;
    for (; *p && *sig; ++sig, ++p) {
        switch (*p) {
            case '.':   /* optional start */
                opt = 1;
                /* fall through */
            case 'J':   /* interpreter */
                ++p;
        }
        if (*p == 'O' && *sig == 'P')
            continue;
        if (pass && (*p == 'P' || *sig == 'P'))
            continue;
        if (*p != *sig)
            return 0;
    }
    if (!*p && !*sig)
        return 1;
    if (*p && opt)
        return 1;
    return 0;
}

int
Parrot_is_builtin(Interp *interpreter, char *func, char *sig)
{
    int bi, i, n, pass;

    i = find_builtin(interpreter, func);
    if (i < 0)
        return -1;
    if (!sig)
        return i;
    n = sizeof(builtins) / sizeof(builtins[0]);
    bi = i;
    for (pass = 0; pass <= 1; ++pass) {
        i = bi;
again:
        if (check_builtin_sig(interpreter, i, sig, pass)) 
            return i;
        if (i < n - 1) {
            /* try next with same name */
            ++i;
            if (strcmp(func, builtins[i].c_name))
                continue;
            goto again;
        }
    }
    return -1;
}

PMC*
Parrot_find_builtin(Interp *interpreter, STRING *func)
{
    int i;
    PMC *m;
    STRING *ns;

    i = find_builtin_s(interpreter, func);
    if (i < 0) {
        ns = CONST_STRING(interpreter, "__parrot_core");
        return Parrot_find_global(interpreter, ns, func);
    }
    m = Parrot_find_global(interpreter,
            builtins[i].namespace,
            builtins[i].meth_name);
    return m;
}

const char *
Parrot_builtin_get_c_namespace(Interp *interpreter, int bi)
{
    int n;

    n = sizeof(builtins) / sizeof(builtins[0]);
    assert(bi >= 0 && bi < n);
    return builtins[bi].c_ns;
}

int
Parrot_builtin_is_class_method(Interp *interpreter, int bi)
{
    int n;

    n = sizeof(builtins) / sizeof(builtins[0]);
    assert(bi >= 0 && bi < n);
    return builtins[bi].signature[2] != 'O';
}

int
Parrot_builtin_is_void(Interp *interpreter, int bi)
{
    int n;

    n = sizeof(builtins) / sizeof(builtins[0]);
    assert(bi >= 0 && bi < n);
    return builtins[bi].signature[0] == 'v';
}

/*

=back

=head1 SEE ALSO

F<ops/math.ops>

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
