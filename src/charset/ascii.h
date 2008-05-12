/* ascii.h
 *  Copyright (C) 2004-2007, The Perl Foundation.
 *  SVN Info
 *     $Id$
 *  Overview:
 *     This is the header for the ascii charset functions
 *  Data Structure and Algorithms:
 *  History:
 *  Notes:
 *  References:
 */

#ifndef PARROT_CHARSET_ASCII_H_GUARD
#define PARROT_CHARSET_ASCII_H_GUARD

/*
 * common functions for ascii-ish charsets
 */

/* HEADERIZER BEGIN: src/charset/ascii.c */
/* Don't modify between HEADERIZER BEGIN / HEADERIZER END.  Your changes will be lost. */

PARROT_WARN_UNUSED_RESULT
INTVAL ascii_compare(PARROT_INTERP,
    ARGIN(const STRING *lhs),
    ARGIN(const STRING *rhs))
        __attribute__nonnull__(1)
        __attribute__nonnull__(2)
        __attribute__nonnull__(3);

PARROT_PURE_FUNCTION
size_t ascii_compute_hash(SHIM_INTERP,
    ARGIN(const STRING *source_string),
    size_t seed)
        __attribute__nonnull__(2);

PARROT_WARN_UNUSED_RESULT
INTVAL ascii_cs_index(PARROT_INTERP,
    ARGIN(STRING *source_string),
    ARGIN(STRING *search_string),
    UINTVAL offset)
        __attribute__nonnull__(1)
        __attribute__nonnull__(2)
        __attribute__nonnull__(3);

PARROT_WARN_UNUSED_RESULT
INTVAL ascii_cs_rindex(PARROT_INTERP,
    ARGIN(STRING *source_string),
    ARGIN(STRING *search_string),
    UINTVAL offset)
        __attribute__nonnull__(1)
        __attribute__nonnull__(2)
        __attribute__nonnull__(3);

PARROT_CANNOT_RETURN_NULL
PARROT_WARN_UNUSED_RESULT
STRING * ascii_get_graphemes(PARROT_INTERP,
    ARGIN(STRING *source_string),
    UINTVAL offset,
    UINTVAL count)
        __attribute__nonnull__(1)
        __attribute__nonnull__(2);

PARROT_WARN_UNUSED_RESULT
PARROT_CANNOT_RETURN_NULL
STRING * ascii_get_graphemes_inplace(PARROT_INTERP,
    ARGIN(STRING *source_string),
    UINTVAL offset,
    UINTVAL count,
    ARGMOD(STRING *dest_string))
        __attribute__nonnull__(1)
        __attribute__nonnull__(2)
        __attribute__nonnull__(5)
        FUNC_MODIFIES(*dest_string);

PARROT_CANNOT_RETURN_NULL
STRING * charset_cvt_ascii_to_binary(PARROT_INTERP,
    ARGIN(STRING *src),
    ARGIN_NULLOK(STRING *dest))
        __attribute__nonnull__(1)
        __attribute__nonnull__(2);

PARROT_CANNOT_RETURN_NULL
STRING * charset_cvt_ascii_to_iso_8859_1(PARROT_INTERP,
    ARGIN(STRING *src),
    ARGIN_NULLOK(STRING *dest))
        __attribute__nonnull__(1)
        __attribute__nonnull__(2);

PARROT_WARN_UNUSED_RESULT
INTVAL mixed_cs_index(PARROT_INTERP,
    ARGIN(STRING *src),
    ARGIN(STRING *search),
    UINTVAL offs)
        __attribute__nonnull__(1)
        __attribute__nonnull__(2)
        __attribute__nonnull__(3);

PARROT_CANNOT_RETURN_NULL
const CHARSET * Parrot_charset_ascii_init(PARROT_INTERP)
        __attribute__nonnull__(1);

/* Don't modify between HEADERIZER BEGIN / HEADERIZER END.  Your changes will be lost. */
/* HEADERIZER END: src/charset/ascii.c */

#endif /* PARROT_CHARSET_ASCII_H_GUARD */

/*
 * Local variables:
 *   c-file-style: "parrot"
 * End:
 * vim: expandtab shiftwidth=4:
 */
