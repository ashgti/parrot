/* memory.h
 *  Copyright: 2001-2003 The Perl Foundation.  All Rights Reserved.
 *  SVN Info
 *     $Id$
 *  Overview:
 *     This is the api header for the memory subsystem
 *  Data Structure and Algorithms:
 *  History:
 *  Notes:
 *  References:
 */

#if !defined(PARROT_MEMORY_H_GUARD)
#define PARROT_MEMORY_H_GUARD
#include <assert.h>
PARROT_API void *mem_sys_allocate(size_t);

PARROT_API void *mem_sys_allocate_zeroed(size_t);

PARROT_API void *mem__sys_realloc(void *, size_t);
#define mem_sys_realloc(x,y) (assert(x!=NULL), mem__sys_realloc(x,y))
PARROT_API void mem_sys_free(void *);

void *mem__internal_allocate(size_t, const char *, int);
#define mem_internal_allocate(x) mem__internal_allocate(x, __FILE__, __LINE__)

void *mem__internal_allocate_zeroed(size_t, const char *, int);
#define mem_internal_allocate_zeroed(x) mem__internal_allocate_zeroed(x, __FILE__, __LINE__)

void *mem__internal_realloc(void *, size_t, const char *, int);
#define mem_internal_realloc(x, y) mem__internal_realloc(x, y, __FILE__, __LINE__)

void mem__internal_free(void *, const char *, int);
#define mem_internal_free(x) mem__internal_free(x, __FILE__, __LINE__)


void mem_setup_allocator(Interp *);

#define mem_allocate_new_stash() NULL
#define mem_allocate_new_stack() NULL
#define mem_sys_memcopy memcpy
#define mem_sys_memmove memmove

#endif /* PARROT_MEMORY_H_GUARD */

/*
 * Local variables:
 * c-indentation-style: bsd
 * c-basic-offset: 4
 * indent-tabs-mode: nil
 * End:
 *
 * vim: expandtab shiftwidth=4:
*/
