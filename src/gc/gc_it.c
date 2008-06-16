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
    Gc_it_data * const gc_priv_data;
    /* Create our private data. We might need to initialize some things
    here, depending on the data we include in this structure */
    arena_base->gc_private        = mem_allocate_zeroed_typed(Gc_it_data);
    gc_priv_data = arena_base->gc_private;
    gc_priv_data->num_generations = 0;
    gc_priv_data->config = GC_IT_INITIAL_CONFIG /* define this later, if needed */
#if GC_IT_PARALLEL_MODE
    gc_priv_data->num_threads = 0;
#endif
    gc_priv_data->state = GC_IT_NEW_MARK;

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

/* Here is a (mostly-complete) list of stuff that needs to be traced
as a global. This tracing work should all get done in
src/gc/dod.c:Parrot_dod_trace_root.
1) interp->iglobals (an aggregate array PMC)
2) "system areas" (src/cpu_dep.c:trace_system_areas)
3) current context CONTEXT(interp)
4) dynamic stack (see src.stack.c:mark_stack)
5) vtables (see src/vtables.c:mark_vtables)
6) exceptions (interp->exception_list)
7) root namespace (interp->root_namespace)
8) The scheduler (interp->scheduler)
9) const subs (src/packfile.c:mark_const_subs)
10) caches (src/oo.c:mark_object_cache)
11) class hash (interp->class_hash)
12) DOD registry (interp->DOD_registry)
13) transaction log (src/stm/backend.c:Parrot_STM_mark_transaction)
14) IO Data (src/io/io.c:Parrot_IOData_mark)
15) IGP pointers (if we haven't done too much work already)
*/

/* Set these to "break" if we break after the given stage. Leave them empty
   if we should fall through to the next stage upon completion */
#define GC_IT_BREAK_AFTER_0
#define GC_IT_BREAK_AFTER_1
#define GC_IT_BREAK_AFTER_2
#define GC_IT_BREAK_AFTER_3
#define GC_IT_BREAK_AFTER_4
#define GC_IT_BREAK_AFTER_5
#define GC_IT_BREAK_AFTER_6
#define GC_IT_BREAK_AFTER_7

/* the number of items we must scan minimum before we move on. If we run a
   mark increment and do not meet this minimum, we enqueue the next root
   (if any) and continue */

#define GC_IT_ITEMS_MARKED_MIN  1

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

    const Arenas * const arena_base = interp->arena_base;
    Gc_it_data * const gc_priv_data = (Gc_it_data)(arena_base->gc_private);
    gc_it_config * const config = &(gc_priv_data->config);

    if(flags & GC_finish_FLAG) {
        gc_priv_data->state = GC_IT_RESUME_MARK;
        Parrot_dod_trace_root(interp) /* Add globals directly to the queue */
        gc_it_finalize_all_pmc(interp);
        gc_it_post_sweep_cleanup(interp);
        gc_priv_data->state = GC_IT_FINAL_CLEANUP;
        return;
    }
    gc_priv_data->item_count = 0; /* items scanned this run */
    switch (gc_priv_data->state) {
        case GC_IT_READY:
            gc_priv_data->state = GC_IT_START_MARK;
            GC_IT_BREAK_AFTER_0;

        case GC_IT_START_MARK:
            gc_priv_data->total_count = 0;
            gc_priv_data->state = GC_IT_MARK_ROOTS;
            GC_IT_BREAK_AFTER_1;

        case GC_IT_MARK_ROOTS:
            Parrot_dod_trace_root(interp);
            gc_priv_data->state = GC_IT_RESUME_MARK;
            GC_IT_BREAK_AFTER_2;

        case GC_IT_RESUME_MARK:
#if GC_IT_INCREMENT_MODE
            do {
                gc_it_enqueue_next_root(interp);
                gc_it_trace(interp);
            } while(gc_priv_data->item_count < GC_IT_ITEMS_MARKED_MIN &&
                    gc_priv_data->root_queue != NULL);
#elif GC_IT_BATCH_MODE
            gc_it_enqueue_all_roots(interp);
            gc_it_trace(interp)
#endif

            if(gc_priv_data->queue == NULL)
                gc_priv_data->state = GC_IT_END_MARK;
            GC_IT_BREAK_AFTER_3;

        case GC_IT_END_MARK:
            /* Don't know if there is anything to be done here */
            gc_priv_data->state == GC_IT_START_SWEEP;
            GC_IT_BREAK_AFTER_4;

        case GC_IT_START_SWEEP:
            gc_it_sweep_normal(interp);
            gc_priv_data->state == GC_IT_RESUME_SWEEP;
            GC_IT_BREAK_AFTER_5;

        case GC_IT_RESUME_SWEEP:
            gc_it_sweep_dead_objects(interp);
            GC_IT_BREAK_AFTER_6;

        case GC_IT_SWEEP_BUFFERS:
            gc_it_sweep_buffers(interp);
            gc_priv_data->state == GC_IT_FINAL_CLEANUP;
            GC_IT_BREAK_AFTER_7;

        case GC_IT_FINAL_CLEANUP:
            gc_it_reset_cards(interp);
            gc_it_post_sweep_cleanup(interp);
            gc_priv_data->state == GC_IT_READY;
    }
    return;

    /* I'm not sure how to deal with all these flags here, or how to integrate
       them into my state machine above. I'll work on this later.

    if(flags & GC_trace_normal)
    if(flags & GC_lazy_FLAG)

    */
}

