/*
Copyright (C) 2001-2009, Parrot Foundation.
$Id$

=head1 NAME

src/gc/gc_ms.c - Implementation of the basic mark & sweep collector

=head1 DESCRIPTION

This code implements the default mark and sweep garbage collector.

=cut

*/

#include "parrot/parrot.h"
#include "gc_private.h"

/* HEADERIZER HFILE: src/gc/gc_private.h */

/* HEADERIZER BEGIN: static */
/* Don't modify between HEADERIZER BEGIN / HEADERIZER END.  Your changes will be lost. */

static void gc_ms_add_free_object(SHIM_INTERP,
    ARGIN(Memory_Pools *mem_pools),
    ARGMOD(Fixed_Size_Pool *pool),
    ARGIN(void *to_add))
        __attribute__nonnull__(2)
        __attribute__nonnull__(3)
        __attribute__nonnull__(4)
        FUNC_MODIFIES(*pool);

static void gc_ms_alloc_objects(PARROT_INTERP,
    ARGIN(Memory_Pools *mem_pools),
    ARGMOD(Fixed_Size_Pool *pool))
        __attribute__nonnull__(1)
        __attribute__nonnull__(2)
        __attribute__nonnull__(3)
        FUNC_MODIFIES(*pool);

PARROT_CAN_RETURN_NULL
static void* gc_ms_allocate_attributes(PARROT_INTERP, ARGIN(PMC *pmc))
        __attribute__nonnull__(1)
        __attribute__nonnull__(2);

PARROT_CAN_RETURN_NULL
static void* gc_ms_allocate_buffer(PARROT_INTERP, size_t size)
        __attribute__nonnull__(1);

PARROT_CAN_RETURN_NULL
static void* gc_ms_allocate_buffer_with_pointers(PARROT_INTERP, size_t size)
        __attribute__nonnull__(1);

PARROT_CAN_RETURN_NULL
static void* gc_ms_allocate_fixed_size_buffer(PARROT_INTERP, size_t size)
        __attribute__nonnull__(1);

PARROT_CAN_RETURN_NULL
static PMC* gc_ms_allocate_pmc_header(PARROT_INTERP, UINTVAL flags)
        __attribute__nonnull__(1);

PARROT_CAN_RETURN_NULL
static STRING* gc_ms_allocate_string_header(PARROT_INTERP, UINTVAL flags)
        __attribute__nonnull__(1);

static void gc_ms_block_gc_mark(PARROT_INTERP)
        __attribute__nonnull__(1);

static void gc_ms_block_gc_sweep(PARROT_INTERP)
        __attribute__nonnull__(1);

static void gc_ms_finalize(PARROT_INTERP,
    ARGIN(Memory_Pools * const mem_pools))
        __attribute__nonnull__(1)
        __attribute__nonnull__(2);

static void gc_ms_free_attributes(PARROT_INTERP, ARGIN(PMC *pmc))
        __attribute__nonnull__(1)
        __attribute__nonnull__(2);

static void gc_ms_free_pmc_header(PARROT_INTERP, ARGFREE(PMC *pmc))
        __attribute__nonnull__(1);

static void gc_ms_free_string_header(PARROT_INTERP, ARGFREE(STRING *string))
        __attribute__nonnull__(1);

PARROT_CANNOT_RETURN_NULL
PARROT_WARN_UNUSED_RESULT
static void * gc_ms_get_free_object(PARROT_INTERP,
    ARGIN(Memory_Pools *mem_pools),
    ARGMOD(Fixed_Size_Pool *pool))
        __attribute__nonnull__(1)
        __attribute__nonnull__(2)
        __attribute__nonnull__(3)
        FUNC_MODIFIES(*pool);

static size_t gc_ms_get_gc_info(PARROT_INTERP, Interpinfo_enum which)
        __attribute__nonnull__(1);

static void gc_ms_init_child_inter(
    ARGIN(Interp *parent_interp),
    ARGIN(Interp *child_interp))
        __attribute__nonnull__(1)
        __attribute__nonnull__(2);

static void gc_ms_mark_and_sweep(PARROT_INTERP, UINTVAL flags)
        __attribute__nonnull__(1);

static void gc_ms_more_traceable_objects(PARROT_INTERP,
    ARGIN(Memory_Pools *mem_pools),
    ARGMOD(Fixed_Size_Pool *pool))
        __attribute__nonnull__(1)
        __attribute__nonnull__(2)
        __attribute__nonnull__(3)
        FUNC_MODIFIES(*pool);

static void gc_ms_pool_init(SHIM_INTERP, ARGMOD(Fixed_Size_Pool *pool))
        __attribute__nonnull__(2)
        FUNC_MODIFIES(*pool);

