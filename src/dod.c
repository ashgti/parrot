/*
Copyright: 2001-2003 The Perl Foundation.  All Rights Reserved.
$Id$

=head1 NAME

src/dod.c - Dead object destruction of the various headers

=head1 DESCRIPTION

This file implements I<dead object destruction>. This is documented in
PPD 9 with suplementary notes in F<docs/dev/dod.dev>.

It's possible to turn on/off the checking of the system stack and
processor registers. The actual checking is implemented in F<src/cpu_dep.c>.

There's also a verbose mode for garbage collection.

=head2 Functions

=over 4

=cut

*/

#define DOD_C_SOURCE
#include "parrot/parrot.h"
#include "parrot/method_util.h"
#include <assert.h>

/* set this to 1 and above to zero to see if unanchored objects
 * are found in system areas. Please note: these objects might be bogus
 */
#define GC_VERBOSE 0

#if ! DISABLE_GC_DEBUG
/* Set when walking the system stack */
int CONSERVATIVE_POINTER_CHASING = 0;
#endif

static size_t find_common_mask(size_t val1, size_t val2);
static int trace_children(struct Parrot_Interp *interpreter, PMC *current);

/*

=item C<static PARROT_INLINE void
mark_special(Parrot_Interp interpreter, PMC* obj)>

Mark a special PMC, if it has a C<PMC_ECT> structure append or prepend
the C<next_for_GC pointer>, else do custom mark directly.

This should really be inline, so if inline isn't available, it would
be better if it was a macro.

=cut

*/

static PARROT_INLINE void
mark_special(Parrot_Interp interpreter, PMC* obj)
{
    int hi_prio;

    /*
     * If the object is shared, we have to use the arena and dod
     * pointers of the originating interpreter.
     *
     * We are possibly changing other interpreters data here, so
     * the mark phase of DOD must run only on one interpreter of a pool
     * at a time. Freeing unused objects could run as parallel
     * operations then.
     * And: to be sure that a shared object is dead, we have to finish
     * the mark phase of all interpreters in a pool that might reference
     * the object.
     */
    if (PObj_is_PMC_shared_TEST(obj)) {
        interpreter = PMC_sync(obj)->owner;
        assert(interpreter);
    }

    if (PObj_needs_early_DOD_TEST(obj))
        ++interpreter->num_early_PMCs_seen;
    if (PObj_high_priority_DOD_TEST(obj) && interpreter->dod_trace_ptr) {
        /* set obj's parent to high priority */
        PObj_high_priority_DOD_SET(interpreter->dod_trace_ptr);
        hi_prio = 1;
    }
    else
        hi_prio = 0;

    if (obj->pmc_ext) {
        if (hi_prio) {
            PMC* tptr = interpreter->dod_trace_ptr;
            if (PMC_next_for_GC(tptr) == tptr) {
                PMC_next_for_GC(obj) = obj;
            }
            else {
                /* put it at the head of the list */
                PMC_next_for_GC(obj) = PMC_next_for_GC(tptr);
            }
            PMC_next_for_GC(tptr) = (PMC*)obj;
        }
        else {
            /* put it on the end of the list */
            PMC_next_for_GC(interpreter->dod_mark_ptr) = obj;

            /* Explicitly make the tail of the linked list be
             * self-referential */
            interpreter->dod_mark_ptr = PMC_next_for_GC(obj) = obj;
        }
    }
    else if (PObj_custom_mark_TEST(obj))
        VTABLE_mark(interpreter, obj);
}

#if ARENA_DOD_FLAGS

/*

=item C<void pobject_lives(struct Parrot_Interp *interpreter, PObj *obj)>

Tag C<obj> as alive.

Used by the GC system when tracing the root set, and used by the PMC GC
handling routines to tag their individual pieces if they have private
ones.

=cut

*/

