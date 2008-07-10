/* pmc.h
 *  Copyright (C) 2001-2007, The Perl Foundation.
 *  SVN Info
 *     $Id$
 *  Overview:
 *     This is the api header for the pmc subsystem
 *  Data Structure and Algorithms:
 *  History:
 *  Notes:
 *  References:
 */

#ifndef PARROT_PMC_H_GUARD
#define PARROT_PMC_H_GUARD

#include "parrot/core_pmcs.h"
#include "parrot/pobj.h"
#include "parrot/thread.h"

#define PARROT_MAX_CLASSES 100

/* HEADERIZER BEGIN: src/pmc.c */
/* Don't modify between HEADERIZER BEGIN / HEADERIZER END.  Your changes will be lost. */

PARROT_API
PARROT_CANNOT_RETURN_NULL
PMC * constant_pmc_new(PARROT_INTERP, INTVAL base_type)
        __attribute__nonnull__(1);

PARROT_API
PARROT_CANNOT_RETURN_NULL
PMC * constant_pmc_new_init(PARROT_INTERP,
    INTVAL base_type,
    ARGIN_NULLOK(PMC *init))
        __attribute__nonnull__(1);

PARROT_API
PARROT_CANNOT_RETURN_NULL
PMC * constant_pmc_new_noinit(PARROT_INTERP, INTVAL base_type)
        __attribute__nonnull__(1);

PARROT_API
void dod_register_pmc(PARROT_INTERP, ARGIN(PMC* pmc))
        __attribute__nonnull__(1)
        __attribute__nonnull__(2);

PARROT_API
void Parrot_create_mro(PARROT_INTERP, INTVAL type)
        __attribute__nonnull__(1);

PARROT_API
INTVAL PMC_is_null(SHIM_INTERP, NULLOK(const PMC *pmc));

PARROT_API
PARROT_CANNOT_RETURN_NULL
PARROT_WARN_UNUSED_RESULT
PMC * pmc_new(PARROT_INTERP, INTVAL base_type)
        __attribute__nonnull__(1);

PARROT_API
PARROT_CANNOT_RETURN_NULL
PMC * pmc_new_init(PARROT_INTERP, INTVAL base_type, ARGOUT(PMC *init))
        __attribute__nonnull__(1)
        __attribute__nonnull__(3)
        FUNC_MODIFIES(*init);

PARROT_API
PARROT_CANNOT_RETURN_NULL
PMC * pmc_new_noinit(PARROT_INTERP, INTVAL base_type)
        __attribute__nonnull__(1);

PARROT_API
INTVAL pmc_register(PARROT_INTERP, ARGIN(STRING *name))
        __attribute__nonnull__(1)
        __attribute__nonnull__(2);

PARROT_API
PARROT_CANNOT_RETURN_NULL
PMC* pmc_reuse(PARROT_INTERP,
    ARGIN(PMC *pmc),
    INTVAL new_type,
    NULLOK(UINTVAL flags))
        __attribute__nonnull__(1)
        __attribute__nonnull__(2);

PARROT_API
PARROT_WARN_UNUSED_RESULT
INTVAL pmc_type(PARROT_INTERP, ARGIN_NULLOK(STRING *name))
        __attribute__nonnull__(1);

PARROT_API
INTVAL pmc_type_p(PARROT_INTERP, ARGIN(PMC *name))
        __attribute__nonnull__(1)
        __attribute__nonnull__(2);

void dod_unregister_pmc(PARROT_INTERP, ARGIN(PMC* pmc))
        __attribute__nonnull__(1)
        __attribute__nonnull__(2);

/* Don't modify between HEADERIZER BEGIN / HEADERIZER END.  Your changes will be lost. */
/* HEADERIZER END: src/pmc.c */

#endif /* PARROT_PMC_H_GUARD */

/*
 * Local variables:
 *   c-file-style: "parrot"
 * End:
 * vim: expandtab shiftwidth=4:
 */
