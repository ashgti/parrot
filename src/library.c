/*
Copyright: 2004 The Perl Foundation.  All Rights Reserved.
$Id$

=head1 NAME

src/library.c - Interface to Parrot's bytecode library

=head1 DESCRIPTION

This file contains a C function to access parrot's bytecode library functions.

=head2 Functions

=over 4

=cut

*/

#include "parrot/parrot.h"
#include <assert.h>
#include "library.str"

#if 0
/*

=item C<static void
library_init(Parrot_Interp interpreter)>

internal helper function - loads the parrotlib bytecode

=cut

*/

static void
library_init(Parrot_Interp interpreter)
{
    /* XXX TODO: file location not known at runtime, should
       be linked with parrot (or use the upcoming config system) */
    Parrot_load_bytecode(interpreter, "runtime/parrot/include/parrotlib.pbc");
}

/*

=item C<void*
Parrot_library_query(Parrot_Interp, const char *func_name, ...)>

Runs the library function with the specified function name and returns
the result.

=cut

*/

void*
Parrot_library_query(Parrot_Interp interpreter, const char *func_name, ...)
{
    static int init_done = 0;
    va_list args;
    void *ret;
    PMC *sub, *prop;
    STRING *str, *name;
    char *csig;
    INTVAL resume = interpreter->resume_flag;

    if (!init_done) {
	library_init(interpreter);

        init_done = 1;
    }

    name = string_from_cstring(interpreter, func_name, strlen(func_name));

    /* get the sub pmc */
    str = CONST_STRING(interpreter, "_parrotlib");
    sub = Parrot_find_global(interpreter, str, name);
    if (!sub) {
        interpreter->resume_flag = resume;
	internal_exception(1, "unkown parrotlib method '%s'", func_name);
	abort();
    }

    /* get the signature */
    str = CONST_STRING(interpreter, "signature");
    prop = VTABLE_getprop(interpreter, sub, str);
    if (!prop) {
        interpreter->resume_flag = resume;
	internal_exception(1, "parrotlib method '%s' has no signature", func_name);
	abort();
    }
    str = VTABLE_get_string(interpreter, prop);
    csig = string_to_cstring(interpreter, str);

    /* call the bytecode method */
    va_start(args, func_name);
    ret = Parrot_runops_fromc_arglist_save(interpreter, sub, csig, args);
    va_end(args);

    string_cstring_free(csig);

    /* done */
    interpreter->resume_flag = resume;
    return ret;
}

STRING*
Parrot_library_fallback_locate(Parrot_Interp interp, const char *file_name, const char **incl)
{
    char *s;
    const char** ptr;
    int length = 0;
    int i, ok = 0;
    STRING *str = NULL; /* gcc uninit warn */

    /* calculate the length of the largest include directory */
    for( ptr = incl; *ptr != 0; ++ptr ) {
        i = strlen(*ptr);
        length = (i > length) ? i : length;
    }

    s = malloc(strlen(file_name) + length + 1);

    for( ptr = incl; (!ok) && (*ptr != 0); ++ptr ) {
        strcpy(s, *ptr);
        strcat(s, file_name);

#ifdef WIN32
        {
            char *p;
            while ( (p = strchr(s, '/')) )
                *p = '\\';
        }
#endif
	str = string_from_cstring(interp, s, strlen(s));
	ok = Parrot_stat_info_intval(interp, str, STAT_EXISTS);
    }
    if (!ok) {
	str = NULL;
    }
    free( s );
    return str;
}

#endif

/*

=item C<char* Parrot_locate_runtime_file(Interp *, const char *file_name,
        enum_runtime_ft type)>

Locate the full patch for C<file_name> and the given file type(s). If
successfull returns a mem_sys_allocate()ed string or NULL otherwise.

The C<enum_runtime_ft type> is one or more of the types defined in
F<include/parrot/library.h>.

*/

char* Parrot_locate_runtime_file(Interp *interpreter, const char *file_name,
        enum_runtime_ft type)
{
    char *full_name, *ext;
    const char **ptr;
    const char *prefix;
    STRING *str;
    const char *include_paths[] = {
        "runtime/parrot/include/",
        "runtime/parrot/",
        "./",
        NULL
    };
    const char *dynext_paths[] = {
        "runtime/parrot/dynext/",
        NULL
    };
    const char **paths;
    int length;

    if (type & PARROT_RUNTIME_FT_DYNEXT)
        paths = dynext_paths;
    else
        paths = include_paths;

    prefix = Parrot_get_runtime_prefix(interpreter, NULL);
    if (!prefix)
        prefix = "";

    ext = strchr(file_name, '.');
    /*
     * if the extension is given use it
     * TODO if not try extensions according to type
     */
    if (!ext) {
        internal_exception(UNIMPLEMENTED, "no extension: file '%s'", file_name);
    }
    length = 0;
    for (ptr = paths; *ptr; ++ptr) {
        int len = strlen(*ptr);
        length = (len > length) ? len : length;
    }
    length += strlen(prefix) + strlen(file_name) + 2;
    full_name = mem_sys_allocate(length);

    for (ptr = paths; *ptr; ++ptr) {
        strcpy(full_name, prefix);
        if (*prefix) {
#ifdef WIN32
            strcat(full_name, "\\");
#else
            strcat(full_name, "/");
#endif
        }
        strcat(full_name, *ptr);
        strcat(full_name, file_name);
#ifdef WIN32
        {
            char *p;
            while ( (p = strchr(full_name, '/')) )
                *p = '\\';
        }
#endif
        str = string_from_cstring(interpreter, full_name, strlen(full_name));
        if (Parrot_stat_info_intval(interpreter, str, STAT_EXISTS))
            return full_name;
    }
    /*
     * finally if prefix is set, try current location
     */
    if (*prefix) {
        strcpy(full_name, file_name);
#ifdef WIN32
        {
            char *p;
            while ( (p = strchr(full_name, '/')) )
                *p = '\\';
        }
#endif
        str = string_from_cstring(interpreter, full_name, strlen(full_name));
        if (Parrot_stat_info_intval(interpreter, str, STAT_EXISTS))
            return full_name;
    }
    return NULL;
}

/*

=back

=head1 SEE ALSO

F<include/parrot/library.h>

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