void pobject_lives(struct Parrot_Interp *interpreter, PObj *obj)
{

    struct Small_Object_Arena *arena = GET_ARENA(obj);
    size_t n = GET_OBJ_N(arena, obj);
    size_t ns = n >> ARENA_FLAG_SHIFT;
    UINTVAL nm = (n & ARENA_FLAG_MASK) << 2;
    UINTVAL *dod_flags = arena->dod_flags + ns;
    if (*dod_flags & ((PObj_on_free_list_FLAG | PObj_live_FLAG) << nm))
        return;

#ifdef DEBUG_DOD_CONSTS
    if (PObj_is_PMC_TEST(obj) &&
            ((PMC*)obj)->vtable->base_type != enum_class_SArray &&
            ((PMC*)obj)->vtable->base_type != enum_class_Null
            )
        assert(!PObj_constant_TEST(obj));
#endif

    ++arena->live_objects;
    *dod_flags |= PObj_live_FLAG << nm;

#if GC_VERBOSE
        if (PObj_report_TEST(obj)) {
            fprintf(stderr, "GC: PMC %p live\n", obj);
        }
#endif
    if (*dod_flags & (PObj_is_special_PMC_FLAG << nm)) {
        /* all PMCs that need special treatment are handled here
         * for normal PMCs, we don't touch the PMC memory itself
         * so that caches stay clean
         */
#if GC_VERBOSE
        if (PObj_report_TEST(obj)) {
            fprintf(stderr, "GC: PMC %p special\n", obj);
        }
#endif
        mark_special(interpreter, (PMC*) obj);
    }
}

#else

void pobject_lives(struct Parrot_Interp *interpreter, PObj *obj)
{
    /* if object is live or on free list return */
    if (PObj_is_live_or_free_TESTALL(obj)) {
        return;
    }
#if ! DISABLE_GC_DEBUG
#  if GC_VERBOSE
    if (CONSERVATIVE_POINTER_CHASING) {
        fprintf(stderr, "GC Warning! Unanchored %s %p version " INTVAL_FMT
                " found in system areas \n",
                PObj_is_PMC_TEST(obj) ? "PMC" : "Buffer",
                obj, PObj_version(obj));
    }
#  endif
#endif
    /* mark it live */
    PObj_live_SET(obj);

    /* if object is a PMC and contains buffers or PMCs, then attach
     * the PMC to the chained mark list
     */
    if (PObj_is_special_PMC_TEST(obj)) {
        mark_special(interpreter, (PMC*) obj);
    }
#if GC_VERBOSE
    /* buffer GC_DEBUG stuff */
    if (! GC_DEBUG(interpreter))
        return;

    if (PObj_report_TEST(obj)) {
        fprintf(stderr, "GC: buffer %p pointing to %p marked live\n",
                obj, PObj_bufstart((Buffer*) obj));
    }
#endif
}

#endif

/*

=item C<static int
trace_active_PMCs(struct Parrot_Interp *interpreter, int trace_stack)>

Do a full trace run and mark all the PMCs as active if they are. Returns
whether the run wasn't aborted; i.e. whether it's safe to proceed with
GC.

=cut

*/

/* XXX . objects.c */
void mark_object_cache(Parrot_Interp);

static int
trace_active_PMCs(struct Parrot_Interp *interpreter, int trace_stack)
{
    PMC *current;
    /* Pointer to the currently being processed PMC
     *
     * initialize locals to zero, so no garbage is on stack
     *
     * note: adding locals here did cause increased DOD runs
     */
    unsigned int i = 0;
    struct Stash *stash = 0;

    /* We have to start somewhere, the interpreter globals is a good place */
    interpreter->dod_mark_ptr = current = interpreter->iglobals;

    /* mark it as used  */
    pobject_lives(interpreter, (PObj *)interpreter->iglobals);
    /* Now, go run through the PMC registers and mark them as live */
    /* First mark the current set. */
    for (i = 0; i < NUM_REGISTERS; i++) {
        if (interpreter->pmc_reg.registers[i]) {
            pobject_lives(interpreter,
                    (PObj *)interpreter->pmc_reg.registers[i]);
        }
    }

    /* Walk through the stashes */
    stash = interpreter->globals;
    while (stash) {
        pobject_lives(interpreter, (PObj *)stash->stash_hash);
        stash = stash->parent_stash;
    }
    /* s. packfile.c */
    mark_const_subs(interpreter);

    mark_stack_chunk_cache(interpreter);
    mark_object_cache(interpreter);

    /* mark NCI meth hash */
    for (i = 0; i < interpreter->nci_method_table_size; ++i) {
        PMC *h = interpreter->nci_method_table[i];
        if (h)
            pobject_lives(interpreter, (PObj*)h);
    }
    /* Now mark the class hash */
    pobject_lives(interpreter, (PObj *)interpreter->class_hash);

    /* Mark the registry if any */
    if (interpreter->DOD_registry)
        pobject_lives(interpreter, (PObj *)interpreter->DOD_registry);

    /* Walk all stacks */
    mark_context(interpreter, &interpreter->ctx);

    /* Walk the iodata */
    Parrot_IOData_mark(interpreter, interpreter->piodata);

    /* quich check, if we can already bail out */
    if (interpreter->lazy_dod && interpreter->num_early_PMCs_seen >=
            interpreter->num_early_DOD_PMCs) {
        return 0;
    }

    /* Find important stuff on the system stack */
    if (trace_stack)
        trace_system_areas(interpreter);
    /* Okay, we've marked the whole root set, and should have a good-sized
     * list 'o things to look at. Run through it */
    return trace_children(interpreter, current);
}

