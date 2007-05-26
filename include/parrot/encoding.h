/* encoding.h
 *  Copyright (C) 2004-2006, The Perl Foundation.
 *  SVN Info
 *     $Id$
 *  Overview:
 *     This is the header for the generic encoding functions
 *  Data Structure and Algorithms:
 *  History:
 *  Notes:
 *  References:
 */

#ifndef PARROT_ENCODING_H_GUARD
#define PARROT_ENCODING_H_GUARD

#include "parrot/parrot.h"

typedef STRING * (*encoding_to_encoding_t)(Interp*, STRING *src, STRING *dest);
typedef UINTVAL (*encoding_get_codepoint_t)(Interp*, const STRING *src, UINTVAL offset);
typedef void (*encoding_set_codepoint_t)(Interp*, STRING *src, UINTVAL offset, UINTVAL codepoint);
typedef UINTVAL (*encoding_get_byte_t)(Interp*, const STRING *src, UINTVAL offset);
typedef void (*encoding_set_byte_t)(Interp*, const STRING *src, UINTVAL offset, UINTVAL count);
typedef STRING *(*encoding_get_codepoints_t)(Interp*, STRING *src, UINTVAL offset, UINTVAL count);
typedef STRING *(*encoding_get_bytes_t)(Interp*, STRING *src, UINTVAL offset, UINTVAL count);
typedef STRING *(*encoding_get_codepoints_inplace_t)(Interp*, STRING *src, UINTVAL offset, UINTVAL count, STRING *dest_string);
typedef STRING *(*encoding_get_bytes_inplace_t)(Interp*, STRING *src, UINTVAL offset, UINTVAL count, STRING *dest_string);
typedef void (*encoding_set_codepoints_t)(Interp*, STRING *src, UINTVAL offset, UINTVAL count, STRING *new_bytes);
typedef void (*encoding_set_bytes_t)(Interp*, STRING *src, UINTVAL offset, UINTVAL count, STRING *new_bytes);
typedef void (*encoding_become_encoding_t)(Interp*, STRING *src);
typedef UINTVAL (*encoding_codepoints_t)(Interp*, STRING *src);
typedef UINTVAL (*encoding_bytes_t)(Interp*, STRING *src);

/* iterator support */

struct string_iterator_t;       /* s. parrot/string.h */

typedef void (*encoding_iter_init_t)(Interp *, STRING *src,
        struct string_iterator_t *);

struct _encoding {
    const char *name;
    UINTVAL max_bytes_per_codepoint;
    encoding_to_encoding_t to_encoding;
    encoding_get_codepoint_t get_codepoint;
    encoding_set_codepoint_t  set_codepoint;
    encoding_get_byte_t  get_byte;
    encoding_set_byte_t  set_byte;
    encoding_get_codepoints_t  get_codepoints;
    encoding_get_codepoints_inplace_t  get_codepoints_inplace;
    encoding_get_bytes_t  get_bytes;
    encoding_get_bytes_inplace_t  get_bytes_inplace;
    encoding_set_codepoints_t  set_codepoints;
    encoding_set_bytes_t  set_bytes;
    encoding_become_encoding_t  become_encoding;
    encoding_codepoints_t  codepoints;
    encoding_bytes_t  bytes;
    encoding_iter_init_t     iter_init;
};

typedef struct _encoding ENCODING;

#if !defined PARROT_NO_EXTERN_ENCODING_PTRS
PARROT_API extern ENCODING *Parrot_fixed_8_encoding_ptr;
PARROT_API extern ENCODING *Parrot_utf8_encoding_ptr;
PARROT_API extern ENCODING *Parrot_utf16_encoding_ptr;
PARROT_API extern ENCODING *Parrot_ucs2_encoding_ptr;
PARROT_API extern ENCODING *Parrot_default_encoding_ptr;
#endif

#define PARROT_DEFAULT_ENCODING Parrot_fixed_8_encoding_ptr
#define PARROT_FIXED_8_ENCODING Parrot_fixed_8_encoding_ptr
#define PARROT_DEFAULT_FOR_UNICODE_ENCODING NULL

