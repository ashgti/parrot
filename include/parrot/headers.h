/* headers.h
 *  Copyright (C) 2001-2003, The Perl Foundation.
 *  SVN Info
 *     $Id$
 *  Overview:
 *     Header management functions. Handles getting of various headers,
 *     and pool creation
 *  Data Structure and Algorithms:
 *  History:
 *  Notes:
 *  References:
 */

#ifndef PARROT_HEADERS_H_GUARD
#define PARROT_HEADERS_H_GUARD

#include "parrot/parrot.h"

/*
 * we need an alignment that is the same as malloc(3) have for
 * allocating Buffer items like FLOATVAL (double)
 * This should be either a config hint or tested
 */
#ifdef MALLOC_ALIGNMENT
#  define BUFFER_ALIGNMENT MALLOC_ALIGNMENT
#else
/* or (2 * sizeof (size_t)) */
#  define BUFFER_ALIGNMENT 8
#endif

#define BUFFER_ALIGN_1 (BUFFER_ALIGNMENT - 1)
#define BUFFER_ALIGN_MASK ~BUFFER_ALIGN_1

#define WORD_ALIGN_1 (sizeof (void *) - 1)
#define WORD_ALIGN_MASK ~WORD_ALIGN_1

/* pool iteration */
typedef enum {
    POOL_PMC    = 0x01,
    POOL_BUFFER = 0x02,
    POOL_CONST  = 0x04,
    POOL_ALL    = 0x07
} pool_iter_enum;

typedef int (*pool_iter_fn)(PARROT_INTERP, struct Small_Object_Pool *, int, void*);


/* HEADERIZER BEGIN: src/headers.c */

void add_pmc_ext(PARROT_INTERP, NOTNULL(PMC *pmc))
        __attribute__nonnull__(1)
        __attribute__nonnull__(2);

void add_pmc_sync(PARROT_INTERP, NOTNULL(PMC *pmc))
        __attribute__nonnull__(1)
        __attribute__nonnull__(2);

PARROT_WARN_UNUSED_RESULT
PARROT_CANNOT_RETURN_NULL
Small_Object_Pool * get_bufferlike_pool(PARROT_INTERP, size_t buffer_size)
        __attribute__nonnull__(1);

PARROT_WARN_UNUSED_RESULT
size_t get_max_buffer_address(PARROT_INTERP)
        __attribute__nonnull__(1);

PARROT_WARN_UNUSED_RESULT
size_t get_max_pmc_address(PARROT_INTERP)
        __attribute__nonnull__(1);

PARROT_WARN_UNUSED_RESULT
size_t get_min_buffer_address(PARROT_INTERP)
        __attribute__nonnull__(1);

PARROT_WARN_UNUSED_RESULT
size_t get_min_pmc_address(PARROT_INTERP)
        __attribute__nonnull__(1);

PARROT_WARN_UNUSED_RESULT
int is_buffer_ptr(PARROT_INTERP, NOTNULL(const void *ptr))
        __attribute__nonnull__(1)
        __attribute__nonnull__(2);

PARROT_WARN_UNUSED_RESULT
int is_pmc_ptr(PARROT_INTERP, NOTNULL(const void *ptr))
        __attribute__nonnull__(1)
        __attribute__nonnull__(2);

PARROT_WARN_UNUSED_RESULT
PARROT_CANNOT_RETURN_NULL
Small_Object_Pool * make_bufferlike_pool(PARROT_INTERP, size_t buffer_size)
        __attribute__nonnull__(1);

PARROT_CANNOT_RETURN_NULL
PARROT_WARN_UNUSED_RESULT
Buffer * new_buffer_header(PARROT_INTERP)
        __attribute__nonnull__(1);

PARROT_WARN_UNUSED_RESULT
PARROT_CANNOT_RETURN_NULL
Small_Object_Pool * new_buffer_pool(PARROT_INTERP)
        __attribute__nonnull__(1);

PARROT_CANNOT_RETURN_NULL
PARROT_WARN_UNUSED_RESULT
void * new_bufferlike_header(PARROT_INTERP, size_t size)
        __attribute__nonnull__(1);

PARROT_WARN_UNUSED_RESULT
PARROT_CANNOT_RETURN_NULL
Small_Object_Pool * new_bufferlike_pool(PARROT_INTERP,
    size_t actual_buffer_size)
        __attribute__nonnull__(1);

PARROT_WARN_UNUSED_RESULT
PARROT_CANNOT_RETURN_NULL
PMC * new_pmc_header(PARROT_INTERP, UINTVAL flags)
        __attribute__nonnull__(1);

PARROT_WARN_UNUSED_RESULT
PARROT_CANNOT_RETURN_NULL
Small_Object_Pool * new_pmc_pool(PARROT_INTERP)
        __attribute__nonnull__(1);

PARROT_CANNOT_RETURN_NULL
PARROT_WARN_UNUSED_RESULT
STRING * new_string_header(PARROT_INTERP, UINTVAL flags)
        __attribute__nonnull__(1);

PARROT_WARN_UNUSED_RESULT
PARROT_CANNOT_RETURN_NULL
Small_Object_Pool * new_string_pool(PARROT_INTERP, INTVAL constant)
        __attribute__nonnull__(1);

void Parrot_destroy_header_pools(PARROT_INTERP)
        __attribute__nonnull__(1);

PARROT_WARN_UNUSED_RESULT
int Parrot_forall_header_pools(PARROT_INTERP,
    int flag,
    NULLOK(void *arg),
    NOTNULL(pool_iter_fn func))
        __attribute__nonnull__(1)
        __attribute__nonnull__(4);

void Parrot_initialize_header_pool_names(PARROT_INTERP)
        __attribute__nonnull__(1);

void Parrot_initialize_header_pools(PARROT_INTERP)
        __attribute__nonnull__(1);

void Parrot_merge_header_pools(
    NOTNULL(Interp *dest_interp),
    NOTNULL(Interp *source_interp))
        __attribute__nonnull__(1)
        __attribute__nonnull__(2);

/* HEADERIZER END: src/headers.c */


#endif /* PARROT_HEADERS_H_GUARD */

/*
 * Local variables:
 *   c-file-style: "parrot"
 * End:
 * vim: expandtab shiftwidth=4:
 */
