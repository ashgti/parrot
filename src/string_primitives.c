/*
Copyright: 2004 The Perl Foundation.  All Rights Reserved.
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
#include <unicode/ucnv.h>
#include <unicode/utypes.h>
#include <unicode/uchar.h>
#include <assert.h>

/*

=item C<void
string_set_data_directory(const char *dir)>

Sets the ICU data directory.

=cut

*/

void
string_set_data_directory(const char *dir)
{
    u_setDataDirectory(dir);
}

/*

=item C<void
string_fill_from_buffer(struct Parrot_Interp *interpreter, const void *buffer,
            UINTVAL len, const char *encoding_name, STRING *s)>

Creates a Parrot string from an "external" buffer.

=cut

*/

void
string_fill_from_buffer(struct Parrot_Interp *interpreter, const void *buffer,
            UINTVAL len, const char *encoding_name, STRING *s)
{
    UErrorCode icuError = U_ZERO_ERROR;
    UConverter *conv = NULL;
    UChar *target = NULL;
    UChar *target_limit = NULL;
    const char *source = NULL;
    const char *source_limit = NULL;

	assert(buffer); assert(encoding_name);

	if( s && !len ) /* XXXX: I _guess_ this is always an empty string--is that right? */
	{
		s->bufused = 0;
		s->strlen = 0;
		return;
	}

	/* big guess--allocate same space for string as buffer needed.
	   may be able to make a more educated guess based on the encoding. */
	Parrot_allocate_string(interpreter, s, len);

    conv = ucnv_open(encoding_name, &icuError);

	if( !conv || icuError != U_ZERO_ERROR )
	{
		/* unknown encoding??? */
		internal_exception(ICU_ERROR,
				"string_fill_from_buffer: ICU error from ucnv_open()");

	}

	target = s->strstart;
	 /* buflen may be larger than what we asked for, so take advantage of the space */
	target_limit = (UChar *)((char *)PObj_bufstart(s) + PObj_buflen(s) - 1);
	source = buffer;
	source_limit = source + len;

	ucnv_toUnicode(conv, &target, target_limit, &source, source_limit, NULL, TRUE, &icuError);

	while( icuError == U_BUFFER_OVERFLOW_ERROR )
	{
		size_t consumed_length = (char *)target - (char *)(s->strstart);

		Parrot_reallocate_string(interpreter, s, 2 * PObj_buflen(s)); /* double size, at least */

		target = (UChar *)((char *)s->strstart + consumed_length);
		target_limit = (UChar *)((char *)PObj_bufstart(s) + PObj_buflen(s) - 1);

		icuError = U_ZERO_ERROR;
		ucnv_toUnicode(conv, &target, target_limit, &source, source_limit, NULL, TRUE, &icuError);
	}

	ucnv_close(conv);

	if( icuError != U_ZERO_ERROR )
	{
		//handle error
		internal_exception(ICU_ERROR,
				"string_fill_from_buffer: ICU error from ucnv_toUnicode()");
	}

	s->representation = enum_stringrep_two; /* temporary; need to promote to rep 4 if has non-BMP characters*/
	s->bufused = (char *)target - (char *)s->strstart;
	string_compute_strlen(interpreter, s);
}

