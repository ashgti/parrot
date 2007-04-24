/*
Copyright (C) 2004, The Perl Foundation.
$Id$

=head1 NAME

src/charset.c - global charset functions

=head1 DESCRIPTION

These are parrot's generic charset handling functions

=over 4

=cut

*/

#define PARROT_NO_EXTERN_CHARSET_PTRS
#include "parrot/parrot.h"

#include "encodings/fixed_8.h"
#include "encodings/utf8.h"
#include "encodings/utf16.h"
#include "encodings/ucs2.h"

#include "charset/ascii.h"
#include "charset/binary.h"
#include "charset/iso-8859-1.h"
#include "charset/unicode.h"

CHARSET *Parrot_iso_8859_1_charset_ptr;
CHARSET *Parrot_binary_charset_ptr;
CHARSET *Parrot_default_charset_ptr;
CHARSET *Parrot_unicode_charset_ptr;
CHARSET *Parrot_ascii_charset_ptr;

/*
 * all registered charsets are collected in one global structure
 */

typedef struct To_converter {
    CHARSET *to;
    charset_converter_t func;
} To_converter;

typedef struct One_charset {
    CHARSET *charset;
    STRING  *name;
    int n_converters;
    To_converter *to_converters;
} One_charset;

typedef struct All_charsets {
    int n_charsets;
    One_charset *set;
} All_charsets;

static All_charsets *all_charsets;


CHARSET *
Parrot_new_charset(Interp *interp)
{
    return mem_allocate_typed(CHARSET);
}

void
Parrot_charsets_encodings_deinit(Interp *interp)
{
    int i, n;

    n = all_charsets->n_charsets;
    for (i = 0; i < n; ++i) {
        if (all_charsets->set[i].n_converters)
            mem_sys_free(all_charsets->set[i].to_converters);
        mem_sys_free(all_charsets->set[i].charset);
    }
    mem_sys_free(all_charsets->set);
    mem_sys_free(all_charsets);
    all_charsets = NULL;
    parrot_deinit_encodings(interp);
}

CHARSET *
Parrot_find_charset(Interp *interp, const char *charsetname)
{
    int i, n;

    n = all_charsets->n_charsets;
    for (i = 0; i < n; ++i) {
        if (!strcmp(all_charsets->set[i].charset->name, charsetname)) {
            return all_charsets->set[i].charset;
        }
    }
    return NULL;
}

CHARSET *
Parrot_load_charset(Interp *interp, const char *charsetname)
{
    internal_exception(UNIMPLEMENTED, "Can't load charsets yet");
    return NULL;
}

/*

=item C<INTVAL Parrot_charset_number(Interp *, STRING *charsetname)>

Return the number of the charset or -1 if not found.

=item C<INTVAL Parrot_charset_number_of_str(Interp *, const STRING *src)>

Return the number of the charset of the given string or -1 if not found.

=cut

*/

INTVAL
Parrot_charset_number(Interp *interp, STRING *charsetname)
{
    int i, n;

    n = all_charsets->n_charsets;
    for (i = 0; i < n; ++i) {
        if (!string_equal(interp, all_charsets->set[i].name, charsetname))
            return i;
    }
    return -1;
}

INTVAL
Parrot_charset_number_of_str(Interp *interp, STRING *src)
{
    int i, n;

    n = all_charsets->n_charsets;
    for (i = 0; i < n; ++i) {
        if (src->charset == all_charsets->set[i].charset)
            return i;
    }
    return -1;
}

STRING*
Parrot_charset_name(Interp *interp, INTVAL number_of_charset)
{
    if (number_of_charset >= all_charsets->n_charsets)
        return NULL;
    return all_charsets->set[number_of_charset].name;
}

CHARSET*
Parrot_get_charset(Interp *interp, INTVAL number_of_charset)
{
    if (number_of_charset >= all_charsets->n_charsets)
        return NULL;
    return all_charsets->set[number_of_charset].charset;
}

const char *
Parrot_charset_c_name(Interp *interp, INTVAL number_of_charset)
{
    if (number_of_charset >= all_charsets->n_charsets)
        return NULL;
    return all_charsets->set[number_of_charset].charset->name;
}

static INTVAL
register_charset(Interp *interp, const char *charsetname,
        CHARSET *charset)
{
    int i, n;

    n = all_charsets->n_charsets;
    for (i = 0; i < n; ++i) {
        if (!strcmp(all_charsets->set[i].charset->name, charsetname))
            return 0;
    }
    /*
     * TODO
     * this needs either a LOCK or we just forbid dynamic
     * loading of charsets from inside threads
     */
    if (!n)
        all_charsets->set = mem_allocate_typed(One_charset);
    else
        all_charsets->set = (One_charset *)mem_sys_realloc(all_charsets->set,
                (n + 1) * sizeof (One_charset));
    all_charsets->n_charsets++;
    all_charsets->set[n].charset = charset;
    all_charsets->set[n].name = const_string(interp, charsetname);
    all_charsets->set[n].n_converters = 0;

    return 1;
}

