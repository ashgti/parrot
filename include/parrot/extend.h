/* extend.h
 *  Copyright (C) 2001-2008, The Perl Foundation.
 *  SVN Info
 *     $Id$
 *  Overview:
 *     This is the Parrot extension mechanism, the face we present to
 *     extension modules and whatnot
 *  Data Structure and Algorithms:
 *  History:
 *  Notes:
 *  References:
 *      extend.c, docs/pdds/pdd11_extending.pod.
 */

#ifndef PARROT_EXTEND_H_GUARD
#define PARROT_EXTEND_H_GUARD

#include <stdarg.h>
#include "parrot/config.h"      /* PARROT_VERSION, PARROT_JIT_CAPABLE... */
#include "parrot/interpreter.h" /* give us the interpreter flags */
#include "parrot/warnings.h"    /* give us the warnings flags    */

/* Two versions of each routine, one for when we're building the core
   and one for when people are writing extensions. If this gets abused
   we'll split this into two pieces and not install the core version,
   but that would be really annoying */
#if defined(PARROT_IN_CORE)

#define Parrot_String STRING *
#define Parrot_PMC PMC *
#define Parrot_Language Parrot_Int
#define Parrot_VTABLE VTABLE *

/* Macro to save off the original stack pointer for DOD scanning. If
   the stacktop was NULL, then set it to the address of the cached
   pointer, which is on the stack and as good a thing as any to use as
   an anchor */
#define PARROT_CALLIN_START(x) void *oldtop = (x)->lo_var_ptr; \
                               if (!oldtop) (x)->lo_var_ptr = &oldtop;
/* Put the stack top back, if what we cached was NULL. Otherwise we
   leave it alone and assume it's OK */
#define PARROT_CALLIN_END(x)   if (!oldtop) (x)->lo_var_ptr = NULL;

#else

typedef void * Parrot_String;
typedef void * Parrot_PMC;
typedef Parrot_Int Parrot_Language;
typedef void * Parrot_Encoding;
typedef void * Parrot_CharType;
typedef const void * Parrot_Const_Encoding;
typedef const void * Parrot_Const_CharType;
typedef const void * Parrot_VTABLE;

#endif

#include "parrot/extend_vtable.h" /* the auto-generated prototypes    */

/* HEADERIZER BEGIN: src/extend.c */
/* Don't modify between HEADERIZER BEGIN / HEADERIZER END.  Your changes will be lost. */

PARROT_API
PARROT_WARN_UNUSED_RESULT
PARROT_CAN_RETURN_NULL
void * Parrot_call_method(PARROT_INTERP,
    Parrot_PMC sub,
    Parrot_PMC obj,
    Parrot_String method,
    ARGIN(const char *signature),
    ...)
        __attribute__nonnull__(1)
        __attribute__nonnull__(5);

PARROT_API
Parrot_Float Parrot_call_method_ret_float(PARROT_INTERP,
    Parrot_PMC sub,
    Parrot_PMC obj,
    Parrot_String method,
    ARGIN(const char *signature),
    ...)
        __attribute__nonnull__(1)
        __attribute__nonnull__(5);

PARROT_API
Parrot_Int Parrot_call_method_ret_int(PARROT_INTERP,
    Parrot_PMC sub,
    Parrot_PMC obj,
    Parrot_String method,
    ARGIN(const char *signature),
    ...)
        __attribute__nonnull__(1)
        __attribute__nonnull__(5);

PARROT_API
PARROT_WARN_UNUSED_RESULT
PARROT_CAN_RETURN_NULL
void* Parrot_call_sub(PARROT_INTERP,
    Parrot_PMC sub,
    ARGIN(const char *signature),
    ...)
        __attribute__nonnull__(1)
        __attribute__nonnull__(3);

PARROT_API
Parrot_Float Parrot_call_sub_ret_float(PARROT_INTERP,
    Parrot_PMC sub,
    ARGIN(const char *signature),
    ...)
        __attribute__nonnull__(1)
        __attribute__nonnull__(3);

PARROT_API
Parrot_Int Parrot_call_sub_ret_int(PARROT_INTERP,
    Parrot_PMC sub,
    ARGIN(const char *signature),
    ...)
        __attribute__nonnull__(1)
        __attribute__nonnull__(3);

PARROT_API
int Parrot_eprintf(NULLOK_INTERP, ARGIN(const char *s), ...)
        __attribute__nonnull__(2);

PARROT_API
PARROT_WARN_UNUSED_RESULT
Parrot_Language Parrot_find_language(SHIM_INTERP, SHIM(char *language));

PARROT_API
int Parrot_fprintf(PARROT_INTERP,
    ARGIN(Parrot_PMC pio),
    ARGIN(const char *s),
    ...)
        __attribute__nonnull__(1)
        __attribute__nonnull__(2)
        __attribute__nonnull__(3);

PARROT_API
void Parrot_free_cstring(ARGIN_NULLOK(char *string));

