/*
Copyright (C) 2001-2009, Parrot Foundation.
$Id$

=head1 NAME

src/string/encoding/ucs2.c - UCS-2 encoding

=head1 DESCRIPTION

UCS-2 encoding with the help of the ICU library.

=head2 Functions

=over 4

=cut

*/

#include "parrot/parrot.h"
#include "../unicode.h"

#if !PARROT_HAS_ICU
PARROT_DOES_NOT_RETURN
static void no_ICU_lib(PARROT_INTERP) /* HEADERIZER SKIP */
{
    Parrot_ex_throw_from_c_args(interp, NULL,
        EXCEPTION_LIBRARY_ERROR,
        "no ICU lib loaded");
}
#endif

/* HEADERIZER HFILE: src/string/encoding/ucs2.h */

/* HEADERIZER BEGIN: static */
/* Don't modify between HEADERIZER BEGIN / HEADERIZER END.  Your changes will be lost. */

static void become_encoding(PARROT_INTERP, SHIM(STRING *src))
        __attribute__nonnull__(1);

PARROT_WARN_UNUSED_RESULT
static UINTVAL bytes(PARROT_INTERP, ARGIN(STRING *src))
        __attribute__nonnull__(1)
        __attribute__nonnull__(2);

PARROT_WARN_UNUSED_RESULT
static UINTVAL codepoints(PARROT_INTERP, ARGIN(STRING *src))
        __attribute__nonnull__(1)
        __attribute__nonnull__(2);

PARROT_WARN_UNUSED_RESULT
static UINTVAL find_cclass(PARROT_INTERP,
    ARGIN(STRING *s),
    ARGIN(const INTVAL *typetable),
    INTVAL flags,
    UINTVAL pos,
    UINTVAL end)
        __attribute__nonnull__(1)
        __attribute__nonnull__(2)
        __attribute__nonnull__(3);

static UINTVAL get_byte(PARROT_INTERP,
    SHIM(const STRING *src),
    SHIM(UINTVAL offset))
        __attribute__nonnull__(1);

PARROT_WARN_UNUSED_RESULT
PARROT_CANNOT_RETURN_NULL
static STRING * get_bytes(PARROT_INTERP,
    SHIM(STRING *src),
    SHIM(UINTVAL offset),
    SHIM(UINTVAL count))
        __attribute__nonnull__(1);

PARROT_WARN_UNUSED_RESULT
PARROT_CANNOT_RETURN_NULL
static STRING * get_bytes_inplace(PARROT_INTERP,
    SHIM(STRING *src),
    SHIM(UINTVAL offset),
    SHIM(UINTVAL count),
    SHIM(STRING *return_string))
        __attribute__nonnull__(1);

static UINTVAL get_codepoint(PARROT_INTERP,
    ARGIN(const STRING *src),
    UINTVAL offset)
        __attribute__nonnull__(1)
        __attribute__nonnull__(2);

PARROT_WARN_UNUSED_RESULT
PARROT_CANNOT_RETURN_NULL
static STRING * get_codepoints(PARROT_INTERP,
    ARGIN(STRING *src),
    UINTVAL offset,
    UINTVAL count)
        __attribute__nonnull__(1)
        __attribute__nonnull__(2);

PARROT_WARN_UNUSED_RESULT
PARROT_CANNOT_RETURN_NULL
static STRING * get_codepoints_inplace(PARROT_INTERP,
    SHIM(STRING *src),
    SHIM(UINTVAL offset),
    SHIM(UINTVAL count),
    SHIM(STRING *dest_string))
        __attribute__nonnull__(1);

static void iter_init(PARROT_INTERP,
    ARGIN(const STRING *src),
    ARGOUT(String_iter *iter))
        __attribute__nonnull__(1)
        __attribute__nonnull__(2)
        __attribute__nonnull__(3)
        FUNC_MODIFIES(*iter);

static void set_byte(PARROT_INTERP,
    SHIM(const STRING *src),
    SHIM(UINTVAL offset),
    SHIM(UINTVAL byte))
        __attribute__nonnull__(1);