PARROT_CAN_RETURN_NULL
static void* gc_ms_reallocate_buffer(PARROT_INTERP,
    void *old,
    size_t newsize)
        __attribute__nonnull__(1);

static int gc_ms_sweep_cb(PARROT_INTERP,
    ARGIN(Memory_Pools *mem_pools),
    ARGMOD(Fixed_Size_Pool *pool),
    int flag,
    ARGMOD(void *arg))
        __attribute__nonnull__(1)
        __attribute__nonnull__(2)
        __attribute__nonnull__(3)
        __attribute__nonnull__(5)
        FUNC_MODIFIES(*pool)
        FUNC_MODIFIES(*arg);

static int gc_ms_trace_active_PMCs(PARROT_INTERP,
    Parrot_gc_trace_type trace)
        __attribute__nonnull__(1);

static void gc_ms_unblock_gc_mark(PARROT_INTERP)
        __attribute__nonnull__(1);

static void gc_ms_unblock_gc_sweep(PARROT_INTERP)
        __attribute__nonnull__(1);

static void Parrot_gc_free_attributes_from_pool(PARROT_INTERP,
    ARGMOD(PMC_Attribute_Pool *pool),
    ARGMOD(void *data))
        __attribute__nonnull__(1)
        __attribute__nonnull__(2)
        __attribute__nonnull__(3)
        FUNC_MODIFIES(*pool)
        FUNC_MODIFIES(*data);

#define ASSERT_ARGS_gc_ms_add_free_object __attribute__unused__ int _ASSERT_ARGS_CHECK = (\
       PARROT_ASSERT_ARG(mem_pools) \
    , PARROT_ASSERT_ARG(pool) \
    , PARROT_ASSERT_ARG(to_add))
#define ASSERT_ARGS_gc_ms_alloc_objects __attribute__unused__ int _ASSERT_ARGS_CHECK = (\
       PARROT_ASSERT_ARG(interp) \
    , PARROT_ASSERT_ARG(mem_pools) \
    , PARROT_ASSERT_ARG(pool))
#define ASSERT_ARGS_gc_ms_allocate_attributes __attribute__unused__ int _ASSERT_ARGS_CHECK = (\
       PARROT_ASSERT_ARG(interp) \
    , PARROT_ASSERT_ARG(pmc))
#define ASSERT_ARGS_gc_ms_allocate_buffer __attribute__unused__ int _ASSERT_ARGS_CHECK = (\
       PARROT_ASSERT_ARG(interp))
#define ASSERT_ARGS_gc_ms_allocate_buffer_with_pointers \
     __attribute__unused__ int _ASSERT_ARGS_CHECK = (\
       PARROT_ASSERT_ARG(interp))
#define ASSERT_ARGS_gc_ms_allocate_fixed_size_buffer \
     __attribute__unused__ int _ASSERT_ARGS_CHECK = (\
       PARROT_ASSERT_ARG(interp))
#define ASSERT_ARGS_gc_ms_allocate_pmc_header __attribute__unused__ int _ASSERT_ARGS_CHECK = (\
       PARROT_ASSERT_ARG(interp))
#define ASSERT_ARGS_gc_ms_allocate_string_header __attribute__unused__ int _ASSERT_ARGS_CHECK = (\
       PARROT_ASSERT_ARG(interp))
#define ASSERT_ARGS_gc_ms_block_gc_mark __attribute__unused__ int _ASSERT_ARGS_CHECK = (\
       PARROT_ASSERT_ARG(interp))
#define ASSERT_ARGS_gc_ms_block_gc_sweep __attribute__unused__ int _ASSERT_ARGS_CHECK = (\
       PARROT_ASSERT_ARG(interp))
#define ASSERT_ARGS_gc_ms_finalize __attribute__unused__ int _ASSERT_ARGS_CHECK = (\
       PARROT_ASSERT_ARG(interp) \
    , PARROT_ASSERT_ARG(mem_pools))
#define ASSERT_ARGS_gc_ms_free_attributes __attribute__unused__ int _ASSERT_ARGS_CHECK = (\
       PARROT_ASSERT_ARG(interp) \
    , PARROT_ASSERT_ARG(pmc))
#define ASSERT_ARGS_gc_ms_free_pmc_header __attribute__unused__ int _ASSERT_ARGS_CHECK = (\
       PARROT_ASSERT_ARG(interp))
#define ASSERT_ARGS_gc_ms_free_string_header __attribute__unused__ int _ASSERT_ARGS_CHECK = (\
       PARROT_ASSERT_ARG(interp))
#define ASSERT_ARGS_gc_ms_get_free_object __attribute__unused__ int _ASSERT_ARGS_CHECK = (\
       PARROT_ASSERT_ARG(interp) \
    , PARROT_ASSERT_ARG(mem_pools) \
    , PARROT_ASSERT_ARG(pool))
