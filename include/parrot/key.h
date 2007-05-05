/* key.h
 *  Copyright (C) 2001-2004, The Perl Foundation.
 *  SVN Info
 *     $Id$
 *  Overview:
 *     This is the api header for the pmc subsystem
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

PARROT_API PMC *key_new(Interp *interp);
PARROT_API PMC *key_new_integer(Interp *interp, INTVAL value);
PARROT_API PMC *key_new_number(Interp *interp, FLOATVAL value);
PARROT_API PMC *key_new_string(Interp *interp, STRING *value);
PARROT_API PMC *key_new_cstring(Interp *interp, const char *value);
PARROT_API PMC *key_new_pmc(Interp *interp, PMC *value);

PARROT_API void key_set_integer(Interp *interp, PMC *key, INTVAL value);
PARROT_API void key_set_number(Interp *interp, PMC *key, FLOATVAL value);
PARROT_API void key_set_register(Interp *interp, PMC *key, INTVAL value,
        INTVAL flag);
PARROT_API void key_set_string(Interp *interp, PMC *key, STRING *value);
PARROT_API void key_set_pmc(Interp *interp, PMC *key, PMC *value);

PARROT_API INTVAL key_type(Interp *interp, PMC *key);
PARROT_API INTVAL key_integer(Interp *interp, PMC *key);
PARROT_API FLOATVAL key_number(Interp *interp, PMC *key);
PARROT_API STRING *key_string(Interp *interp, PMC *key);
PARROT_API PMC *key_pmc(Interp *interp, PMC *key);
PARROT_API PMC *key_next(Interp *interp, PMC *key);

PARROT_API PMC *key_append(Interp *interp, PMC *key1, PMC *key2);

PARROT_API void key_mark(Interp *interp, PMC *key);

PARROT_API STRING *key_set_to_string(Interp *interpreter, PMC *key);

#endif /* PARROT_KEY_H_GUARD */

/*
 * Local variables:
 *   c-file-style: "parrot"
 * End:
 * vim: expandtab shiftwidth=4:
 */
