/*
Copyright (C) 2001-2009, The Perl Foundation.
$Id$

=head1 NAME

src/gc/incremental_ms.c - Incremental mark and sweep garbage collection

=head1 DESCRIPTION

The following comments describe a new garbage collection scheme for
Parrot.

The scheme of this algorithm is described in the literature with these
keywords:

 - non-copying, mark & sweep
 - incremental
 - realtime
 - incremental update with write barrier

Further we might try this optimization

 - treadmill optimization or
 - implict reclamation

=head1 Drawbacks of the current mark and sweep collector.

 * can take arbitrary time to complete (1s for 1 Meg objects)
 * can't be used in multi-threaded Parrot
 * works fast for plain (non-aggregate) objects but suffers badly
   for nested aggregates or HLL objects
 * the sweep phase takes time proportional to the allocated storage

=head1 INCREMENTAL GARBAGE COLLECTION

=head2 Terms

=over 4

=item object

An item like a buffer header or a PMC which is managed by Parrot's
dynamic memory system.

=item aggregate

An object that possibly holds references to other objects. For example
an array, hash, or reference PMC.

=item tri-color marking

All objects have one of three colors: white, grey, or black.

At the beginning of a DOD run all objects are white (not yet visited).
During marking objects are greyed (visited - found alive), but their
contents isn't yet scanned. A fully scanned grey object gets finally
colored black. It will not again be rescanned in this DOD run.

Only aggregates can be grey, non-containers are blackened immediately.

Objects on the free-list are sometimes denoted having the color off-white
or ecru.

=item DOD

Dead object detection. Please note that in Parrot src and docs this
stands for the stop-the-world garbage collector that recycles objects.

=item GC

In Parrot tree the copying garbage collector that recycles string and
buffer memory. Configure.pl has a switch to use a malloc library instead,
which makes string and buffer memory non-moving.

=item collector

The DOD and reclamation system.

=item mutator

The normal operation of the program which may or may not change the
collectors view of objects.

=item incremental

Garbage collection and normal program operation is interleaved. This
guarantees short and bounded pause times. Garbage collection doesn't
significantly interrupt program execution, collector and mutator are
running pseudo-parallel.

=item root set

All structures in the interpreter that might point to objects. E.g.
stacks, globals, and of course the registers. All objects the
interpreter works with, are directly or indirectly reachable starting
from the root set.

=item the tri-color invariant

At no time a black object may reference a white one directly. Actually
this is the strong incarnation of the invariant - all paths from black
objects to white objects lead over at least one grey object.

The weak tri-color invariant is: there is at least one such path to a
white object, so that it's reachable.

The strong invariant is the basic idea of mark and sweep too. But as the
mutator isn't running during DOD the invariant is never violated.

Due to this invariant, after the root set has been marked and when all
greyed objects are marked (blackened), the white objects have to be
dead.

=item paint it black

Or, which color do new objects have?

Actually this should be tunable. Or it depends. If objects are born
white and die immediately, they get collected in the same GC cycle. OTOH
when these objects are stored into an existing (black) array, we have to
do more work to keep the tri-color invariant valid.

Anyway, when allocating new objects white, the collector must run more
often or must do more work per increment to make the algorithm stop
somewhen.

=item write barrier

To keep the tri-color invariant valid all pointer stores into black
objects have to be tracked. If a white object would be stored into a
black array, and this object isn't refered to by another object it would
get collected. The write barrier greys the white object, so that it get
scanned later or alternatively greys the aggregate for a rescan. The latter
can be better, if a sequence of such stores would happen.

=back

=head2 Data structure overview

The incremental mark and sweep collector has an additional structure in
the arena_base that keeps track of the collector's state. Pool and arena
structures are unchanged. Only the allocation of new arena blocks is done
much more fine grained in e.g. 8K blocks.

=head2 Implicit reclamation (optional)

=over 4

=item from-space

The graph of all objects found live during the last collection.

=item to-space

The work area of the collector. During marking live objects are "moved"
from the from-space into the to-space. This is the same as the text_for_GC
list used in src/gc/api.c. The to-space is initially empty. During marking
it gets greyed and finally all reachable objects are black.

=item free-list

New objects are allocated from the free-list. The free-list is adjacent
to the to-space. Allocating a new objects thus means, moving the free
pointer one word forward and paint the new object black.

=back

All objects get two additional pointers (forward, backward) and are
arranged like in this scheme:


    <-- allocation direction         marking -->
            |                          |
  [w] <--> [w] <--> [b] <--> [b] <--> [g] <--> [g] <--> [w] <-> [w]

            ^        ^                 ^                 ^
            |        |                 |                 |
   free-list-ptr     to-space          scan-pointer      from-space

Objects get "moved" during collection by rearranging the doubly-linked
object pointers. At the end of a DOD run (when the last grey object is
blackened), the from-space and the free-list are merged serving
as the new free-list of the next DOD cycle. This operation is just a few
pointer manipulations that replaces the sweep phase of a mark and sweep
collector.

=head2 Phases of operation

=over 4

=item a) initialization

After interpreter creation the DOD system is initialized by marking
parts of the root set (globals, internal structures).

=item b) program operation

For each bunch of allocated objects (A) the collector does k.A work, for
some constant k > 1. As new objects are allocated black the number of
whites is reduced steadily. This means that the throttle factor k could
be less then one too, but this could highly increase average memory usage.

To keep the memory usage limited k > 1 must hold.

=item c) near the end of a DOD cycle

The rest of the root set is scanned, i.e. the registers. By deferring
scanning of registers all temporaries that might have exist somewhen
just stay unscanned - they will be collected in this DOD cycle, if
we allocate new objects white or in the next DOD cycle.

=item d) finishing a DOD cycle

The current sweep of the whole arena is done, or with implicit reclamation:

Garbage gets appended to the free-list by merging the unscanned
from-space with the free-list, these objects are all considered white.
All other items are in the to-space and are black. These objects
constitute the from-space of the new collection cycle.

Now he meaning of the black bit is reversed effectively setting the new
from-space to white.

The next DOD cycle is initialized in one step a) and the new cycle starts.

Alternatively the mutator could run and allocate objects for some time,
without starting the collector again, if there are plenty of free objects on
all free-lists.

=item e) collect buffer memory

Finally, we might trigger a collect run on string and buffer memory if
there is an impending shortage of resources. While the copying compactor
is rather independent of the collector that cleans object headers, it's
more efficient to collect buffer memory when the live information is
accurate. This avoids copying of dead buffer memory.

=back

=head2 Comparison with our current mark and sweep collector

  MS  ... mark and sweep (stop-the-world)
  IMS ... incremental mark and sweep
  IMIR .. incremental mark implicit reclamation

                       MS                 IMS               IMIR
  ------------------------------------------------------------------------
  operation            stop-the-world     incremental       incremental
  time per DOD cycle   unbounded          bounded           bounded
  size overhead        1 word             1 word            2 words
  time overhead        O(2*live + dead)   O(2*live + dead)  O(live)   2)

Notes:

  2) it should be possible to mark containers at once by using the
     information of the from-space pointers and tracking changes
     to the aggregate.

=head2 Implementation details and unsorted remarks

=over 4

=item the object graph

The MS and IMS scheme use the next_for_GC pointer for keeping track of
references. This interferes with the freeze functionality, which can use
the same pointer to keep track of visited objects.

IMIR has a dedicated pointer pair to build the object graph.

=item Greying objects

Greying objects is done depth-first. This has much better cache locality
then visiting an object again much later. In the picture above this means
that grey objects are inserted at the left end of the mark chain immediately
to the right of the object that gets blackened.

=item big aggregates

Greying has to be done in increments. Big aggregates can't have a mark
vtable that could run arbitrarily long. This means that the DOD system
must know the layout of arrays, hashes, and objects. This is currently
true for arrays and objects but not for hashes. But the latter need some
refactoring of internals anyway.

To avoid visiting all aggregate elements, it could be better to track
the graph of old aggregates by using a write barrier for all writes into
the array. This would basically create a generational collector. The old
generation (the aggregate) isn't scanned. But changes to this "old
generation" are tracked and reflected in the collectors graph of
objects.

=item timely destruction

The interpreter arena has a count of currently active objects that need
timely destruction. When during scope exit an high priority sweep is
triggered, we have basically two cases:

1) all of these objects were already seen by this DOD run - the scope
exit can continue.

2) Not all objects were seen - they might be alive or not. This means
that the DOD cycle must run to the end to decide, if these objects are
alive (or again until all are found alive).

To increase performance its likely that we need some additional
information that keeps track of the location of such objects and just
try to mark paths to objects that need timely destruction.

=item concurrent or parallel collection

As the described algorithm is already incremental its well-suited for
parallel collection in a multi-threaded Parrot. The work of greying
objects can be done in parallel by atomically handling a bunch of
objects to another thread. After doing some increments of marking, these
objects then get returned to the shared to-space. The parallel
collection is finished when the last object is blackened and all threads
have reached the thread barrier rendezvous point. (Please note the very
different meaning of barrier here).

But also a single-threaded Parrot can vastly take advantage by running
increments of the collection during waiting for I/O completion or during
a sleep opcode.

=back

=head1 FUNCTIONS

=over 4

=cut

*/

