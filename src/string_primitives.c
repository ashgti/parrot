/*
Copyright (C) 2006, The Perl Foundation.
$Id$

=head1 NAME

src/string_primitives.c - String Primitives

=head1 DESCRIPTION

This file collects together all the functions that call into the ICU
API.

=head2 Functions

=over 4

=cut

*/

#include "parrot/parrot.h"
#if PARROT_HAS_ICU
#include <unicode/ucnv.h>
#include <unicode/utypes.h>
#include <unicode/uchar.h>
#include <unicode/ustring.h>
#else
#include <ctype.h>
#endif
#include <assert.h>

/*

=item C<void
string_set_data_directory(const char *dir)>

Set the directory where ICU finds its data files (encodings, locales,
etc.).

=cut

*/

void
string_set_data_directory(const char *dir)
{
#if PARROT_HAS_ICU
    u_setDataDirectory(dir);

    /* Since u_setDataDirectory doesn't have a result code, we'll spot
       check that everything is okay by making sure that '9' had decimal
       value 9. Using 57 rather than '9' so that the encoding of this
       source code file isn't an issue.... (Don't want to get bitten by
       EBCDIC.) */

    if (!u_isdigit(57) || (u_charDigitValue(57) != 9)) {
            internal_exception(ICU_ERROR,
                "string_set_data_directory: ICU data files not found"
                "(apparently) for directory [%s]", dir);
    }
#else
    internal_exception(ICU_ERROR,
        "string_set_data_directory: parrot compiled without ICU support" );
#endif
}

/*

=item C<void
string_fill_from_buffer(Interp *interpreter, const void *buffer,
            UINTVAL len, const char *encoding_name, STRING *s)>

Creates a Parrot string from an "external" buffer, converting from any
supported encoding into Parrot string's internal format.

=cut

*/

void
string_fill_from_buffer(Interp *interpreter, const void *buffer,
            UINTVAL len, const char *encoding_name, STRING *s)
{
#if PARROT_HAS_ICU
    UErrorCode icuError = U_ZERO_ERROR;
    UConverter *conv = NULL;
    UChar *target = NULL;
    UChar *target_limit = NULL;
    const char *source = NULL;
    const char *source_limit = NULL;

    assert(buffer); assert(encoding_name);

    if (s && !len) {
        /* XXX: I _guess_ this is always an empty string--is that right? */
        s->bufused = 0;
        s->strlen = 0;
        return;
    }

    /* big guess--allocate same space for string as buffer needed.
       may be able to make a more educated guess based on the encoding. */
    Parrot_allocate_string(interpreter, s, len);

    conv = ucnv_open(encoding_name, &icuError);

    if (!conv || icuError != U_ZERO_ERROR) {
        /* unknown encoding??? */
        internal_exception(ICU_ERROR,
                "string_fill_from_buffer: ICU error from ucnv_open()");

    }

    target = s->strstart;
    /* buflen may be larger than what we asked for,
     * so take advantage of the space
     */
    target_limit = (UChar *)((char *)PObj_bufstart(s) + PObj_buflen(s) - 1);
    source = buffer;
    source_limit = source + len;

    ucnv_toUnicode(conv, &target, target_limit, &source,
            source_limit, NULL, TRUE, &icuError);

    while (icuError == U_BUFFER_OVERFLOW_ERROR) {
        const size_t consumed_length = (char *)target - (char *)(s->strstart);

        /* double size, at least */
        Parrot_reallocate_string(interpreter, s, 2 * PObj_buflen(s));

        target = (UChar *)((char *)s->strstart + consumed_length);
        target_limit = (UChar *)((char *)PObj_bufstart(s) + PObj_buflen(s) - 1);

        icuError = U_ZERO_ERROR;
        ucnv_toUnicode(conv, &target, target_limit, &source,
                source_limit, NULL, TRUE, &icuError);
    }

    ucnv_close(conv);

    if (icuError != U_ZERO_ERROR) {
        /* handle error */
        internal_exception(ICU_ERROR,
                "string_fill_from_buffer: ICU error from ucnv_toUnicode()");
    }

    internal_exception(UNIMPLEMENTED, "Can't do unicode yet");

    /* temporary; need to promote to rep 4 if has non-BMP characters*/
    s->bufused = (char *)target - (char *)s->strstart;
    string_compute_strlen(interpreter, s);
#else
    internal_exception(ICU_ERROR,
        "string_fill_from_buffer: parrot compiled without ICU support" );
#endif
}


