/*
Copyright (C) 2010, Parrot Foundation.
$Id$

=head1 NAME

src/string/encoding/ucs4.c - UCS-4 encoding

=head1 DESCRIPTION

UCS-4 encoding with the help of the ICU library.

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

/* HEADERIZER HFILE: src/string/encoding/ucs4.h */

/* HEADERIZER BEGIN: static */
/* Don't modify between HEADERIZER BEGIN / HEADERIZER END.  Your changes will be lost. */

PARROT_WARN_UNUSED_RESULT
static UINTVAL bytes(SHIM_INTERP, ARGIN(const STRING *src))
        __attribute__nonnull__(2);

PARROT_WARN_UNUSED_RESULT
static UINTVAL codepoints(PARROT_INTERP, ARGIN(const STRING *src))
        __attribute__nonnull__(1)
        __attribute__nonnull__(2);

PARROT_WARN_UNUSED_RESULT
static UINTVAL find_cclass(PARROT_INTERP,
    ARGIN(const STRING *s),
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
    SHIM(const STRING *src),
    SHIM(UINTVAL offset),
    SHIM(UINTVAL count))
        __attribute__nonnull__(1);

static UINTVAL get_codepoint(PARROT_INTERP,
    ARGIN(const STRING *src),
    UINTVAL offset)
        __attribute__nonnull__(1)
        __attribute__nonnull__(2);

PARROT_WARN_UNUSED_RESULT
PARROT_CANNOT_RETURN_NULL
static STRING * get_codepoints(PARROT_INTERP,
    ARGIN(const STRING *src),
    UINTVAL offset,
    UINTVAL count)
        __attribute__nonnull__(1)
        __attribute__nonnull__(2);

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

PARROT_WARN_UNUSED_RESULT
PARROT_CANNOT_RETURN_NULL
static STRING * to_encoding(PARROT_INTERP, ARGIN(const STRING *src))
        __attribute__nonnull__(1)
        __attribute__nonnull__(2);

static UINTVAL ucs4_decode_and_advance(PARROT_INTERP,
    ARGMOD(String_iter *i))
        __attribute__nonnull__(1)
        __attribute__nonnull__(2)
        FUNC_MODIFIES(*i);

static void ucs4_encode_and_advance(PARROT_INTERP,
    ARGMOD(String_iter *i),
    UINTVAL c)
        __attribute__nonnull__(1)
        __attribute__nonnull__(2)
        FUNC_MODIFIES(*i);

static size_t ucs4_hash(PARROT_INTERP,
    ARGIN(const STRING *s),
    size_t hashval)
        __attribute__nonnull__(1)
        __attribute__nonnull__(2);

static void ucs4_set_position(PARROT_INTERP,
    ARGMOD(String_iter *i),
    UINTVAL n)
        __attribute__nonnull__(1)
        __attribute__nonnull__(2)
        FUNC_MODIFIES(*i);

#define ASSERT_ARGS_bytes __attribute__unused__ int _ASSERT_ARGS_CHECK = (\
       PARROT_ASSERT_ARG(src))
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
#define ASSERT_ARGS_get_codepoint __attribute__unused__ int _ASSERT_ARGS_CHECK = (\
       PARROT_ASSERT_ARG(interp) \
    , PARROT_ASSERT_ARG(src))
#define ASSERT_ARGS_get_codepoints __attribute__unused__ int _ASSERT_ARGS_CHECK = (\
       PARROT_ASSERT_ARG(interp) \
    , PARROT_ASSERT_ARG(src))
#define ASSERT_ARGS_iter_init __attribute__unused__ int _ASSERT_ARGS_CHECK = (\
       PARROT_ASSERT_ARG(interp) \
    , PARROT_ASSERT_ARG(src) \
    , PARROT_ASSERT_ARG(iter))
#define ASSERT_ARGS_set_byte __attribute__unused__ int _ASSERT_ARGS_CHECK = (\
       PARROT_ASSERT_ARG(interp))
#define ASSERT_ARGS_to_encoding __attribute__unused__ int _ASSERT_ARGS_CHECK = (\
       PARROT_ASSERT_ARG(interp) \
    , PARROT_ASSERT_ARG(src))
#define ASSERT_ARGS_ucs4_decode_and_advance __attribute__unused__ int _ASSERT_ARGS_CHECK = (\
       PARROT_ASSERT_ARG(interp) \
    , PARROT_ASSERT_ARG(i))
#define ASSERT_ARGS_ucs4_encode_and_advance __attribute__unused__ int _ASSERT_ARGS_CHECK = (\
       PARROT_ASSERT_ARG(interp) \
    , PARROT_ASSERT_ARG(i))
#define ASSERT_ARGS_ucs4_hash __attribute__unused__ int _ASSERT_ARGS_CHECK = (\
       PARROT_ASSERT_ARG(interp) \
    , PARROT_ASSERT_ARG(s))
