/*
Copyright (C) 2001-2009, Parrot Foundation.
$Id$

=head1 NAME

src/gc/mark_sweep.c - Small Object Pools and general mark/sweep GC behavior

=head1 DESCRIPTION

This file implements a number of routines related to the mark and sweep phases
of the garbage collector system. The functions in this file attempt to perform
various useful utilities without interfering with the operations of the
particular GC core. Individual GC cores may choose to use these functions or
to reimplement their functionality, hopefully without causing any problems
throughout the rest of Parrot.

=head2 Functions

=over 4

=cut

*/

#include "parrot/parrot.h"
#include "gc_private.h"

/* HEADERIZER HFILE: src/gc/gc_private.h */

/* HEADERIZER BEGIN: static */
/* Don't modify between HEADERIZER BEGIN / HEADERIZER END.  Your changes will be lost. */

static void fix_pmc_syncs(
    ARGMOD(Interp *dest_interp),
    ARGIN(Fixed_Size_Pool *pool))
        __attribute__nonnull__(1)
        __attribute__nonnull__(2)
        FUNC_MODIFIES(*dest_interp);

static void free_buffer(SHIM_INTERP,
    ARGIN(Memory_Pools *mem_pools),
    ARGMOD(Fixed_Size_Pool *pool),
    ARGMOD(Buffer *b))
        __attribute__nonnull__(2)
        __attribute__nonnull__(3)
        __attribute__nonnull__(4)
        FUNC_MODIFIES(*pool)
        FUNC_MODIFIES(*b);

static void free_buffer_malloc(SHIM_INTERP,
    SHIM(Fixed_Size_Pool *pool),
    ARGMOD(Buffer *b))
        __attribute__nonnull__(3)
        FUNC_MODIFIES(*b);

static void free_pmc_in_pool(PARROT_INTERP,
    ARGIN(Memory_Pools *mem_pools),
    SHIM(Fixed_Size_Pool *pool),
    ARGMOD(PObj *p))
        __attribute__nonnull__(1)
        __attribute__nonnull__(2)
        __attribute__nonnull__(4)
        FUNC_MODIFIES(*p);

static void free_pool(ARGMOD(Fixed_Size_Pool *pool))
        __attribute__nonnull__(1)
        FUNC_MODIFIES(*pool);

PARROT_WARN_UNUSED_RESULT
PARROT_CANNOT_RETURN_NULL
static Fixed_Size_Pool * new_bufferlike_pool(PARROT_INTERP,
    ARGIN(Memory_Pools *mem_pools),
    size_t actual_buffer_size)
        __attribute__nonnull__(1)
        __attribute__nonnull__(2);

PARROT_MALLOC
PARROT_CANNOT_RETURN_NULL
static Fixed_Size_Pool * new_fixed_size_obj_pool(
    size_t object_size,
    size_t objects_per_alloc);

PARROT_WARN_UNUSED_RESULT
PARROT_CANNOT_RETURN_NULL
static Fixed_Size_Pool * new_pmc_pool(PARROT_INTERP,
    ARGIN(Memory_Pools *mem_pools))
        __attribute__nonnull__(1)
        __attribute__nonnull__(2);

PARROT_WARN_UNUSED_RESULT
PARROT_CANNOT_RETURN_NULL
static Fixed_Size_Pool * new_string_pool(PARROT_INTERP,
    ARGIN(Memory_Pools *mem_pools),
    INTVAL constant)
        __attribute__nonnull__(1)
        __attribute__nonnull__(2);

static void Parrot_gc_allocate_new_attributes_arena(PARROT_INTERP,
    ARGMOD(PMC_Attribute_Pool *pool))
        __attribute__nonnull__(1)
        __attribute__nonnull__(2)
        FUNC_MODIFIES(*pool);

PARROT_CANNOT_RETURN_NULL
static PMC_Attribute_Pool * Parrot_gc_create_attrib_pool(PARROT_INTERP,
    size_t attrib_idx)
        __attribute__nonnull__(1);

static void Parrot_gc_merge_buffer_pools(PARROT_INTERP,
    ARGMOD(Fixed_Size_Pool *dest),
    ARGMOD(Fixed_Size_Pool *source))
        __attribute__nonnull__(1)
        __attribute__nonnull__(2)
        __attribute__nonnull__(3)
        FUNC_MODIFIES(*dest)
        FUNC_MODIFIES(*source);

static int sweep_cb_buf(PARROT_INTERP,
    ARGIN(Memory_Pools *mem_pools),
    ARGMOD(Fixed_Size_Pool *pool),
    SHIM(int flag),
    ARGIN(void *arg))
        __attribute__nonnull__(1)
        __attribute__nonnull__(2)
        __attribute__nonnull__(3)
        __attribute__nonnull__(5)
        FUNC_MODIFIES(*pool);

static int sweep_cb_pmc(PARROT_INTERP,
    ARGIN(Memory_Pools *mem_pools),
    ARGMOD(Fixed_Size_Pool *pool),
    SHIM(int flag),
    SHIM(void *arg))
        __attribute__nonnull__(1)
        __attribute__nonnull__(2)
        __attribute__nonnull__(3)
        FUNC_MODIFIES(*pool);

#define ASSERT_ARGS_fix_pmc_syncs __attribute__unused__ int _ASSERT_ARGS_CHECK = (\
       PARROT_ASSERT_ARG(dest_interp) \
    , PARROT_ASSERT_ARG(pool))
#define ASSERT_ARGS_free_buffer __attribute__unused__ int _ASSERT_ARGS_CHECK = (\
       PARROT_ASSERT_ARG(mem_pools) \
    , PARROT_ASSERT_ARG(pool) \
    , PARROT_ASSERT_ARG(b))
#define ASSERT_ARGS_free_buffer_malloc __attribute__unused__ int _ASSERT_ARGS_CHECK = (\
       PARROT_ASSERT_ARG(b))
#define ASSERT_ARGS_free_pmc_in_pool __attribute__unused__ int _ASSERT_ARGS_CHECK = (\
       PARROT_ASSERT_ARG(interp) \
    , PARROT_ASSERT_ARG(mem_pools) \
    , PARROT_ASSERT_ARG(p))
#define ASSERT_ARGS_free_pool __attribute__unused__ int _ASSERT_ARGS_CHECK = (\
       PARROT_ASSERT_ARG(pool))
#define ASSERT_ARGS_new_bufferlike_pool __attribute__unused__ int _ASSERT_ARGS_CHECK = (\
       PARROT_ASSERT_ARG(interp) \
    , PARROT_ASSERT_ARG(mem_pools))
#define ASSERT_ARGS_new_fixed_size_obj_pool __attribute__unused__ int _ASSERT_ARGS_CHECK = (0)
#define ASSERT_ARGS_new_pmc_pool __attribute__unused__ int _ASSERT_ARGS_CHECK = (\
       PARROT_ASSERT_ARG(interp) \
    , PARROT_ASSERT_ARG(mem_pools))
