/* resources.c
 *  Copyright: 2001-2003 The Perl Foundation.  All Rights Reserved.
 *  CVS Info
 *     $Id$
 *  Overview:
 *     Handles the accessing of small object pools (header pools)
 *  Data Structure and Algorithms:
 *
 *  History:
 *     Initial version by Mike Lambert on 2002.05.27
 *  Notes:
 *  References:
 */

#include "parrot/parrot.h"
#include <assert.h>

#define GC_DEBUG_REPLENISH_LEVEL_FACTOR 0.0
#define GC_DEBUG_UNITS_PER_ALLOC_GROWTH_FACTOR 1
#define REPLENISH_LEVEL_FACTOR 0.3
/* this factor is totally arbitrary, but gives good timings for stress.pasm */
#define UNITS_PER_ALLOC_GROWTH_FACTOR 1.75

#define POOL_MAX_BYTES 65536*128

INTVAL
contained_in_pool(struct Parrot_Interp *interpreter,
        struct Small_Object_Pool *pool, void *ptr)
{
    struct Small_Object_Arena *arena;

    for (arena = pool->last_Arena; arena; arena = arena->prev) {
        ptrdiff_t ptr_diff = (ptrdiff_t)ptr - (ptrdiff_t)arena->start_objects;

        if (0 <= ptr_diff
                && ptr_diff <
                (ptrdiff_t)(arena->used * pool->object_size)
                && ptr_diff % pool->object_size == 0)
            return 1;
    }
    return 0;
}

int
Parrot_is_const_pmc(Parrot_Interp interpreter, PMC *pmc)
{
    struct Small_Object_Pool *pool
        = interpreter->arena_base->constant_pmc_pool;
    int c;
#if  ARENA_DOD_FLAGS
    c = GET_ARENA(pmc)->pool == pool;
#else
    c = contained_in_pool(interpreter, pool, pmc);
#endif

    /* some paranoia first */
    assert(!!PObj_constant_TEST(pmc) == !!c);
    return c;
}

static int find_free_list(struct Small_Object_Pool *pool);

/* We're out of traceable objects. Try a DOD, then get some more if needed */
void
more_traceable_objects(struct Parrot_Interp *interpreter,
        struct Small_Object_Pool *pool)
{
    if (pool->skip)
        pool->skip = 0;
    else {
        Parrot_do_dod_run(interpreter, 1);
        if (pool->num_free_objects <= pool->replenish_level)
            pool->skip = 1;
    }

    /* requires that num_free_objects be updated in Parrot_do_dod_run. If dod
     * is disabled, then we must check the free list directly. */
#if  ARENA_DOD_FLAGS
    if (!find_free_list(pool)) {
        (*pool->alloc_objects) (interpreter, pool);
    }
#else
    if (!pool->free_list) {
        (*pool->alloc_objects) (interpreter, pool);
    }
#endif
}

/* We're out of non-traceable objects. Get some more */
void
more_non_traceable_objects(struct Parrot_Interp *interpreter,
        struct Small_Object_Pool *pool)
{
    (*pool->alloc_objects) (interpreter, pool);
}

#if  ARENA_DOD_FLAGS
static int
find_free_list(struct Small_Object_Pool *pool)
{
    struct Small_Object_Arena *arena;

    for (arena = pool->last_Arena; arena; arena = arena->prev) {
        if (arena->free_list) {
            pool->free_arena = arena;
            return 1;
        }
    }
    return 0;
}

void
add_free_object(struct Parrot_Interp *interpreter,
        struct Small_Object_Arena *arena, void *to_add)
{
    *(void **)to_add = arena->free_list;
    arena->free_list = to_add;
}

/* Get a new object from the free pool and return it */
void *
get_free_object(struct Parrot_Interp *interpreter,
        struct Small_Object_Pool *pool)
{
    void *ptr;
    struct Small_Object_Arena *arena;
    size_t n;

    /* if we don't have any objects */
    if (!pool->free_arena)
        (*pool->alloc_objects) (interpreter, pool);
    if (!pool->free_arena->free_list)
        if (!find_free_list(pool))
            (*pool->more_objects) (interpreter, pool);

