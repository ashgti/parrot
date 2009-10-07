/* key.h
 *  Copyright (C) 2001-2007, Parrot Foundation.
 *  SVN Info
 *     $Id$
 *  Overview:
 *     This is the API header for the PMC subsystem
 *  Data Structure and Algorithms:
 *  History:
 *  Notes:
 *  References:
 */

#ifndef PARROT_KEY_H_GUARD
#define PARROT_KEY_H_GUARD

#include "parrot/parrot.h"

/*
 Type of Keys.
 C<KEY_register_FLAG> used for indirect referencing. E.g.
   $S0 = "foo"
   $P1 = $P0[$S0]
 In this case C<[$S0]> will have type C<KEY_string_FLAG | KEY_register_FLAG>
 and store I<integer> value of string register number.
*/
typedef enum {
    KEY_integer_FLAG        = PObj_private0_FLAG,
    KEY_number_FLAG         = PObj_private1_FLAG,
    KEY_string_FLAG         = PObj_private2_FLAG,
    KEY_pmc_FLAG            = PObj_private3_FLAG,
    KEY_register_FLAG       = PObj_private4_FLAG,

    KEY_type_FLAGS          = KEY_integer_FLAG         |
                              KEY_number_FLAG          |
                              KEY_string_FLAG          |
                              KEY_pmc_FLAG             |
                              KEY_register_FLAG

} KEY_flags;

/* HEADERIZER BEGIN: src/key.c */
/* Don't modify between HEADERIZER BEGIN / HEADERIZER END.  Your changes will be lost. */

PARROT_EXPORT
PARROT_CANNOT_RETURN_NULL
PARROT_IGNORABLE_RESULT
PMC * key_append(PARROT_INTERP, ARGMOD(PMC *key1), ARGIN(PMC *key2))
        __attribute__nonnull__(1)
        __attribute__nonnull__(2)
        __attribute__nonnull__(3)
        FUNC_MODIFIES(*key1);

PARROT_EXPORT
PARROT_WARN_UNUSED_RESULT
INTVAL key_integer(PARROT_INTERP, ARGIN(PMC *key))
        __attribute__nonnull__(1)
        __attribute__nonnull__(2);

PARROT_EXPORT
void key_mark(PARROT_INTERP, ARGIN(PMC *key))
        __attribute__nonnull__(1)
        __attribute__nonnull__(2);

PARROT_EXPORT
PARROT_CANNOT_RETURN_NULL
PARROT_WARN_UNUSED_RESULT
PMC * key_new(PARROT_INTERP)
        __attribute__nonnull__(1);

PARROT_EXPORT
PARROT_CANNOT_RETURN_NULL
PARROT_WARN_UNUSED_RESULT
PMC * key_new_cstring(PARROT_INTERP, ARGIN_NULLOK(const char *value))
        __attribute__nonnull__(1);

PARROT_EXPORT
PARROT_CANNOT_RETURN_NULL
PARROT_WARN_UNUSED_RESULT
PMC * key_new_integer(PARROT_INTERP, INTVAL value)
        __attribute__nonnull__(1);

PARROT_EXPORT
PARROT_CANNOT_RETURN_NULL
PARROT_WARN_UNUSED_RESULT
PMC * key_new_number(PARROT_INTERP, FLOATVAL value)
        __attribute__nonnull__(1);

PARROT_EXPORT
PARROT_CANNOT_RETURN_NULL
PARROT_WARN_UNUSED_RESULT
PMC * key_new_pmc(PARROT_INTERP, ARGIN(PMC *value))
        __attribute__nonnull__(1)
        __attribute__nonnull__(2);

PARROT_EXPORT
PARROT_CANNOT_RETURN_NULL
PARROT_WARN_UNUSED_RESULT
PMC * key_new_string(PARROT_INTERP, ARGIN(STRING *value))
        __attribute__nonnull__(1)
        __attribute__nonnull__(2);

PARROT_EXPORT
PARROT_CAN_RETURN_NULL
PARROT_WARN_UNUSED_RESULT
PMC * key_next(PARROT_INTERP, ARGIN(PMC *key))
        __attribute__nonnull__(1)
        __attribute__nonnull__(2);

PARROT_EXPORT
PARROT_WARN_UNUSED_RESULT
FLOATVAL key_number(PARROT_INTERP, ARGIN(PMC *key))
        __attribute__nonnull__(1)
        __attribute__nonnull__(2);

PARROT_EXPORT
PARROT_CANNOT_RETURN_NULL
PARROT_WARN_UNUSED_RESULT
PMC * key_pmc(PARROT_INTERP, ARGIN(PMC *key))
        __attribute__nonnull__(1)
        __attribute__nonnull__(2);

PARROT_EXPORT
void key_set_integer(PARROT_INTERP, ARGMOD(PMC *key), INTVAL value)
        __attribute__nonnull__(1)
        __attribute__nonnull__(2)
        FUNC_MODIFIES(*key);

PARROT_EXPORT
void key_set_number(PARROT_INTERP, ARGMOD(PMC *key), FLOATVAL value)
        __attribute__nonnull__(1)
        __attribute__nonnull__(2)
        FUNC_MODIFIES(*key);

PARROT_EXPORT
void key_set_pmc(PARROT_INTERP, ARGMOD(PMC *key), ARGIN(PMC *value))
        __attribute__nonnull__(1)
        __attribute__nonnull__(2)
        __attribute__nonnull__(3)
        FUNC_MODIFIES(*key);

PARROT_EXPORT
void key_set_register(PARROT_INTERP,
    ARGMOD(PMC *key),
    INTVAL value,
    INTVAL flag)
        __attribute__nonnull__(1)
        __attribute__nonnull__(2)
        FUNC_MODIFIES(*key);

