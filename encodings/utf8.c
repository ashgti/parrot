/* utf8.c
 *  Copyright: 2001-2003 The Perl Foundation.  All Rights Reserved.
 *  CVS Info
 *     $Id$
 *  Overview:
 *     This defines the UTF-8 encoding routines.
 *  Data Structure and Algorithms:
 *  History:
 *  Notes:
 *  References:
 */

#include "parrot/parrot.h"
#include "parrot/unicode.h"

const char Parrot_utf8skip[256] = {
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,     /* ascii */
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,     /* ascii */
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,     /* ascii */
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,     /* ascii */
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,     /* ascii */
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,     /* ascii */
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,     /* ascii */
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,     /* ascii */
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,     /* bogus */
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,     /* bogus */
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,     /* bogus */
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,     /* bogus */
    2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,     /* scripts */
    2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,     /* scripts */
    3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,     /* cjk etc. */
    4, 4, 4, 4, 4, 4, 4, 4, 5, 5, 5, 5, 6, 6, 6, 6      /* cjk etc. */
};

#if 0
typedef unsigned char utf8_t;
#endif

static UINTVAL
utf8_characters(const void *ptr, UINTVAL bytes)
{
    const utf8_t *u8ptr = ptr;
    const utf8_t *u8end = u8ptr + bytes;
    UINTVAL characters = 0;

    while (u8ptr < u8end) {
        u8ptr += UTF8SKIP(u8ptr);
        characters++;
    }

    if (u8ptr > u8end) {
        internal_exception(MALFORMED_UTF8, "Unaligned end in UTF-8 string\n");
    }

    return characters;
}

static UINTVAL
utf8_decode(const void *ptr)
{
    const utf8_t *u8ptr = ptr;
    UINTVAL c = *u8ptr;

    if (UTF8_IS_START(c)) {
        UINTVAL len = UTF8SKIP(u8ptr);
        UINTVAL count;

        c &= UTF8_START_MASK(len);
        for (count = 1; count < len; count++) {
            u8ptr++;
            if (!UTF8_IS_CONTINUATION(*u8ptr)) {
                internal_exception(MALFORMED_UTF8, "Malformed UTF-8 string\n");
            }
            c = UTF8_ACCUMULATE(c, *u8ptr);
        }

        if (UNICODE_IS_SURROGATE(c)) {
            internal_exception(MALFORMED_UTF8, "Surrogate in UTF-8 string\n");
        }
    }
    else if (!UNICODE_IS_INVARIANT(c)) {
        internal_exception(MALFORMED_UTF8, "Malformed UTF-8 string\n");
    }

    return c;
}

static void *
utf8_encode(void *ptr, UINTVAL c)
{
    utf8_t *u8ptr = (utf8_t *)ptr;
    UINTVAL len = UNISKIP(c);
    utf8_t *u8end = u8ptr + len - 1;

    if (c > 0x10FFFF || UNICODE_IS_SURROGATE(c)) {
        internal_exception(INVALID_CHARACTER,
                           "Invalid character for UTF-8 encoding\n");
    }

    while (u8end > u8ptr) {
        *u8end-- =
            (utf8_t)((c & UTF8_CONTINUATION_MASK) | UTF8_CONTINUATION_MARK);
        c >>= UTF8_ACCUMULATION_SHIFT;
    }
    *u8end = (utf8_t)((c & UTF8_START_MASK(len)) | UTF8_START_MARK(len));

    return u8ptr + len;
}

static const void *
utf8_skip_forward(const void *ptr, UINTVAL n)
{
    const utf8_t *u8ptr = (const utf8_t *)ptr;

    while (n-- > 0) {
        u8ptr += UTF8SKIP(u8ptr);
    }

    return u8ptr;
}

static const void *
utf8_skip_backward(const void *ptr, UINTVAL n)
{
    const utf8_t *u8ptr = (const utf8_t *)ptr;

    while (n-- > 0) {
        u8ptr--;
        while (UTF8_IS_CONTINUATION(*u8ptr))
            u8ptr--;
    }

    return u8ptr;
}

const ENCODING utf8_encoding = {
    enum_encoding_utf8,
    "utf8",
    UTF8_MAXLEN,
    utf8_characters,
    utf8_decode,
    utf8_encode,
    utf8_skip_forward,
    utf8_skip_backward
};

/*
 * Local variables:
 * c-indentation-style: bsd
 * c-basic-offset: 4
 * indent-tabs-mode: nil
 * End:
 *
 * vim: expandtab shiftwidth=4:
*/