    arena = pool->free_arena;
    ptr = arena->free_list;
    arena->free_list = *(void **)ptr;
    n = GET_OBJ_N(arena, ptr);
    arena->dod_flags[ n >> ARENA_FLAG_SHIFT ] &=
         ~((PObj_on_free_list_FLAG << (( n & ARENA_FLAG_MASK ) << 2)));
#if ! DISABLE_GC_DEBUG
    if (GC_DEBUG(interpreter))
        ((Buffer*)ptr)->version++;
#endif
    return ptr;
}

#else
/* Add an unused object back to the free pool for later reuse */
void
add_free_object(struct Parrot_Interp *interpreter,
        struct Small_Object_Pool *pool, void *to_add)
{
    PObj_flags_SETTO((PObj *)to_add, PObj_on_free_list_FLAG);
    *(void **)to_add = pool->free_list;
    pool->free_list = to_add;
}

/* Get a new object from the free pool and return it */
void *
get_free_object(struct Parrot_Interp *interpreter,
        struct Small_Object_Pool *pool)
{
    void *ptr;

    /* if we don't have any objects */
    if (!pool->free_list)
        (*pool->more_objects) (interpreter, pool);

    ptr = pool->free_list;
    pool->free_list = *(void **)ptr;
#if ! DISABLE_GC_DEBUG
    if (GC_DEBUG(interpreter))
        ((Buffer*)ptr)->version++;
#endif
    return ptr;
}
#endif


static void
add_to_free_list(struct Parrot_Interp *interpreter,
        struct Small_Object_Pool *pool,
        struct Small_Object_Arena *arena,
        UINTVAL start,
        UINTVAL end)
{
#if ARENA_DOD_FLAGS
    UINTVAL *dod_flags;
#endif
    UINTVAL i;
    void *object;

    pool->total_objects += end - start;
    arena->used = end;

    /* Move all the new objects into the free list */
    object = (void *)((char *)arena->start_objects +
            start * pool->object_size);
#if ARENA_DOD_FLAGS
    pool->replenish_level =
                (size_t)(pool->total_objects * REPLENISH_LEVEL_FACTOR);
    dod_flags = arena->dod_flags + (start >> ARENA_FLAG_SHIFT);
#endif
    for (i = start; i < end; i++) {
#if ARENA_DOD_FLAGS
        if (! (i & ARENA_FLAG_MASK)) {
            *dod_flags = ALL_FREE_MASK;
            ++dod_flags;
        }
        pool->add_free_object (interpreter, arena, object);
#else
        pool->add_free_object (interpreter, pool, object);
#endif
        object = (void *)((char *)object + pool->object_size);
    }
#if ARENA_DOD_FLAGS
    /* set last */
    *dod_flags = ALL_FREE_MASK;
#endif
}

/* We have no more headers on the free header pool. Go allocate more
 * and put them on */
void
alloc_objects(struct Parrot_Interp *interpreter,
        struct Small_Object_Pool *pool)
{
    struct Small_Object_Arena *new_arena;
    size_t size;
    UINTVAL start, end;

    /* Setup memory for the new objects */
#if ARENA_DOD_FLAGS
    size_t offset;

    /* check old arena first */
    if (pool->last_Arena &&
            pool->last_Arena->used < pool->last_Arena->total_objects) {
        start = pool->last_Arena->used;
        end = start << 2;
        if (end > pool->last_Arena->total_objects)
            end = pool->last_Arena->total_objects;
        add_to_free_list(interpreter, pool, pool->last_Arena, start, end);
        return;
    }