#define ASSERT_ARGS_gc_ms_get_gc_info __attribute__unused__ int _ASSERT_ARGS_CHECK = (\
       PARROT_ASSERT_ARG(interp))
#define ASSERT_ARGS_gc_ms_init_child_inter __attribute__unused__ int _ASSERT_ARGS_CHECK = (\
       PARROT_ASSERT_ARG(parent_interp) \
    , PARROT_ASSERT_ARG(child_interp))
#define ASSERT_ARGS_gc_ms_mark_and_sweep __attribute__unused__ int _ASSERT_ARGS_CHECK = (\
       PARROT_ASSERT_ARG(interp))
#define ASSERT_ARGS_gc_ms_more_traceable_objects __attribute__unused__ int _ASSERT_ARGS_CHECK = (\
       PARROT_ASSERT_ARG(interp) \
    , PARROT_ASSERT_ARG(mem_pools) \
    , PARROT_ASSERT_ARG(pool))
#define ASSERT_ARGS_gc_ms_pool_init __attribute__unused__ int _ASSERT_ARGS_CHECK = (\
       PARROT_ASSERT_ARG(pool))
#define ASSERT_ARGS_gc_ms_reallocate_buffer __attribute__unused__ int _ASSERT_ARGS_CHECK = (\
       PARROT_ASSERT_ARG(interp))
#define ASSERT_ARGS_gc_ms_sweep_cb __attribute__unused__ int _ASSERT_ARGS_CHECK = (\
       PARROT_ASSERT_ARG(interp) \
    , PARROT_ASSERT_ARG(mem_pools) \
    , PARROT_ASSERT_ARG(pool) \
    , PARROT_ASSERT_ARG(arg))
#define ASSERT_ARGS_gc_ms_trace_active_PMCs __attribute__unused__ int _ASSERT_ARGS_CHECK = (\
       PARROT_ASSERT_ARG(interp))
#define ASSERT_ARGS_gc_ms_unblock_gc_mark __attribute__unused__ int _ASSERT_ARGS_CHECK = (\
       PARROT_ASSERT_ARG(interp))
#define ASSERT_ARGS_gc_ms_unblock_gc_sweep __attribute__unused__ int _ASSERT_ARGS_CHECK = (\
       PARROT_ASSERT_ARG(interp))
#define ASSERT_ARGS_Parrot_gc_free_attributes_from_pool \
     __attribute__unused__ int _ASSERT_ARGS_CHECK = (\
       PARROT_ASSERT_ARG(interp) \
    , PARROT_ASSERT_ARG(pool) \
    , PARROT_ASSERT_ARG(data))
/* Don't modify between HEADERIZER BEGIN / HEADERIZER END.  Your changes will be lost. */
/* HEADERIZER END: static */

/*

=head2 Primary MS Functions

=over 4

=item C<void Parrot_gc_ms_init(PARROT_INTERP)>

Initialize the state structures of the gc system. Called immediately before
creation of memory pools. This function must set the function pointers
for C<add_free_object_fn>, C<get_free_object_fn>, C<alloc_object_fn>, and
C<more_object_fn>.

=cut

*/

void
Parrot_gc_ms_init(PARROT_INTERP)
{
    ASSERT_ARGS(Parrot_gc_ms_init)

    Memory_Pools *mem_pools = mem_allocate_zeroed_typed(Memory_Pools);
    GC_Subsystem *gc = interp->gc_sys;

    gc->init_child_interp  = gc_ms_init_child_inter;
    gc->do_gc_mark         = gc_ms_mark_and_sweep;
    gc->finalize_gc_system = NULL;

    gc->allocate_pmc_header     = gc_ms_allocate_pmc_header;
    gc->free_pmc_header         = gc_ms_free_pmc_header;
    gc->allocate_string_header  = gc_ms_allocate_string_header;
    gc->free_string_header      = gc_ms_free_string_header;
    gc->allocate_attributes     = gc_ms_allocate_attributes;
    gc->free_attributes         = gc_ms_free_attributes;

    gc->allocate_buffer               = gc_ms_allocate_buffer;
    gc->reallocate_buffer             = gc_ms_reallocate_buffer;
    gc->allocate_buffer_with_pointers = gc_ms_allocate_buffer_with_pointers;

    gc->block_gc_mark           = gc_ms_block_gc_mark;
    gc->unblock_gc_mark         = gc_ms_unblock_gc_mark;
    gc->block_gc_sweep          = gc_ms_block_gc_sweep;
    gc->unblock_gc_sweep        = gc_ms_unblock_gc_sweep;

    gc->get_gc_info             = gc_ms_get_gc_info;

    mem_pools->init_pool          = gc_ms_pool_init;
    mem_pools->num_sized          = 0;
    mem_pools->num_attribs        = 0;
    mem_pools->attrib_pools       = NULL;
    mem_pools->sized_header_pools = NULL;

    initialize_var_size_pools(interp, mem_pools);
    initialize_fixed_size_pools(interp, mem_pools);
    Parrot_gc_initialize_fixed_size_pools(interp, mem_pools, GC_NUM_INITIAL_FIXED_SIZE_POOLS);

    gc->gc_private = mem_pools;
}

