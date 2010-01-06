/*
Copyright (C) 2001-2009, Parrot Foundation.
$Id$

=head1 NAME

src/gc/alloc_resources.c - Allocate and deallocate buffer resources such as
STRINGS.

=head1 DESCRIPTION

Functions to manage non-PObj memory, including strings and buffers.

=head2 Parrot Memory Management Code

=over 4

=cut

*/

#include "parrot/parrot.h"
#include "gc_private.h"


#define RECLAMATION_FACTOR 0.20
#define WE_WANT_EVER_GROWING_ALLOCATIONS 0

/* show allocated blocks on stderr */
#define RESOURCE_DEBUG 0
#define RESOURCE_DEBUG_SIZE 1000000

#define POOL_SIZE 65536 * 2

typedef void (*compact_f) (Interp *, Variable_Size_Pool *);

/* HEADERIZER HFILE: src/gc/gc_private.h */

/* HEADERIZER BEGIN: static */
/* Don't modify between HEADERIZER BEGIN / HEADERIZER END.  Your changes will be lost. */

static void alloc_new_block(PARROT_INTERP,
    size_t size,
    ARGMOD(Variable_Size_Pool *pool),
    ARGIN(const char *why))
        __attribute__nonnull__(1)
        __attribute__nonnull__(3)
        __attribute__nonnull__(4)
        FUNC_MODIFIES(*pool);

PARROT_CANNOT_RETURN_NULL
PARROT_WARN_UNUSED_RESULT
static const char* buffer_location(PARROT_INTERP, ARGIN(const PObj *b))
        __attribute__nonnull__(1)
        __attribute__nonnull__(2);

static void check_fixed_size_obj_pool(ARGMOD(Fixed_Size_Pool * pool))
        __attribute__nonnull__(1)
        FUNC_MODIFIES(* pool);

static void check_memory_system(PARROT_INTERP)
        __attribute__nonnull__(1);

static void check_var_size_obj_pool(ARGMOD(Variable_Size_Pool *pool))
        __attribute__nonnull__(1)
        FUNC_MODIFIES(*pool);

static void debug_print_buf(PARROT_INTERP, ARGIN(const Buffer *b))
        __attribute__nonnull__(1)
        __attribute__nonnull__(2);

PARROT_MALLOC
PARROT_CANNOT_RETURN_NULL
static Variable_Size_Pool * new_memory_pool(
    size_t min_block,
    NULLOK(compact_f compact));

#define ASSERT_ARGS_alloc_new_block __attribute__unused__ int _ASSERT_ARGS_CHECK = (\
       PARROT_ASSERT_ARG(interp) \
    , PARROT_ASSERT_ARG(pool) \
    , PARROT_ASSERT_ARG(why))
#define ASSERT_ARGS_buffer_location __attribute__unused__ int _ASSERT_ARGS_CHECK = (\
       PARROT_ASSERT_ARG(interp) \
    , PARROT_ASSERT_ARG(b))
#define ASSERT_ARGS_check_fixed_size_obj_pool __attribute__unused__ int _ASSERT_ARGS_CHECK = (\
       PARROT_ASSERT_ARG(pool))
#define ASSERT_ARGS_check_memory_system __attribute__unused__ int _ASSERT_ARGS_CHECK = (\
       PARROT_ASSERT_ARG(interp))
#define ASSERT_ARGS_check_var_size_obj_pool __attribute__unused__ int _ASSERT_ARGS_CHECK = (\
       PARROT_ASSERT_ARG(pool))
#define ASSERT_ARGS_debug_print_buf __attribute__unused__ int _ASSERT_ARGS_CHECK = (\
       PARROT_ASSERT_ARG(interp) \
    , PARROT_ASSERT_ARG(b))
#define ASSERT_ARGS_new_memory_pool __attribute__unused__ int _ASSERT_ARGS_CHECK = (0)
/* Don't modify between HEADERIZER BEGIN / HEADERIZER END.  Your changes will be lost. */
/* HEADERIZER END: static */


/*

=item C<static void alloc_new_block(PARROT_INTERP, size_t size,
Variable_Size_Pool *pool, const char *why)>

Allocate a new memory block. We allocate either the requested size or the
default size, whichever is larger. Add the new block to the given memory
pool. The given C<char *why> text is used for debugging.

=cut

*/

