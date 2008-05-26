/*
Copyright (C) 2001-2008, The Perl Foundation.
$Id: gc_ims.c 27452 2008-05-12 02:15:12Z Whiteknight $

=head1 NAME

src/gc/gc_it.c - Incremental Tricolor Garbage Collector

=head1 DESCRIPTION

This garbage collector, as described in PDD09, will use a tricolor 
incremental marking scheme. More details to be fleshed out later.
*/

#include "parrot/parrot.h"
#include "parrot/dod.h"

#if #PARROT_GC_IT

/*

=item C<Parrot_gc_it_init>

Initializes the GC, sets the necessary pointers in the interpreter.

=cut

*/

void
Parrot_gc_it_init(PARROT_INTERP)
{
    Arenas * const arena_base = interp->arena_base;
    arena_base->gc_private    = mem_allocate_zeroed_typed(Gc_ims_private);

    /* set function hooks according to pdd09 */

    arena_base->do_dod_run        = Parrot_gc_it_run;
    arena_base->de_init_gc_system = Parrot_gc_it_deinit;
    arena_base->init_pool         = Parrot_gc_it_pool_init;

}

/*

=item C<Parrot_gc_it_run>

Run the GC. Not sure what it means to "run" yet, might only run one 
increment, or one step from many.

=cut

*/

void
Parrot_gc_it_run(PARROT_INTERP, int flags)
{
}

/*

=item C<Parrot_gc_it_deinit>

Kill the GC and reset everything.

=cut

*/

void
Parrot_gc_it_deinit(PARROT_INTERP)
{
}

/*

=item C<Parrot_gc_in_pool_init>

Initialize a new memory pool.

=cut

*/

void
Parrot_gc_it_pool_init(PARROT_INTERP, struct Small_Object_Pool * sop)
{
}

#endif  /* PARROT_GC_IT */


/*

=head1 HISTORY

Initial version by Whiteknight (2008.05.26)

=cut

*/

/*
 * Local variables:
 *   c-file-style: "parrot"
 * End:
 * vim: expandtab shiftwidth=4:
 */
