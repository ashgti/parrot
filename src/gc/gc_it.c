/*
Copyright (C) 2001-2008, The Perl Foundation.
$Id: gc_ims.c 27452 2008-05-12 02:15:12Z Whiteknight $

=head1 NAME

src/gc/gc_it.c - Incremental Tricolor Garbage Collector

=head1 DESCRIPTION

This garbage collector, as described in PDD09, will use a tricolor
incremental marking scheme. More details to be fleshed out later.

=head1 NOTES

This file is under heavy manipulation, and it isn't going to be the
prettiest or most standards-compliant code for now. We can add
spit and polish later.

ALL YOUR DOCUMENTATION ARE BELONG TO HERE.
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

    /* Create our private data. We might need to initialize some things
    here, depending on the data we include in this structure */
    arena_base->gc_private        = mem_allocate_zeroed_typed(Gc_it_data);
    arena_base->gc_private->num_generations = 0;
    arena_base->gc_private->flags = GC_IT_FLAG_NEW;

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


/*
 * Basic Algorithm:
 * 1) Determine which pool/generation to scan
 * 2) Mark root items as being grey
 * 3) For all grey items, mark all children as grey, then mark item as black
 * 4) repeat (3) until there are no grey items in current pool/generation
 * 5) mark all objects grey that appear in IGP lists. Repeat (1) - (4) for these
 * 6) Add all items that are still white to the free list
 * 7) Scan through simple buffers, add white objects to free list
 * 8) reset all flags to white
 *
 * Only reclaim objects where there are no greys in the current area,
 * and when a run has completed. Do not reclaim, for instance, any white
 * objects after a run has completed and all black objects have been turned
 * back to white (which would free all objects, alive or dead).
 */

void
Parrot_gc_it_run(PARROT_INTERP, int flags)
{
/* Check flags, figure out our mode of operation. Here are some:

    GC_trace_normal = run a trace, or a partial trace, but not a sweep.
        set the macro CHECK_IF_WE_BREAK_BEFORE_SWEEP, however we do that,
        to break off the function after the trace phase.
    GC_trace_stack_FLAG = we need to trace the C stack and the processor
        registers too. I wouldn't even know how to start doing this.
    GC_lazy_FLAG = Appears we run a sweep phase if we have already marked,
        or complete the current mark and then sweep. Or, we find some other
        way to free items that are obviously dead.
    GC_finish_FLAG = set all objects to dead, call all finalizers on PMC
        objects. We don't need to mark free items, since everything is going
        to die anyway.

    If GC_trace_normal && (GC_IT_FLAG_NEW_MARK || GC_IT_FLAG_RESUME_MARK)
        run a trace or resume the current trace.

    if GC_trace_normal && (GC_IT_FLAG_NEW_SWEEP || GC_IT_FLAG_RESUME_SWEEP)
        do nothing

    if GC_lazy_FLAG && (GC_IT_FLAG_NEW_SWEEP || GC_IT_FLAG_RESUME_SWEEP)
        run a sweep, possibly to conclusion.

    if GC_finish_FLAG
        run a complete sweep of the PMC area calling finalizers only but not
        marking objects or adding objects to the free list
*/

    Arenas * arena_base = interp->arena_base;
    Gc_it_data * gc_priv_data = (Gc_it_data)(arena_base->gc_private);
    Gc_it_state state = gc_priv_data->state;
    gc_it_config * config = &(gc_priv_data->config);

    if (arena_base->DOD_block_level)
        return;
    ++arena_base->DOD_block_level;
    if(flags & GC_finish_FLAG) {
        gc_it_finalize_all_pmc(interp);
        --arena_base->DOD_block_level;
    }
    if(flags & GC_trace_normal) {
        if(state == GC_IT_STATE_NEW_SWEEP || state == GC_IT_STATE_RESUME_SWEEP) {
            /* mark is finished, maintain state, wait to run a sweep */
            --arena_base->DOD_block_level;
            return
        }
        if(gc_it_trace_normal(interp)) {
            /* trace has been run as requested, return */
            --arena_base->DOD_block_level;
            return;
        }
    }
    if(flags & GC_lazy_FLAG) {
        if(state == GC_IT_STATE_NEW_SWEEP || state == GC_IT_STATE_RESUME_SWEEP)
            gc_it_sweep_normal(interp);
        --arena_base->DOD_block_level;
        return;
    }
}