PARROT_API
Parrot_Int Parrot_get_intreg(PARROT_INTERP, Parrot_Int regnum)
        __attribute__nonnull__(1);

PARROT_API
Parrot_Float Parrot_get_numreg(PARROT_INTERP, Parrot_Int regnum)
        __attribute__nonnull__(1);

PARROT_API
Parrot_PMC Parrot_get_pmcreg(PARROT_INTERP, Parrot_Int regnum)
        __attribute__nonnull__(1);

PARROT_API
Parrot_String Parrot_get_strreg(PARROT_INTERP, Parrot_Int regnum)
        __attribute__nonnull__(1);

PARROT_API
PARROT_PURE_FUNCTION
Parrot_VTABLE Parrot_get_vtable(PARROT_INTERP, Parrot_Int id)
        __attribute__nonnull__(1);

PARROT_API
PARROT_WARN_UNUSED_RESULT
PARROT_CANNOT_RETURN_NULL
Parrot_String Parrot_new_string(PARROT_INTERP,
    ARGIN_NULLOK(const char *buffer),
    Parrot_UInt length,
    ARGIN_NULLOK(const char * const encoding_name),
    Parrot_UInt flags)
        __attribute__nonnull__(1);

PARROT_API
void Parrot_PMC_delete_pmckey(PARROT_INTERP, Parrot_PMC pmc, Parrot_PMC key)
        __attribute__nonnull__(1);

PARROT_API
PARROT_MALLOC
PARROT_CAN_RETURN_NULL
char * Parrot_PMC_get_cstring(PARROT_INTERP, Parrot_PMC pmc)
        __attribute__nonnull__(1);

PARROT_API
PARROT_MALLOC
PARROT_CAN_RETURN_NULL
char * Parrot_PMC_get_cstring_intkey(PARROT_INTERP,
    Parrot_PMC pmc,
    Parrot_Int key)
        __attribute__nonnull__(1);

PARROT_API
PARROT_MALLOC
PARROT_CAN_RETURN_NULL
char * Parrot_PMC_get_cstringn(PARROT_INTERP,
    ARGIN(Parrot_PMC pmc),
    ARGOUT(Parrot_Int *length))
        __attribute__nonnull__(1)
        __attribute__nonnull__(2)
        __attribute__nonnull__(3)
        FUNC_MODIFIES(*length);

PARROT_API
PARROT_MALLOC
PARROT_CAN_RETURN_NULL
char * Parrot_PMC_get_cstringn_intkey(PARROT_INTERP,
    ARGIN(Parrot_PMC pmc),
    ARGOUT(Parrot_Int *length),
    Parrot_Int key)
        __attribute__nonnull__(1)
        __attribute__nonnull__(2)
        __attribute__nonnull__(3)
        FUNC_MODIFIES(*length);

PARROT_API
Parrot_Int Parrot_PMC_get_intval(PARROT_INTERP, Parrot_PMC pmc)
        __attribute__nonnull__(1);

PARROT_API
Parrot_Int Parrot_PMC_get_intval_intkey(PARROT_INTERP,
    Parrot_PMC pmc,
    Parrot_Int key)
        __attribute__nonnull__(1);

PARROT_API
Parrot_Int Parrot_PMC_get_intval_pmckey(PARROT_INTERP,
    Parrot_PMC pmc,
    Parrot_PMC key)
        __attribute__nonnull__(1);

PARROT_API
Parrot_Float Parrot_PMC_get_numval(PARROT_INTERP, Parrot_PMC pmc)
        __attribute__nonnull__(1);

PARROT_API
Parrot_Float Parrot_PMC_get_numval_intkey(PARROT_INTERP,
    Parrot_PMC pmc,
    Parrot_Int key)
        __attribute__nonnull__(1);

PARROT_API
Parrot_PMC Parrot_PMC_get_pmc_intkey(PARROT_INTERP,
    Parrot_PMC pmc,
    Parrot_Int key)
        __attribute__nonnull__(1);

PARROT_API
PARROT_WARN_UNUSED_RESULT
PARROT_CAN_RETURN_NULL
void * Parrot_PMC_get_pointer_intkey(PARROT_INTERP,
    Parrot_PMC pmc,
    Parrot_Int key)
        __attribute__nonnull__(1);

PARROT_API
Parrot_String Parrot_PMC_get_string_intkey(PARROT_INTERP,
    Parrot_PMC pmc,
    Parrot_Int key)
        __attribute__nonnull__(1);

PARROT_API
Parrot_PMC Parrot_PMC_new(PARROT_INTERP, Parrot_Int type)
        __attribute__nonnull__(1);

PARROT_API
Parrot_PMC Parrot_PMC_null(void);

PARROT_API
void Parrot_PMC_push_intval(PARROT_INTERP, Parrot_PMC pmc, Parrot_Int value)
        __attribute__nonnull__(1);

PARROT_API
void Parrot_PMC_push_numval(PARROT_INTERP,
    Parrot_PMC pmc,
    Parrot_Float value)
        __attribute__nonnull__(1);

