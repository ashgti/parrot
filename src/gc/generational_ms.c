/*
Copyright (C) 2001-2009, The Perl Foundation.
$Id$

=head1 NAME

src/gc/generational_ms.c - Generational mark and sweep garbage collection

=head1 OVERVIEW

The following comments describe a generational garbage collection
scheme for Parrot.

Keywords:

 - non-copying, mark & sweep
 - generational
 - implicit reclamation, treadmill

=head1 DESCRIPTION

A plain mark & sweep collector performs work depending on the amount
of all allocated objects. The advantage of a generational GC is
achieved by not processing all objects. This is based on the weak
generational hypothesis, which states that young objects are likely to
die early. Old objects, which have survived a few GC cycles tend to be
long-lived.

The terms young and old objects imply that there is some ordering in
object creation time and the ordering is also followed by object
references.

Specifically object references have to follow the marking direction.
In pure functional programming languages this can be a very simple
scheme:

        +------------+    object references
        v            |
   old   .... young .... youngest
                            |
                   <--------  scan direction

If (simplified) the only reference-like operation of the interpreter
is:

   cons = (car, cdr)

and the object references "car" and "cdr" are created prior to the
"aggregate" "cons", all object references point always to older
objects.  By scanning from the youngest to the oldest objects, all
non-marked objects can be reclaimed immediately. And the scan can be
aborted at any time after some processing, creating a generational GC
in a trivial way.

But the programming languages we are serving are working basically the
other direction, when it comes to object history:

  @a[$i] = $n

A reference operation like this needs first an aggregate and then the
contents of it. So the scan direction is from old objects to younger
ones.  In such a scheme it's a bit more complicated to skip parts of
the objects.

To take advantage of not processing all the objects, these are divided
into generations, e.g.:

   old               young := nursery
   generation 0      generation 1

A mark phase now processes the root set and only objects from the
young generation. When all objects are either referenced by the root
set or only by the young generation, the algorithm is correct and
complete.

But there is of course the possibilty that a young object is
stored into an aggregate of an older generation. This case is tracked
by the write barrier, which remembers all such operations in the IGP
(inter generational pointer) list. When now generation 1 is marked,
the IGP list can be considered as an extension to the root set, so
that again all live objects of the young generation are detected.


=head2 Structures

=over 4

=item C<typedef struct _gc_gms_gen Gc_gms_gen>

Describes the state of one generation for one pool.

=item C<typedef struct _gc_gms_hdr Gc_gms_hdr>

This header is in front of all Parrot objects. It forms a doubly-linked
list of all objects in one pool and points to its generation.

=item PObj_to_GMSH(o)

=item GMSH_to_PObj(p)

These two macros convert from and to headers and objects.

=item C<typedef struct _gc_gms_hdr_list Gc_gms_hdr_list>

A chained list of headers used e.g. for the IGP list.

=back

=cut

*/

#include "parrot/parrot.h"
#include "parrot/gc_api.h"
#include "parrot/gc_mark_sweep.h"

#if PARROT_GC_GMS

typedef struct Gc_gms_private {
    UINTVAL current_gen_no;             /* the nursery generation number */
} Gc_gms_private;

/* HEADERIZER HFILE: include/parrot/gc_api.h */

/* HEADERIZER BEGIN: static */
/* Don't modify between HEADERIZER BEGIN / HEADERIZER END.  Your changes will be lost. */

static int end_cycle_cb(PARROT_INTERP,
    ARGMOD(Small_Object_Pool *pool),
    int flag,
    SHIM(void *arg))
        __attribute__nonnull__(1)
        __attribute__nonnull__(2)
        FUNC_MODIFIES(*pool);

static void gc_gms_add_free_object(PARROT_INTERP,
    SHIM(Small_Object_Pool *pool),
    SHIM(PObj *to_add))
        __attribute__nonnull__(1);

static void gc_gms_alloc_objects(PARROT_INTERP,
    ARGMOD(Small_Object_Pool *pool))
        __attribute__nonnull__(1)
        __attribute__nonnull__(2)
        FUNC_MODIFIES(*pool);

static void gc_gms_chain_objects(PARROT_INTERP,
    ARGMOD(Small_Object_Pool *pool),
    ARGIN(Small_Object_Arena *new_arena),
    size_t real_size)
        __attribute__nonnull__(1)
        __attribute__nonnull__(2)
        __attribute__nonnull__(3)
        FUNC_MODIFIES(*pool);

static void gc_gms_clear_hdr_list(PARROT_INTERP, ARGMOD(Gc_gms_hdr_list *l))
        __attribute__nonnull__(1)
        __attribute__nonnull__(2)
        FUNC_MODIFIES(*l);

static void gc_gms_clear_igp(PARROT_INTERP, ARGIN(Gc_gms_gen *gen))
        __attribute__nonnull__(1)
        __attribute__nonnull__(2);

PARROT_MALLOC
PARROT_CANNOT_RETURN_NULL
static Gc_gms_gen * gc_gms_create_gen(PARROT_INTERP,
    ARGMOD(Small_Object_Pool *pool),
    size_t gen_no)
        __attribute__nonnull__(1)
        __attribute__nonnull__(2)
        FUNC_MODIFIES(*pool);

static void gc_gms_end_cycle(PARROT_INTERP)
        __attribute__nonnull__(1);

PARROT_WARN_UNUSED_RESULT
PARROT_CANNOT_RETURN_NULL
static Gc_gms_gen * gc_gms_find_gen(PARROT_INTERP,
    ARGIN(const Gc_gms_hdr *h),
    UINTVAL gen_no)
        __attribute__nonnull__(1)
        __attribute__nonnull__(2);

PARROT_WARN_UNUSED_RESULT
PARROT_CANNOT_RETURN_NULL
static PObj * gc_gms_get_free_object(PARROT_INTERP,
    ARGMOD(Small_Object_Pool *pool))
        __attribute__nonnull__(1)
        __attribute__nonnull__(2)
        FUNC_MODIFIES(*pool);

static void gc_gms_init_gen(PARROT_INTERP, ARGMOD(Small_Object_Pool *pool))
        __attribute__nonnull__(1)
        __attribute__nonnull__(2)
        FUNC_MODIFIES(*pool);

static void gc_gms_init_mark(PARROT_INTERP)
        __attribute__nonnull__(1);

static void gc_gms_merge_gen(PARROT_INTERP,
    ARGMOD(Small_Object_Pool *pool),
    int flag,
    SHIM(Gc_gms_plan *plan))
        __attribute__nonnull__(1)
        __attribute__nonnull__(2)
        FUNC_MODIFIES(*pool);

static void gc_gms_more_objects(PARROT_INTERP,
    ARGMOD(Small_Object_Pool *pool))
        __attribute__nonnull__(1)
        __attribute__nonnull__(2)
        FUNC_MODIFIES(*pool);

static void gc_gms_pool_init(PARROT_INTERP, ARGMOD(Small_Object_Pool *pool))
        __attribute__nonnull__(1)
        __attribute__nonnull__(2)
        FUNC_MODIFIES(*pool);

static void gc_gms_promote(PARROT_INTERP,
    ARGIN(Gc_gms_hdr *h),
    UINTVAL gen_no)
        __attribute__nonnull__(1)
        __attribute__nonnull__(2);

static void gc_gms_set_gen(PARROT_INTERP)
        __attribute__nonnull__(1);

