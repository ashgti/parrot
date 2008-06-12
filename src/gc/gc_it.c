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
    Gc_it_data * gc_priv_data;
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

/* Here we have two options: Enter the trace directly, or spawn a tracing
 * thread.
 * I'll deal with this later
 */
    if(gc_priv_data->state == GC_IT_NEW_MARK) {
        gc_it_find_all_roots(interp);
        gc_priv_data->total_count = 0;
        gc_priv_data->state == GC_IT_RESUME_MARK;
#if GC_IT_BREAK_AFTER_FIND_ROOTS
        return;
#endif
    }

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
        /* Two options here. In batch mode, enqueue all globals immediately.
           in increment mode, enqueue only the next root variable or IGP (if
           it's an aggregate). If there are no more root aggregates, enqueue
           all simple buffers */
#if GC_IT_INCREMENT_MODE
        gc_it_enqueue_next_root(interp);
#elif GC_IT_BATCH_MODE
        gc_it_enqueue_all_roots(interp);
#endif

        /* Two more options here. In single-threaded mode, run the trace
           function. In parallel mode, attempt to spawn a new GC thread to
           run the trace in the background */
#if GC_IT_SERIAL_MODE
        gc_it_trace_normal(interp);
#elif GC_IT_PARALLEL_MODE
        gc_it_trace_threaded(interp);
#endif
        --arena_base->DOD_block_level;
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

    while(cur_item = cur_pool->queue) {
        /*
        Move the children's headers into the queue, and possibly mark them
        grey as well.
        */
#define GC_IT_MARK_CHILDREN_GREY(x, y) gc_it_mark_children_grey(x, y)
        GC_IT_MARK_CHILDREN_GREY(cur_pool, cur_item);

        /* Mark the current node's card black, and return it to the list of
           all items.
           1) Find the pool/arena that is associated with this node
           2) Find the card associated with this node
           3) mark the card black
           4) remove the node from the queue
       */
#define GC_IT_MARK_NODE_BLACK(x, y) gc_it_mark_node_black(x, y)
        GC_IT_MARK_NODE_BLACK(cur_pool, cur_item);

        gc_priv_data->total_count++;
    }
}

#if GC_IT_PARALLEL_MODE
void
gc_it_trace_threaded(PARROT_INTERP)
{
    /* Check the current number of threads running. If we have space, launch
       another thread to help with the mark. */
}
#endif

#if GC_IT_SERIAL_MODE
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
#endif

void
gc_it_find_all_roots(PARROT_INTERP)
{
    /* Find all the root items, and possibly all IGPs, and add them to
       gc_priv_data->root_queue. Make sure the pointer at the end of
       the linked list is NULL. */
    const Gc_it_data *gc_priv_data = interp->arena_base->gc_private;
    /* Here is a (mostly-complete) list of stuff I probably need to add here.
       See src/gc/dod.c:Parrot_dod_trace_root() for more details.
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
       15) IGP pointers
    */
}

#if GC_IT_BATCH_MODE
void
gc_it_enqueue_all_roots(PARROT_INTERP)
{
    const Gc_it_data * gc_priv_data = interp->arena_base->gc_private;
    gc_priv_data->queue = gc_priv_data->root_queue;
    gc_priv_data->root_queue = NULL;
}
#endif

#if GC_IT_INCREMENT_MODE
void
gc_it_enqueue_next_root(PARROT_INTERP)
{
    /* enqueue next root, algorithm:
        1) Get the next root item from gc_priv_data->root_queue
        2) if it's an aggregate item, add it to the queue and return
        4) if there are no more aggregates to be had, add all simple buffers
           to the queue
    */
    const Gc_it_data *gc_priv_data = interp->arena_base->gc_private;
    const Gc_it_hdr *hdr = gc_priv_data->root_queue;
    const Gc_it_hdr *head, *ptr;
    gc_priv_data->root_queue = hdr->next;
    if(GC_IT_IS_AGGREGATE(hdr)) { /* This needs to be defined */
        /* if we are enqueueing a new root, the queue should be empty. However,
           we can pretend that it isn't for now (just to be safe) */
        hdr->next = gc_priv_data->queue;
        gc_priv_data->queue = hdr;
        return;
    }
    head = hdr;
    ptr = gc_priv_data->root_queue;
    /* the item is just a buffer, so we add it to the queue. We scan through
       the root_queue until we find the next aggregate (or until the end of
       the list). We add the aggregate and all items in between to the queue. */
    while(1) {
        if(!GC_IT_IS_AGGREGATE(ptr)) {
            /* If it's not an aggregate, move to the next item, unless the
               next item is NULL, then break. */
            if(ptr->next == NULL)
                break;
            ptr = ptr->next;
        }
        else
        {
            /* we found an aggregate. Add this item and all previous buffers
               to the queue at once. */
            hdr->next = gc_priv_root->root_queue;
            gc_priv_data->root_queue = ptr->next;
            ptr->next = NULL;
            return
        }
    }
    /* If we fall through here, we've hit a NULL in our loop. This means the
       queue is exhaused, and all remaining items, if any, were non-aggregates.
       Add the whole shoot-and-match to the queue. */
    hdr->next = gc_priv_data->root_queue;
    ptr->next = gc_priv_data->queue;
    gc_priv_data->queue = head;
}
#endif

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
    /* This might have already been taken care of in gc_it_find_all_roots() */
}