#define ASSERT_ARGS_ucs4_set_position __attribute__unused__ int _ASSERT_ARGS_CHECK = (\
       PARROT_ASSERT_ARG(interp) \
    , PARROT_ASSERT_ARG(i))
/* Don't modify between HEADERIZER BEGIN / HEADERIZER END.  Your changes will be lost. */
/* HEADERIZER END: static */

#include "ucs4.h"

#if PARROT_HAS_ICU
#  include <unicode/ustring.h>
#endif

/*

=item C<static STRING * to_encoding(PARROT_INTERP, const STRING *src)>

Converts the string C<src> to this particular encoding.

=cut

*/

PARROT_WARN_UNUSED_RESULT
PARROT_CANNOT_RETURN_NULL
static STRING *
to_encoding(PARROT_INTERP, ARGIN(const STRING *src))
{
    ASSERT_ARGS(to_encoding)
#if PARROT_HAS_ICU
    if (src->encoding == Parrot_ucs4_encoding_ptr) {
        return Parrot_str_clone(interp, src);
    }
    else {
        UINTVAL len = Parrot_str_length(interp, src);
        STRING *res = Parrot_str_new_init(interp, NULL, len * sizeof (UChar32),
                           Parrot_ucs4_encoding_ptr, Parrot_unicode_charset_ptr, 0);
        UChar32 *buf = (UChar32 *) res->strstart;
        UINTVAL offs;
        for (offs = 0; offs < len; offs++){
            buf[offs] = src->encoding->get_codepoint(interp, src, offs);
        };
        res->strlen  = len;
        res->bufused = len * sizeof (UChar32);

        return res;
    }
#else
    UNUSED(src);
    no_ICU_lib(interp);
#endif

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
    const UChar32 * const s = (const UChar32*) src->strstart;
    UNUSED(interp);
    return s[offset];
#else
    UNUSED(offset);
    UNUSED(src);
    no_ICU_lib(interp);
#endif
}


/*

=item C<static UINTVAL find_cclass(PARROT_INTERP, const STRING *s, const INTVAL
*typetable, INTVAL flags, UINTVAL pos, UINTVAL end)>

Stub, the charset level handles this for unicode strings.

=cut

*/

PARROT_WARN_UNUSED_RESULT
static UINTVAL
find_cclass(PARROT_INTERP, ARGIN(const STRING *s), ARGIN(const INTVAL *typetable),
INTVAL flags, UINTVAL pos, UINTVAL end)
{
    ASSERT_ARGS(find_cclass)

    UNUSED(s);
    UNUSED(typetable);
    UNUSED(flags);
    UNUSED(pos);
    UNUSED(end);

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
    Parrot_ex_throw_from_c_args(interp, NULL,
        EXCEPTION_UNIMPLEMENTED,
        "No get_byte for UCS-4");
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
    Parrot_ex_throw_from_c_args(interp, NULL,
        EXCEPTION_UNIMPLEMENTED,
        "No set_byte for UCS-4");
}

/*

=item C<static STRING * get_codepoints(PARROT_INTERP, const STRING *src, UINTVAL
offset, UINTVAL count)>

Returns the C<count> codepoints stored at position C<offset> in string
C<src> as a new string.

=cut

*/

PARROT_WARN_UNUSED_RESULT
PARROT_CANNOT_RETURN_NULL
static STRING *
get_codepoints(PARROT_INTERP, ARGIN(const STRING *src), UINTVAL offset, UINTVAL count)
{
    ASSERT_ARGS(get_codepoints)
#if PARROT_HAS_ICU
    return Parrot_str_new_init(interp, (char*)src->strstart + offset * sizeof (UChar32),
                               count * sizeof (UChar32), src->encoding, src->charset, 0);
#else
    UNUSED(src);
    UNUSED(offset);
    UNUSED(count);
    no_ICU_lib(interp);
#endif
}

/*

=item C<static STRING * get_bytes(PARROT_INTERP, const STRING *src, UINTVAL
offset, UINTVAL count)>

Returns the bytes in string C<src> at position C<offset> and length C<count>.

=cut

*/

PARROT_WARN_UNUSED_RESULT
PARROT_CANNOT_RETURN_NULL
static STRING *
get_bytes(PARROT_INTERP, SHIM(const STRING *src), SHIM(UINTVAL offset),
        SHIM(UINTVAL count))
{
    ASSERT_ARGS(get_bytes)
    Parrot_ex_throw_from_c_args(interp, NULL,
        EXCEPTION_UNIMPLEMENTED,
        "No get_bytes for UCS-4");
}


/*

=item C<static UINTVAL codepoints(PARROT_INTERP, const STRING *src)>

Returns the number of codepoints in string C<src>.

=cut

*/

PARROT_WARN_UNUSED_RESULT
static UINTVAL
codepoints(PARROT_INTERP, ARGIN(const STRING *src))
{
    ASSERT_ARGS(codepoints)
#if PARROT_HAS_ICU
    UNUSED(interp);
    return src->bufused / sizeof (UChar32);
#else
    UNUSED(src);
    no_ICU_lib(interp);
#endif
}