static void gc_gms_setto_black(PARROT_INTERP,
    ARGMOD(Gc_gms_hdr *h),
    int priority)
        __attribute__nonnull__(1)
        __attribute__nonnull__(2)
        FUNC_MODIFIES(*h);

static void gc_gms_setto_gray(PARROT_INTERP,
    ARGIN(Gc_gms_hdr *h),
    int priority)
        __attribute__nonnull__(1)
        __attribute__nonnull__(2);

static void gc_gms_store_hdr_list(PARROT_INTERP,
    ARGMOD(Gc_gms_hdr_list *l),
    ARGIN(Gc_gms_hdr *h))
        __attribute__nonnull__(1)
        __attribute__nonnull__(2)
        __attribute__nonnull__(3)
        FUNC_MODIFIES(*l);

static void gc_gms_store_igp(PARROT_INTERP, ARGIN(Gc_gms_hdr *h))
        __attribute__nonnull__(1)
        __attribute__nonnull__(2);

static void gc_gms_sweep(PARROT_INTERP)
        __attribute__nonnull__(1);

static int gc_gms_trace_children(PARROT_INTERP)
        __attribute__nonnull__(1);

static int gc_gms_trace_root(PARROT_INTERP, int trace_stack)
        __attribute__nonnull__(1);

static void gc_gms_use_gen(PARROT_INTERP,
    ARGMOD(Small_Object_Pool *pool),
    int flag,
    ARGIN(const Gc_gms_plan *plan))
        __attribute__nonnull__(1)
        __attribute__nonnull__(2)
        __attribute__nonnull__(4)
        FUNC_MODIFIES(*pool);

static void gms_debug_verify(PARROT_INTERP,
    ARGMOD(Small_Object_Pool *pool),
    ARGIN(const char *action))
        __attribute__nonnull__(1)
        __attribute__nonnull__(2)
        __attribute__nonnull__(3)
        FUNC_MODIFIES(*pool);

static int init_mark_cb(PARROT_INTERP,
    ARGMOD(Small_Object_Pool *pool),
    int flag,
    ARGIN(void *arg))
        __attribute__nonnull__(1)
        __attribute__nonnull__(2)
        __attribute__nonnull__(4)
        FUNC_MODIFIES(*pool);

static void parrot_gc_gms_deinit(PARROT_INTERP)
        __attribute__nonnull__(1);

static void parrot_gc_gms_run(PARROT_INTERP, UINTVAL flags)
        __attribute__nonnull__(1);

PARROT_WARN_UNUSED_RESULT
static int set_gen_cb(PARROT_INTERP,
    ARGIN(Small_Object_Pool *pool),
    int flag,
    ARGIN(void *arg))
        __attribute__nonnull__(1)
        __attribute__nonnull__(2)
        __attribute__nonnull__(4);

static int sweep_cb_buf(PARROT_INTERP,
    ARGMOD(Small_Object_Pool *pool),
    int flag,
    SHIM(void *arg))
        __attribute__nonnull__(1)
        __attribute__nonnull__(2)
        FUNC_MODIFIES(*pool);

static int sweep_cb_pmc(PARROT_INTERP,
    ARGIN(Small_Object_Pool *pool),
    int flag,
    SHIM(void *arg))
        __attribute__nonnull__(1)
        __attribute__nonnull__(2);

static int trace_children_cb(PARROT_INTERP,
    ARGIN(Small_Object_Pool *pool),
    int flag,
    SHIM(void *arg))
        __attribute__nonnull__(1)
        __attribute__nonnull__(2);

static int trace_igp_cb(PARROT_INTERP,
    ARGIN(Small_Object_Pool *pool),
    int flag,
    SHIM(void *arg))
        __attribute__nonnull__(1)
        __attribute__nonnull__(2);

#define ASSERT_ARGS_end_cycle_cb __attribute__unused__ int _ASSERT_ARGS_CHECK = \
       PARROT_ASSERT_ARG(interp) \
    || PARROT_ASSERT_ARG(pool)
#define ASSERT_ARGS_gc_gms_add_free_object __attribute__unused__ int _ASSERT_ARGS_CHECK = \
       PARROT_ASSERT_ARG(interp)
#define ASSERT_ARGS_gc_gms_alloc_objects __attribute__unused__ int _ASSERT_ARGS_CHECK = \
       PARROT_ASSERT_ARG(interp) \
    || PARROT_ASSERT_ARG(pool)
#define ASSERT_ARGS_gc_gms_chain_objects __attribute__unused__ int _ASSERT_ARGS_CHECK = \
       PARROT_ASSERT_ARG(interp) \
    || PARROT_ASSERT_ARG(pool) \
    || PARROT_ASSERT_ARG(new_arena)
#define ASSERT_ARGS_gc_gms_clear_hdr_list __attribute__unused__ int _ASSERT_ARGS_CHECK = \
       PARROT_ASSERT_ARG(interp) \
    || PARROT_ASSERT_ARG(l)
#define ASSERT_ARGS_gc_gms_clear_igp __attribute__unused__ int _ASSERT_ARGS_CHECK = \
       PARROT_ASSERT_ARG(interp) \
    || PARROT_ASSERT_ARG(gen)
#define ASSERT_ARGS_gc_gms_create_gen __attribute__unused__ int _ASSERT_ARGS_CHECK = \
       PARROT_ASSERT_ARG(interp) \
    || PARROT_ASSERT_ARG(pool)
#define ASSERT_ARGS_gc_gms_end_cycle __attribute__unused__ int _ASSERT_ARGS_CHECK = \
       PARROT_ASSERT_ARG(interp)
#define ASSERT_ARGS_gc_gms_find_gen __attribute__unused__ int _ASSERT_ARGS_CHECK = \
       PARROT_ASSERT_ARG(interp) \
    || PARROT_ASSERT_ARG(h)
#define ASSERT_ARGS_gc_gms_get_free_object __attribute__unused__ int _ASSERT_ARGS_CHECK = \
       PARROT_ASSERT_ARG(interp) \
    || PARROT_ASSERT_ARG(pool)
#define ASSERT_ARGS_gc_gms_init_gen __attribute__unused__ int _ASSERT_ARGS_CHECK = \
       PARROT_ASSERT_ARG(interp) \
    || PARROT_ASSERT_ARG(pool)
#define ASSERT_ARGS_gc_gms_init_mark __attribute__unused__ int _ASSERT_ARGS_CHECK = \
       PARROT_ASSERT_ARG(interp)
#define ASSERT_ARGS_gc_gms_merge_gen __attribute__unused__ int _ASSERT_ARGS_CHECK = \
       PARROT_ASSERT_ARG(interp) \
    || PARROT_ASSERT_ARG(pool)
#define ASSERT_ARGS_gc_gms_more_objects __attribute__unused__ int _ASSERT_ARGS_CHECK = \
       PARROT_ASSERT_ARG(interp) \
    || PARROT_ASSERT_ARG(pool)
#define ASSERT_ARGS_gc_gms_pool_init __attribute__unused__ int _ASSERT_ARGS_CHECK = \
       PARROT_ASSERT_ARG(interp) \
    || PARROT_ASSERT_ARG(pool)
#define ASSERT_ARGS_gc_gms_promote __attribute__unused__ int _ASSERT_ARGS_CHECK = \
       PARROT_ASSERT_ARG(interp) \
    || PARROT_ASSERT_ARG(h)
