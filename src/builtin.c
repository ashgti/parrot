/*
Copyright (C) 2003-2007, The Perl Foundation.
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
#include "parrot/compiler.h"
#include "builtin.str"


typedef struct _builtin {
    ARGIN(const char * const c_name);       /* short name 'cos' */
    ARGIN(const char * const signature);    /* e.g. PP */
    ARGIN(const char * const c_ns);         /* namespace */
    ARGIN_NULLOK(STRING *meth_name);        /* internal name e.g. '__cos' */
    ARGIN_NULLOK(STRING *_namespace);       /* same */
} Builtins;

#define N_BUILTINS (int)(sizeof (builtins) / sizeof (builtins[0]))
static Builtins builtins[] = {
    /*
     * Please keep this list sorted
     */
    { "acos",   "PJO",          "Float",        NULL, NULL },
    { "asec",   "PJO",          "Float",        NULL, NULL },
    { "asin",   "PJO",          "Float",        NULL, NULL },
    { "atan2",  "PJOP",         "Float",        NULL, NULL },
    { "atan",   "PJO",          "Float",        NULL, NULL },
    { "chdir",  "vJOS",         "OS",           NULL, NULL },
    { "chroot", "vJOS",         "OS",           NULL, NULL },
    { "copy",   "vJOSS",        "File",         NULL, NULL },
    { "cosh",   "PJO",          "Float",        NULL, NULL },
    { "cos",    "PJO",          "Float",        NULL, NULL },
    { "cwd",    "SJO",          "OS",           NULL, NULL },
    { "exp",    "PJO",          "Float",        NULL, NULL },
    { "index",  "IJSS.I",       "String",       NULL, NULL },
    { "is_integer", "IJS",      "String",       NULL, NULL },
    { "link",   "vJOSS",        "OS",           NULL, NULL },
    { "ln",     "PJO",          "Float",        NULL, NULL },
    { "log10",  "PJO",          "Float",        NULL, NULL },
    { "log2",   "PJO",          "Float",        NULL, NULL },
    { "lower",  "PJO",          "String",       NULL, NULL },
    { "lstat",  "PJOS",         "OS",           NULL, NULL },
    { "mkdir",  "vJOSI",        "OS",           NULL, NULL },
    { "open",   "PJS.S",        "ParrotIO",     NULL, NULL },
    { "puts",   "IJOS",         "ParrotIO",     NULL, NULL },
    { "rename", "vJOSS",        "File",         NULL, NULL },
    { "reverse", "vJS",         "String",       NULL, NULL },
    { "rm",     "vJOS",         "OS",           NULL, NULL },
    { "say",    "IJOS",         "ParrotIO",     NULL, NULL },
    { "say",    "IJS",          "ParrotIO",     NULL, NULL },
    { "say",    "vJOS",         "ParrotIO",     NULL, NULL },
    { "say",    "vJS",          "ParrotIO",     NULL, NULL },
    { "say",    "vJI",          "ParrotIO",     NULL, NULL },
    { "say",    "vJN",          "ParrotIO",     NULL, NULL },
    { "sech",   "PJO",          "Float",        NULL, NULL },
    { "sec",    "PJO",          "Float",        NULL, NULL },
    { "sinh",   "PJO",          "Float",        NULL, NULL },
    { "sin",    "PJO",          "Float",        NULL, NULL },
    { "sqrt",   "PJO",          "Float",        NULL, NULL },
    { "symlink", "vJOSS",       "OS",           NULL, NULL },
    { "tanh",   "PJO",          "Float",        NULL, NULL },
    { "tan",    "PJO",          "Float",        NULL, NULL },
    { "trans",  "vJSP",         "String",       NULL, NULL },
    { "umask",  "IJOI",         "OS",           NULL, NULL },
    /*
     * Please keep this list sorted
     */
};

/* HEADERIZER HFILE: include/parrot/builtin.h */

/* HEADERIZER BEGIN: static */

PARROT_WARN_UNUSED_RESULT
static int check_builtin_sig(
    size_t i,
    ARGIN(const char *sig),
    int convert_pmcs)
        __attribute__nonnull__(2);

PARROT_WARN_UNUSED_RESULT
static int find_builtin(ARGIN(const char *func))
        __attribute__nonnull__(1);

PARROT_WARN_UNUSED_RESULT
static int find_builtin_s(PARROT_INTERP, ARGIN(const STRING *func))
        __attribute__nonnull__(1)
        __attribute__nonnull__(2);

/* HEADERIZER END: static */

/*

=item C<void Parrot_init_builtins>

Initialize the builtins structure.

=cut

*/

void
Parrot_init_builtins(PARROT_INTERP)
{
    size_t i;

    for (i = 0; i < N_BUILTINS; ++i) {
        builtins[i].meth_name  = const_string(interp, builtins[i].c_name);
        builtins[i]._namespace = const_string(interp, builtins[i].c_ns);
    }
}

/*

=item C<static int find_builtin>

RT#48260: Not yet documented!!!

=cut

*/

PARROT_WARN_UNUSED_RESULT
static int
find_builtin(ARGIN(const char *func))
{
    int low  = 0;
    int high = N_BUILTINS - 1;

    /* binary search */
    while (low <= high) {
        int i   = (low + high) / 2;
        const int cmp = strcmp(func, builtins[i].c_name);

        if (!cmp) {
            /* we have to loop here because there is currently more than one
               entry for the 'say' opcode and we depend on having the first
               one so we can check signatures. --mdiep */
            while (i>0 && strcmp(func, builtins[i-1].c_name) == 0)
                i--;
            return i;
        }
        else if (cmp > 0)
            low  = i + 1;
        else if (cmp < 0)
            high = i - 1;
    }
    return -1;
}