static void
alloc_new_block(PARROT_INTERP, size_t size, ARGMOD(Variable_Size_Pool *pool),
        ARGIN(const char *why))
{
    ASSERT_ARGS(alloc_new_block)
    Memory_Block *new_block;

    const size_t alloc_size = (size > pool->minimum_block_size)
            ? size : pool->minimum_block_size;

#if RESOURCE_DEBUG
    fprintf(stderr, "new_block (%s) size %u -> %u\n",
        why, size, alloc_size);
#else
    UNUSED(why)
#endif

    /* Allocate a new block. Header info's on the front */
    new_block = (Memory_Block *)mem_internal_allocate_zeroed(
        sizeof (Memory_Block) + alloc_size);

    if (!new_block) {
        fprintf(stderr, "out of mem allocsize = %d\n", (int)alloc_size);
        exit(EXIT_FAILURE);
    }

    new_block->free  = alloc_size;
    new_block->size  = alloc_size;

    new_block->next  = NULL;
    new_block->start = (char *)new_block + sizeof (Memory_Block);
    new_block->top   = new_block->start;

    /* Note that we've allocated it */
    interp->mem_pools->memory_allocated += alloc_size;

    /* If this is for a public pool, add it to the list */
    new_block->prev = pool->top_block;

    /* If we're not first, then tack us on the list */
    if (pool->top_block)
        pool->top_block->next = new_block;

    pool->top_block        = new_block;
    pool->total_allocated += alloc_size;
}

/*

=item C<void * mem_allocate(PARROT_INTERP, size_t size, Variable_Size_Pool
*pool)>

Allocates memory for headers.

Alignment problems history:

See L<http://archive.develooper.com/perl6-internals%40perl.org/msg12310.html>
for details.

- return aligned pointer *if needed*
- return strings et al at unaligned i.e. void* boundaries
- remember alignment in a buffer header bit
  use this in compaction code
- reduce alignment to a reasonable value i.e. MALLOC_ALIGNMENT
  aka 2*sizeof (size_t) or just 8 (TODO make a config hint)

Buffer memory layout:

                    +-----------------+
                    |  ref_count   |f |    # GC header
  obj->bufstart  -> +-----------------+
                    |  data           |
                    v                 v

 * if PObj_is_COWable is set, then we have
   - a ref_count, {inc, dec}remented by 2 always
   - the lo bit 'f' means 'is being forwarded" - what TAIL_flag was

 * if PObj_align_FLAG is set, obj->bufstart is aligned like discussed above
 * obj->buflen is the usable length excluding the optional GC part.

=cut

*/

PARROT_MALLOC
PARROT_CANNOT_RETURN_NULL
void *
mem_allocate(PARROT_INTERP, size_t size, ARGMOD(Variable_Size_Pool *pool))
{
    ASSERT_ARGS(mem_allocate)
    void *return_val;

    /* we always should have one block at least */
    PARROT_ASSERT(pool->top_block);

    /* If not enough room, try to find some */
    if (pool->top_block->free < size) {
        /*
         * force a GC mark run to get live flags set
         * for incremental M&S collection is run from there
         * but only if there may be something worth collecting!
         * TODO pass required allocation size to the GC system,
         *      so that collection can be skipped if needed
         */
        if (!interp->mem_pools->gc_mark_block_level
        &&   interp->mem_pools->mem_allocs_since_last_collect) {
            Parrot_gc_mark_and_sweep(interp, GC_trace_stack_FLAG);

            if (interp->gc_sys->sys_type != INF) {
                /* Compact the pool if allowed and worthwhile */
                if (pool->compact) {
                    /* don't bother reclaiming if it's only a small amount */
                    if ((pool->possibly_reclaimable * pool->reclaim_factor +
                         pool->guaranteed_reclaimable) > size) {
                        (*pool->compact) (interp, pool);
                    }
                }
            }
        }
        if (pool->top_block->free < size) {
            if (pool->minimum_block_size < 65536 * 16)
                pool->minimum_block_size *= 2;
            /*
             * TODO - Big blocks
             *
             * Mark the block as big block (it has just one item)
             * And don't set big blocks as the top_block.
             */
            alloc_new_block(interp, size, pool, "compact failed");

            interp->mem_pools->mem_allocs_since_last_collect++;

            if (pool->top_block->free < size) {
                fprintf(stderr, "out of mem\n");
                exit(EXIT_FAILURE);
            }
        }
    }

    /* TODO inline the fast path */
    return_val             = pool->top_block->top;
    pool->top_block->top  += size;
    pool->top_block->free -= size;

    return return_val;
}