#define ASSERT_ARGS_gc_gms_set_gen __attribute__unused__ int _ASSERT_ARGS_CHECK = \
       PARROT_ASSERT_ARG(interp)
#define ASSERT_ARGS_gc_gms_setto_black __attribute__unused__ int _ASSERT_ARGS_CHECK = \
       PARROT_ASSERT_ARG(interp) \
    || PARROT_ASSERT_ARG(h)
#define ASSERT_ARGS_gc_gms_setto_gray __attribute__unused__ int _ASSERT_ARGS_CHECK = \
       PARROT_ASSERT_ARG(interp) \
    || PARROT_ASSERT_ARG(h)
#define ASSERT_ARGS_gc_gms_store_hdr_list __attribute__unused__ int _ASSERT_ARGS_CHECK = \
       PARROT_ASSERT_ARG(interp) \
    || PARROT_ASSERT_ARG(l) \
    || PARROT_ASSERT_ARG(h)
#define ASSERT_ARGS_gc_gms_store_igp __attribute__unused__ int _ASSERT_ARGS_CHECK = \
       PARROT_ASSERT_ARG(interp) \
    || PARROT_ASSERT_ARG(h)
#define ASSERT_ARGS_gc_gms_sweep __attribute__unused__ int _ASSERT_ARGS_CHECK = \
       PARROT_ASSERT_ARG(interp)
#define ASSERT_ARGS_gc_gms_trace_children __attribute__unused__ int _ASSERT_ARGS_CHECK = \
       PARROT_ASSERT_ARG(interp)
#define ASSERT_ARGS_gc_gms_trace_root __attribute__unused__ int _ASSERT_ARGS_CHECK = \
       PARROT_ASSERT_ARG(interp)
#define ASSERT_ARGS_gc_gms_use_gen __attribute__unused__ int _ASSERT_ARGS_CHECK = \
       PARROT_ASSERT_ARG(interp) \
    || PARROT_ASSERT_ARG(pool) \
    || PARROT_ASSERT_ARG(plan)
#define ASSERT_ARGS_gms_debug_verify __attribute__unused__ int _ASSERT_ARGS_CHECK = \
       PARROT_ASSERT_ARG(interp) \
    || PARROT_ASSERT_ARG(pool) \
    || PARROT_ASSERT_ARG(action)
#define ASSERT_ARGS_init_mark_cb __attribute__unused__ int _ASSERT_ARGS_CHECK = \
       PARROT_ASSERT_ARG(interp) \
    || PARROT_ASSERT_ARG(pool) \
    || PARROT_ASSERT_ARG(arg)
#define ASSERT_ARGS_parrot_gc_gms_deinit __attribute__unused__ int _ASSERT_ARGS_CHECK = \
       PARROT_ASSERT_ARG(interp)
#define ASSERT_ARGS_parrot_gc_gms_run __attribute__unused__ int _ASSERT_ARGS_CHECK = \
       PARROT_ASSERT_ARG(interp)
#define ASSERT_ARGS_set_gen_cb __attribute__unused__ int _ASSERT_ARGS_CHECK = \
       PARROT_ASSERT_ARG(interp) \
    || PARROT_ASSERT_ARG(pool) \
    || PARROT_ASSERT_ARG(arg)
#define ASSERT_ARGS_sweep_cb_buf __attribute__unused__ int _ASSERT_ARGS_CHECK = \
       PARROT_ASSERT_ARG(interp) \
    || PARROT_ASSERT_ARG(pool)
#define ASSERT_ARGS_sweep_cb_pmc __attribute__unused__ int _ASSERT_ARGS_CHECK = \
       PARROT_ASSERT_ARG(interp) \
    || PARROT_ASSERT_ARG(pool)
#define ASSERT_ARGS_trace_children_cb __attribute__unused__ int _ASSERT_ARGS_CHECK = \
       PARROT_ASSERT_ARG(interp) \
    || PARROT_ASSERT_ARG(pool)
#define ASSERT_ARGS_trace_igp_cb __attribute__unused__ int _ASSERT_ARGS_CHECK = \
       PARROT_ASSERT_ARG(interp) \
    || PARROT_ASSERT_ARG(pool)
/* Don't modify between HEADERIZER BEGIN / HEADERIZER END.  Your changes will be lost. */
/* HEADERIZER END: static */


/*

=pod

 * XXX

Main problem TODO 1):

 [ PCont ]       ... continuation object in old generation
    |
    v
 [ Stack chunk ] --> [ e.g. P register frame ]  ... new generation

By pushing a new stack chunk onto the (old) existing stack frame,
we'd need a WRITE_BARRIER that promotes the stack chunk to the old
generation of the continuation.
This would also need an IGP entry for the stack chunk buffer. But -
as buffers aren't really containers in Parrot - this isn't possible.

To get that right, the code needs better support by the running
interpreter.
 - never promote continuations (and stacks) in the current stack frame
   to an old generation
 - create scope_enter / scope_exit opcodes

A scope_enter happens on a subroutine call *and' with new_pad /
push_pad opcodes. Each lexical scope must have its distinct register
frame, else timely destruction can't work.
If the frame needs active destruction, the old frame should be
converted to the (new-1) generation, the inner frame is the nursery.
On scope exit the newest (nursery) generation is collected and the
current generation number is reset back to (new-1).

If the scope_enter doesn't indicate timely destruction, generation
promoting should be done only, if object statistics indicate the
presence of a fair amount of live objects.

TODO 2) in lazy sweep
If timely destruction didn't find (all) eager objects, go back to
older generations, until all these objects have been seen.

TODO 3) interpreter startup
After all internal structures are created, promote interpreter state
into initial first old generation by running one GC cycle before
program execution begins (or just treat all objects as being alive).

=cut

*/

/*
 * call code to verify chain of pointers after each change
 * this is very expensive, but should be used during development
 */
#  define GC_GMS_DEBUG 0

#  define UNITS_PER_ALLOC_GROWTH_FACTOR 1.75
#  define POOL_MAX_BYTES 65536*128

/*

=head2 Initialization functions

=over 4

=item C<static void parrot_gc_gms_deinit>

Free used resources.

=cut

*/

static void
parrot_gc_gms_deinit(PARROT_INTERP)
{
    ASSERT_ARGS(parrot_gc_gms_deinit)
    Arenas * const arena_base = interp->arena_base;

    /*
     * TODO free generations
     */
    mem_sys_free(arena_base->gc_private);
    arena_base->gc_private = NULL;
}

/*

=item C<static void gc_gms_pool_init>

Initialize pool variables. This function must set the pool function pointers
for C<add_free_object>, C<get_free_object>, C<alloc_objects>, and
C<more_objects>.

=cut

*/

static void
gc_gms_pool_init(PARROT_INTERP, ARGMOD(Small_Object_Pool *pool))
{
    ASSERT_ARGS(gc_gms_pool_init)
    pool->add_free_object = gc_gms_add_free_object;
    pool->get_free_object = gc_gms_get_free_object;
    pool->alloc_objects   = gc_gms_alloc_objects;
    pool->more_objects    = gc_gms_more_objects;

    /* initialize generations */
    gc_gms_init_gen(interp, pool);
    pool->white = pool->white_fin = pool->free_list = &pool->marker;

    pool->object_size += sizeof (Gc_gms_hdr);
}

/*

=item C<void Parrot_gc_gms_init>

Initialize the state structures of the gc system. Called immediately before
creation of memory pools.

=cut

*/