static void set_bytes(PARROT_INTERP,
    SHIM(STRING *src),
    SHIM(UINTVAL offset),
    SHIM(UINTVAL count),
    SHIM(STRING *new_bytes))
        __attribute__nonnull__(1);

static void set_codepoint(PARROT_INTERP,
    ARGIN(STRING *src),
    UINTVAL offset,
    UINTVAL codepoint)
        __attribute__nonnull__(1)
        __attribute__nonnull__(2);

static void set_codepoints(PARROT_INTERP,
    SHIM(STRING *src),
    SHIM(UINTVAL offset),
    SHIM(UINTVAL count),
    SHIM(STRING *new_codepoints))
        __attribute__nonnull__(1);

PARROT_WARN_UNUSED_RESULT
PARROT_CANNOT_RETURN_NULL
static STRING * to_encoding(PARROT_INTERP,
    ARGIN(STRING *src),
    ARGMOD(STRING *dest))
        __attribute__nonnull__(1)
        __attribute__nonnull__(2)
        __attribute__nonnull__(3)
        FUNC_MODIFIES(*dest);

static UINTVAL ucs2_decode_and_advance(PARROT_INTERP,
    ARGMOD(String_iter *i))
        __attribute__nonnull__(1)
        __attribute__nonnull__(2)
        FUNC_MODIFIES(*i);

static void ucs2_encode_and_advance(PARROT_INTERP,
    ARGMOD(String_iter *i),
    UINTVAL c)
        __attribute__nonnull__(1)
        __attribute__nonnull__(2)
        FUNC_MODIFIES(*i);

static void ucs2_set_position(SHIM_INTERP,
    ARGMOD(String_iter *i),
    UINTVAL n)
        __attribute__nonnull__(2)
        FUNC_MODIFIES(*i);

#define ASSERT_ARGS_become_encoding __attribute__unused__ int _ASSERT_ARGS_CHECK = (\
       PARROT_ASSERT_ARG(interp))
#define ASSERT_ARGS_bytes __attribute__unused__ int _ASSERT_ARGS_CHECK = (\
       PARROT_ASSERT_ARG(interp) \
    , PARROT_ASSERT_ARG(src))
#define ASSERT_ARGS_codepoints __attribute__unused__ int _ASSERT_ARGS_CHECK = (\
       PARROT_ASSERT_ARG(interp) \
    , PARROT_ASSERT_ARG(src))
#define ASSERT_ARGS_find_cclass __attribute__unused__ int _ASSERT_ARGS_CHECK = (\
       PARROT_ASSERT_ARG(interp) \
    , PARROT_ASSERT_ARG(s) \
    , PARROT_ASSERT_ARG(typetable))
#define ASSERT_ARGS_get_byte __attribute__unused__ int _ASSERT_ARGS_CHECK = (\
       PARROT_ASSERT_ARG(interp))
#define ASSERT_ARGS_get_bytes __attribute__unused__ int _ASSERT_ARGS_CHECK = (\
       PARROT_ASSERT_ARG(interp))
#define ASSERT_ARGS_get_bytes_inplace __attribute__unused__ int _ASSERT_ARGS_CHECK = (\
       PARROT_ASSERT_ARG(interp))
#define ASSERT_ARGS_get_codepoint __attribute__unused__ int _ASSERT_ARGS_CHECK = (\
       PARROT_ASSERT_ARG(interp) \
    , PARROT_ASSERT_ARG(src))
#define ASSERT_ARGS_get_codepoints __attribute__unused__ int _ASSERT_ARGS_CHECK = (\
       PARROT_ASSERT_ARG(interp) \
    , PARROT_ASSERT_ARG(src))
#define ASSERT_ARGS_get_codepoints_inplace __attribute__unused__ int _ASSERT_ARGS_CHECK = (\
       PARROT_ASSERT_ARG(interp))
