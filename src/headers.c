/*
Copyright (C) 2001-2003, The Perl Foundation.
$Id$

=head1 NAME

src/headers.c - Header management functions

=head1 DESCRIPTION

Handles getting of various headers, and pool creation.

=cut

*/

#include "parrot/parrot.h"
#include <assert.h>

#ifndef GC_IS_MALLOC
#  define PMC_HEADERS_PER_ALLOC 512
#  define BUFFER_HEADERS_PER_ALLOC 256
#  define STRING_HEADERS_PER_ALLOC 256
#else /* GC_IS_MALLOC */
#  define PMC_HEADERS_PER_ALLOC 512
#  define BUFFER_HEADERS_PER_ALLOC 512
#  define STRING_HEADERS_PER_ALLOC 512
#endif /* GC_IS_MALLOC */

#  define CONSTANT_PMC_HEADERS_PER_ALLOC 64

/*


=head2 Buffer Header Functions for small-object lookup table

=over 4

=item C<static void *
get_free_buffer(Interp *interp, Small_Object_Pool *pool)>

Gets a free C<Buffer> from C<pool> and returns it. Memory is cleared.

=cut

*/

static void *
get_free_buffer(Interp *interp, Small_Object_Pool *pool)
{
    PObj *buffer = (PObj *)pool->get_free_object(interp, pool);

    /* don't mess around with flags */
    PObj_bufstart(buffer) = NULL;
    PObj_buflen(buffer) = 0;

    if (pool->object_size  - GC_HEADER_SIZE > sizeof (PObj))
        memset(buffer + 1, 0,
                pool->object_size - sizeof (PObj) - GC_HEADER_SIZE);
    return buffer;
}

/*

=back

=head2 Header Pool Creation Functions

=over 4

=item C<Small_Object_Pool * new_pmc_pool(Interp *interp)>

Creates an new pool for PMCs and returns it.

=cut

*/

Small_Object_Pool * new_pmc_pool(Interp *interp)
{
    int num_headers = PMC_HEADERS_PER_ALLOC;
    Small_Object_Pool *pmc_pool =
        new_small_object_pool(interp, sizeof (PMC), num_headers);

    pmc_pool->mem_pool = NULL;
    (interp->arena_base->init_pool)(interp, pmc_pool);
    return pmc_pool;
}

/*

=item C<Small_Object_Pool *
new_bufferlike_pool(Interp *interp, size_t actual_buffer_size)>

Creates a new pool for buffer-like structures. Usually you would need
C<make_bufferlike_pool()>.

=cut

*/

Small_Object_Pool *
new_bufferlike_pool(Interp *interp,
        size_t actual_buffer_size)
{
    int num_headers = BUFFER_HEADERS_PER_ALLOC;
    size_t buffer_size =
            (actual_buffer_size + sizeof (void *) - 1) & ~(sizeof (void *) - 1);
    Small_Object_Pool *pool =
            new_small_object_pool(interp, buffer_size, num_headers);

    pool->mem_pool = interp->arena_base->memory_pool;
    (interp->arena_base->init_pool)(interp, pool);
    return pool;
}

/*

=item C<Small_Object_Pool *
new_buffer_pool(Interp *interp)>

Non-constant strings and plain Buffers are in the sized header pools.

=cut

*/

Small_Object_Pool *
new_buffer_pool(Interp *interp)
{
    return make_bufferlike_pool(interp, sizeof (Buffer));
}

/*

=item C<Small_Object_Pool *
new_string_pool(Interp *interp, INTVAL constant)>

Creates a new pool for C<STRINGS> and returns it.

=cut

*/

Small_Object_Pool *
new_string_pool(Interp *interp, INTVAL constant)
{
    Small_Object_Pool *pool;
    if (constant) {
        pool = new_bufferlike_pool(interp, sizeof (STRING));
        pool->mem_pool = interp->arena_base->constant_string_pool;
    }
    else
        pool = make_bufferlike_pool(interp, sizeof (STRING));
    pool->objects_per_alloc = STRING_HEADERS_PER_ALLOC;
    return pool;
}

