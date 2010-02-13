/*
Copyright (C) 2001-2009, Parrot Foundation.
$Id$

=head1 NAME

src/gc/gc_boehm.c - A Boehm GC

=head1 DESCRIPTION

TODO

=cut

*/

#include "parrot/parrot.h"
#include "gc_private.h"

#ifdef PARROT_HAS_BOEHM_GC

#include <gc.h>
#include <gc_typed.h>

typedef struct boehm_gc_data {
    GC_word pmc_layout;
    GC_word pmc_descriptor;

    GC_word string_layout;
    GC_word string_descriptor;
} boehm_gc_data;

/* HEADERIZER HFILE: src/gc/gc_private.h */

/* HEADERIZER BEGIN: static */
/* Don't modify between HEADERIZER BEGIN / HEADERIZER END.  Your changes will be lost. */

static void gc_boehm_allocate_buffer_storage(PARROT_INTERP,
    ARGMOD(Buffer *buffer),
    size_t size)
        __attribute__nonnull__(1)
        __attribute__nonnull__(2)
        FUNC_MODIFIES(*buffer);

static Buffer* gc_boehm_allocate_bufferlike_header(PARROT_INTERP,
    size_t size)
        __attribute__nonnull__(1);

static void* gc_boehm_allocate_fixed_size_storage(PARROT_INTERP,
    size_t size)
        __attribute__nonnull__(1);

static void* gc_boehm_allocate_pmc_attributes(PARROT_INTERP, PMC *pmc)
        __attribute__nonnull__(1);

static PMC* gc_boehm_allocate_pmc_header(PARROT_INTERP, UINTVAL flags)
        __attribute__nonnull__(1);

static STRING* gc_boehm_allocate_string_header(PARROT_INTERP, UINTVAL flags)
        __attribute__nonnull__(1);

static void gc_boehm_allocate_string_storage(PARROT_INTERP,
    STRING *str,
    size_t size)
        __attribute__nonnull__(1);

static void gc_boehm_compact_memory_pool(PARROT_INTERP)
        __attribute__nonnull__(1);

static void gc_boehm_finalize_cb(ARGIN(void *obj), ARGIN(void *user_data))
        __attribute__nonnull__(1)
        __attribute__nonnull__(2);

static void gc_boehm_free_bufferlike_header(PARROT_INTERP,
    Buffer *b,
    size_t size)
        __attribute__nonnull__(1);

static void gc_boehm_free_fixed_size_storage(PARROT_INTERP,
    size_t size,
    void *data)
        __attribute__nonnull__(1);

static void gc_boehm_free_pmc_attributes(PARROT_INTERP, PMC *pmc)
        __attribute__nonnull__(1);

static void gc_boehm_free_pmc_header(PARROT_INTERP, PMC *pmc)
        __attribute__nonnull__(1);

static void gc_boehm_free_string_header(PARROT_INTERP, STRING *s)
        __attribute__nonnull__(1);

static size_t gc_boehm_get_gc_info(PARROT_INTERP, Interpinfo_enum what)
        __attribute__nonnull__(1);

static void gc_boehm_mark_and_sweep(SHIM_INTERP, UINTVAL flags);
static void gc_boehm_reallocate_buffer_storage(PARROT_INTERP,
    ARGMOD(Buffer *buffer),
    size_t size)
        __attribute__nonnull__(1)
        __attribute__nonnull__(2)
        FUNC_MODIFIES(*buffer);

static void gc_boehm_reallocate_string_storage(PARROT_INTERP,
    STRING *str,
    size_t size)
        __attribute__nonnull__(1);

#define ASSERT_ARGS_gc_boehm_allocate_buffer_storage \
     __attribute__unused__ int _ASSERT_ARGS_CHECK = (\
       PARROT_ASSERT_ARG(interp) \
    , PARROT_ASSERT_ARG(buffer))
#define ASSERT_ARGS_gc_boehm_allocate_bufferlike_header \
     __attribute__unused__ int _ASSERT_ARGS_CHECK = (\
       PARROT_ASSERT_ARG(interp))
#define ASSERT_ARGS_gc_boehm_allocate_fixed_size_storage \
     __attribute__unused__ int _ASSERT_ARGS_CHECK = (\
       PARROT_ASSERT_ARG(interp))
#define ASSERT_ARGS_gc_boehm_allocate_pmc_attributes \
     __attribute__unused__ int _ASSERT_ARGS_CHECK = (\
       PARROT_ASSERT_ARG(interp))