#define ASSERT_ARGS_iter_init __attribute__unused__ int _ASSERT_ARGS_CHECK = (\
       PARROT_ASSERT_ARG(interp) \
    , PARROT_ASSERT_ARG(src) \
    , PARROT_ASSERT_ARG(iter))
#define ASSERT_ARGS_set_byte __attribute__unused__ int _ASSERT_ARGS_CHECK = (\
       PARROT_ASSERT_ARG(interp))
#define ASSERT_ARGS_set_bytes __attribute__unused__ int _ASSERT_ARGS_CHECK = (\
       PARROT_ASSERT_ARG(interp))
#define ASSERT_ARGS_set_codepoint __attribute__unused__ int _ASSERT_ARGS_CHECK = (\
       PARROT_ASSERT_ARG(interp) \
    , PARROT_ASSERT_ARG(src))
#define ASSERT_ARGS_set_codepoints __attribute__unused__ int _ASSERT_ARGS_CHECK = (\
       PARROT_ASSERT_ARG(interp))
#define ASSERT_ARGS_to_encoding __attribute__unused__ int _ASSERT_ARGS_CHECK = (\
       PARROT_ASSERT_ARG(interp) \
    , PARROT_ASSERT_ARG(src) \
    , PARROT_ASSERT_ARG(dest))
#define ASSERT_ARGS_ucs2_decode_and_advance __attribute__unused__ int _ASSERT_ARGS_CHECK = (\
       PARROT_ASSERT_ARG(interp) \
    , PARROT_ASSERT_ARG(i))
#define ASSERT_ARGS_ucs2_encode_and_advance __attribute__unused__ int _ASSERT_ARGS_CHECK = (\
       PARROT_ASSERT_ARG(interp) \
    , PARROT_ASSERT_ARG(i))
#define ASSERT_ARGS_ucs2_set_position __attribute__unused__ int _ASSERT_ARGS_CHECK = (\
       PARROT_ASSERT_ARG(i))
/* Don't modify between HEADERIZER BEGIN / HEADERIZER END.  Your changes will be lost. */
/* HEADERIZER END: static */

#include "ucs2.h"

#if PARROT_HAS_ICU
#  include <unicode/ustring.h>
#endif

#define UNIMPL Parrot_ex_throw_from_c_args(interp, NULL, EXCEPTION_UNIMPLEMENTED, \
    "unimpl ucs2")

/*

=item C<static STRING * to_encoding(PARROT_INTERP, STRING *src, STRING *dest)>

Converts the string C<src> to this particular encoding.  If C<dest> is
provided, it will contain the result.  Otherwise this function operates in
place.

=cut

*/

PARROT_WARN_UNUSED_RESULT
PARROT_CANNOT_RETURN_NULL
static STRING *
to_encoding(PARROT_INTERP, ARGIN(STRING *src), ARGMOD(STRING *dest))
{
    ASSERT_ARGS(to_encoding)
    STRING * const result =
        Parrot_utf16_encoding_ptr->to_encoding(interp, src, dest);

    /* conversion to utf16 downgrads to ucs-2 if possible - check result */
    if (result->encoding == Parrot_utf16_encoding_ptr)
        Parrot_ex_throw_from_c_args(interp, NULL, EXCEPTION_INVALID_ENCODING,
            "can't convert string with surrogates to ucs2");

    return result;
}

/*

=item C<static UINTVAL get_codepoint(PARROT_INTERP, const STRING *src, UINTVAL
offset)>

Returns the codepoint in string C<src> at position C<offset>.

=cut

*/

static UINTVAL
get_codepoint(PARROT_INTERP, ARGIN(const STRING *src), UINTVAL offset)
{
    ASSERT_ARGS(get_codepoint)
#if PARROT_HAS_ICU
    UChar * const s = (UChar*) src->strstart;
    return s[offset];
#else
    UNUSED(offset)
    no_ICU_lib(interp);
#endif
}

/*

=item C<static void set_codepoint(PARROT_INTERP, STRING *src, UINTVAL offset,
UINTVAL codepoint)>

Sets, in string C<src> at position C<offset>, the codepoint C<codepoint>.

=cut

*/