/*

=item C<Small_Object_Pool *
make_bufferlike_pool(Interp *interp, size_t buffer_size)>

Make and return a bufferlike header pool.

=cut

*/

Small_Object_Pool *
make_bufferlike_pool(Interp *interp, size_t buffer_size)
{
    UINTVAL idx;
    UINTVAL num_old = interp->arena_base->num_sized;
    Small_Object_Pool **sized_pools =
            interp->arena_base->sized_header_pools;

    idx = (buffer_size - sizeof (Buffer)) / sizeof (void *);

    /* Expand the array of sized resource pools, if necessary */
    if (num_old <= idx) {
        UINTVAL num_new = idx + 1;
        sized_pools = (Small_Object_Pool **)mem_internal_realloc(sized_pools,
                                           num_new * sizeof (void *));
        memset(sized_pools + num_old, 0, sizeof (void *) * (num_new - num_old));

        interp->arena_base->sized_header_pools = sized_pools;
        interp->arena_base->num_sized = num_new;
    }

    if (sized_pools[idx] == NULL) {
        sized_pools[idx] = new_bufferlike_pool(interp, buffer_size);
    }

    return sized_pools[idx];
}

/*

=item C<Small_Object_Pool *
get_bufferlike_pool(Interp *interp, size_t buffer_size)>

Return a bufferlike header pool, it must exist.

=cut

*/

Small_Object_Pool *
get_bufferlike_pool(Interp *interp, size_t buffer_size)
{
    Small_Object_Pool **sized_pools =
            interp->arena_base->sized_header_pools;

    return sized_pools[ (buffer_size - sizeof (Buffer)) / sizeof (void *) ];
}

/*

=item C<PMC *
new_pmc_header(Interp *interp)>

Get a header.

=cut

*/

static PMC_EXT * new_pmc_ext(Parrot_Interp);

PMC *
new_pmc_header(Interp *interp, UINTVAL flags)
{
    Small_Object_Pool *pool;
    PMC *pmc;

    pool = flags & PObj_constant_FLAG ?
        interp->arena_base->constant_pmc_pool :
        interp->arena_base->pmc_pool;
    pmc = (PMC *)pool->get_free_object(interp, pool);
    /* clear flags, set is_PMC_FLAG */
    if (flags & PObj_is_PMC_EXT_FLAG) {
        flags |= PObj_is_special_PMC_FLAG;
        pmc->pmc_ext = new_pmc_ext(interp);
        if (flags & PObj_is_PMC_shared_FLAG) {
            add_pmc_sync(interp, pmc);
        }
    }
    else
        pmc->pmc_ext = NULL;
    PObj_get_FLAGS(pmc) |= PObj_is_PMC_FLAG|flags;
    pmc->vtable = NULL;
#if ! PMC_DATA_IN_EXT
    PMC_data(pmc) = NULL;
#endif
    return pmc;
}

/*

=item C<PMC_EXT *
new_pmc_ext(Interp *interp)>

Creates a new C<PMC_EXT> and returns it.

=cut

*/

static PMC_EXT *
new_pmc_ext(Interp *interp)
{
    Small_Object_Pool *pool = interp->arena_base->pmc_ext_pool;
    PMC_EXT *ptr;
    /*
     * can't use normal get_free_object--PMC_EXT doesn't have flags
     * it isn't a Buffer
     */
    if (!pool->free_list)
        (*pool->more_objects) (interp, pool);
    ptr = (PMC_EXT *)pool->free_list;
    pool->free_list = *(void **)ptr;
    memset(ptr, 0, sizeof (PMC_EXT));
    return ptr;
}