#include "parrot/parrot.h"
#include "parrot/gc_api.h"
#include "parrot/gc_mark_sweep.h"

/* HEADERIZER HFILE: include/parrot/gc_api.h */

/* HEADERIZER BEGIN: static */
/* Don't modify between HEADERIZER BEGIN / HEADERIZER END.  Your changes will be lost. */

static int collect_cb(PARROT_INTERP,
    ARGMOD(Small_Object_Pool *pool),
    SHIM(int flag),
    ARGIN(void *arg))
        __attribute__nonnull__(1)
        __attribute__nonnull__(2)
        __attribute__nonnull__(4)
        FUNC_MODIFIES(*pool);

static void gc_ims_add_free_object(PARROT_INTERP,
    ARGMOD(Small_Object_Pool *pool),
    ARGOUT(void *to_add))
        __attribute__nonnull__(1)
        __attribute__nonnull__(2)
        __attribute__nonnull__(3)
        FUNC_MODIFIES(*pool)
        FUNC_MODIFIES(*to_add);

static void gc_ims_alloc_objects(PARROT_INTERP,
    ARGMOD(Small_Object_Pool *pool))
        __attribute__nonnull__(1)
        __attribute__nonnull__(2)
        FUNC_MODIFIES(*pool);

PARROT_CANNOT_RETURN_NULL
PARROT_WARN_UNUSED_RESULT
static void * gc_ims_get_free_object(PARROT_INTERP,
    ARGMOD(Small_Object_Pool *pool))
        __attribute__nonnull__(1)
        __attribute__nonnull__(2)
        FUNC_MODIFIES(*pool);