/*

=item C<static UINTVAL bytes(PARROT_INTERP, const STRING *src)>

Returns the number of bytes in string C<src>.

=cut

*/

PARROT_WARN_UNUSED_RESULT
static UINTVAL
bytes(SHIM_INTERP, ARGIN(const STRING *src))
{
    ASSERT_ARGS(bytes)
    return src->bufused;
}

/*

=item C<static UINTVAL ucs4_decode_and_advance(PARROT_INTERP, String_iter *i)>

Moves the string iterator C<i> to the next UCS-4 codepoint.

=cut

*/

static UINTVAL
ucs4_decode_and_advance(PARROT_INTERP, ARGMOD(String_iter *i))
{
    ASSERT_ARGS(ucs4_decode_and_advance)
#if PARROT_HAS_ICU
    const UChar32 * const s = (const UChar32 *) i->str->strstart;
    size_t pos              = i->bytepos / sizeof (UChar32);
    const UChar32         c = s[pos++];
    ++i->charpos;
    i->bytepos = pos * sizeof (UChar32);
    return c;
#else
    UNUSED(i);
    no_ICU_lib(interp);
#endif
}

/*

=item C<static void ucs4_encode_and_advance(PARROT_INTERP, String_iter *i,
UINTVAL c)>

With the string iterator C<i>, appends the codepoint C<c> and advances to the
next position in the string.

=cut

*/

static void
ucs4_encode_and_advance(PARROT_INTERP, ARGMOD(String_iter *i), UINTVAL c)
{
    ASSERT_ARGS(ucs4_encode_and_advance)
#if PARROT_HAS_ICU
    UChar32 *s   = (UChar32 *) i->str->strstart;
    size_t   pos = i->bytepos / sizeof (UChar32);
    s[pos++] = (UChar32) c;
    ++i->charpos;
    i->bytepos = pos * sizeof (UChar32);
#else
    UNUSED(i);
    no_ICU_lib(interp);
#endif
}

#if PARROT_HAS_ICU
/*

=item C<static size_t ucs4_hash(PARROT_INTERP, const STRING *s, size_t hashval)>

Returns the hashed value of the string, given a seed in hashval.

=cut

*/

static size_t
ucs4_hash(PARROT_INTERP, ARGIN(const STRING *s), size_t hashval)
{
    ASSERT_ARGS(ucs4_hash)
    const UChar32 *pos = (const UChar32*) s->strstart;
    UINTVAL len = s->strlen;
    UNUSED(interp);

    while (len--) {
        hashval += hashval << 5;
        hashval += *(pos++);
    }

    return hashval;
}
#endif

/*

=item C<static void ucs4_set_position(PARROT_INTERP, String_iter *i, UINTVAL n)>

Moves the string iterator C<i> to the position C<n> in the string.

=cut

*/

static void
ucs4_set_position(PARROT_INTERP, ARGMOD(String_iter *i), UINTVAL n)
{
    ASSERT_ARGS(ucs4_set_position)
#if PARROT_HAS_ICU
    i->charpos = n;
    i->bytepos = n * sizeof (UChar32);
#else
    UNUSED(i);
    UNUSED(n);
    no_ICU_lib(interp);
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
    UNUSED(interp);
    iter->str             = src;
    iter->bytepos         = 0;
    iter->charpos         = 0;
    iter->get_and_advance = ucs4_decode_and_advance;
    iter->set_and_advance = ucs4_encode_and_advance;
    iter->set_position    = ucs4_set_position;
#else
    UNUSED(src);
    UNUSED(iter);
    no_ICU_lib(interp);
#endif
}

/*

=item C<void Parrot_encoding_ucs4_init(PARROT_INTERP)>

Initializes the UCS-4 encoding.

=cut

*/

void
Parrot_encoding_ucs4_init(PARROT_INTERP)
{
    ASSERT_ARGS(Parrot_encoding_ucs4_init)
    ENCODING * const return_encoding = Parrot_new_encoding(interp);

    static const ENCODING base_encoding = {
        "ucs4",
        4, /* Max bytes per codepoint */
        to_encoding,
        get_codepoint,
        get_byte,
        set_byte,
        get_codepoints,
        get_bytes,
        codepoints,
        bytes,
        iter_init,
        find_cclass,
#if PARROT_HAS_ICU
        ucs4_hash
#else
        NULL
#endif
    };
    STRUCT_COPY_FROM_STRUCT(return_encoding, base_encoding);
    Parrot_register_encoding(interp, "ucs4", return_encoding);

    return;
}

/*

=back

=head1 SEE ALSO

F<src/string/encoding/fixed_8.c>,
F<src/string/encoding/utf8.c>,
F<src/string/encoding/utf16.c>,
F<src/string/encoding/ucs2.c>,
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