/*

=item C<static void gc_ms_init_child_inter(Interp *parent_interp, Interp
*child_interp)>

Initialize child interp.

=cut

*/
static void
gc_ms_init_child_inter(ARGIN(Interp *parent_interp), ARGIN(Interp *child_interp))
{
    ASSERT_ARGS(gc_ms_init_child_inter)
}

/*

=item C<static void gc_ms_mark_and_sweep(PARROT_INTERP, UINTVAL flags)>

Runs the stop-the-world mark & sweep (MS) collector.

=cut

*/

static void
gc_ms_mark_and_sweep(PARROT_INTERP, UINTVAL flags)
{
    ASSERT_ARGS(gc_ms_mark_and_sweep)
    Memory_Pools * const mem_pools = (Memory_Pools*)interp->gc_sys->gc_private;
    int total_free = 0;

    if (mem_pools->gc_mark_block_level)
        return;

    if (interp->pdb && interp->pdb->debugger) {
        /* The debugger could have performed a mark. Make sure everything is
           marked dead here, so that when we sweep it all gets collected */
        Parrot_gc_clear_live_bits(interp, mem_pools->pmc_pool);
    }

    if (flags & GC_finish_FLAG) {
        gc_ms_finalize(interp, mem_pools);
        return;
    }

    ++mem_pools->gc_mark_block_level;
    mem_pools->lazy_gc = flags & GC_lazy_FLAG;

    /* tell the threading system that we're doing GC mark */
    pt_gc_start_mark(interp);
    Parrot_gc_run_init(interp, mem_pools);

    /* compact STRING pools to collect free headers and allocated buffers */
    Parrot_gc_compact_memory_pool(interp);

    /* Now go trace the PMCs. returning true means we did a complete trace.
       false means it was a lazy trace. */
    if (gc_ms_trace_active_PMCs(interp, (flags & GC_trace_stack_FLAG)
        ? GC_TRACE_FULL : GC_TRACE_ROOT_ONLY)) {

        mem_pools->gc_trace_ptr = NULL;
        mem_pools->gc_mark_ptr  = NULL;

        /* We've done the mark, now do the sweep. Pass the sweep callback
           function to the PMC pool and all the sized pools. */
        header_pools_iterate_callback(interp, mem_pools, POOL_BUFFER | POOL_PMC,
            (void*)&total_free, gc_ms_sweep_cb);

    }
    else {
        ++mem_pools->gc_lazy_mark_runs;

        Parrot_gc_clear_live_bits(interp, mem_pools->pmc_pool);
    }

    pt_gc_stop_mark(interp);

    /* Note it */
    mem_pools->gc_mark_runs++;
    --mem_pools->gc_mark_block_level;

    return;
}

/*

=item C<static void gc_ms_finalize(PARROT_INTERP, Memory_Pools * const
mem_pools)>

Perform the finalization run, freeing all PMCs.

=cut

*/

static void
gc_ms_finalize(PARROT_INTERP, ARGIN(Memory_Pools * const mem_pools))
{
    ASSERT_ARGS(gc_ms_finalize)
    Parrot_gc_clear_live_bits(interp, mem_pools->pmc_pool);
    Parrot_gc_clear_live_bits(interp, mem_pools->constant_pmc_pool);

    /* keep the scheduler and its kids alive for Task-like PMCs to destroy
     * themselves; run a sweep to collect them */
    if (interp->scheduler) {
        Parrot_gc_mark_PMC_alive(interp, interp->scheduler);
        VTABLE_mark(interp, interp->scheduler);
        Parrot_gc_sweep_pool(interp, mem_pools, mem_pools->pmc_pool);
    }

    /* now sweep everything that's left */
    Parrot_gc_sweep_pool(interp, mem_pools, mem_pools->pmc_pool);
    Parrot_gc_sweep_pool(interp, mem_pools, mem_pools->constant_pmc_pool);
}

/*

=item C<static PMC* gc_ms_allocate_pmc_header(PARROT_INTERP, UINTVAL flags)>

=item C<static void gc_ms_free_pmc_header(PARROT_INTERP, PMC *pmc)>

=item C<static STRING* gc_ms_allocate_string_header(PARROT_INTERP, UINTVAL
flags)>

=item C<static void gc_ms_free_string_header(PARROT_INTERP, STRING *string)>

=item C<static void* gc_ms_allocate_attributes(PARROT_INTERP, PMC *pmc)>

=item C<static void gc_ms_free_attributes(PARROT_INTERP, PMC *pmc)>

Various allocation/deallocation routines.

=cut

*/