static void
set_codepoint(PARROT_INTERP, ARGIN(STRING *src), UINTVAL offset, UINTVAL codepoint)
{
    ASSERT_ARGS(set_codepoint)
#if PARROT_HAS_ICU
    UChar * const s = (UChar*) src->strstart;
    s[offset] = codepoint;
#else
    UNUSED(src)
    UNUSED(offset)
    UNUSED(codepoint)
    no_ICU_lib(interp);
#endif
}

/*

=item C<static UINTVAL find_cclass(PARROT_INTERP, STRING *s, const INTVAL
*typetable, INTVAL flags, UINTVAL pos, UINTVAL end)>

Stub, the charset level handles this for unicode strings.

=cut

*/

PARROT_WARN_UNUSED_RESULT
static UINTVAL
find_cclass(PARROT_INTERP, ARGIN(STRING *s), ARGIN(const INTVAL *typetable),
INTVAL flags, UINTVAL pos, UINTVAL end)
{
    Parrot_ex_throw_from_c_args(interp, NULL,
        EXCEPTION_UNIMPLEMENTED,
        "No find_cclass support in unicode encoding plugins");
}

/*

=item C<static UINTVAL get_byte(PARROT_INTERP, const STRING *src, UINTVAL
offset)>

Returns the byte in string C<src> at position C<offset>.

=cut

*/

static UINTVAL
get_byte(PARROT_INTERP, SHIM(const STRING *src), SHIM(UINTVAL offset))
{
    ASSERT_ARGS(get_byte)
    UNIMPL;
}

/*

=item C<static void set_byte(PARROT_INTERP, const STRING *src, UINTVAL offset,
UINTVAL byte)>

Sets, in string C<src> at position C<offset>, the byte C<byte>.

=cut

*/

static void
set_byte(PARROT_INTERP, SHIM(const STRING *src), SHIM(UINTVAL offset),
        SHIM(UINTVAL byte))
{
    ASSERT_ARGS(set_byte)
    UNIMPL;
}

/*

=item C<static STRING * get_codepoints(PARROT_INTERP, STRING *src, UINTVAL
offset, UINTVAL count)>

Returns the codepoints in string C<src> at position C<offset> and length
C<count>.

=cut

*/

PARROT_WARN_UNUSED_RESULT
PARROT_CANNOT_RETURN_NULL
static STRING *
get_codepoints(PARROT_INTERP, ARGIN(STRING *src), UINTVAL offset, UINTVAL count)
{
    ASSERT_ARGS(get_codepoints)
    STRING * const return_string = Parrot_str_new_COW(interp, src);
#if PARROT_HAS_ICU
    return_string->strstart = (char*)src->strstart + offset * sizeof (UChar);
    return_string->bufused = count * sizeof (UChar);
#else
    {
        String_iter iter;
        UINTVAL start;

        iter_init(interp, src, &iter);
        iter.set_position(interp, &iter, offset);
        start = iter.bytepos;
        return_string->strstart = (char *)return_string->strstart + start;
        iter.set_position(interp, &iter, offset + count);
        return_string->bufused = iter.bytepos - start;
    }
#endif
    return_string->strlen = count;
    return_string->hashval = 0;
    return return_string;
}

/*

=item C<static STRING * get_bytes(PARROT_INTERP, STRING *src, UINTVAL offset,
UINTVAL count)>

Returns the bytes in string C<src> at position C<offset> and length C<count>.

=cut

*/

PARROT_WARN_UNUSED_RESULT
PARROT_CANNOT_RETURN_NULL
static STRING *
get_bytes(PARROT_INTERP, SHIM(STRING *src), SHIM(UINTVAL offset),
        SHIM(UINTVAL count))
{
    ASSERT_ARGS(get_bytes)
    UNIMPL;
}


/*

=item C<static STRING * get_codepoints_inplace(PARROT_INTERP, STRING *src,
UINTVAL offset, UINTVAL count, STRING *dest_string)>

Gets from string C<src> at position C<offset> C<count> codepoints and returns
them in C<return_string>.

=cut

*/