PARROT_EXPORT
void
Parrot_gc_gms_init(PARROT_INTERP)
{
    ASSERT_ARGS(Parrot_gc_gms_init)
    Arenas * const arena_base = interp->arena_base;

    arena_base->gc_private = mem_sys_allocate_zeroed(sizeof (Gc_gms_private));

    /*
     * set function hooks according to pdd09
     */
    arena_base->do_gc_mark         = parrot_gc_gms_run;
    arena_base->finalize_gc_system = parrot_gc_gms_deinit;
    arena_base->init_pool          = gc_gms_pool_init;

}

/*

=back

=head2 Interface functions

=over 4

=item C<static void gc_gms_add_free_object>

Unused. White (dead) objects are added in a bunch to the free_list.

=cut

*/

static void
gc_gms_add_free_object(PARROT_INTERP, SHIM(Small_Object_Pool *pool),
        SHIM(PObj *to_add))
{
    ASSERT_ARGS(gc_gms_add_free_object)
    Parrot_ex_throw_from_c_args(interp, NULL, 1, "gms abuse");
}


/*

=item C<static void gc_gms_chain_objects>

TODO: interfere active_destroy and put these items into a
separate white area, so that a sweep has just to run through these
objects

Header chain layout:
- all objects are chained together forming a circular list
- pool->marker is the "anchor" of the circle (shown twice below)

 1) object allocation

 1a) one bunch of allocated objects was consumed: the free ptr did
     hit the marker

 +===+---+---+---+---+---+===+
 I M I w | w | w | w | w I M I
 +   +---+---+---+---+---+   +
       ^                   ^
       |                   |
       white               free == marker

 All these pointer ranges include the first element, but not the last one.

  [white ... free_list)   is the list of all whites

 1b) after allocating another bunch of objects

 +===+---+---+---+---+---+---+---+---+---+---+===+
 I M I w | w | w | w | w | f | f | f | f | f I M I
 +   +---+---+---+---+---+---+---+---+---+---+   +
       ^                   ^                   ^
       |                   |                   |
       white               free                marker

=cut

*/

static void
gc_gms_chain_objects(PARROT_INTERP, ARGMOD(Small_Object_Pool *pool),
        ARGIN(Small_Object_Arena *new_arena), size_t real_size)
{
    ASSERT_ARGS(gc_gms_chain_objects)
    Gc_gms_hdr *next, *prev;
    size_t i, n;

    Gc_gms_hdr *p = new_arena->start_objects;
    Gc_gms_hdr * const marker = &pool->marker;

    PARROT_ASSERT(pool->free_list == marker);

    /* update pool statistics */
    n = new_arena->total_objects;
    pool->total_objects += n;
    pool->num_free_objects += n;
    new_arena->used = n;
    /* initially all is pointing to marker */
    if (pool->white == marker) {
        /* set origin of first allocation */
        marker->next = p;
        p->prev = marker;
        pool->white = pool->white_fin = p;
        prev = marker;
    }
    else
        prev = marker->prev;
    /* chain objects together by inserting to the left of marker */

    /* point to end of last object */
    p = (void*) ((char*) p + real_size * n);
    next = marker;
    for (i = 0; i < n; ++i) {
        p = (void*) ((char *)p - real_size);
        p->next = next;
        next->prev = p;
#  ifndef NDEBUG
        p->gen = (void *)0xdeadbeef;
#  endif
        next = p;
    }
    PARROT_ASSERT(p == new_arena->start_objects);
    p->prev = prev;
    prev->next = p;
    pool->free_list = p;
    PARROT_ASSERT(p != marker);
}

/*

=item C<static void gc_gms_alloc_objects>

Allocate new objects for the given pool.

=cut

*/

static void
gc_gms_alloc_objects(PARROT_INTERP, ARGMOD(Small_Object_Pool *pool))
{
    ASSERT_ARGS(gc_gms_alloc_objects)
    const size_t real_size = pool->object_size;
    Small_Object_Arena * const new_arena = mem_internal_allocate(sizeof (Small_Object_Arena));
    const size_t size = real_size * pool->objects_per_alloc;

    new_arena->start_objects = mem_internal_allocate(size);
    /* insert arena in list */
    Parrot_append_arena_in_pool(interp, pool, new_arena, size);
    /* create chain of objects, set free pointer */
    gc_gms_chain_objects(interp, pool, new_arena, real_size);

    /* allocate more next time */
    pool->objects_per_alloc = (UINTVAL) pool->objects_per_alloc *
        UNITS_PER_ALLOC_GROWTH_FACTOR;
    size = real_size * pool->objects_per_alloc;
    if (size > POOL_MAX_BYTES) {
        pool->objects_per_alloc = POOL_MAX_BYTES / real_size;
    }
}

/*

=item C<static void gc_gms_more_objects>

Run a GC cycle or allocate new objects for the given pool.

=cut

*/

static void
gc_gms_more_objects(PARROT_INTERP, ARGMOD(Small_Object_Pool *pool))
{
    ASSERT_ARGS(gc_gms_more_objects)
    if (pool->skip)
        pool->skip = 0;
    else if (pool->last_Arena) {
        Parrot_do_gc_run(interp, GC_trace_stack_FLAG);
        if (pool->num_free_objects <= pool->replenish_level)
            pool->skip = 1;
    }

    if (pool->free_list == &pool->marker) {
        (*pool->alloc_objects) (interp, pool);
    }
}

/*

=item C<static PObj * gc_gms_get_free_object>

Get a new object off the free_list in the given pool.

2) object consumption
   the free ptr moves towards the marker

=cut

*/

PARROT_WARN_UNUSED_RESULT
PARROT_CANNOT_RETURN_NULL
static PObj *
gc_gms_get_free_object(PARROT_INTERP, ARGMOD(Small_Object_Pool *pool))
{
    ASSERT_ARGS(gc_gms_get_free_object)
    PObj *ptr;
    Gc_gms_hdr *hdr;

    hdr = pool->free_list;
    if (hdr == &pool->marker)
        (pool->more_objects)(interp, pool);

    hdr = pool->free_list;
    pool->free_list = hdr->next;
    hdr->gen = pool->last_gen;
    ptr = GMSH_to_PObj(hdr);
    PObj_flags_SETTO((PObj*) ptr, 0);
    return ptr;
}

/*

=back

=head2 Generation handling functions

overall header chain layout

           gen 0         gen 1      ...    gen N
  marker [first last) [first last)  ...   [first last)  marker

The last (youngest) generation N holds these (pool) pointers:

  [ black ... gray )          during marking
  [ gray ... white )          during marking
  [ white ... free_list )     allocated items
  [ free_list ... marker )    free items

The black, white, and generation ranges have additionally (TODO)
*fin variants, which refer to PMCs that need destruction/finalization.
These are always in front of the ranges to be processed first.

=over 4

=item C<static Gc_gms_gen * gc_gms_create_gen>

Create a generation structure for the given generation number.

=cut

*/

PARROT_MALLOC
PARROT_CANNOT_RETURN_NULL
static Gc_gms_gen *
gc_gms_create_gen(PARROT_INTERP, ARGMOD(Small_Object_Pool *pool), size_t gen_no)
{
    ASSERT_ARGS(gc_gms_create_gen)
    Gc_gms_gen * const gen = mem_sys_allocate(sizeof (*gen));

    gen->gen_no = gen_no;
    gen->pool = pool;
    gen->timely_destruct_obj_sofar = 0;
    gen->black_color = b_PObj_live_FLAG;
    gen->prev = NULL;
    gen->next = NULL;
    gen->first = gen->last = gen->fin = &pool->marker;
    gen->igp.first = NULL;
    gen->igp.last = NULL;

    return gen;
}