/* Unescape a single character. We assume that we're at the start of a
   sequence, right after the \ */
Parrot_UInt4
string_unescape_one(Interp *interpreter, UINTVAL *offset,
        STRING *string)
{
    UINTVAL workchar = 0;
    UINTVAL charcount = 0;
    const UINTVAL len = string_length(interpreter, string);
    /* Well, not right now */
    UINTVAL codepoint = CHARSET_GET_BYTE(interpreter, string, *offset);
    ++*offset;
    switch (codepoint) {
        case 'x':
            codepoint = CHARSET_GET_BYTE(interpreter, string, *offset);
            if (codepoint >= '0' && codepoint <= '9') {
                workchar = codepoint - '0';
            } else if (codepoint >= 'a' && codepoint <= 'f') {
                workchar = codepoint - 'a' + 10;
            } else if (codepoint >= 'A' && codepoint <= 'F') {
                workchar = codepoint - 'A' + 10;
            } else if (codepoint == '{') {
                int i;
                ++*offset;
                workchar = 0;
                for (i = 0; i < 8 && *offset < len; ++i, ++*offset) {
                    codepoint = CHARSET_GET_BYTE(interpreter, string, *offset);
                    if (codepoint == '}') {
                        ++*offset;
                        return workchar;
                    }
                    workchar *= 16;
                    if (codepoint >= '0' && codepoint <= '9') {
                        workchar += codepoint - '0';
                    } else if (codepoint >= 'a' && codepoint <= 'f') {
                        workchar += codepoint - 'a' + 10;
                    } else if (codepoint >= 'A' && codepoint <= 'F') {
                        workchar += codepoint - 'A' + 10;
                    } else {
                        internal_exception(UNIMPLEMENTED,
                                "Illegal escape sequence inside {}");
                    }
                }
                if (*offset == len)
                    internal_exception(UNIMPLEMENTED,
                            "Illegal escape sequence no '}'");
            } else {
                internal_exception(UNIMPLEMENTED, "Illegal escape sequence in");
            }
            ++*offset;
            if (*offset < len) {
                workchar *= 16;
                codepoint = CHARSET_GET_BYTE(interpreter, string, *offset);
                if (codepoint >= '0' && codepoint <= '9') {
                    workchar += codepoint - '0';
                } else if (codepoint >= 'a' && codepoint <= 'f') {
                    workchar += codepoint - 'a' + 10;
                } else if (codepoint >= 'A' && codepoint <= 'F') {
                    workchar += codepoint - 'A' + 10;
                } else {
                    return workchar;
                }
            } else {
                return workchar;
            }
            ++*offset;
            return workchar;
        case 'c':
            codepoint = CHARSET_GET_BYTE(interpreter, string, *offset);
            if (codepoint >= 'A' && codepoint <= 'Z') {
                workchar = codepoint - 'A' + 1;
            } else {
                internal_exception(UNIMPLEMENTED, "Illegal escape sequence");
            }
            ++*offset;
            return workchar;
        case 'u':
            workchar = 0;
            for (charcount = 0; charcount < 4; charcount++) {
                if (*offset < len) {
                    workchar *= 16;
                    codepoint = CHARSET_GET_BYTE(interpreter, string, *offset);
                    if (codepoint >= '0' && codepoint <= '9') {
                        workchar += codepoint - '0';
                    } else if (codepoint >= 'a' && codepoint <= 'f') {
                        workchar += codepoint - 'a' + 10;
                    } else if (codepoint >= 'A' && codepoint <= 'F') {
                        workchar += codepoint - 'A' + 10;
                    } else {
                        internal_exception(UNIMPLEMENTED,
                                "Illegal escape sequence in uxxx escape");
                    }
                } else {
                    internal_exception(UNIMPLEMENTED,
                        "Illegal escape sequence in uxxx escape - too short");
                }
                ++*offset;
            }
            return workchar;
        case 'U':
            workchar = 0;
            for (charcount = 0; charcount < 8; charcount++) {
                if (*offset < len) {
                    workchar *= 16;
                    codepoint = CHARSET_GET_BYTE(interpreter, string, *offset);
                    if (codepoint >= '0' && codepoint <= '9') {
                        workchar += codepoint - '0';
                    } else if (codepoint >= 'a' && codepoint <= 'f') {
                        workchar += codepoint - 'a' + 10;
                    } else if (codepoint >= 'A' && codepoint <= 'F') {
                        workchar += codepoint - 'A' + 10;
                    } else {
                        internal_exception(UNIMPLEMENTED,
                                "Illegal escape sequence in Uxxx escape");
                    }
                } else {
                    internal_exception(UNIMPLEMENTED,
                        "Illegal escape sequence in uxxx escape - too short");
                }
                ++*offset;
            }
            return workchar;
        case '0':
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
            workchar = codepoint - '0';
            if (*offset < len) {
                workchar *= 8;
                codepoint = CHARSET_GET_BYTE(interpreter, string, *offset);
                if (codepoint >= '0' && codepoint <= '7') {
                    workchar += codepoint - '0';
                } else {
                    return workchar;
                }
            } else {
                return workchar;
            }
            ++*offset;
            if (*offset < len) {
                workchar *= 8;
                codepoint = CHARSET_GET_BYTE(interpreter, string, *offset);
                if (codepoint >= '0' && codepoint <= '7') {
                    workchar += codepoint - '0';
                } else {
                    return workchar;
                }
            } else {
                return workchar;
            }
            ++*offset;
            return workchar;
        case 'a':
            return 7; /* bell */
        case 'b':
            return 8; /* bs */
        case 't':
            return 9;
        case 'n':
            return 10;
        case 'v':
            return 11;
        case 'f':
            return 12;
        case 'r':
            return 13;
        case 'e':
            return 27;
        case 92: /* \ */
            return 92;
        case '"':
            return '"';
    }

    return codepoint;  /* any not special return the char */ 
}

