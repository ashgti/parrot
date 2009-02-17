/* slice.h
 *  Copyright (C) 2001-2008, Parrot Foundation.
 *  SVN Info
 *     $Id$
 *  Overview:
 *     This is the api header for slices.
 *  Data Structure and Algorithms:
 *  History:
 *  Notes:
 *  References:
 */

#ifndef PARROT_SLICE_H_GUARD
#define PARROT_SLICE_H_GUARD

#ifdef PARROT_IN_CORE

typedef struct RUnion {
    INTVAL i;
    STRING *s;
} RUnion;

typedef struct _parrot_range_t {
    int type;                      /* enum_type_INTVAL or STRING */
    RUnion start;                  /* start of this range */
    RUnion end;                    /* end of this range */
    RUnion step;                   /* step of this range */
    RUnion cur;                    /* current value */
    struct _parrot_range_t *next;  /* next in chain */
} parrot_range_t;

#endif /* PARROT_IN_CORE */
#endif /* PARROT_SLICE_H_GUARD */

/*
 * Local variables:
 *   c-file-style: "parrot"
 * End:
 * vim: expandtab shiftwidth=4:
 */