/*

=item C<static int
trace_children(struct Parrot_Interp *interpreter, PMC *current)>

Returns whether the tracing process wasn't aborted.

=cut

*/

static int
trace_children(struct Parrot_Interp *interpreter, PMC *current)
{
    PMC *prev = NULL;
    unsigned i = 0;
    UINTVAL mask = PObj_is_PMC_ptr_FLAG | PObj_is_buffer_ptr_FLAG
        | PObj_custom_mark_FLAG;

    int lazy_dod = interpreter->lazy_dod;

    /*
     * First phase of mark is finished. Now if we are the owner
     * of a shared pool, we must run the mark phase of other
     * interpreters in our pool, so that live shared PMCs in that
     * interpreter are appended to our mark_ptrs chain
     *
     * If there is a count of shared PMCs and we have already seen
     * all these, we could skip that
     */
    pt_DOD_mark_root_finished(interpreter);

    for (; current != prev; current = PMC_next_for_GC(current)) {
        UINTVAL bits = PObj_get_FLAGS(current) & mask;

        if (lazy_dod && interpreter->num_early_PMCs_seen >=
                interpreter->num_early_DOD_PMCs) {
            return 0;
        }
        interpreter->dod_trace_ptr = current;
        if (!PObj_needs_early_DOD_TEST(current))
            PObj_high_priority_DOD_CLEAR(current);

        /* mark properties */
        if (PMC_metadata(current)) {
            pobject_lives(interpreter, (PObj *)PMC_metadata(current));
        }
        /* Start by checking if there's anything at all. This assumes that the
         * largest percentage of PMCs won't have anything in their data
         * pointer that we need to trace */
        if (bits) {
            if (bits == PObj_is_PMC_ptr_FLAG) {
                if (PMC_data(current)) {
                    pobject_lives(interpreter, PMC_data(current));
                }
            }
            else if (bits == PObj_is_buffer_ptr_FLAG) {
                if (PMC_data(current)) {
                    pobject_lives(interpreter, PMC_data(current));
                }
            }
            else if (bits == PObj_is_buffer_of_PMCs_ptr_FLAG) {
                /* buffer of PMCs */
                Buffer *trace_buf = PMC_data(current);

                if (trace_buf) {
                    PMC **cur_pmc = PObj_bufstart(trace_buf);

                    /* Mark the damn buffer as used! */
                    pobject_lives(interpreter, trace_buf);
                    for (i = 0; i < PObj_buflen(trace_buf) / sizeof(*cur_pmc); i++) {
                        if (cur_pmc[i]) {
                            pobject_lives(interpreter, (PObj *)cur_pmc[i]);
                        }
                    }
                }
            }
            else {
                /* All that's left is the custom */
                VTABLE_mark(interpreter, current);
            }
        }

        prev = current;
    }
    return 1;
}

/*

=item C<static void
trace_active_buffers(struct Parrot_Interp *interpreter)>

Scan any buffers in string registers and other non-PMC places and mark
them as active.

=cut

*/