#define ASSERT_ARGS_new_string_pool __attribute__unused__ int _ASSERT_ARGS_CHECK = (\
       PARROT_ASSERT_ARG(interp) \
    , PARROT_ASSERT_ARG(mem_pools))
#define ASSERT_ARGS_Parrot_gc_allocate_new_attributes_arena \
     __attribute__unused__ int _ASSERT_ARGS_CHECK = (\
       PARROT_ASSERT_ARG(interp) \
    , PARROT_ASSERT_ARG(pool))
#define ASSERT_ARGS_Parrot_gc_create_attrib_pool __attribute__unused__ int _ASSERT_ARGS_CHECK = (\
       PARROT_ASSERT_ARG(interp))
#define ASSERT_ARGS_Parrot_gc_merge_buffer_pools __attribute__unused__ int _ASSERT_ARGS_CHECK = (\
       PARROT_ASSERT_ARG(interp) \
    , PARROT_ASSERT_ARG(dest) \
    , PARROT_ASSERT_ARG(source))
#define ASSERT_ARGS_sweep_cb_buf __attribute__unused__ int _ASSERT_ARGS_CHECK = (\
       PARROT_ASSERT_ARG(interp) \
    , PARROT_ASSERT_ARG(mem_pools) \
    , PARROT_ASSERT_ARG(pool) \
    , PARROT_ASSERT_ARG(arg))
#define ASSERT_ARGS_sweep_cb_pmc __attribute__unused__ int _ASSERT_ARGS_CHECK = (\
       PARROT_ASSERT_ARG(interp) \
    , PARROT_ASSERT_ARG(mem_pools) \
    , PARROT_ASSERT_ARG(pool))
/* Don't modify between HEADERIZER BEGIN / HEADERIZER END.  Your changes will be lost. */
/* HEADERIZER END: static */

/* Set when walking the system stack */
int CONSERVATIVE_POINTER_CHASING = 0;

/*

=item C<void Parrot_gc_run_init(PARROT_INTERP, Memory_Pools *mem_pools)>

Prepares the collector for a mark & sweep GC run. Initializes the various
fields in the Memory_Pools structure that need to be zeroed out prior to the
mark phase.

=cut

*/

void
Parrot_gc_run_init(PARROT_INTERP, ARGIN(Memory_Pools *mem_pools))
{
    ASSERT_ARGS(Parrot_gc_run_init)

    mem_pools->gc_trace_ptr        = NULL;
    mem_pools->gc_mark_start       = NULL;
    mem_pools->num_early_PMCs_seen = 0;
}

/*

=item C<int Parrot_gc_trace_root(PARROT_INTERP, Memory_Pools *mem_pools,
Parrot_gc_trace_type trace)>

Traces the root set with behavior that's dependent on the flags passed.
Returns 0 if it's a lazy GC run and all objects that need timely destruction
were found. Returns 1 otherwise.

The flag C<trace> can have these values:

=over 4

=item * GC_TRACE_ROOT_ONLY

trace normal roots

=item * GC_TRACE_SYSTEM_ONLY

trace system areas only

=item * GC_TRACE_FULL

trace whole root set, including system areas.

=back

=cut

*/

int
Parrot_gc_trace_root(PARROT_INTERP, ARGIN(Memory_Pools *mem_pools), Parrot_gc_trace_type trace)
{
    ASSERT_ARGS(Parrot_gc_trace_root)
    PObj    *obj;

    /* note: adding locals here did cause increased GC runs */
    mark_context_start();

    if (trace == GC_TRACE_SYSTEM_ONLY) {
        trace_system_areas(interp, mem_pools);
        return 0;
    }

    /* We have to start somewhere; the interpreter globals is a good place */
    if (!mem_pools->gc_mark_start) {
        mem_pools->gc_mark_start
            = mem_pools->gc_mark_ptr
            = interp->iglobals;
    }

    /* mark the list of iglobals */
    Parrot_gc_mark_PMC_alive(interp, interp->iglobals);

    /* mark the current continuation */
    obj = (PObj *)interp->current_cont;
    if (obj && obj != (PObj *)NEED_CONTINUATION)
        Parrot_gc_mark_PMC_alive(interp, (PMC *)obj);

    /* mark the current context. */
    Parrot_gc_mark_PMC_alive(interp, CURRENT_CONTEXT(interp));

    /* mark the dynamic environment. */
    Parrot_gc_mark_PMC_alive(interp, interp->dynamic_env);

    /* mark the vtables: the data, Class PMCs, etc. */
    mark_vtables(interp);

    /* mark the root_namespace */
    Parrot_gc_mark_PMC_alive(interp, interp->root_namespace);

    /* mark the concurrency scheduler */
    Parrot_gc_mark_PMC_alive(interp, interp->scheduler);

    /* s. packfile.c */
    mark_const_subs(interp);

    /* mark caches and freelists */
    mark_object_cache(interp);

    /* Now mark the class hash */
    Parrot_gc_mark_PMC_alive(interp, interp->class_hash);

    /* Now mark the HLL stuff */
    Parrot_gc_mark_PMC_alive(interp, interp->HLL_info);
    Parrot_gc_mark_PMC_alive(interp, interp->HLL_namespace);

    /* Mark the registry */
    PARROT_ASSERT(interp->gc_registry);
    Parrot_gc_mark_PMC_alive(interp, interp->gc_registry);

    /* Mark the MMD cache. */
    if (interp->op_mmd_cache)
        Parrot_mmd_cache_mark(interp, interp->op_mmd_cache);

    /* Walk the iodata */
    Parrot_IOData_mark(interp, interp->piodata);

    if (trace == GC_TRACE_FULL)
        trace_system_areas(interp, mem_pools);

    /* quick check to see if we have already marked all impatient PMCs. If we
       have, return 0 and exit here. This will alert other parts of the GC
       that if we are in a lazy run we can just stop it. */
    if (mem_pools->lazy_gc
    &&  mem_pools->num_early_PMCs_seen >= mem_pools->num_early_gc_PMCs)
        return 0;

    return 1;
}


/*

=item C<void Parrot_gc_sweep_pool(PARROT_INTERP, Memory_Pools *mem_pools,
Fixed_Size_Pool *pool)>

Puts any buffers/PMCs that are marked as "dead" or "black" onto the pool
free list.

=cut

*/