static void gc_ims_pool_init(SHIM_INTERP, ARGMOD(Small_Object_Pool *pool))
        __attribute__nonnull__(2)
        FUNC_MODIFIES(*pool);

static int parrot_gc_ims_collect(PARROT_INTERP, int check_only)
        __attribute__nonnull__(1);

static void parrot_gc_ims_deinit(PARROT_INTERP)
        __attribute__nonnull__(1);

static void parrot_gc_ims_mark(PARROT_INTERP)
        __attribute__nonnull__(1);

static void parrot_gc_ims_reinit(PARROT_INTERP)
        __attribute__nonnull__(1);

static void parrot_gc_ims_run(PARROT_INTERP, UINTVAL flags)
        __attribute__nonnull__(1);

static void parrot_gc_ims_run_increment(PARROT_INTERP)
        __attribute__nonnull__(1);

static void parrot_gc_ims_sweep(PARROT_INTERP)
        __attribute__nonnull__(1);

static int sweep_cb(PARROT_INTERP,
    ARGMOD(Small_Object_Pool *pool),
    int flag,
    ARGIN(void *arg))
        __attribute__nonnull__(1)
        __attribute__nonnull__(2)
        __attribute__nonnull__(4)
        FUNC_MODIFIES(*pool);

#define ASSERT_ARGS_collect_cb __attribute__unused__ int _ASSERT_ARGS_CHECK = \
       PARROT_ASSERT_ARG(interp) \
    || PARROT_ASSERT_ARG(pool) \
    || PARROT_ASSERT_ARG(arg)
#define ASSERT_ARGS_gc_ims_add_free_object __attribute__unused__ int _ASSERT_ARGS_CHECK = \
       PARROT_ASSERT_ARG(interp) \
    || PARROT_ASSERT_ARG(pool) \
    || PARROT_ASSERT_ARG(to_add)
#define ASSERT_ARGS_gc_ims_alloc_objects __attribute__unused__ int _ASSERT_ARGS_CHECK = \
       PARROT_ASSERT_ARG(interp) \
    || PARROT_ASSERT_ARG(pool)
#define ASSERT_ARGS_gc_ims_get_free_object __attribute__unused__ int _ASSERT_ARGS_CHECK = \
       PARROT_ASSERT_ARG(interp) \
    || PARROT_ASSERT_ARG(pool)