PARROT_CAN_RETURN_NULL
static PMC*
gc_ms_allocate_pmc_header(PARROT_INTERP, UINTVAL flags)
{
    ASSERT_ARGS(gc_ms_allocate_pmc_header)
    Memory_Pools           *mem_pools = (Memory_Pools*)interp->gc_sys->gc_private;
    Fixed_Size_Pool * const pool      = flags & PObj_constant_FLAG
            ? mem_pools->constant_pmc_pool
            : mem_pools->pmc_pool;

    return (PMC *)pool->get_free_object(interp, mem_pools, pool);
}

static void
gc_ms_free_pmc_header(PARROT_INTERP, ARGFREE(PMC *pmc))
{
    ASSERT_ARGS(gc_ms_free_pmc_header)
    Memory_Pools           *mem_pools = (Memory_Pools*)interp->gc_sys->gc_private;
    Fixed_Size_Pool * const pool      = pmc->flags & PObj_constant_FLAG
            ? mem_pools->constant_pmc_pool
            : mem_pools->pmc_pool;
    pool->add_free_object(interp, mem_pools, pool, (PObj *)pmc);
    pool->num_free_objects++;
}


PARROT_CAN_RETURN_NULL
static STRING*
gc_ms_allocate_string_header(PARROT_INTERP, UINTVAL flags)
{
    ASSERT_ARGS(gc_ms_allocate_string_header)
    Memory_Pools           *mem_pools = (Memory_Pools*)interp->gc_sys->gc_private;
    Fixed_Size_Pool * const pool      = flags & PObj_constant_FLAG
            ? mem_pools->constant_string_header_pool
            : mem_pools->string_header_pool;
    return (STRING*)pool->get_free_object(interp, mem_pools, pool);
}

static void
gc_ms_free_string_header(PARROT_INTERP, ARGFREE(STRING *string))
{
    ASSERT_ARGS(gc_ms_free_string_header)
    if (!PObj_constant_TEST(string)) {
        Memory_Pools           *mem_pools = (Memory_Pools*)interp->gc_sys->gc_private;
        Fixed_Size_Pool * const pool      = string->flags & PObj_constant_FLAG
                ? mem_pools->constant_string_header_pool
                : mem_pools->string_header_pool;
        pool->add_free_object(interp, mem_pools, pool, (PObj *)string);
        pool->num_free_objects++;
    }
}


PARROT_CAN_RETURN_NULL
static void*
gc_ms_allocate_attributes(PARROT_INTERP, ARGIN(PMC *pmc))
{
    ASSERT_ARGS(gc_ms_allocate_attributes)
    Memory_Pools * const mem_pools = (Memory_Pools*)interp->gc_sys->gc_private;
    const size_t         attr_size = pmc->vtable->attr_size;
#if GC_USE_FIXED_SIZE_ALLOCATOR
    PMC_Attribute_Pool * const pool = Parrot_gc_get_attribute_pool(interp,
            mem_pools, attr_size);
    void * const attrs = Parrot_gc_get_attributes_from_pool(interp, pool);
    memset(attrs, 0, attr_size);
    PMC_data(pmc) = attrs;
    return attrs;
#else
    void * const data =  mem_sys_allocate_zeroed(attr_size);
    PMC_data(pmc) = data;
    return data;
#endif
}

static void
gc_ms_free_attributes(PARROT_INTERP, ARGIN(PMC *pmc))
{
    ASSERT_ARGS(gc_ms_free_attributes)
    Memory_Pools * const mem_pools = (Memory_Pools*)interp->gc_sys->gc_private;
    void                *data      = PMC_data(pmc);
    if (data) {

#if GC_USE_FIXED_SIZE_ALLOCATOR
        const size_t attr_size = pmc->vtable->attr_size;
        const size_t item_size = attr_size < sizeof (void *) ? sizeof (void *) : attr_size;
        PMC_Attribute_Pool ** const pools = mem_pools->attrib_pools;
        const size_t idx = item_size - sizeof (void *);
        Parrot_gc_free_attributes_from_pool(interp, pools[idx], data);
#else
        mem_sys_free(PMC_data(pmc));
        PMC_data(pmc) = NULL;
#endif
    }
}


PARROT_CAN_RETURN_NULL
static void*
gc_ms_allocate_buffer(PARROT_INTERP, size_t size)
{
    ASSERT_ARGS(gc_ms_allocate_buffer)
    Memory_Pools * const mem_pools = (Memory_Pools*)interp->gc_sys->gc_private;
    return mem_allocate(interp, mem_pools, size, mem_pools->memory_pool);
}

