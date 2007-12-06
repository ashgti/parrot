/*
Copyright (C) 2001-2007, The Perl Foundation.
$Id$

=head1 NAME

src/io/io_layers.c - IO Layer Handling

=head1 DESCRIPTION

The Parrot IO subsystem uses a per-interpreter stack to provide a
layer-based approach to IO. Each layer implements a subset of the
C<ParrotIOLayerAPI> vtable. To find an IO function the layer
stack is searched downwards until a non-C<NULL> function pointer
is found for that particular slot.

=head2 Layer and Stack Functions

=over 4

=cut

*/

#include "parrot/parrot.h"
#include "parrot/io.h"
#include "io_private.h"

/* HEADERIZER HFILE: include/parrot/io.h */

/*

=item C<PARROT_API
PARROT_MALLOC
PARROT_CANNOT_RETURN_NULL
ParrotIOLayer *
PIO_base_new_layer(NULLOK(ParrotIOLayer *proto))>

The default IO layer constructor. Creates and returns a new
C<ParrotIOLayer>. If a prototype C<*proto> is supplied then its values
will be copied to the new instance.

=cut

*/

PARROT_API
PARROT_MALLOC
PARROT_CANNOT_RETURN_NULL
ParrotIOLayer *
PIO_base_new_layer(NULLOK(ParrotIOLayer *proto))
{
    ParrotIOLayer * const new_layer = mem_allocate_typed(ParrotIOLayer);

    /*
     * XXX use managed memory here ?
     */
    if (proto) {
        /* FIXME: Flag here to indicate whether to free strings */
        new_layer->name  = proto->name;
        new_layer->flags = proto->flags;
        new_layer->api   = proto->api;
    }
    else {
        new_layer->name = NULL;
        new_layer->flags = 0;
        new_layer->api = NULL;
    }
    new_layer->self = 0;
    new_layer->up = NULL;
    new_layer->down = NULL;
    return new_layer;
}

/*

=item C<PARROT_API
void
PIO_base_delete_layer(NULLOK(ParrotIOLayer *layer))>

The default IO layer destructor. Frees the memory associated with
C<*layer>.

=cut

*/

PARROT_API
void
PIO_base_delete_layer(NULLOK(ParrotIOLayer *layer))
{
    if (layer != NULL)
        mem_sys_free(layer);
}

/*

=item C<PARROT_API
INTVAL
PIO_push_layer(PARROT_INTERP, NULLOK(PMC *pmc), NULLOK(ParrotIOLayer *layer))>

Push a layer onto an IO object (C<*pmc>) or the default stack.

=cut

*/

PARROT_API
INTVAL
PIO_push_layer(PARROT_INTERP, NULLOK(PMC *pmc), NULLOK(ParrotIOLayer *layer))
{
    if (layer == NULL)
        return -1;

    if (!PMC_IS_NULL(pmc)) {
        ParrotIOLayer    *t;
        ParrotIO * const io = PMC_data_typed(pmc, ParrotIO *);

        if (!io)
            return -1;

        /* Error( 1st layer must be terminal) */
        if (io->stack == NULL && (layer->flags & PIO_L_TERMINAL) == 0)
            return -1;

        /* Check and see if this layer already is on stack
         * This is a internals sanity check not a user level
         * check, at least until I fix copy-on-write stacks.
         * -Melvin
         */
        for (t = io->stack; t; t = t->down) {
            if (t == layer)
                return -1;
        }

        /* if this is a global layer create a copy first */
        if (!(io->stack->flags & PIO_L_LAYER_COPIED))
            io->stack = PIO_copy_stack(io->stack);

        layer->down = io->stack;

        if (io->stack)
            io->stack->up = layer;

        io->stack = layer;

        PMC_struct_val(pmc) = layer;

        if (layer->api->Pushed)
            (*layer->api->Pushed) (layer, io);
    }
    else {
        ParrotIOLayer *t;
        ParrotIOData * const d = interp->piodata;
        if (d->default_stack == NULL && (layer->flags & PIO_L_TERMINAL) == 0) {
            /* Error( 1st layer must be terminal) */
            return -1;
        }
        /* Sanity check */
        for (t = d->default_stack; t; t = t->down) {
            if (t == layer)
                return -1;
        }

        layer->down = d->default_stack;
        if (d->default_stack)
            d->default_stack->up = layer;
        d->default_stack = layer;
        return 0;
    }
    return -1;
}

/*

=item C<PARROT_WARN_UNUSED_RESULT
PARROT_CAN_RETURN_NULL
PARROT_API
ParrotIOLayer *
PIO_get_layer(SHIM_INTERP, NOTNULL(const char *name))>

RT#48260: Not yet documented!!!

=cut

*/

