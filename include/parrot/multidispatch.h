/* multidispatch.h
 *  Copyright (C) 2003-2007, The Perl Foundation.
 *  SVN Info
 *     $Id$
 *  Overview:
 *     This is the API header for the mmd subsystem
 *  Data Structure and Algorithms:
 *  History:
 *  Notes:
 *  References:
 */

#ifndef PARROT_MMD_H_GUARD
#define PARROT_MMD_H_GUARD

#include "parrot/config.h"
#include "parrot/compiler.h"
#include "parrot/parrot.h"

#define PARROT_MMD_MAX_CLASS_DEPTH 1000

/* function typedefs */
typedef PMC*    (*mmd_f_p_ppp)(PARROT_INTERP, PMC *, PMC *, PMC *);
typedef PMC*    (*mmd_f_p_pip)(PARROT_INTERP, PMC *, INTVAL, PMC *);
typedef PMC*    (*mmd_f_p_pnp)(PARROT_INTERP, PMC *, FLOATVAL, PMC *);
typedef PMC*    (*mmd_f_p_psp)(PARROT_INTERP, PMC *, STRING *, PMC *);

typedef void (*mmd_f_v_pp)(PARROT_INTERP, PMC *, PMC *);
typedef void (*mmd_f_v_pi)(PARROT_INTERP, PMC *, INTVAL);
typedef void (*mmd_f_v_pn)(PARROT_INTERP, PMC *, FLOATVAL);
typedef void (*mmd_f_v_ps)(PARROT_INTERP, PMC *, STRING *);

typedef INTVAL (*mmd_f_i_pp)(PARROT_INTERP, PMC *, PMC *);

typedef struct _MMD_init {
        INTVAL func_nr;
        INTVAL left, right;
        funcptr_t func_ptr;
} MMD_init;

typedef struct _MMD_table {
    funcptr_t *mmd_funcs;     /* The functions for the MMD table */
    UINTVAL x;               /* The x coord for each table */
    UINTVAL y;               /* The y coord for each table */
} MMD_table;

typedef struct _multi_func_list {
        const char *multi_name;
        const char *short_sig;
        const char *full_sig;
        funcptr_t func_ptr;
} multi_func_list;


/* HEADERIZER BEGIN: src/multidispatch.c */
/* Don't modify between HEADERIZER BEGIN / HEADERIZER END.  Your changes will be lost. */

PARROT_API
PARROT_WARN_UNUSED_RESULT
PARROT_CANNOT_RETURN_NULL
funcptr_t get_mmd_dispatch_type(PARROT_INTERP,
    INTVAL func_nr,
    INTVAL left_type,
    INTVAL right_type,
    ARGOUT(int *is_pmc))
        __attribute__nonnull__(1)
        __attribute__nonnull__(5)
        FUNC_MODIFIES(*is_pmc);

PARROT_API
PARROT_WARN_UNUSED_RESULT
PARROT_CANNOT_RETURN_NULL
PMC* Parrot_build_sig_object_from_varargs(PARROT_INTERP,
    ARGIN(const char *sig),
    va_list args)
        __attribute__nonnull__(1)
        __attribute__nonnull__(2);

PARROT_API
void Parrot_mmd_add_function(PARROT_INTERP,
    INTVAL func_nr,
    NULLOK(funcptr_t function))
        __attribute__nonnull__(1);

PARROT_API
void Parrot_mmd_add_multi_from_c_args(PARROT_INTERP,
    ARGIN(const char *sub_name),
    ARGIN(const char *short_sig),
    ARGIN(const char *long_sig),
    ARGIN(funcptr_t multi_func_ptr))
        __attribute__nonnull__(1)
        __attribute__nonnull__(2)
        __attribute__nonnull__(3)
        __attribute__nonnull__(4)
        __attribute__nonnull__(5);

PARROT_API
void Parrot_mmd_add_multi_from_long_sig(PARROT_INTERP,
    ARGIN(STRING *sub_name),
    ARGIN(STRING *long_sig),
    ARGIN(PMC *sub_obj))
        __attribute__nonnull__(1)
        __attribute__nonnull__(2)
        __attribute__nonnull__(3)
        __attribute__nonnull__(4);

PARROT_API
void Parrot_mmd_add_multi_list_from_c_args(PARROT_INTERP,
    ARGIN(const multi_func_list *mmd_info),
    INTVAL elements)
        __attribute__nonnull__(1)
        __attribute__nonnull__(2);

PARROT_API
void Parrot_mmd_destroy(PARROT_INTERP)
        __attribute__nonnull__(1);

PARROT_API
PARROT_CANNOT_RETURN_NULL
PARROT_WARN_UNUSED_RESULT
PMC * Parrot_mmd_find_multi_from_long_sig(PARROT_INTERP,
    ARGIN(STRING *name),
    ARGIN(STRING *long_sig))
        __attribute__nonnull__(1)
        __attribute__nonnull__(2)
        __attribute__nonnull__(3);

PARROT_API
PARROT_WARN_UNUSED_RESULT
PARROT_CANNOT_RETURN_NULL
PMC* Parrot_mmd_find_multi_from_sig_obj(PARROT_INTERP,
    ARGIN(STRING *name),
    ARGIN(PMC *invoke_sig))
        __attribute__nonnull__(1)
        __attribute__nonnull__(2)
        __attribute__nonnull__(3);

PARROT_API
PARROT_CAN_RETURN_NULL
void Parrot_mmd_multi_dispatch_from_c_args(PARROT_INTERP,
    ARGIN(const char *name),
    ARGIN(const char *sig),
    ...)
        __attribute__nonnull__(1)
        __attribute__nonnull__(2)
        __attribute__nonnull__(3);

PARROT_API
void Parrot_mmd_register(PARROT_INTERP,
    INTVAL func_nr,
    INTVAL left_type,
    INTVAL right_type,
    NULLOK(funcptr_t funcptr))
        __attribute__nonnull__(1);

PARROT_API
void Parrot_mmd_register_sub(PARROT_INTERP,
    INTVAL func_nr,
    INTVAL left_type,
    INTVAL right_type,
    ARGIN(const PMC *sub))
        __attribute__nonnull__(1)
        __attribute__nonnull__(5);

PARROT_API
PARROT_CANNOT_RETURN_NULL
PARROT_WARN_UNUSED_RESULT
PMC * Parrot_MMD_search_default_infix(PARROT_INTERP,
    ARGIN(STRING *meth),
    INTVAL left_type,
    INTVAL right_type)
        __attribute__nonnull__(1)
        __attribute__nonnull__(2);

PARROT_API
PARROT_CAN_RETURN_NULL
PARROT_WARN_UNUSED_RESULT
PMC * Parrot_mmd_sort_manhattan(PARROT_INTERP, ARGIN(PMC *candidates))
        __attribute__nonnull__(1)
        __attribute__nonnull__(2);

PARROT_API
PARROT_CAN_RETURN_NULL
PARROT_WARN_UNUSED_RESULT
PMC * Parrot_mmd_sort_manhattan_by_sig_pmc(PARROT_INTERP,
    ARGIN(PMC *candidates),
    ARGIN(PMC* invoke_sig))
        __attribute__nonnull__(1)
        __attribute__nonnull__(2)
        __attribute__nonnull__(3);

/* Don't modify between HEADERIZER BEGIN / HEADERIZER END.  Your changes will be lost. */
/* HEADERIZER END: src/multidispatch.c */


#endif /* PARROT_MMD_H_GUARD */

/*
 * Local variables:
 *   c-file-style: "parrot"
 * End:
 * vim: expandtab shiftwidth=4:
 */
