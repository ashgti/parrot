/* dynext.c
 *  Copyright: 2001-2003 The Perl Foundation.  All Rights Reserved.
 *  CVS Info
 *     $Id$
 *  Overview:
 *     Dynamic extension stuff
 *  Data Structure and Algorithms:
 *  History:
 *     Initial rev by leo 2003.08.06
 *  Notes:
 *  References:
 */

#include "parrot/parrot.h"
#include "parrot/dynext.h"

/*
 * set a property
 */
static void
set_cstring_prop(Parrot_Interp interpreter, PMC *lib_pmc, const char *what,
        STRING *name)
{
    PMC *prop;
    STRING *key;

    prop = pmc_new(interpreter, enum_class_PerlString);
    VTABLE_set_string_native(interpreter, prop, name);
    key = string_from_cstring(interpreter, what, 0);
    VTABLE_setprop(interpreter, lib_pmc, key, prop);
}

/*
 * store a library PMC in interpreter iglobals
 */
static void
store_lib_pmc(Parrot_Interp interpreter, PMC* lib_pmc, STRING *path,
        STRING *type)
{
    PMC *iglobals, *dyn_libs;

    iglobals = interpreter->iglobals;
    dyn_libs = VTABLE_get_pmc_keyed_int(interpreter, iglobals,
            IGLOBALS_DYN_LIBS);
    if (!dyn_libs) {
        dyn_libs = pmc_new(interpreter, enum_class_PerlArray);
        VTABLE_set_pmc_keyed_int(interpreter, iglobals,
                IGLOBALS_DYN_LIBS, dyn_libs);
    }
    /*
     * remember path/file in props
     */
    set_cstring_prop(interpreter, lib_pmc, "_filename", path);
    set_cstring_prop(interpreter, lib_pmc, "_type", type);

    VTABLE_push_pmc(interpreter, dyn_libs, lib_pmc);
}

/*
 * check if a library PMC with the filename path exists
 * if yes return it
 *
 */
static PMC*
is_loaded(Parrot_Interp interpreter, STRING *path)
{
    PMC *iglobals, *dyn_libs, *prop, *lib_pmc;
    STRING *key;
    INTVAL i, n;

    iglobals = interpreter->iglobals;
    dyn_libs = VTABLE_get_pmc_keyed_int(interpreter, iglobals,
            IGLOBALS_DYN_LIBS);
    if (!dyn_libs)
        return NULL;
    n = VTABLE_elements(interpreter, dyn_libs);
    key = string_from_cstring(interpreter, "_filename", 0);
    /* we could use an ordered hash for faster lookup here */
    for (i = 0; i < n; i++) {
        lib_pmc = VTABLE_get_pmc_keyed_int(interpreter, dyn_libs, i);
        prop = VTABLE_getprop(interpreter, lib_pmc, key);
        if (!string_compare(interpreter,
                    VTABLE_get_string(interpreter, prop), path))
            return lib_pmc;
    }
    return NULL;
}

/*
 * return path and handle of a dynamic lib
 */
