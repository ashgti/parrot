/* misc.h
 *  Copyright (C) 2001-2008, Parrot Foundation.
 *  SVN Info
 *     $Id$
 *  Overview:
 *     Miscellaneous functions, mainly the Parrot_sprintf family
 *  Data Structure and Algorithms:
 *     Uses a generalized formatting algorithm (spf_render.c) with
 *     a specialized vtable (spf_vtable.c) to handle argument extraction.
 *  History:
 *     This file used to be a simple one, with just a few function prototypes
 *     in it.  When misc.c was split in three, this file took on the job of
 *     being the internal header as well.
 *     To expose the guts of Parrot_sprintf, define IN_SPF_SYSTEM before
 *     including this file.
 *  Notes:
 *  References: misc.c, spf_vtable.c, spf_render.c
 */

#ifndef PARROT_MISC_H_GUARD
#define PARROT_MISC_H_GUARD

#include "parrot/parrot.h"

#define FLOAT_IS_ZERO(f) ((f) == 0.0)

#ifndef PARROT_HAS_C99_SNPRINTF
#  define snprintf Parrot_secret_snprintf
#endif

typedef int (*reg_move_func)(PARROT_INTERP, unsigned char d, unsigned char s, void *);

/* HEADERIZER BEGIN: src/utils.c */
/* Don't modify between HEADERIZER BEGIN / HEADERIZER END.  Your changes will be lost. */

PARROT_EXPORT
INTVAL Parrot_byte_index(SHIM_INTERP,
    ARGIN(const STRING *base),
    ARGIN(const STRING *search),
    UINTVAL start_offset)
        __attribute__nonnull__(2)
        __attribute__nonnull__(3);

PARROT_EXPORT
PARROT_WARN_UNUSED_RESULT
INTVAL Parrot_byte_rindex(SHIM_INTERP,
    ARGIN(const STRING *base),
    ARGIN(const STRING *search),
    UINTVAL start_offset)
        __attribute__nonnull__(2)
        __attribute__nonnull__(3);

PARROT_EXPORT
FLOATVAL Parrot_float_rand(INTVAL how_random);

PARROT_EXPORT
INTVAL Parrot_int_rand(INTVAL how_random);

PARROT_EXPORT
INTVAL Parrot_range_rand(INTVAL from, INTVAL to, INTVAL how_random);

PARROT_EXPORT
void Parrot_register_move(PARROT_INTERP,
    int n_regs,
    ARGOUT(unsigned char *dest_regs),
    ARGIN(unsigned char *src_regs),
    unsigned char temp_reg,
    reg_move_func mov,
    reg_move_func mov_alt,
    ARGIN(void *info))
        __attribute__nonnull__(1)
        __attribute__nonnull__(3)
        __attribute__nonnull__(4)
        __attribute__nonnull__(8)
        FUNC_MODIFIES(*dest_regs);

PARROT_EXPORT
void Parrot_srand(INTVAL seed);

PARROT_EXPORT
INTVAL Parrot_uint_rand(INTVAL how_random);

PARROT_CONST_FUNCTION
FLOATVAL floatval_mod(FLOATVAL n2, FLOATVAL n3);

PARROT_CONST_FUNCTION
INTVAL intval_mod(INTVAL i2, INTVAL i3);

void Parrot_quicksort(PARROT_INTERP,
    ARGMOD(void **data),
    UINTVAL n,
    ARGIN(PMC *cmp))
        __attribute__nonnull__(1)
        __attribute__nonnull__(2)
        __attribute__nonnull__(4)
        FUNC_MODIFIES(*data);

PARROT_WARN_UNUSED_RESULT
PARROT_CANNOT_RETURN_NULL
PMC* tm_to_array(PARROT_INTERP, ARGIN(const struct tm *tm))
        __attribute__nonnull__(1)
        __attribute__nonnull__(2);

#define ASSERT_ARGS_Parrot_byte_index __attribute__unused__ int _ASSERT_ARGS_CHECK = (\
       PARROT_ASSERT_ARG(base) \
    , PARROT_ASSERT_ARG(search))
#define ASSERT_ARGS_Parrot_byte_rindex __attribute__unused__ int _ASSERT_ARGS_CHECK = (\
       PARROT_ASSERT_ARG(base) \
    , PARROT_ASSERT_ARG(search))
#define ASSERT_ARGS_Parrot_float_rand __attribute__unused__ int _ASSERT_ARGS_CHECK = (0)
#define ASSERT_ARGS_Parrot_int_rand __attribute__unused__ int _ASSERT_ARGS_CHECK = (0)
#define ASSERT_ARGS_Parrot_range_rand __attribute__unused__ int _ASSERT_ARGS_CHECK = (0)
#define ASSERT_ARGS_Parrot_register_move __attribute__unused__ int _ASSERT_ARGS_CHECK = (\
       PARROT_ASSERT_ARG(interp) \
    , PARROT_ASSERT_ARG(dest_regs) \
    , PARROT_ASSERT_ARG(src_regs) \
    , PARROT_ASSERT_ARG(info))
