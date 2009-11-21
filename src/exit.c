/*
Copyright (C) 2001-2009, Parrot Foundation.
$Id$

=head1 NAME

src/exit.c - Exit Handling

=head1 DESCRIPTION

Parrot's version of C<exit()>, C<on_exit()>, and friends.

C<Parrot_on_exit()> allows you register exit handlers which will be
called by C<Parrot_exit()> when the interpreter exits.

=head2 Functions

=over 4

=cut

*/

#include <stdlib.h>
#include "parrot/parrot.h"

/* HEADERIZER HFILE: include/parrot/exit.h */

/*

=item C<void Parrot_on_exit(PARROT_INTERP, exit_handler_f function, void *arg)>

Register the specified function to be called on exit.

=cut

*/

PARROT_EXPORT
void
Parrot_on_exit(PARROT_INTERP, NOTNULL(exit_handler_f function), ARGIN_NULLOK(void *arg))
{
    ASSERT_ARGS(Parrot_on_exit)

    handler_node_t * const new_node = mem_allocate_typed(handler_node_t);

    new_node->function        = function;
    new_node->arg             = arg;
    new_node->next            = interp->exit_handler_list;
    interp->exit_handler_list = new_node;
}

/*

=item C<void Parrot_exit(PARROT_INTERP, int status)>

Exit, calling any registered exit handlers.

=cut

*/

PARROT_EXPORT
PARROT_DOES_NOT_RETURN
void
Parrot_exit(PARROT_INTERP, int status)
{
    ASSERT_ARGS(Parrot_exit)
    /* call all the exit handlers */
    /* we are well "below" the runloop now, where lo_var_ptr
     * is set usually - exit handlers may run some resource-hungry
     * stuff like printing profile stats - a GC run would kill
     * resources
     * http://rt.perl.org/rt3/Ticket/Display.html?id=46405 (resolved)
     */
    /*
     * we don't allow new exit_handlers being installed inside exit handlers
     * - do we?
     * and: interp->exit_handler_list is gone, after the last exit handler
     *      (Parrot_really_destroy) has run
     */
    handler_node_t *node = interp->exit_handler_list;

    Parrot_block_GC_mark(interp);
    Parrot_block_GC_sweep(interp);

    while (node) {
        handler_node_t * const next = node->next;

        (node->function)(interp, status, node->arg);
        mem_sys_free(node);
        node = next;
    }

    exit(status);
}

/*

=back

=head1 SEE ALSO

F<include/parrot/exit.h> and F<t/src/exit.t>.

=head1 HISTORY

Initial version by Josh Wilmes.

=cut

*/


/*
 * Local variables:
 *   c-file-style: "parrot"
 * End:
 * vim: expandtab shiftwidth=4:
 */