PARROT_WARN_UNUSED_RESULT
PARROT_CANNOT_RETURN_NULL
static STRING *
get_codepoints_inplace(PARROT_INTERP, SHIM(STRING *src),
        SHIM(UINTVAL offset), SHIM(UINTVAL count), SHIM(STRING *dest_string))
{
    ASSERT_ARGS(get_codepoints_inplace)
    UNIMPL;
}

/*

=item C<static STRING * get_bytes_inplace(PARROT_INTERP, STRING *src, UINTVAL
offset, UINTVAL count, STRING *return_string)>

Gets from string C<src> at position C<offset> C<count> bytes and returns them
in C<return_string>.

=cut

*/

PARROT_WARN_UNUSED_RESULT
PARROT_CANNOT_RETURN_NULL
static STRING *
get_bytes_inplace(PARROT_INTERP, SHIM(STRING *src),
        SHIM(UINTVAL offset), SHIM(UINTVAL count), SHIM(STRING *return_string))
{
    ASSERT_ARGS(get_bytes_inplace)
    UNIMPL;
}

/*

=item C<static void set_codepoints(PARROT_INTERP, STRING *src, UINTVAL offset,
UINTVAL count, STRING *new_codepoints)>

Replaces in string C<src> at position C<offset> for C<count> codepoints with
the contents of string C<new_codepoints>.

=cut

*/

static void
set_codepoints(PARROT_INTERP, SHIM(STRING *src),
        SHIM(UINTVAL offset), SHIM(UINTVAL count), SHIM(STRING *new_codepoints))
{
    ASSERT_ARGS(set_codepoints)
    UNIMPL;
}

/*

=item C<static void set_bytes(PARROT_INTERP, STRING *src, UINTVAL offset,
UINTVAL count, STRING *new_bytes)>

Replaces in string C<src> at position C<offset> for C<count> bytes with the
contents of string C<new_bytes>.

=cut

*/

static void
set_bytes(PARROT_INTERP, SHIM(STRING *src),
        SHIM(UINTVAL offset), SHIM(UINTVAL count), SHIM(STRING *new_bytes))
{
    ASSERT_ARGS(set_bytes)
    UNIMPL;
}

/*

=item C<static void become_encoding(PARROT_INTERP, STRING *src)>

Unconditionally makes the string be in this encoding, if that's valid

=cut

*/

static void
become_encoding(PARROT_INTERP, SHIM(STRING *src))
{
    ASSERT_ARGS(become_encoding)
    UNIMPL;
}


/*

=item C<static UINTVAL codepoints(PARROT_INTERP, STRING *src)>

Returns the number of codepoints in string C<src>.

=cut

*/

PARROT_WARN_UNUSED_RESULT
static UINTVAL
codepoints(PARROT_INTERP, ARGIN(STRING *src))
{
    ASSERT_ARGS(codepoints)
#if PARROT_HAS_ICU
    return src->bufused / sizeof (UChar);
#else
    no_ICU_lib(interp);
#endif
}

/*

=item C<static UINTVAL bytes(PARROT_INTERP, STRING *src)>

Returns the number of bytes in string C<src>.

=cut

*/

PARROT_WARN_UNUSED_RESULT
static UINTVAL
bytes(PARROT_INTERP, ARGIN(STRING *src))
{
    ASSERT_ARGS(bytes)
    return src->bufused;
}

/*

=item C<static UINTVAL ucs2_decode_and_advance(PARROT_INTERP, String_iter *i)>

Moves the string iterator C<i> to the next UCS-2 codepoint.

=cut

*/

static UINTVAL
ucs2_decode_and_advance(PARROT_INTERP, ARGMOD(String_iter *i))
{
    ASSERT_ARGS(ucs2_decode_and_advance)

#if PARROT_HAS_ICU
    UChar * const s = (UChar*) i->str->strstart;
    size_t pos = i->bytepos / sizeof (UChar);

    /* TODO either make sure that we don't go past end or use SAFE
     *      iter versions
     */
    const UChar c = s[pos++];
    i->charpos++;
    i->bytepos = pos * sizeof (UChar);
    return c;
#else
    /* This function must never be called if compiled without ICU.
     * See TT #557
     */
    PARROT_ASSERT(0);
    return (UINTVAL)0; /* Stop the static analyzers from panicing */
#endif
}

