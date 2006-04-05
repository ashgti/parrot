/* stacks.h
 *  Copyright: 2001-2003 The Perl Foundation.  All Rights Reserved.
 *  SVN Info
 *     $Id$
 *  Overview:
 *     Stack handling routines for Parrot
 *  Data Structure and Algorithms:
 *  History:
 *  Notes:
 *  References: See also enums.h
 */

#if !defined(PARROT_STACKS_H_GUARD)
#define PARROT_STACKS_H_GUARD

#include "parrot/parrot.h"

#define STACK_CHUNK_LIMIT 100000

typedef struct Stack_Entry {
    UnionVal entry;
    Stack_entry_type  entry_type;
    void (*cleanup)(Interp *, struct Stack_Entry *);
} Stack_Entry_t;

typedef struct Stack_Chunk {
    pobj_t obj;
    int size;
    const char * name;
    struct Stack_Chunk *prev;
#if ! DISABLE_GC_DEBUG && defined(I386)
    void * dummy;   /* force 8 byte align for mmx and sse moves */
#endif
    union { /* force appropriate alignment of 'data' */
	void *data;
#ifndef I386
	double d_dummy;         /* align double values on stack */
#endif
    } u;
} Stack_Chunk_t;

#define STACK_DATAP(chunk)    &chunk->u.data
/* #define STACK_ITEMSIZE(chunk) PObj_buflen(chunk) */


typedef void (*Stack_cleanup_method)(Interp*, Stack_Entry_t *);

#define STACK_CLEANUP_NULL ((Stack_cleanup_method)NULLfunc)

PARROT_API void stack_system_init(Interp *interpreter);
PARROT_API void stack_destroy(Stack_Chunk_t * top);

/*
 * stack_common functions
 */
Stack_Chunk_t * register_new_stack(Parrot_Interp, const char *name, size_t);
Stack_Chunk_t * cst_new_stack_chunk(Parrot_Interp, const Stack_Chunk_t *chunk);
void* stack_prepare_push(Parrot_Interp, Stack_Chunk_t **stack_p);
void* stack_prepare_pop(Parrot_Interp, Stack_Chunk_t **stack_p);
void mark_stack_chunk_cache(Parrot_Interp interpreter);

/*
 * pad, user, control stacks
 */

PARROT_API Stack_Chunk_t * new_stack(Interp *interpreter, const char *name);
PARROT_API void mark_stack(Interp *, Stack_Chunk_t * cur_stack);

PARROT_API size_t stack_height(Interp *interpreter, Stack_Chunk_t *stack_base);

PARROT_API Stack_Entry_t * stack_entry(Interp *intepreter, Stack_Chunk_t *stack_base,
                          INTVAL stack_depth);

PARROT_API void rotate_entries(Interp *interpreter, Stack_Chunk_t **stack_base,
                    INTVAL num_entries);

PARROT_API void stack_push(Interp *interpreter, Stack_Chunk_t **stack_base,
                void *thing, Stack_entry_type type,
                Stack_cleanup_method cleanup);

PARROT_API void *stack_pop(Interp *interpreter, Stack_Chunk_t **stack_base,
                void *where, Stack_entry_type type);

PARROT_API void *pop_dest(Interp *interpreter);

PARROT_API void *stack_peek(Interp *interpreter, Stack_Chunk_t *stack,
                Stack_entry_type *type);

PARROT_API Stack_entry_type get_entry_type(Interp *interpreter, Stack_Entry_t *entry);



#define ERROR_STACK_EMPTY 1
#define ERROR_STACK_SHALLOW 1
#define ERROR_BAD_STACK_TYPE 1

#endif /* PARROT_STACKS_H_GUARD */

/*
 * Local variables:
 * c-indentation-style: bsd
 * c-basic-offset: 4
 * indent-tabs-mode: nil
 * End:
 *
 * vim: expandtab shiftwidth=4:
*/