#define GC_IT_MARK_NODE_BLACK(gc_data, hdr) \
    gc_it_mark_card((hdr), GC_IT_CARD_BLACK); \
    (gc_data)->queue = (hdr)->next; \
    (hdr)->next = NULL;
#define GC_IT_MARK_NODE_GREY(gc_data, hdr) \
    (hdr)->next = (gc_data)->queue; \
    (gc_data)->queue = (hdr);
#define GC_IT_MARK_CHILDREN_GREY(x, y) gc_it_mark_children_grey(x, y)

void
gc_it_trace(PARROT_INTERP)
{
    /* Trace the queue to completion. */

#if GC_IT_SERIAL_MODE

    /* Trace the queue here, return when the queue has been completely traced */
    gc_it_trace_normal(interp);

#elif GC_IT_PARALLEL_MODE

    /* start a thread to trace the queue, if the thread has not already been
       created. */
    gc_it_trace_threaded(interp);

#endif

}

#if GC_IT_SERIAL_MODE
void
gc_it_trace_normal(PARROT_INTERP)
{
    Gc_it_data * const gc_priv_data = interp->arena_base->gc_private;
    Gc_it_hdr * cur_item;

    while(cur_item = cur_pool->queue) {
        /* for each item, add all chidren to the queue, and then mark the item
           black. Once black, the item can be removed from the queue and
           discarded */
        GC_IT_MARK_CHILDREN_GREY(gc_priv_data, cur_item);
        GC_IT_MARK_NODE_BLACK(gc_priv_data, cur_item);
        gc_priv_data->total_count++; /* total items since beginning of scan */
        gc_priv_data->item_count++;  /* number of items this increment */
    }
}

inline static void
gc_it_sweep_dead_objects(PARROT_INTERP)
{
    const Arenas * const arena_base = interp->arena_base;
    Gc_it_data * const gc_priv_data = arena_base->gc_private;

/*
 * 6) move all white objects to the free list
 * Traverse each pool, and each arena in each pool. For each arena, take a
 * pointer to the card. Card items which are still white can be freed.
 * We set the card value to GC_IT_CARD_NEW, and move the header to the free
 * list. Items which are new, black, or grey can be ignored.
 */

}

inline static void
gc_it_sweep_simple_buffers(PARROT_INTERP)
{

/*
 * 7) Scan through simple buffers, add white objects to free list
 * Buffers don't contain pointers, so we don't need to perform a
 * full tree-based scan. Find the ones that are not marked at this
 * point and free them.
 */

}
#endif

#if GC_IT_PARALLEL_MODE
void
gc_it_trace_threaded(PARROT_INTERP)
{
    /* Check the current number of threads running. If we have space, launch
       another thread to help with the mark. This is low priority, so we'll
       deal with it later. */
}

void
gc_it_mark_threaded(PARROT_INTERP)
{
    /* spawn a thread to perform the current mark phase */
}
#endif

#if GC_IT_BATCH_MODE
inline static void
gc_it_enqueue_all_roots(PARROT_INTERP)
{
    /* We've already found all the roots and if we are working in batch
       mode we just take the list we've already gotten as the queue. */
    Gc_it_data * const gc_priv_data = interp->arena_base->gc_private;
    gc_priv_data->queue = gc_priv_data->root_queue;
    gc_priv_data->root_queue = NULL;
}
#endif

