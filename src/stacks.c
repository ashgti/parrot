/*
Copyright (C) 2001-2007, The Perl Foundation.
$Id$

=head1 NAME

src/stacks.c - Stack handling routines for Parrot

=head1 DESCRIPTION

The stack is stored as a linked list of chunks (C<Stack_Chunk>),
where each chunk has room for one entry.

=head2 Functions

=over 4

=cut

*/

#include "parrot/parrot.h"
#include <assert.h>

/*

=item C<Stack_Chunk_t *
new_stack(Interp *interp, const char *name)>

Create a new stack and name it. C<< stack->name >> is used for
debugging/error reporting.

=cut

*/

Stack_Chunk_t *
new_stack(Interp *interp, const char *name /*NN*/)
{

    return register_new_stack(interp, name, sizeof (Stack_Entry_t));
}


/*

=item C<void
mark_stack(Interp *interp, Stack_Chunk_t *chunk)>

Mark entries in a stack structure during DOD.

=cut

*/

void
mark_stack(Interp *interp, Stack_Chunk_t *chunk /*NN*/)
{
    for (; ; chunk = chunk->prev) {
        Stack_Entry_t *entry;

        pobject_lives(interp, (PObj*)chunk);
        if (chunk == chunk->prev)
            break;
        entry = (Stack_Entry_t *)STACK_DATAP(chunk);
        switch (entry->entry_type) {
            case STACK_ENTRY_PMC:
                if (UVal_pmc(entry->entry))
                    pobject_lives(interp, (PObj *)UVal_pmc(entry->entry));
                break;
            case STACK_ENTRY_STRING:
                if (UVal_str(entry->entry))
                    pobject_lives(interp, (PObj *)UVal_str(entry->entry));
                break;
            default:
                break;
        }
    }
}
/*

=item C<void
stack_destroy(Stack_Chunk_t * top)>

GC does it all.

=cut

*/

void
stack_destroy(Stack_Chunk_t * top)
{
   /* GC does it all */
}

/*

=item C<size_t
stack_height(Interp *interp, const Stack_Chunk_t *top)>

Returns the height of the stack. The maximum "depth" is height - 1.

=cut

*/

size_t
stack_height(Interp *interp, const Stack_Chunk_t *chunk /*NN*/)
{
    size_t height = 0;

    for (; ; chunk = chunk->prev) {
        if (chunk == chunk->prev)
            break;
        ++height;
    }

    return height;
}


/*

=item C<Stack_Entry_t *
stack_entry(Interp *interp, Stack_Chunk_t *stack, INTVAL depth)>

If C<< depth >= 0 >>, return the entry at that depth from the top of the
stack, with 0 being the top entry. If C<depth < 0>, then return the
entry C<|depth|> entries from the bottom of the stack. Returns C<NULL>
if C<|depth| > number> of entries in stack.

=cut

*/

Stack_Entry_t *
stack_entry(Interp *interp, Stack_Chunk_t *stack /*NN*/, INTVAL depth)
{
    Stack_Chunk_t *chunk;
    Stack_Entry_t *entry = NULL;
    size_t offset = (size_t)depth;

    /* For negative depths, look from the bottom of the stack up. */
    if (depth < 0) {
        depth = stack_height(interp,
                             CONTEXT(interp->ctx)->user_stack) + depth;
        if (depth < 0)
            return NULL;
        offset = (size_t)depth;
    }
    chunk = stack;          /* Start at top */
    while (offset) {
        if (chunk == chunk->prev)
            break;
        --offset;
        chunk = chunk->prev;
    }
    if (chunk == chunk->prev)
        return NULL;
    entry = (Stack_Entry_t *)STACK_DATAP(chunk);
    return entry;
}

/*

=item C<void
rotate_entries(Interp *interp, Stack_Chunk_t **stack_p,
               INTVAL num_entries)>

Rotate the top N entries by one.  If C<N > 0>, the rotation is bubble
up, so the top most element becomes the Nth element.  If C<N < 0>, the
rotation is bubble down, so that the Nth element becomes the top most
element.

=cut

*/