void
Parrot_gc_sweep_pool(PARROT_INTERP, ARGIN(Memory_Pools *mem_pools), ARGMOD(Fixed_Size_Pool *pool))
{
    ASSERT_ARGS(Parrot_gc_sweep_pool)

    PObj               *b;
    Fixed_Size_Arena   *cur_arena;
    gc_object_fn_type   gc_object   = pool->gc_object;
    UINTVAL             total_used  = 0;
    const UINTVAL       object_size = pool->object_size;
    UINTVAL             i;


    /* Run through all the PObj header pools and mark */
    for (cur_arena = pool->last_Arena; cur_arena; cur_arena = cur_arena->prev) {
        const size_t objects_end = cur_arena->used;
        UINTVAL      i;
        b = (PObj *)cur_arena->start_objects;

        /* loop only while there are objects in the arena */
        for (i = objects_end; i; i--) {

            /* if it's on free list, do nothing */

            if (PObj_live_TEST(b)) {
                total_used++;
                PObj_live_CLEAR(b);
                PObj_get_FLAGS(b) &= ~PObj_custom_GC_FLAG;
            }
            else if (!PObj_on_free_list_TEST(b)) {
                /* it must be dead */


                if (PObj_is_shared_TEST(b)) {
                    /* only mess with shared objects if we
                     * (and thus everyone) is suspended for
                     * a GC run.
                     * XXX wrong thing to do with "other" GCs
                     */
                    if (!(interp->thread_data
                    &&   (interp->thread_data->state & THREAD_STATE_SUSPENDED_GC))) {
                        ++total_used;
                        goto next;
                    }
                }

                if (gc_object)
                    gc_object(interp, mem_pools, pool, b);

                pool->add_free_object(interp, mem_pools, pool, b);
            }
next:
            b = (PObj *)((char *)b + object_size);
        }
    }

    pool->num_free_objects = pool->total_objects - total_used;
}


/*

=item C<INTVAL contained_in_pool(PARROT_INTERP, const Fixed_Size_Pool *pool,
const void *ptr)>

Returns whether the given C<*ptr> points to a location in C<pool>.

=cut

*/

PARROT_WARN_UNUSED_RESULT
INTVAL
contained_in_pool(PARROT_INTERP, ARGIN(const Fixed_Size_Pool *pool), ARGIN(const void *ptr))
{
    ASSERT_ARGS(contained_in_pool)
    const Fixed_Size_Arena *arena;

    for (arena = pool->last_Arena; arena; arena = arena->prev) {
        const ptrdiff_t ptr_diff =
            (ptrdiff_t)ptr - (ptrdiff_t)arena->start_objects;

        if (0 <= ptr_diff
              && ptr_diff < (ptrdiff_t)(arena->used * pool->object_size)
              && ptr_diff % pool->object_size == 0)
            return 1;
    }

    return 0;
}


/*

=item C<void mark_special(PARROT_INTERP, PMC *obj)>

Handles marking a PMC. Specifically, calls the VTABLE_mark for that PMC
if one is present. Also handles marking shared PMCs.

=cut

*/

void
mark_special(PARROT_INTERP, ARGIN(PMC *obj))
{
    ASSERT_ARGS(mark_special)

    /*
     * If the object is shared, we have to use the arena and gc
     * pointers of the originating interpreter.
     *
     * We are possibly changing another interpreter's data here, so
     * the mark phase of GC must run only on one interpreter of a pool
     * at a time. However, freeing unused objects can occur in parallel.
     * And: to be sure that a shared object is dead, we have to finish
     * the mark phase of all interpreters in a pool that might reference
     * the object.
     */
    if (PObj_is_PMC_shared_TEST(obj)) {
        interp = PMC_sync(obj)->owner;
        PARROT_ASSERT(interp);

        // XXX
        //if (!interp->mem_pools->gc_mark_ptr)
        //    interp->mem_pools->gc_mark_ptr = obj;
    }

    PObj_get_FLAGS(obj) |= PObj_custom_GC_FLAG;

    /* clearing the flag is much more expensive then testing */
    if (!PObj_needs_early_gc_TEST(obj))
        PObj_high_priority_gc_CLEAR(obj);

    /* mark properties */
    Parrot_gc_mark_PMC_alive(interp, PMC_metadata(obj));

    if (PObj_custom_mark_TEST(obj)) {
        PARROT_ASSERT(!PObj_on_free_list_TEST(obj));
        VTABLE_mark(interp, obj);
    }
}


/*

=item C<void Parrot_gc_clear_live_bits(PARROT_INTERP, const Fixed_Size_Pool
*pool)>

Resets the PMC pool, so all objects are marked as "White". This
is done after a GC run to reset the system and prepare for the
next mark phase.

=cut

*/

void
Parrot_gc_clear_live_bits(PARROT_INTERP, ARGIN(const Fixed_Size_Pool *pool))
{
    ASSERT_ARGS(Parrot_gc_clear_live_bits)
    Fixed_Size_Arena *arena;
    const UINTVAL object_size = pool->object_size;

    for (arena = pool->last_Arena; arena; arena = arena->prev) {
        Buffer *b = (Buffer *)arena->start_objects;
        UINTVAL i;

        for (i = 0; i < arena->used; i++) {
            PObj_live_CLEAR(b);
            b = (Buffer *)((char *)b + object_size);
        }
    }
}


/*

=item C<void Parrot_add_to_free_list(PARROT_INTERP, Fixed_Size_Pool *pool,
Fixed_Size_Arena *arena)>

Adds the objects in the newly allocated C<arena> to the free list of the pool.

=cut

*/

void
Parrot_add_to_free_list(PARROT_INTERP,
        ARGMOD(Fixed_Size_Pool  *pool),
        ARGMOD(Fixed_Size_Arena *arena))
{
    ASSERT_ARGS(Parrot_add_to_free_list)
    UINTVAL  i;
    void    *object;
    const UINTVAL num_objects = pool->objects_per_alloc;

    pool->total_objects += num_objects;
    object = (void *)arena->start_objects;
#if GC_USE_LAZY_ALLOCATOR
    /* Don't move anything onto the free list. Set the pointers and do it
       lazily when we allocate. */
    {
        const size_t total_size = num_objects * pool->object_size;
        pool->newfree = arena->start_objects;
        pool->newlast = (void*)((char*)object + total_size);
        arena->used = 0;
    }
#else
    /* Move all the new objects into the free list */
    arena->used          = num_objects;
    for (i = 0; i < num_objects; i++) {
        pool->add_free_object(interp, pool, object);
        object = (void *)((char *)object + pool->object_size);
    }
#endif
    pool->num_free_objects += num_objects;
}


/*

=item C<void Parrot_append_arena_in_pool(PARROT_INTERP, Fixed_Size_Pool *pool,
Fixed_Size_Arena *new_arena, size_t size)>

Insert the new arena into the pool's structure. Arenas are stored in a
linked list, so add the new arena to the list. Set information in the
arenas structure, such as the number of objects allocated in it.

=cut

*/