static void
register_static_converters(Interp *interp)
{
    Parrot_register_charset_converter(interp,
            Parrot_iso_8859_1_charset_ptr, Parrot_ascii_charset_ptr,
            charset_cvt_iso_8859_1_to_ascii);
    Parrot_register_charset_converter(interp,
            Parrot_iso_8859_1_charset_ptr, Parrot_binary_charset_ptr,
            charset_cvt_ascii_to_binary);

    Parrot_register_charset_converter(interp,
            Parrot_ascii_charset_ptr, Parrot_binary_charset_ptr,
            charset_cvt_ascii_to_binary);
    Parrot_register_charset_converter(interp,
            Parrot_ascii_charset_ptr, Parrot_iso_8859_1_charset_ptr,
            charset_cvt_ascii_to_iso_8859_1);
}

INTVAL
Parrot_register_charset(Interp *interp, const char *charsetname,
        CHARSET *charset)
{
    if (!all_charsets) {
        all_charsets = mem_allocate_typed(All_charsets);
        all_charsets->n_charsets = 0;
        all_charsets->set = NULL;
    }
    if (!strcmp("binary", charsetname)) {
        Parrot_binary_charset_ptr = charset;
        return register_charset(interp, charsetname, charset);
    }
    if (!strcmp("iso-8859-1", charsetname)) {
        Parrot_iso_8859_1_charset_ptr = charset;
        return register_charset(interp, charsetname, charset);
    }
    if (!strcmp("unicode", charsetname)) {
        Parrot_unicode_charset_ptr = charset;
        return register_charset(interp, charsetname, charset);
    }
    if (!strcmp("ascii", charsetname)) {
        if (!Parrot_default_charset_ptr) {
            Parrot_default_charset_ptr = charset;
        }
        Parrot_ascii_charset_ptr = charset;
        return register_charset(interp, charsetname, charset);
    }
    return 0;
}

void
Parrot_charsets_encodings_init(Interp *interp)
{
    /* the order is crucial here:
     * 1) encodings, default = fixed_8
     * 2) charsets   default = ascii
     */
    Parrot_encoding_fixed_8_init(interp);
    Parrot_encoding_utf8_init(interp);
    Parrot_encoding_ucs2_init(interp);
    Parrot_encoding_utf16_init(interp);

    Parrot_charset_ascii_init(interp);
    Parrot_charset_iso_8859_1_init(interp);
    Parrot_charset_binary_init(interp);
    Parrot_charset_unicode_init(interp);

    /*
     * now encoding strings don't have a charset yet - set default
     */
    parrot_init_encodings_2(interp);
    /*
     * now install charset converters
     */
    register_static_converters(interp);
}

INTVAL
Parrot_make_default_charset(Interp *interp, const char *charsetname,
        CHARSET *charset)
{
    Parrot_default_charset_ptr = charset;
    return 1;
}

CHARSET *
Parrot_default_charset(Interp *interp)
{
    return Parrot_default_charset_ptr;
}


charset_converter_t
Parrot_find_charset_converter(Interp *interp, CHARSET *lhs, CHARSET *rhs)
{
    int i, j, n, nc;

    n = all_charsets->n_charsets;
    for (i = 0; i < n; ++i) {
        if (lhs == all_charsets->set[i].charset) {
            One_charset *left = all_charsets->set + i;

            nc = left->n_converters;
            for (j = 0; j < nc; ++j) {
                if (left->to_converters[j].to == rhs)
                    return left->to_converters[j].func;
            }
        }
    }
    return NULL;
}

void
Parrot_register_charset_converter(Interp *interp,
        CHARSET *lhs, CHARSET *rhs, charset_converter_t func)
{
    int i, n, nc;

    n = all_charsets->n_charsets;
    for (i = 0; i < n; ++i) {
        if (lhs == all_charsets->set[i].charset) {
            One_charset *left = all_charsets->set + i;

            nc = left->n_converters++;
            if (nc) {
                left->to_converters = (To_converter *)mem_sys_realloc(
                        left->to_converters, sizeof (To_converter) * (nc + 1));
            }
            else
                left->to_converters = (To_converter *)mem_sys_allocate(sizeof (To_converter));
            left->to_converters[nc].to = rhs;
            left->to_converters[nc].func = func;
        }
    }
}

/*
 * Local variables:
 *   c-file-style: "parrot"
 * End:
 * vim: expandtab shiftwidth=4:
 */