PARROT_API
void Parrot_PMC_set_cstring(PARROT_INTERP,
    Parrot_PMC pmc,
    ARGIN_NULLOK(const char *value))
        __attribute__nonnull__(1);

PARROT_API
void Parrot_PMC_set_cstring_intkey(PARROT_INTERP,
    Parrot_PMC pmc,
    Parrot_Int key,
    ARGIN_NULLOK(const char *value))
        __attribute__nonnull__(1);

PARROT_API
void Parrot_PMC_set_cstringn(PARROT_INTERP,
    Parrot_PMC pmc,
    ARGIN_NULLOK(const char *value),
    Parrot_UInt length)
        __attribute__nonnull__(1);

PARROT_API
void Parrot_PMC_set_cstringn_intkey(PARROT_INTERP,
    Parrot_PMC pmc,
    Parrot_Int key,
    ARGIN_NULLOK(const char *value),
    Parrot_UInt length)
        __attribute__nonnull__(1);

PARROT_API
void Parrot_PMC_set_intval(PARROT_INTERP, Parrot_PMC pmc, Parrot_Int value)
        __attribute__nonnull__(1);

PARROT_API
void Parrot_PMC_set_intval_intkey(PARROT_INTERP,
    Parrot_PMC pmc,
    Parrot_Int key,
    Parrot_Int value)
        __attribute__nonnull__(1);

PARROT_API
void Parrot_PMC_set_numval(PARROT_INTERP,
    Parrot_PMC pmc,
    Parrot_Float value)
        __attribute__nonnull__(1);

PARROT_API
void Parrot_PMC_set_numval_intkey(PARROT_INTERP,
    Parrot_PMC pmc,
    Parrot_Int key,
    Parrot_Float value)
        __attribute__nonnull__(1);

PARROT_API
void Parrot_PMC_set_pmc_intkey(PARROT_INTERP,
    Parrot_PMC pmc,
    Parrot_Int key,
    Parrot_PMC value)
        __attribute__nonnull__(1);

PARROT_API
void Parrot_PMC_set_pmc_pmckey(PARROT_INTERP,
    Parrot_PMC pmc,
    Parrot_PMC key,
    Parrot_PMC value)
        __attribute__nonnull__(1);

PARROT_API
void Parrot_PMC_set_pointer_intkey(PARROT_INTERP,
    ARGIN(Parrot_PMC pmc),
    Parrot_Int key,
    ARGIN_NULLOK(void *value))
        __attribute__nonnull__(1)
        __attribute__nonnull__(2);

PARROT_API
void Parrot_PMC_set_string(PARROT_INTERP,
    Parrot_PMC pmc,
    Parrot_String value)
        __attribute__nonnull__(1);

PARROT_API
void Parrot_PMC_set_string_intkey(PARROT_INTERP,
    Parrot_PMC pmc,
    Parrot_Int key,
    Parrot_String value)
        __attribute__nonnull__(1);

PARROT_API
void Parrot_PMC_set_vtable(SHIM_INTERP,
    Parrot_PMC pmc,
    Parrot_VTABLE vtable);

PARROT_API
Parrot_Int Parrot_PMC_typenum(PARROT_INTERP,
    ARGIN_NULLOK(const char *_class))
        __attribute__nonnull__(1);

PARROT_API
int Parrot_printf(NULLOK_INTERP, ARGIN(const char *s), ...)
        __attribute__nonnull__(2);

PARROT_API
void Parrot_register_pmc(PARROT_INTERP, Parrot_PMC pmc)
        __attribute__nonnull__(1);

PARROT_API
void Parrot_set_intreg(PARROT_INTERP, Parrot_Int regnum, Parrot_Int value)
        __attribute__nonnull__(1);

PARROT_API
void Parrot_set_numreg(PARROT_INTERP, Parrot_Int regnum, Parrot_Float value)
        __attribute__nonnull__(1);

PARROT_API
void Parrot_set_pmcreg(PARROT_INTERP, Parrot_Int regnum, Parrot_PMC value)
        __attribute__nonnull__(1);

PARROT_API
void Parrot_set_strreg(PARROT_INTERP,
    Parrot_Int regnum,
    Parrot_String value)
        __attribute__nonnull__(1);

PARROT_API
void Parrot_unregister_pmc(PARROT_INTERP, Parrot_PMC pmc)
        __attribute__nonnull__(1);

PARROT_API
int Parrot_vfprintf(PARROT_INTERP,
    ARGIN(Parrot_PMC pio),
    ARGIN(const char *s),
    va_list args)
        __attribute__nonnull__(1)
        __attribute__nonnull__(2)
        __attribute__nonnull__(3);

/* Don't modify between HEADERIZER BEGIN / HEADERIZER END.  Your changes will be lost. */
/* HEADERIZER END: src/extend.c */

#endif /* PARROT_EXTEND_H_GUARD */

/*
 * Local variables:
 *   c-file-style: "parrot"
 * End:
 * vim: expandtab shiftwidth=4:
 */