void
Parrot_append_arena_in_pool(PARROT_INTERP,
    ARGMOD(Fixed_Size_Pool *pool),
    ARGMOD(Fixed_Size_Arena *new_arena), size_t size)
{
    ASSERT_ARGS(Parrot_append_arena_in_pool)

    /* Maintain the *_arena_memory invariant for stack walking code. Set it
     * regardless if we're the first pool to be added. */
    if (!pool->last_Arena
    || (pool->start_arena_memory > (size_t)new_arena->start_objects))
        pool->start_arena_memory = (size_t)new_arena->start_objects;

    if (!pool->last_Arena
    || (pool->end_arena_memory < (size_t)new_arena->start_objects + size))
        pool->end_arena_memory = (size_t)new_arena->start_objects + size;

    new_arena->total_objects = pool->objects_per_alloc;
    new_arena->next          = NULL;
    new_arena->prev          = pool->last_Arena;

    if (new_arena->prev)
        new_arena->prev->next = new_arena;

    pool->last_Arena = new_arena;
    // XXX mem_pools->header_allocs_since_last_collect++;
}

/*

=back

=head2 Header Pool Creation Functions

=over 4

=item C<static Fixed_Size_Pool * new_pmc_pool(PARROT_INTERP, Memory_Pools
*mem_pools)>

Creates and initializes a new pool for PMCs and returns it.

=cut

*/

PARROT_WARN_UNUSED_RESULT
PARROT_CANNOT_RETURN_NULL
static Fixed_Size_Pool *
new_pmc_pool(PARROT_INTERP, ARGIN(Memory_Pools *mem_pools))
{
    ASSERT_ARGS(new_pmc_pool)
    const int num_headers = PMC_HEADERS_PER_ALLOC;
    Fixed_Size_Pool * const pmc_pool =
        new_fixed_size_obj_pool(sizeof (PMC), num_headers);

    pmc_pool->mem_pool   = NULL;
    pmc_pool->gc_object  = free_pmc_in_pool;

    (mem_pools->init_pool)(interp, pmc_pool);
    return pmc_pool;
}

/*

=item C<static void free_pmc_in_pool(PARROT_INTERP, Memory_Pools *mem_pools,
Fixed_Size_Pool *pool, PObj *p)>

Frees a PMC that is no longer being used. Calls a custom C<destroy> VTABLE
method if one is available.

=cut

*/

static void
free_pmc_in_pool(PARROT_INTERP, ARGIN(Memory_Pools *mem_pools), SHIM(Fixed_Size_Pool *pool),
        ARGMOD(PObj *p))
{
    ASSERT_ARGS(free_pmc_in_pool)
    PMC    * const pmc        = (PMC *)p;

    /* TODO collect objects with finalizers */
    if (PObj_needs_early_gc_TEST(p)) {
        --mem_pools->num_early_gc_PMCs;
    }

    Parrot_pmc_destroy(interp, pmc);
}


/*

=item C<static Fixed_Size_Pool * new_bufferlike_pool(PARROT_INTERP, Memory_Pools
*mem_pools, size_t actual_buffer_size)>

Creates a new pool for buffer-like structures. This is called from
C<get_bufferlike_pool()>, and should probably not be called directly.

=cut

*/

PARROT_WARN_UNUSED_RESULT
PARROT_CANNOT_RETURN_NULL
static Fixed_Size_Pool *
new_bufferlike_pool(PARROT_INTERP, ARGIN(Memory_Pools *mem_pools), size_t actual_buffer_size)
{
    ASSERT_ARGS(new_bufferlike_pool)
    const int num_headers          = BUFFER_HEADERS_PER_ALLOC;
    const size_t buffer_size       =
            (actual_buffer_size + sizeof (void *) - 1) & ~(sizeof (void *) - 1);
    Fixed_Size_Pool * const pool =
            new_fixed_size_obj_pool(buffer_size, num_headers);

    pool->gc_object = (gc_object_fn_type)free_buffer;

    pool->mem_pool  = mem_pools->memory_pool;
    (mem_pools->init_pool)(interp, pool);
    return pool;
}

/*

=item C<static Fixed_Size_Pool * new_fixed_size_obj_pool(size_t object_size,
size_t objects_per_alloc)>

Creates a new C<Fixed_Size_Pool> and returns a pointer to it.
Initializes the pool structure based on the size of objects in the
pool and the number of items to allocate in each arena.

=cut

*/

PARROT_MALLOC
PARROT_CANNOT_RETURN_NULL
static Fixed_Size_Pool *
new_fixed_size_obj_pool(size_t object_size, size_t objects_per_alloc)
{
    ASSERT_ARGS(new_fixed_size_obj_pool)
    Fixed_Size_Pool * const pool =
        mem_internal_allocate_zeroed_typed(Fixed_Size_Pool);

    pool->last_Arena        = NULL;
    pool->free_list         = NULL;
    pool->mem_pool          = NULL;
    pool->object_size       = object_size;
    pool->objects_per_alloc = objects_per_alloc;
#if GC_USE_LAZY_ALLOCATOR
    pool->newfree           = NULL;
    pool->newlast           = NULL;
#endif

    return pool;
}

/*

=item C<static Fixed_Size_Pool * new_string_pool(PARROT_INTERP, Memory_Pools
*mem_pools, INTVAL constant)>

Creates a new pool for C<STRING>s and returns it. This calls
C<get_bufferlike_pool> internally, which in turn calls C<new_bufferlike_pool>.

=cut

*/

PARROT_WARN_UNUSED_RESULT
PARROT_CANNOT_RETURN_NULL
static Fixed_Size_Pool *
new_string_pool(PARROT_INTERP, ARGIN(Memory_Pools *mem_pools), INTVAL constant)
{
    ASSERT_ARGS(new_string_pool)
    Fixed_Size_Pool *pool;
    if (constant) {
        pool            = new_bufferlike_pool(interp, mem_pools, sizeof (STRING));
        pool->gc_object = NULL;
        pool->mem_pool  = mem_pools->constant_string_pool;
    }
    else
        pool = get_bufferlike_pool(interp, mem_pools, sizeof (STRING));

    pool->objects_per_alloc = STRING_HEADERS_PER_ALLOC;

    return pool;
}

/*

=item C<static void free_buffer_malloc(PARROT_INTERP, Fixed_Size_Pool *pool,
Buffer *b)>

Frees the given buffer, returning the storage space to the operating system
and removing it from Parrot's memory management system. If the buffer is COW,
The buffer is not freed if the reference count is greater then 1.

=cut

*/

static void
free_buffer_malloc(SHIM_INTERP, SHIM(Fixed_Size_Pool *pool),
        ARGMOD(Buffer *b))
{
    ASSERT_ARGS(free_buffer_malloc)
    /* free allocated space at (int *)bufstart - 1, but not if it used COW or is
     * external */
    Buffer_buflen(b) = 0;

    if (!Buffer_bufstart(b) || PObj_is_external_or_free_TESTALL(b))
        return;

    if (PObj_COW_TEST(b)) {
        INTVAL * const refcount = Buffer_bufrefcountptr(b);

        if (--(*refcount) == 0) {
            mem_sys_free(refcount); /* the actual bufstart */
        }
    }
    else
        mem_sys_free(Buffer_bufrefcountptr(b));
}