/*

=item C<static const char* buffer_location(PARROT_INTERP, const PObj *b)>

Recturns a constant string representing the location of the given
PObj C<b> in one of the PMC registers. If the PMC is not located
in one of the PMC registers of the current context, returns the
string C<"???">.

=cut

*/

#if RESOURCE_DEBUG
PARROT_CANNOT_RETURN_NULL
PARROT_WARN_UNUSED_RESULT
static const char*
buffer_location(PARROT_INTERP, ARGIN(const PObj *b))
{
    ASSERT_ARGS(buffer_location)
    int i;
    static char reg[10];

    Parrot_Context* const ctx = CONTEXT(interp);

    for (i = 0; i < ctx->n_regs_used[REGNO_STR]; ++i) {
        PObj * const obj = (PObj *) CTX_REG_STR(interp, ctx, i);
        if (obj == b) {
            sprintf(reg, "S%d", i);
            return reg;
        }
    }

    return "???";
}

/*

=item C<static void debug_print_buf(PARROT_INTERP, const Buffer *b)>

Prints a debug statement with information about the given PObj C<b>.
=cut

*/

static void
debug_print_buf(PARROT_INTERP, ARGIN(const Buffer *b))
{
    ASSERT_ARGS(debug_print_buf)
    fprintf(stderr, "found %p, len %d, flags 0x%08x at %s\n",
            b, (int)Buffer_buflen(b), (uint)PObj_get_FLAGS(b),
            buffer_location(interp, b));
}
#endif

/*

=back

=head2 Compaction Code

=over 4

=item C<void compact_pool(PARROT_INTERP, Variable_Size_Pool *pool)>

Compact the string buffer pool. Does not perform a GC scan, or mark items
as being alive in any way.

=cut

*/

