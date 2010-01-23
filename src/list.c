/*
Copyright (C) 2002-2009, Parrot Foundation.
License:  Artistic 2.0, see README and LICENSE for details
$Id$

=head1 NAME

src/list.c - List aka array routines

=head1 DESCRIPTION

List is roughly based on concepts of IntList (thanks to Steve),
so I don't repeat them here.

Especially the same invariants hold, except an empty list is really
empty, meaning, push does first check for space.

The main differences are:

- List can hold items of different size, it's suitable for ints and PMCs
..., calculations are still done in terms of items. The item_size is
specified at list creation time with the "type" argument.

If you later store different item types in the list, as stated
initially, you'll get probably not what you want - so don't do this.

- List does auto grow. The caller may implement a different behaviour if
she likes.

- Error checking for out of bounds access is minimal, caller knows
better, what should be done.

- List structure itself is different from List_chunk, implying:

=over 4

=item * end of list is not C<< list->prev >> but C<< list->end >>

=item * start of list is list->first

=item * the list of chunks is not closed, detecting the end is more simple

=item * no spare is keeped, didn't improve due to size constraints

=item * the List object itself doesn't move around for shift/unshift

=back

- list chunks don't have C<< ->start >> and C<< ->end >>
fields. Instead the list has C<< ->start >>, which is start of first
chunk, and C<< ->cap >>, the total usable capacity in the list.

- number of items in chunks are not fixed, but there is a mode
using same sized chunks

=head2 Grow policy

=over 4

=item C<enum_grow_fixed>

All chunks are of C<MAX_ITEMS> size, chosen, when the first access to
the array is indexed and beyond C<MIN_ITEMS> and below 10 *
C<MAX_ITEMS>

If the first access is beyond 10 * C<MAX_ITEMS> a sparse chunk will
be created.

To avoid this - and the performance penalty - set the array size
before setting elements.

    new P0, 'Array'
    set P0, 100000  # sets fixed sized, no sparse

This is only meaningful, if a lot of the entries are used too.

=item C<enum_grow_growing>

Chunk sizes grow from C<MIN_ITEMS> to C<MAX_ITEMS>, this will be selected
for pushing data on an empty array.

=item C<enum_grow_mixed>

Mixture of above chunk types and when sparse chunks are present, or
after insert and delete.

The chunks hold the information, how many chunks are of the same type,
beginning from the current, and how many items are included in this
range. See C<get_chunk> below for details.

=back

=head2 Sparse lists


To save memory, List can handle sparse arrays. This code snippet:

new P0, 'IntList'
set P0[1000000], 42

generates 3 List_chunks, one at the beginning of the array, a
big sparse chunk and a chunk for the actual data.

Setting values inside sparse chunks changes them to real chunks.
For poping/shifting inside sparse chunks, s. return value below.

=head2 Chunk types

=over 4

=item C<fixed_items>

Have allocated space, size is a power of 2, consecutive chunks are same sized.

=item C<grow_items>

Same, but consecutive chunks are growing.

=item C<no_power_2>

Have allocated space but any size.

=item C<sparse>

Only dummy allocation, C<< chunk->items >> holds the items of this sparse
hole.

=back

=head2 Data types

A List can hold various datatypes. See F<src/datatypes.h> for the
enumeration of types.

Not all are yet implemented in C<Parrot_pmc_array_set>/C<Parrot_pmc_array_item>, see the
C<switch()>.

Arbitrary length data:

Construct initializer with:

=over 4

=item C<enum_type_sized>

=item C<item_size> (in bytes)

=item C<items_per_chunk> (rounded up to power of 2, default C<MAX_ITEMS>)

=back

In C<Parrot_pmc_array_assign> the values are copied into the array, C<Parrot_pmc_array_get>
returns a pointer as for all other data types.

See F<src/list_2.t> and C<Parrot_pmc_array_new_init()>.

=head2 Return value

List get functions return a C<(void*)> pointer to the location of the
stored data. The caller has to extract the value from this pointer.

For non existent data beyond the dimensions of the array a C<NULL>
pointer is returned.

For non existing data inside sparse holes, a pointer C<(void*)-1> is
returned.

The caller can decide to assume these data as undef or 0 or whatever is
appropriate.

=head2 Testing

See F<t/src/{int, }list.c> and F<t/pmc/{int, }list.t>.

Also all array usage depends on list.

=head2 Functions

=over 4

=cut

*/

#include "parrot/parrot.h"

/* HEADERIZER HFILE: include/parrot/list.h */

/* HEADERIZER BEGIN: static */
/* Don't modify between HEADERIZER BEGIN / HEADERIZER END.  Your changes will be lost. */

PARROT_IGNORABLE_RESULT
PARROT_CANNOT_RETURN_NULL
static List_chunk * add_chunk_at_end(PARROT_INTERP,
    ARGMOD(List *list),
    UINTVAL idx)
        __attribute__nonnull__(1)
        __attribute__nonnull__(2)
        FUNC_MODIFIES(*list);

PARROT_IGNORABLE_RESULT
PARROT_CANNOT_RETURN_NULL
static List_chunk * add_chunk_at_start(PARROT_INTERP,
    ARGMOD(List *list),
    UINTVAL idx)
        __attribute__nonnull__(1)
        __attribute__nonnull__(2)
        FUNC_MODIFIES(*list);

PARROT_WARN_UNUSED_RESULT
PARROT_CANNOT_RETURN_NULL
static List_chunk * alloc_next_size(PARROT_INTERP,
    ARGMOD(List *list),
    int where,
    UINTVAL idx)
        __attribute__nonnull__(1)
        __attribute__nonnull__(2)
        FUNC_MODIFIES(*list);

PARROT_CANNOT_RETURN_NULL
PARROT_WARN_UNUSED_RESULT
static List_chunk * allocate_chunk(PARROT_INTERP,
    ARGIN(List *list),
    UINTVAL items,
    UINTVAL size)
        __attribute__nonnull__(1)
        __attribute__nonnull__(2);

PARROT_WARN_UNUSED_RESULT
PARROT_CANNOT_RETURN_NULL
static List_chunk * get_chunk(PARROT_INTERP,
    ARGMOD(List *list),
    ARGMOD(UINTVAL *idx))
        __attribute__nonnull__(1)
        __attribute__nonnull__(2)
        __attribute__nonnull__(3)
        FUNC_MODIFIES(*list)
        FUNC_MODIFIES(*idx);

static void Parrot_pmc_array_append(PARROT_INTERP,
    ARGMOD(List *list),
    ARGIN_NULLOK(void *item),
    int type,
    UINTVAL idx)
        __attribute__nonnull__(1)
        __attribute__nonnull__(2)
        FUNC_MODIFIES(*list);

PARROT_WARN_UNUSED_RESULT
PARROT_CANNOT_RETURN_NULL
static void * Parrot_pmc_array_item(PARROT_INTERP,
    ARGMOD(List *list),
    int type,
    INTVAL idx)
        __attribute__nonnull__(1)
        __attribute__nonnull__(2)
        FUNC_MODIFIES(*list);

static void Parrot_pmc_array_set(PARROT_INTERP,
    ARGMOD(List *list),
    ARGIN_NULLOK(void *item),
    INTVAL type,
    INTVAL idx)
        __attribute__nonnull__(1)
        __attribute__nonnull__(2)
        FUNC_MODIFIES(*list);

static void rebuild_chunk_list(PARROT_INTERP, ARGMOD(List *list))
        __attribute__nonnull__(1)
        __attribute__nonnull__(2)
        FUNC_MODIFIES(*list);

static void rebuild_chunk_ptrs(ARGMOD(List *list), int cut)
        __attribute__nonnull__(1)
        FUNC_MODIFIES(*list);

static void rebuild_fix_ends(ARGMOD(List *list))
        __attribute__nonnull__(1)
        FUNC_MODIFIES(*list);

static void rebuild_other(PARROT_INTERP, ARGMOD(List *list))
        __attribute__nonnull__(1)
        __attribute__nonnull__(2)
        FUNC_MODIFIES(*list);

static void rebuild_sparse(ARGMOD(List *list))
        __attribute__nonnull__(1)
        FUNC_MODIFIES(*list);