/*

=item C<static void free_buffer(PARROT_INTERP, Memory_Pools *mem_pools,
Fixed_Size_Pool *pool, Buffer *b)>

Frees a buffer, returning it to the memory pool for Parrot to possibly
reuse later.

=cut

*/

static void
free_buffer(SHIM_INTERP, ARGIN(Memory_Pools *mem_pools),
        ARGMOD(Fixed_Size_Pool *pool), ARGMOD(Buffer *b))
{
    ASSERT_ARGS(free_buffer)
    Variable_Size_Pool * const mem_pool = (Variable_Size_Pool *)pool->mem_pool;

    /* XXX Jarkko reported that on irix pool->mem_pool was NULL, which really
     * shouldn't happen */
    if (mem_pool) {
        if (!PObj_COW_TEST(b))
            mem_pool->guaranteed_reclaimable += Buffer_buflen(b);
        else
            mem_pool->possibly_reclaimable   += Buffer_buflen(b);
    }

    Buffer_buflen(b) = 0;
}


/*

=item C<Fixed_Size_Pool * get_bufferlike_pool(PARROT_INTERP, Memory_Pools
*mem_pools, size_t buffer_size)>

Makes and return a bufferlike header pool for objects of a given size. If a
pool for objects of that size already exists, no new pool will be created and
the pointer to the existing pool is returned.

=cut

*/

PARROT_WARN_UNUSED_RESULT
PARROT_CANNOT_RETURN_NULL
Fixed_Size_Pool *
get_bufferlike_pool(PARROT_INTERP, ARGIN(Memory_Pools *mem_pools), size_t buffer_size)
{
    ASSERT_ARGS(get_bufferlike_pool)
    Fixed_Size_Pool **sized_pools   = mem_pools->sized_header_pools;
    const UINTVAL       num_old     = mem_pools->num_sized;
    const UINTVAL       idx         = GET_SIZED_POOL_IDX(buffer_size);

    /* Expands the array of sized resource pools, if necessary */
    if (num_old <= idx) {
        const UINTVAL num_new = idx + 1;
        /* XXX: use mem_sys_realloc_zeroed to do this easier? If we want the
                same debugging behavior as mem_internal_realloc, we would
                need to add a new function/macro for
                mem_internal_realloc_zeroed, to mirror mem_sys_realloc_zeroed. */
        sized_pools = (Fixed_Size_Pool **)mem_internal_realloc(sized_pools,
                                           num_new * sizeof (void *));
        memset(sized_pools + num_old, 0, sizeof (void *) * (num_new - num_old));

        mem_pools->sized_header_pools = sized_pools;
        mem_pools->num_sized = num_new;
    }

    if (sized_pools[idx] == NULL)
        sized_pools[idx] = new_bufferlike_pool(interp, mem_pools, buffer_size);

    return sized_pools[idx];
}

/*

=item C<void initialize_fixed_size_pools(PARROT_INTERP, Memory_Pools
*mem_pools)>

The initialization routine for the interpreter's header pools. Initializes
pools for string headers, constant string headers, buffers, PMCs and
constant PMCs.

The C<string_header_pool> actually lives in the
sized pools, although no other sized pools are created here.

=cut

*/

void
initialize_fixed_size_pools(PARROT_INTERP, ARGIN(Memory_Pools *mem_pools))
{
    ASSERT_ARGS(initialize_fixed_size_pools)

    /* Init the constant string header pool */
    mem_pools->constant_string_header_pool       = new_string_pool(interp, mem_pools, 1);
    mem_pools->constant_string_header_pool->name = "constant_string_header";

    /* Init the buffer header pool
     *
     * The string_header_pool actually lives in the
     * sized_header_pools. These pool pointers only provide faster access in
     * new_*_header */

    /* Init the string header pool */
    mem_pools->string_header_pool       = new_string_pool(interp, mem_pools, 0);
    mem_pools->string_header_pool->name = "string_header";

    /* Init the PMC header pool */
    mem_pools->pmc_pool                 = new_pmc_pool(interp, mem_pools);
    mem_pools->pmc_pool->name           = "pmc";

    /* constant PMCs */
    mem_pools->constant_pmc_pool                    = new_pmc_pool(interp, mem_pools);
    mem_pools->constant_pmc_pool->name              = "constant_pmc";
    mem_pools->constant_pmc_pool->objects_per_alloc =
        CONSTANT_PMC_HEADERS_PER_ALLOC;
}


/*

=item C<int header_pools_iterate_callback(PARROT_INTERP, Memory_Pools
*mem_pools, int flag, void *arg, pool_iter_fn func)>

Iterates through header pools, invoking the given callback function on each
pool in the list matching the given criteria. Determines which pools to iterate
over depending on flags passed to the function.  Returns the callback's return
value, if non-zero. A non-zero return value from the callback function will
terminate the iteration prematurely.

=over 4

=item flag is one of

  POOL_PMC
  POOL_BUFFER
  POOL_CONST
  POOL_ALL

Only matching pools will be used. Notice that it is not possible to iterate
over certain sets of pools using the provided flags in the single pass. For
instance, both the PMC pool and the constant PMC pool cannot be iterated over
in a single pass.

=item arg

This argument is passed on to the iteration function.

=item pool_iter_fn

Called with C<(Parrot_Interp, Fixed_Size_Pool *, int flag, void *arg)>.  If
the function returns a non-zero value, iteration will stop.

=back

=cut

*/

PARROT_IGNORABLE_RESULT
int
header_pools_iterate_callback(PARROT_INTERP, ARGIN(Memory_Pools *mem_pools),
        int flag, ARGIN_NULLOK(void *arg), NOTNULL(pool_iter_fn func))
{
    ASSERT_ARGS(header_pools_iterate_callback)

    if (flag & POOL_PMC) {
        Fixed_Size_Pool *pool = flag & POOL_CONST
            ? mem_pools->constant_pmc_pool
            : mem_pools->pmc_pool;

        const int ret_val = (func)(interp, mem_pools, pool,
            flag & (POOL_PMC | POOL_CONST) , arg);

        if (ret_val)
            return ret_val;
    }

    if (flag & POOL_BUFFER) {
        INTVAL i;

        if (flag & POOL_CONST) {
            const int ret_val = (func)(interp, mem_pools,
                mem_pools->constant_string_header_pool,
                POOL_BUFFER | POOL_CONST, arg);

            if (ret_val)
                return ret_val;
        }

        for (i = mem_pools->num_sized - 1; i >= 0; --i) {
            Fixed_Size_Pool * const pool = mem_pools->sized_header_pools[i];

            if (pool) {
                const int ret_val = (func)(interp, mem_pools, pool, POOL_BUFFER, arg);
                if (ret_val)
                    return ret_val;
            }
        }
    }

    return 0;
}