void
gc_it_trace_normal(PARROT_INTERP)
{
    Arenas * const arena_base = interp->arena_base;
    Gc_it_data * const gc_priv_data = arena_base->gc_private;
    Gc_it_hdr * cur_item;
    Gc_it_pool_data * pool_data = cur_pool->gc_it_data;

/*
 * 1) Determine which pool/generation to scan
 * All globals are added to the queue, it might not make a lot of sense
 * to try to differentiate between items in different generations here.
 * At the very least, we want to scan outgoing links from the older generations,
 * in case some of them point to the current generation that we are scanning.
 */

/*
 * 2) Mark root items as grey
 * I don't currently know how to determine which items are root. However,
 * When we find them, we can mark them
 *
 * 5) mark all objects grey that appear in incoming IGP lists.
 * I think we need to do this by adding all incoming IGP pointers to the
 * queue here at the beginning, and then processing the whole set.
 */

    if(gc_priv_data->state == GC_IT_START_MARK) {
        gc_priv_data->total_count = 0;   /* number of elements scanned this run */
        gc_it_enqueue_globals(interp);   /* Do point #2 */
        gc_it_enqueue_igp(interp);       /* Do point #5 */
        gc_priv_data->state++; /* set state to "resume" */
    }
    gc_priv_data->item_count = 0; /* reset per-increment count */

/*
 * 3) for all grey items, mark children as grey. Then mark as black
 * I'll call some kind of inline-able sub-function here.
 * We need to keep track of items that require finalization, that might
 * mean a separate queue and a separate loop, or a flag somewhere that we
 * need to test and preserve.
 *
 * 4) Repeat (3) until there are no grey items in current pool/generation
 * At any point, we should be able to break out of this loop at any time,
 * because state is stored in the white/grey/black lists and in the bitmap,
 * which is kept across runs.
 */

    while(cur_item = cur_pool->gray) {
        /* All the macros defined here should not really include function
        calls unless there is a lot of work to do. If possible, we should
        try to keep this efficient, remembering that we are looping over
        a potentially large number of items */

        /*
        Move the children's headers into the queue, and possibly mark them
        grey as well.
        */
#define GC_IT_MARK_CHILDREN_GREY(x, y) gc_it_mark_children_grey(x, y)
        GC_IT_MARK_CHILDREN_GREY(cur_pool, cur_item);

        /* Mark the current node's card black, and return it to the list of
        all items */
#define GC_IT_MARK_NODE_BLACK(x, y) gc_it_mark_node_black(x, y)
        GC_IT_MARK_NODE_BLACK(cur_pool, cur_item);

        gc_priv_data->item_count++;
        gc_priv_data->total_count++;
        /* TODO Determine if we need to stop the sweep, set gc_priv_data->stop
           if we need to take a break */
        if(GC_IT_NEED_TO_BREAK_AFTER_N_INCREMENTS) {
            gc_priv_data->state = GC_IT_RESUME_MARK;
            return;
        }
    }
}