/*

=back

=head2 Character Property Functions

=over

=item C<UINTVAL
Parrot_char_digit_value(Interp *interpreter, UINTVAL character)>

Returns the decimal digit value of the specified character if it is a decimal
digit character. If not, then -1 is returned.

Note that as currently written, C<Parrot_char_digit_value()> can
correctly return the decimal digit value of characters for which
C<Parrot_char_is_digit()> returns false.

=cut

*/

UINTVAL
Parrot_char_digit_value(Interp *interpreter, UINTVAL character)
{
#if PARROT_HAS_ICU
    return u_charDigitValue(character);
#else
    if ((character >= 0x30) || (character <= 0x39))
        return character - 0x30;
    return -1;
#endif
}

/*

=item C<INTVAL
Parrot_char_is_alnum(Interp *interpreter, UINTVAL character)>

Returns whether the specified character is an alphanumeric character.

=cut

*/

INTVAL
Parrot_char_is_alnum(Interp *interpreter, UINTVAL character)
{
#if PARROT_HAS_ICU
    return u_isalnum(character);
#else
    return isalnum(character);
#endif
}

/*

=item C<INTVAL
Parrot_char_is_alpha(Interp *interpreter, UINTVAL character)>

Returns whether the specified character is an letter character.

=cut

*/

INTVAL
Parrot_char_is_alpha(Interp *interpreter, UINTVAL character)
{
#if PARROT_HAS_ICU
    return u_isalpha(character);
#else
    return isalpha(character);
#endif
}

/*

=item C<INTVAL
Parrot_char_is_ascii(Interp *interpreter, UINTVAL character)>

Returns whether the specified character is an ASCII character.

=cut

*/

INTVAL
Parrot_char_is_ascii(Interp *interpreter, UINTVAL character)
{
    return character < 128;
}

/*

=item C<INTVAL
Parrot_char_is_blank(Interp *interpreter, UINTVAL character)>

Returns whether the specified character is a "blank" or "horizontal
space", a character that visibly separates words on a line.

=cut

*/

INTVAL
Parrot_char_is_blank(Interp *interpreter, UINTVAL character)
{
#if PARROT_HAS_ICU
    return u_isblank(character);
#else
    return (character == 0x20) || (character == 0x09);
#endif
}

/*

=item C<INTVAL
Parrot_char_is_cntrl(Interp *interpreter, UINTVAL character)>

Returns whether the specified character is a control character.

=cut

*/

INTVAL
Parrot_char_is_cntrl(Interp *interpreter, UINTVAL character)
{
#if PARROT_HAS_ICU
    return u_iscntrl(character);
#else
    return iscntrl(character);
#endif
}

/*

=item C<INTVAL
Parrot_char_is_digit(Interp *interpreter, UINTVAL character)>

Returns whether the specified character is a digit character.

=cut

*/