PARROT_API ENCODING *Parrot_new_encoding(Interp*);
PARROT_API ENCODING *Parrot_load_encoding(Interp*, const char *encoding_name);
PARROT_API ENCODING *Parrot_find_encoding(Interp *, const char *encodingname);
PARROT_API INTVAL Parrot_register_encoding(Interp *, const char *encodingname, ENCODING *encoding);
PARROT_API INTVAL Parrot_make_default_encoding(Interp *, const char *encodingname, ENCODING *encoding);
PARROT_API ENCODING *Parrot_default_encoding(Interp *);
typedef INTVAL (*encoding_converter_t)(Interp *, ENCODING *lhs, ENCODING *rhs);
PARROT_API encoding_converter_t Parrot_find_encoding_converter(Interp *, ENCODING *lhs, ENCODING *rhs);

void parrot_init_encodings_2(Interp *);
void parrot_deinit_encodings(Interp *);

PARROT_API INTVAL Parrot_encoding_number(Interp *, STRING *encodingname);
PARROT_API INTVAL Parrot_encoding_number_of_str(Interp *, STRING *src);
PARROT_API STRING* Parrot_encoding_name(Interp *, INTVAL number_of_encoding);
PARROT_API ENCODING* Parrot_get_encoding(Interp *, INTVAL number_of_encoding);
PARROT_API const char * Parrot_encoding_c_name(Interp *, INTVAL number_of_encoding);

#define ENCODING_MAX_BYTES_PER_CODEPOINT(i, src) \
    ((const ENCODING *)src->encoding)->max_bytes_per_codepoint
#define ENCODING_GET_CODEPOINT(i, src, offset) \
    ((const ENCODING *)src->encoding)->get_codepoint(i, src, offset)
#define ENCODING_SET_CODEPOINT(i, src, offset, codepoint) \
    ((const ENCODING *)src->encoding)->set_codepoint(i, src, offset, codepoint)
#define ENCODING_GET_BYTE(i, src, offset) \
    ((const ENCODING *)src->encoding)->get_byte(i, src, offset)
#define ENCODING_SET_BYTE(i, src, offset, value) \
    ((const ENCODING *)src->encoding)->set_byte(i, src, offset, value)
#define ENCODING_GET_CODEPOINTS(i, src, offset, count) \
    ((const ENCODING *)src->encoding)->get_codepoints(i, src, offset, count)
#define ENCODING_GET_CODEPOINTS_INPLACE(i, src, offset, count, dest) \
    ((const ENCODING *)src->encoding)->get_codepoints_inplace(i, src, offset, count, dest)
#define ENCODING_GET_BYTES(i, src, offset, count) \
    ((const ENCODING *)src->encoding)->get_bytes(i, src, offset, count)
#define ENCODING_GET_BYTES_INPLACE(i, src, offset, count, dest) \
    ((const ENCODING *)src->encoding)->get_bytes_inplace(i, src, offset, count, dest)
#define ENCODING_SET_CODEPOINTS(i, src, offset, count, newdata) \
    ((const ENCODING *)src->encoding)->set_codepoints(i, src, offset, count, newdata)
#define ENCODING_SET_BYTES(i, src, offset, count, newdata) \
    ((const ENCODING *)src->encoding)->set_bytes(i, src, offset, count, newdata)
#define ENCODING_BECOME_ENCODING(i, src) \
    ((const ENCODING *)src->encoding)->become_encoding(i, src)
#define ENCODING_CODEPOINTS(i, src) \
    ((const ENCODING *)src->encoding)->codepoints(i, src)
#define ENCODING_BYTES(i, src) \
    ((const ENCODING *)src->encoding)->bytes(i, src)
#define ENCODING_ITER_INIT(i, src, iter) \
    ((const ENCODING *)src->encoding)->iter_init(i, src, iter)

#endif /* PARROT_ENCODING_H_GUARD */

/*
 * Local variables:
 *   c-file-style: "parrot"
 * End:
 * vim: expandtab shiftwidth=4:
 */