PARROT_WARN_UNUSED_RESULT
PARROT_CAN_RETURN_NULL
PARROT_API
ParrotIOLayer *
PIO_get_layer(SHIM_INTERP, NOTNULL(const char *name))
{
    ParrotIOLayer **t;

    for (t = pio_registered_layers; *t; ++t)
        if (strcmp(name, (*t)->name) == 0)
            return *t;
    return NULL;
}

/*

=item C<void
PIO_push_layer_str(PARROT_INTERP, NOTNULL(PMC *pmc), NULLOK(STRING *ls))>

Push a layer onto an IO object (C<*pmc>).

=cut

*/

void
PIO_push_layer_str(PARROT_INTERP, NOTNULL(PMC *pmc), NULLOK(STRING *ls))
{
    char * const cls = string_to_cstring(interp, ls);
    ParrotIOLayer * const l = PIO_get_layer(interp, cls);
    ParrotIOLayer * newlayer;

    string_cstring_free(cls);
    if (!l)
        real_exception(interp, NULL, 1, "Layer not found");

    /* make private copy */
    newlayer = PIO_base_new_layer(l);
    newlayer->flags |= PIO_L_LAYER_COPIED;
    PIO_push_layer(interp, pmc, newlayer);
}

/*

=item C<PARROT_API
PARROT_IGNORABLE_RESULT
PARROT_CAN_RETURN_NULL
ParrotIOLayer *
PIO_pop_layer(PARROT_INTERP, NULLOK(PMC *pmc))>

Pop a layer from an IO object (C<*pmc>) or the default stack.

=cut

*/

PARROT_API
PARROT_IGNORABLE_RESULT
PARROT_CAN_RETURN_NULL
ParrotIOLayer *
PIO_pop_layer(PARROT_INTERP, NULLOK(PMC *pmc))
{
    ParrotIO * const io = PMC_data_typed(pmc, ParrotIO *);

    if (!PMC_IS_NULL(pmc)) {
        ParrotIOLayer *layer;

        if (!io)
            return 0;

        /* if this is a global layer create a copy first */
        if (!(io->stack->flags & PIO_L_LAYER_COPIED))
            io->stack = PIO_copy_stack(io->stack);

        layer = io->stack;

        if (layer) {
            io->stack           = layer->down;
            PMC_struct_val(pmc) = io->stack;
            io->stack->up       = 0;
            layer->up           = 0;
            layer->down         = 0;

            if (layer->api->Popped)
                (*layer->api->Popped) (layer, io);

            return layer;
        }

        return layer;
    }
    /* Null io object - use default stack */
    else {
        ParrotIOData  * const d     = interp->piodata;
        ParrotIOLayer * const layer = d->default_stack;

        if (layer) {
            d->default_stack     = layer->down;
            d->default_stack->up = NULL;
            layer->up            = 0;
            layer->down          = 0;

            return layer;
        }
    }

    return NULL;
}

/*

=item C<PARROT_IGNORABLE_RESULT
PARROT_CANNOT_RETURN_NULL
STRING *
PIO_pop_layer_str(PARROT_INTERP, NOTNULL(PMC *pmc))>

Pop a layer from an IO object (C<*pmc>) and return the name of the
popped layer. The layer gets freed.

=cut

*/

PARROT_IGNORABLE_RESULT
PARROT_CANNOT_RETURN_NULL
STRING *
PIO_pop_layer_str(PARROT_INTERP, NOTNULL(PMC *pmc))
{
    ParrotIOLayer * const layer = PIO_pop_layer(interp, pmc);
    STRING * const ls = string_make(interp, layer->name,
                                    strlen(layer->name), "iso-8859-1", 0);
    mem_sys_free(layer);
    return ls;
}

/*

=item C<PARROT_API
PARROT_IGNORABLE_RESULT
PARROT_CANNOT_RETURN_NULL
ParrotIOLayer *
PIO_copy_stack(NULLOK(ParrotIOLayer *stack))>

Primarily used to copy the default IO stack for a new IO object. Later
we will do some funky copy-on-write stuff.

=cut

*/

PARROT_API
PARROT_IGNORABLE_RESULT
PARROT_CANNOT_RETURN_NULL
ParrotIOLayer *
PIO_copy_stack(NULLOK(ParrotIOLayer *stack))
{
    ParrotIOLayer *ptr_new = NULL;
    ParrotIOLayer *ptr_last = NULL;
    ParrotIOLayer **ptr_ptr_new = &ptr_new;

    while (stack) {
        *ptr_ptr_new = PIO_base_new_layer(stack);
        (*ptr_ptr_new)->flags |= PIO_L_LAYER_COPIED;
        (*ptr_ptr_new)->up = ptr_last;
        stack = stack->down;
        ptr_last = *ptr_ptr_new;
        ptr_ptr_new = &((*ptr_ptr_new)->down);
    }

    return ptr_new;
}


/*
 * Local variables:
 *   c-file-style: "parrot"
 * End:
 * vim: expandtab shiftwidth=4:
 */