/*

=item C<void
add_pmc_ext(Interp *interp, PMC *pmc)>

Adds a new C<PMC_EXT> to C<pmc>.

=cut

*/

void
add_pmc_ext(Interp *interp, PMC *pmc)
{
    pmc->pmc_ext = new_pmc_ext(interp);
    PObj_is_PMC_EXT_SET(pmc);
#ifdef PARROT_GC_IMS
    /*
     * preserve DDD color: a simple PMC  live = black
     *                     an aggregate  live = grey
     * set'em black
     */
    if (PObj_live_TEST(pmc))
        PObj_get_FLAGS(pmc) |= PObj_custom_GC_FLAG;
#endif
}

/*

=item C<PMC *
add_pmc_sync(Interp *interp, PMC *pmc)>

Adds a PMC_sync field to C<pmc>.

=cut

*/

void
add_pmc_sync(Interp *interp, PMC *pmc)
{
    if (!PObj_is_PMC_EXT_TEST(pmc)) {
        add_pmc_ext(interp, pmc);
    }
    PMC_sync(pmc) = (struct _Sync *)mem_internal_allocate(sizeof (*PMC_sync(pmc)));
    PMC_sync(pmc)->owner = interp;
    MUTEX_INIT(PMC_sync(pmc)->pmc_lock);
}

/*

=item C<STRING *
new_string_header(Interp *interp, UINTVAL flags)>

Returns a new C<STRING> header.

=cut

*/

STRING *
new_string_header(Interp *interp, UINTVAL flags)
{
    STRING *string = (STRING *)get_free_buffer(interp, (flags & PObj_constant_FLAG)
            ? interp->arena_base->constant_string_header_pool :
            interp->arena_base->string_header_pool);
    PObj_get_FLAGS(string) |= flags | PObj_is_string_FLAG|PObj_is_COWable_FLAG;
    SET_NULL(string->strstart);
    return string;
}

/*

=item C<Buffer *
new_buffer_header(Interp *interp)>

Creates and returns a new C<Buffer>.

=cut

*/

Buffer *
new_buffer_header(Interp *interp)
{
    return (Buffer *)get_free_buffer(interp,
            interp->arena_base->buffer_header_pool);
}


/*

=item C<void *
new_bufferlike_header(Interp *interp, size_t size)>

Creates and returns a new buffer-like header.

=cut

*/

void *
new_bufferlike_header(Interp *interp, size_t size)
{
    Small_Object_Pool *pool;

    pool = get_bufferlike_pool(interp, size);

    return get_free_buffer(interp, pool);
}

/*

=item C<size_t
get_max_buffer_address(Interp *interp)>

Calculates the maximum buffer address and returns it.

=cut

*/

size_t
get_max_buffer_address(Interp *interp)
{
    UINTVAL i;
    size_t max = 0;
    Arenas *arena_base = interp->arena_base;

    for (i = 0; i < arena_base->num_sized; i++) {
        if (arena_base->sized_header_pools[i]) {
            if (arena_base-> sized_header_pools[i]->end_arena_memory > max)
                max = arena_base->sized_header_pools[i]->end_arena_memory;
        }
    }

    return max;
}

/*

=item C<size_t
get_min_buffer_address(Interp *interp)>

Calculates the minimum buffer address and returns it.

=cut

*/

size_t
get_min_buffer_address(Interp *interp)
{
    UINTVAL i;
    Arenas *arena_base = interp->arena_base;
    size_t min = (size_t) -1;

    for (i = 0; i < arena_base->num_sized; i++) {
        if (arena_base->sized_header_pools[i] &&
            arena_base->sized_header_pools[i]->start_arena_memory) {
            if (arena_base->sized_header_pools[i]->start_arena_memory < min)
                min = arena_base->sized_header_pools[i]->start_arena_memory;
        }
    }
    return min;
}

/*

=item C<size_t
get_max_pmc_address(Interp *interp)>

Calculates the maximum PMC address and returns it.

=cut

*/