void
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
}

inline void
gc_it_mark_node_black(Gc_it_pool_data * pool_data, Gc_it_hdr * obj)
{
    /* mark the current node black, and remove it from the queue if
       it is on the queue */
    gc_it_mark_card(obj, GC_IT_CARD_BLACK);
    pool_data->queue = obj->next;
    obj->next = NULL;
}

inline void
gc_it_mark_node_grey(Gc_it_pool_data * pool, Gc_it_hdr * obj)
{
    /* Mark the current node as grey, add it to the grey queue */
    /* This function might become a macro or an inline function */
    /* gc_it_mark_card(obj, GC_IT_GREY); */
    obj->next = pool_data->queue;
    pool_data->queue = obj;
}

void
gc_it_free_white_items(PARROT_INTERP)
{
    /* Add all items which are still white to the free lists.
       Possibly also clear the bitmaps */
    /* This can be a macro or an inline function, or else it could be
       an incremental process, traversing through a pool-at-a-time */
    /* Algorithm:
       1) loop over every pool (or one pool at a time, if incremental)
       2) loop over every arena in each pool
       3) traverse the cards. when we find a white flag, add the corresponding
          object to the free list. If it's a PMC, and if it has the finalize
          flag, then we must call the custom finalization routine first. */
}

void gc_it_clear_cards(PARROT_INTERP)
{
    /* If we don't do it in gc_it_free_white_items, or somewhere else
       then clear the cards here. Go through each pool, through each
       arena, clear the card in each with a memset */
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
    const Gc_it_hdr *hdr = PObj_to_IT_HDR(to_add);
    const Gc_it_hdr *temp = pool->free_list;
    pool->free_list = hdr;
    hdr->next = temp;
    Gc_it_mark_card(hdr, GC_IT_CARD_WHITE); /* just in case */
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
    hdr->parent_pool = pool;
    /* I still don't know if I need to keep the items stored in a list, or if
       they can just float until I need them. I'm erring on the side of
       computational simplicity */
    hdr->next = NULL;
    ptr = IT_HDR_to_PObj(hdr);
    /* Figure out where in the arena we are now, and set the card to mark this
       item as GC_IT_CARD_NEW */
    PObj_flags_SETTO((PObj*) ptr, 0);
    return ptr;
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
    UINTVAL i;
    const size_t real_size = pool->object_size;
    const size_t card_size = (real_size / 4 + ((real_size % 4) ? (1) : (0)));
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
    /* ...the downside is this messy pointer arithmetic, which I've probably
       screwed up because I can't remember whether a typecast has higher
       precidence then -> or not. */
    new_arena->cards = ((Gc_it_cards*) ((Small_Object_Arena*)new_arena)+1);
    new_arena->start_objects = ((void*) (((Gc_it_card*)(new_arenas->cards))+card_size);
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
    Gc_it_hdr *p = new_arena->start_objects;
    void *temp;
    UINTVAL i;
    const size_t num_objs = new_arena->total_objects;

    for(i = 0; i < num_objs - 1; i++) {
        /* Here is what needs to happen in this loop:
           1) calculate the address of the next GC header in the arena
           2) set the ->next field of the current object to the address of the
              next object.
           3) move the current pointer to the next object, and repeat.
           If my pointer arithmetic is all good, this should work.
       */
        temp = (Gc_it_ptr *)p+1;
        temp = (char *)temp+(pool->object_size);
        p->next = temp;
        p = temp;
    }
    p->next = pool->free_list;
    pool->free_list = new_arena->start_objects;
}

/* Mark an object's card with a given flag */

static void
gc_it_set_card_mark(Gc_it_hdr * hdr, UINTVAL flag)
{
    Gc_it_card * card = &(hdr->parent_pool->cards[hdr->index / 4]);
    switch (hdr->index % 4) {
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

static void
gc_it_set_card_mark_obj(PObj * obj, UINTVAL flag)
{
    /* this will become a macro */
    gc_it_mark_card(PObj_to_IT_HDR(obj), flag);
}

/* return an object's card value */

static UINTVAL
gc_it_get_card_mark(Gc_it_hdr * hdr)
{
    const Gc_it_card *card = &(hdr->parent_pool->cards[hdr->index / 4]);
    switch (hdr->index % 4) {
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

static UINTVAL
gc_it_get_card_mark_obj(PObj *obj)
{
    return gc_it_get_card_mark(PObj_to_IT_HDR(obj));
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
    const Gc_it_data * gc_priv_data = interp->arena_base->gc_private;
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