static void
trace_active_buffers(struct Parrot_Interp *interpreter)
{
    UINTVAL i;

    /* First mark the current set. We assume that all pointers in S registers
     * are pointing to valid buffers. This is not a good assumption, but it'll
     * do for now */
    for (i = 0; i < NUM_REGISTERS; i++) {
        Buffer *reg = (Buffer *)interpreter->string_reg.registers[i];

        if (reg)
            pobject_lives(interpreter, reg);
    }

    /* The interpreter has a few strings of its own */
    if (interpreter->current_file)
        pobject_lives(interpreter, (PObj *)interpreter->current_file);
    if (interpreter->current_package)
        pobject_lives(interpreter, (PObj *)interpreter->current_package);
}

#ifdef GC_IS_MALLOC

/*

=item C<void
clear_cow(struct Parrot_Interp *interpreter, struct Small_Object_Pool *pool,
        int cleanup)>

Clear the COW ref count.

=cut

*/

void
clear_cow(struct Parrot_Interp *interpreter, struct Small_Object_Pool *pool,
        int cleanup)
{
    UINTVAL object_size = pool->object_size;
    struct Small_Object_Arena *cur_arena;
    UINTVAL i;
    Buffer *b;
    int *refcount;

    /* clear refcount for COWable objects. */
    for (cur_arena = pool->last_Arena;
            NULL != cur_arena; cur_arena = cur_arena->prev) {
        b = cur_arena->start_objects;
        for (i = 0; i < cur_arena->used; i++) {
            if (!PObj_on_free_list_TEST(b)) {
                if (cleanup) {
                    /* clear COWed external FLAG */
                    PObj_external_CLEAR(b);
                    /* the real external flag */
                    if (PObj_bufstart_external_TEST(b))
                        PObj_external_SET(b);
                    /* if cleanup (Parrot_destroy) constants are dead too */
                    PObj_constant_CLEAR(b);
                    PObj_live_CLEAR(b);
                }

                if (PObj_COW_TEST(b) && PObj_bufstart(b) &&
                        !PObj_external_TEST(b)) {
                    refcount = (int *) PObj_bufstart(b) - 1;
                    *refcount = 0;
                }
            }
            b = (Buffer *)((char *)b + object_size);
        }
    }
}

/*

=item C<void
used_cow(struct Parrot_Interp *interpreter, struct Small_Object_Pool *pool,
        int cleanup)>

Find other users of COW's C<bufstart>.

=cut

*/

void
used_cow(struct Parrot_Interp *interpreter, struct Small_Object_Pool *pool,
        int cleanup)
{
    UINTVAL object_size = pool->object_size;
    struct Small_Object_Arena *cur_arena;
    UINTVAL i;
    Buffer *b;
    int *refcount;

    for (cur_arena = pool->last_Arena;
            NULL != cur_arena; cur_arena = cur_arena->prev) {
        b = cur_arena->start_objects;
        for (i = 0; i < cur_arena->used; i++) {
            if (!PObj_on_free_list_TEST(b) &&
                    PObj_COW_TEST(b) &&
                    PObj_bufstart(b) &&
                    !PObj_external_TEST(b)) {
                refcount = (int *) PObj_bufstart(b) - 1;
                /* mark users of this bufstart by incrementing refcount */
                if (PObj_live_TEST(b))
                    *refcount = 1 << 29;        /* ~infinite usage */
                else
                    (*refcount)++;      /* dead usage */
            }
            b = (Buffer *)((char *)b + object_size);
        }
    }
}
#endif /* GC_IS_MALLOC */

#if ARENA_DOD_FLAGS

/*

=item C<static void
clear_live_counter(struct Parrot_Interp *interpreter,
        struct Small_Object_Pool *pool)>

Clear the live counter.

=cut

*/

static void
clear_live_counter(struct Parrot_Interp *interpreter,
        struct Small_Object_Pool *pool)
{
    struct Small_Object_Arena *arena;
    for (arena = pool->last_Arena; arena; arena = arena->prev)
        arena->live_objects = 0;
}

# define REDUCE_ARENAS 0

#  if REDUCE_ARENAS
/* -lt:
 * count free objects per arena
 * - if we find more then one totally unused arena
 *   free all but one arena - this is the only possibility to
 *   reduce the amount of free objects
 *
 * doesn't really work or speed things up - disabled
 */
# define REDUCE_ARENA_DEBUG 0

#if REDUCE_ARENA_DEBUG
#define debug(x) printf x
#else
#define debug(x)
#endif