#define ASSERT_ARGS_gc_boehm_allocate_pmc_header __attribute__unused__ int _ASSERT_ARGS_CHECK = (\
       PARROT_ASSERT_ARG(interp))
#define ASSERT_ARGS_gc_boehm_allocate_string_header \
     __attribute__unused__ int _ASSERT_ARGS_CHECK = (\
       PARROT_ASSERT_ARG(interp))
#define ASSERT_ARGS_gc_boehm_allocate_string_storage \
     __attribute__unused__ int _ASSERT_ARGS_CHECK = (\
       PARROT_ASSERT_ARG(interp))
#define ASSERT_ARGS_gc_boehm_compact_memory_pool __attribute__unused__ int _ASSERT_ARGS_CHECK = (\
       PARROT_ASSERT_ARG(interp))
#define ASSERT_ARGS_gc_boehm_finalize_cb __attribute__unused__ int _ASSERT_ARGS_CHECK = (\
       PARROT_ASSERT_ARG(obj) \
    , PARROT_ASSERT_ARG(user_data))
#define ASSERT_ARGS_gc_boehm_free_bufferlike_header \
     __attribute__unused__ int _ASSERT_ARGS_CHECK = (\
       PARROT_ASSERT_ARG(interp))
#define ASSERT_ARGS_gc_boehm_free_fixed_size_storage \
     __attribute__unused__ int _ASSERT_ARGS_CHECK = (\
       PARROT_ASSERT_ARG(interp))
#define ASSERT_ARGS_gc_boehm_free_pmc_attributes __attribute__unused__ int _ASSERT_ARGS_CHECK = (\
       PARROT_ASSERT_ARG(interp))
#define ASSERT_ARGS_gc_boehm_free_pmc_header __attribute__unused__ int _ASSERT_ARGS_CHECK = (\
       PARROT_ASSERT_ARG(interp))
#define ASSERT_ARGS_gc_boehm_free_string_header __attribute__unused__ int _ASSERT_ARGS_CHECK = (\
       PARROT_ASSERT_ARG(interp))
#define ASSERT_ARGS_gc_boehm_get_gc_info __attribute__unused__ int _ASSERT_ARGS_CHECK = (\
       PARROT_ASSERT_ARG(interp))
#define ASSERT_ARGS_gc_boehm_mark_and_sweep __attribute__unused__ int _ASSERT_ARGS_CHECK = (0)
#define ASSERT_ARGS_gc_boehm_reallocate_buffer_storage \
     __attribute__unused__ int _ASSERT_ARGS_CHECK = (\
       PARROT_ASSERT_ARG(interp) \
    , PARROT_ASSERT_ARG(buffer))
#define ASSERT_ARGS_gc_boehm_reallocate_string_storage \
     __attribute__unused__ int _ASSERT_ARGS_CHECK = (\
       PARROT_ASSERT_ARG(interp))
/* Don't modify between HEADERIZER BEGIN / HEADERIZER END.  Your changes will be lost. */
/* HEADERIZER END: static */

/*

=head1 Functions

=over 4

=item C<static void gc_boehm_mark_and_sweep(PARROT_INTERP, UINTVAL flags)>

This function would perform a GC run.

This function is called from the GC API function C<Parrot_gc_mark_and_sweep>.

Flags can be a combination of these values:

  GC_finish_FLAG
  GC_lazy_FLAG
  GC_trace_stack_FLAG

=cut

*/

static void
gc_boehm_mark_and_sweep(SHIM_INTERP, UINTVAL flags)
{
    ASSERT_ARGS(gc_boehm_mark_and_sweep)
    UNUSED(flags);
    GC_gcollect();
}

/*

=item C<static void gc_boehm_compact_memory_pool(PARROT_INTERP)>

Stub for compacting memory pools.

=cut

*/
static void
gc_boehm_compact_memory_pool(PARROT_INTERP)
{
}