#define ASSERT_ARGS_gc_ims_pool_init __attribute__unused__ int _ASSERT_ARGS_CHECK = \
       PARROT_ASSERT_ARG(pool)
#define ASSERT_ARGS_parrot_gc_ims_collect __attribute__unused__ int _ASSERT_ARGS_CHECK = \
       PARROT_ASSERT_ARG(interp)
#define ASSERT_ARGS_parrot_gc_ims_deinit __attribute__unused__ int _ASSERT_ARGS_CHECK = \
       PARROT_ASSERT_ARG(interp)
#define ASSERT_ARGS_parrot_gc_ims_mark __attribute__unused__ int _ASSERT_ARGS_CHECK = \
       PARROT_ASSERT_ARG(interp)
#define ASSERT_ARGS_parrot_gc_ims_reinit __attribute__unused__ int _ASSERT_ARGS_CHECK = \
       PARROT_ASSERT_ARG(interp)
#define ASSERT_ARGS_parrot_gc_ims_run __attribute__unused__ int _ASSERT_ARGS_CHECK = \
       PARROT_ASSERT_ARG(interp)
#define ASSERT_ARGS_parrot_gc_ims_run_increment __attribute__unused__ int _ASSERT_ARGS_CHECK = \
       PARROT_ASSERT_ARG(interp)
#define ASSERT_ARGS_parrot_gc_ims_sweep __attribute__unused__ int _ASSERT_ARGS_CHECK = \
       PARROT_ASSERT_ARG(interp)
#define ASSERT_ARGS_sweep_cb __attribute__unused__ int _ASSERT_ARGS_CHECK = \
       PARROT_ASSERT_ARG(interp) \
    || PARROT_ASSERT_ARG(pool) \
    || PARROT_ASSERT_ARG(arg)
/* Don't modify between HEADERIZER BEGIN / HEADERIZER END.  Your changes will be lost. */
/* HEADERIZER END: static */


/* size of one arena */
#define ALLOCATION_BLOCK_SIZE 8192

/*
 * each ALLOCATIONS_INIT allocations of any object an incremental
 * step is triggered
 */
#define ALLOCATIONS_INIT      1024*4

/* a mark step does allocations * throttle work */
#define THROTTLE              1.3

/*
 * if we have at the end total * refill free objects
 * we just do nothing
 */
#define REFILL_FACTOR         0.5

/*
 * we run the copying collector, if memory pool statistics indicate
 * that this amount of the total size could be freed
 *
 * This factor also depends on the allocation color of buffer headers,
 * which is set to black now. So we are always one DOD cycle behind
 * and the statistics are rather wrong.
 */
#define MEM_POOL_RECLAIM      0.2

#if 0
#  define IMS_DEBUG(x) fprintf (x)
#else
#  define IMS_DEBUG(x)
#endif

typedef enum {          /* these states have to be in execution order */
    GC_IMS_INITIAL,     /* memory subsystem setup */
    GC_IMS_STARTING,    /* wait for DOD_block to clear */
    GC_IMS_RE_INIT,     /* start of normal operation - mark root */
    GC_IMS_MARKING,     /* mark children */
    GC_IMS_START_SWEEP, /* mark finished, start sweep buffers */
    GC_IMS_SWEEP,       /* sweep buffers */
    GC_IMS_COLLECT,     /* collect buffer memory */
    GC_IMS_FINISHED,    /* update statistics */
    GC_IMS_CONSUMING,   /* when we have plenty of free objects */
    GC_IMS_DEAD         /* gc is already shutdown */

} gc_ims_state_enum;

typedef struct Gc_ims_private {
    gc_ims_state_enum   state;
    size_t      allocations;    /* get_free_object count */
    size_t      alloc_trigger;  /* after this number of allocations a gc
                                   increment is triggered */
    double      throttle;       /* throttle * allocations per increment work */
    size_t      increments;     /* increment count */
    int         lazy;           /* timely destruction run */
    size_t      n_objects;      /* live count of prev run */
    size_t      n_extended_PMCs;/* PMCs found during mark_special */
} Gc_ims_private;


/*

=item C<static void gc_ims_add_free_object>

Add object C<to_add> to the free_list in the given pool.
C<pool->num_free_objects> has to be updated by the caller.

=cut

*/