/*

=item C<static void gc_gms_init_gen>

Initalize the generation system by creating the first two generations.

=cut

*/

static void
gc_gms_init_gen(PARROT_INTERP, ARGMOD(Small_Object_Pool *pool))
{
    ASSERT_ARGS(gc_gms_init_gen)
    Gc_gms_private *gmsp;
    /*
     * Generations are numbered beginning at zero
     * 0 ... oldest
     * 1 ... next oldest
     *
     * If a constant hash PMC refers to non-constant header buffers or
     * items, these items can be placed in the constant generation 0
     * XXX: OTOH this would work only for this GC subsystem.
     */
    pool->first_gen = gc_gms_create_gen(interp, pool, 0);
    pool->last_gen = pool->first_gen;
    gmsp = interp->arena_base->gc_private;
    gmsp->current_gen_no = 0;
}

/*

=item C<static Gc_gms_gen * gc_gms_find_gen>

RT #48260: Not yet documented!!!

=cut

*/

PARROT_WARN_UNUSED_RESULT
PARROT_CANNOT_RETURN_NULL
static Gc_gms_gen *
gc_gms_find_gen(PARROT_INTERP, ARGIN(const Gc_gms_hdr *h), UINTVAL gen_no)
{
    ASSERT_ARGS(gc_gms_find_gen)
    Gc_gms_gen *gen;
    const Small_Object_Pool * const pool = h->gen->pool;

    PARROT_ASSERT(pool);

    for (gen = pool->first_gen; gen; gen = gen->next) {
        if (gen_no == gen->gen_no)
            return gen;
        if (gen->gen_no > gen_no) {
            gen = NULL;
            break;
        }
    }
    /* we could create generations lazily - not all object sizes
     * might exist in every generation
     *
     * TODO insert generation
     */
    Parrot_ex_throw_from_c_args(interp, NULL, 1,
        "generation %d not found for hdr %p", gen_no, h);
}

/*

=item C<static void gc_gms_promote>

RT #48260: Not yet documented!!!

=cut

*/

static void
gc_gms_promote(PARROT_INTERP, ARGIN(Gc_gms_hdr *h), UINTVAL gen_no)
{
    ASSERT_ARGS(gc_gms_promote)
    Gc_gms_gen *gen;
    Gc_gms_hdr *prev, *next;
    Small_Object_Pool * const pool = h->gen->pool;

    /* unsnap from current generation */
    prev = h->prev;
    next = h->next;
    if (h == pool->white) {
        pool->white = next;
    }
    prev->next = next;
    next->prev = prev;

    /* locate generation pointer */
    gen = gc_gms_find_gen(interp, h, gen_no);
    PARROT_ASSERT(gen->last);
    PARROT_ASSERT(gen->first);

    /* TODO if it needs destroy put it in front */
    next = gen->last;
    if (h == next)
        next = gen->last = h->next;
    prev = next->prev;
    if (gen->first == &pool->marker)
        gen->first = h;
    h->prev = prev;
    h->next = next;
    prev->next = h;
    next->prev = h;
#  if GC_GMS_DEBUG
    gms_debug_verify(interp, pool, "promote");
#  endif
}

/*

=item C<static void gc_gms_store_hdr_list>

RT #48260: Not yet documented!!!

=cut

*/

static void
gc_gms_store_hdr_list(PARROT_INTERP, ARGMOD(Gc_gms_hdr_list *l), ARGIN(Gc_gms_hdr *h))
{
    ASSERT_ARGS(gc_gms_store_hdr_list)
    Gc_gms_hdr_store * const s = l->last;

    /* if it's not created or if it's full allocate new store */
    if (!s || s->ptr == &s->store[GC_GMS_STORE_SIZE]) {
        s = mem_sys_allocate(sizeof (Gc_gms_hdr_store));
        s->ptr = &s->store[0];
        s->next = NULL;
        /* chain new store to old one */
        if (l->first) {
            PARROT_ASSERT(l->last);
            l->last->next = s;
        }
        else {
            l->first = s;
        }
        l->last = s;
    }
    *(s->ptr)++ = h;
}

/*

=item C<static void gc_gms_clear_hdr_list>

RT #48260: Not yet documented!!!

=cut

*/

static void
gc_gms_clear_hdr_list(PARROT_INTERP, ARGMOD(Gc_gms_hdr_list *l))
{
    ASSERT_ARGS(gc_gms_clear_hdr_list)
    Gc_gms_hdr_store *s, *next;

    for (s = l->first; s; s = next) {
        next = s->next;
        mem_sys_free(s);
    }
    l->first = l->last = NULL;
}

/*

=item C<static void gc_gms_store_igp>

RT #48260: Not yet documented!!!

=cut

*/

static void
gc_gms_store_igp(PARROT_INTERP, ARGIN(Gc_gms_hdr *h))
{
    ASSERT_ARGS(gc_gms_store_igp)
    Gc_gms_gen * const gen = h->gen;
    Gc_gms_hdr_list * const igp = &gen->igp;

    gc_gms_store_hdr_list(interp, igp, h);
}

/*

=item C<static void gc_gms_clear_igp>

RT #48260: Not yet documented!!!

=cut

*/

static void
gc_gms_clear_igp(PARROT_INTERP, ARGIN(Gc_gms_gen *gen))
{
    ASSERT_ARGS(gc_gms_clear_igp)
    Gc_gms_hdr_list * const igp = &gen->igp;

    gc_gms_clear_hdr_list(interp, igp);
}

/*

=item C<void parrot_gc_gms_wb>

Called by the write barrier. The aggregate belongs to an older generation
then the I<new> value written into it. Put the header of the new value
onto the IGP list for the current generation, if it contains pointers
to other items, and promote it to the old generation.

=cut

*/

void
parrot_gc_gms_wb(PARROT_INTERP, ARGIN(PMC *agg), ARGIN(void *old),
    ARGIN(void *_new))
{
    ASSERT_ARGS(parrot_gc_gms_wb)
    Gc_gms_hdr * const nh = PObj_to_GMSH(_new);
    Gc_gms_hdr * const ah = PObj_to_GMSH(agg);

    /* if this may be an aggregate store it in IGP list, thus making
     * it a possible root for this generation
     */
    if (PObj_is_PMC_TEST((PObj *)_new) && ((PMC *)_new)->pmc_ext)
        gc_gms_store_igp(interp, nh);

    /* promote RHS to old generation of aggregate */
    gc_gms_promote(interp, nh, ah->gen->gen_no);

    /*
     * TODO check old - its overwritten, increment overwrite count,
     * if it's an aggregate all contents *may* be dead now, so
     * increment overwrite count by elements
     */
}

/*

=item C<void parrot_gc_gms_wb_key>

RT #48260: Not yet documented!!!

=cut

*/

void
parrot_gc_gms_wb_key(PARROT_INTERP, ARGIN(PMC *agg), ARGIN(void *old),
    ARGIN(void *old_key), ARGIN(void *_new), ARGIN(void *new_key))
{
    ASSERT_ARGS(parrot_gc_gms_wb_key)
    Gc_gms_hdr *nh, *ah;

    /* handle hash values */
    parrot_gc_gms_wb(interp, agg, old, _new);

    /* if hash keys are PObj* then promote new key too */

    /* TODO: check if key is a PObj */

    nh = PObj_to_GMSH(new_key);
    ah = PObj_to_GMSH(agg);

    /* promote new key to old generation of aggregate */
    gc_gms_promote(interp, nh, ah->gen->gen_no);
}