#if GC_IT_INCREMENT_MODE
static void
gc_it_enqueue_next_root(PARROT_INTERP)
{
    /* enqueue next root, algorithm:
        1) Get the next root item from gc_priv_data->root_queue
        2) if it's an aggregate item, add it to the queue and return
        3) if not, mark the item as live, remove from queue, and procede to
           next item in the list
        I probably need to double-check some of my loop logic, but this is a
        good start.
    */
    Gc_it_data * const gc_priv_data = interp->arena_base->gc_private;
    Gc_it_hdr * hdr = gc_priv_data->root_queue;

    do {
        gc_priv_data->root_queue = hdr->next;
        if(PObj_is_PMC_TEST(PObj_to_IT_HDR(hdr))) {
            /* add the item to the queue. return */
            hdr->next = gc_priv_data->queue;
            gc_priv_data->queue = hdr;
            return;
        }
        else {
            /* mark the buffer immediately, set the header to float, grab the
               next item from the root_queue */
            gc_it_set_card_mark(hdr, GC_IT_MARK_BLACK);
            hdr->next = NULL;
            hdr = gc_priv_data->root_queue;
        }
    } while(gc_priv_data->root_queue != NULL);
    /* If we've fallen through here, that means there are no more root objects,
       no more objects to mark, and we move on to the next stage */
    gc_priv_data->queue = NULL;
}
#endif

static inline void
gc_it_mark_children_grey(Small_Object_Pool * pool, Gc_it_hdr * hdr)
{
    /* Add all children of the current node to the queue for processing. */
    /* This function could become a macro or an inline function*/
    const PObj * obj = IT_HDR_to_PObj(hdr);
    const UINTVAL flags = PObj_get_FLAGs(obj);
    if(PObj_is_PMC_TEST(obj)) {
        /* add the metadata PMC, if it exists */
        if(PMC_metadata(obj))
            pobject_lives(interp, (PObj *)PMC_metadata(obj));
        /* add any headers that have been added by the system into the
           _next_for_GC field, since that seems like a place where the
           system wants me to look. */
        if(PMC_next_for_GC(obj) != obj) {
            /* loop through this list, add them all wherever they need to go.
               I dont know whether these items should be added to queue or
               the free list, so I won't implement anything here */
        }
    }
    if(flags & PObj_data_is_PMC_array_FLAG)
        Parrot_dod_trace_pmc_data(interp, obj);
    if(flags & PObj_custom_mark_FLAG)
        VTABLE_mark(interp, obj);
    /* If the item is shared, we need to do some magic trickery with it. I
       don't know if I'm going to do said trickery here, or offload it to a
       function like src/gc/dod.c:mark_special (which is where some of the
       other logic in this function originated) */
}