static void
gc_ims_add_free_object(PARROT_INTERP, ARGMOD(Small_Object_Pool *pool), ARGOUT(void *to_add))
{
    ASSERT_ARGS(gc_ims_add_free_object)
    *(void **)to_add = pool->free_list;
    pool->free_list  = to_add;
#if DISABLE_GC_DEBUG
    UNUSED(interp);
#else
    if (GC_DEBUG(interp)  && pool == interp->arena_base->pmc_pool) {
        PMC * const p = (PMC *)to_add;
        p->vtable     = interp->vtables[enum_class_Null];
    }
#endif
}

/*

=item C<static void * gc_ims_get_free_object>

Get a new object off the free_list in the given pool.

=cut

*/

PARROT_CANNOT_RETURN_NULL
PARROT_WARN_UNUSED_RESULT
static void *
gc_ims_get_free_object(PARROT_INTERP, ARGMOD(Small_Object_Pool *pool))
{
    ASSERT_ARGS(gc_ims_get_free_object)
    PObj *ptr;
    Arenas * const arena_base    = interp->arena_base;
    Gc_ims_private * const g_ims = (Gc_ims_private *)arena_base->gc_private;

    if (++g_ims->allocations >= g_ims->alloc_trigger) {
        g_ims->allocations = 0;
        parrot_gc_ims_run_increment(interp);
    }

    /* if we don't have any objects */
    if (!pool->free_list)
        (*pool->alloc_objects) (interp, pool);

    ptr             = (PObj *)pool->free_list;
    pool->free_list = *(void **)ptr;

    /*
     * buffers are born black, PMCs not yet?
     * XXX this does not solve the problem of storing keys in hashes
     *     in the next DOD cycle (if the key isn't marked elsewhere ?)
     */
    PObj_flags_SETTO(ptr, pool == arena_base->pmc_pool ? 0 : PObj_live_FLAG);
    --pool->num_free_objects;
    return (void *)ptr;
}


/*

=item C<static void gc_ims_alloc_objects>

Allocate new objects for the given pool.

=cut

*/

static void
gc_ims_alloc_objects(PARROT_INTERP, ARGMOD(Small_Object_Pool *pool))
{
    ASSERT_ARGS(gc_ims_alloc_objects)
    Small_Object_Arena *new_arena;
    size_t size;

    pool->objects_per_alloc  = ALLOCATION_BLOCK_SIZE / pool->object_size;

    /* Setup memory for the new objects */
    new_arena                = mem_allocate_typed(Small_Object_Arena);
    size                     = ALLOCATION_BLOCK_SIZE;
    new_arena->start_objects = mem_sys_allocate(size);

    Parrot_append_arena_in_pool(interp, pool, new_arena, size);

    Parrot_add_to_free_list(interp, pool, new_arena);
}


/*

=item C<static void gc_ims_pool_init>

Initializes a pool by setting the appropriate function pointers to add, get,
and allocate objects.

=cut

*/

static void
gc_ims_pool_init(SHIM_INTERP, ARGMOD(Small_Object_Pool *pool))
{
    ASSERT_ARGS(gc_ims_pool_init)
    pool->add_free_object = gc_ims_add_free_object;
    pool->get_free_object = gc_ims_get_free_object;
    pool->alloc_objects   = gc_ims_alloc_objects;
    pool->more_objects    = pool->alloc_objects;
}


/*

=item C<static void parrot_gc_ims_deinit>

Shuts down this GC system.

=cut

*/

static void
parrot_gc_ims_deinit(PARROT_INTERP)
{
    ASSERT_ARGS(parrot_gc_ims_deinit)
    Arenas * const arena_base = interp->arena_base;

    mem_sys_free(arena_base->gc_private);
    arena_base->gc_private = NULL;
}


/*

=item C<void Parrot_gc_ims_init>

Initialize the state structures of the gc system. Called immediately before
creation of memory pools. This function must set the function pointers
for C<add_free_object_fn>, C<get_free_object_fn>, C<alloc_objects_fn>, and
C<more_objects_fn>.

=cut

*/

void
Parrot_gc_ims_init(PARROT_INTERP)
{
    ASSERT_ARGS(Parrot_gc_ims_init)
    Arenas * const arena_base = interp->arena_base;
    arena_base->gc_private    = mem_allocate_zeroed_typed(Gc_ims_private);

    /* set function hooks according to pdd09 */

    arena_base->do_gc_mark         = parrot_gc_ims_run;
    arena_base->finalize_gc_system = parrot_gc_ims_deinit;
    arena_base->init_pool          = gc_ims_pool_init;

    /* run init state */
    parrot_gc_ims_run_increment(interp);
}


/*

=item C<static void parrot_gc_ims_reinit>

Reinitialize the collector for the next collection cycle.

=cut

*/