/*
{

    UErrorCode myError = U_ZERO_ERROR;
    UConverter *conv = ucnv_open("ISO-8859-1", &myError);

    NSData *data = [NSData dataWithContentsOfFile:@"/var/tmp/ja.txt"];


    UConverter *conv2 = ucnv_open("UTF-8", &myError);
    UChar *outputBuffer = malloc(50*sizeof(UChar));
    UChar *start = outputBuffer;
    char *source = [data bytes];
    UErrorCode convError = 0;

    ucnv_toUnicode(conv2, &outputBuffer, &(outputBuffer[50]), &source, source + [data length], NULL, TRUE, &convError);

    NSLog(@"conv error = %i", convError);
    NSLog(@"conv length = %i", outputBuffer - start);


    UConverter *conv3 = ucnv_open("UCS-2", &myError); //ISO-8859-1"
    char *outbuffer = malloc(50);
    char *outstart = outbuffer;
    UErrorCode redoErr = 0;

    NSLog(@"conv3 = %p", conv3);
    NSLog(@"%s", ucnv_getName(conv3, &redoErr));

        UErrorCode callbackStatus = 0;

          ucnv_setFromUCallBack(conv3,
                       UCNV_FROM_U_CALLBACK_STOP, //UCNV_FROM_U_CALLBACK_ESCAPE
                       NULL,
                       NULL,
                       NULL,
                       &callbackStatus);

    ucnv_fromUnicode(conv3, &outbuffer, outbuffer + 50, &start, outputBuffer, NULL, TRUE, &redoErr);
//    ucnv_fromUnicode(conv3, NULL, NULL, &start, outputBuffer, NULL, TRUE, &redoErr);

    NSLog(@"redo error = %i", redoErr);
    NSLog(@"%s", u_errorName(redoErr));
    NSLog(@"redo length = %i", outbuffer - outstart);

NSLog(@"%x", *outstart);
NSLog(@"%d", *outstart);
NSLog(@"%x", outstart[1]);
    NSLog(@"%@", [[NSString alloc] initWithData:[NSData dataWithBytes:outstart length:(outbuffer - outstart)] encoding:NSISOLatin1StringEncoding]);

{
    UConverter *jisConv = ucnv_open("shift_jis", &myError);
    USet *set =  uset_open(0, 0);
    UErrorCode setError = U_ZERO_ERROR;

    ucnv_getUnicodeSet( jisConv, set, UCNV_ROUNDTRIP_SET, &setError);

    NSLog(@"set size = %d", uset_size(set));
    NSLog(@"contains range = %d", uset_containsRange(set, 0x10000, 0x10ffff));
    NSLog(@"uset_containsNoneOfRange = %d", uset_containsNoneOfRange(set, 0x10000, 0x10ffff));
}
{
    UConverter *utf8Conv = ucnv_open("UTF-8", &myError);
    USet *set =  uset_open(0, 0);
    UErrorCode setError = U_ZERO_ERROR;

    ucnv_getUnicodeSet( utf8Conv, set, UCNV_ROUNDTRIP_SET, &setError);

    NSLog(@"set size = %d", uset_size(set));
    NSLog(@"contains range = %d", uset_containsRange(set, 0x10000, 0x10ffff));
    NSLog(@"uset_containsNoneOfRange = %d", uset_containsNoneOfRange(set, 0x10000, 0x10ffff));
}
    [pool release];
    return 0;
}
*/

/*

=item C<UINTVAL 
Parrot_char_digit_value(struct Parrot_Interp *interpreter, UINTVAL character)>

Returns the decimal digit value of the specified character if it is a decimal
digit character. If not, then -1 is returned. 

Note that as currently written, C<Parrot_char_digit_value()> can
correctly return the decimal digit value of characters for which
C<Parrot_char_is_digit()> returns false.

=cut

*/

UINTVAL Parrot_char_digit_value(struct Parrot_Interp *interpreter, UINTVAL character)
{
	return u_charDigitValue(character);
}

/*

=item C<INTVAL
Parrot_char_is_alnum(struct Parrot_Interp *interpreter, UINTVAL character)>

Returns whether the specified character is an alphanumeric character.

=cut

*/

INTVAL
Parrot_char_is_alnum(struct Parrot_Interp *interpreter, UINTVAL character)
{
	return u_isalnum(character);
}

/*

=item C<INTVAL
Parrot_char_is_alpha(struct Parrot_Interp *interpreter, UINTVAL character)>

Returns whether the specified character is an letter character.

=cut

*/