typedef struct Gc_gms_plan {
    int merge_gen;
    int gen_no;
} Gc_gms_plan;

/*

=item C<static void gc_gms_merge_gen>

RT #48260: Not yet documented!!!

=cut

*/

static void
gc_gms_merge_gen(PARROT_INTERP, ARGMOD(Small_Object_Pool *pool),
        int flag, SHIM(Gc_gms_plan *plan))
{
    ASSERT_ARGS(gc_gms_merge_gen)
    Gc_gms_hdr *h;

    /* run through the blacks and set their generation pointer
     * to the previous generation
     */
    Gc_gms_gen * const gen = pool->last_gen;
    Gc_gms_gen * const prev = gen->prev;

    for (h = pool->black; h != pool->free_list; h = h->next) {
        h->gen = prev;
        /* TODO update statistics */
        /* TODO merge hdrs that need finalization */
    }
    prev->last = pool->free_list;
    /*
     * clear IGP for gen
     */
    gc_gms_clear_igp(interp, gen);
}

/*

=item C<static void gc_gms_use_gen>

RT #48260: Not yet documented!!!

=cut

*/

static void
gc_gms_use_gen(PARROT_INTERP, ARGMOD(Small_Object_Pool *pool),
        int flag, ARGIN(const Gc_gms_plan *plan))
{
    ASSERT_ARGS(gc_gms_use_gen)
    Gc_gms_gen *gen, *prev;
    UINTVAL next_gen;

    /* set hdr pointers in last generation */
    gen        = pool->last_gen;
    gen->first = pool->black;
    gen->fin   = pool->black_fin;
    gen->last  = pool->free_list;

    /* create and append a new generation */
    next_gen       = plan->gen_no + 1;
    gen            = gc_gms_create_gen(interp, pool, next_gen);
    prev           = pool->last_gen;
    pool->last_gen = gen;
    prev->next     = gen;
    gen->prev      = prev;

    /* set generation in interpreter */
    interp->gc_generation = next_gen;
}

/*

=item C<static int set_gen_cb>

RT #48260: Not yet documented!!!

=cut

*/

PARROT_WARN_UNUSED_RESULT
static int
set_gen_cb(PARROT_INTERP, ARGIN(Small_Object_Pool *pool), int flag, ARGIN(void *arg))
{
    ASSERT_ARGS(set_gen_cb)
    Gc_gms_plan * const plan = (Gc_gms_plan *)arg;

    if (plan->merge_gen)
        gc_gms_merge_gen(interp, pool, flag, plan);
    else
        gc_gms_use_gen(interp, pool, flag, plan);
    return 0;
}

/*

=item C<static void gc_gms_set_gen>

RT #48260: Not yet documented!!!

=cut

*/

static void
gc_gms_set_gen(PARROT_INTERP)
{
    ASSERT_ARGS(gc_gms_set_gen)
    Gc_gms_plan plan;
    Gc_gms_private *gmsp;
    /*
     * there are these basic plans
     * 1) Use the black as the next old generation
     * 2) Merge the blacks to the existing older generation
     *    The plan to use depends on the interpreter, specifically, if
     *    we are doing a lazy run, entering a new scope, or what not.
     * 3) If we are leaving a scope (denoted by a lazy DOD run
     *    and we had created one or more generations in this scope
     *    go back by resetting the generation number to the outer
     *    scope's generation
     * 4) Check the overwrite count of older generations. If there is
     *    a significant percentage of possibly dead objects, scan
     *    older generations too.
     *
     * TODO only 1 and 2 done for now
     *      3) and 4) need to reset live flags of the previous generation(s)
     *      or better use the per-generation black_color for marking
     */
    gmsp = interp->arena_base->gc_private;
    plan.merge_gen = 0;
    plan.gen_no = gmsp->current_gen_no;
    if (gmsp->current_gen_no > 0)
        plan.merge_gen = 1;
    else
        gmsp->current_gen_no = 1;
    Parrot_forall_header_pools(interp, POOL_ALL, &plan, set_gen_cb);
}

/*

=back

=head2 Marking functions

Header chain layout

Init: gray := black := white

 3) marking the root set

 3a) the white 'h' is to be set to gray to be scanned for children

 +---+---+---+---+---+---+->      +---+->
 | b | b | g | g | g | w          | h |
 +---+---+---+---+---+---+      <-+---+
   ^       ^           ^
   |       |           |
   black   gray        white

 3b) DFS if 'h' needs timely destruction

 +---+---+---+---+---+---+---+->
 | b | b | h | g | g | g | w
 +---+---+---+---+---+---+---+
   ^       ^               ^
   |       |               |
   black   gray            white


 3c) BFS in the normal case

 +---+---+---+---+---+---+---+->
 | b | b | g | g | g | h | w
 +---+---+---+---+---+---+---+
   ^       ^               ^
   |       |               |
   black   gray            white

 3d) the white is a scalar and immediately blackened


 +---+---+---+---+---+---+---+->
 | b | b | h | g | g | g | w
 +---+---+---+---+---+---+---+
   ^           ^           ^
   |           |           |
   black       gray        white

 3e) blacken the gray 'h' during trace_children

 +---+---+---+---+---+---+---+->
 | b | b | h | g | g | g | w
 +---+---+---+---+---+---+---+
   ^       ^               ^
   |       |               |
   black   gray            white


 +---+---+---+---+---+---+---+->
 | b | b | h | g | g | g | w
 +---+---+---+---+---+---+---+
   ^           ^           ^
   |           |           |
   black       gray        white

=over 4

=cut

*/

/*

=item C<static void gc_gms_setto_gray>

Set the white header C<h> to gray.

=cut

*/

static void
gc_gms_setto_gray(PARROT_INTERP, ARGIN(Gc_gms_hdr *h), int priority)
{
    ASSERT_ARGS(gc_gms_setto_gray)
    Small_Object_Pool * const pool = h->gen->pool;
    /*
     * TODO high_priority like in src/gc/api.c
     */
    /*
     * if the white is adjacent to gray, move pointer
     */
    if (pool->white == h && (!priority || pool->white == pool->gray))
        pool->white = h->next;
    else {
        Gc_gms_hdr *next, *prev;

        prev = h->prev;
        next = h->next;
        if (h == pool->white)
            pool->white = next;
        prev->next = next;
        next->prev = prev;

        if (priority) {
            /* insert at gray */
            next = pool->gray;       /* DFS */
            pool->gray = h;
        }
        else {
            /* insert before white */
            next = pool->white;                  /* BFS */
        }
        prev = next->prev;
        h->next = next;
        h->prev = prev;
        next->prev = h;
        prev->next = h;

        /* if there wasn't any gray or black before */
        if (pool->gray == pool->white) {
            pool->gray = h;
            if (pool->black == pool->white) {
                pool->black = h;
            }
        }
    }
    PARROT_ASSERT(h != pool->white);
    /* verify all these pointer moves */
#  if GC_GMS_DEBUG
    gms_debug_verify(interp, pool, "to_gray");
#  endif
}

