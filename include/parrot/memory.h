/* memory.h
 *  Copyright (C) 2001-2008, The Perl Foundation.
 *  SVN Info
 *     $Id$
 *  Overview:
 *     This is the API header for the memory subsystem
 *  Data Structure and Algorithms:
 *  History:
 *  Notes:
 *  References:
 */

#ifndef PARROT_MEMORY_H_GUARD
#define PARROT_MEMORY_H_GUARD
#include <assert.h>

/* Use these macros instead of calling the functions listed below. */
/* They protect against things like passing null to mem__sys_realloc, */
/* which is not portable. */
#define mem_internal_allocate(x) mem__internal_allocate((x), __FILE__, __LINE__)
#define mem_internal_allocate_typed(t) \
    (t *)mem__internal_allocate(sizeof (t), __FILE__, __LINE__)
#define mem_internal_allocate_zeroed(x) mem__internal_allocate_zeroed(x, \
    __FILE__, __LINE__)
#define mem_internal_allocate_zeroed_typed(t) \
    (t *)mem__internal_allocate_zeroed(sizeof (t), __FILE__, __LINE__)

#define mem_internal_realloc(x, y) mem__internal_realloc(x, y, __FILE__, __LINE__)
#define mem_internal_free(x) mem__internal_free(x, __FILE__, __LINE__)

#define mem_allocate_new_stash() NULL
#define mem_allocate_new_stack() NULL
#define mem_sys_memcopy memcpy
#define mem_sys_memmove memmove

#define mem_allocate_typed(type)            (type *)mem_sys_allocate(sizeof (type))
#define mem_allocate_n_typed(n, type)       (type *)mem_sys_allocate((n) * sizeof(type))
#define mem_allocate_zeroed_typed(type)     (type *)mem_sys_allocate_zeroed(sizeof (type))
#define mem_allocate_n_zeroed_typed(n, type) (type *)mem_sys_allocate_zeroed((n) * sizeof(type))
#define mem_realloc_n_typed(p, n, type)     (p) = (type *)mem_sys_realloc((p), (n)*sizeof(type))
#define mem_copy_n_typed(dest, src, n, type) memcpy((dest), (src), (n)*sizeof(type))

/* HEADERIZER BEGIN: src/gc/memory.c */

PARROT_API
PARROT_MALLOC
PARROT_CANNOT_RETURN_NULL
void * mem_sys_allocate(size_t size);

PARROT_API
PARROT_MALLOC
PARROT_CANNOT_RETURN_NULL
void * mem_sys_allocate_zeroed(size_t size);

PARROT_API
void mem_sys_free(ARGFREE(void *from));

PARROT_API
PARROT_MALLOC
PARROT_CANNOT_RETURN_NULL
void * mem_sys_realloc(ARGFREE(void *from), size_t size);

PARROT_API
PARROT_MALLOC
PARROT_CANNOT_RETURN_NULL
void * mem_sys_realloc_zeroed(
    ARGFREE(void *from),
    size_t size,
    size_t old_size);

PARROT_MALLOC
PARROT_CANNOT_RETURN_NULL
void * mem__internal_allocate(
    size_t size,
    ARGIN(const char *file),
    int line)
        __attribute__nonnull__(2);

PARROT_MALLOC
PARROT_CANNOT_RETURN_NULL
void * mem__internal_allocate_zeroed(
    size_t size,
    ARGIN(const char *file),
    int line)
        __attribute__nonnull__(2);

void mem__internal_free(
    ARGFREE(void *from),
    ARGIN(const char *file),
    int line)
        __attribute__nonnull__(2);

PARROT_MALLOC
PARROT_CANNOT_RETURN_NULL
void * mem__internal_realloc(
    ARGFREE(void *from),
    size_t size,
    ARGIN(const char *file),
    int line)
        __attribute__nonnull__(3);

void mem_setup_allocator(PARROT_INTERP)
        __attribute__nonnull__(1);

/* HEADERIZER END: src/gc/memory.c */

#endif /* PARROT_MEMORY_H_GUARD */

/*
 * Local variables:
 *   c-file-style: "parrot"
 * End:
 * vim: expandtab shiftwidth=4:
 */