/*

=item C<static PMC* gc_boehm_allocate_pmc_header(PARROT_INTERP, UINTVAL flags)>

=item C<static void gc_boehm_free_pmc_header(PARROT_INTERP, PMC *pmc)>

=item C<static STRING* gc_boehm_allocate_string_header(PARROT_INTERP, UINTVAL
flags)>

=item C<static void gc_boehm_free_string_header(PARROT_INTERP, STRING *s)>

=item C<static Buffer* gc_boehm_allocate_bufferlike_header(PARROT_INTERP, size_t
size)>

=item C<static void gc_boehm_free_bufferlike_header(PARROT_INTERP, Buffer *b,
size_t size)>

=item C<static void* gc_boehm_allocate_pmc_attributes(PARROT_INTERP, PMC *pmc)>

=item C<static void gc_boehm_free_pmc_attributes(PARROT_INTERP, PMC *pmc)>

=item C<static void gc_boehm_allocate_string_storage(PARROT_INTERP, STRING *str,
size_t size)>

=item C<static void gc_boehm_reallocate_string_storage(PARROT_INTERP, STRING
*str, size_t size)>

=item C<static void gc_boehm_allocate_buffer_storage(PARROT_INTERP, Buffer
*buffer, size_t size)>

=item C<static void gc_boehm_reallocate_buffer_storage(PARROT_INTERP, Buffer
*buffer, size_t size)>

=item C<static void* gc_boehm_allocate_fixed_size_storage(PARROT_INTERP, size_t
size)>

=item C<static void gc_boehm_free_fixed_size_storage(PARROT_INTERP, size_t size,
void *data)>

Functions for allocating/deallocating various objects.

*/

static PMC*
gc_boehm_allocate_pmc_header(PARROT_INTERP, UINTVAL flags)
{
    boehm_gc_data *d = (boehm_gc_data*)interp->gc_sys->gc_private;
    PMC *pmc = (PMC*)GC_malloc_explicitly_typed(sizeof(PMC), d->pmc_descriptor);
    //PMC *pmc = (PMC*)GC_MALLOC(sizeof(PMC));
    //GC_REGISTER_FINALIZER_NO_ORDER(pmc, gc_boehm_finalize_cb, interp, NULL, NULL);
    return pmc;
}

static void
gc_boehm_free_pmc_header(PARROT_INTERP, PMC *pmc)
{
    if (pmc) {
        /* Unregister finalizer */
        //GC_REGISTER_FINALIZER_NO_ORDER(pmc, NULL, interp, NULL, NULL);
        /* If PMC was destroyed manually - do nothing */
        if (!PObj_on_free_list_TEST(pmc))
            Parrot_pmc_destroy(interp, pmc);
        //GC_FREE(pmc);
    }
}


static STRING*
gc_boehm_allocate_string_header(PARROT_INTERP, UINTVAL flags)
{
    boehm_gc_data *d = (boehm_gc_data*)interp->gc_sys->gc_private;
    return (STRING*)GC_malloc_explicitly_typed(sizeof(STRING), d->string_descriptor);
}

static void
gc_boehm_free_string_header(PARROT_INTERP, STRING *s)
{
    if (s)
        GC_FREE(s);
}


static Buffer*
gc_boehm_allocate_bufferlike_header(PARROT_INTERP, size_t size)
{
    return (Buffer*)GC_MALLOC(sizeof(Buffer));
}

static void
gc_boehm_free_bufferlike_header(PARROT_INTERP, Buffer *b, size_t size)
{
    if (b)
        GC_FREE(b);
}


static void*
gc_boehm_allocate_pmc_attributes(PARROT_INTERP, PMC *pmc)
{
    const size_t attr_size = pmc->vtable->attr_size;
    PMC_data(pmc) = GC_MALLOC(attr_size);
    return PMC_data(pmc);
}

static void
gc_boehm_free_pmc_attributes(PARROT_INTERP, PMC *pmc)
{
    if (PMC_data(pmc))
        GC_FREE(PMC_data(pmc));
}


static void
gc_boehm_allocate_string_storage(PARROT_INTERP, STRING *str, size_t size)
{
    char *mem;

    Buffer_buflen(str)   = 0;
    Buffer_bufstart(str) = NULL;

    if (size == 0)
        return;

    mem      = (char *)GC_MALLOC_ATOMIC(size);

    Buffer_bufstart(str) = str->strstart = mem;
    Buffer_buflen(str)   = size;
}

static void
gc_boehm_reallocate_string_storage(PARROT_INTERP, STRING *str, size_t size)
{
    char *mem;

    mem      = (char *)GC_MALLOC_ATOMIC(size);
    strncpy(mem, str->strstart, str->strlen);

    Buffer_bufstart(str) = str->strstart = mem;
    Buffer_buflen(str)   = size;
}

static void
gc_boehm_allocate_buffer_storage(PARROT_INTERP, ARGMOD(Buffer *buffer), size_t size)
{
    char *mem;

    Buffer_buflen(buffer)   = 0;
    Buffer_bufstart(buffer) = NULL;

    if (size == 0)
        return;

    mem      = (char *)GC_MALLOC_ATOMIC(size);

    Buffer_bufstart(buffer) = mem;
    Buffer_buflen(buffer)   = size;
}