/*

=item C<static void gc_gms_setto_black>

Set the white header C<h> to black.

=cut

*/

static void
gc_gms_setto_black(PARROT_INTERP, ARGMOD(Gc_gms_hdr *h), int priority)
{
    ASSERT_ARGS(gc_gms_setto_black)
    Small_Object_Pool * const pool = h->gen->pool;

    /*
     * TODO high_priority like src/gc/api.c
     * TODO if h needs destructions insert in front of chain
     */
    /*
     * if the white is adjacent to black, move pointer
     */
    if (pool->black == h) {
        PARROT_ASSERT(pool->gray == h);
        PARROT_ASSERT(pool->white == h);
        pool->white = h->next;
        pool->gray = h->next;
    }
    else {
        Gc_gms_hdr *next, *prev;

        prev = h->prev;
        next = h->next;
        if (h == pool->white) {
            pool->white = next;
            if (h == pool->gray)
                pool->gray = next;
        }
        prev->next = next;
        next->prev = prev;

        /* insert before gray */
        next = pool->gray;
        prev = next->prev;
        h->next = next;
        h->prev = prev;
        next->prev = h;
        prev->next = h;
        if (pool->black == pool->gray) {
            pool->black = h;
        }
    }
    PARROT_ASSERT(h != pool->white);
    PARROT_ASSERT(h != pool->gray);
#  if GC_GMS_DEBUG
    gms_debug_verify(interp, pool, "to_black");
#  endif
}

/*

=item C<void parrot_gc_gms_pobject_lives>

Set the object live - called by the pobject_lives macro

=cut

*/

PARROT_EXPORT
void
parrot_gc_gms_pobject_lives(PARROT_INTERP, ARGMOD(PObj *obj))
{
    ASSERT_ARGS(parrot_gc_gms_pobject_lives)
    Gc_gms_hdr *h;
    int priority;

    PObj_live_SET(obj);
    priority =  PObj_needs_early_gc_TEST(obj);
    if (priority)
        ++interp->arena_base->num_early_PMCs_seen;
    h = PObj_to_GMSH(obj);
    /* unsnap it from white, put it into gray or black */
    if (PObj_is_PMC_TEST(obj) && ((PMC*)obj)->pmc_ext)
        gc_gms_setto_gray(interp, h, priority);
    else
        gc_gms_setto_black(interp, h, priority);
}

/*

=item C<static int init_mark_cb>

RT #48260: Not yet documented!!!

=cut

*/

static int
init_mark_cb(PARROT_INTERP, ARGMOD(Small_Object_Pool *pool), int flag, ARGIN(void *arg))
{
    ASSERT_ARGS(init_mark_cb)
    pool->gray = pool->black = pool->black_fin = pool->white;
#  if GC_GMS_DEBUG
    gms_debug_verify(interp, pool, "init_mark");
#  endif
    return 0;
}

/*

=item C<static void gc_gms_init_mark>

Initialize the mark phase of GC.

=cut

*/

static void
gc_gms_init_mark(PARROT_INTERP)
{
    ASSERT_ARGS(gc_gms_init_mark)
    Arenas * const arena_base = interp->arena_base;

    arena_base->gc_trace_ptr        = NULL;
    arena_base->gc_mark_start       = NULL;
    arena_base->num_early_PMCs_seen = 0;
    arena_base->num_extended_PMCs   = 0;

    Parrot_forall_header_pools(interp, POOL_ALL, 0, init_mark_cb);
}

/*

=item C<static int trace_igp_cb>

RT #48260: Not yet documented!!!

=cut

*/

static int
trace_igp_cb(PARROT_INTERP, ARGIN(Small_Object_Pool *pool), int flag, SHIM(void *arg))
{
    ASSERT_ARGS(trace_igp_cb)
    Gc_gms_hdr_store *s;
    Gc_gms_gen * const gen = pool->last_gen;
    Gc_gms_hdr_list * const igp = &gen->igp;

    for (s = igp->first; s; s = s->next) {
        const Gc_gms_hdr **p;
        for (p = s->store; p < s->ptr; ++p) {
            Gc_gms_hdr * const h = *p;
            pobject_lives(interp, GMSH_to_PObj(h));
        }
    }
    return 0;
}

/*

=item C<static int gc_gms_trace_root>

Trace the root set. If C<trace_stack> is true, trace system areas.

=cut

*/

static int
gc_gms_trace_root(PARROT_INTERP, int trace_stack)
{
    ASSERT_ARGS(gc_gms_trace_root)
    const int ret = Parrot_gc_trace_root(interp, trace_stack);

    if (ret == 0)
        return 0;
    Parrot_forall_header_pools(interp, POOL_ALL, 0, trace_igp_cb);
    return ret;
}

/*

=item C<static int trace_children_cb>

RT #48260: Not yet documented!!!

=cut

*/

static int
trace_children_cb(PARROT_INTERP, ARGIN(Small_Object_Pool *pool), int flag, SHIM(void *arg))
{
    ASSERT_ARGS(trace_children_cb)
    Arenas * const arena_base = interp->arena_base;
    const int lazy_gc = arena_base->lazy_gc;
    Gc_gms_hdr *h;

    for (h = pool->gray; h != pool->white;) {
        PMC * const current = (PMC*)GMSH_to_PObj(h);
        UINTVAL bits;

        if (lazy_gc && arena_base->num_early_PMCs_seen >=
                arena_base->num_early_gc_PMCs) {
            return 1;
        }
        /* TODO propagate flag in pobject_lives */
        arena_base->gc_trace_ptr = current;
        if (!PObj_needs_early_gc_TEST(current))
            PObj_high_priority_DOD_CLEAR(current);

        /* mark children */
        if (PObj_custom_mark_TEST(current)) {
            VTABLE_mark(interp, current);
        }
        if (h != pool->gray) {
            /* if a gray was inserted DFS, it is next */
            h = pool->gray;
        }
        else {
            h = h->next;
            pool->gray = h;
        }
    }
    return 0;
}

/*

=item C<static int gc_gms_trace_children>

Traverse gray objects: mark and blacken. Returns 0 if the trace was aborted
lazily.

=cut

*/

static int
gc_gms_trace_children(PARROT_INTERP)
{
    ASSERT_ARGS(gc_gms_trace_children)
    return !Parrot_forall_header_pools(interp, POOL_PMC, 0,
            trace_children_cb);
}

/*

=item C<static int sweep_cb_pmc>

move everything from white up to the free_list to the free_list
scan for active destroy objects
TODO put these in front of the pool at pool->white_fin

=cut

*/

static int
sweep_cb_pmc(PARROT_INTERP, ARGIN(Small_Object_Pool *pool), int flag, SHIM(void *arg))
{
    ASSERT_ARGS(sweep_cb_pmc)
    Gc_gms_hdr *h;
    Arenas * const arena_base = interp->arena_base;

    /* TODO object stats */

    for (h = pool->white; h != pool->free_list; h = h->next) {
        PMC * const obj = (PMC*)GMSH_to_PObj(h);
        if (PObj_needs_early_gc_TEST(obj))
            --arena_base->num_early_gc_PMCs;
        if (PObj_active_destroy_TEST(obj))
            VTABLE_destroy(interp, (PMC *)obj);
        if (PObj_is_PMC_EXT_TEST(obj) && obj->pmc_ext) {
            /* if the PMC has a PMC_EXT structure,
             * return it to the pool
             */
            Small_Object_Pool * const ext_pool = arena_base->pmc_ext_pool;
            ext_pool->add_free_object(interp, ext_pool, obj->pmc_ext);
        }

    }
    pool->free_list = pool->white;
    return 0;
}

