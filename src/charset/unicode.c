/*
Copyright: 2005 The Perl Foundation.  All Rights Reserved.
$Id$

=head1 NAME

src/charset/unicode.c

=head1 DESCRIPTION

This file implements the charset functions for unicode data

=cut

*/

#include "parrot/parrot.h"
#include "unicode.h"
#include "ascii.h"
#include "tables.h"

#ifdef EXCEPTION
#  undef EXCEPTION
#endif

#if PARROT_HAS_ICU
#include <unicode/ucnv.h>
#include <unicode/utypes.h>
#include <unicode/uchar.h>
#include <unicode/ustring.h>
#include <unicode/unorm.h>
#endif
#define EXCEPTION(err, str) \
    real_exception(interpreter, NULL, err, str)

#define UNIMPL EXCEPTION(UNIMPLEMENTED, "unimplemented unicode")

static void
set_graphemes(Interp *interpreter, STRING *source_string,
        UINTVAL offset, UINTVAL replace_count, STRING *insert_string)
{
    ENCODING_SET_CODEPOINTS(interpreter, source_string, offset,
            replace_count, insert_string);
}

static STRING *
get_graphemes(Interp *interpreter, STRING *source_string,
        UINTVAL offset, UINTVAL count)
{
    return ENCODING_GET_CODEPOINTS(interpreter, source_string, offset, count);
}

static STRING *
get_graphemes_inplace(Interp *interpreter, STRING *source_string,
        UINTVAL offset, UINTVAL count, STRING *dest_string)
{
    return ENCODING_GET_CODEPOINTS_INPLACE(interpreter, source_string,
            offset, count, dest_string);
}

static STRING*
to_charset(Interp *interpreter, STRING *src, STRING *dest)
{
    charset_converter_t conversion_func;

    if ((conversion_func = Parrot_find_charset_converter(interpreter,
                    src->charset, Parrot_unicode_charset_ptr))) {
         return conversion_func(interpreter, src, dest);
    }
    return Parrot_utf8_encoding_ptr->to_encoding(interpreter, src, dest);
}


static STRING*
compose(Interp *interpreter, STRING *src)
{
#if PARROT_HAS_ICU
    STRING *dest;
    int src_len, dest_len;
    UErrorCode err;
    /*
       U_STABLE int32_t U_EXPORT2 
       unorm_normalize(const UChar *source, int32_t sourceLength,
       UNormalizationMode mode, int32_t options,
       UChar *result, int32_t resultLength,
       UErrorCode *status);
       */
    dest_len = src_len = src->strlen;
    dest = string_make_direct(interpreter, NULL, src_len * sizeof(UChar),
            src->encoding, src->charset, 0);
    err = U_ZERO_ERROR;
    dest_len = unorm_normalize(src->strstart, src_len,
            UNORM_DEFAULT,      /* default is NFC */
            0,                  /* options 0 default - no specific icu version */
            dest->strstart, dest_len,
            &err);
    dest->bufused = dest_len * sizeof(UChar);
    if (!U_SUCCESS(err)) {
        err = U_ZERO_ERROR;
        Parrot_reallocate_string(interpreter, dest, dest->bufused);
        dest_len = unorm_normalize(src->strstart, src_len,
                UNORM_DEFAULT,      /* default is NFC */
                0,                  /* options 0 default - no specific icu version */
                dest->strstart, dest_len,
                &err);
        assert(U_SUCCESS(err));
        dest->bufused = dest_len * sizeof(UChar);
    }
    dest->strlen = dest_len;
    return dest;
#else
    real_exception(interpreter, NULL, E_LibraryNotLoadedError,
            "no ICU lib loaded");
    return NULL;
#endif
}

static STRING*
decompose(Interp *interpreter, STRING *src)
{
    UNIMPL;
    return NULL;
}