/*

=item C<static void
reduce_arenas(struct Parrot_Interp *interpreter,
        struct Small_Object_Pool *pool, UINTVAL free_arenas)>

Reduce the number of memory arenas by freeing any that have no live
objects.

=cut

*/

static void
reduce_arenas(struct Parrot_Interp *interpreter,
        struct Small_Object_Pool *pool, UINTVAL free_arenas)
{
    struct Small_Object_Arena *arena, *next, *prev;
    int i;
    PObj * o;

    /* debugging stuff */
    for (i = 0, arena = pool->last_Arena; arena; arena = arena->prev)
        i++;
    debug(("\ttotal %d arenas - ", i));

    for (next = arena = pool->last_Arena; arena; ) {
        prev = arena->prev;
        if (arena->live_objects == 0) {
            if (--free_arenas <= 0)
                break;
            next->prev = prev;
            pool->total_objects -= arena->used;
#if ! ARENA_DOD_FLAGS
            mem_sys_free(arena->start_objects);
#endif
            mem_sys_free(arena);

        }
        else {
            next = arena;
        }
        arena = prev;
    }
    for (i = 0, arena = pool->last_Arena; arena; arena = arena->prev)
        i++;
    debug(("now %d arenas\n", i));

}

#  endif
#endif

/*

=item C<void
free_unused_pobjects(struct Parrot_Interp *interpreter,
        struct Small_Object_Pool *pool)>

Put any buffers/PMCs that are now unused, on to the pools free list. If
C<GC_IS_MALLOC> bufstart gets freed too if possible. Avoid buffers that
are immune from collection (ie, constant).

=cut

*/

void
free_unused_pobjects(struct Parrot_Interp *interpreter,
        struct Small_Object_Pool *pool)
{
    struct Small_Object_Arena *cur_arena;
    UINTVAL i, total_used = 0;
    UINTVAL object_size = pool->object_size;
    size_t nm;
#if REDUCE_ARENAS
    UINTVAL free_arenas = 0, old_total_used = 0;
#endif