INTVAL
Parrot_char_is_digit(Interp *interpreter, UINTVAL character)
{
#if PARROT_HAS_ICU
    return u_isdigit(character);
#else
    return isdigit(character);
#endif
}

/*

=item C<INTVAL
Parrot_char_is_graph(Interp *interpreter, UINTVAL character)>

Returns whether the specified character is a a "graphic" character
(printable, excluding spaces).

=cut

*/

INTVAL
Parrot_char_is_graph(Interp *interpreter, UINTVAL character)
{
#if PARROT_HAS_ICU
	return u_isgraph(character);
#else
    return isgraph(character);
#endif
}

/*

=item C<INTVAL
Parrot_char_is_lower(Interp *interpreter, UINTVAL character)>

Returns whether the specified character is a lowercase letter.

=cut

*/

INTVAL
Parrot_char_is_lower(Interp *interpreter, UINTVAL character)
{
#if PARROT_HAS_ICU
    return u_islower(character);
#else
    return islower(character);
#endif
}

/*

=item C<INTVAL
Parrot_char_is_print(Interp *interpreter, UINTVAL character)>

Returns whether the specified character is a printable character.

=cut

*/

INTVAL
Parrot_char_is_print(Interp *interpreter, UINTVAL character)
{
#if PARROT_HAS_ICU
	return u_isprint(character);
#else
    return isprint(character);
#endif
}

/*

=item C<INTVAL
Parrot_char_is_punct(Interp *interpreter, UINTVAL character)>

Returns whether the specified character is a punctuation character.

=cut

*/

INTVAL
Parrot_char_is_punct(Interp *interpreter, UINTVAL character)
{
#if PARROT_HAS_ICU
    return u_ispunct(character);
#else
    return ispunct(character);
#endif
}

/*

=item C<INTVAL
Parrot_char_is_space(Interp *interpreter, UINTVAL character)>

=item C<INTVAL
Parrot_char_is_UWhiteSpace(Interp *interpreter, UINTVAL character)>

=item C<INTVAL
Parrot_char_is_Whitespace(Interp *interpreter, UINTVAL character)>

=item C<INTVAL
Parrot_char_is_JavaSpaceChar(Interp *interpreter, UINTVAL character)>

Returns whether the specified character is a space character.

=cut

*/

INTVAL
Parrot_char_is_space(Interp *interpreter, UINTVAL character)
{
#if PARROT_HAS_ICU
    return u_isspace(character);
#else
    return isspace(character);
#endif
}

INTVAL
Parrot_char_is_UWhiteSpace(Interp *interpreter, UINTVAL character)
{
#if PARROT_HAS_ICU
    return u_isUWhiteSpace(character);
#else
    return isspace(character);
#endif
}

INTVAL
Parrot_char_is_Whitespace(Interp *interpreter, UINTVAL character)
{
#if PARROT_HAS_ICU
    return u_isWhitespace(character);
#else
    return isspace(character);
#endif
}

INTVAL
Parrot_char_is_JavaSpaceChar(Interp *interpreter, UINTVAL character)
{
#if PARROT_HAS_ICU
    return u_isJavaSpaceChar(character);
#else
    return isspace(character);
#endif
}

/*

=item C<INTVAL
Parrot_char_is_upper(Interp *interpreter, UINTVAL character)>

Returns whether the specified character is an uppercase character.

=cut

*/

INTVAL
Parrot_char_is_upper(Interp *interpreter, UINTVAL character)
{
#if PARROT_HAS_ICU
    return u_isupper(character);
#else
    return isupper(character);
#endif
}

/*

=item C<INTVAL
Parrot_char_is_xdigit(Interp *interpreter, UINTVAL character)>

Returns whether the specified character is a hexadecimal digit character.

=cut

*/

INTVAL
Parrot_char_is_xdigit(Interp *interpreter, UINTVAL character)
{
#if PARROT_HAS_ICU
    return u_isxdigit(character);
#else
    return isxdigit(character);
#endif
}

/*

=back

=head1 SEE ALSO

=over

=item F<include/parrot/string_primitives.h>

=item F<include/parrot/string.h>

=item F<src/string.c>

=back

=cut

*/

/*
 * Local variables:
 * c-indentation-style: bsd
 * c-basic-offset: 4
 * indent-tabs-mode: nil
 * End:
 *
 * vim: expandtab shiftwidth=4:
 */
