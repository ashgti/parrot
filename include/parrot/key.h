/* key.h
 *  Copyright (C) 2001-2007, The Perl Foundation.
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

typedef enum {
    KEY_integer_FLAG        = PObj_private0_FLAG,
    KEY_number_FLAG         = PObj_private1_FLAG,
    KEY_hash_iterator_FLAGS = PObj_private0_FLAG | PObj_private1_FLAG,
    KEY_string_FLAG         = PObj_private2_FLAG,
    KEY_pmc_FLAG            = PObj_private3_FLAG,
    KEY_register_FLAG       = PObj_private4_FLAG,

    KEY_start_slice_FLAG    = PObj_private5_FLAG,
    KEY_end_slice_FLAG      = PObj_private6_FLAG,
    KEY_inf_slice_FLAG      = PObj_private7_FLAG,

    KEY_type_FLAGS          = KEY_integer_FLAG         |
                              KEY_number_FLAG          |
                              KEY_string_FLAG          |
                              KEY_pmc_FLAG             |
                              KEY_register_FLAG        |
                              KEY_hash_iterator_FLAGS

} KEY_flags;

#define KEY_IS_HASH_ITERATOR(k) \
    ((PObj_get_FLAGS((k)) & KEY_type_FLAGS) == KEY_hash_iterator_FLAGS)

/* HEADERIZER BEGIN: src/key.c */

PARROT_API
PARROT_CANNOT_RETURN_NULL
PARROT_IGNORABLE_RESULT
PMC * key_append(SHIM_INTERP, NOTNULL(PMC *key1), NOTNULL(PMC *key2))
        __attribute__nonnull__(2)
        __attribute__nonnull__(3);

PARROT_API
PARROT_WARN_UNUSED_RESULT
INTVAL key_integer(PARROT_INTERP, NOTNULL(PMC *key))
        __attribute__nonnull__(1)
        __attribute__nonnull__(2);

PARROT_API
void key_mark(PARROT_INTERP, NOTNULL(PMC *key))
        __attribute__nonnull__(1)
        __attribute__nonnull__(2);

PARROT_API
PARROT_CANNOT_RETURN_NULL
PARROT_WARN_UNUSED_RESULT
PMC * key_new(PARROT_INTERP)
        __attribute__nonnull__(1);

PARROT_API
PARROT_CANNOT_RETURN_NULL
PARROT_WARN_UNUSED_RESULT
PMC * key_new_cstring(PARROT_INTERP, ARGIN_NULLOK(const char *value))
        __attribute__nonnull__(1);

PARROT_API
PARROT_CANNOT_RETURN_NULL
PARROT_WARN_UNUSED_RESULT
PMC * key_new_integer(PARROT_INTERP, INTVAL value)
        __attribute__nonnull__(1);

PARROT_API
PARROT_CANNOT_RETURN_NULL
PARROT_WARN_UNUSED_RESULT
PMC * key_new_number(PARROT_INTERP, FLOATVAL value)
        __attribute__nonnull__(1);

PARROT_API
PARROT_CANNOT_RETURN_NULL
PARROT_WARN_UNUSED_RESULT
PMC * key_new_pmc(PARROT_INTERP, NOTNULL(PMC *value))
        __attribute__nonnull__(1)
        __attribute__nonnull__(2);

PARROT_API
PARROT_CANNOT_RETURN_NULL
PARROT_WARN_UNUSED_RESULT
PMC * key_new_string(PARROT_INTERP, NOTNULL(STRING *value))
        __attribute__nonnull__(1)
        __attribute__nonnull__(2);

PARROT_API
PARROT_CAN_RETURN_NULL
PARROT_WARN_UNUSED_RESULT
PMC * key_next(SHIM_INTERP, NOTNULL(PMC *key))
        __attribute__nonnull__(2);

PARROT_API
PARROT_WARN_UNUSED_RESULT
FLOATVAL key_number(PARROT_INTERP, NOTNULL(PMC *key))
        __attribute__nonnull__(1)
        __attribute__nonnull__(2);

PARROT_API
PARROT_CANNOT_RETURN_NULL
PARROT_WARN_UNUSED_RESULT
PMC * key_pmc(PARROT_INTERP, NOTNULL(PMC *key))
        __attribute__nonnull__(1)
        __attribute__nonnull__(2);

PARROT_API
void key_set_integer(SHIM_INTERP, NOTNULL(PMC *key), INTVAL value)
        __attribute__nonnull__(2);

PARROT_API
void key_set_number(SHIM_INTERP, NOTNULL(PMC *key), FLOATVAL value)
        __attribute__nonnull__(2);

PARROT_API
void key_set_pmc(PARROT_INTERP, NOTNULL(PMC *key), NOTNULL(PMC *value))
        __attribute__nonnull__(1)
        __attribute__nonnull__(2)
        __attribute__nonnull__(3);

PARROT_API
void key_set_register(SHIM_INTERP,
    NOTNULL(PMC *key),
    INTVAL value,
    INTVAL flag)
        __attribute__nonnull__(2);

PARROT_API
void key_set_string(SHIM_INTERP, NOTNULL(PMC *key), NOTNULL(STRING *value))
        __attribute__nonnull__(2)
        __attribute__nonnull__(3);

PARROT_API
PARROT_CANNOT_RETURN_NULL
PARROT_WARN_UNUSED_RESULT
STRING * key_set_to_string(PARROT_INTERP, NULLOK(PMC *key))
        __attribute__nonnull__(1);

PARROT_API
PARROT_WARN_UNUSED_RESULT
PARROT_CANNOT_RETURN_NULL
STRING * key_string(PARROT_INTERP, NOTNULL(PMC *key))
        __attribute__nonnull__(1)
        __attribute__nonnull__(2);

PARROT_API
PARROT_WARN_UNUSED_RESULT
INTVAL key_type(SHIM_INTERP, ARGIN(const PMC *key));

/* HEADERIZER END: src/key.c */

#endif /* PARROT_KEY_H_GUARD */

/*
 * Local variables:
 *   c-file-style: "parrot"
 * End:
 * vim: expandtab shiftwidth=4:
 */