void
compact_pool(PARROT_INTERP, ARGMOD(Variable_Size_Pool *pool))
{
    ASSERT_ARGS(compact_pool)
    INTVAL        j;
    UINTVAL       total_size;

    Memory_Block *new_block;     /* A pointer to our working block */
    char         *cur_spot;      /* Where we're currently copying to */

    Fixed_Size_Arena *cur_buffer_arena;
    Memory_Pools * const      mem_pools = interp->mem_pools;

    /* Bail if we're blocked */
    if (mem_pools->gc_sweep_block_level)
        return;

    ++mem_pools->gc_sweep_block_level;

    /* We're collecting */
    mem_pools->mem_allocs_since_last_collect    = 0;
    mem_pools->header_allocs_since_last_collect = 0;
    mem_pools->gc_collect_runs++;

    /* total - reclaimable == currently used. Add a minimum block to the
     * current amount, so we can avoid having to allocate it in the future. */
    {
        Memory_Block *cur_block = pool->top_block;

        total_size = 0;

        while (cur_block) {
            /*
             * TODO - Big blocks
             *
             * Currently all available blocks are compacted into on new
             * block with total_size. This is more than suboptimal, if
             * the block has just one live item from a big allocation.
             *
             * But currently it's unknown if the buffer memory is alive
             * as the live bits are in Buffer headers. We have to run the
             * compaction loop below to check liveness. OTOH if this
             * compaction is running through all the buffer headers, there
             * is no relation to the block.
             *
             *
             * Moving the life bit into the buffer thus also solves this
             * problem easily.
             */
            total_size += cur_block->size - cur_block->free;
            cur_block   = cur_block->prev;
        }
    }
    /*
     * XXX for some reason the guarantee isn't correct
     *     TODO check why
     */

    /* total_size -= pool->guaranteed_reclaimable; */

    /* this makes for ever increasing allocations but fewer collect runs */
#if WE_WANT_EVER_GROWING_ALLOCATIONS
    total_size += pool->minimum_block_size;
#endif

    /* Snag a block big enough for everything */
    alloc_new_block(interp, total_size, pool, "inside compact");

    new_block = pool->top_block;

    /* Start at the beginning */
    cur_spot  = new_block->start;

    /* Run through all the Buffer header pools and copy */
    for (j = (INTVAL)mem_pools->num_sized - 1; j >= 0; --j) {
        Fixed_Size_Pool * const header_pool = mem_pools->sized_header_pools[j];
        UINTVAL       object_size;

        if (!header_pool)
            continue;

        object_size = header_pool->object_size;

        for (cur_buffer_arena = header_pool->last_Arena;
                cur_buffer_arena;
                cur_buffer_arena = cur_buffer_arena->prev) {
            Buffer *b = (Buffer *) cur_buffer_arena->start_objects;
            UINTVAL i;
            const size_t objects_end = cur_buffer_arena->used;

            for (i = objects_end; i; --i) {
                INTVAL *ref_count = NULL;

                /* ! (on_free_list | constant | external | sysmem) */
                if (Buffer_buflen(b) && PObj_is_movable_TESTALL(b)) {
                    ptrdiff_t offset = 0;
#if RESOURCE_DEBUG
                    if (Buffer_buflen(b) >= RESOURCE_DEBUG_SIZE)
                        debug_print_buf(interp, b);
#endif

                    /* we can't perform the math all the time, because
                     * strstart might be in unallocated memory */
                    if (PObj_is_COWable_TEST(b)) {
                        ref_count = Buffer_bufrefcountptr(b);

                        if (PObj_is_string_TEST(b)) {
                            offset = (ptrdiff_t)((STRING *)b)->strstart -
                                (ptrdiff_t)Buffer_bufstart(b);
                        }
                    }

                    /* buffer has already been moved; just change the header */
                    if (PObj_COW_TEST(b) &&
                        (ref_count && *ref_count & Buffer_moved_FLAG)) {
                        /* Find out who else references our data */
                        Buffer * const hdr = *((Buffer **)Buffer_bufstart(b));


                        PARROT_ASSERT(PObj_is_COWable_TEST(b));

                        /* Make sure they know that we own it too */
                        PObj_COW_SET(hdr);

                        /* TODO incr ref_count, after fixing string too
                         * Now make sure we point to where the other guy does */
                        Buffer_bufstart(b) = Buffer_bufstart(hdr);

                        /* And if we're a string, update strstart */
                        /* Somewhat of a hack, but if we get per-pool
                         * collections, it should help ease the pain */
                        if (PObj_is_string_TEST(b)) {
                            ((STRING *)b)->strstart = (char *)Buffer_bufstart(b) +
                                    offset;
                        }
                    }
                    else {
                        cur_spot = aligned_mem(b, cur_spot);

                        if (PObj_is_COWable_TEST(b)) {
                            INTVAL * const new_ref_count = ((INTVAL*) cur_spot) - 1;
                            *new_ref_count        = 2;
                        }

                        /* Copy our memory to the new pool */
                        memcpy(cur_spot, Buffer_bufstart(b), Buffer_buflen(b));

                        /* If we're COW */
                        if (PObj_COW_TEST(b)) {
                            PARROT_ASSERT(PObj_is_COWable_TEST(b));

                            /* Let the old buffer know how to find us */
                            *((Buffer **)Buffer_bufstart(b)) = b;

                            /* No guarantees that our data is still COW, so
                             * assume not, and let the above code fix-up */
                            PObj_COW_CLEAR(b);

                            /* Finally, let the tail know that we've moved, so
                             * that any other references can know to look for
                             * us and not re-copy */
                            if (ref_count)
                                *ref_count |= Buffer_moved_FLAG;
                        }

                        Buffer_bufstart(b) = cur_spot;

                        if (PObj_is_string_TEST(b)) {
                            ((STRING *)b)->strstart = (char *)Buffer_bufstart(b) +
                                    offset;
                        }

                        cur_spot += Buffer_buflen(b);
                    }
                }
                b = (Buffer *)((char *)b + object_size);
            }
        }
    }

    /* Okay, we're done with the copy. Set the bits in the pool struct */
    /* First, where we allocate next */
    new_block->top = cur_spot;

    PARROT_ASSERT(new_block->size >= (size_t)new_block->top -
            (size_t)new_block->start);

    /* How much is free. That's the total size minus the amount we used */
    new_block->free = new_block->size - (new_block->top - new_block->start);

    mem_pools->memory_collected += (new_block->top - new_block->start);

    /* Now we're done. We're already on the pool's free list, so let us be the
     * only one on the free list and free the rest */
    {
        Memory_Block *cur_block = new_block->prev;

        PARROT_ASSERT(new_block == pool->top_block);

        while (cur_block) {
            Memory_Block * const next_block = cur_block->prev;

            /* Note that we don't have it any more */
            mem_pools->memory_allocated -= cur_block->size;

            /* We know the pool body and pool header are a single chunk, so
             * this is enough to get rid of 'em both */
            mem_internal_free(cur_block);
            cur_block = next_block;
        }

        /* Set our new pool as the only pool */
        new_block->prev       = NULL;
        pool->total_allocated = total_size;
    }

    pool->guaranteed_reclaimable = 0;
    pool->possibly_reclaimable   = 0;

    --mem_pools->gc_sweep_block_level;
}