INTVAL
Parrot_char_is_alpha(struct Parrot_Interp *interpreter, UINTVAL character)
{
	return u_isalpha(character);
}

/*

=item C<INTVAL
Parrot_char_is_ascii(struct Parrot_Interp *interpreter, UINTVAL character)>

Returns whether the specified character is an ASCII character.

=cut

*/

INTVAL
Parrot_char_is_ascii(struct Parrot_Interp *interpreter, UINTVAL character)
{
	return (character < 128);
}

/*

=item C<INTVAL
Parrot_char_is_blank(struct Parrot_Interp *interpreter, UINTVAL character)>

Returns whether the specified character is a "blank" or "horizontal
space", a character that visibly separates words on a line.

=cut

*/

INTVAL
Parrot_char_is_blank(struct Parrot_Interp *interpreter, UINTVAL character)
{
	return u_isblank(character);
}

/*

=item C<INTVAL
Parrot_char_is_cntrl(struct Parrot_Interp *interpreter, UINTVAL character)>

Returns whether the specified character is a control character.

=cut

*/

INTVAL
Parrot_char_is_cntrl(struct Parrot_Interp *interpreter, UINTVAL character)
{
	return u_iscntrl(character);
}

/*

=item C<INTVAL
Parrot_char_is_digit(struct Parrot_Interp *interpreter, UINTVAL character)>

Returns whether the specified character is a digit character.

=cut

*/

INTVAL
Parrot_char_is_digit(struct Parrot_Interp *interpreter, UINTVAL character)
{
	return u_isdigit(character);
}

/*

=item C<INTVAL
Parrot_char_is_graph(struct Parrot_Interp *interpreter, UINTVAL character)>

Returns whether the specified character is a a "graphic" character
(printable, excluding spaces).

=cut

*/

INTVAL
Parrot_char_is_graph(struct Parrot_Interp *interpreter, UINTVAL character)
{
	return u_isgraph(character);
}

/*

=item C<INTVAL
Parrot_char_is_lower(struct Parrot_Interp *interpreter, UINTVAL character)>

Returns whether the specified character is a lowercase letter.

=cut

*/

INTVAL
Parrot_char_is_lower(struct Parrot_Interp *interpreter, UINTVAL character)
{
	return u_islower(character);
}

/*

=item C<INTVAL
Parrot_char_is_print(struct Parrot_Interp *interpreter, UINTVAL character)>

Returns whether the specified character is a printable character.

=cut

*/

INTVAL
Parrot_char_is_print(struct Parrot_Interp *interpreter, UINTVAL character)
{
	return u_isprint(character);
}

/*

=item C<INTVAL
Parrot_char_is_punct(struct Parrot_Interp *interpreter, UINTVAL character)>

Returns whether the specified character is a punctuation character.

=cut

*/

INTVAL
Parrot_char_is_punct(struct Parrot_Interp *interpreter, UINTVAL character)
{
	return u_ispunct(character);
}

/*

=item C<INTVAL
Parrot_char_is_space(struct Parrot_Interp *interpreter, UINTVAL character)>

Returns whether the specified character is a space character.

=cut

*/

INTVAL
Parrot_char_is_space(struct Parrot_Interp *interpreter, UINTVAL character)
{
	return u_isspace(character);
}

/*

=item C<INTVAL
Parrot_char_is_upper(struct Parrot_Interp *interpreter, UINTVAL character)>

Returns whether the specified character is an uppercase character.

=cut

*/

INTVAL
Parrot_char_is_upper(struct Parrot_Interp *interpreter, UINTVAL character)
{
	return u_isupper(character);
}

/*

=item C<INTVAL
Parrot_char_is_xdigit(struct Parrot_Interp *interpreter, UINTVAL character)>

Returns whether the specified character is a hexadecimal digit character.

=cut

*/

INTVAL
Parrot_char_is_xdigit(struct Parrot_Interp *interpreter, UINTVAL character)
{
	return u_isxdigit(character);
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
