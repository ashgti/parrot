/* charset.h
 *  Copyright (C) 2004, The Perl Foundation.
 *  SVN Info
 *     $Id$
 *  Overview:
 *     This is the header for the 8-bit fixed-width encoding
 *  Data Structure and Algorithms:
 *  History:
 *  Notes:
 *  References:
 */

#if !defined(PARROT_CHARSET_H_GUARD)
#define PARROT_CHARSET_H_GUARD


#include "parrot/encoding.h"
#include "parrot/cclass.h"

struct _charset;
typedef struct _charset CHARSET;


#if !defined PARROT_NO_EXTERN_CHARSET_PTRS
PARROT_API extern CHARSET *Parrot_iso_8859_1_charset_ptr;
PARROT_API extern CHARSET *Parrot_binary_charset_ptr;
PARROT_API extern CHARSET *Parrot_default_charset_ptr;
PARROT_API extern CHARSET *Parrot_unicode_charset_ptr;
PARROT_API extern CHARSET *Parrot_ascii_charset_ptr;
#endif

#define PARROT_DEFAULT_CHARSET Parrot_ascii_charset_ptr
#define PARROT_BINARY_CHARSET Parrot_binary_charset_ptr
#define PARROT_UNICODE_CHARSET Parrot_unicode_charset_ptr

typedef STRING *(*charset_get_graphemes_t)(Interp *, STRING *source_string, UINTVAL offset, UINTVAL count);
typedef STRING *(*charset_get_graphemes_inplace_t)(Interp *, STRING *source_string, UINTVAL offset, UINTVAL count, STRING *dest_string);
typedef void (*charset_set_graphemes_t)(Interp *, STRING *source_string, UINTVAL offset, UINTVAL replace_count, STRING *insert_string);

typedef STRING * (*charset_to_charset_t)(Interp *, STRING *source_string,
        STRING *dest);
typedef STRING * (*charset_from_unicode_t)(Interp *, STRING *source_string,
        STRING *dest);
typedef STRING* (*charset_compose_t)(Interp *, STRING *source_string);
typedef STRING* (*charset_decompose_t)(Interp *, STRING *source_string);
typedef void (*charset_upcase_t)(Interp *, STRING *source_string);
typedef void (*charset_downcase_t)(Interp *, STRING *source_string);
typedef void (*charset_titlecase_t)(Interp *, STRING *source_string);
typedef void (*charset_upcase_first_t)(Interp *, STRING *source_string);
typedef void (*charset_downcase_first_t)(Interp *, STRING *source_string);
typedef void (*charset_titlecase_first_t)(Interp *, STRING *source_string);
typedef INTVAL (*charset_compare_t)(Interp *, STRING *lhs, STRING *rhs);
typedef INTVAL (*charset_index_t)(Interp *, STRING *source_string, STRING *search_string, UINTVAL offset);
typedef INTVAL (*charset_rindex_t)(Interp *, STRING *source_string, STRING *search_string, UINTVAL offset);
typedef UINTVAL (*charset_validate_t)(Interp *, STRING *source_string);
typedef INTVAL (*charset_is_cclass_t)(Interp *, INTVAL, STRING *source_string, UINTVAL offset);
typedef INTVAL (*charset_find_cclass_t)(Interp *, INTVAL, STRING *source_string, UINTVAL offset, UINTVAL count);
typedef INTVAL (*charset_find_not_cclass_t)(Interp *, INTVAL, STRING *source_string, UINTVAL offset, UINTVAL count);
typedef INTVAL (*charset_is_wordchar_t)(Interp *, STRING *source_string, UINTVAL offset);
typedef INTVAL (*charset_find_wordchar_t)(Interp *, STRING *source_string, UINTVAL offset);
typedef INTVAL (*charset_find_not_wordchar_t)(Interp *, STRING *source_string, UINTVAL offset);
typedef INTVAL (*charset_is_whitespace_t)(Interp *, STRING *source_string, UINTVAL offset);
typedef INTVAL (*charset_find_whitespace_t)(Interp *, STRING *source_string, UINTVAL offset);
typedef INTVAL (*charset_find_not_whitespace_t)(Interp *, STRING *source_string, UINTVAL offset);
typedef INTVAL (*charset_is_digit_t)(Interp *, STRING *source_string, UINTVAL offset);
typedef INTVAL (*charset_find_digit_t)(Interp *, STRING *source_string, UINTVAL offset);
typedef INTVAL (*charset_find_not_digit_t)(Interp *, STRING *source_string, UINTVAL offset);
typedef INTVAL (*charset_is_punctuation_t)(Interp *, STRING *source_string, UINTVAL offset);
typedef INTVAL (*charset_find_punctuation_t)(Interp *, STRING *source_string, UINTVAL offset);
typedef INTVAL (*charset_find_not_punctuation_t)(Interp *, STRING *source_string, UINTVAL offset);
typedef INTVAL (*charset_is_newline_t)(Interp *, STRING *source_string, UINTVAL offset);
typedef INTVAL (*charset_find_newline_t)(Interp *, STRING *source_string, UINTVAL offset);
typedef INTVAL (*charset_find_not_newline_t)(Interp *, STRING *source_string, UINTVAL offset);
typedef INTVAL (*charset_find_word_boundary_t)(Interp *, STRING *source_string, UINTVAL offset);
typedef STRING *(*charset_string_from_codepoint_t)(Interp *, UINTVAL codepoint);
typedef size_t (*charset_compute_hash_t)(Interp *, STRING *, size_t seed);