/*

=item C<size_t aligned_size(const Buffer *buffer, size_t len)>

Determines the size of Buffer C<buffer> which has nominal length C<len>.
The actual size in RAM of the Buffer might be different because of
alignment issues.

=cut

*/

PARROT_PURE_FUNCTION
PARROT_WARN_UNUSED_RESULT
size_t
aligned_size(ARGIN(const Buffer *buffer), size_t len)
{
    ASSERT_ARGS(aligned_size)
    if (PObj_is_COWable_TEST(buffer))
        len += sizeof (void*);
    if (PObj_aligned_TEST(buffer))
        len = (len + BUFFER_ALIGN_1) & BUFFER_ALIGN_MASK;
    else
        len = (len + WORD_ALIGN_1) & WORD_ALIGN_MASK;
    return len;
}

/*

=item C<char * aligned_mem(const Buffer *buffer, char *mem)>

Returns a pointer to the aligned allocated storage for Buffer C<buffer>,
which might not be the same as the pointer to C<buffeR> because of
memory alignment.

=cut

*/

PARROT_CANNOT_RETURN_NULL
PARROT_WARN_UNUSED_RESULT
char *
aligned_mem(ARGIN(const Buffer *buffer), ARGIN(char *mem))
{
    ASSERT_ARGS(aligned_mem)
    if (PObj_is_COWable_TEST(buffer))
        mem += sizeof (void*);
    if (PObj_aligned_TEST(buffer))
        mem = (char*)(((unsigned long)(mem + BUFFER_ALIGN_1)) &
                BUFFER_ALIGN_MASK);
    else
        mem = (char*)(((unsigned long)(mem + WORD_ALIGN_1)) & WORD_ALIGN_MASK);

    return mem;
}

/*

=item C<size_t aligned_string_size(size_t len)>

Determines the size of a string of length C<len> in RAM, accounting for
alignment.

=cut

*/

PARROT_CONST_FUNCTION
PARROT_WARN_UNUSED_RESULT
size_t
aligned_string_size(size_t len)
{
    ASSERT_ARGS(aligned_string_size)
    len += sizeof (void*);
    len = (len + WORD_ALIGN_1) & WORD_ALIGN_MASK;
    return len;
}

/*

=back

=head2 Parrot Re/Allocate Code

=over 4

=item C<static Variable_Size_Pool * new_memory_pool(size_t min_block, compact_f
compact)>

Allocate a new C<Variable_Size_Pool> structures, and set some initial values.
return a pointer to the new pool.

=cut

*/

PARROT_MALLOC
PARROT_CANNOT_RETURN_NULL
static Variable_Size_Pool *
new_memory_pool(size_t min_block, NULLOK(compact_f compact))
{
    ASSERT_ARGS(new_memory_pool)
    Variable_Size_Pool * const pool = mem_internal_allocate_typed(Variable_Size_Pool);

    pool->top_block              = NULL;
    pool->compact                = compact;
    pool->minimum_block_size     = min_block;
    pool->total_allocated        = 0;
    pool->guaranteed_reclaimable = 0;
    pool->possibly_reclaimable   = 0;
    pool->reclaim_factor         = RECLAMATION_FACTOR;

    return pool;
}

/*

=item C<void initialize_var_size_pools(PARROT_INTERP)>

Initialize the managed memory pools. Parrot maintains two C<Variable_Size_Pool>
structures, the general memory pool and the constant string pool. Create
and initialize both pool structures, and allocate initial blocks of memory
for both.

=cut

*/

void
initialize_var_size_pools(PARROT_INTERP)
{
    ASSERT_ARGS(initialize_var_size_pools)
    Memory_Pools * const mem_pools = interp->mem_pools;

    mem_pools->memory_pool   = new_memory_pool(POOL_SIZE, &compact_pool);
    alloc_new_block(interp, POOL_SIZE, mem_pools->memory_pool, "init");

    /* Constant strings - not compacted */
    mem_pools->constant_string_pool = new_memory_pool(POOL_SIZE, NULL);
    alloc_new_block(interp, POOL_SIZE, mem_pools->constant_string_pool, "init");
}