static void split_chunk(PARROT_INTERP,
    ARGMOD(List *list),
    ARGMOD(List_chunk *chunk),
    UINTVAL ix)
        __attribute__nonnull__(1)
        __attribute__nonnull__(2)
        __attribute__nonnull__(3)
        FUNC_MODIFIES(*list)
        FUNC_MODIFIES(*chunk);

#define ASSERT_ARGS_add_chunk_at_end __attribute__unused__ int _ASSERT_ARGS_CHECK = (\
       PARROT_ASSERT_ARG(interp) \
    , PARROT_ASSERT_ARG(list))
#define ASSERT_ARGS_add_chunk_at_start __attribute__unused__ int _ASSERT_ARGS_CHECK = (\
       PARROT_ASSERT_ARG(interp) \
    , PARROT_ASSERT_ARG(list))
#define ASSERT_ARGS_alloc_next_size __attribute__unused__ int _ASSERT_ARGS_CHECK = (\
       PARROT_ASSERT_ARG(interp) \
    , PARROT_ASSERT_ARG(list))
#define ASSERT_ARGS_allocate_chunk __attribute__unused__ int _ASSERT_ARGS_CHECK = (\
       PARROT_ASSERT_ARG(interp) \
    , PARROT_ASSERT_ARG(list))
#define ASSERT_ARGS_get_chunk __attribute__unused__ int _ASSERT_ARGS_CHECK = (\
       PARROT_ASSERT_ARG(interp) \
    , PARROT_ASSERT_ARG(list) \
    , PARROT_ASSERT_ARG(idx))
#define ASSERT_ARGS_Parrot_pmc_array_append __attribute__unused__ int _ASSERT_ARGS_CHECK = (\
       PARROT_ASSERT_ARG(interp) \
    , PARROT_ASSERT_ARG(list))
#define ASSERT_ARGS_Parrot_pmc_array_item __attribute__unused__ int _ASSERT_ARGS_CHECK = (\
       PARROT_ASSERT_ARG(interp) \
    , PARROT_ASSERT_ARG(list))
#define ASSERT_ARGS_Parrot_pmc_array_set __attribute__unused__ int _ASSERT_ARGS_CHECK = (\
       PARROT_ASSERT_ARG(interp) \
    , PARROT_ASSERT_ARG(list))
#define ASSERT_ARGS_rebuild_chunk_list __attribute__unused__ int _ASSERT_ARGS_CHECK = (\
       PARROT_ASSERT_ARG(interp) \
    , PARROT_ASSERT_ARG(list))
#define ASSERT_ARGS_rebuild_chunk_ptrs __attribute__unused__ int _ASSERT_ARGS_CHECK = (\
       PARROT_ASSERT_ARG(list))
#define ASSERT_ARGS_rebuild_fix_ends __attribute__unused__ int _ASSERT_ARGS_CHECK = (\
       PARROT_ASSERT_ARG(list))
#define ASSERT_ARGS_rebuild_other __attribute__unused__ int _ASSERT_ARGS_CHECK = (\
       PARROT_ASSERT_ARG(interp) \
    , PARROT_ASSERT_ARG(list))
#define ASSERT_ARGS_rebuild_sparse __attribute__unused__ int _ASSERT_ARGS_CHECK = (\
       PARROT_ASSERT_ARG(list))
#define ASSERT_ARGS_split_chunk __attribute__unused__ int _ASSERT_ARGS_CHECK = (\
       PARROT_ASSERT_ARG(interp) \
    , PARROT_ASSERT_ARG(list) \
    , PARROT_ASSERT_ARG(chunk))
/* Don't modify between HEADERIZER BEGIN / HEADERIZER END.  Your changes will be lost. */
/* HEADERIZER END: static */

#define chunk_list_size(list) \
                (Buffer_buflen(&(list)->chunk_list) / sizeof (List_chunk *))

/* hide the ugly cast somehow: */
#define chunk_list_ptr(list, idx) \
        ((List_chunk**) Buffer_bufstart(&(list)->chunk_list))[(idx)]

/*

=item C<static List_chunk * allocate_chunk(PARROT_INTERP, List *list, UINTVAL
items, UINTVAL size)>

Makes a new chunk, and allocates C<size> bytes for buffer storage from the
generic memory pool. The chunk holds C<items> items. Marks the chunk as
being part of C<< list->container >>, if it exists, for the purposes of GC. Does
not install the chunk into C<< list->container >> yet.

=cut

*/

PARROT_CANNOT_RETURN_NULL
PARROT_WARN_UNUSED_RESULT
static List_chunk *
allocate_chunk(PARROT_INTERP, ARGIN(List *list), UINTVAL items, UINTVAL size)
{
    ASSERT_ARGS(allocate_chunk)
    List_chunk *chunk;

    Parrot_block_GC_mark(interp);
    /* Parrot_block_GC_sweep(interp); - why */

    chunk = (List_chunk *)Parrot_gc_new_bufferlike_header(interp, sizeof (*chunk));

    chunk->items    = items;
    chunk->n_chunks = 0;
    chunk->n_items  = 0;
    chunk->next     = NULL;
    chunk->prev     = NULL;
    Parrot_gc_allocate_buffer_storage_aligned(interp, (Buffer *)chunk, size);
    memset(Buffer_bufstart((Buffer*)chunk), 0, size);

    Parrot_unblock_GC_mark(interp);

    /* Parrot_unblock_GC_sweep(interp); */
    return chunk;
}


/*

=item C<static void rebuild_chunk_ptrs(List *list, int cut)>

Rebuilds C<list> and updates/optimizes chunk usage. Deletes empty chunks,
counts chunks, and fixes C<prev> pointers.

=cut

*/

static void
rebuild_chunk_ptrs(ARGMOD(List *list), int cut)
{
    ASSERT_ARGS(rebuild_chunk_ptrs)
    List_chunk *chunk, *prev;
    UINTVAL start = list->start;
    UINTVAL len = 0;
    UINTVAL cap = 0;

    for (prev = NULL, chunk = list->first; chunk; chunk = chunk->next) {
        /* skip empty chunks, first is empty, when all items get skipped due
         * to list->start */
        if (chunk->items == start) {
            if (prev)
                prev->next = chunk->next;
            else
                list->first = chunk->next;
            start = 0;
            continue;
        }

        len++;

        start       = 0;
        chunk->prev = prev;
        prev        = chunk;
        list->last  = chunk;

        if (cut && cap > list->start + list->length && chunk != list->first) {
            list->last = chunk->prev ? chunk->prev : list->first;
            len--;
            break;
        }

        cap += chunk->items;
    }
    if (list->last)
        list->last->next = NULL;

    list->cap = cap;

    if (list->first)
        list->first->prev = NULL;

    list->n_chunks = len;
}


/*

=item C<static void rebuild_sparse(List *list)>

Combines adjacent sparse chunks in C<list>.

=cut

*/

static void
rebuild_sparse(ARGMOD(List *list))
{
    ASSERT_ARGS(rebuild_sparse)

    List_chunk *chunk   = list->first;
    List_chunk *prev    = NULL;
    int         changes = 0;

    for (; chunk; chunk = chunk->next) {
        if (prev && (prev->flags & sparse) && (chunk->flags & sparse)) {
            prev->items += chunk->items;
            chunk->items = 0;
            changes++;
            continue;
        }
        prev = chunk;
    }

    if (changes)
        rebuild_chunk_ptrs(list, 0);
}


/*

=item C<static void rebuild_other(PARROT_INTERP, List *list)>

Combines adjacent irregular chunks in C<list>.

=cut

*/