static STRING *
get_path(Interp *interpreter, STRING *lib, void **handle)
{
    char *cpath;
    STRING *path;
    /* TODO runtime path for dynamic extensions */
    /* TODO $SO extension */
#ifndef RUNTIME_DYNEXT
#  define RUNTIME_DYNEXT "runtime/parrot/dynext/"
#endif

    /*
     * Current dir with no extension
     */
    path = string_copy(interpreter, lib);
    cpath = string_to_cstring(interpreter, lib);
    *handle = Parrot_dlopen(cpath);
    /*
     * first look in current dir
     */
    if (!*handle) {
        string_cstring_free(cpath);
        path = Parrot_sprintf_c(interpreter, "%Ss%s",
                                lib,
                                PARROT_DLL_EXTENSION);
        cpath = string_to_cstring(interpreter, path);
        *handle = Parrot_dlopen(cpath);
    }
    if (!*handle) {
        /*
         * then in runtime/ ...
         */
        /* TODO only if not an absolute path */
        string_cstring_free(cpath);
        path = Parrot_sprintf_c(interpreter, "%s%Ss%s",
                RUNTIME_DYNEXT,
                lib,
                PARROT_DLL_EXTENSION);
        cpath = string_to_cstring(interpreter, path);
        *handle = Parrot_dlopen(cpath);
    }
    if (!*handle) {
        /*
         * then in runtime/ with no extension
         */
        /* TODO only if not an absolute path */
        string_cstring_free(cpath);
        path = Parrot_sprintf_c(interpreter, "%s%Ss",
                                RUNTIME_DYNEXT,
                                lib);
        cpath = string_to_cstring(interpreter, path);
        *handle = Parrot_dlopen(cpath);
    }
    if (!*handle) {
        const char * err = Parrot_dlerror();
        fprintf(stderr, "Couldn't load '%s': %s\n",
                cpath, err ? err : "unknow reason");
        string_cstring_free(cpath);
        return NULL;
    }
    string_cstring_free(cpath);
    return path;
}

/*
 * dynamic library loader
 * the initializer is currently unused
 *
 * calls Parrot_lib_load_%s which performs the registration of the lib once
 *       Parrot_lib_init_%s gets called (if exists) to perform
 *                          thread specific setup
 */

PMC *
Parrot_load_lib(Interp *interpreter, STRING *lib, PMC *initializer)
{
    STRING *path, *load_func_name, *init_func_name, *type;
    void * handle;
    PMC *(*load_func)(Interp *);
    void (*init_func)(Interp *, PMC *);
    char *cinit_func_name, *cload_func_name;
    PMC *lib_pmc;

    UNUSED(initializer);
    path = get_path(interpreter, lib, &handle);
    if (!path) {
        /*
         * XXX internal_exception? return a PerlUndef?
         */
        return PMCNULL;
    }
    /*
     * TODO move the class_count_mutex here
     *
     * LOCK()
     */
    lib_pmc = is_loaded(interpreter, path);
    if (lib_pmc) {
        Parrot_dlclose(handle);
        /* UNLOCK */
        return lib_pmc;
    }
    load_func_name = Parrot_sprintf_c(interpreter, "Parrot_lib_%Ss_load", lib);
    cload_func_name = string_to_cstring(interpreter, load_func_name);
    load_func = (PMC * (*)(Interp *))D2FPTR(Parrot_dlsym(handle,
                cload_func_name));
    string_cstring_free(cload_func_name);
    if (!load_func) {
        /* seems to be a native/NCI lib */
        /*
         * this PMC should better be constant, but then all the contents
         * and the metadata have to be constant too
         * s. also build_tools/ops2c.pl and lib/Parrot/Pmc2c.pm
         */
        lib_pmc = pmc_new(interpreter, enum_class_ParrotLibrary);
        type = string_from_cstring(interpreter, "NCI", 0);
    }
    else {
        lib_pmc = (*load_func)(interpreter);
        /* we could set a private flag in the PMC header too
         * but currently only ops files have struct_val set
         */
        type = string_from_cstring(interpreter,
                lib_pmc->cache.struct_val ? "Ops" : "PMC", 0);
    }
    /*
     *  call init, if it exists
     */
    init_func_name = Parrot_sprintf_c(interpreter, "Parrot_lib_%Ss_init", lib);
    cinit_func_name = string_to_cstring(interpreter, init_func_name);
    init_func = (void (*)(Interp *, PMC *))D2FPTR(Parrot_dlsym(handle,
                cinit_func_name));
    string_cstring_free(cinit_func_name);
    if (init_func)
        (init_func)(interpreter, lib_pmc);
    PMC_data(lib_pmc) = handle;
    /*
     * remember lib_pmc in iglobals
     */
    store_lib_pmc(interpreter, lib_pmc, path, type);
    /* UNLOCK */
    return lib_pmc;
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