void
rotate_entries(Interp *interp,
               Stack_Chunk_t **stack_p /*NN*/,
               INTVAL num_entries)
{
    Stack_Chunk_t * const stack = *stack_p;
    INTVAL depth = num_entries - 1;

    if (num_entries >= -1 && num_entries <= 1) {
        return;
    }


    if (num_entries < 0) {
        INTVAL i;
        Stack_Entry_t temp;

        num_entries = -num_entries;
        depth = num_entries - 1;

        if (stack_height(interp, stack) < (size_t)num_entries) {
            internal_exception(ERROR_STACK_SHALLOW, "Stack too shallow!");
        }

        temp = *stack_entry(interp, stack, depth);
        for (i = depth; i > 0; i--) {
            *stack_entry(interp, stack, i) =
                *stack_entry(interp, stack, i - 1);
        }

        *stack_entry(interp, stack, 0) = temp;
    }
    else {
        INTVAL i;
        Stack_Entry_t temp;

        if (stack_height(interp, stack) < (size_t)num_entries) {
            internal_exception(ERROR_STACK_SHALLOW, "Stack too shallow!");
        }
        temp = *stack_entry(interp, stack, 0);
        for (i = 0; i < depth; i++) {
            *stack_entry(interp, stack, i) =
                *stack_entry(interp, stack, i + 1);
        }

        *stack_entry(interp, stack, depth) = temp;
    }
}

/*

=item C<void
stack_push(Interp *interp, Stack_Chunk_t **stack_p,
           void *thing, Stack_entry_type type, Stack_cleanup_method cleanup)>

Push something on the generic stack.

Note that the cleanup pointer, if non-C<NULL>, points to a routine
that'll be called when the entry is removed from the stack. This is
handy for those cases where you need some sort of activity to take place
when an entry is removed, such as when you push a lexical lock onto the
call stack, or localize (or tempify, or whatever we're calling it)
variable or something.

=cut

*/

void
stack_push(Interp *interp, Stack_Chunk_t **stack_p /*NN*/,
           void *thing, Stack_entry_type type, Stack_cleanup_method cleanup)
{
    Stack_Entry_t *entry = (Stack_Entry_t *)stack_prepare_push(interp, stack_p);

    /* Remember the type */
    entry->entry_type = type;
    /* Remember the cleanup function */
    entry->cleanup = cleanup;
    /* Store our thing */
    switch (type) {
        case STACK_ENTRY_INT:
        case STACK_ENTRY_MARK:
            UVal_int(entry->entry) = *(INTVAL *)thing;
            break;
        case STACK_ENTRY_FLOAT:
            UVal_num(entry->entry) = *(FLOATVAL *)thing;
            break;
        case STACK_ENTRY_ACTION:
        case STACK_ENTRY_PMC:
            UVal_pmc(entry->entry) = (PMC *)thing;
            break;
        case STACK_ENTRY_STRING:
            UVal_str(entry->entry) = (String *)thing;
            break;
        case STACK_ENTRY_POINTER:
        case STACK_ENTRY_DESTINATION:
            UVal_ptr(entry->entry) = thing;
            break;
        default:
            internal_exception(ERROR_BAD_STACK_TYPE,
                    "Invalid Stack_Entry_type!");
            break;
    }
}

/*

=item C<void *
stack_pop(Interp *interp, Stack_Chunk_t **stack_p,
          void *where, Stack_entry_type type)>

Pop off an entry and return a pointer to the contents.

=cut

*/

void *
stack_pop(Interp *interp, Stack_Chunk_t **stack_p /*NN*/,
          void *where, Stack_entry_type type)
{
    Stack_Entry_t * const entry = (Stack_Entry_t *)stack_prepare_pop(interp, stack_p);

    /* Types of 0 mean we don't care */
    if (type && entry->entry_type != type) {
        internal_exception(ERROR_BAD_STACK_TYPE,
                           "Wrong type on top of stack!\n");
    }

    /* Cleanup routine? */
    if (entry->cleanup != STACK_CLEANUP_NULL) {
        (*entry->cleanup) (interp, entry);
    }

    /* Sometimes the caller doesn't care what the value was */
    if (where == NULL) {
        return NULL;
    }

    /* Snag the value */
    switch (type) {
    case STACK_ENTRY_MARK:
    case STACK_ENTRY_INT:
        *(INTVAL *)where   = UVal_int(entry->entry);
        break;
    case STACK_ENTRY_FLOAT:
        *(FLOATVAL *)where = UVal_num(entry->entry);
        break;
    case STACK_ENTRY_ACTION:
    case STACK_ENTRY_PMC:
        *(PMC **)where     = UVal_pmc(entry->entry);
        break;
    case STACK_ENTRY_STRING:
        *(String **)where  = UVal_str(entry->entry);
        break;
    case STACK_ENTRY_POINTER:
    case STACK_ENTRY_DESTINATION:
        *(void **)where    = UVal_ptr(entry->entry);
        break;
    default:
        internal_exception(ERROR_BAD_STACK_TYPE,
                           "Wrong type on top of stack!\n");
        break;
    }

    return where;
}