size_t
get_max_pmc_address(Interp *interp)
{
    return interp->arena_base->pmc_pool->end_arena_memory;
}

/*

=item C<size_t
get_min_pmc_address(Interp *interp)>

Calculates the maximum PMC address and returns it.

=cut

*/

size_t
get_min_pmc_address(Interp *interp)
{
    return interp->arena_base->pmc_pool->start_arena_memory;
}

/*

=item C<int
is_buffer_ptr(Interp *interp, void *ptr)>

Checks that C<ptr> is actually a C<Buffer>.

=cut

*/

int
is_buffer_ptr(Interp *interp, void *ptr)
{
    UINTVAL i;
    Arenas *arena_base = interp->arena_base;;

    for (i = 0; i < arena_base->num_sized; i++) {
        if (arena_base->sized_header_pools[i] &&
                contained_in_pool(interp,
                        arena_base->sized_header_pools[i], ptr))
            return 1;
    }

    return 0;
}

/*

=item C<int
is_pmc_ptr(Interp *interp, void *ptr)>

Checks that C<ptr> is actually a PMC.

=cut

*/

int
is_pmc_ptr(Interp *interp, void *ptr)
{
    return contained_in_pool(interp,
            interp->arena_base->pmc_pool, ptr);
}


/*

=item C<void
Parrot_initialize_header_pools(Interp *interp)>

Initialize the pools for the tracked resources.

=cut

*/

void
Parrot_initialize_header_pools(Interp *interp)
{
    Arenas *arena_base;

    arena_base = interp->arena_base;
    /* Init the constant string header pool */
    arena_base->constant_string_header_pool = new_string_pool(interp, 1);
    arena_base->constant_string_header_pool->name = "constant_string_header";

    /* Init the buffer header pool
     *
     * note: the buffer_header_pool and the string_header_pool are actually
     * living in the sized_header_pools, this pool pointers are only
     * here for faster access in new_*_header
     */
    arena_base->buffer_header_pool = new_buffer_pool(interp);
    arena_base->buffer_header_pool->name = "buffer_header";

    /* Init the string header pool */
    arena_base->string_header_pool = new_string_pool(interp, 0);
    arena_base->string_header_pool->name = "string_header";

    /* Init the PMC header pool */
    arena_base->pmc_pool = new_pmc_pool(interp);
    arena_base->pmc_pool->name = "pmc";

    /* pmc extension buffer */
    arena_base->pmc_ext_pool =
        new_small_object_pool(interp, sizeof (struct PMC_EXT), 1024);
    /*
     * pmc_ext isn't a managed item. If a PMC has a pmc_ext structure
     * it is returned to the pool instantly - the structure is never
     * marked.
     * Use GS MS pool functions
     */
    gc_pmc_ext_pool_init(interp, arena_base->pmc_ext_pool);
    arena_base->pmc_ext_pool->name = "pmc_ext";

    /* constant PMCs */
    arena_base->constant_pmc_pool = new_pmc_pool(interp);
    arena_base->constant_pmc_pool->name = "constant_pmc";
    arena_base->constant_pmc_pool->objects_per_alloc =
       CONSTANT_PMC_HEADERS_PER_ALLOC;
}

/*

=item C<int Parrot_forall_header_pools(Interp *, int flag,
                                       void *arg, pool_iter_fn)>

Iterate through all header pools by calling the passed function. Returns
zero if the iteration didn't stop or the returned value.

=over 4

=item flag is one of

  POOL_PMC
  POOL_BUFFER
  POOL_CONST
  POOL_ALL

Only matching pools will be used.

=item arg

This argument is passed on to the iteration function.

=item pool_iter_fn

It is called with C<Interp*, Small_Object_Pool *, int flag, void *arg)>
If the function returns a non-zero value iteration will stop.

=back


=cut

*/