#define ASSERT_ARGS_Parrot_srand __attribute__unused__ int _ASSERT_ARGS_CHECK = (0)
#define ASSERT_ARGS_Parrot_uint_rand __attribute__unused__ int _ASSERT_ARGS_CHECK = (0)
#define ASSERT_ARGS_floatval_mod __attribute__unused__ int _ASSERT_ARGS_CHECK = (0)
#define ASSERT_ARGS_intval_mod __attribute__unused__ int _ASSERT_ARGS_CHECK = (0)
#define ASSERT_ARGS_Parrot_quicksort __attribute__unused__ int _ASSERT_ARGS_CHECK = (\
       PARROT_ASSERT_ARG(interp) \
    , PARROT_ASSERT_ARG(data) \
    , PARROT_ASSERT_ARG(cmp))
#define ASSERT_ARGS_tm_to_array __attribute__unused__ int _ASSERT_ARGS_CHECK = (\
       PARROT_ASSERT_ARG(interp) \
    , PARROT_ASSERT_ARG(tm))
/* Don't modify between HEADERIZER BEGIN / HEADERIZER END.  Your changes will be lost. */
/* HEADERIZER END: src/utils.c */


/* HEADERIZER BEGIN: src/misc.c */
/* Don't modify between HEADERIZER BEGIN / HEADERIZER END.  Your changes will be lost. */

PARROT_EXPORT
PARROT_WARN_UNUSED_RESULT
PARROT_CANNOT_RETURN_NULL
STRING * Parrot_psprintf(PARROT_INTERP,
    ARGIN(STRING *pat),
    ARGOUT(PMC *ary))
        __attribute__nonnull__(1)
        __attribute__nonnull__(2)
        __attribute__nonnull__(3)
        FUNC_MODIFIES(*ary);

PARROT_EXPORT
int Parrot_secret_snprintf(
    ARGOUT(char *buffer),
    NULLOK(const size_t len),
    ARGIN(const char *format),
    ...)
        __attribute__nonnull__(1)
        __attribute__nonnull__(3)
        FUNC_MODIFIES(*buffer);

PARROT_EXPORT
void Parrot_snprintf(PARROT_INTERP,
    ARGOUT(char *targ),
    size_t len,
    ARGIN(const char *pat),
    ...)
        __attribute__nonnull__(1)
        __attribute__nonnull__(2)
        __attribute__nonnull__(4)
        FUNC_MODIFIES(*targ);

PARROT_EXPORT
PARROT_WARN_UNUSED_RESULT
PARROT_CANNOT_RETURN_NULL
STRING * Parrot_sprintf_c(PARROT_INTERP, ARGIN(const char *pat), ...)
        __attribute__nonnull__(1)
        __attribute__nonnull__(2);

PARROT_EXPORT
PARROT_WARN_UNUSED_RESULT
PARROT_CANNOT_RETURN_NULL
STRING * Parrot_sprintf_s(PARROT_INTERP, ARGIN(STRING *pat), ...)
        __attribute__nonnull__(1)
        __attribute__nonnull__(2);

PARROT_EXPORT
void Parrot_vsnprintf(PARROT_INTERP,
    ARGOUT(char *targ),
    size_t len,
    ARGIN(const char *pat),
    va_list args)
        __attribute__nonnull__(1)
        __attribute__nonnull__(2)
        __attribute__nonnull__(4)
        FUNC_MODIFIES(*targ);

PARROT_EXPORT
PARROT_CANNOT_RETURN_NULL
STRING * Parrot_vsprintf_c(PARROT_INTERP,
    ARGIN(const char *pat),
    va_list args)
        __attribute__nonnull__(1)
        __attribute__nonnull__(2);

PARROT_EXPORT
PARROT_WARN_UNUSED_RESULT
PARROT_CANNOT_RETURN_NULL
STRING * Parrot_vsprintf_s(PARROT_INTERP, ARGIN(STRING *pat), va_list args)
        __attribute__nonnull__(1)
        __attribute__nonnull__(2);

#define ASSERT_ARGS_Parrot_psprintf __attribute__unused__ int _ASSERT_ARGS_CHECK = (\
       PARROT_ASSERT_ARG(interp) \
    , PARROT_ASSERT_ARG(pat) \
    , PARROT_ASSERT_ARG(ary))
#define ASSERT_ARGS_Parrot_secret_snprintf __attribute__unused__ int _ASSERT_ARGS_CHECK = (\
       PARROT_ASSERT_ARG(buffer) \
    , PARROT_ASSERT_ARG(format))
#define ASSERT_ARGS_Parrot_snprintf __attribute__unused__ int _ASSERT_ARGS_CHECK = (\
       PARROT_ASSERT_ARG(interp) \
    , PARROT_ASSERT_ARG(targ) \
    , PARROT_ASSERT_ARG(pat))
#define ASSERT_ARGS_Parrot_sprintf_c __attribute__unused__ int _ASSERT_ARGS_CHECK = (\
       PARROT_ASSERT_ARG(interp) \
    , PARROT_ASSERT_ARG(pat))