/*

=item C<static void ucs2_encode_and_advance(PARROT_INTERP, String_iter *i,
UINTVAL c)>

With the string iterator C<i>, appends the codepoint C<c> and advances to the
next position in the string.

=cut

*/

static void
ucs2_encode_and_advance(PARROT_INTERP, ARGMOD(String_iter *i), UINTVAL c)
{
    ASSERT_ARGS(ucs2_encode_and_advance)

#if PARROT_HAS_ICU
    UChar * const s = (UChar*) i->str->strstart;
    UINTVAL pos = i->bytepos / sizeof (UChar);
    s[pos++] = (UChar)c;
    i->charpos++;
    i->bytepos = pos * sizeof (UChar);
#else
    /* This function must never be called if compiled without ICU.
     * See TT #557
     */
    PARROT_ASSERT(0);
#endif
}

/*

=item C<static void ucs2_set_position(PARROT_INTERP, String_iter *i, UINTVAL n)>

Moves the string iterator C<i> to the position C<n> in the string.

=cut

*/

static void
ucs2_set_position(SHIM_INTERP, ARGMOD(String_iter *i), UINTVAL n)
{
    ASSERT_ARGS(ucs2_set_position)

#if PARROT_HAS_ICU
    i->charpos = n;
    i->bytepos = n * sizeof (UChar);
#else
    /* This function must never be called if compiled without ICU.
     * See TT #557
     */
    PARROT_ASSERT(0);
#endif
}


/*

=item C<static void iter_init(PARROT_INTERP, const STRING *src, String_iter
*iter)>

Initializes for string C<src> the string iterator C<iter>.

=cut

*/

static void
iter_init(PARROT_INTERP, ARGIN(const STRING *src), ARGOUT(String_iter *iter))
{
    ASSERT_ARGS(iter_init)
#if PARROT_HAS_ICU
    iter->str             = src;
    iter->bytepos         = 0;
    iter->charpos         = 0;
    iter->get_and_advance = ucs2_decode_and_advance;
    iter->set_and_advance = ucs2_encode_and_advance;
    iter->set_position    = ucs2_set_position;
#else
    no_ICU_lib(interp);
#endif
}

/*

=item C<ENCODING * Parrot_encoding_ucs2_init(PARROT_INTERP)>

Initializes the UCS-2 encoding.

=cut

*/

PARROT_CANNOT_RETURN_NULL
ENCODING *
Parrot_encoding_ucs2_init(PARROT_INTERP)
{
    ASSERT_ARGS(Parrot_encoding_ucs2_init)
    ENCODING * const return_encoding = Parrot_new_encoding(interp);

    static const ENCODING base_encoding = {
        "ucs2",
        2, /* Max bytes per codepoint 0 .. 0x10ffff */
        to_encoding,
        get_codepoint,
        set_codepoint,
        get_byte,
        set_byte,
        get_codepoints,
        get_codepoints_inplace,
        get_bytes,
        get_bytes_inplace,
        set_codepoints,
        set_bytes,
        become_encoding,
        codepoints,
        bytes,
        iter_init,
        find_cclass
    };
    STRUCT_COPY_FROM_STRUCT(return_encoding, base_encoding);
    Parrot_register_encoding(interp, "ucs2", return_encoding);
    return return_encoding;
}

/*

=back

=head1 SEE ALSO

F<src/string/encoding/fixed_8.c>,
F<src/string/encoding/utf8.c>,
F<src/string.c>,
F<include/parrot/string.h>,
F<docs/string.pod>.

=cut

*/


/*
 * Local variables:
 *   c-file-style: "parrot"
 * End:
 * vim: expandtab shiftwidth=4:
 */