/*

=item C<void merge_pools(Variable_Size_Pool *dest, Variable_Size_Pool *source)>

Merge two memory pools together. Do this by moving all memory blocks
from the C<*source> pool into the C<*dest> pool. The C<source> pool
is emptied, but is not destroyed here.

=cut

*/

void
merge_pools(ARGMOD(Variable_Size_Pool *dest), ARGMOD(Variable_Size_Pool *source))
{
    ASSERT_ARGS(merge_pools)
    Memory_Block *cur_block;

    cur_block = source->top_block;

    while (cur_block) {
        Memory_Block * const next_block = cur_block->prev;

        if (cur_block->free == cur_block->size)
            mem_internal_free(cur_block);
        else {
            cur_block->next        = NULL;
            cur_block->prev        = dest->top_block;

            dest->top_block        = cur_block;
            dest->total_allocated += cur_block->size;
        }
        cur_block = next_block;
    }

    dest->guaranteed_reclaimable += source->guaranteed_reclaimable;
    dest->possibly_reclaimable   += source->possibly_reclaimable;

    source->top_block              = NULL;
    source->total_allocated        = 0;
    source->possibly_reclaimable   = 0;
    source->guaranteed_reclaimable = 0;
}

/*

=item C<static void check_memory_system(PARROT_INTERP)>

Checks the memory system of parrot on any corruptions, including
the string system.

=cut

*/

static void
check_memory_system(PARROT_INTERP)
{
    ASSERT_ARGS(check_memory_system)
    size_t i;
    Memory_Pools * const mem_pools = interp->mem_pools;

    check_var_size_obj_pool(mem_pools->memory_pool);
    check_var_size_obj_pool(mem_pools->constant_string_pool);
    check_fixed_size_obj_pool(mem_pools->pmc_pool);
    check_fixed_size_obj_pool(mem_pools->constant_pmc_pool);
    check_fixed_size_obj_pool(mem_pools->string_header_pool);
    check_fixed_size_obj_pool(mem_pools->constant_string_header_pool);

    for (i = 0; i < mem_pools->num_sized; i++) {
        Fixed_Size_Pool * pool = mem_pools->sized_header_pools[i];
        if (pool != NULL && pool != mem_pools->string_header_pool)
            check_fixed_size_obj_pool(pool);
    }
}

/*

=item C<static void check_fixed_size_obj_pool(Fixed_Size_Pool * pool)>

Checks a small object pool, if it contains buffer it checks the buffers also.

=cut

*/

static void
check_fixed_size_obj_pool(ARGMOD(Fixed_Size_Pool * pool))
{
    ASSERT_ARGS(check_fixed_size_obj_pool)
    size_t total_objects;
    size_t last_free_list_count;
    Fixed_Size_Arena * arena_walker;
    size_t free_objects;
    PObj * object;
    size_t i;
    size_t count;
    GC_MS_PObj_Wrapper * pobj_walker;

    count = 10000000; /*detect unendless loop just use big enough number*/

    total_objects = pool->total_objects;
    last_free_list_count = 1;
    free_objects = 0;

    arena_walker = pool->last_Arena;
    while (arena_walker != NULL) {
        total_objects -= arena_walker->total_objects;
        object = (PObj*)arena_walker->start_objects;
        for (i = 0; i < arena_walker->total_objects; ++i) {
            if (PObj_on_free_list_TEST(object)) {
                ++free_objects;
                pobj_walker = (GC_MS_PObj_Wrapper*)object;
                if (pobj_walker->next_ptr == NULL)
                    /* should happen only once at the end */
                    --last_free_list_count;
                else {
                    /* next item on free list should also be flaged as free item */
                    pobj_walker = (GC_MS_PObj_Wrapper*)pobj_walker->next_ptr;
                    PARROT_ASSERT(PObj_on_free_list_TEST((PObj*)pobj_walker));
                }
            }
            else if (pool->mem_pool != NULL) {
                /*then it means we are a buffer*/
                check_buffer_ptr((Buffer*)object, pool->mem_pool);
            }
            object = (PObj*)((char *)object + pool->object_size);
            PARROT_ASSERT(--count);
        }
        /*check the list*/
        if (arena_walker->prev != NULL)
            PARROT_ASSERT(arena_walker->prev->next == arena_walker);
        arena_walker = arena_walker->prev;
        PARROT_ASSERT(--count);
    }

    count = 10000000;

    PARROT_ASSERT(free_objects == pool->num_free_objects);

    pobj_walker = (GC_MS_PObj_Wrapper*)pool->free_list;
    while (pobj_walker != NULL) {
        PARROT_ASSERT(pool->start_arena_memory <= (size_t)pobj_walker);
        PARROT_ASSERT(pool->end_arena_memory > (size_t)pobj_walker);
        PARROT_ASSERT(PObj_on_free_list_TEST((PObj*)pobj_walker));
        --free_objects;
        pobj_walker = (GC_MS_PObj_Wrapper*)pobj_walker->next_ptr;
        PARROT_ASSERT(--count);
    }

    PARROT_ASSERT(total_objects == 0);
    PARROT_ASSERT(last_free_list_count == 0 || pool->num_free_objects == 0);
    PARROT_ASSERT(free_objects == 0);
}

