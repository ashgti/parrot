/*
Copyright: 2004 The Perl Foundation.  All Rights Reserved.
$Id$

=head1 NAME

charset/ascii.c

=head1 DESCRIPTION

This file implements the charset functions for ascii data

=cut

*/

#include "parrot/parrot.h"
#include "binary.h"

/* The encoding we prefer, given a choice */
static ENCODING *preferred_encoding;

static STRING *get_graphemes(Interp *interpreter, STRING *source_string, UINTVAL offset, UINTVAL count) {
  STRING *return_string = NULL;

  return return_string;
}

static void set_graphemes(Interp *interpreter, STRING *source_string, UINTVAL offset, UINTVAL replace_count, STRING *insert_string) {
}

static void to_charset(Interp *interpreter, STRING *source_string, CHARSET *new_charset) {
}

static STRING *copy_to_charset(Interp *interpreter, STRING *source_string, CHARSET *new_charset) {
  STRING *return_string = NULL;

  return return_string;
}

static void to_unicode(Interp *interpreter, STRING *source_string) {
    internal_exception(UNIMPLEMENTED, "to_unicode for binary not implemented");
}

/* A noop. can't compose binary */
static void compose(Interp *interpreter, STRING *source_string) {
}

/* A noop. can't decompose binary */
static void decompose(Interp *interpreter, STRING *source_string) {
}

static void upcase(Interp *interpreter, STRING *source_string) {
    internal_error(INVALID_CHARTYPE, "Can't upcase binary data");
}

static void downcase(Interp *interpreter, STRING *source_string) {
    internal_error(INVALID_CHARTYPE, "Can't downcase binary data");
}

static void titlecase(Interp *interpreter, STRING *source_string) {
    internal_error(INVALID_CHARTYPE, "Can't titlecase binary data");
}

static void upcase_first(Interp *interpreter, STRING *source_string) {
    internal_error(INVALID_CHARTYPE, "Can't upcase binary data");
}

static void downcase_first(Interp *interpreter, STRING *source_string) {
    internal_error(INVALID_CHARTYPE, "Can't downcase binary data");
}

static void titlecase_first(Interp *interpreter, STRING *source_string) {
    internal_error(INVALID_CHARTYPE, "Can't titlecase binary data");
}

static INTVAL compare(Interp *interpreter, STRING *lhs, STRING *rhs) {
  return 0;
}

static INTVAL index(Interp *interpreter, STRING *source_string, STRING *search_string, UINTVAL offset) {
  return -1;
}

static INTVAL rindex(Interp *interpreter, STRING *source_string, STRING *search_string, UINTVAL offset) {
  return -1;
}

/* Binary's always valid */
static UINTVAL validate(Interp *interpreter, STRING *source_string) {
    return 1;
}

/* No word chars in binary data */
static INTVAL is_wordchar(Interp *interpreter, STRING *source_string, UINTVAL offset) {
  return 0;
}

static INTVAL find_wordchar(Interp *interpreter, STRING *source_string, UINTVAL offset) {
  return -1;
}

static INTVAL find_not_wordchar(Interp *interpreter, STRING *source_string, UINTVAL offset) {
  return offset;
}

static INTVAL is_whitespace(Interp *interpreter, STRING *source_string, UINTVAL offset) {
  return 0;
}

static INTVAL find_whitespace(Interp *interpreter, STRING *source_string, UINTVAL offset) {
  return -1;
}

static INTVAL find_not_whitespace(Interp *interpreter, STRING *source_string, UINTVAL offset) {
  return offset;
}

static INTVAL is_digit(Interp *interpreter, STRING *source_string, UINTVAL offset) {
  return 0;
}

static INTVAL find_digit(Interp *interpreter, STRING *source_string, UINTVAL offset) {
  return -1;
}

static INTVAL find_not_digit(Interp *interpreter, STRING *source_string, UINTVAL offset) {
  return offset;
}

static INTVAL is_punctuation(Interp *interpreter, STRING *source_string, UINTVAL offset) {
  return 0;
}

static INTVAL find_punctuation(Interp *interpreter, STRING *source_string, UINTVAL offset) {
  return -1;
}

static INTVAL find_not_punctuation(Interp *interpreter, STRING *source_string, UINTVAL offset) {
  return offset;
}

static INTVAL is_newline(Interp *interpreter, STRING *source_string, UINTVAL offset) {
  return 0;
}

static INTVAL find_newline(Interp *interpreter, STRING *source_string, UINTVAL offset) {
  return -1;
}

static INTVAL find_not_newline(Interp *interpreter, STRING *source_string, UINTVAL offset) {
  return offset;
}

static INTVAL find_word_boundary(Interp *interpreter, STRING *source_string, UINTVAL offset) {
  return -1;
}

CHARSET *Parrot_charset_binary_init(Interp *interpreter) {
  CHARSET *return_set = Parrot_new_charset(interpreter);
  CHARSET base_set = {
      "binary",
      get_graphemes,
      set_graphemes,
      to_charset,
      copy_to_charset,
      to_unicode,
      compose,
      decompose,
      upcase,
      downcase,
      titlecase,
      upcase_first,
      downcase_first,
      titlecase_first,
      compare,
      index,
      rindex,
      validate,
      is_wordchar,
      find_wordchar,
      find_not_wordchar,
      is_whitespace,
      find_whitespace,
      find_not_whitespace,
      is_digit,
      find_digit,
      find_not_digit,
      is_punctuation,
      find_punctuation,
      find_not_punctuation,
      is_newline,
      find_newline,
      find_not_newline,
      find_word_boundary
  };

  preferred_encoding = Parrot_load_encoding(interpreter, "fixed_8");
  
  memcpy(return_set, &base_set, sizeof(CHARSET));
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