    /* Run through all the buffer header pools and mark */
    for (cur_arena = pool->last_Arena;
            NULL != cur_arena; cur_arena = cur_arena->prev) {
        Buffer *b = cur_arena->start_objects;

#if ARENA_DOD_FLAGS
        UINTVAL * dod_flags = cur_arena->dod_flags - 1;
#endif
        for (i = nm = 0; i < cur_arena->used; i++) {
#if ARENA_DOD_FLAGS
            if (! (i & ARENA_FLAG_MASK)) {
                /* reset live bits for previous bunch of objects */
                if (i)
                    *dod_flags &= ~ALL_LIVE_MASK;
                ++dod_flags;
                /* if all are on free list, skip one bunch */
                if (*dod_flags == ALL_FREE_MASK) {  /* all on free list */
                    i += ARENA_FLAG_MASK;       /* + 1 in loop */
                    b = (Buffer *)((char *)b + object_size*(ARENA_FLAG_MASK+1));
                    continue;
                }
                nm = 0;
            }
            else
                nm += 4;

            /* If it's not live or on the free list, put it on the free list.
             * Note that it is technically possible to have a Buffer be both
             * on_free_list and live, because of our conservative stack-walk
             * collection. We must be wary of this case. */
            if ((*dod_flags & (PObj_on_free_list_FLAG << nm)))
                ; /* if its on free list, do nothing */
            else if ((*dod_flags & (PObj_live_FLAG << nm)))
#else
            if (PObj_on_free_list_TEST(b))
                ; /* if its on free list, do nothing */
            else if (PObj_live_TEST(b))
#endif
            {
                /* its live */
                total_used++;
#if !ARENA_DOD_FLAGS
                PObj_live_CLEAR(b);
#endif
            }
            else {
                /* it must be dead */
#if GC_VERBOSE
                if (GC_DEBUG(interpreter) && PObj_report_TEST(b)) {
                    fprintf(stderr, "Freeing pobject %p\n", b);
                    if (PObj_is_PMC_TEST(b)) {
                        fprintf(stderr, "\t = PMC type %s\n",
                                (char*) ((PMC*)b)->vtable->whoami->strstart);
                    }
                }
#endif
                /* if object is a PMC and needs destroying */
                if (PObj_is_PMC_TEST(b)) {
                    /* then destroy it here
                    */
                    if (PObj_needs_early_DOD_TEST(b))
                        --interpreter->num_early_DOD_PMCs;
                    if (PObj_active_destroy_TEST(b))
                        VTABLE_destroy(interpreter, (PMC *)b);

                    if (PObj_is_PMC_EXT_TEST(b)) {
                        /* if the PMC has a PMC_EXT structure,
                         * return it to the pool/arena
                         */
                        struct Small_Object_Pool *ext_pool =
                            interpreter->arena_base->pmc_ext_pool;
                        ext_pool->add_free_object(interpreter, ext_pool,
                                ((PMC *)b)->pmc_ext);
                    }
                }
                /* else object is a buffer(like) */
                else if (PObj_sysmem_TEST(b) && PObj_bufstart(b)) {
                    /* has sysmem allocated, e.g. string_pin */
                    mem_sys_free(PObj_bufstart(b));
                    PObj_bufstart(b) = NULL;
                    PObj_buflen(b) = 0;
                }
                else {
#ifdef GC_IS_MALLOC
                    /* free allocated space at (int*)bufstart - 1,
                     * but not if it is used COW or external
                     */
                    if (PObj_bufstart(b) && !PObj_is_external_or_free_TESTALL(b)) {
                        if (PObj_COW_TEST(b)) {
                            int *refcount = ((int *)PObj_bufstart(b) - 1);

                            if (!--(*refcount))
                                free(refcount); /* the actual bufstart */
                        }
                        else
                            free((int*)PObj_bufstart(b) - 1);
                    }
#else
                    /*
                     * XXX Jarkko did report that on irix pool->mem_pool
                     *     was NULL, which really shouldn't happen
                     */
                    if (pool->mem_pool) {
                        if (!PObj_COW_TEST(b)) {
                            ((struct Memory_Pool *)
                             pool->mem_pool)->guaranteed_reclaimable +=
                                PObj_buflen(b);
                        }
                        ((struct Memory_Pool *)
                         pool->mem_pool)->possibly_reclaimable +=
                            PObj_buflen(b);
                    }
#endif
                    PObj_buflen(b) = 0;
                }
#if ARENA_DOD_FLAGS
                *dod_flags |= PObj_on_free_list_FLAG << nm;
#else
                PObj_flags_SETTO((PObj *)b, PObj_on_free_list_FLAG);
#endif
                pool->add_free_object(interpreter, pool, b);
            }
            b = (Buffer *)((char *)b + object_size);
        }
#if ARENA_DOD_FLAGS
        /* reset live bits on last bunch of objects */
        *dod_flags &= ~ALL_LIVE_MASK;
#  if REDUCE_ARENAS
        /* not strictly only for ARENA_DOD_FLAGS, but
         * live_objects is only defined there
         */
        if ( (cur_arena->live_objects = total_used - old_total_used) == 0)
            ++free_arenas;
        old_total_used = total_used;
#  endif
#endif
    }
    pool->num_free_objects = pool->total_objects - total_used;
#if ARENA_DOD_FLAGS
#  if REDUCE_ARENAS
#define REPLENISH_LEVEL_FACTOR 0.3
    if (free_arenas > 1) {
        debug(("pool %s: %d free_arenas\n", pool->name, (int)free_arenas));
        pool->replenish_level =
            (size_t)(pool->total_objects * REPLENISH_LEVEL_FACTOR);
        i = 0;
        while (pool->num_free_objects - i * pool->last_Arena->used *
                REPLENISH_LEVEL_FACTOR > pool->replenish_level &&
                i < free_arenas)
            ++i;
        debug(("\t may free %d\n", (int)i));
        reduce_arenas(interpreter, pool, i-1);
        pool->replenish_level =
            (size_t)(pool->total_objects * REPLENISH_LEVEL_FACTOR);
        pool->num_free_objects = pool->total_objects - total_used;
        pool->skip = 0;
    }
#  undef debug
#  endif
#endif
}

#ifndef PLATFORM_STACK_WALK

/*

=item C<static size_t find_common_mask(size_t val1, size_t val2)>

Find a mask covering the longest common bit-prefix of C<val1> and
C<val2>.

=cut

*/