/*

=item C<static void check_var_size_obj_pool(Variable_Size_Pool *pool)>

Checks a memory pool, containing buffer data

=cut

*/

static void
check_var_size_obj_pool(ARGMOD(Variable_Size_Pool *pool))
{
    ASSERT_ARGS(check_var_size_obj_pool)
    size_t count;
    Memory_Block * block_walker;
    count = 10000000; /*detect unendless loop just use big enough number*/

    block_walker = (Memory_Block *)pool->top_block;
    while (block_walker != NULL) {
        PARROT_ASSERT(block_walker->start == (char *)block_walker +
            sizeof (Memory_Block));
        PARROT_ASSERT((size_t)(block_walker->top -
            block_walker->start) == block_walker->size - block_walker->free);

        /*check the list*/
        if (block_walker->prev != NULL)
            PARROT_ASSERT(block_walker->prev->next == block_walker);
        block_walker = block_walker->prev;
        PARROT_ASSERT(--count);
    }
}

/*

=item C<void check_buffer_ptr(Buffer * pobj, Variable_Size_Pool * pool)>

Checks wether the buffer is within the bounds of the memory pool

=cut

*/

void
check_buffer_ptr(ARGMOD(Buffer * pobj), ARGMOD(Variable_Size_Pool * pool))
{
    ASSERT_ARGS(check_buffer_ptr)
    Memory_Block * cur_block = pool->top_block;
    char * bufstart;

    bufstart = (char*)Buffer_bufstart(pobj);

    if (bufstart == NULL && Buffer_buflen(pobj) == 0)
        return;

    if (PObj_external_TEST(pobj) || PObj_sysmem_TEST(pobj)) {
        /*buffer does not come from the memory pool*/
        if (PObj_is_string_TEST(pobj)) {
            PARROT_ASSERT(((STRING *) pobj)->strstart >=
                (char *) Buffer_bufstart(pobj));
            PARROT_ASSERT(((STRING *) pobj)->strstart +
                ((STRING *) pobj)->strlen <=
                (char *) Buffer_bufstart(pobj) + Buffer_buflen(pobj));
        }
        return;
    }

    if (PObj_is_COWable_TEST(pobj))
        bufstart -= sizeof (void*);

    while (cur_block) {
        if ((char *)bufstart >= cur_block->start &&
            (char *)Buffer_bufstart(pobj) +
            Buffer_buflen(pobj) < cur_block->start + cur_block->size) {
            if (PObj_is_string_TEST(pobj)) {
                PARROT_ASSERT(((STRING *)pobj)->strstart >=
                    (char *)Buffer_bufstart(pobj));
                PARROT_ASSERT(((STRING *)pobj)->strstart +
                    ((STRING *)pobj)->strlen <= (char *)Buffer_bufstart(pobj) +
                    Buffer_buflen(pobj));
            }
            return;
        }
        cur_block = cur_block->prev;
    }
    PARROT_ASSERT(0);
}

/*

=back

=head1 SEE ALSO

F<src/gc/memory.c>.

=head1 HISTORY

Initial version by Dan on 2001.10.2.

=cut

*/

/*
 * Local variables:
 *   c-file-style: "parrot"
 * End:
 * vim: expandtab shiftwidth=4:
 */