PARROT_API CHARSET *Parrot_new_charset(Interp *);
PARROT_API CHARSET *Parrot_load_charset(Interp *, const char *charsetname);
PARROT_API CHARSET *Parrot_find_charset(Interp *, const char *charsetname);
PARROT_API INTVAL Parrot_register_charset(Interp *, const char *charsetname, CHARSET *charset);
PARROT_API INTVAL Parrot_make_default_charset(Interp *, const char *charsetname, CHARSET *charset);
PARROT_API CHARSET *Parrot_default_charset(Interp *);
typedef STRING* (*charset_converter_t)(Interp *, STRING *src, STRING *dst);
PARROT_API charset_converter_t Parrot_find_charset_converter(Interp *, CHARSET *lhs, CHARSET *rhs);
PARROT_API void Parrot_register_charset_converter(Interp *,
        CHARSET *lhs, CHARSET *rhs, charset_converter_t func);

PARROT_API void Parrot_charsets_encodings_init(Interp *);
PARROT_API void Parrot_charsets_encodings_deinit(Interp *);

PARROT_API INTVAL Parrot_charset_number(Interp *, STRING *charsetname);
PARROT_API STRING* Parrot_charset_name(Interp *, INTVAL);
PARROT_API const char* Parrot_charset_c_name(Interp *, INTVAL);
PARROT_API INTVAL Parrot_charset_number_of_str(Interp *, STRING *src);
PARROT_API CHARSET* Parrot_get_charset(Interp *, INTVAL number_of_charset);

struct _charset {
    const char *name;
    charset_get_graphemes_t get_graphemes;
    charset_get_graphemes_inplace_t get_graphemes_inplace;
    charset_set_graphemes_t set_graphemes;
    charset_to_charset_t to_charset;
    charset_compose_t compose;
    charset_decompose_t decompose;
    charset_upcase_t upcase;
    charset_downcase_t downcase;
    charset_titlecase_t titlecase;
    charset_upcase_first_t upcase_first;
    charset_downcase_first_t downcase_first;
    charset_titlecase_first_t titlecase_first;
    charset_compare_t compare;
    charset_index_t index;
    charset_rindex_t rindex;
    charset_validate_t validate;
    charset_is_cclass_t is_cclass;
    charset_find_cclass_t find_cclass;
    charset_find_not_cclass_t find_not_cclass;
    charset_string_from_codepoint_t string_from_codepoint;
    charset_compute_hash_t compute_hash;
    ENCODING *preferred_encoding;
};

