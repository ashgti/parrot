/* stacks.h
 *  Copyright: 2001-2003 The Perl Foundation.  All Rights Reserved.
 *  CVS Info
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

#define STACK_CHUNK_DEPTH 256
#define STACK_CHUNK_LIMIT 100

typedef struct Stack_Entry {
    UnionVal entry;
    Stack_entry_type  entry_type;
    void (*cleanup)(struct Stack_Entry *);
} Stack_Entry_t;

typedef struct Stack_Chunk {
    pobj_t obj;
    size_t used;
    int n_chunks;
    int chunk_limit;
    size_t item_size;
    size_t items_per_chunk;
    const char * name;
    struct Stack_Chunk *next;
    struct Stack_Chunk *prev;
} Stack_Chunk_t;


typedef void (*Stack_cleanup_method)(Stack_Entry_t *);

#define STACK_CLEANUP_NULL ((Stack_cleanup_method)NULLfunc)

void stack_system_init(Interp *interpreter);
void stack_destroy(Stack_Chunk_t * top);

/*
 * stack_common functions
 */
Stack_Chunk_t * cst_new_stack(Parrot_Interp, const char *name, size_t, size_t);
Stack_Chunk_t * stack_copy(Parrot_Interp, Stack_Chunk_t *stack);
void stack_unmake_COW(Parrot_Interp, Stack_Chunk_t *stack);
void* stack_prepare_push(Parrot_Interp, Stack_Chunk_t **stack_p);
void* stack_prepare_pop(Parrot_Interp, Stack_Chunk_t **stack_p);

/*
 * pad, user, control stacks
 */

Stack_Chunk_t * new_stack(Interp *interpreter, const char *name);
void mark_stack(struct Parrot_Interp *, Stack_Chunk_t * cur_stack);

size_t stack_height(Interp *interpreter, Stack_Chunk_t *stack_base);

Stack_Entry_t * stack_entry(Interp *intepreter, Stack_Chunk_t *stack_base,
                          Intval stack_depth);

void rotate_entries(Interp *interpreter, Stack_Chunk_t **stack_base,
                    Intval num_entries);

void stack_push(Interp *interpreter, Stack_Chunk_t **stack_base,
                void *thing, Stack_entry_type type,
                Stack_cleanup_method cleanup);

void *stack_pop(Interp *interpreter, Stack_Chunk_t **stack_base,
                void *where, Stack_entry_type type);

void *pop_dest(Interp *interpreter);

void *stack_peek(Interp *interpreter, Stack_Chunk_t *stack,
                Stack_entry_type *type);

Stack_entry_type get_entry_type(Interp *interpreter, Stack_Entry_t *entry);



#define ERROR_STACK_EMPTY 1
#define ERROR_STACK_SHALLOW 1
#define ERROR_BAD_STACK_TYPE 1

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