PARROT_CAN_RETURN_NULL
static void*
gc_ms_reallocate_buffer(PARROT_INTERP, void *old, size_t newsize)
{
    ASSERT_ARGS(gc_ms_allocate_buffer)
    Memory_Pools * const mem_pools = (Memory_Pools*)interp->gc_sys->gc_private;
    size_t copysize;
    char  *mem;
    //Variable_Size_Pool * const pool = interp->mem_pools->memory_pool;
    size_t new_size, needed, old_size;

    Buffer *buffer = (Buffer *)old;

    /*
     * If there is no old buffer just allocate new one
     */
    if (!buffer)
        return gc_ms_allocate_buffer(interp, newsize);

    /*
     * we don't shrink buffers
     */
    if (newsize <= Buffer_buflen(buffer))
        return old;

    /*
     * same as below but barely used and tested - only 3 list related
     * tests do use true reallocation
     *
     * list.c, which does _reallocate, has 2 reallocations
     * normally, which play ping pong with buffers.
     * The normal case is therefore always to allocate a new block
     */
    new_size = aligned_size(buffer, newsize);
    old_size = aligned_size(buffer, Buffer_buflen(buffer));

    needed   = new_size - old_size;

#if 0
    XXX
    if ((pool->top_block->free >= needed)
    &&  (pool->top_block->top  == (char *)Buffer_bufstart(buffer) + old_size)) {
        pool->top_block->free -= needed;
        pool->top_block->top  += needed;
        Buffer_buflen(buffer) = newsize;
        return;
    }

    copysize = Buffer_buflen(buffer);

    if (!PObj_COW_TEST(buffer))
        pool->guaranteed_reclaimable += copysize;
    else
        pool->possibly_reclaimable   += copysize;
#endif

    mem = (char *)gc_ms_allocate_buffer(interp, new_size);
    mem = aligned_mem(buffer, mem);

    /* We shouldn't ever have a 0 from size, but we do. If we can track down
     * those bugs, this can be removed which would make things cheaper */
    if (copysize)
        memcpy(mem, Buffer_bufstart(buffer), copysize);

    return mem;
}

PARROT_CAN_RETURN_NULL
static void*
gc_ms_allocate_buffer_with_pointers(PARROT_INTERP, size_t size)
{
    ASSERT_ARGS(gc_ms_allocate_buffer_with_pointers)
    return gc_ms_allocate_fixed_size_buffer(interp, size);
}

PARROT_CAN_RETURN_NULL
static void*
gc_ms_allocate_fixed_size_buffer(PARROT_INTERP, size_t size)
{
    ASSERT_ARGS(gc_ms_allocate_fixed_size_buffer)

    Memory_Pools * const mem_pools = (Memory_Pools*)interp->gc_sys->gc_private;
    PMC_Attribute_Pool *pool = NULL;
    const size_t idx = (size < sizeof (void *)) ? 0 : (size - sizeof (void *));

    /* get the pool directly, if possible, for great speed */
    if (mem_pools->num_attribs > idx)
        pool = mem_pools->attrib_pools[idx];

    /* otherwise create it */
    if (!pool)
        pool = Parrot_gc_get_attribute_pool(interp, mem_pools, size);

    return Parrot_gc_get_attributes_from_pool(interp, pool);
}

static void
gc_ms_block_gc_mark(PARROT_INTERP)
{
    ASSERT_ARGS(gc_ms_block_gc_sweep)
    Memory_Pools * const mem_pools = (Memory_Pools*)interp->gc_sys->gc_private;
    mem_pools->gc_mark_block_level++;
}

static void
gc_ms_unblock_gc_mark(PARROT_INTERP)
{
    ASSERT_ARGS(gc_ms_unblock_gc_sweep)
    Memory_Pools * const mem_pools = (Memory_Pools*)interp->gc_sys->gc_private;
    if (mem_pools->gc_mark_block_level)
        mem_pools->gc_mark_block_level--;
}


static void
gc_ms_block_gc_sweep(PARROT_INTERP)
{
    ASSERT_ARGS(gc_ms_block_gc_sweep)
    Memory_Pools * const mem_pools = (Memory_Pools*)interp->gc_sys->gc_private;
    mem_pools->gc_sweep_block_level++;
}

static void
gc_ms_unblock_gc_sweep(PARROT_INTERP)
{
    ASSERT_ARGS(gc_ms_unblock_gc_sweep)
    Memory_Pools * const mem_pools = (Memory_Pools*)interp->gc_sys->gc_private;
    if (mem_pools->gc_sweep_block_level)
        mem_pools->gc_sweep_block_level--;
}

static size_t
gc_ms_get_gc_info(PARROT_INTERP, Interpinfo_enum which)
{
    ASSERT_ARGS(gc_ms_get_gc_info)
    return 0;
}