int
Parrot_forall_header_pools(Interp *interp, int flag, void *arg,
        pool_iter_fn func)
{
    Small_Object_Pool *pool;
    Arenas *arena_base;
    int ret_val, i;

    arena_base = interp->arena_base;

    if ((flag & (POOL_PMC | POOL_CONST)) == (POOL_PMC | POOL_CONST)) {
        ret_val = (func)(interp, arena_base->constant_pmc_pool,
                POOL_PMC | POOL_CONST, arg);
        if (ret_val)
            return ret_val;
    }
    if (flag & POOL_PMC) {
        ret_val = (func)(interp, arena_base->pmc_pool, POOL_PMC, arg);
        if (ret_val)
            return ret_val;
    }
    if ((flag & (POOL_BUFFER | POOL_CONST)) == (POOL_BUFFER | POOL_CONST)) {
        ret_val = (func)(interp, arena_base->constant_string_header_pool,
                POOL_BUFFER | POOL_CONST, arg);
        if (ret_val)
            return ret_val;
    }
    if (!(flag & POOL_BUFFER))
        return 0;
    for (i = 0; i < (INTVAL)interp->arena_base->num_sized; i++) {
        pool = arena_base->sized_header_pools[i];
        if (!pool)
            continue;
        ret_val = (func)(interp, pool, POOL_BUFFER, arg);
        if (ret_val)
            return ret_val;
    }
    return 0;
}

/*

=item C<void
Parrot_destroy_header_pools(Interp *interp)>

Destroys the header pools.

=cut

*/

static void
free_pool(Interp *interp, Small_Object_Pool *pool)
{
    Small_Object_Arena *cur_arena, *next;
    for (cur_arena = pool->last_Arena; cur_arena;) {
        next = cur_arena->prev;
        mem_internal_free(cur_arena->start_objects);
        mem_internal_free(cur_arena);
        cur_arena = next;
    }
    mem_internal_free(pool);
}

static int
sweep_cb_buf(Interp *interp, Small_Object_Pool *pool, int flag,
        void *arg)
{
    int pass = (int)(INTVAL)arg;

#ifdef GC_IS_MALLOC
    if (pass == 0)
        clear_cow(interp, pool, 1);
    else if (pass == 1)
        used_cow(interp, pool, 1);
    else
#endif
    {
        Parrot_dod_sweep(interp, pool);
        free_pool(interp, pool);
    }
    return 0;

}

static int
sweep_cb_pmc(Interp *interp, Small_Object_Pool *pool, int flag,
        void *arg)
{
    Parrot_dod_sweep(interp, pool);
    free_pool(interp, pool);
    return 0;
}

void
Parrot_destroy_header_pools(Interp *interp)
{
    INTVAL pass, start;

    /* const/non const COW strings life in different pools
     * so in first pass
     * COW refcount is done, in 2. refcounting
     * in 3rd freeing
     */
#ifdef GC_IS_MALLOC
    start = 0;
#else
    start = 2;
#endif
    Parrot_forall_header_pools(interp, POOL_PMC | POOL_CONST, 0,
            sweep_cb_pmc);

    for (pass = start; pass <= 2; pass++) {
        Parrot_forall_header_pools(interp, POOL_BUFFER | POOL_CONST,
                (void *)pass, sweep_cb_buf);

    }
    free_pool(interp, interp->arena_base->pmc_ext_pool);
    mem_internal_free(interp->arena_base->sized_header_pools);
}

/*

=item C<void
Parrot_merge_header_pools(Interp *dest_interp, Interp *source_interp)>

Merge the header pools of C<source_interp> into those of C<dest_interp>.
(Used to deal with shared objects left after interpreter destruction.)

=cut

*/