/*

=over 4

=item C<void * Parrot_gc_get_attributes_from_pool(PARROT_INTERP,
PMC_Attribute_Pool * pool)>

Get a new fixed-size storage space from the given pool. The pool contains
information on the size of the item to allocate already.

=item C<static void Parrot_gc_allocate_new_attributes_arena(PARROT_INTERP,
PMC_Attribute_Pool *pool)>

Allocate a new arena of fixed-sized data structures for the given pool.

=item C<void Parrot_gc_initialize_fixed_size_pools(PARROT_INTERP, Memory_Pools *
const mem_pools, size_t init_num_pools)>

Initialize the pools (zeroize)

=item C<PMC_Attribute_Pool * Parrot_gc_get_attribute_pool(PARROT_INTERP,
Memory_Pools *mem_pools, size_t attrib_size)>

Find a fixed-sized data structure pool given the size of the object to
allocate. If the pool does not exist, create it.

=item C<static PMC_Attribute_Pool * Parrot_gc_create_attrib_pool(PARROT_INTERP,
size_t attrib_idx)>

Create a new pool for fixed-sized data items with the given C<attrib_size>.

=back

=cut

*/

PARROT_CANNOT_RETURN_NULL
void *
Parrot_gc_get_attributes_from_pool(PARROT_INTERP, ARGMOD(PMC_Attribute_Pool * pool))
{
    ASSERT_ARGS(Parrot_gc_get_attributes_from_pool)
    PMC_Attribute_Free_List *item;

#if GC_USE_LAZY_ALLOCATOR
    if (pool->free_list) {
        item            = pool->free_list;
        pool->free_list = item->next;
    }
    else if (pool->newfree) {
        item          = pool->newfree;
        pool->newfree = (PMC_Attribute_Free_List *)
                        ((char *)(pool->newfree) + pool->attr_size);
        if (pool->newfree >= pool->newlast)
            pool->newfree = NULL;
    }
    else {
        Parrot_gc_allocate_new_attributes_arena(interp, pool);
        return Parrot_gc_get_attributes_from_pool(interp, pool);
    }
#else
    if (pool->free_list == NULL)
        Parrot_gc_allocate_new_attributes_arena(interp, pool);
    item            = pool->free_list;
    pool->free_list = item->next;
#endif

    pool->num_free_objects--;
    return (void *)item;
}


static void
Parrot_gc_allocate_new_attributes_arena(PARROT_INTERP, ARGMOD(PMC_Attribute_Pool *pool))
{
    ASSERT_ARGS(Parrot_gc_allocate_new_attributes_arena)
    PMC_Attribute_Free_List *list, *next, *first;

    size_t       i;
    const size_t num_items  = pool->objects_per_alloc;
    const size_t item_size  = pool->attr_size;
    const size_t item_space = item_size * num_items;
    const size_t total_size = sizeof (PMC_Attribute_Arena) + item_space;

    PMC_Attribute_Arena * const new_arena = (PMC_Attribute_Arena *)mem_internal_allocate(
        total_size);

    new_arena->prev = NULL;
    new_arena->next = pool->top_arena;
    pool->top_arena = new_arena;
    next            = (PMC_Attribute_Free_List *)(new_arena + 1);

#if GC_USE_LAZY_ALLOCATOR
    pool->newfree   = next;
    pool->newlast   = (PMC_Attribute_Free_List *)((char *)next + item_space);
#else
    pool->free_list = next;
    for (i = 0; i < num_items; i++) {
        list        = next;
        list->next  = (PMC_Attribute_Free_List *)((char *)list + item_size);
        next        = list->next;
    }
    list->next      = pool->free_list;
#endif

    pool->num_free_objects += num_items;
    pool->total_objects    += num_items;
}

void
Parrot_gc_initialize_fixed_size_pools(PARROT_INTERP, ARGIN(Memory_Pools * const mem_pools),
        size_t init_num_pools)
{
    ASSERT_ARGS(Parrot_gc_initialize_fixed_size_pools)
    PMC_Attribute_Pool **pools;
    const size_t total_size = (init_num_pools + 1) * sizeof (void *);

    pools = (PMC_Attribute_Pool **)mem_internal_allocate(total_size);
    memset(pools, 0, total_size);

    mem_pools->attrib_pools = pools;
    mem_pools->num_attribs = init_num_pools;
}


PARROT_CANNOT_RETURN_NULL
PMC_Attribute_Pool *
Parrot_gc_get_attribute_pool(PARROT_INTERP, ARGIN(Memory_Pools *mem_pools), size_t attrib_size)
{
    ASSERT_ARGS(Parrot_gc_get_attribute_pool)

    PMC_Attribute_Pool **pools = mem_pools->attrib_pools;
    const size_t         idx   = (attrib_size < sizeof (void *))
                               ? 0
                               : attrib_size - sizeof (void *);

    if (mem_pools->num_attribs <= idx) {
        const size_t total_length = idx + GC_ATTRIB_POOLS_HEADROOM;
        const size_t total_size   = total_length * sizeof (void *);
        const size_t current_size = mem_pools->num_attribs;
        const size_t diff         = total_length - current_size;

        pools = (PMC_Attribute_Pool **)mem_internal_realloc(pools, total_size);
        memset(pools + current_size, 0, diff * sizeof (void *));
        mem_pools->attrib_pools = pools;
        mem_pools->num_attribs = total_length;
    }

    if (!pools[idx]) {
        PMC_Attribute_Pool * const pool = Parrot_gc_create_attrib_pool(interp, idx);
        /* Create the first arena now, so we don't have to check for it later */
        Parrot_gc_allocate_new_attributes_arena(interp, pool);
        pools[idx] = pool;
    }

    return pools[idx];
}

PARROT_CANNOT_RETURN_NULL
static PMC_Attribute_Pool *
Parrot_gc_create_attrib_pool(PARROT_INTERP, size_t attrib_idx)
{
    ASSERT_ARGS(Parrot_gc_create_attrib_pool)
    const size_t attrib_size = attrib_idx + sizeof (void *);
    const size_t num_objs_raw =
        (GC_FIXED_SIZE_POOL_SIZE - sizeof (PMC_Attribute_Arena)) / attrib_size;
    const size_t num_objs = (num_objs_raw == 0)?(1):(num_objs_raw);
    PMC_Attribute_Pool * const newpool = mem_internal_allocate_typed(PMC_Attribute_Pool);

    newpool->attr_size         = attrib_size;
    newpool->total_objects     = 0;
    newpool->objects_per_alloc = num_objs;
    newpool->num_free_objects  = 0;
    newpool->free_list         = NULL;
    newpool->top_arena         = NULL;

    return newpool;
}

/*

=item C<void Parrot_gc_destroy_header_pools(PARROT_INTERP, Memory_Pools
*mem_pools)>

Performs a garbage collection sweep on all pools, then frees them.  Calls
C<header_pools_iterate_callback> to loop over all the pools, passing
C<sweep_cb_pmc> and C<sweep_cb_buf> callback routines. Frees the array of sized
header pointers in the C<Memory_Pools> structure too.

=cut

*/