PARROT_EXPORT
void key_set_string(PARROT_INTERP, ARGMOD(PMC *key), ARGIN(STRING *value))
        __attribute__nonnull__(1)
        __attribute__nonnull__(2)
        __attribute__nonnull__(3)
        FUNC_MODIFIES(*key);

PARROT_EXPORT
PARROT_CANNOT_RETURN_NULL
PARROT_WARN_UNUSED_RESULT
STRING * key_set_to_string(PARROT_INTERP, ARGIN_NULLOK(PMC *key))
        __attribute__nonnull__(1);

PARROT_EXPORT
PARROT_WARN_UNUSED_RESULT
INTVAL key_type(SHIM_INTERP, ARGIN(const PMC *key))
        __attribute__nonnull__(2);

PARROT_WARN_UNUSED_RESULT
PARROT_CAN_RETURN_NULL
STRING * key_string(PARROT_INTERP, ARGIN(PMC *key))
        __attribute__nonnull__(1)
        __attribute__nonnull__(2);

#define ASSERT_ARGS_key_append __attribute__unused__ int _ASSERT_ARGS_CHECK = (\
       PARROT_ASSERT_ARG(interp) \
    , PARROT_ASSERT_ARG(key1) \
    , PARROT_ASSERT_ARG(key2))
#define ASSERT_ARGS_key_integer __attribute__unused__ int _ASSERT_ARGS_CHECK = (\
       PARROT_ASSERT_ARG(interp) \
    , PARROT_ASSERT_ARG(key))
#define ASSERT_ARGS_key_mark __attribute__unused__ int _ASSERT_ARGS_CHECK = (\
       PARROT_ASSERT_ARG(interp) \
    , PARROT_ASSERT_ARG(key))
#define ASSERT_ARGS_key_new __attribute__unused__ int _ASSERT_ARGS_CHECK = (\
       PARROT_ASSERT_ARG(interp))
#define ASSERT_ARGS_key_new_cstring __attribute__unused__ int _ASSERT_ARGS_CHECK = (\
       PARROT_ASSERT_ARG(interp))
#define ASSERT_ARGS_key_new_integer __attribute__unused__ int _ASSERT_ARGS_CHECK = (\
       PARROT_ASSERT_ARG(interp))
#define ASSERT_ARGS_key_new_number __attribute__unused__ int _ASSERT_ARGS_CHECK = (\
       PARROT_ASSERT_ARG(interp))
#define ASSERT_ARGS_key_new_pmc __attribute__unused__ int _ASSERT_ARGS_CHECK = (\
       PARROT_ASSERT_ARG(interp) \
    , PARROT_ASSERT_ARG(value))
#define ASSERT_ARGS_key_new_string __attribute__unused__ int _ASSERT_ARGS_CHECK = (\
       PARROT_ASSERT_ARG(interp) \
    , PARROT_ASSERT_ARG(value))
#define ASSERT_ARGS_key_next __attribute__unused__ int _ASSERT_ARGS_CHECK = (\
       PARROT_ASSERT_ARG(interp) \
    , PARROT_ASSERT_ARG(key))
#define ASSERT_ARGS_key_number __attribute__unused__ int _ASSERT_ARGS_CHECK = (\
       PARROT_ASSERT_ARG(interp) \
    , PARROT_ASSERT_ARG(key))
#define ASSERT_ARGS_key_pmc __attribute__unused__ int _ASSERT_ARGS_CHECK = (\
       PARROT_ASSERT_ARG(interp) \
    , PARROT_ASSERT_ARG(key))
#define ASSERT_ARGS_key_set_integer __attribute__unused__ int _ASSERT_ARGS_CHECK = (\
       PARROT_ASSERT_ARG(interp) \
    , PARROT_ASSERT_ARG(key))
#define ASSERT_ARGS_key_set_number __attribute__unused__ int _ASSERT_ARGS_CHECK = (\
       PARROT_ASSERT_ARG(interp) \
    , PARROT_ASSERT_ARG(key))
#define ASSERT_ARGS_key_set_pmc __attribute__unused__ int _ASSERT_ARGS_CHECK = (\
       PARROT_ASSERT_ARG(interp) \
    , PARROT_ASSERT_ARG(key) \
    , PARROT_ASSERT_ARG(value))
#define ASSERT_ARGS_key_set_register __attribute__unused__ int _ASSERT_ARGS_CHECK = (\
       PARROT_ASSERT_ARG(interp) \
    , PARROT_ASSERT_ARG(key))
#define ASSERT_ARGS_key_set_string __attribute__unused__ int _ASSERT_ARGS_CHECK = (\
       PARROT_ASSERT_ARG(interp) \
    , PARROT_ASSERT_ARG(key) \
    , PARROT_ASSERT_ARG(value))
#define ASSERT_ARGS_key_set_to_string __attribute__unused__ int _ASSERT_ARGS_CHECK = (\
       PARROT_ASSERT_ARG(interp))
#define ASSERT_ARGS_key_type __attribute__unused__ int _ASSERT_ARGS_CHECK = (\
       PARROT_ASSERT_ARG(key))
#define ASSERT_ARGS_key_string __attribute__unused__ int _ASSERT_ARGS_CHECK = (\
       PARROT_ASSERT_ARG(interp) \
    , PARROT_ASSERT_ARG(key))
/* Don't modify between HEADERIZER BEGIN / HEADERIZER END.  Your changes will be lost. */
/* HEADERIZER END: src/key.c */

#endif /* PARROT_KEY_H_GUARD */

/*
 * Local variables:
 *   c-file-style: "parrot"
 * End:
 * vim: expandtab shiftwidth=4:
 */