/*

=item C<static int gc_ms_trace_active_PMCs(PARROT_INTERP, Parrot_gc_trace_type
trace)>

Performs a full trace run and marks all the PMCs as active if they
are. Returns whether the run completed, that is, whether it's safe
to proceed with GC.

=cut

*/

static int
gc_ms_trace_active_PMCs(PARROT_INTERP, Parrot_gc_trace_type trace)
{
    ASSERT_ARGS(gc_ms_trace_active_PMCs)
    Memory_Pools *mem_pools = (Memory_Pools*)interp->gc_sys->gc_private;

    if (!Parrot_gc_trace_root(interp, mem_pools, trace))
        return 0;

    pt_gc_mark_root_finished(interp);
    mem_pools->gc_trace_ptr  = NULL;
    return 1;

}

/*

=item C<static int gc_ms_sweep_cb(PARROT_INTERP, Memory_Pools *mem_pools,
Fixed_Size_Pool *pool, int flag, void *arg)>

Sweeps the given pool for the MS collector. This function also ends
the profiling timer, if profiling is enabled. Returns the total number
of objects freed.

=cut

*/

static int
gc_ms_sweep_cb(PARROT_INTERP, ARGIN(Memory_Pools *mem_pools), ARGMOD(Fixed_Size_Pool *pool),
        int flag, ARGMOD(void *arg))
{
    ASSERT_ARGS(gc_ms_sweep_cb)
    int * const total_free = (int *) arg;

    Parrot_gc_sweep_pool(interp, mem_pools, pool);

    *total_free += pool->num_free_objects;

    return 0;
}

/*

=back

=head2 MS Pool Functions

=over 4

=item C<static void gc_ms_pool_init(PARROT_INTERP, Fixed_Size_Pool *pool)>

Initialize a memory pool for the MS garbage collector system. Sets the
function pointers necessary to perform basic operations on a pool, such
as object allocation.

=cut

*/

static void
gc_ms_pool_init(SHIM_INTERP, ARGMOD(Fixed_Size_Pool *pool))
{
    ASSERT_ARGS(gc_ms_pool_init)
    pool->add_free_object = gc_ms_add_free_object;
    pool->get_free_object = gc_ms_get_free_object;
    pool->alloc_objects   = gc_ms_alloc_objects;
    pool->more_objects    = gc_ms_more_traceable_objects;
}

/*

=item C<static void gc_ms_more_traceable_objects(PARROT_INTERP, Memory_Pools
*mem_pools, Fixed_Size_Pool *pool)>

We're out of traceable objects. First we try a GC run to free some up. If
that doesn't work, allocate a new arena.

=cut

*/

static void
gc_ms_more_traceable_objects(PARROT_INTERP, ARGIN(Memory_Pools *mem_pools),
        ARGMOD(Fixed_Size_Pool *pool))
{
    ASSERT_ARGS(gc_ms_more_traceable_objects)

    if (pool->skip)
        pool->skip = 0;
    else {
        Fixed_Size_Arena * const arena = pool->last_Arena;
        if (arena
        &&  arena->used == arena->total_objects)
                Parrot_gc_mark_and_sweep(interp, GC_trace_stack_FLAG);
    }

    /* requires that num_free_objects be updated in Parrot_gc_mark_and_sweep.
       If gc is disabled, then we must check the free list directly. */
#if GC_USE_LAZY_ALLOCATOR
    if ((!pool->free_list || pool->num_free_objects < pool->replenish_level)
        && !pool->newfree)
        (*pool->alloc_objects) (interp, mem_pools, pool);
#else
    if (!pool->free_list || pool->num_free_objects < pool->replenish_level)
    (*pool->alloc_objects) (interp, mem_pools, pool);
#endif
}

/*

=item C<static void gc_ms_add_free_object(PARROT_INTERP, Memory_Pools
*mem_pools, Fixed_Size_Pool *pool, void *to_add)>

Add an unused object back to the pool's free list for later reuse. Set
the PObj flags to indicate that the item is free.

=cut

*/

static void
gc_ms_add_free_object(SHIM_INTERP, ARGIN(Memory_Pools *mem_pools), ARGMOD(Fixed_Size_Pool *pool),
    ARGIN(void *to_add))
{
    ASSERT_ARGS(gc_ms_add_free_object)
    GC_MS_PObj_Wrapper *object = (GC_MS_PObj_Wrapper *)to_add;

    PObj_flags_SETTO(object, PObj_on_free_list_FLAG);

    object->next_ptr = pool->free_list;
    pool->free_list  = object;
}