#define CHARSET_GET_GRAPEMES(interp, source, offset, count) ((CHARSET *)source->charset)->get_graphemes(interp, source, offset, count)
#define CHARSET_GET_GRAPHEMES_INPLACE(interp, source, dest, offset, count) ((CHARSET *)source->charset)->get_graphemes(interp, source, dest, offset, count)
#define CHARSET_SET_GRAPHEMES(interp, source, offset, replace_count, insert) ((CHARSET *)source->charset)->set_graphemes(interp, source, offset, replace_count, insert)
#define CHARSET_TO_UNICODE(interp, source, dest) ((CHARSET *)source->charset)->to_unicode(interp, source, dest)
#define CHARSET_COMPOSE(interp, source) ((CHARSET *)source->charset)->compose(interp, source)
#define CHARSET_DECOMPOSE(interp, source) ((CHARSET *)source->charset)->decompose(interp, source)
#define CHARSET_UPCASE(interp, source) ((CHARSET *)source->charset)->upcase(interp, source)
#define CHARSET_DOWNCASE(interp, source) ((CHARSET *)source->charset)->downcase(interp, source)
#define CHARSET_TITLECASE(interp, source) ((CHARSET *)source->charset)->titlecase(interp, source)
#define CHARSET_UPCASE_FIRST(interp, source) ((CHARSET *)source->charset)->upcase_first(interp, source)
#define CHARSET_DOWNCASE_FIRST(interp, source) ((CHARSET *)source->charset)->downcase_first(interp, source)
#define CHARSET_TITLECASE_FIRST(interp, source) ((CHARSET *)source->charset)->titlecase_first(interp, source)
#define CHARSET_COMPARE(interp, lhs, rhs) ((CHARSET *)lhs->charset)->compare(interp, lhs, rhs)
#define CHARSET_INDEX(interp, source, search, offset) ((CHARSET *)source->charset)->index(interp, source, search, offset)
#define CHARSET_RINDEX(interp, source, search, offset) ((CHARSET *)source->charset)->rindex(interp, source, search, offset)
#define CHARSET_VALIDATE(interp, source, offset) ((CHARSET *)source->charset)->validate(interp, source)
#define CHARSET_IS_CCLASS(interp, flags, source, offset) ((CHARSET *)source->charset)->is_cclass(interp, flags, source, offset)
#define CHARSET_FIND_CCLASS(interp, flags, source, offset, count) ((CHARSET *)source->charset)->find_cclass(interp, flags, source, offset, count)
#define CHARSET_FIND_NOT_CCLASS(interp, flags, source, offset, count) ((CHARSET *)source->charset)->find_not_cclass(interp, flags, source, offset, count)
#define CHARSET_COMPUTE_HASH(interp, source, seed) ((CHARSET *)source->charset)->compute_hash(interp, source, seed)
#define CHARSET_GET_PREFERRED_ENCODING(interp, source) ((CHARSET *)source->charset)->preferred_encoding

#define CHARSET_TO_ENCODING(interp, source) ((ENCODING *)source->encoding)->to_encoding(interp, source)
#define CHARSET_COPY_TO_ENCODING(interp, source) ((ENCODING *)source->encoding)->copy_to_encoding(interp, source)
#define CHARSET_GET_CODEPOINT(interp, source, offset) ((ENCODING *)source->encoding)->get_codepoint(interp, source, offset)
#define CHARSET_SET_CODEPOINT(interp, source, offset, codepoint) ((ENCODING *)source->encoding)->set_codepoint(interp, source, offset, codepoint)
#define CHARSET_GET_BYTE(interp, source, offset) ((ENCODING *)source->encoding)->get_byte(interp, source, offset)
#define CHARSET_SET_BYTE(interp, source, offset, value) ((ENCODING *)source->encoding)->set_byte(interp, source, offset, value)
#define CHARSET_GET_CODEPOINTS(interp, source, offset, count) ((ENCODING *)source->encoding)->get_codepoints(interp, source, offset, count)
#define CHARSET_GET_CODEPOINTS_INPLACE(interp, source, dest, offset, count) ((ENCODING *)source->encoding)->get_codepoints_inplace(interp, source, dest, offset, count)
#define CHARSET_GET_BYTES(interp, source, offset, count) ((ENCODING *)source->encoding)->get_bytes(interp, source, offset, count)
#define CHARSET_GET_BYTES_INPLACE(interp, source, offset, count, dest) ((ENCODING *)source->encoding)->get_bytes(interp, source, offset, count, dest)
#define CHARSET_SET_CODEPOINTS(interp, source, offset, count, newdata) ((ENCODING *)source->encoding)->set_codepoints(interp, source, offset, count, newdata)
#define CHARSET_SET_BYTES(interp, source, offset, count, newdata) ((ENCODING *)source->encoding)->set_bytes(interp, source, offset, count, newdata)
#define CHARSET_BECOME_ENCODING(interp, source) ((ENCODING *)source->encoding)->become_encoding(interp, source)
#define CHARSET_CODEPOINTS(interp, source) ((ENCODING *)source->encoding)->codepoints(interp, source)
#define CHARSET_BYTES(interp, source) ((ENCODING *)source->encoding)->bytes(interp, source)


#endif /* PARROT_CHARSET_H_GUARD */

/*
 * Local variables:
 *   c-file-style: "parrot"
 * End:
 * vim: expandtab shiftwidth=4:
 */