/*

=item C<static int sweep_cb_buf>

RT #48260: Not yet documented!!!

=cut

*/

static int
sweep_cb_buf(PARROT_INTERP, ARGMOD(Small_Object_Pool *pool), int flag, SHIM(void *arg))
{
    ASSERT_ARGS(sweep_cb_buf)
    Gc_gms_hdr *h;

    /* TODO object stats */

    for (h = pool->white; h != pool->free_list; h = h->next) {
        PObj * const obj = GMSH_to_PObj(h);
        /*
         * this is ugly, we still have to sweep all buffers
         */
        if (PObj_sysmem_TEST(obj) && PObj_bufstart(obj)) {
            /* has sysmem allocated, e.g. Parrot_str_pin */
            mem_sys_free(PObj_bufstart(obj));
            PObj_bufstart(obj) = NULL;
            PObj_buflen(obj) = 0;
        }
        else {
#  ifdef GC_IS_MALLOC
            /* free allocated space at (int*)bufstart - 1,
             * but not if it is used COW or external
             */
            if (PObj_bufstart(obj) &&
                    !PObj_is_external_or_free_TESTALL(obj)) {
                if (PObj_COW_TEST(obj)) {
                    INTVAL *refcount = PObj_bufrefcountptr(obj);

                    if (!--(*refcount))
                        free(refcount); /* the actual bufstart */
                }
                else
                    free(PObj_bufrefcountptr(obj));
            }
#  else
            /*
             * XXX Jarkko did report that on irix pool->mem_pool
             *     was NULL, which really shouldn't happen
             */
            if (pool->mem_pool) {
                if (!PObj_COW_TEST(obj)) {
                    ((Memory_Pool *)
                     pool->mem_pool)->guaranteed_reclaimable +=
                        PObj_buflen(obj);
                }
                ((Memory_Pool *)
                 pool->mem_pool)->possibly_reclaimable +=
                    PObj_buflen(obj);
            }
#  endif
            PObj_buflen(obj) = 0;
        }
    }
    pool->free_list = pool->white;
    return 0;
}

/*

=item C<static void gc_gms_sweep>

Free unused resources, put white objects onto free_list.

=cut

*/

static void
gc_gms_sweep(PARROT_INTERP)
{
    ASSERT_ARGS(gc_gms_sweep)
    Parrot_forall_header_pools(interp, POOL_PMC, 0, sweep_cb_pmc);
    Parrot_forall_header_pools(interp, POOL_BUFFER, 0, sweep_cb_buf);
}

/*

=item C<static int end_cycle_cb>

RT #48260: Not yet documented!!!

=cut

*/

static int
end_cycle_cb(PARROT_INTERP, ARGMOD(Small_Object_Pool *pool), int flag, SHIM(void *arg))
{
    ASSERT_ARGS(end_cycle_cb)
    Gc_gms_hdr *h;
    /*
     * clear live flags
     * TODO just swap black and white
     */
    if (!pool->black || pool->black == &pool->marker)
        return 0;
    for (h = pool->black; h != pool->white; h = h->next)
        PObj_live_CLEAR(GMSH_to_PObj(h));
    pool->black = pool->black_fin = pool->gray = pool->white;
    return 0;
}

/*

=item C<static void gc_gms_end_cycle>

RT #48260: Not yet documented!!!

=cut

*/

static void
gc_gms_end_cycle(PARROT_INTERP)
{
    ASSERT_ARGS(gc_gms_end_cycle)
    Parrot_forall_header_pools(interp, POOL_ALL, 0, end_cycle_cb);
}

/*

=back

=head2 Interface function main entry

=over 4

=item C<static void parrot_gc_gms_run>

Interface to C<Parrot_do_gc_run>. C<flags> is one of:

  GC_lazy_FLAG   ... timely destruction
  GC_finish_FLAG ... run a final sweep to destruct objects at
                      interpreter shutdown

=cut

*/

static void
parrot_gc_gms_run(PARROT_INTERP, UINTVAL flags)
{
    ASSERT_ARGS(parrot_gc_gms_run)
    Arenas * const arena_base = interp->arena_base;
    Gc_gms_private *g_gms;

    if (arena_base->DOD_block_level) {
        return;
    }
    ++arena_base->DOD_block_level;
    g_gms = arena_base->gc_private;
    if (flags & GC_finish_FLAG) {
        Small_Object_Pool * const pool = arena_base->pmc_pool;

        pool->white = pool->marker.next;
        /* XXX need to sweep over objects that have finalizers only */
        Parrot_forall_header_pools(interp, POOL_PMC, 0, sweep_cb_pmc);
        gc_gms_end_cycle(interp);
        --arena_base->DOD_block_level;
        return;
    }

    /* normal or lazy DOD run */
    arena_base->gc_runs++;
    arena_base->lazy_gc = (flags & GC_lazy_FLAG);
    gc_gms_init_mark(interp);
    if (gc_gms_trace_root(interp, !arena_base->lazy_gc) &&
            gc_gms_trace_children(interp)) {
        gc_gms_sweep(interp);
        gc_gms_set_gen(interp);
    }
    else {
        /*
         * successful lazy DOD run
         */
        ++arena_base->lazy_gc_runs;
    }
    gc_gms_end_cycle(interp);
    --arena_base->DOD_block_level;
}

/*

=item C<static void gms_debug_verify>

RT #48260: Not yet documented!!!

=cut

*/

#  if GC_GMS_DEBUG
static void
gms_debug_verify(PARROT_INTERP, ARGMOD(Small_Object_Pool *pool), ARGIN(const char *action))
{
    ASSERT_ARGS(gms_debug_verify)
    Gc_gms_hdr *h;
    int bf, gf, wf, ff;
    size_t i;

    const size_t n = pool->total_objects;

    bf = gf = wf = ff = 0;


    for (i = 0, h = &pool->marker; i <= n + 10; ++i) {
        if (i && h == &pool->marker)
            break;
        if (h == pool->black)
            bf++;
        if (h == pool->gray)
            gf++;
        if (h == pool->white)
            wf++;
        if (h == pool->free_list)
            ff++;
        h = h->next;
    }
    if (i != n + 1)
        fprintf(stderr, "gms_verify %s: chain corrupt %u objs %u total\n",
                action, i, n);
    if (bf != 1)
        fprintf(stderr, "gms_verify %s: found %u blacks\n", action, bf);
    if (gf != 1)
        fprintf(stderr, "gms_verify %s: found %u grays\n", action, gf);
    if (wf != 1)
        fprintf(stderr, "gms_verify %s: found %u whites\n", action, wf);
    if (ff != 1)
        fprintf(stderr, "gms_verify %s: found %u frees\n", action, ff);
}


#  endif  /* GC_GMS_DEBUG */

#endif  /* PARROT_GC_GMS */

/*

=back

=head1 SEE ALSO

F<src/gc/api.c>, F<include/parrot/gc_api.h>, F<include/parrot/pobj.h>,
F<src/gc/incremental_ms.c>

=head1 HISTORY

Initial version by leo (2005.01.12 - 2005.01.30)

=cut

*/


/*
 * Local variables:
 *   c-file-style: "parrot"
 * End:
 * vim: expandtab shiftwidth=4:
 */
