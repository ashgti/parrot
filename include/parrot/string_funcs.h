/* string_funcs.h
 *  Copyright: (When this is determined...it will go here)
 *  CVS Info
 *     $Id$
 *  Overview:
 *     This is the api header for the string subsystem
 *  Data Structure and Algorithms:
 *  History:
 *  Notes:
 *  References:
 */

#if !defined(PARROT_STRING_FUNCS_H_GUARD)
#define PARROT_STRING_FUNCS_H_GUARD

#ifdef PARROT_IN_CORE

/* Declarations of accessors */

INTVAL string_compute_strlen(STRING *);
INTVAL string_max_bytes(STRING*, INTVAL);
STRING *string_concat(struct Parrot_Interp *, STRING *, STRING *, UINTVAL);
STRING *string_append(struct Parrot_Interp *, STRING *, STRING *, UINTVAL);
STRING *string_from_c_string(struct Parrot_Interp *, const void *, UINTVAL);
STRING *string_repeat(struct Parrot_Interp *, const STRING *, UINTVAL,
                      STRING **);
STRING *string_chopn(STRING *, INTVAL);
STRING *string_substr(struct Parrot_Interp *, STRING *, INTVAL,
                      INTVAL, STRING **);
STRING *string_replace(struct Parrot_Interp *, STRING *, INTVAL, INTVAL,
                       STRING *, STRING **);
STRING *string_nprintf(struct Parrot_Interp *,
                       STRING *, INTVAL, const char *, ...);
INTVAL string_compare(struct Parrot_Interp *, STRING *, STRING *);
INTVAL string_bool(const STRING *);
const char *Parrot_string_cstring(const STRING *);

/* Declarations of other functions */
UINTVAL string_length(const STRING *);
INTVAL string_ord(const STRING *, INTVAL idx);
FLOATVAL string_to_num(const STRING *);
INTVAL string_to_int(const STRING *);
STRING *string_from_int(struct Parrot_Interp *, INTVAL i);
STRING *int_to_str(struct Parrot_Interp *,
           char *tc, HUGEINTVAL num, char base);
STRING *string_from_num(struct Parrot_Interp *, FLOATVAL f);
STRING *string_grow(struct Parrot_Interp *, STRING *s, INTVAL addlen);
STRING *string_make(struct Parrot_Interp *, const void *buff,
                    UINTVAL len, const ENCODING *, UINTVAL flags,
                    const CHARTYPE *);
STRING *string_copy(struct Parrot_Interp *, STRING *);
STRING *string_set(struct Parrot_Interp *, STRING *d, STRING *s);
STRING *string_transcode(struct Parrot_Interp *, STRING *src,
                         const ENCODING *, const CHARTYPE *,
                         STRING **dest_ptr);
void string_init(void);
INTVAL string_index(const STRING *, UINTVAL idx);
INTVAL string_str_index(struct Parrot_Interp *interpreter, const STRING *s,
        const STRING *s2, UINTVAL start);
char *string_to_cstring(struct Parrot_Interp *, STRING *);
void string_cstring_free(void *);
void string_pin(struct Parrot_Interp *, STRING *);
void string_unpin(struct Parrot_Interp *, STRING *);

#endif

#endif

/*
 * Local variables:
 * c-indentation-style: bsd
 * c-basic-offset: 4
 * indent-tabs-mode: nil
 * End:
 *
 * vim: expandtab shiftwidth=4:
*/