static void
upcase(Interp *interpreter, STRING *src)
{
#if PARROT_HAS_ICU

    UErrorCode err;
    int dest_len, src_len, needed;

    src = Parrot_utf16_encoding_ptr->to_encoding(interpreter, src, NULL);
    /*
       U_CAPI int32_t U_EXPORT2
       u_strToUpper(UChar *dest, int32_t destCapacity,
       const UChar *src, int32_t srcLength,
       const char *locale,
       UErrorCode *pErrorCode);
       */
    err = U_ZERO_ERROR;
    /* use all available space - see below XXX */
    /* TODO downcase, titlecase too */
    dest_len = PObj_buflen(src) / sizeof(UChar);
    src_len = src->bufused / sizeof(UChar);
    /*
     * XXX troubles:
     *   t/op/string_cs_45  upcase unicode:"\u01f0"
     *   this creates \u004a \u030c J+NON-SPACING HACEK
     *   the string needs resizing, *if* the src buffer is
     *   too short. *But* with icu 3.2/3.4 the src string is
     *   overwritten with partial result, despite the icu docs sayeth:
     *
     *      The source string and the destination buffer
     *      are allowed to overlap.
     *
     *  Workaround:  'preflighting' returns needed length
     *  Alternative: forget about inplace operation - create new result
     *
     *  TODO downcase, titlecase
     */
    needed = u_strToUpper(NULL, 0,
            src->strstart, src_len,
            NULL,       /* locale = default */
            &err);
    if (needed > dest_len) {
        Parrot_reallocate_string(interpreter, src, needed * sizeof(UChar));
        dest_len = needed;
    }
    err = U_ZERO_ERROR;
    dest_len = u_strToUpper(src->strstart, dest_len,
            src->strstart, src_len,
            NULL,       /* locale = default */
            &err);
    assert(U_SUCCESS(err));
    src->bufused = dest_len * sizeof(UChar);
    /* downgrade if possible */
    if (dest_len == (int)src->strlen)
        src->encoding = Parrot_ucs2_encoding_ptr;
    else {
        /* string is likely still ucs2 if it was earlier
         * but strlen changed tue to combining char
         */
        src->strlen = dest_len;
    }
#else
    real_exception(interpreter, NULL, E_LibraryNotLoadedError,
            "no ICU lib loaded");
#endif
}

static void
downcase(Interp *interpreter, STRING *src)
{
#if PARROT_HAS_ICU

    UErrorCode err;
    int dest_len, src_len;

    src = Parrot_utf16_encoding_ptr->to_encoding(interpreter, src, NULL);
    /*
U_CAPI int32_t U_EXPORT2
u_strToLower(UChar *dest, int32_t destCapacity,
             const UChar *src, int32_t srcLength,
             const char *locale,
             UErrorCode *pErrorCode);
     */
    err = U_ZERO_ERROR;
    src_len = src->bufused / sizeof(UChar);
    dest_len = u_strToLower(src->strstart, src_len,
            src->strstart, src_len,
            NULL,       /* locale = default */
            &err);
    src->bufused = dest_len * sizeof(UChar);
    if (!U_SUCCESS(err)) {
        err = U_ZERO_ERROR;
        Parrot_reallocate_string(interpreter, src, src->bufused);
        dest_len = u_strToLower(src->strstart, dest_len,
                src->strstart, src_len,
                NULL,       /* locale = default */
                &err);
        assert(U_SUCCESS(err));
    }
    /* downgrade if possible */
    if (dest_len == (int)src->strlen)
        src->encoding = Parrot_ucs2_encoding_ptr;
#else
    real_exception(interpreter, NULL, E_LibraryNotLoadedError,
            "no ICU lib loaded");
#endif
}

static void
titlecase(Interp *interpreter, STRING *src)
{
#if PARROT_HAS_ICU

    UErrorCode err;
    int dest_len, src_len;

    src = Parrot_utf16_encoding_ptr->to_encoding(interpreter, src, NULL);
    /*
U_CAPI int32_t U_EXPORT2
u_strToTitle(UChar *dest, int32_t destCapacity,
             const UChar *src, int32_t srcLength,
             UBreakIterator *titleIter,
             const char *locale,
             UErrorCode *pErrorCode);
     */
    err = U_ZERO_ERROR;
    src_len = src->bufused / sizeof(UChar);
    dest_len = u_strToTitle(src->strstart, src_len,
            src->strstart, src_len,
            NULL,       /* default titleiter */
            NULL,       /* locale = default */
            &err);
    src->bufused = dest_len * sizeof(UChar);
    if (!U_SUCCESS(err)) {
        err = U_ZERO_ERROR;
        Parrot_reallocate_string(interpreter, src, src->bufused);
        dest_len = u_strToTitle(src->strstart, dest_len,
                src->strstart, src_len,
                NULL, NULL,
                &err);
        assert(U_SUCCESS(err));
    }
    /* downgrade if possible */
    if (dest_len == (int)src->strlen)
        src->encoding = Parrot_ucs2_encoding_ptr;
#else
    real_exception(interpreter, NULL, E_LibraryNotLoadedError,
            "no ICU lib loaded");
#endif
}