static void
rebuild_other(PARROT_INTERP, ARGMOD(List *list))
{
    ASSERT_ARGS(rebuild_other)
    List_chunk *chunk   = list->first;
    List_chunk *prev    = NULL;
    int         changes = 0;

    for (; chunk; chunk = chunk->next) {
        /* two adjacent irregular chunks */
        if (prev && (prev->flags & no_power_2) && (chunk->flags & no_power_2)) {
            /* DONE don't make chunks bigger then MAX_ITEMS, no - make then
             * but: if bigger, split them in a next pass
             * TODO test the logic that solves the above problem */
            if (prev->items + chunk->items > MAX_ITEMS) {
                Parrot_gc_reallocate_buffer_storage(interp, (Buffer *)prev,
                        MAX_ITEMS * list->item_size);

                mem_sys_memmove(
                        (char *) Buffer_bufstart(&prev->data) +
                        prev->items * list->item_size,
                        (const char *) Buffer_bufstart(&chunk->data),
                        (MAX_ITEMS - prev->items) * list->item_size);
                mem_sys_memmove(
                        (char *) Buffer_bufstart(&chunk->data),
                        (const char *) Buffer_bufstart(&chunk->data) +
                        (MAX_ITEMS - prev->items) * list->item_size,
                        (chunk->items - (MAX_ITEMS - prev->items))
                                                        * list->item_size);
                chunk->items = chunk->items - (MAX_ITEMS - prev->items);
                prev->items  = MAX_ITEMS;
            }
            else {
                Parrot_gc_reallocate_buffer_storage(interp, (Buffer *)prev,
                        (prev->items + chunk->items) * list->item_size);

                mem_sys_memmove(
                        (char *) Buffer_bufstart(&prev->data) +
                        prev->items * list->item_size,
                        (const char *) Buffer_bufstart(&chunk->data),
                        chunk->items * list->item_size);
                prev->items += chunk->items;
                chunk->items = 0;
            }
            changes++;
            continue;
        }
        prev = chunk;
    }

    if (changes)
        rebuild_chunk_ptrs(list, 0);
}


/*

=item C<static void rebuild_fix_ends(List *list)>

Resets some values in C<list> and the lists's first chunk.  Called by
C<rebuild_chunk_list()>.

=cut

*/

static void
rebuild_fix_ends(ARGMOD(List *list))
{
    ASSERT_ARGS(rebuild_fix_ends)
    List_chunk * const chunk = list->first;

    /* first is irregular, next is empty */
    if (list->n_chunks <= 2 && (chunk->flags & no_power_2)
    && (!chunk->next
    ||   chunk->next->items == 0
    ||   list->start + list->length <= chunk->items)) {

        chunk->flags      = 0;
        list->grow_policy = enum_grow_unknown;
        list->cap        += Buffer_buflen(&chunk->data) / list->item_size - chunk->items;
        chunk->items      = Buffer_buflen(&chunk->data) / list->item_size;
    }

    /* XXX - still needed? - if last is empty and last->prev not full then
     * delete last - combine small chunks if list is big */
}


/*

=item C<static void rebuild_chunk_list(PARROT_INTERP, List *list)>

Optimizes a modified C<list>: combines adjacent chunks if they are both sparse
or irregular and updates the grow policies and computes list statistics.

=cut

*/

static void
rebuild_chunk_list(PARROT_INTERP, ARGMOD(List *list))
{
    ASSERT_ARGS(rebuild_chunk_list)
    List_chunk *chunk, *prev, *first;
    UINTVAL len;

    Parrot_block_GC_mark(interp);
    Parrot_block_GC_sweep(interp);

    /* count chunks and fix prev pointers */
    rebuild_chunk_ptrs(list, 0);

    /* if not regular, check & optimize */
    if (list->grow_policy == enum_grow_mixed) {
        rebuild_sparse(list);
        rebuild_other(interp, list);
        rebuild_fix_ends(list);
    }

    /* allocate a new chunk_list buffer, if old one has moved or is too small */
    len = list->n_chunks;
    if (list->collect_runs != Parrot_gc_count_collect_runs(interp)
    ||  len > chunk_list_size(list)) {
        /* round up to reasonable size */
        len = 1 << (ld(len) + 1);

        if (len < 4)
            len = 4;

        Parrot_gc_reallocate_buffer_storage(interp, (Buffer *)list,
                len * sizeof (List_chunk *));

        list->collect_runs = Parrot_gc_count_collect_runs(interp);
    }

    /* reset type, actual state of chunks will show, what we really have */
    list->grow_policy = enum_grow_unknown;

    /* fill chunk_list and update statistics */
    first = chunk = list->first;
    for (prev = NULL, len = 0; chunk; chunk = chunk->next) {
        chunk_list_ptr(list, len) = chunk;
        len++;

        /* look what type of chunks we have this is always correct: */
        chunk->n_chunks = 1;
        chunk->n_items  = chunk->items;

        /* sparse hole or irregular chunk */
        if (chunk->flags & (sparse | no_power_2)) {
            List_chunk *next;

            /* add next sparse or no_power_2 chunks up so that get_chunk will
             * skip this range of chunks, when the idx is beyond this block. */
            for (next = chunk->next; next; next = next->next)
                if (next->flags & (sparse | no_power_2)) {
                    chunk->n_chunks++;
                    chunk->n_items += next->items;
                }
                else
                    break;
            first             = chunk->next;
            list->grow_policy = enum_grow_mixed;
            continue;
        }

        /* clear flag, next chunks will tell what comes */
        chunk->flags = enum_grow_unknown;

        if (first && first != chunk) {
            /* constant chunk block */
            if (first->items == chunk->items) {
                first->n_chunks++;
                first->n_items += chunk->items;
                first->flags = fixed_items;

                /* TODO optimize for fixed but non MAX_ITEMS lists */
                if (first->items == MAX_ITEMS)
                    list->grow_policy |= enum_grow_fixed;
                else
                    list->grow_policy |= enum_grow_mixed;
            }

            /* growing chunk block could optimize small growing blocks, they
             * are probably not worth the effort. */
            else if (prev && (prev->items == chunk->items >> 1)) {
                first->n_chunks++;
                first->n_items += chunk->items;
                first->flags = grow_items;
                list->grow_policy |= enum_grow_growing;
            }
            /* different growing scheme starts here */
            else
                first = chunk;
        }

        prev = chunk;
    }

    /* if we have some mixture of grow_policies, then set it to _mixed */
    if (list->grow_policy && list->grow_policy != enum_grow_growing
    &&  list->grow_policy != enum_grow_fixed)
        list->grow_policy = enum_grow_mixed;

    Parrot_unblock_GC_mark(interp);
    Parrot_unblock_GC_sweep(interp);
}


/*

=item C<static List_chunk * alloc_next_size(PARROT_INTERP, List *list, int
where, UINTVAL idx)>

Calculates the size and number of items for the next chunk and allocates it.
Adds the number of allocated items to the list's total, but does not
directly add the chunk to the C<list>.

=cut

*/

PARROT_WARN_UNUSED_RESULT
PARROT_CANNOT_RETURN_NULL
static List_chunk *
alloc_next_size(PARROT_INTERP, ARGMOD(List *list), int where, UINTVAL idx)
{
    ASSERT_ARGS(alloc_next_size)
    List_chunk *new_chunk;
    UINTVAL     items, size;
    const int   much      = idx - list->cap >= MIN_ITEMS;
    int         do_sparse = (INTVAL)idx - (INTVAL)list->cap >= 10 * MAX_ITEMS;

    if (list->item_type == enum_type_sized) {
        do_sparse = 0;
        items     = list->items_per_chunk;
        size      = items * list->item_size;

        list->grow_policy = items == MAX_ITEMS ?
            enum_grow_fixed : enum_grow_mixed;
    }
    else if (do_sparse) {
        PARROT_ASSERT(where);
        /* don't add sparse chunk at start of list */
        if (!list->n_chunks) {
            do_sparse = 0;
            items     = MAX_ITEMS;

            /* if we need more, the next allocation will allocate the rest */
            size              = items * list->item_size;
            list->grow_policy = enum_grow_fixed;
        }
        else {
            /* allocate a dummy chunk holding many items virtually */
            size  = list->item_size;
            items = idx - list->cap - 1;

            /* round down this function will then be called again, to add the
             * final real chunk, with the rest of the needed size */
            items            &= ~(MAX_ITEMS - 1);
            list->grow_policy = enum_grow_mixed;
        }
    }
    /* initial size for empty lists grow_policy is not yet known or was
     * different */
    else if (!list->cap) {
#ifdef ONLY_FIXED_ALLOCATIONS
        list->grow_policy = enum_grow_fixed;
#else
        list->grow_policy = enum_grow_unknown;
#endif
        /* more then MIN_ITEMS, i.e. indexed access beyond length */
        if (much) {
            list->grow_policy = enum_grow_fixed;
            items             = MAX_ITEMS;
        }
        else {
            /* TODO make bigger for small items like char */
            items = MIN_ITEMS;
        }
        size = items * list->item_size;
    }
    else {
        if (list->grow_policy & (enum_grow_fixed | enum_grow_mixed))
            items = MAX_ITEMS;
        else {
            items = where ? list->last->items : list->first->items;
            /* push: allocate at end, more if possbile */
            if (where) {
                if (items < MAX_ITEMS) {
                    items <<= 1;
                    list->grow_policy = enum_grow_growing;
                }
            }
            /* unshift: if possible, make less items */
            else {
                list->grow_policy = enum_grow_growing;
                if (items > MIN_ITEMS)
                    items >>= 1;        /* allocate less */
                /* if not: second allocation from unshift */
                else {
                    list->grow_policy = enum_grow_mixed;
                    items = MAX_ITEMS;
                }
            }
        }
        size = items * list->item_size;
    }

    new_chunk  = allocate_chunk(interp, list, items, size);
    list->cap += items;

    if (do_sparse)
        new_chunk->flags |= sparse;

    return new_chunk;
}