static void
parrot_gc_ims_reinit(PARROT_INTERP)
{
    ASSERT_ARGS(parrot_gc_ims_reinit)
    Gc_ims_private *g_ims;
    Arenas * const  arena_base = interp->arena_base;

    arena_base->lazy_gc = 0;
    Parrot_gc_ms_run_init(interp);

    /*
     * trace root set w/o system areas
     * TODO also skip volatile roots
     */
    Parrot_gc_trace_root(interp, GC_TRACE_ROOT_ONLY);

    g_ims        = (Gc_ims_private *)arena_base->gc_private;
    g_ims->state = GC_IMS_MARKING;

}


/*

=item C<static void parrot_gc_ims_mark>

Mark a bunch of children.

The work depends on item counts with and without a next_for_GC field.
The former are marked immediately, only the latter need real work here.

=cut

*/

static void
parrot_gc_ims_mark(PARROT_INTERP)
{
    ASSERT_ARGS(parrot_gc_ims_mark)
    size_t todo;
    double work_factor;
    PMC   *next;

    Arenas * const arena_base    = (Arenas *)interp->arena_base;
    Gc_ims_private * const g_ims = (Gc_ims_private *)arena_base->gc_private;

    /* use statistics from the previous run */
    if (g_ims->n_objects)
        work_factor = (double)g_ims->n_extended_PMCs / g_ims->n_objects;
    else
        work_factor = 1.0;

    todo = (size_t)(g_ims->alloc_trigger * g_ims->throttle * work_factor);

    PARROT_ASSERT(arena_base->lazy_gc == 0);
    Parrot_gc_trace_children(interp, todo);

    /* check if we are finished with marking -- the end is self-referential */
    next = arena_base->gc_mark_start;

    if (next == PMC_next_for_GC(next))
        g_ims->state = GC_IMS_START_SWEEP;
}


/*

=item C<static int sweep_cb>

Callback to sweep a header pool (see Parrot_forall_header_pools).

=cut

*/

static int
sweep_cb(PARROT_INTERP, ARGMOD(Small_Object_Pool *pool), int flag, ARGIN(void *arg))
{
    ASSERT_ARGS(sweep_cb)
    int * const n_obj = (int *)arg;

    Parrot_gc_sweep(interp, pool);

    if (interp->profile && (flag & POOL_PMC))
        Parrot_gc_profile_end(interp, PARROT_PROF_GC_cp);

    *n_obj += pool->total_objects - pool->num_free_objects;

    return 0;
}


/*

=item C<static void parrot_gc_ims_sweep>

Free unused objects in all header pools.

TODO split work per pool.

=cut

*/

static void
parrot_gc_ims_sweep(PARROT_INTERP)
{
    ASSERT_ARGS(parrot_gc_ims_sweep)
    Arenas * const  arena_base = interp->arena_base;
    Gc_ims_private *g_ims      = (Gc_ims_private *)arena_base->gc_private;
    size_t          n_objects;
    int             ignored;

    IMS_DEBUG((stderr, "\nSWEEP\n"));

    /*
     * as we are now gonna kill objects, make sure that we
     * have traced the current stack
     * except for a lazy run, which is invoked from the run loop
     */

    /* TODO mark volatile roots */
    Parrot_gc_trace_root(interp, g_ims->lazy ? GC_TRACE_ROOT_ONLY
                                             : GC_TRACE_FULL);

    /* mark (again) rest of children */
    Parrot_gc_trace_children(interp, (size_t) -1);

    /* now sweep all */
    n_objects = 0;
    ignored   = Parrot_forall_header_pools(interp, POOL_BUFFER | POOL_PMC,
            (void*)&n_objects, sweep_cb);
    UNUSED(ignored);

    if (interp->profile)
        Parrot_gc_profile_end(interp, PARROT_PROF_GC_cb);

    g_ims->state           = GC_IMS_COLLECT;
    g_ims->n_objects       = n_objects;
    g_ims->n_extended_PMCs = arena_base->num_extended_PMCs;
}


/*

=item C<static int collect_cb>

Callback to collect a header pool (see Parrot_forall_header_pools).

=cut

*/

#if !defined(GC_IS_MALLOC) || !GC_IS_MALLOC

