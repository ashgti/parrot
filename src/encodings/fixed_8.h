/* fixed_8.h
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

#if !defined(PARROT_ENCODING_FIXED_8_H_GUARD)
#define PARROT_ENCODING_FIXED_8_H_GUARD

static UINTVAL get_codepoint(Interp *interp, const STRING *source_string, UINTVAL offset);
static void set_codepoint(Interp *interp, STRING *source_string, UINTVAL offset, UINTVAL codepoint);
static UINTVAL get_byte(Interp *interp, const STRING *source_string, UINTVAL offset);
static void set_byte(Interp *interp, const STRING *source_string, UINTVAL offset, UINTVAL byte);
static STRING *get_codepoints(Interp *interp, STRING *source_string, UINTVAL offset, UINTVAL count);
static STRING *get_bytes(Interp *interp, STRING *source_string, UINTVAL offset, UINTVAL count);
static STRING *get_bytes_inplace(Interp *interp, STRING *source_string, UINTVAL offset, UINTVAL count, STRING *dest_string);
static void set_codepoints(Interp *interp, STRING *source_string, UINTVAL offset, UINTVAL count, STRING *new_codepoints);
static void set_bytes(Interp *interp, STRING *source_string, UINTVAL offset, UINTVAL count, STRING *new_bytes);
static void become_encoding(Interp *interp, STRING *source_string);
static UINTVAL codepoints(Interp *interp, STRING *source_string);
static UINTVAL bytes(Interp *interp, STRING *source_string);
ENCODING *Parrot_encoding_fixed_8_init(Interp *interp);

#endif /* PARROT_ENCODING_FIXED_8_H_GUARD */

/*
 * Local variables:
 *   c-file-style: "parrot"
 * End:
 * vim: expandtab shiftwidth=4:
 */