/*

=item C<static List_chunk * add_chunk_at_start(PARROT_INTERP, List *list,
UINTVAL idx)>

Adds a new chunk to the start of C<list>.

=cut

*/

PARROT_IGNORABLE_RESULT
PARROT_CANNOT_RETURN_NULL
static List_chunk *
add_chunk_at_start(PARROT_INTERP, ARGMOD(List *list), UINTVAL idx)
{
    ASSERT_ARGS(add_chunk_at_start)
    List_chunk * const chunk     = list->first;
    List_chunk * const new_chunk = alloc_next_size(interp, list, enum_add_at_start, idx);

    new_chunk->next = chunk;
    list->first     = new_chunk;

    if (!list->last)
        list->last = new_chunk;

    rebuild_chunk_list(interp, list);

    return new_chunk;
}


/*

=item C<static List_chunk * add_chunk_at_end(PARROT_INTERP, List *list, UINTVAL
idx)>

Adds a new chunk to the end of C<list>.

=cut

*/

PARROT_IGNORABLE_RESULT
PARROT_CANNOT_RETURN_NULL
static List_chunk *
add_chunk_at_end(PARROT_INTERP, ARGMOD(List *list), UINTVAL idx)
{
    ASSERT_ARGS(add_chunk_at_end)
    List_chunk * const chunk     = list->last;
    List_chunk * const new_chunk = alloc_next_size(interp, list, enum_add_at_end, idx);

    if (chunk)
        chunk->next = new_chunk;

    if (!list->first)
        list->first = new_chunk;

    list->last = new_chunk;

    rebuild_chunk_list(interp, list);

    return new_chunk;
}


/*

=item C<UINTVAL ld(UINTVAL x)>

Calculates log2(x), or a useful approximation thereof. Stolen from
F<src/malloc.c>.

=cut

*/

PARROT_EXPORT
PARROT_CONST_FUNCTION
PARROT_WARN_UNUSED_RESULT
UINTVAL
ld(UINTVAL x)
{
    ASSERT_ARGS(ld)
    UINTVAL m;                  /* bit position of highest set bit of m */

    /* On intel, use BSRL instruction to find highest bit */
#if defined(__GNUC__) && defined(i386)

  __asm__("bsrl %1,%0\n\t":"=r"(m)
  :        "g"(x));

#else
    {
        /*
         * Based on branch-free nlz algorithm in chapter 5 of Henry S. Warren
         * Jr's book "Hacker's Delight". */

        unsigned int n = ((x - 0x100) >> 16) & 8;

        x <<= n;
        m = ((x - 0x1000) >> 16) & 4;
        n += m;
        x <<= m;
        m = ((x - 0x4000) >> 16) & 2;
        n += m;
        x = (x << m) >> 14;
        m = 13 - n + (x & ~(x >> 1));
    }
#endif
    return m;
}


/*

=item C<static List_chunk * get_chunk(PARROT_INTERP, List *list, UINTVAL *idx)>

Get the chunk for C<idx>, also update the C<idx> to point into the chunk.

This routine will be called for every operation on list, so it's
optimized to be fast and needs an up-to-date chunk statistic.
C<rebuild_chunk_list> provides the necessary chunk statistics.

The scheme of operations is:

    if all_chunks_are_MAX_ITEMS
         chunk = chunk_list[ idx / MAX_ITEMS ]
         idx =   idx % MAX_ITEMS
         done.

    chunk = first
    repeat
         if (index < chunk->items)
             done.

     if (index >= items_in_chunk_block)
         index -= items_in_chunk_block
         chunk += chunks_in_chunk_block
         continue

     calc chunk and index in this block
     done.

One chunk_block consists of chunks of the same type: fixed, growing or
other. So the time to look up a chunk doesn't depend on the array
length, but on the complexity of the array. C<rebuild_chunk_list> tries
to reduce the complexity, but may fail, if you e.g. do a prime sieve by
actually C<Parrot_pmc_array_delet>ing the none prime numbers.

The complexity of the array is how many different C<chunk_blocks> are
there. They come from:

- initially fixed: 1

- initially growing: 2

- first unshift: 1 except for initially fixed arrays

- insert: 1 - 3

- delete: 1 - 2

- sparse hole: 3 (could be 2, code assumes access at either end now)

There could be some optimizer that, after detecting almost only indexed access
after some time, reorganizes the array to be all C<MAX_ITEMS> sized when this
would improve performance.

=cut

*/

PARROT_WARN_UNUSED_RESULT
PARROT_CANNOT_RETURN_NULL
static List_chunk *
get_chunk(PARROT_INTERP, ARGMOD(List *list), ARGMOD(UINTVAL *idx))
{
    ASSERT_ARGS(get_chunk)
    List_chunk *chunk;
    UINTVAL i;

    if (list->collect_runs != Parrot_gc_count_collect_runs(interp))
        rebuild_chunk_list(interp, list);
#ifdef SLOW_AND_BORING
    /* in SLOW_AND_BORING mode, we loop through each chunk, and determine if
       idx is in the chunk using basic bounds checking. If the loop completes
       without finding idx we panic. "Panic" is probably not the best
       reaction, however. */
    UNUSED(interp);
    for (chunk = list->first; chunk; chunk = chunk->next) {
        if (*idx < chunk->items)
            return chunk;
        *idx -= chunk->items;
    }

    Parrot_ex_throw_from_c_args(interp, NULL, EXCEPTION_INTERNAL_PANIC,
        "Reached end of list %p without finding item index %d\n",
        list, *idx);
#endif

    /* fixed sized chunks - easy: all MAX_ITEMS sized */
    if (list->grow_policy == enum_grow_fixed) {
        chunk = chunk_list_ptr(list, *idx >> LD_MAX);
        *idx &= MAX_MASK;
        return chunk;
    }

    /* else look at chunks flags, what grow type follows and adjust chunks and
     * idx */
    for (i = 0, chunk = list->first; chunk;) {
        /* if we have no more items, we have found the chunk */
        if (*idx < chunk->items)
            return chunk;

        /* now look, if we can use the range of items in chunk_block: if idx
         * is beyond n_items, skip n_chunks */
        if (*idx >= chunk->n_items) {
            i    += chunk->n_chunks;
            *idx -= chunk->n_items;
            chunk = chunk_list_ptr(list, i);
            continue;
        }

        /* we are inside this range of items */
        if (chunk->flags & fixed_items) {
            /* all chunks are chunk->items big, a power of 2 */
            chunk = chunk_list_ptr(list, i + (*idx >> ld(chunk->items)));
            *idx &= chunk->items - 1;
            return chunk;
        }

/*
 * Here is a small table, providing the basics of growing sized
 * addressing, for people like me, whose math lessons are +30 years
 * in the past ;-)
 * assuming MIN_ITEMS=4
 *
 * ch#  size    idx     +4      bit             ld2(idx) -ld2(4)
 *
 * 0    4       0..3    4..7    0000 01xx       2       0
 * 1    8       4..11   8..15   0000 1xxx       3       1
 * 2    16      12..27  16..31  0001 xxxx       4       2
 * ...
 * 8    1024    1020..  ...2047                 10      8
 */

        if (chunk->flags & grow_items) {
            /* the next chunks are growing from chunk->items ... last->items */
            const UINTVAL ld_first = ld(chunk->items);
            const UINTVAL slot     = ld(*idx + chunk->items) - ld_first;

            /* we are in this growing area, so we are done */
            PARROT_ASSERT(slot < chunk->n_chunks);
            *idx -= (1 << (ld_first + slot)) - chunk->items;
            return chunk_list_ptr(list, i + slot);
        }

        if (chunk->flags & (sparse | no_power_2)) {
            /* these chunks hold exactly chunk->items */
            *idx -= chunk->items;
            chunk = chunk->next;
            i++;
            continue;
        }

        Parrot_ex_throw_from_c_args(interp, NULL, EXCEPTION_INTERNAL_PANIC,
            "Cannot determine how to find location %d in list %p of %d items\n",
            *idx, list, list->cap);
    }

    Parrot_ex_throw_from_c_args(interp, NULL, EXCEPTION_INTERNAL_PANIC,
        "Cannot find index %d in list %p of %d items using any method\n",
        *idx, list, list->cap);
}