static void
upcase_first(Interp *interpreter, STRING *source_string)
{
    UNIMPL;
}

static void
downcase_first(Interp *interpreter, STRING *source_string)
{
    UNIMPL;
}

static void
titlecase_first(Interp *interpreter, STRING *source_string)
{
    UNIMPL;
}

static INTVAL
compare(Interp *interpreter, STRING *lhs, STRING *rhs)
{
    String_iter l_iter, r_iter;
    UINTVAL offs, cl, cr, min_len, l_len, r_len;

    /* TODO make optimized equal - strings are equal length then already */
    ENCODING_ITER_INIT(interpreter, lhs, &l_iter);
    ENCODING_ITER_INIT(interpreter, rhs, &r_iter);
    l_len = lhs->strlen;
    r_len = rhs->strlen;
    min_len = l_len > r_len ? r_len : l_len;
    for (offs = 0; offs < min_len; ++offs) {
        cl = l_iter.get_and_advance(interpreter, &l_iter);
        cr = r_iter.get_and_advance(interpreter, &r_iter);
        if (cl != cr)
            return cl < cr ? -1 : 1;
    }
    if (l_len < r_len) {
        return -1;
    }
    if (l_len > r_len) {
        return 1;
    }
    return 0;
}


static INTVAL
cs_rindex(Interp *interpreter, STRING *source_string,
        STRING *search_string, UINTVAL offset)
{
    UNIMPL;
    return 0;
}

static UINTVAL
validate(Interp *interpreter, STRING *src)
{
    UINTVAL codepoint, offset;

    for (offset = 0; offset < string_length(interpreter, src); ++offset) {
        codepoint = ENCODING_GET_CODEPOINT(interpreter, src, offset);
        /* Check for Unicode non-characters */
        if (codepoint >= 0xfdd0 &&
            (codepoint <= 0xfdef || (codepoint & 0xfffe) == 0xfffe) &&
                codepoint <= 0x10ffff)
                    return 0;
    }
    return 1;
}

#if PARROT_HAS_ICU
static int
is_foo(Interp *interpreter, UINTVAL codepoint, int bit)
{
    switch (bit) {
        case enum_cclass_uppercase:
            return u_isupper(codepoint);
            break;
        case enum_cclass_lowercase:
            return u_islower(codepoint);
            break;
        case enum_cclass_alphabetic:
            return u_isalpha(codepoint);
            break;
        case enum_cclass_numeric:
            return u_isdigit(codepoint);
            /* XXX which one
               return u_charDigitValue(codepoint);
               */
            break;
        case enum_cclass_hexadecimal:
            return u_isxdigit(codepoint);
            break;
        case enum_cclass_whitespace:
            return u_isspace(codepoint);
            break;
        case enum_cclass_printing:
            return u_isprint(codepoint);
            break;
        case enum_cclass_graphical:
            return u_isgraph(codepoint);
            break;
        case enum_cclass_blank:
            return u_isblank(codepoint);
            break;
        case enum_cclass_control:
            return u_iscntrl(codepoint);
            break;
        case enum_cclass_alphanumeric:
            return u_isalnum(codepoint);
            break;
        case enum_cclass_word:
            return u_isalnum(codepoint) || codepoint == '_';
            break;
        default:
            UNIMPL;
    }
    return 0;
}
#endif

static INTVAL
is_cclass(Interp *interpreter, PARROT_CCLASS_FLAGS flags, STRING *source_string, UINTVAL offset)
{
    UINTVAL codepoint;
#if PARROT_HAS_ICU
    int bit, mask;
#endif

    if (offset >= source_string->strlen)
        return 0;
    codepoint = ENCODING_GET_CODEPOINT(interpreter, source_string, offset);
    if (codepoint >= 256) {
#if PARROT_HAS_ICU
        for (mask = enum_cclass_uppercase;
                mask <= enum_cclass_word ; mask <<= 1) {
            bit = mask & flags;
            if (!bit)
                continue;
            if (is_foo(interpreter, codepoint, bit))
                return 1;
        }
        return 0;
#else
        real_exception(interpreter, NULL, E_LibraryNotLoadedError,
                "no ICU lib loaded");
#endif
    }
    return (Parrot_iso_8859_1_typetable[codepoint] & flags) ? 1 : 0;
}