    size = ARENA_SIZE;
    /*
     * [ Note: Linux ]
     * Albeit we reserve 8 pools with 4MB each, the memory footprint
     * of a running program is much smaller, if only a few objects per pool
     * are used. The unused pages of the arenas are only mapped (they use space
     * in the page tables, but no physical memory).
     * Because of the COW pages strategy of the kernel, there shouldn't
     * be a problem with many running instances too at least if these get
     * fork'ed or were created as threads.
     * For many unique parrot processes, you might either reduce ARENA_SIZE
     * or turn off ARENA_DOD_FLAGS.
     */
    new_arena = Parrot_memalign(ARENA_ALIGN, size);
    /* offset in bytes of whole Objects */
    offset = ( 1 + sizeof(struct Small_Object_Arena) / pool->object_size) *
        pool->object_size;
    new_arena->start_objects = (char *)new_arena + offset;
    size -= offset;
    pool->objects_per_alloc = size / pool->object_size;
    new_arena->object_size = pool->object_size;
#ifdef SIMULATE_DIRTY_AND_RUN_SLOW
    /* memory gets washed in get_free_*, so we don't need zeroed
     * memory here
     */
    memset(new_arena->start_objects, 0xff, size); /* simulate dirty */
#endif
    new_arena->dod_flags = mem_sys_allocate(ARENA_FLAG_SIZE(pool));
    new_arena->pool = pool;
    new_arena->free_list = NULL;
#else
    new_arena = mem_sys_allocate(sizeof(struct Small_Object_Arena));
    size = pool->object_size * pool->objects_per_alloc;
    /* could be mem_sys_allocate too, but calloc is fast */
    new_arena->start_objects = mem_sys_allocate_zeroed(size);
#endif

    /* Maintain the *_arena_memory invariant for stack walking code. Set it
     * regardless if we're the first pool to be added. */
    if (!pool->last_Arena
            || (pool->start_arena_memory > (size_t)new_arena->start_objects))
        pool->start_arena_memory = (size_t)new_arena->start_objects;

    if (!pool->last_Arena || (pool->end_arena_memory <
                    (size_t)new_arena->start_objects + size))
        pool->end_arena_memory = (size_t)new_arena->start_objects + size;

    /* Hook up the new object block into the object pool */
#if ARENA_DOD_FLAGS
    /* not the first one - put all on free list */
    if (pool->last_Arena) {
        start = 0;
        end = pool->objects_per_alloc;
    }
    else {
        /* first arena, start with x objects */
        start = 0;
        end = 1024;
    }
    pool->free_arena = new_arena;
#else
    start = 0;
    end = pool->objects_per_alloc;
#endif
    new_arena->total_objects = pool->objects_per_alloc;
    new_arena->next = NULL;
    new_arena->prev = pool->last_Arena;
    if (new_arena->prev) {
        new_arena->prev->next = new_arena;
    }
    pool->last_Arena = new_arena;
    interpreter->header_allocs_since_last_collect++;

    add_to_free_list(interpreter, pool, new_arena, start, end);

#if ! ARENA_DOD_FLAGS
    /* Allocate more next time */
    if (GC_DEBUG(interpreter)) {
        pool->objects_per_alloc *= GC_DEBUG_UNITS_PER_ALLOC_GROWTH_FACTOR;
        pool->replenish_level =
                (size_t)(pool->total_objects *
                GC_DEBUG_REPLENISH_LEVEL_FACTOR);
    }
    else {
        pool->objects_per_alloc *= UNITS_PER_ALLOC_GROWTH_FACTOR;
        pool->replenish_level =
                (size_t)(pool->total_objects * REPLENISH_LEVEL_FACTOR);
    }
    /* check alloc size against maximum */
    size = pool->object_size * pool->objects_per_alloc;

    if (size > POOL_MAX_BYTES) {
        pool->objects_per_alloc = POOL_MAX_BYTES / pool->object_size;
    }
#endif
}

struct Small_Object_Pool *
new_small_object_pool(struct Parrot_Interp *interpreter,
        size_t object_size, size_t objects_per_alloc)
{
    struct Small_Object_Pool *pool;

    pool = mem_sys_allocate_zeroed(sizeof(struct Small_Object_Pool));
    SET_NULL(pool->last_Arena);
    SET_NULL(pool->free_list);
    SET_NULL(pool->mem_pool);
    pool->object_size = object_size;
    pool->objects_per_alloc = objects_per_alloc;
    pool->add_free_object = add_free_object;
    pool->get_free_object = get_free_object;
    pool->alloc_objects = alloc_objects;
    return pool;
}

/*
 * Local variables:
 * c-indentation-style: bsd
 * c-basic-offset: 4
 * indent-tabs-mode: nil
 * End:
 *
 * vim: expandtab shiftwidth=4:
*/