static void
gc_it_reset_cards(PARROT_INTERP)
{
    /*
     * 8) reset all flags to white
     * Iterate over all pools and all arenas. For each arena, memset cards
     * in use to GC_IT_CARD_WHITE. Cards on the free list should stay
     * GC_IT_CARD_NEW
     */
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

Adds a dead item to the free list. I suppose this is for cases where other
systems are trying to manage their own memory manually.

=cut

*/

static void
gc_it_add_free_object(PARROT_INTERP, ARGMOD(Small_Object_Pool *pool),
        PObj *to_add)
{
    /* I don't really know what this is for, so I'm going to wing it. */
    /* Objects that aren't on the free list already, or on the queue, are
       just floating. We can add this to the end of the free list very
       easily. */
    const Gc_it_hdr * hdr = PObj_to_IT_HDR(to_add);
    const Gc_it_hdr * const temp = pool->free_list;
    pool->free_list = hdr;
    hdr->next = temp;
    Gc_it_mark_card(hdr, GC_IT_CARD_NEW); /* just in case */
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
    Gc_it_hdr *hdr;

    /* If there are no objects, allocate a new arena */
    hdr = pool->free_list;
    if (hdr == NULL)
        (pool->more_objects)(interp, pool);

    /* pull the first header off the free list */
    hdr = pool->free_list;
    pool->free_list = hdr->next;
    hdr->next = NULL;

    /* return pointer to the object from the header */
    return IT_HDR_to_PObj(hdr);
}

/*

=item C<static void gc_it_alloc_objects>

Allocate a new Small_Object_Arena from the OS. Set up the arena as
necessary. The arena contains enough space to house several objects
of the given size, and we allocate locally from these arenas as needed.

=cut

*/

static void
gc_it_alloc_objects(PARROT_INTERP, ARGMOD(Small_Object_Pool *pool))
{
    const size_t real_size = pool->object_size;
    const size_t num_objects = pool->objects_per_alloc;
    const size_t card_size = (num_objects / 4 + ((num_objects % 4) ? (1) : (0)));
    const size_t size = real_size * pool->objects_per_alloc + /* the stuff */
                        sizeof(Small_Object_Arena) + /* for the arena struct */
                        card_size * sizeof(Gc_it_card); /* for the card */

    /* Instead of two separate allocations here, let's do a single big
       allocation, and append the two doodads together. We save on spacial
       locality, fewer allocations, less fragmentation, etc.
       Ideally, this is what we should have in the memory location:

       [ Small_Object_Arena | Cards | ... Objects ... ]

       And each object looks like this:

       [ Gc_it_hdr | Data ]

    */

    Small_Object_Arena * const new_arena = mem_internal_allocate(size);
    new_arena->card_info.card_size = card_size;
    new_arena->card_info.last_index = num_objects - 1;

    /* ...the downside is this messy pointer arithmetic. */
    new_arena->cards = ((Gc_it_cards*) ((Small_Object_Arena*)new_arena) + 1);
    new_arena->start_objects = ((void*) (((Gc_it_card*)(new_arenas->cards)) + card_size);
    memset(new_arena->cards, GC_IT_CARD_ALL_NEW, card_size);

    /* insert new_arena in pool's arena linked list */
    Parrot_append_arena_in_pool(interp, pool, new_arena, real_size * pool->objects_per_alloc);

    /* Add all these new objects we've created into the pool's free list */
    gc_it_add_arena_to_free_list(interp, pool, new_arena);

    /* allocate more next time */
    pool->objects_per_alloc = (UINTVAL) pool->objects_per_alloc *
        UNITS_PER_ALLOC_GROWTH_FACTOR;
    size = real_size * pool->objects_per_alloc;
    if (size > POOL_MAX_BYTES) {
        pool->objects_per_alloc = POOL_MAX_BYTES / real_size;
    }
}

static void
gc_it_add_arena_to_free_list(PARROT_INTERP, 
                             ARGMOD(Small_Object_Pool *pool),
                             ARGMOD(Small_Object_Arena *new_arena))
{
    Gc_it_hdr * p = new_arena->start_objects;
    register UINTVAL i;
    const size_t num_objs = new_arena->total_objects;

    for(i = 0; i < num_objs - 1; i++) {
        /* Here is what needs to happen in this loop:
           1) add the current object to the free list
           2) calculate the address of the next GC header in the arena
           3) set the ->next field of the current object to the address of the
              next object.
           4) move the current pointer to the next object
           5) set the index values now, for faster retrieval later
           6) repeat for all items in the arena
           My pointer voodoo might be mistaken, so we need to check this
           closely.
        */

        /* Add the current item to the free list */
        p->next = pool->free_list;
        pool->free_list = p;

        /* Cache the object's parent pool and card addresses */
        p->parent_pool = pool;
        p->index.num.card = i / 4;
        p->index.num.flag = i % 4;

        /* Find the next item in the arena with voodoo pointer magic */
        p = ((Gc_it_hdr *)p) + 1;
        p = (Gc_it_hdr *)(((size_t)p) + (pool->object_size));
    }
    p->next = pool->free_list;
    pool->free_list = new_arena->start_objects;
}

/* Mark an object's card with a given flag */

static void
gc_it_set_card_mark(Gc_it_hdr * hdr, UINTVAL flag)
{
    Gc_it_card * const card = &(hdr->parent_pool->cards[hdr->index.num.card]);
    switch (hdr->index.num.flag) {
        case 0:
            card->_f->flag1 = flag;
            break;
        case 1:
            card->_f->flag2 = flag;
            break;
        case 2:
            card->_f->flag3 = flag;
            break;
        case 3:
            card->_f->flag3 = flag;
            break;
    }
}

/* return an object's card value */

static UINTVAL
gc_it_get_card_mark(Gc_it_hdr * hdr)
{
    const Gc_it_card * const card = &(hdr->parent_pool->cards[hdr->index.num.card]);
    switch (hdr->index.num.flag) {
        case 0:
            return card->_f->flag1;
        case 1:
            return card->_f->flag2;
        case 2:
            return card->_f->flag3;
        case 3:
            return card->_f->flag4;
    }
}

/*

=item C<static void gc_it_more_objects>

Try to allocate new objects. If the mark phase is finished, run a quick sweep.
If the sweep frees up some objects, return one of those. Otherwise, we allocate
a new arena and return an object from that.

=cut

*/

static void
gc_it_more_objects(PARROT_INTERP, ARGMOD(Small_Object_Pool *pool))
{
    const Gc_it_data * const gc_priv_data = interp->arena_base->gc_private;
    const Gc_it_state state = gc_priv_data->state;
    if(state == GC_IT_NEW_SWEEP || state == GC_IT_RESUME_SWEEP) {
        gc_it_sweep_normal(interp);
        if(pool->free_list != NULL)
            return;
    }
    gc_it_alloc_objects(interp, pool);
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