/*

=item C<static void split_chunk(PARROT_INTERP, List *list, List_chunk *chunk,
UINTVAL ix)>

Splits a sparse chunk, so that we have

- allocated space at C<idx>

if sparse is big:

- C<MAX_ITEMS> near C<idx> and if there is still sparse space after the
real chunk, this also C<n*MAX_ITEMS> sized, so that consecutive writing
would make C<MAX_ITEMS> sized real chunks.

=cut

*/

static void
split_chunk(PARROT_INTERP, ARGMOD(List *list), ARGMOD(List_chunk *chunk), UINTVAL ix)
{
    ASSERT_ARGS(split_chunk)
    /* allocate space at idx */
    if (chunk->items <= MAX_ITEMS) {
        /* it fits, just allocate */
        Parrot_gc_reallocate_buffer_storage(interp, (Buffer *)chunk,
                chunk->items * list->item_size);

        chunk->flags |= no_power_2;
        chunk->flags &= ~sparse;
    }
    else {
        /* split chunk->items: n3 = n*MAX_ITEMS after chunk n2 = MAX_ITEMS
         * chunk n1 = rest before */
        const INTVAL idx = ix;
        const INTVAL n2  = MAX_ITEMS;
        const INTVAL n3  = ((chunk->items - idx) / MAX_ITEMS) * MAX_ITEMS;
        const INTVAL n1  = chunk->items - n2 - n3;

        chunk->items     = n2;

        Parrot_gc_reallocate_buffer_storage(interp, (Buffer *)chunk,
                chunk->items * list->item_size);

        chunk->flags &= ~sparse;

        if (n3) {
            List_chunk * const new_chunk = allocate_chunk(interp, list, n3, list->item_size);

            new_chunk->flags |= sparse;
            new_chunk->next   = chunk->next;

            if (chunk->next)
                chunk->next = new_chunk;
            else
                list->last = new_chunk;
        }

        /* size before idx */
        if (n1 > 0) {
            /* insert a new sparse chunk before this one */
            List_chunk * const new_chunk = allocate_chunk(interp, list, n1, list->item_size);

            new_chunk->flags |= sparse;
            new_chunk->next   = chunk;

            if (chunk->prev)
                chunk->prev->next = new_chunk;
            else
                list->first = new_chunk;
        }
    }

    rebuild_chunk_list(interp, list);
}


/*

=item C<static void Parrot_pmc_array_set(PARROT_INTERP, List *list, void *item,
INTVAL type, INTVAL idx)>

Sets C<item> of type C<type> in chunk at C<idx>.

=cut

*/

static void
Parrot_pmc_array_set(PARROT_INTERP, ARGMOD(List *list), ARGIN_NULLOK(void *item),
        INTVAL type, INTVAL idx)
{
    ASSERT_ARGS(Parrot_pmc_array_set)
    List_chunk  *chunk = get_chunk(interp, list, (UINTVAL *)&idx);
    const INTVAL oidx  = idx;

    PARROT_ASSERT(chunk);

    /* if this is a sparse chunk: split in possibly 2 sparse parts before and
     * after then make a real chunk, rebuild chunk list and set item */
    if (chunk->flags & sparse) {
        split_chunk(interp, list, chunk, idx);
        /* reget chunk and idx */
        idx   = oidx;
        chunk = get_chunk(interp, list, (UINTVAL *)&idx);
        PARROT_ASSERT(chunk);
        PARROT_ASSERT(!(chunk->flags & sparse));
    }

    switch (type) {
      case enum_type_sized:
        /* copy data into list */
        memcpy(&((char *) Buffer_bufstart(&chunk->data))[idx * list->item_size],
                item, list->item_size);
        break;
      case enum_type_char:
        ((char *) Buffer_bufstart(&chunk->data))[idx] = (char)PTR2INTVAL(item);
        break;
      case enum_type_short:
        ((short *) Buffer_bufstart(&chunk->data))[idx] = (short)PTR2INTVAL(item);
        break;
      case enum_type_int:
        ((int *) Buffer_bufstart(&chunk->data))[idx] = (int)PTR2INTVAL(item);
        break;
      case enum_type_INTVAL:
        ((INTVAL *) Buffer_bufstart(&chunk->data))[idx] = PTR2INTVAL(item);
        break;
      case enum_type_FLOATVAL:
        ((FLOATVAL *) Buffer_bufstart(&chunk->data))[idx] = *(FLOATVAL *)item;
        break;
      case enum_type_PMC:
        ((PMC **) Buffer_bufstart(&chunk->data))[idx] = (PMC *)item;
        break;
      case enum_type_STRING:
        ((STRING **) Buffer_bufstart(&chunk->data))[idx] = (STRING *)item;
        break;
      default:
        Parrot_ex_throw_from_c_args(interp, NULL, 1, "Unknown list entry type\n");
        break;
    }
}


/*

=item C<static void * Parrot_pmc_array_item(PARROT_INTERP, List *list, int type,
INTVAL idx)>

Get the pointer to the item of type C<type> in the chunk at C<idx>.

=cut

*/

PARROT_WARN_UNUSED_RESULT
PARROT_CANNOT_RETURN_NULL
static void *
Parrot_pmc_array_item(PARROT_INTERP, ARGMOD(List *list), int type, INTVAL idx)
{
    ASSERT_ARGS(Parrot_pmc_array_item)
    List_chunk * const chunk = get_chunk(interp, list, (UINTVAL *)&idx);
    /* if this is a sparse chunk return -1, the caller may decide to return 0
     * or undef or whatever */
    if (chunk->flags & sparse) {
#ifdef INTLIST_EMUL
        static int null = 0;

        return (void *)&null;
#else
        return (void *)-1;
#endif
    }

    switch (type) {
      case enum_type_sized:
        return (void *)&((char *)
                Buffer_bufstart(&chunk->data))[idx * list->item_size];
      case enum_type_char:
        return (void *)&((char *) Buffer_bufstart(&chunk->data))[idx];
      case enum_type_short:
        return (void *)&((short *) Buffer_bufstart(&chunk->data))[idx];
      case enum_type_int:
        return (void *)&((int *) Buffer_bufstart(&chunk->data))[idx];
      case enum_type_INTVAL:
        return (void *)&((INTVAL *) Buffer_bufstart(&chunk->data))[idx];
      case enum_type_FLOATVAL:
        return (void *)&((FLOATVAL *) Buffer_bufstart(&chunk->data))[idx];
      case enum_type_PMC:
        return (void *)&((PMC **) Buffer_bufstart(&chunk->data))[idx];
      case enum_type_STRING:
        return (void *)&((STRING **) Buffer_bufstart(&chunk->data))[idx];
      default:
        Parrot_ex_throw_from_c_args(interp, NULL, 1, "Unknown list entry type\n");
    }
}


/*

=item C<static void Parrot_pmc_array_append(PARROT_INTERP, List *list, void
*item, int type, UINTVAL idx)>

Adds one or more chunks to end of list.

=cut

*/

