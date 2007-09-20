/*
Copyright (C) 2001-2007, The Perl Foundation.
$Id$

=head1 NAME

src/stack_common.c - Common stack handling routines for Parrot

=head1 DESCRIPTION

Both the register stacks and stacks implemented in F<src/stacks.c>
have a common functionality, which is implemented in this file.

These stacks all differ only in the size of items.

=head2 Functions

=cut

*/

#include "parrot/parrot.h"
#include "parrot/stacks.h"

/* HEADERIZER HFILE: include/parrot/stacks.h */

/*

FUNCDOC:
Called from C<make_interpreter()> to initialize the interpreter's
register stacks.

*/

PARROT_API
void
stack_system_init(SHIM_INTERP)
{
}


/*

FUNCDOC:
Create a new stack and name it. C<< stack->name >> is used for
debugging/error reporting.

*/

PARROT_API
PARROT_WARN_UNUSED_RESULT
PARROT_CANNOT_RETURN_NULL
Stack_Chunk_t *
register_new_stack(PARROT_INTERP, NOTNULL(const char *name), size_t item_size)
{
    Stack_Chunk_t *chunk;

    item_size += offsetof(Stack_Chunk_t, u.data);
    make_bufferlike_pool(interp, item_size);
    chunk = (Stack_Chunk_t *)new_bufferlike_header(interp, item_size);
    chunk->prev = chunk;        /* mark the top of the stack */
    chunk->name = name;
    chunk->size = item_size;    /* TODO store the pool instead the size */
    return chunk;
}

/*

FUNCDOC:
Get a new chunk either from the freelist or allocate one.

*/

PARROT_API
PARROT_WARN_UNUSED_RESULT
PARROT_CANNOT_RETURN_NULL
Stack_Chunk_t *
cst_new_stack_chunk(PARROT_INTERP, NOTNULL(const Stack_Chunk_t *chunk))
{
    Small_Object_Pool * const pool =
        get_bufferlike_pool(interp, chunk->size);
    Stack_Chunk_t * const new_chunk = (Stack_Chunk_t *)pool->get_free_object(interp, pool);

    PObj_bufstart(new_chunk) = NULL;
    PObj_buflen(new_chunk) = 0;

    new_chunk->size = chunk->size;
    new_chunk->name = chunk->name;
    return new_chunk;
}

/*

FUNCDOC:
Return a pointer, where new entries go for push.

*/

PARROT_API
PARROT_WARN_UNUSED_RESULT
PARROT_CANNOT_RETURN_NULL
void*
stack_prepare_push(PARROT_INTERP, NOTNULL(Stack_Chunk_t **stack_p))
{
    Stack_Chunk_t * const chunk = *stack_p;
    Stack_Chunk_t * const new_chunk = cst_new_stack_chunk(interp, chunk);

    new_chunk->prev = chunk;
    *stack_p = new_chunk;
    return STACK_DATAP(new_chunk);
}


/*

FUNCDOC:
Return a pointer, where new entries are popped off.

*/

PARROT_API
PARROT_WARN_UNUSED_RESULT
PARROT_CANNOT_RETURN_NULL
void*
stack_prepare_pop(PARROT_INTERP, NOTNULL(Stack_Chunk_t **stack_p))
{
    Stack_Chunk_t * const chunk = *stack_p;

    /*
     * the first entry (initial top) refers to itself
     */
    if (chunk == chunk->prev) {
        real_exception(interp, NULL, ERROR_STACK_EMPTY, "No entries on %sStack!",
                chunk->name);
    }
    *stack_p = chunk->prev;
    return STACK_DATAP(chunk);
}

/*

=head1 SEE ALSO

F<include/parrot/stacks.h>, F<src/stacks.c>, F<src/registers.c>

=cut

*/



/*
 * Local variables:
 *   c-file-style: "parrot"
 * End:
 * vim: expandtab shiftwidth=4:
 */