/*

=item C<static void * gc_ms_get_free_object(PARROT_INTERP, Memory_Pools
*mem_pools, Fixed_Size_Pool *pool)>

Free object allocator for the MS garbage collector system. If there are no
free objects, call C<gc_ms_add_free_object> to either free them up with a
GC run, or allocate new objects. If there are objects available on the
free list, pop it off and return it.

=cut

*/

PARROT_CANNOT_RETURN_NULL
PARROT_WARN_UNUSED_RESULT
static void *
gc_ms_get_free_object(PARROT_INTERP, ARGIN(Memory_Pools *mem_pools), ARGMOD(Fixed_Size_Pool *pool))
{
    ASSERT_ARGS(gc_ms_get_free_object)
    PObj *ptr;
    PObj *free_list = (PObj *)pool->free_list;

#if GC_USE_LAZY_ALLOCATOR
    if (!free_list && !pool->newfree) {
        (*pool->more_objects)(interp, mem_pools, pool);
        free_list = (PObj *)pool->free_list;
    }

    if (free_list) {
        ptr             = free_list;
        pool->free_list = ((GC_MS_PObj_Wrapper *)ptr)->next_ptr;
    }
    else {
        Fixed_Size_Arena * const arena = pool->last_Arena;
        ptr           = (PObj *)pool->newfree;
        pool->newfree = (void *)((char *)pool->newfree + pool->object_size);
        arena->used++;

        if (pool->newfree >= pool->newlast)
            pool->newfree = NULL;

        PARROT_ASSERT(ptr < (PObj *)pool->newlast);
    }
#else
    /* if we don't have any objects */
    if (!free_list) {
        (*pool->more_objects)(interp, pool);
        free_list = (PObj *)pool->free_list;
    }

    ptr             = free_list;
    pool->free_list = ((GC_MS_PObj_Wrapper*)ptr)->next_ptr;
#endif

    /* PObj_flags_SETTO(ptr, 0); */
    memset(ptr, 0, pool->object_size);

    --pool->num_free_objects;

    return ptr;
}


/*

=item C<static void gc_ms_alloc_objects(PARROT_INTERP, Memory_Pools *mem_pools,
Fixed_Size_Pool *pool)>

New arena allocator function for the MS garbage collector system. Allocates
and initializes a new memory arena in the given pool. Adds all the new
objects to the pool's free list for later allocation.

=cut

*/

static void
gc_ms_alloc_objects(PARROT_INTERP, ARGIN(Memory_Pools *mem_pools), ARGMOD(Fixed_Size_Pool *pool))
{
    ASSERT_ARGS(gc_ms_alloc_objects)
    /* Setup memory for the new objects */

    Fixed_Size_Arena * const new_arena =
        mem_internal_allocate_typed(Fixed_Size_Arena);

    const size_t size = pool->object_size * pool->objects_per_alloc;
    size_t alloc_size;

    /* could be mem_internal_allocate too, but calloc is fast */
    new_arena->start_objects = mem_internal_allocate_zeroed(size);

    Parrot_append_arena_in_pool(interp, pool, new_arena, size);

    PARROT_ASSERT(pool->last_Arena);

    Parrot_add_to_free_list(interp, pool, new_arena);

    /* Allocate more next time */
    if (GC_DEBUG(interp)) {
        pool->objects_per_alloc *= GC_DEBUG_UNITS_PER_ALLOC_GROWTH_FACTOR;
        pool->replenish_level =
                (size_t)(pool->total_objects *
                GC_DEBUG_REPLENISH_LEVEL_FACTOR);
    }
    else {
        pool->objects_per_alloc = (size_t)(pool->objects_per_alloc *
            UNITS_PER_ALLOC_GROWTH_FACTOR);
        pool->replenish_level   =
                (size_t)(pool->total_objects * REPLENISH_LEVEL_FACTOR);
    }

    /* check alloc size against maximum */
    alloc_size = pool->object_size * pool->objects_per_alloc;

    if (alloc_size > POOL_MAX_BYTES)
        pool->objects_per_alloc = POOL_MAX_BYTES / pool->object_size;
}

/*

=item C<static void Parrot_gc_free_attributes_from_pool(PARROT_INTERP,
PMC_Attribute_Pool *pool, void *data)>

Frees a fixed-size data item back to the pool for later reallocation.  Private
to this file.

*/

static void
Parrot_gc_free_attributes_from_pool(PARROT_INTERP,
    ARGMOD(PMC_Attribute_Pool *pool),
    ARGMOD(void *data))
{
    ASSERT_ARGS(Parrot_gc_free_attributes_from_pool)
    PMC_Attribute_Free_List * const item = (PMC_Attribute_Free_List *)data;

    item->next      = pool->free_list;
    pool->free_list = item;

    pool->num_free_objects++;
}

/*

=back

=cut

*/

/*
 * Local variables:
 *   c-file-style: "parrot"
 * End:
 * vim: expandtab shiftwidth=4:
 */
