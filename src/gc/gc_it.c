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

/* Open Questions:
1) Should GC headers exist in the small object pools, or should they be
   separate? I could create a separate pool for GC headers, managed manually
   by the GC.
2) Should the Arenas object have a fourth pointer for an initialization
   function? Alternatively, reuse the current deinit function pointer
   with an optional init/deinit flag. This makes the GC more dynamic.
*/

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
    Arenas * const arena_base = interp->arena_base;

    /*
     * TODO free generation structures without moving or destroying
     * any objects. Maybe need to free GC headers?
     */
    mem_sys_free(arena_base->gc_private);
    arena_base->gc_private = NULL;
}

/*

=item C<Parrot_gc_in_pool_init>

Initialize a new memory pool. Set the pointers to the necesary functions,
and set the size of the objects to include the GC header.

=cut

*/

static void
Parrot_gc_it_pool_init(PARROT_INTERP, ARGMOD(Small_Object_Pool *pool))
{
    /* Set up function pointers for pool. */
    pool->add_free_object = gc_it_add_free_object;
    pool->get_free_object = gc_it_get_free_object;
    pool->alloc_objects   = gc_it_alloc_objects;
    pool->more_objects    = gc_it_more_objects;

    /* initialize generations */
    gc_gms_init_gen(interp, pool);
    pool->white = pool->white_fin = pool->free_list = &pool->marker;

    /* Increase allocated space to account for GC header */
    pool->object_size += sizeof (Gc_it_hdr);
}

/*

=item C<gc_it_add_free_object>

Adds a dead item to the free list. GC GMS doesnt use this, and I'm
not entirely certain what I should do with it either.

=cut

*/

static void
gc_it_add_free_object(PARROT_INTERP, SHIM(Small_Object_Pool *pool),
        SHIM(PObj *to_add))
{
    real_exception(interp, NULL, 1, "GC does not support add_free_object");
}

/*

=item C<gc_it_get_free_object>

Get an object from the pool's free list and return a pointer to it.

=cut

*/

PARROT_WARN_UNUSED_RESULT
PARROT_CANNOT_RETURN_NULL
static PObj *
gc_it_get_free_object(PARROT_INTERP, ARGMOD(Small_Object_Pool *pool))
{
    PObj *ptr;
    Gc_it_hdr *hdr;

    hdr = pool->free_list;
    if (hdr == &pool->marker)       /* At the end of the pool, allocate new arena */
        (pool->more_objects)(interp, pool);

    hdr = pool->free_list;
    pool->free_list = hdr->next;
    hdr->gen = pool->gc_it_data->last_gen;
    ptr = IT_HDR_to_PObj(hdr);
    PObj_flags_SETTO((PObj*) ptr, 0);
    return ptr;
}

/*

=item C<static void gc_it_alloc_objects>

Allocate a new Small_Object_Arena from the OS. Set up the arena as
necessary.

=cut

*/

static void
gc_it_alloc_objects(PARROT_INTERP, ARGMOD(Small_Object_Pool *pool))
{
    const size_t real_size = pool->object_size;
    const size_t size = real_size * pool->objects_per_alloc;
    Small_Object_Arena * const new_arena =
        mem_internal_allocate(sizeof (Small_Object_Arena));
    /* Allocate a space with enough storage for objects_per_alloc objects */
    new_arena->start_objects = mem_internal_allocate(size);
    /* insert new_arena in pool's arena linked list */
    Parrot_append_arena_in_pool(interp, pool, new_arena, size);

    /* GC GMS currently creates a ring or "chain" of objects. I
     * Don't know if i'm going to use the same. This function
     * call sets that up, but I might not keep it.
    gc_gms_chain_objects(interp, pool, new_arena, real_size);
     */

    /* allocate more next time */
    pool->objects_per_alloc = (UINTVAL) pool->objects_per_alloc *
        UNITS_PER_ALLOC_GROWTH_FACTOR;
    size = real_size * pool->objects_per_alloc;
    if (size > POOL_MAX_BYTES) {
        pool->objects_per_alloc = POOL_MAX_BYTES / real_size;
    }
}

/*

=item C<static void gc_it_more_objects>

Run a GC cycle or allocate new objects for the given pool.

Currently appears to do nothing under the following circumstances:
pool->free_list != &pool->marker AND
1) Pool->skip == 0 && pool->last_Arena == NULL
2) pool->skip == 1

I have to figure out why this would be the case and correct it if
needed.

=cut

*/

static void
gc_it_more_objects(PARROT_INTERP, ARGMOD(Small_Object_Pool *pool))
{
    if (pool->skip)
        pool->skip = 0;
    else if (pool->last_Arena) {
        Parrot_do_dod_run(interp, DOD_trace_stack_FLAG);
        if (pool->num_free_objects <= pool->replenish_level)
            pool->skip = 1;
    }

    if (pool->free_list == &pool->marker) {
        (*pool->alloc_objects) (interp, pool);
    }
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