void
Parrot_gc_destroy_header_pools(PARROT_INTERP, ARGIN(Memory_Pools *mem_pools))
{
    ASSERT_ARGS(Parrot_gc_destroy_header_pools)
    INTVAL pass;

    /* const/non const COW strings life in different pools
     * so in first pass
     * COW refcount is done, in 2. refcounting
     * in 3rd freeing
     */
    const INTVAL start = 2;

    header_pools_iterate_callback(interp, mem_pools, POOL_PMC, NULL, sweep_cb_pmc);
    header_pools_iterate_callback(interp, mem_pools, POOL_PMC | POOL_CONST, NULL,
            sweep_cb_pmc);

    for (pass = start; pass <= 2; pass++) {
        header_pools_iterate_callback(interp, mem_pools, POOL_BUFFER | POOL_CONST,
                (void *)pass, sweep_cb_buf);
    }

    mem_internal_free(mem_pools->sized_header_pools);

    if (mem_pools->attrib_pools) {
        unsigned int i;
        for (i = 0; i < mem_pools->num_attribs; i++) {
            PMC_Attribute_Pool  *pool  = mem_pools->attrib_pools[i];
            PMC_Attribute_Arena *arena;

            if (!pool)
                continue;

            arena = pool->top_arena;

            while (arena) {
                PMC_Attribute_Arena *next = arena->next;
                mem_internal_free(arena);
                arena = next;
            }
            mem_internal_free(pool);
        }

        mem_internal_free(mem_pools->attrib_pools);
    }

    mem_pools->attrib_pools       = NULL;
    mem_pools->sized_header_pools = NULL;
}

/*

=item C<static int sweep_cb_pmc(PARROT_INTERP, Memory_Pools *mem_pools,
Fixed_Size_Pool *pool, int flag, void *arg)>

Performs a garbage collection sweep of the given pmc pool, then frees it. Calls
C<Parrot_gc_sweep_pool> to perform the sweep, and C<free_pool> to free the pool and
all its arenas. Always returns C<0>.

=cut

*/

static int
sweep_cb_pmc(PARROT_INTERP, ARGIN(Memory_Pools *mem_pools), ARGMOD(Fixed_Size_Pool *pool),
        SHIM(int flag), SHIM(void *arg))
{
    ASSERT_ARGS(sweep_cb_pmc)
    Parrot_gc_sweep_pool(interp, mem_pools, pool);
    free_pool(pool);
    return 0;
}

/*

=item C<static int sweep_cb_buf(PARROT_INTERP, Memory_Pools *mem_pools,
Fixed_Size_Pool *pool, int flag, void *arg)>

Performs a final garbage collection sweep, then frees the pool. Calls
C<Parrot_gc_sweep_pool> to perform the sweep, and C<free_pool> to free the pool and
all its arenas.

=cut

*/

static int
sweep_cb_buf(PARROT_INTERP, ARGIN(Memory_Pools *mem_pools), ARGMOD(Fixed_Size_Pool *pool), SHIM(int flag),
        ARGIN(void *arg))
{
    ASSERT_ARGS(sweep_cb_buf)
    UNUSED(arg);
    Parrot_gc_sweep_pool(interp, mem_pools, pool);
    free_pool(pool);

    return 0;
}

/*

=item C<static void free_pool(Fixed_Size_Pool *pool)>

Frees a pool and all of its arenas. Loops through the list of arenas backwards
and returns each to the memory manager. Then, frees the pool structure itself.

=cut

*/

static void
free_pool(ARGMOD(Fixed_Size_Pool *pool))
{
    ASSERT_ARGS(free_pool)
    Fixed_Size_Arena *cur_arena;

    for (cur_arena = pool->last_Arena; cur_arena;) {
        Fixed_Size_Arena * const next = cur_arena->prev;
        mem_internal_free(cur_arena->start_objects);
        mem_internal_free(cur_arena);
        cur_arena = next;
    }
    mem_internal_free(pool);
}


/*

=item C<void Parrot_gc_destroy_memory_pools(PARROT_INTERP, Memory_Pools
*mem_pools)>

Destroys the memory pool and the constant string pool. Loop through both
pools and destroy all memory blocks contained in them. Once all the
blocks are freed, free the pools themselves.

=cut

*/

void
Parrot_gc_destroy_memory_pools(PARROT_INTERP, ARGIN(Memory_Pools *mem_pools))
{
    ASSERT_ARGS(Parrot_gc_destroy_memory_pools)
    int i;

    for (i = 0; i < 2; i++) {
        Variable_Size_Pool * const pool = i ?
                mem_pools->constant_string_pool :
                mem_pools->memory_pool;
        Memory_Block *cur_block;

        cur_block = pool->top_block;

        while (cur_block) {
            Memory_Block * const next_block = cur_block->prev;
            mem_internal_free(cur_block);
            cur_block = next_block;
        }

        mem_internal_free(pool);
    }
}

/*

=item C<int Parrot_gc_ptr_in_memory_pool(PARROT_INTERP, Memory_Pools *mem_pools,
void *bufstart)>

Determines if the given C<bufstart> pointer points to a location inside the
memory pool. Returns 1 if the pointer is in the memory pool, 0 otherwise.

=cut

*/

PARROT_WARN_UNUSED_RESULT
int
Parrot_gc_ptr_in_memory_pool(PARROT_INTERP, ARGIN(Memory_Pools *mem_pools),
        ARGIN(void *bufstart))
{
    ASSERT_ARGS(Parrot_gc_ptr_in_memory_pool)
    Variable_Size_Pool * const pool = mem_pools->memory_pool;
    Memory_Block * cur_block = pool->top_block;

    while (cur_block) {
        if ((char *)bufstart >= cur_block->start &&
            (char *) bufstart < cur_block->start + cur_block->size) {
            return 1;
        }
        cur_block = cur_block->prev;
    }
    return 0;
}

/*

=item C<int Parrot_gc_ptr_is_pmc(PARROT_INTERP, Memory_Pools *mem_pools, void
*ptr)>

Determines if a given pointer is a PMC or not. It is a PMC if it is properly
contained in one of this interpreters PMC pools.

=cut

*/

int
Parrot_gc_ptr_is_pmc(PARROT_INTERP, ARGIN(Memory_Pools *mem_pools), ARGIN(void *ptr))
{
    ASSERT_ARGS(Parrot_gc_ptr_is_pmc)
        return contained_in_pool(interp, mem_pools->pmc_pool, ptr) ||
        contained_in_pool(interp, mem_pools->constant_pmc_pool, ptr);
}

/*

=item C<int Parrot_gc_get_pmc_index(PARROT_INTERP, Memory_Pools *mem_pools, PMC*
pmc)>

Gets the index of the PMC in the pool. The first PMC in the first pool is 1,
the second is 2, etc.

=cut

*/