static void
Parrot_pmc_array_append(PARROT_INTERP, ARGMOD(List *list),
    ARGIN_NULLOK(void *item), int type, UINTVAL idx)
{
    ASSERT_ARGS(Parrot_pmc_array_append)
    /* initially, list may be empty, also used by assign */
    while (idx >= list->cap){
        add_chunk_at_end(interp, list, idx);
    }
    Parrot_pmc_array_set(interp, list, item, type, idx);

    /* invariant: prepare for next push */
    if (idx >= list->cap - 1)
        add_chunk_at_end(interp, list, 0);
}


/*

=back

=head2 Public Interface Functions

=over 4

=item C<List * Parrot_pmc_array_new(PARROT_INTERP, PARROT_DATA_TYPE type)>

Returns a new list of type C<type>.

=cut

*/

PARROT_EXPORT
PARROT_WARN_UNUSED_RESULT
PARROT_CANNOT_RETURN_NULL
List *
Parrot_pmc_array_new(PARROT_INTERP, PARROT_DATA_TYPE type)
{
    ASSERT_ARGS(Parrot_pmc_array_new)
    List * const list = (List *)Parrot_gc_new_bufferlike_header(interp,
                             sizeof (*list));

    list->item_type = type;
    switch (type) {
      case enum_type_sized:       /* gets overridden below */
      case enum_type_char:
        list->item_size = sizeof (char);
        break;
      case enum_type_short:
        list->item_size = sizeof (short);
        break;
      case enum_type_int:
        list->item_size = sizeof (int);
        break;
      case enum_type_INTVAL:
        list->item_size = sizeof (INTVAL);
        break;
      case enum_type_FLOATVAL:
        list->item_size = sizeof (FLOATVAL);
        break;
      case enum_type_PMC:
        list->item_size = sizeof (PMC *);
        break;
      case enum_type_STRING:
        list->item_size = sizeof (STRING *);
        break;
      default:
        Parrot_ex_throw_from_c_args(interp, NULL, 1, "Unknown list type\n");
        break;
    }

    return list;
}


/*

=item C<void Parrot_pmc_array_pmc_new(PARROT_INTERP, PMC *container)>

Creates a new list containing PMC* values in C<PMC_data(container)>.

=cut

*/

PARROT_EXPORT
void
Parrot_pmc_array_pmc_new(PARROT_INTERP, ARGMOD(PMC *container))
{
    ASSERT_ARGS(Parrot_pmc_array_pmc_new)

    List * const l      = Parrot_pmc_array_new(interp, enum_type_PMC);
    l->container        = container;
    PMC_data(container) = l;
}


/*

=item C<List * Parrot_pmc_array_new_init(PARROT_INTERP, PARROT_DATA_TYPE type,
PMC *init)>

C<Parrot_pmc_array_new_init()> uses these initializers:

    0 ... size (set initial size of list)
    1 ... array dimensions (multiarray)
    2 ... type (overriding type parameter)
    3 ... item_size for enum_type_sized
    4 ... items_per_chunk

=cut

*/

PARROT_EXPORT
PARROT_WARN_UNUSED_RESULT
PARROT_CANNOT_RETURN_NULL
List *
Parrot_pmc_array_new_init(PARROT_INTERP, PARROT_DATA_TYPE type, ARGIN(PMC *init))
{
    ASSERT_ARGS(Parrot_pmc_array_new_init)
    List  *list;
    PMC   *multi_key       = NULL;
    INTVAL size            = 0;
    INTVAL item_size       = 0;
    INTVAL items_per_chunk = 0;

    INTVAL i, len;

    if (!init->vtable)
        Parrot_ex_throw_from_c_args(interp, NULL, 1,
            "Illegal initializer for init\n");

    len = VTABLE_elements(interp, init);

    if (len & 1)
        Parrot_ex_throw_from_c_args(interp, NULL, 1,
            "Illegal initializer for init: odd elements\n");

    for (i = 0; i < len; i += 2) {
        const INTVAL key = VTABLE_get_integer_keyed_int(interp, init, i);
        const INTVAL val = i + 1;
        switch (key) {
          case 0:
            size = VTABLE_get_integer_keyed_int(interp, init, val);
            break;
          case 1:
            multi_key = VTABLE_get_pmc_keyed_int(interp, init, val);
            break;
          case 2:
            {
                const INTVAL result =
                    VTABLE_get_integer_keyed_int(interp, init, val);
                type   = (PARROT_DATA_TYPE)result;
            }
            break;
          case 3:
            item_size = VTABLE_get_integer_keyed_int(interp, init, val);
            break;
          case 4:
            items_per_chunk = VTABLE_get_integer_keyed_int(
                        interp, init, val);
            break;
          default:
            Parrot_ex_throw_from_c_args(interp, NULL, 1,
                    "Invalid initializer for list\n");
        }
    }

    list = Parrot_pmc_array_new(interp, type);

    if (list->item_type == enum_type_sized) { /* override item_size */

        if (!item_size)
            Parrot_ex_throw_from_c_args(interp, NULL, 1,
                "No item_size for type_sized list\n");

        list->item_size       = item_size;
        list->items_per_chunk =
            items_per_chunk
                ? (1 << (ld(items_per_chunk) + 1)) /* make power of 2 */
                : MAX_ITEMS;
    }

    if (size)
        Parrot_pmc_array_set_length(interp, list, size);

    return list;
}


/*

=item C<void Parrot_pmc_array_pmc_new_init(PARROT_INTERP, PMC *container, PMC
*init)>

Creates a new list of PMC* values in C<PMC_data(container)>.

=cut

*/

PARROT_EXPORT
void
Parrot_pmc_array_pmc_new_init(PARROT_INTERP, ARGMOD(PMC *container), ARGIN(PMC *init))
{
    ASSERT_ARGS(Parrot_pmc_array_pmc_new_init)

    List * const l      = Parrot_pmc_array_new_init(interp, enum_type_PMC, init);
    l->container        = container;
    PMC_data(container) = l;
}


/*

=item C<List * Parrot_pmc_array_clone(PARROT_INTERP, const List *other)>

Returns a clone of the C<other> list.

TODO - Barely tested. Optimize new array structure, fixed if big.

=cut

*/

PARROT_EXPORT
PARROT_WARN_UNUSED_RESULT
PARROT_CANNOT_RETURN_NULL
List *
Parrot_pmc_array_clone(PARROT_INTERP, ARGIN(const List *other))
{
    ASSERT_ARGS(Parrot_pmc_array_clone)
    List       *l;
    List_chunk *chunk, *prev;
    UINTVAL     i;

    Parrot_block_GC_mark(interp);
    Parrot_block_GC_sweep(interp);

    l = Parrot_pmc_array_new(interp, other->item_type);

    STRUCT_COPY(l, other);
    Buffer_buflen(&l->chunk_list)   = 0;
    Buffer_bufstart(&l->chunk_list) = NULL;

    for (chunk = other->first, prev = NULL; chunk; chunk = chunk->next) {
        List_chunk * const new_chunk = allocate_chunk(interp, l,
                chunk->items, Buffer_buflen(&chunk->data));

        new_chunk->flags = chunk->flags;

        if (!prev)
            l->first   = new_chunk;
        else
            prev->next = new_chunk;

        prev = new_chunk;

        if (!(new_chunk->flags & sparse)) {
            switch (l->item_type) {
              case enum_type_PMC:
                for (i = 0; i < chunk->items; i++) {
                    PMC * const op = ((PMC **) Buffer_bufstart(&chunk->data))[i];

                    if (op)
                        ((PMC **) Buffer_bufstart(&new_chunk->data))[i] =
                            VTABLE_clone(interp, op);
                }
                break;
              case enum_type_STRING:
                for (i = 0; i < chunk->items; i++) {
                    STRING *s = ((STRING **) Buffer_bufstart(&chunk->data))[i];
                    if (s)
                        ((STRING **) Buffer_bufstart(&new_chunk->data))[i] =
                                Parrot_str_copy(interp, s);
                }
                break;
              default:
                mem_sys_memcopy(Buffer_bufstart(&new_chunk->data),
                        Buffer_bufstart(&chunk->data), Buffer_buflen(&chunk->data));
                break;
            }
        }
    }

    rebuild_chunk_list(interp, l);
    Parrot_unblock_GC_mark(interp);
    Parrot_unblock_GC_sweep(interp);

    return l;
}