void
gc_it_sweep_normal(PARROT_INTERP)
{

    const Arenas * arena_base = interp->arena_base;
    Gc_it_data * gc_priv_data = arena_base->gc_private;

/*
 * 6) move all white objects to the free list
 * After we have traced all incoming IGP and run through the pool, all remaining
 * white objects can be freed. Do that, or enqueue it so it can be done later.
 * Also, finalize any items that require it.
 */

    if(gc_priv_data->state == GC_IT_NEW_SWEEP) {
        gc_it_free_white_items(interp);
        if(NEED_TO_STOP_AFTER_WHITE_SWEEP) {
            gc_priv_data->state = GC_IT_RESUME_SWEEP;
            return;
        }
    }

/*
 * 7) Scan through simple buffers, add white objects to free list
 * Buffers don't contain pointers, so we don't need to perform a
 * full tree-based scan. Find the ones that are not marked at this
 * point and free them.
 */

    gc_it_sweep_simple_buffers(interp);

/*
 * 8) reset all flags to white
 * Reset the whole card to white. All items should be out of the grey list, and
 * back in the generic "items" list. All newly created items should be appended
 * into the "items" list.
 * Reset all flags and stuff for a new scan.
 */

    gc_it_reset_cards(interp);
    gc_priv_data->state = GC_IT_NEW_MARK;
}

void
gc_it_enqueue_globals(PARROT_INTERP)
{
    /* find globals. Make sure they all have headers (I don't know what
       they are going to look like here). Add those headers to the grey
       grey list queue. */
}

void gc_it_enqueue_igp(PARROT_INTERP)
{
    /* Find all incoming IGP to the currently scanned pool/arena/whatever
       and add them to the grey pile. This way, we ensure that they get
       processed in a timely manner, and we don't need to use two loops
       or anything funny.
       */
    /* I'm not sure about the arguments that this function will require,
       I'm sure it doesnt need to be the whole damn iterpreter structure
       unless these IGPs get way out of hand. */
}

void
gc_it_mark_children_grey(Small_Object_Pool * pool, Gc_it_hdr * obj)
{
    /* Add all children of the current node to the queue for processing.
    Also we might want to mark the card grey as well (although that
    seems like unnecessary work) */
    /* This function could become a macro or an inline function*/
}

inline void
gc_it_mark_node_black(Small_Object_Pool * pool, Gc_it_hdr * obj)
{
    /* Mark the current object black. It should already be grey.
    do an OR with PObj_is_fully_marked_FLAG. We could warn about an
    object here that is white instead of grey, but that should never
    happen, and will waste an entire conditional.

    move the node from the queue to the items list, or the finalize list
    if that's what's needed (we might need to use a separate function
    or something). */
    /* This function should probably become a macro or an inline function */
}

inline void
gc_it_mark_node_grey(Small_Object_Pool * pool, Gc_it_hdr * obj)
{
    /* Mark the current node as grey, add it to the grey queue */
    /* This function might become a macro or an inline function */
}

void
gc_it_free_white_items(PARROT_INTERP)
{
    /* Add all items which are still white to the free lists.
       Possibly also clear the bitmaps */
    /* This can be a macro or an inline function */
}

void gc_it_clear_cards(PARROT_INTERP)
{
    /* If we don't do it in gc_it_free_white_items, or somewhere else
       then clear the cards here. Don't know what all that's going to
       entail. */
    /* This can be a macro or an inline function */
}

void gc_it_finalize_all_pmc(PARROT_INTERP)
{
    /* Run through the PMC pool here, call all finalize methods on PMCs
       that have them.
       At this point, we may have performed a mark but not a sweep, so some
       PMCs that are still "white" might need finalization. Go through
       all items, (white, black and grey) and finalize all. Don't bother
       adding items to free list here, the program is terminating. */
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
    /* Algorithm:
     * 1) Free all generation structures
     * 2) Free all GC headers in all pools and arenas, if possible
     * 3) Free any additional memory that i will create in the future
     */
    mem_sys_free(arena_base->gc_private);
    arena_base->gc_private        = NULL;
    /* Null-out the function pointers, except the init pointer
       who knows? the interp might want to load us up again. */
    arena_base->do_dod_run        = NULL;
    arena_base->de_init_gc_system = NULL;
    arena_base->init_pool         = NULL;
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
