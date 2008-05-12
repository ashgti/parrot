/* exit.h
 *  Copyright (C) 2001-2007, The Perl Foundation.
 *  SVN Info
 *     $Id$
 *  Overview:
 *
 *  Data Structure and Algorithms:
 *  History:
 *  Notes:
 *  References:
 *      exit.c
 */

#ifndef PARROT_EXIT_H_GUARD
#define PARROT_EXIT_H_GUARD

#include "parrot/compiler.h"    /* compiler capabilities */

typedef void (*exit_handler_f)(PARROT_INTERP, int , void *);

typedef struct _handler_node_t {
    exit_handler_f function;
    void *arg;
    struct _handler_node_t *next;
} handler_node_t;

/* HEADERIZER BEGIN: src/exit.c */
/* Don't modify between HEADERIZER BEGIN / HEADERIZER END.  Your changes will be lost. */

PARROT_API
PARROT_DOES_NOT_RETURN
void Parrot_exit(PARROT_INTERP, int status)
        __attribute__nonnull__(1);

PARROT_API
void Parrot_on_exit(PARROT_INTERP,
    NOTNULL(exit_handler_f function),
    ARGIN_NULLOK(void *arg))
        __attribute__nonnull__(1)
        __attribute__nonnull__(2);

/* Don't modify between HEADERIZER BEGIN / HEADERIZER END.  Your changes will be lost. */
/* HEADERIZER END: src/exit.c */

#endif /* PARROT_EXIT_H_GUARD */

/*
 * Local variables:
 *   c-file-style: "parrot"
 * End:
 * vim: expandtab shiftwidth=4:
 */