/*

=item C<void Parrot_pmc_array_mark(PARROT_INTERP, List *list)>

Marks the list and its contents as live for the memory management system.

=cut

*/

PARROT_EXPORT
void
Parrot_pmc_array_mark(PARROT_INTERP, ARGMOD(List *list))
{
    ASSERT_ARGS(Parrot_pmc_array_mark)
    List_chunk *chunk;

    for (chunk = list->first; chunk; chunk = chunk->next) {
        Parrot_gc_mark_PObj_alive(interp, (PObj *)chunk);

        if (list->item_type == enum_type_PMC
        ||  list->item_type == enum_type_STRING) {
            if (!(chunk->flags & sparse)) {
                PObj **p = ((PObj **) Buffer_bufstart(&chunk->data));
                UINTVAL i;

                for (i = 0; i < chunk->items; i++, ++p) {
                    if (*p)
                        Parrot_gc_mark_PObj_alive(interp, *p);
                }
            }

        }
    }

    Parrot_gc_mark_PObj_alive(interp, (PObj *)list);
}


/*

=item C<void Parrot_pmc_array_visit(PARROT_INTERP, List *list, visit_info
*info)>

This is used by freeze/thaw to visit the contents of the list.

C<pinfo> is the visit info, (see include/parrot/pmc_freeze.h>).

=cut

*/

PARROT_EXPORT
void
Parrot_pmc_array_visit(PARROT_INTERP, ARGIN(List *list), ARGMOD(visit_info *info))
{
    ASSERT_ARGS(Parrot_pmc_array_visit)
    List_chunk        *chunk;
    UINTVAL            idx;

    const UINTVAL n = Parrot_pmc_array_length(interp, list);
    PARROT_ASSERT(list->item_type == enum_type_PMC);

    for (idx = 0, chunk = list->first; chunk; chunk = chunk->next) {
        /* TODO deleted elements */
        if (!(chunk->flags & sparse)) {
            UINTVAL i;
            for (i = 0; i < chunk->items && idx < n; i++, idx++) {
                VISIT_PMC(interp, info, ((PMC **)Buffer_bufstart(&chunk->data))[i]);
            }
        }
        /*
         * TODO handle sparse
         */
    }
}


/*

=item C<INTVAL Parrot_pmc_array_length(PARROT_INTERP, const List *list)>

Returns the length of the list.

=cut

*/

PARROT_WARN_UNUSED_RESULT
PARROT_PURE_FUNCTION
INTVAL
Parrot_pmc_array_length(SHIM_INTERP, ARGIN(const List *list))
{
    ASSERT_ARGS(Parrot_pmc_array_length)
    return list->length;
}


/*

=item C<void Parrot_pmc_array_set_length(PARROT_INTERP, List *list, INTVAL len)>

Sets the length of the list to C<len>.

=cut

*/

PARROT_EXPORT
void
Parrot_pmc_array_set_length(PARROT_INTERP, ARGMOD(List *list), INTVAL len)
{
    ASSERT_ARGS(Parrot_pmc_array_set_length)

    if (len < 0)
        len += list->length;

    if (len >= 0) {
        const UINTVAL idx = list->start + (UINTVAL)len;
        list->length      = len;

        if (idx >= list->cap) {
            /* assume user will fill it, so don't generate sparse chunks */
            if (!list->cap && idx > MAX_ITEMS) {
                while (idx - MAX_ITEMS >= list->cap) {
                    add_chunk_at_end(interp, list, list->cap + MAX_ITEMS);
                }
            }

            Parrot_pmc_array_append(interp, list, NULL, list->item_type, idx);
        }
        else
            rebuild_chunk_ptrs(list, 1);

        rebuild_chunk_list(interp, list);
    }
}


/*

=item C<void Parrot_pmc_array_insert(PARROT_INTERP, List *list, INTVAL idx,
INTVAL n_items)>

Makes room for C<n_items> at C<idx>.

=cut

*/

PARROT_EXPORT
void
Parrot_pmc_array_insert(PARROT_INTERP, ARGMOD(List *list), INTVAL idx, INTVAL n_items)
{
    ASSERT_ARGS(Parrot_pmc_array_insert)
    List_chunk *chunk;

    PARROT_ASSERT(idx >= 0);
    idx += list->start;

    PARROT_ASSERT(n_items >= 0);
    if (n_items == 0)
        return;

    /* empty list */
    if (idx >= (INTVAL)list->cap) {
        idx         += n_items;
        list->length = idx;

        while (idx >= (INTVAL)list->cap)
            add_chunk_at_end(interp, list, idx);

        return;
    }

    list->length += n_items;
    list->cap    += n_items;
    chunk         = get_chunk(interp, list, (UINTVAL *)&idx);

    /* the easy case: */
    if (chunk->flags & sparse)
        chunk->items += n_items;
    else {
        List_chunk *new_chunk;
        INTVAL      items;

        /* 1. cut this chunk at idx */
        list->grow_policy = enum_grow_mixed;

        /* allocate a sparse chunk, n_items big */
        new_chunk         = allocate_chunk(interp, list, n_items, list->item_size);
        new_chunk->flags |= sparse;
        items             = chunk->items - idx;

        if (items) {
            /* allocate a small chunk, holding the rest of chunk beyond idx */
            List_chunk *rest = allocate_chunk(interp, list, items,
                                     items * list->item_size);

            chunk->flags     = no_power_2;
            rest->flags     |= no_power_2;

            /* hang them together */
            rest->next      = chunk->next;
            chunk->next     = new_chunk;
            new_chunk->next = rest;

            /* copy data over */
            mem_sys_memmove(
                    (char *)Buffer_bufstart(&rest->data),
                    (char *)Buffer_bufstart(&chunk->data) + idx * list->item_size,
                    items * list->item_size);
        }
        else {
            new_chunk->next = chunk->next;
            chunk->next     = new_chunk;
        }

        chunk->items = idx;
    }

    rebuild_chunk_list(interp, list);
}


/*

=item C<void Parrot_pmc_array_delete(PARROT_INTERP, List *list, INTVAL idx,
INTVAL n_items)>

Deletes C<n_items> at C<idx>.

=cut

*/

PARROT_EXPORT
void
Parrot_pmc_array_delete(PARROT_INTERP, ARGMOD(List *list), INTVAL idx, INTVAL n_items)
{
    ASSERT_ARGS(Parrot_pmc_array_delete)
    List_chunk *chunk;

    PARROT_ASSERT(idx >= 0);
    PARROT_ASSERT(n_items >= 0);

    if (n_items == 0)
        return;

    idx  += list->start;
    chunk = get_chunk(interp, list, (UINTVAL *)&idx);

    /* deleting beyond end? */
    if (idx + n_items > (INTVAL)list->length)
        n_items = list->length - idx;

    list->length     -= n_items;
    list->cap        -= n_items;
    list->grow_policy = enum_grow_mixed;

    while (n_items > 0) {
        if (idx + n_items <= (INTVAL)chunk->items) {
            /* chunk is bigger then we want to delete */
            if (!(chunk->flags & sparse)) {
                chunk->flags = no_power_2;
                if (idx + n_items <= (INTVAL)chunk->items) {
#ifdef __LCC__
                    /* LCC has a bug where it can't handle all the temporary
                     * variables created in this one line.  adding an explicit
                     * one fixes things.  No need to force this workaround on
                     * less brain-damaged compilers though */
                    size_t tmp_size = (chunk->items - idx - n_items) *
                            list->item_size;

                    mem_sys_memmove(
                            (char *) Buffer_bufstart(&chunk->data) +
                            idx * list->item_size,
                            (char *) Buffer_bufstart(&chunk->data) +
                            (idx + n_items) * list->item_size, tmp_size);
#else
                    mem_sys_memmove(
                            (char *) Buffer_bufstart(&chunk->data) +
                            idx * list->item_size,
                            (char *) Buffer_bufstart(&chunk->data) +
                            (idx + n_items) * list->item_size,
                            (chunk->items - idx - n_items) * list->item_size);
#endif
                }
            }
            chunk->items -= n_items;
            break;
        }

        if (idx == 0 && n_items >= (INTVAL)chunk->items) {
            /* delete this chunk */
            n_items -= chunk->items;

            /* rebuild_chunk_list will kill it because: */
            chunk->items = 0;

            if (!chunk->prev)
                list->first = chunk->next;
        }
        else if (idx) {
            /* else shrink chunk, it starts at idx then */
            if (!(chunk->flags & sparse))
                chunk->flags = no_power_2;

            n_items     -= chunk->items - idx;
            chunk->items = idx;
        }

        idx   = 0;
        chunk = chunk->next;
    }

    rebuild_chunk_ptrs(list, 1);
    rebuild_chunk_list(interp, list);
}