static INTVAL
find_cclass(Interp *interpreter, PARROT_CCLASS_FLAGS flags, STRING *source_string, UINTVAL offset, UINTVAL count)
{
    UINTVAL pos = offset;
    UINTVAL end = offset + count;
    UINTVAL codepoint;
#if PARROT_HAS_ICU
    int bit, mask;
#endif

    assert(source_string != 0);
    end = source_string->strlen < end ? source_string->strlen : end;
    for (; pos < end; ++pos) {
        codepoint = ENCODING_GET_CODEPOINT(interpreter, source_string, pos);
        if (codepoint >= 256) {
#if PARROT_HAS_ICU
            for (mask = enum_cclass_uppercase;
                    mask <= enum_cclass_word ; mask <<= 1) {
                bit = mask & flags;
                if (!bit)
                    continue;
                if (is_foo(interpreter, codepoint, bit))
                    return pos;
            }
#else
            real_exception(interpreter, NULL, E_LibraryNotLoadedError,
                    "no ICU lib loaded");
#endif
        }
        else {
            if (Parrot_iso_8859_1_typetable[codepoint] & flags) {
                return pos;
            }
        }
    }
    return end;
}

static INTVAL
find_not_cclass(Interp *interpreter, PARROT_CCLASS_FLAGS flags, STRING *source_string, UINTVAL offset, UINTVAL count)
{
    UINTVAL pos = offset;
    UINTVAL end = offset + count;
    UINTVAL codepoint;
#if PARROT_HAS_ICU
    int bit, mask;
#endif

    assert(source_string != 0);
    end = source_string->strlen < end ? source_string->strlen : end;
    for (; pos < end; ++pos) {
        codepoint = ENCODING_GET_CODEPOINT(interpreter, source_string, pos);
        if (codepoint >= 256) {
#if PARROT_HAS_ICU
            for (mask = enum_cclass_uppercase;
                    mask <= enum_cclass_word ; mask <<= 1) {
                bit = mask & flags;
                if (!bit)
                    continue;
                if (!is_foo(interpreter, codepoint, bit))
                    return pos;
            }
#else
            real_exception(interpreter, NULL, E_LibraryNotLoadedError,
                    "no ICU lib loaded");
#endif
        }
        else {
            if (!(Parrot_iso_8859_1_typetable[codepoint] & flags)) {
                return pos;
            }
        }
    }
    return end;
}

static STRING *
string_from_codepoint(Interp *interpreter, UINTVAL codepoint)
{
    STRING *dest;
    String_iter iter;

    dest = string_make(interpreter, "", 1, "unicode", 0);
    dest->strlen = 1;
    ENCODING_ITER_INIT(interpreter, dest, &iter);
    iter.set_and_advance(interpreter, &iter, codepoint);
    dest->bufused = iter.bytepos;

    return dest;
}

static size_t
compute_hash(Interp *interpreter, STRING *src, size_t seed)
{
    String_iter iter;
    size_t hashval = seed;
    UINTVAL offs, c;

    ENCODING_ITER_INIT(interpreter, src, &iter);
    for (offs = 0; offs < src->strlen; ++offs) {
        c = iter.get_and_advance(interpreter, &iter);
        hashval += hashval << 5;
        hashval += c;
    }
    return hashval;
}

CHARSET *
Parrot_charset_unicode_init(Interp *interpreter)
{
    CHARSET *return_set = Parrot_new_charset(interpreter);
    static const CHARSET base_set = {
        "unicode",
        get_graphemes,
        get_graphemes_inplace,
        set_graphemes,
        to_charset,
        compose,
        decompose,
        upcase,
        downcase,
        titlecase,
        upcase_first,
        downcase_first,
        titlecase_first,
        compare,
        mixed_cs_index,
        cs_rindex,
        validate,
        is_cclass,
        find_cclass,
        find_not_cclass,
        string_from_codepoint,
        compute_hash,
        NULL
    };

    memcpy(return_set, &base_set, sizeof(CHARSET));
    /*
     * for now use utf8
     * TODO replace it with a fixed uint_16 or uint_32 encoding
     */
    return_set->preferred_encoding = Parrot_utf8_encoding_ptr;
    Parrot_register_charset(interpreter, "unicode", return_set);
    return return_set;
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