/*

=item C<static int find_builtin_s>

RT#48260: Not yet documented!!!

=cut

*/

PARROT_WARN_UNUSED_RESULT
static int
find_builtin_s(PARROT_INTERP, ARGIN(const STRING *func))
{
    int low  = 0;
    int high = N_BUILTINS - 1;

    /* binary search */ /* XXX isn't there a standard C func for this? */
    while (low <= high) {
        const int i   = (low + high) / 2;
        const int cmp = string_compare(interp, func, builtins[i].meth_name);

        if (!cmp)
            return i;
        else if (cmp > 0)
            low  = i + 1;
        else if (cmp < 0)
            high = i - 1;
    }
    return -1;
}

/*

=item C<static int check_builtin_sig>

RT#48260: Not yet documented!!!

=cut

*/

PARROT_WARN_UNUSED_RESULT
static int
check_builtin_sig(size_t i, ARGIN(const char *sig), int convert_pmcs)
{
    const Builtins * const b = builtins + i;
    const char *p;
    int opt = 0;

    p = b->signature;

    /* opcodes don't have a void type,
     * so skip to the next type if the builtin is void */
    if (*p != *sig && *p == 'v')
        ++p;

    /*
     * Check that each type in the signatures match.
     */
    for (; *p && *sig; ++sig, ++p) {
        switch (*p) {
            case '.':   /* start of :optional parameters */
                opt = 1;
                /* fall through */
            case 'J':   /* interpreter */
                ++p;
            default:
                break;
        }
        if (*p == 'O' && *sig == 'P')
            continue;
        if (convert_pmcs && (*p == 'P' || *sig == 'P'))
            continue;
        if (*p != *sig)
            return 0;
    }

    /* if we've reached the end of both signatures */
    if (!*p && !*sig)
        return 1;

    /* if we're at the end of the passed in signature and the rest of the
     * builtin's signature is :optional args */
    if (*p && opt)
        return 1;

    return 0;
}

/*

=item C<int Parrot_is_builtin>

Return the index of the builtin or -1 on failure.

=cut

*/

PARROT_WARN_UNUSED_RESULT
int
Parrot_is_builtin(ARGIN(const char *func), ARGIN_NULLOK(const char *sig))
{
    int bi, i, convert_pmcs;

    i = find_builtin(func);
    if (i < 0)
        return -1;

    /* if the sig is NULL, we don't need to check it -- just return */
    if (!sig)
        return i;

    /* First, try to find an exact match. If we can't find one, see if we can
     * find a match by allowing conversions to/from PMCs. */
    bi = i;
    for (convert_pmcs = 0; convert_pmcs <= 1; ++convert_pmcs) {
        i = bi;
again:
        if (check_builtin_sig(i, sig, convert_pmcs))
            return i;
        if (i < N_BUILTINS - 1) {
            /* try next with same name */
            ++i;
            /* if the name of the next builtin matches, check its signature */
            if (strcmp(func, builtins[i].c_name) == 0)
                goto again;
        }
    }

    return -1;
}

/*

=item C<PMC* Parrot_find_builtin>

Return the NCI PMC of the builtin or NULL.

=cut

*/

PARROT_WARN_UNUSED_RESULT
PARROT_CAN_RETURN_NULL
PMC*
Parrot_find_builtin(PARROT_INTERP, NOTNULL(STRING *func))
{
    const int i = find_builtin_s(interp, func);

    /* XXX What's going on here? --mdiep, 31 July 07 */
    if (i < 0)
        return Parrot_find_global_s(interp,
                                    CONST_STRING(interp, "__parrot_core"),
                                    func);
    else
        return Parrot_find_global_s(interp,
                                    builtins[i]._namespace,
                                    builtins[i].meth_name);
}

/*

=item C<const char * Parrot_builtin_get_c_namespace>

RT#48260: Not yet documented!!!

=cut

*/

PARROT_CONST_FUNCTION
PARROT_WARN_UNUSED_RESULT
PARROT_CANNOT_RETURN_NULL
const char *
Parrot_builtin_get_c_namespace(int bi)
{
    PARROT_ASSERT(bi >= 0 && bi < N_BUILTINS);
    return builtins[bi].c_ns;
}

/*

=item C<int Parrot_builtin_is_class_method>

RT#48260: Not yet documented!!!

=cut

*/

PARROT_CONST_FUNCTION
PARROT_WARN_UNUSED_RESULT
int
Parrot_builtin_is_class_method(int bi)
{
    PARROT_ASSERT(bi >= 0 && bi < N_BUILTINS);
    return builtins[bi].signature[2] != 'O';
}

/*

=item C<int Parrot_builtin_is_void>

RT#48260: Not yet documented!!!

=cut

*/

PARROT_CONST_FUNCTION
PARROT_WARN_UNUSED_RESULT
int
Parrot_builtin_is_void(int bi)
{
    PARROT_ASSERT(bi >= 0 && bi < N_BUILTINS);
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
 *   c-file-style: "parrot"
 * End:
 * vim: expandtab shiftwidth=4:
 */