static void
gc_boehm_reallocate_buffer_storage(PARROT_INTERP, ARGMOD(Buffer *buffer), size_t size)
{
    char *mem;

    mem = (char *)GC_REALLOC(Buffer_bufstart(buffer), size);

    Buffer_bufstart(buffer) = mem;
    Buffer_buflen(buffer)   = size;
}


static void*
gc_boehm_allocate_fixed_size_storage(PARROT_INTERP, size_t size)
{
    return GC_MALLOC(size);
}

static void
gc_boehm_free_fixed_size_storage(PARROT_INTERP, size_t size, void *data)
{
    if (data)
        GC_FREE(data);
}

/*

=item C<static size_t gc_boehm_get_gc_info(PARROT_INTERP, Interpinfo_enum what)>

Stub for GC introspection function.

=cut

*/
static size_t
gc_boehm_get_gc_info(PARROT_INTERP, Interpinfo_enum what)
{
    return 0;
}

/*

=item C<static void gc_boehm_finalize_cb(void *obj, void *user_data)>

this function is passed to the finalizer

=cut

*/
static void
gc_boehm_finalize_cb(ARGIN(void *obj), ARGIN(void *user_data))
{
    ASSERT_ARGS(gc_boehm_finalize_cb)
    PMC           *pmc    = (PMC*)obj;
    Parrot_Interp  interp = (Parrot_Interp)user_data;

    /* If PMC was destroyed manually - do nothing */
    if (!PObj_on_free_list_TEST(pmc))
        Parrot_pmc_destroy(interp, pmc);
}



/*

=item C<void Parrot_gc_boehm_init(PARROT_INTERP)>

=cut

*/

void
Parrot_gc_boehm_init(PARROT_INTERP)
{
    ASSERT_ARGS(Parrot_gc_boehm_init)

    //GC_enable_incremental();
    //GC_time_limit = GC_TIME_UNLIMITED;

    /* Alias for gc_sys */
    GC_Subsystem *gc_sys = interp->gc_sys;

    boehm_gc_data *gc_private = (boehm_gc_data*)GC_MALLOC_ATOMIC(sizeof(boehm_gc_data));
    /* PMC layout */
    /* 11110 -> 0x1F */
    gc_private->pmc_layout = 0x1F;
    gc_private->pmc_descriptor = GC_make_descriptor(&gc_private->pmc_layout, 5);
    /* STRING layout */
    /* 000000010 -> 0x2 */
    gc_private->string_layout = 0x2;
    gc_private->string_descriptor = GC_make_descriptor(&gc_private->string_layout, 9);

    gc_sys->gc_private = gc_private;

    gc_sys->do_gc_mark         = gc_boehm_mark_and_sweep;
    gc_sys->finalize_gc_system = NULL;

    gc_sys->do_gc_mark              = gc_boehm_mark_and_sweep;
    gc_sys->compact_string_pool     = gc_boehm_compact_memory_pool;

    gc_sys->allocate_pmc_header     = gc_boehm_allocate_pmc_header;
    gc_sys->free_pmc_header         = gc_boehm_free_pmc_header;

    gc_sys->allocate_string_header  = gc_boehm_allocate_string_header;
    gc_sys->free_string_header      = gc_boehm_free_string_header;

    gc_sys->allocate_bufferlike_header  = gc_boehm_allocate_bufferlike_header;
    gc_sys->free_bufferlike_header      = gc_boehm_free_bufferlike_header;

    gc_sys->allocate_pmc_attributes = gc_boehm_allocate_pmc_attributes;
    gc_sys->free_pmc_attributes     = gc_boehm_free_pmc_attributes;

    gc_sys->allocate_string_storage = gc_boehm_allocate_string_storage;
    gc_sys->reallocate_string_storage = gc_boehm_reallocate_string_storage;

    gc_sys->allocate_buffer_storage = gc_boehm_allocate_buffer_storage;
    gc_sys->reallocate_buffer_storage = gc_boehm_reallocate_buffer_storage;

    gc_sys->allocate_fixed_size_storage = gc_boehm_allocate_fixed_size_storage;
    gc_sys->free_fixed_size_storage     = gc_boehm_free_fixed_size_storage;

    gc_sys->get_gc_info      = gc_boehm_get_gc_info;

}

#endif

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