static size_t
find_common_mask(size_t val1, size_t val2)
{
    int i;
    int bound = sizeof(size_t) * 8;

    for (i = 0; i < bound; i++) {
        if (val1 == val2) {
            return ~(size_t)0 << i;
        }
        val1 >>= 1;
        val2 >>= 1;
    }

    internal_exception(INTERP_ERROR,
            "Unexpected condition in find_common_mask()!\n");
    return 0;
}

/*

=item C<void
trace_mem_block(struct Parrot_Interp *interpreter,
                size_t lo_var_ptr, size_t hi_var_ptr)>

Traces the memory block between C<lo_var_ptr> and C<hi_var_ptr>.

=cut

*/

void
trace_mem_block(struct Parrot_Interp *interpreter,
                size_t lo_var_ptr, size_t hi_var_ptr)
{
    size_t prefix, tmp_ptr;
    ptrdiff_t cur_var_ptr;

    size_t buffer_min = get_min_buffer_address(interpreter);
    size_t buffer_max = get_max_buffer_address(interpreter);
    size_t pmc_min = get_min_pmc_address(interpreter);
    size_t pmc_max = get_max_pmc_address(interpreter);

    size_t mask = find_common_mask(buffer_min < pmc_min ? buffer_min : pmc_min,
            buffer_max > pmc_max ? buffer_max : pmc_max);

    if (!lo_var_ptr || !hi_var_ptr)
        return;
    if (lo_var_ptr < hi_var_ptr) {
        tmp_ptr = hi_var_ptr;
        hi_var_ptr = lo_var_ptr;
        lo_var_ptr = tmp_ptr;
    }
    /* Get the expected prefix */
    prefix = mask & buffer_min;


    for (cur_var_ptr = hi_var_ptr;
            (ptrdiff_t)cur_var_ptr < (ptrdiff_t)lo_var_ptr;
            cur_var_ptr = (size_t)((ptrdiff_t)cur_var_ptr + sizeof(void *))
            ) {
        size_t ptr = *(size_t *)cur_var_ptr;

        /* Do a quick approximate range check by bit-masking */
        if ((ptr & mask) == prefix || !prefix) {
            /* Note that what we find via the stack or registers are not
             * guaranteed to be live pmcs/buffers, and could very well their
             * bufstart/vtable destroyed due to the linked list of free
             * headers... */
            if (pmc_min <= ptr && ptr < pmc_max &&
                    is_pmc_ptr(interpreter, (void *)ptr)) {
                /* ...so ensure that pobject_lives checks PObj_on_free_list_FLAG
                 * before adding it to the next_for_GC list, to have
                 * vtable->mark() called. */
                pobject_lives(interpreter, (PObj *)ptr);
            }
            else if (buffer_min <= ptr && ptr < buffer_max &&
                    is_buffer_ptr(interpreter, (void *)ptr)) {
                /* ...and since pobject_lives doesn't care about bufstart, it
                 * doesn't really matter if it sets a flag */
                pobject_lives(interpreter, (PObj *)ptr);
            }
        }
    }
    return;
}
#endif

/*

=item C<static void clear_live_bits(Parrot_Interp interpreter)>

Run through all PMC arenas and clear live bits.

=cut

*/

static void
clear_live_bits(Parrot_Interp interpreter)
{
    struct Small_Object_Pool *pool = interpreter->arena_base->pmc_pool;
    struct Small_Object_Arena *arena;
    UINTVAL i;
#if !ARENA_DOD_FLAGS
    UINTVAL object_size = pool->object_size;
#endif

    for (arena = pool->last_Arena; arena; arena = arena->prev) {
#if ARENA_DOD_FLAGS
        UINTVAL * dod_flags = arena->dod_flags;
        for (i = 0; i < arena->used; i += (ARENA_FLAG_MASK+1)) {
            /* reset live bits for a bunch of objects */
            *dod_flags &= ~ALL_LIVE_MASK;
            ++dod_flags;
        }
#else
        Buffer *b = arena->start_objects;
        for (i = 0; i < arena->used; i++) {
            PObj_live_CLEAR(b);
            b = (Buffer *)((char *)b + object_size);
        }
#endif
    }
}

/*

=item C<static PARROT_INLINE void
profile_dod_start(Parrot_Interp interpreter)>

Records the start time of a DOD run when profiling is enabled.

=cut

*/