static void fix_pmc_syncs(Interp *dest_interp, Small_Object_Pool *pool) {
    /* XXX largely copied from dod_sweep */
    Small_Object_Arena *cur_arena;
    UINTVAL object_size = pool->object_size;
    size_t i;
    size_t nm;
    for (cur_arena = pool->last_Arena;
            NULL != cur_arena; cur_arena = cur_arena->prev) {
        Buffer *b = (Buffer *)cur_arena->start_objects;

        for (i = nm = 0; i < cur_arena->used; i++) {
            if (PObj_on_free_list_TEST(b))
                ; /* if it's on free list, do nothing */
            else {
                if (PObj_is_PMC_TEST(b)) {
                    PMC *p = (PMC *)b;
                    if (PObj_is_PMC_shared_TEST(p)) {
                        PMC_sync(p)->owner = dest_interp;
                    }
                    else {
                        /* fprintf(stderr, "BAD PMC: address=%p,
                                   base_type=%d\n",
                                   p, p->vtable->base_type); */
                        assert(0);
                    }
                }
            }

            b = (Buffer *)((char *)b + object_size);
        }
    }
}

void
Parrot_merge_header_pools(Interp *dest_interp, Interp *source_interp) {
    Arenas *dest_arena;
    Arenas *source_arena;
    UINTVAL i;

    dest_arena = dest_interp->arena_base;
    source_arena = source_interp->arena_base;

    /* heavily borrowed from forall_header_pools */

    fix_pmc_syncs(dest_interp, source_arena->constant_pmc_pool);
    Parrot_small_object_pool_merge(dest_interp, dest_arena->constant_pmc_pool,
            source_arena->constant_pmc_pool);
    fix_pmc_syncs(dest_interp, source_arena->pmc_pool);
    Parrot_small_object_pool_merge(dest_interp, dest_arena->pmc_pool,
            source_arena->pmc_pool);
    Parrot_small_object_pool_merge(dest_interp,
            dest_arena->constant_string_header_pool,
            source_arena->constant_string_header_pool);
    Parrot_small_object_pool_merge(dest_interp,
            dest_arena->pmc_ext_pool,
            source_arena->pmc_ext_pool);

    for (i = 0; i < source_arena->num_sized; ++i) {
        if (!source_arena->sized_header_pools[i]) {
            continue;
        }

        if (i >= dest_arena->num_sized ||
            !dest_arena->sized_header_pools[i]) {
            make_bufferlike_pool(dest_interp, i * sizeof (void *)
                + sizeof (Buffer));
            assert(dest_arena->sized_header_pools[i]);
        }

        Parrot_small_object_pool_merge(dest_interp,
            dest_arena->sized_header_pools[i],
            source_arena->sized_header_pools[i]);
    }
}

#if 0

/*

=item C<void
Parrot_initialize_header_pool_names(Interp *interp)>

If we want these names, they must be added in DOD.

=cut

*/

void
Parrot_initialize_header_pool_names(Interp *interp)
{
    interp->arena_base->string_header_pool->name
            = string_make(interp, "String Pool", strlen("String Pool"),
            0, PObj_constant_FLAG, 0);
    interp->arena_base->pmc_pool->name
            = string_make(interp, "PMC Pool", strlen("PMC Pool"),
            0, PObj_constant_FLAG, 0);
    /* Set up names for each header pool, * now that we have a constant string
     *
     * * pool available to us */
    interp->arena_base->constant_string_header_pool->name
            =
            string_make(interp, "Constant String Pool",
            strlen("Constant String Pool"), 0, PObj_constant_FLAG, 0);
    interp->arena_base->buffer_header_pool->name =
            string_make(interp, "Generic Header Pool",
            strlen("Generic Header Pool"), 0, PObj_constant_FLAG, 0);
}

#endif

/*

=back

=head1 SEE ALSO

F<include/parrot/headers.h>.

=head1 HISTORY

Initial version by Mike Lambert on 2002.05.27.

=cut

*/


/*
 * Local variables:
 *   c-file-style: "parrot"
 * End:
 * vim: expandtab shiftwidth=4:
 */