#define ASSERT_ARGS_Parrot_sprintf_s __attribute__unused__ int _ASSERT_ARGS_CHECK = (\
       PARROT_ASSERT_ARG(interp) \
    , PARROT_ASSERT_ARG(pat))
#define ASSERT_ARGS_Parrot_vsnprintf __attribute__unused__ int _ASSERT_ARGS_CHECK = (\
       PARROT_ASSERT_ARG(interp) \
    , PARROT_ASSERT_ARG(targ) \
    , PARROT_ASSERT_ARG(pat))
#define ASSERT_ARGS_Parrot_vsprintf_c __attribute__unused__ int _ASSERT_ARGS_CHECK = (\
       PARROT_ASSERT_ARG(interp) \
    , PARROT_ASSERT_ARG(pat))
#define ASSERT_ARGS_Parrot_vsprintf_s __attribute__unused__ int _ASSERT_ARGS_CHECK = (\
       PARROT_ASSERT_ARG(interp) \
    , PARROT_ASSERT_ARG(pat))
/* Don't modify between HEADERIZER BEGIN / HEADERIZER END.  Your changes will be lost. */
/* HEADERIZER END: src/misc.c */


#ifdef IN_SPF_SYSTEM

    /* The size of all our buffers (and the maximum width of a float
     * field).
     */
#  define PARROT_SPRINTF_BUFFER_SIZE 4096

    /* The maximum floating-point precision--hopefully this and the
     * width cap will help head off buffer overruns.
     */
#  define PARROT_SPRINTF_MAX_PREC 3 * PARROT_SPRINTF_BUFFER_SIZE / 4

#  define cstr2pstr(cstr) string_make(interp, (cstr), strlen(cstr), "ascii", 0)
#  define char2pstr(ch)   string_make(interp, &(ch), 1, "ascii", 0)

    /* SPRINTF DATA STRUCTURE AND FLAGS */

    typedef enum {
        SIZE_REG = 0,
        SIZE_SHORT,
        SIZE_LONG,
        SIZE_HUGE,
        SIZE_XVAL,
        SIZE_OPCODE,
        SIZE_PMC,
        SIZE_PSTR
    } spf_type_t;

    /* SPRINTF ARGUMENT OBJECT */

    typedef struct sprintf_obj SPRINTF_OBJ;

    typedef STRING *(*sprintf_getchar_t)       (PARROT_INTERP, INTVAL, ARGIN(SPRINTF_OBJ *));
    typedef HUGEINTVAL(*sprintf_getint_t)      (PARROT_INTERP, INTVAL, ARGIN(SPRINTF_OBJ *));
    typedef UHUGEINTVAL(*sprintf_getuint_t)    (PARROT_INTERP, INTVAL, ARGIN(SPRINTF_OBJ *));
    typedef HUGEFLOATVAL(*sprintf_getfloat_t)  (PARROT_INTERP, INTVAL, ARGIN(SPRINTF_OBJ *));
    typedef STRING *(*sprintf_getstring_t)     (PARROT_INTERP, INTVAL, ARGIN(SPRINTF_OBJ *));
    typedef void *(*sprintf_getptr_t)          (PARROT_INTERP, INTVAL, ARGIN(SPRINTF_OBJ *));

    struct sprintf_obj {
        void *data;
        INTVAL index;

        sprintf_getchar_t getchr;
        sprintf_getint_t getint;
        sprintf_getuint_t getuint;
        sprintf_getfloat_t getfloat;
        sprintf_getstring_t getstring;
        sprintf_getptr_t getptr;
    };

    extern SPRINTF_OBJ pmc_core;
    extern const SPRINTF_OBJ va_core;

/* HEADERIZER BEGIN: src/spf_render.c */
/* Don't modify between HEADERIZER BEGIN / HEADERIZER END.  Your changes will be lost. */

PARROT_WARN_UNUSED_RESULT
PARROT_CANNOT_RETURN_NULL
STRING * Parrot_sprintf_format(PARROT_INTERP,
    ARGIN(STRING *pat),
    ARGIN(SPRINTF_OBJ *obj))
        __attribute__nonnull__(1)
        __attribute__nonnull__(2)
        __attribute__nonnull__(3);

#define ASSERT_ARGS_Parrot_sprintf_format __attribute__unused__ int _ASSERT_ARGS_CHECK = (\
       PARROT_ASSERT_ARG(interp) \
    , PARROT_ASSERT_ARG(pat) \
    , PARROT_ASSERT_ARG(obj))
/* Don't modify between HEADERIZER BEGIN / HEADERIZER END.  Your changes will be lost. */
/* HEADERIZER END: src/spf_render.c */

#endif /* IN_SPF_SYSTEM */


/*
 * generated src/revision.c
 */
PARROT_EXPORT int Parrot_config_revision(void);

#endif /* PARROT_MISC_H_GUARD */

/*
 * Local variables:
 *   c-file-style: "parrot"
 * End:
 * vim: expandtab shiftwidth=4:
 */