int
Parrot_gc_get_pmc_index(PARROT_INTERP, ARGIN(Memory_Pools *mem_pools),
        ARGIN(PMC* pmc))
{
    ASSERT_ARGS(Parrot_gc_get_pmc_index)
    UINTVAL id = 1;     /* first PMC in first arena */
    Fixed_Size_Arena *arena;
    Fixed_Size_Pool *pool;

    pool = mem_pools->pmc_pool;
    for (arena = pool->last_Arena; arena; arena = arena->prev) {
        const ptrdiff_t ptr_diff = (ptrdiff_t)pmc - (ptrdiff_t)arena->start_objects;
        if (ptr_diff >= 0 && ptr_diff <
                (ptrdiff_t)(arena->used * pool->object_size)) {
            PARROT_ASSERT(ptr_diff % pool->object_size == 0);
            id += ptr_diff / pool->object_size;
            return id;
        }
        id += arena->total_objects;
    }

    pool = mem_pools->constant_pmc_pool;
    for (arena = pool->last_Arena; arena; arena = arena->prev) {
        const ptrdiff_t ptr_diff = (ptrdiff_t)pmc - (ptrdiff_t)arena->start_objects;
        if (ptr_diff >= 0 && ptr_diff <
                (ptrdiff_t)(arena->used * pool->object_size)) {
            PARROT_ASSERT(ptr_diff % pool->object_size == 0);
            id += ptr_diff / pool->object_size;
            return id;
        }
        id += arena->total_objects;
    }

    Parrot_ex_throw_from_c_args(interp, NULL, 1, "Couldn't find PMC in arenas");
}

/*

=item C<void Parrot_gc_merge_header_pools(PARROT_INTERP, Memory_Pools
*dest_arena, Memory_Pools *source_arena)>

Merges the header pools of C<source_arena> into those of C<dest_arena>.
(Used to deal with shared objects left after interpreter destruction.)

=cut

*/

void
Parrot_gc_merge_header_pools(PARROT_INTERP, ARGIN(Memory_Pools *dest_arena),
    ARGIN(Memory_Pools *source_arena))
{
    ASSERT_ARGS(Parrot_gc_merge_header_pools)

    UINTVAL        i;

    /* heavily borrowed from forall_header_pools */
    fix_pmc_syncs(interp, source_arena->constant_pmc_pool);
    Parrot_gc_merge_buffer_pools(interp, dest_arena->constant_pmc_pool,
            source_arena->constant_pmc_pool);

    fix_pmc_syncs(interp, source_arena->pmc_pool);
    Parrot_gc_merge_buffer_pools(interp, dest_arena->pmc_pool,
            source_arena->pmc_pool);

    Parrot_gc_merge_buffer_pools(interp,
            dest_arena->constant_string_header_pool,
            source_arena->constant_string_header_pool);

    for (i = 0; i < source_arena->num_sized; ++i) {
        if (!source_arena->sized_header_pools[i])
            continue;

        if (i >= dest_arena->num_sized
        || !dest_arena->sized_header_pools[i]) {
            Fixed_Size_Pool *ignored = get_bufferlike_pool(interp, dest_arena,
                    i * sizeof (void *));
            UNUSED(ignored);
            PARROT_ASSERT(dest_arena->sized_header_pools[i]);
        }

        Parrot_gc_merge_buffer_pools(interp,
            dest_arena->sized_header_pools[i],
            source_arena->sized_header_pools[i]);
    }
}


/*

=item C<static void Parrot_gc_merge_buffer_pools(PARROT_INTERP, Fixed_Size_Pool
*dest, Fixed_Size_Pool *source)>

Merge pool C<source> into pool C<dest>. Combines the free lists directly,
moves all arenas to the new pool, and remove the old pool. To merge, the
two pools must have the same object size, and the same name (if they have
names).

=cut

*/

static void
Parrot_gc_merge_buffer_pools(PARROT_INTERP,
        ARGMOD(Fixed_Size_Pool *dest), ARGMOD(Fixed_Size_Pool *source))
{
    ASSERT_ARGS(Parrot_gc_merge_buffer_pools)
    Fixed_Size_Arena  *cur_arena;
    GC_MS_PObj_Wrapper  *free_list_end;

    PARROT_ASSERT(dest->object_size == source->object_size);
    PARROT_ASSERT((dest->name == NULL && source->name == NULL)
                || STREQ(dest->name, source->name));

    dest->total_objects += source->total_objects;

    /* append new free_list to old */
    /* XXX this won't work with, e.g., gc_gms */
    free_list_end = dest->free_list;

    if (free_list_end == NULL)
        dest->free_list = source->free_list;
    else {
        while (free_list_end->next_ptr)
            free_list_end = free_list_end->next_ptr;

        free_list_end->next_ptr = source->free_list;
    }

    /* now append source arenas */
    cur_arena = source->last_Arena;

    while (cur_arena) {
        size_t                     total_objects;
        Fixed_Size_Arena * const next_arena = cur_arena->prev;

        cur_arena->next = cur_arena->prev = NULL;

        total_objects   = cur_arena->total_objects;

        Parrot_append_arena_in_pool(interp, dest, cur_arena,
            cur_arena->total_objects);

        /* XXX needed? */
        cur_arena->total_objects = total_objects;

        cur_arena = next_arena;
    }

    /* remove things from source */
    source->last_Arena       = NULL;
    source->free_list        = NULL;
    source->total_objects    = 0;
    source->num_free_objects = 0;
}

/*

=item C<static void fix_pmc_syncs(Interp *dest_interp, Fixed_Size_Pool *pool)>

Walks through the given arena, looking for all live and shared PMCs,
transferring their sync values to the destination interpreter.

=cut

*/

static void
fix_pmc_syncs(ARGMOD(Interp *dest_interp), ARGIN(Fixed_Size_Pool *pool))
{
    ASSERT_ARGS(fix_pmc_syncs)
    Fixed_Size_Arena *cur_arena;
    const UINTVAL       object_size = pool->object_size;

    for (cur_arena = pool->last_Arena; cur_arena; cur_arena = cur_arena->prev) {
        PMC   *p = (PMC *)((char*)cur_arena->start_objects);
        size_t i;

        for (i = 0; i < cur_arena->used; i++) {
            if (!PObj_on_free_list_TEST(p) && PObj_is_PMC_TEST(p)) {
                if (PObj_is_PMC_shared_TEST(p))
                    PMC_sync(p)->owner = dest_interp;
                else
                    Parrot_ex_throw_from_c_args(dest_interp, NULL,
                        EXCEPTION_INTERP_ERROR,
                        "Unshared PMC still alive after interpreter"
                        "destruction. address=%p, base_type=%d\n",
                        p, p->vtable->base_type);
            }

            p = (PMC *)((char *)p + object_size);
        }
    }
}


/*

=back

=head1 SEE ALSO

F<include/parrot/mark_sweep.h>, F<docs/memory_internals.pod>.

=cut

*/

/*
 * Local variables:
 *   c-file-style: "parrot"
 * End:
 * vim: expandtab shiftwidth=4:
 */