static int
collect_cb(PARROT_INTERP, ARGMOD(Small_Object_Pool *pool), SHIM(int flag), ARGIN(void *arg))
{
    ASSERT_ARGS(collect_cb)
    const int           check_only = (int)(INTVAL)arg;
    Memory_Pool * const mem_pool   = pool->mem_pool;

    /* check if there is an associated memory pool */
    if (!mem_pool)
        return 0;

    /* and if the memory pool supports compaction */
    if (!mem_pool->compact)
        return 0;

    /*
     * several header pools can share one memory pool
     * if that pool is already compacted, the following is zero
     */
    if (!mem_pool->guaranteed_reclaimable)
        return 0;

    /* check used size */
    if ((mem_pool->possibly_reclaimable * mem_pool->reclaim_factor +
                mem_pool->guaranteed_reclaimable) >=
            mem_pool->total_allocated * MEM_POOL_RECLAIM) {
        IMS_DEBUG((stderr, "COMPACT\n"));
        if (check_only)
            return 1;
        mem_pool->compact(interp, mem_pool);
    }

    return 0;
}

#endif


/*

=item C<static int parrot_gc_ims_collect>

Run the copying collector in memory pools, if it could yield some free memory.

=cut

*/

static int
parrot_gc_ims_collect(PARROT_INTERP, int check_only)
{
    ASSERT_ARGS(parrot_gc_ims_collect)
#if defined(GC_IS_MALLOC) && GC_IS_MALLOC
    UNUSED(interp);
    UNUSED(check_only);
#else
    Arenas * const  arena_base = interp->arena_base;
    Gc_ims_private *g_ims;
    int             ret;

    if (!check_only && interp->profile)
        Parrot_gc_profile_start(interp);

    g_ims = (Gc_ims_private *)arena_base->gc_private;

    ret   = Parrot_forall_header_pools(interp, POOL_BUFFER,
            (void *)(long)check_only, collect_cb);

    if (ret)
        return ret;

    if (check_only)
        return 0;

    if (interp->profile)
        Parrot_gc_profile_end(interp, PARROT_PROF_GC);

    g_ims->state = GC_IMS_FINISHED;
#endif
    return 0;
}


/*

=item C<static void parrot_gc_ims_run_increment>

Run one increment of collection. This function is triggered by object
allocation.

=cut

*/

static void
parrot_gc_ims_run_increment(PARROT_INTERP)
{
    ASSERT_ARGS(parrot_gc_ims_run_increment)
    Arenas * const arena_base    = interp->arena_base;
    Gc_ims_private * const g_ims = (Gc_ims_private *)arena_base->gc_private;

    if (arena_base->gc_mark_block_level || g_ims->state == GC_IMS_DEAD)
        return;

    ++g_ims->increments;
    IMS_DEBUG((stderr, "state = %d => ", g_ims->state));

    switch (g_ims->state) {
        case GC_IMS_INITIAL:
            g_ims->state         = GC_IMS_STARTING;
            g_ims->alloc_trigger = ALLOCATIONS_INIT;
            g_ims->throttle      = THROTTLE;
            break;
        case GC_IMS_STARTING:
            /*  fall through and start */
            /* FALLTHRU */
        case GC_IMS_RE_INIT:
            parrot_gc_ims_reinit(interp);
            break;

        case GC_IMS_MARKING:
            parrot_gc_ims_mark(interp);
            break;

        case GC_IMS_START_SWEEP:
            g_ims->state = GC_IMS_SWEEP;
            /* fall through */
        case GC_IMS_SWEEP:
            parrot_gc_ims_sweep(interp);
            /* fall through */
        case GC_IMS_COLLECT:
            (void)parrot_gc_ims_collect(interp, 0);
            break;
        case GC_IMS_FINISHED:
            ++arena_base->gc_runs;
            g_ims->state = GC_IMS_CONSUMING;
            /* fall through */
        case GC_IMS_CONSUMING:
            /*
             * This currently looks only at PMCs and string_headers.
             * There shouldn't be other pools that could run out of
             * headers independent of PMCs
             */
            if (arena_base->pmc_pool->num_free_objects <
                    arena_base->pmc_pool->total_objects * REFILL_FACTOR) {
                g_ims->state = GC_IMS_STARTING;
            }
            else if (arena_base->string_header_pool->num_free_objects <
                    arena_base->string_header_pool->total_objects *
                    REFILL_FACTOR) {
                g_ims->state = GC_IMS_STARTING;
            }
            break;
        default:
            PANIC(interp, "Unknown state in gc_ims");
    }

    IMS_DEBUG((stderr, "%d\n", g_ims->state));
}