static PARROT_INLINE void
profile_dod_start(Parrot_Interp interpreter)
{
    if (Interp_flags_TEST(interpreter, PARROT_PROFILE_FLAG)) {
        interpreter->profile->dod_time = Parrot_floatval_time();
    }
}

/*

=item C<static PARROT_INLINE void
profile_dod_end(Parrot_Interp interpreter)>

Records the end time of a DOD run when porfiling is enabled.

=cut

*/

static PARROT_INLINE void
profile_dod_end(Parrot_Interp interpreter)
{
    if (Interp_flags_TEST(interpreter, PARROT_PROFILE_FLAG)) {
        RunProfile *profile = interpreter->profile;
        FLOATVAL now = Parrot_floatval_time();

        profile->data[PARROT_PROF_DOD].numcalls++;
        profile->data[PARROT_PROF_DOD].time += now - profile->dod_time;
        profile->starttime += now - profile->dod_time;
    }
}

/*

=item C<void
Parrot_do_dod_run(struct Parrot_Interp *interpreter, UINTVAL flags)>

See if we can find some unused headers.

=cut

*/

void
Parrot_do_dod_run(struct Parrot_Interp *interpreter, UINTVAL flags)
{
    struct Small_Object_Pool *header_pool;
    int j;
    /* XXX these should go into the interpreter */
    int total_free = 0;

    if (interpreter->DOD_block_level) {
        return;
    }
    Parrot_block_DOD(interpreter);
    /*
     * tell the threading system that we gonna DOD mark
     */
    pt_DOD_start_mark(interpreter);

    interpreter->lazy_dod = flags & DOD_lazy_FLAG;
    interpreter->dod_trace_ptr = NULL;
    interpreter->num_early_PMCs_seen = 0;

    if (interpreter->profile)
        profile_dod_start(interpreter);

#if ARENA_DOD_FLAGS
    clear_live_counter(interpreter, interpreter->arena_base->pmc_pool);
    for (j = 0; j < (INTVAL)interpreter->arena_base->num_sized; j++) {
        header_pool = interpreter->arena_base->sized_header_pools[j];
        if (header_pool)
            clear_live_counter(interpreter, header_pool);
    }
#endif
    /* Now go trace the PMCs */
    if (trace_active_PMCs(interpreter, flags & DOD_trace_stack_FLAG)) {
        /* And the buffers */
        trace_active_buffers(interpreter);

        /*
         * mark is now finished
         */
        pt_DOD_stop_mark(interpreter);
        /* Now put unused PMCs on the free list */
        header_pool = interpreter->arena_base->pmc_pool;
        free_unused_pobjects(interpreter, header_pool);
        total_free += header_pool->num_free_objects;

        /* And unused buffers on the free list */
        for (j = 0; j < (INTVAL)interpreter->arena_base->num_sized; j++) {
            header_pool = interpreter->arena_base->sized_header_pools[j];
            if (header_pool) {
#ifdef GC_IS_MALLOC
                used_cow(interpreter, header_pool, 0);
#endif
                free_unused_pobjects(interpreter, header_pool);
                total_free += header_pool->num_free_objects;
#ifdef GC_IS_MALLOC
                clear_cow(interpreter, header_pool, 0);
#endif
            }
        }
    }
    else {
        /* it was an aborted lazy dod run - we should clear
         * the live bits, but e.g. t/pmc/timer_7 succeeds w/o this
         */
        pt_DOD_stop_mark(interpreter);
#if 1
        clear_live_bits(interpreter);
#endif
    }
    /* Note it */
    interpreter->dod_runs++;
    interpreter->dod_trace_ptr = NULL;
    if (interpreter->profile)
        profile_dod_end(interpreter);
    Parrot_unblock_DOD(interpreter);
    return;
}

/*

=back

=head1 SEE ALSO

F<include/parrot/dod.h>, F<src/cpu_dep.c>, F<docs/dev/dod.dev> and
F<docs/pdds/pdd09_gc.pod>.

=head1 HISTORY

Initial version by Mike Lambert on 2002.05.27.

=cut

*/

/*
 * Local variables:
 * c-indentation-style: bsd
 * c-basic-offset: 4
 * indent-tabs-mode: nil
 * End:
 *
 * vim: expandtab shiftwidth=4:
*/
