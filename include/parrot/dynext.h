/*
 * Copyright (C) 2003-2007, The Perl Foundation
 */

/* dynext.h
*
* $Id$
*
*   Parrot dynamic extensions
*/

#ifndef PARROT_DYNEXT_H_GUARD
#define PARROT_DYNEXT_H_GUARD

/* HEADERIZER BEGIN: src/dynext.c */

PARROT_API
PARROT_WARN_UNUSED_RESULT
PARROT_CANNOT_RETURN_NULL
PMC * Parrot_clone_lib_into(
    ARGMOD(Interp *d),
    ARGMOD(Interp *s),
    ARGIN(PMC *lib_pmc))
        __attribute__nonnull__(1)
        __attribute__nonnull__(2)
        __attribute__nonnull__(3)
        FUNC_MODIFIES(*d)
        FUNC_MODIFIES(*s);

PARROT_API
PARROT_CANNOT_RETURN_NULL
PMC * Parrot_init_lib(PARROT_INTERP,
    ARGIN_NULLOK(PMC *(*load_func)(PARROT_INTERP)),
    ARGIN_NULLOK(void (*init_func)(PARROT_INTERP,
    ARGIN_NULLOK(PMC *))))
        __attribute__nonnull__(1);

PARROT_API
PARROT_WARN_UNUSED_RESULT
PARROT_CANNOT_RETURN_NULL
PMC * Parrot_load_lib(PARROT_INTERP,
    ARGIN_NULLOK(STRING *lib),
    SHIM(PMC *initializer))
        __attribute__nonnull__(1);

/* HEADERIZER END: src/dynext.c */

#endif /* PARROT_DYNEXT_H_GUARD */

/*
 * Local variables:
 *   c-file-style: "parrot"
 * End:
 * vim: expandtab shiftwidth=4:
 */