/*

=item C<void *
pop_dest(Interp *interp)>

Pop off a destination entry and return a pointer to the contents.

=cut

*/

void *
pop_dest(Interp *interp)
{
    /* We don't mind the extra call, so we do this: (previous comment
     * said we *do* mind, but I say let the compiler decide) */
    void *dest;
    (void)stack_pop(interp, &interp->dynamic_env,
                    &dest, STACK_ENTRY_DESTINATION);
    return dest;
}

/*

=item C<void *
stack_peek(Interp *interp, Stack_Chunk_t *stack_base,
           Stack_entry_type *type)>

Peek at stack and return pointer to entry and the type of the entry.

=cut

*/

void *
stack_peek(Interp *interp, Stack_Chunk_t *stack_base /*NN*/,
           Stack_entry_type *type /*NULLOK*/)
{
    Stack_Entry_t * const entry = stack_entry(interp, stack_base, 0);
    if (entry == NULL) {
        return NULL;
    }

    if (type != NULL) {
        *type = entry->entry_type;
    }
    switch (entry->entry_type) {
        case STACK_ENTRY_POINTER:
        case STACK_ENTRY_DESTINATION:
            return UVal_ptr(entry->entry);
        default:
            return (void *) UVal_pmc(entry->entry);
    }
}

/*

Returns the stack entry type of C<entry>.

*/

Stack_entry_type
get_entry_type(Interp *interp, const Stack_Entry_t *entry /*NN*/)
{
    return entry->entry_type;
}

/*

=item C<void
Parrot_dump_dynamic_environment(Interp *interp,
                                Stack_Chunk_t *dynamic_env)>

Print a representation of the dynamic stack to the standard error (using
C<PIO_eprintf>).  This is used only temporarily for debugging.

=cut

*/

void
Parrot_dump_dynamic_environment(Interp *interp,
                                Stack_Chunk_t *dynamic_env)
{
    int height = (int) stack_height(interp, dynamic_env);

    while (dynamic_env->prev != dynamic_env) {
        const Stack_Entry_t * const e = stack_entry(interp, dynamic_env, 0);
        if (! e)
            internal_exception(1, "Control stack damaged");

        PIO_eprintf(interp, "[%4d:  chunk %p entry %p "
                                 "type %d cleanup %p]\n",
                    height, dynamic_env, e,
                    e->entry_type, e->cleanup);
        if (e->entry_type == STACK_ENTRY_PMC
                || e->entry_type == STACK_ENTRY_ACTION) {
            PMC * const thing = UVal_pmc(e->entry);

            PIO_eprintf(interp, "[        PMC %p type %d => %Ss]\n",
                        thing, thing->vtable->base_type,
                        VTABLE_get_string(interp, thing));
        }
        else if (e->entry_type == STACK_ENTRY_MARK) {
            PIO_eprintf(interp, "[        mark %d]\n",
                        UVal_int(e->entry));
        }
        dynamic_env = dynamic_env->prev;
        height--;
    }
    PIO_eprintf(interp, "[%4d:  chunk %p %s base]\n",
                height, dynamic_env, dynamic_env->name);
}

/*

=back

=head1 SEE ALSO

F<include/parrot/stacks.h>, F<include/parrot/enums.h>, and
F<src/stack_common.c>

=cut

*/


/*
 * Local variables:
 *   c-file-style: "parrot"
 * End:
 * vim: expandtab shiftwidth=4:
 */