/*

=item C<static void parrot_gc_ims_run>

Interface to C<Parrot_do_gc_run>. C<flags> is one of:

  GC_lazy_FLAG   ... timely destruction
  GC_finish_FLAG ... run until live bits are clear

=cut

*/

static void
parrot_gc_ims_run(PARROT_INTERP, UINTVAL flags)
{
    ASSERT_ARGS(parrot_gc_ims_run)
    int                    lazy;
    Arenas * const         arena_base = interp->arena_base;
    Gc_ims_private * const g_ims      = (Gc_ims_private *)arena_base->gc_private;

    if (arena_base->gc_mark_block_level || g_ims->state == GC_IMS_DEAD)
        return;

    if (flags & GC_finish_FLAG) {
        /*
         * called from really_destroy. This interpreter is gonna die.
         * The destruction includes a sweep over PMCs, so that
         * destructors/finalizers are called.
         *
         * Be sure live bits are clear.
         */
        if (g_ims->state >= GC_IMS_RE_INIT || g_ims->state < GC_IMS_FINISHED)
            Parrot_gc_clear_live_bits(interp);

        Parrot_gc_sweep(interp, interp->arena_base->pmc_pool);
        g_ims->state = GC_IMS_DEAD;

        return;
    }

    /* make the test happy that checks the count ;) */
    arena_base->gc_runs++;

    lazy = flags & GC_lazy_FLAG;

    if (!lazy) {
        /* run a full cycle
         * TODO if we are called from mem_allocate() in src/resources.c:
         *   * pass needed size
         *   * test   examples/benchmarks/gc_header_new.pasm
         */
        if (!parrot_gc_ims_collect(interp, 1)) {
            parrot_gc_ims_run_increment(interp);
            return;
        }

        if (g_ims->state >= GC_IMS_FINISHED)
            g_ims->state = GC_IMS_STARTING;

        while (1) {
            parrot_gc_ims_run_increment(interp);
            if (g_ims->state > GC_IMS_COLLECT)
                break;
        }

        return;
    }

    /* lazy DOD handling */
    IMS_DEBUG((stderr, "\nLAZY state = %d\n", g_ims->state));
    g_ims->lazy = lazy;

    if (g_ims->state >= GC_IMS_COLLECT) {
        /* we are beyond sweep, timely destruction is done */
        if (arena_base->num_early_PMCs_seen >= arena_base->num_early_gc_PMCs)
            return;

        /* when not all seen, start a fresh cycle */
        g_ims->state = GC_IMS_RE_INIT;

        /* run init, which clears lazy seen counter */
        parrot_gc_ims_run_increment(interp);
    }

    /*
     *  run through all steps until we see enough PMCs that need timely
     *  destruction or we finished sweeping
     */
    while (arena_base->num_early_PMCs_seen < arena_base->num_early_gc_PMCs) {
        parrot_gc_ims_run_increment(interp);
        if (g_ims->state >= GC_IMS_COLLECT)
            break;
    }

    /* if we stopped early, the lazy run was successful */
    if (g_ims->state < GC_IMS_COLLECT)
        ++arena_base->lazy_gc_runs;

    g_ims->lazy = 0;
}


/*

=item C<void Parrot_gc_ims_wb>

Write barrier called by the GC_WRITE_BARRIER macro. Always when storing
a white object into a black aggregate, either the object must
be greyed or the aggregate must be rescanned -- so grey it.

=cut

*/

#define DOD_IMS_GREY_NEW 1

void
Parrot_gc_ims_wb(PARROT_INTERP, ARGMOD(PMC *agg), ARGMOD(PMC *_new))
{
    ASSERT_ARGS(Parrot_gc_ims_wb)
#if DOD_IMS_GREY_NEW
    IMS_DEBUG((stderr, "%d agg %p mark %p\n",
                ((Gc_ims_private *)interp->arena_base->
                gc_private)->state, agg, _new));
    pobject_lives(interp, (PObj*)_new);
#else
    PObj_get_FLAGS(agg) &= ~ (PObj_live_FLAG|PObj_custom_GC_FLAG);
    pobject_lives(interp, (PObj*)agg);
#endif
}

/*

=back

=head1 SEE ALSO

F<src/gc/api.c>, F<include/parrot/gc_api.h>, F<include/parrot/pobj.h>,

=head1 HISTORY

Initial version by leo (2004.08.12 - 2004.08.15)

=cut

*/

/*
 * Local variables:
 *   c-file-style: "parrot"
 * End:
 * vim: expandtab shiftwidth=4:
 */