/*

=item C<void Parrot_pmc_array_push(PARROT_INTERP, List *list, void *item, int
type)>

Pushes C<item> of type C<type> on to the end of the list.

=cut

*/

PARROT_EXPORT
void
Parrot_pmc_array_push(PARROT_INTERP, ARGMOD(List *list), ARGIN_NULLOK(void *item), int type)
{
    ASSERT_ARGS(Parrot_pmc_array_push)
    const INTVAL idx = list->start + list->length++;

    Parrot_pmc_array_append(interp, list, item, type, idx);
}


/*

=item C<void Parrot_pmc_array_unshift(PARROT_INTERP, List *list, void *item, int
type)>

Pushes C<item> of type C<type> on to the start of the list.

=cut

*/

PARROT_EXPORT
void
Parrot_pmc_array_unshift(PARROT_INTERP, ARGMOD(List *list), ARGIN(void *item), int type)
{
    ASSERT_ARGS(Parrot_pmc_array_unshift)
    List_chunk *chunk;

    if (list->start == 0) {
        chunk       = add_chunk_at_start(interp, list, 0);
        list->start = chunk->items;
    }

    /* XXX This chunk is unused.  Why are we getting it? */
    else
        chunk = list->first;

    Parrot_pmc_array_set(interp, list, item, type, --list->start);
    list->length++;
}


/*

=item C<void * Parrot_pmc_array_pop(PARROT_INTERP, List *list, int type)>

Removes and returns the last item of type C<type> from the end of the list.

=cut

*/

PARROT_EXPORT
PARROT_CAN_RETURN_NULL
void *
Parrot_pmc_array_pop(PARROT_INTERP, ARGMOD(List *list), int type)
{
    ASSERT_ARGS(Parrot_pmc_array_pop)
    List_chunk *chunk = list->last;
    UINTVAL     idx;

    if (list->length == 0)
        return NULL;

    idx = list->start + --list->length;

    if (list->length == 0)
        list->start = 0;

    /* shrink array if necessary */
    if (idx < list->cap - chunk->items) {
        list->cap  -= chunk->items;
        chunk       = list->last = chunk->prev;
        chunk->next = NULL;

        if (list->n_chunks <= 2)
            list->first = list->last;

        rebuild_chunk_list(interp, list);
    }

    return Parrot_pmc_array_item(interp, list, type, idx);
}


/*

=item C<void * Parrot_pmc_array_shift(PARROT_INTERP, List *list, int type)>

Removes and returns the first item of type C<type> from the start of the list.

=cut

*/

PARROT_EXPORT
PARROT_CAN_RETURN_NULL
void *
Parrot_pmc_array_shift(PARROT_INTERP, ARGMOD(List *list), int type)
{
    ASSERT_ARGS(Parrot_pmc_array_shift)
    void       *ret;
    List_chunk *chunk = list->first;
    UINTVAL     idx;

    if (list->length == 0)
        return NULL;

    idx = list->start;

    /* optimize push + shift on empty lists */
    if (--list->length == 0)
        list->start = 0;
    else
        list->start++;

    ret = Parrot_pmc_array_item(interp, list, type, idx);

    if (list->start >= chunk->items) {
        list->cap  -= chunk->items;
        chunk       = list->first = chunk->next ? chunk->next : list->last;
        list->start = 0;

        rebuild_chunk_list(interp, list);

        if (list->n_chunks == 1)
            list->last = list->first;
    }

    return ret;
}


/*

=item C<void Parrot_pmc_array_assign(PARROT_INTERP, List *list, INTVAL idx, void
*item, int type)>

Assigns C<item> of type C<type> to index C<idx>.

=cut

*/

PARROT_EXPORT
void
Parrot_pmc_array_assign(PARROT_INTERP, ARGMOD(List *list), INTVAL idx,
    ARGIN_NULLOK(void *item), int type)
{
    ASSERT_ARGS(Parrot_pmc_array_assign)
    const INTVAL length = list->length;

    if (idx < -length)
        idx = -idx - length - 1;
    else if (idx < 0)
        idx += length;
    if (idx >= length) {
        Parrot_pmc_array_append(interp, list, item, type, list->start + idx);
        list->length = idx + 1;
    }
    else
        Parrot_pmc_array_set(interp, list, item, type, list->start + idx);
}


/*

=item C<void * Parrot_pmc_array_get(PARROT_INTERP, List *list, INTVAL idx, int
type)>

Returns the item of type C<type> at index C<idx>.

=cut

*/

PARROT_EXPORT
PARROT_CAN_RETURN_NULL
PARROT_WARN_UNUSED_RESULT
void *
Parrot_pmc_array_get(PARROT_INTERP, ARGMOD(List *list), INTVAL idx, int type)
{
    ASSERT_ARGS(Parrot_pmc_array_get)
    const INTVAL length = list->length;

    if (idx >= length || -idx > length)
        return NULL;

    if (idx < 0)
        idx += length;

    idx += list->start;

    return Parrot_pmc_array_item(interp, list, type, idx);
}


/*

=item C<void Parrot_pmc_array_splice(PARROT_INTERP, List *list, List
*value_list, INTVAL offset, INTVAL count)>

Replaces C<count> items starting at C<offset> with the items in C<value>.

If C<count> is 0 then the items in C<value> will be inserted after C<offset>.

=cut

*/

PARROT_EXPORT
void
Parrot_pmc_array_splice(PARROT_INTERP, ARGMOD(List *list), ARGMOD_NULLOK(List *value_list),
        INTVAL offset, INTVAL count)
{
    ASSERT_ARGS(Parrot_pmc_array_splice)
    const INTVAL value_length = value_list ? value_list->length : 0;
    const INTVAL length       = list->length;
    const int    type         = list->item_type;
    INTVAL       i, j;

    if (value_list && type != value_list->item_type)
        Parrot_ex_throw_from_c_args(interp, NULL, 1,
            "Item type mismatch in splice\n");

    /* start from end */
    if (offset < 0) {
        offset += length;
        if (offset < 0)
            Parrot_ex_throw_from_c_args(interp, NULL, EXCEPTION_OUT_OF_BOUNDS,
                "illegal splice offset\n");
    }

    /* "leave that many elements off the end of the array" */
    if (count < 0) {
        count += length - offset + 1;
        if (count < 0)
            count = 0;
    }

    /* replace count items at offset with values */
    for (i = j = 0; i < count && j < value_length; i++, j++) {
        void *val = Parrot_pmc_array_get(interp, value_list, j, type);

        /* no clone here, if the HL wants to reuse the values, the HL has to */
        /* clone the values */

        /* XXX We don't know that val is non-NULL coming back from Parrot_pmc_array_get. */
        /* We need to check that we're not dereferencing NULL. */
        if (type == enum_type_PMC)
            val = *(PMC **)val;
        else if (type == enum_type_STRING)
            val = *(STRING **)val;

        Parrot_pmc_array_assign(interp, list, offset + i, val, type);
    }

    /* if we still have values in value_list, insert them */
    if (j < value_length) {
        /* make room for the remaining values */
        Parrot_pmc_array_insert(interp, list, offset + i, value_length - j);

        for (; j < value_length; i++, j++) {
            void *val = Parrot_pmc_array_get(interp, value_list, j, type);

            /* XXX We don't know that val is non-NULL coming back from
             * Parrot_pmc_array_get; check that we're not dereferencing NULL. */
            if (type == enum_type_PMC)
                val = *(PMC **)val;
            else if (type == enum_type_STRING)
                val = *(STRING **)val;

            Parrot_pmc_array_assign(interp, list, offset + i, val, type);
        }
    }
    /* else delete the rest */
    else
        Parrot_pmc_array_delete(interp, list, offset + i, count - i);
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
