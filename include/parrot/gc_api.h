/* gc_api.h
 *  Copyright (C) 2001-2009, Parrot Foundation.
 *  SVN Info
 *     $Id$
 *  Overview:
 *     Handles dead object destruction of the various headers
 *  History:
 *     Initial version by Mike Lambert on 2002.05.27
 */

#ifndef PARROT_GC_API_H_GUARD
#define PARROT_GC_API_H_GUARD

#include "parrot/parrot.h"

/*
 * we need an alignment that is the same as malloc(3) have for
 * allocating Buffer items like FLOATVAL (double)
 * This should be either a config hint or tested
 */
#ifdef MALLOC_ALIGNMENT
#  define BUFFER_ALIGNMENT MALLOC_ALIGNMENT
#else
/* or (2 * sizeof (size_t)) */
#  define BUFFER_ALIGNMENT 8
#endif

#define BUFFER_ALIGN_1 (BUFFER_ALIGNMENT - 1)
#define BUFFER_ALIGN_MASK ~BUFFER_ALIGN_1

#define WORD_ALIGN_1 (sizeof (void *) - 1)
#define WORD_ALIGN_MASK ~WORD_ALIGN_1

/* pool iteration */
typedef enum {
    POOL_PMC    = 0x01,
    POOL_BUFFER = 0x02,
    POOL_CONST  = 0x04,
    POOL_ALL    = 0x07
} pool_iter_enum;

struct Memory_Block;
struct Var_Size_Pool;
struct Fixed_Size_Pool;
struct Fixed_Size_Arena;
struct Memory_Pools;

typedef enum {
    GC_TRACE_FULL        = 1,
    GC_TRACE_ROOT_ONLY   = 2,
    GC_TRACE_SYSTEM_ONLY = 3
} Parrot_gc_trace_type;

typedef int (*pool_iter_fn)(PARROT_INTERP, struct Fixed_Size_Pool *, int, void*);
typedef void (*add_free_object_fn_type)(PARROT_INTERP, struct Fixed_Size_Pool *, void *);
typedef void * (*get_free_object_fn_type)(PARROT_INTERP, struct Fixed_Size_Pool *);
typedef void (*alloc_objects_fn_type)(PARROT_INTERP, struct Fixed_Size_Pool *);
typedef void (*gc_object_fn_type)(PARROT_INTERP, struct Fixed_Size_Pool *, PObj *);


/* &gen_from_enum(interpinfo.pasm) prefix(INTERPINFO_) */

typedef enum {
    TOTAL_MEM_ALLOC = 1,
    GC_MARK_RUNS,
    GC_COLLECT_RUNS,
    ACTIVE_PMCS,
    ACTIVE_BUFFERS,
    TOTAL_PMCS,
    TOTAL_BUFFERS,
    HEADER_ALLOCS_SINCE_COLLECT,
    MEM_ALLOCS_SINCE_COLLECT,
    TOTAL_COPIED,
    IMPATIENT_PMCS,
    GC_LAZY_MARK_RUNS,
    EXTENDED_PMCS,
    CURRENT_RUNCORE,

    /* interpinfo_p constants */
    CURRENT_SUB,
    CURRENT_CONT,
    CURRENT_OBJECT,
    CURRENT_LEXPAD,

    /* interpinfo_s constants */
    EXECUTABLE_FULLNAME,
    EXECUTABLE_BASENAME,
    RUNTIME_PREFIX
} Interpinfo_enum;

/* &end_gen */

#define GC_trace_stack_FLAG    (UINTVAL)(1 << 0)   /* trace system areas and stack */
#define GC_trace_normal        (UINTVAL)(1 << 0)   /* the same */
#define GC_lazy_FLAG           (UINTVAL)(1 << 1)   /* timely destruction run */
#define GC_finish_FLAG         (UINTVAL)(1 << 2)   /* on Parrot exit: mark (almost) all PMCs dead and */
                                                   /* garbage collect. */

/* HEADERIZER BEGIN: src/gc/api.c */
/* Don't modify between HEADERIZER BEGIN / HEADERIZER END.  Your changes will be lost. */

PARROT_EXPORT
void Parrot_block_GC_mark(PARROT_INTERP)
        __attribute__nonnull__(1);

PARROT_EXPORT
void Parrot_block_GC_sweep(PARROT_INTERP)
        __attribute__nonnull__(1);

PARROT_EXPORT
void Parrot_gc_mark_PMC_alive_fun(PARROT_INTERP, ARGMOD_NULLOK(PMC *obj))
        __attribute__nonnull__(1)
        FUNC_MODIFIES(*obj);

PARROT_EXPORT
void Parrot_gc_mark_PObj_alive(PARROT_INTERP, ARGMOD(PObj *obj))
        __attribute__nonnull__(1)
        __attribute__nonnull__(2)
        FUNC_MODIFIES(*obj);

PARROT_EXPORT
void Parrot_gc_mark_STRING_alive_fun(PARROT_INTERP,
    ARGMOD_NULLOK(STRING *obj))
        __attribute__nonnull__(1)
        FUNC_MODIFIES(*obj);

PARROT_EXPORT
unsigned int Parrot_is_blocked_GC_mark(PARROT_INTERP)
        __attribute__nonnull__(1);

PARROT_EXPORT
unsigned int Parrot_is_blocked_GC_sweep(PARROT_INTERP)
        __attribute__nonnull__(1);

PARROT_EXPORT
void Parrot_unblock_GC_mark(PARROT_INTERP)
        __attribute__nonnull__(1);

PARROT_EXPORT
void Parrot_unblock_GC_sweep(PARROT_INTERP)
        __attribute__nonnull__(1);

int Parrot_gc_active_pmcs(PARROT_INTERP)
        __attribute__nonnull__(1);

int Parrot_gc_active_sized_buffers(PARROT_INTERP)
        __attribute__nonnull__(1);

void Parrot_gc_add_pmc_sync(PARROT_INTERP, ARGMOD(PMC *pmc))
        __attribute__nonnull__(1)
        __attribute__nonnull__(2)
        FUNC_MODIFIES(*pmc);

void Parrot_gc_allocate_buffer_storage_aligned(PARROT_INTERP,
    ARGOUT(Buffer *buffer),
    size_t size)
        __attribute__nonnull__(1)
        __attribute__nonnull__(2)
        FUNC_MODIFIES(*buffer);

PARROT_CANNOT_RETURN_NULL
void * Parrot_gc_allocate_fixed_size_storage(PARROT_INTERP, size_t size)
        __attribute__nonnull__(1);

PARROT_CANNOT_RETURN_NULL
void * Parrot_gc_allocate_pmc_attributes(PARROT_INTERP, ARGMOD(PMC *pmc))
        __attribute__nonnull__(1)
        __attribute__nonnull__(2)
        FUNC_MODIFIES(*pmc);

void Parrot_gc_allocate_string_storage(PARROT_INTERP,
    ARGOUT(STRING *str),
    size_t size)
        __attribute__nonnull__(1)
        __attribute__nonnull__(2)
        FUNC_MODIFIES(*str);

void Parrot_gc_compact_memory_pool(PARROT_INTERP)
        __attribute__nonnull__(1);

void Parrot_gc_completely_unblock(PARROT_INTERP)
        __attribute__nonnull__(1);

size_t Parrot_gc_count_collect_runs(PARROT_INTERP)
        __attribute__nonnull__(1);

size_t Parrot_gc_count_lazy_mark_runs(PARROT_INTERP)
        __attribute__nonnull__(1);

size_t Parrot_gc_count_mark_runs(PARROT_INTERP)
        __attribute__nonnull__(1);

void Parrot_gc_destroy_header_pools(PARROT_INTERP)
        __attribute__nonnull__(1);

void Parrot_gc_destroy_memory_pools(PARROT_INTERP)
        __attribute__nonnull__(1);

void Parrot_gc_finalize(PARROT_INTERP)
        __attribute__nonnull__(1);

void Parrot_gc_free_bufferlike_header(PARROT_INTERP,
    ARGMOD(Buffer *obj),
    size_t size)
        __attribute__nonnull__(1)
        __attribute__nonnull__(2)
        FUNC_MODIFIES(*obj);

void Parrot_gc_free_fixed_size_storage(PARROT_INTERP,
    size_t size,
    ARGMOD(void *data))
        __attribute__nonnull__(1)
        __attribute__nonnull__(3)
        FUNC_MODIFIES(*data);

void Parrot_gc_free_pmc_attributes(PARROT_INTERP, ARGMOD(PMC *pmc))
        __attribute__nonnull__(1)
        __attribute__nonnull__(2)
        FUNC_MODIFIES(*pmc);

void Parrot_gc_free_pmc_header(PARROT_INTERP, ARGMOD(PMC *pmc))
        __attribute__nonnull__(1)
        __attribute__nonnull__(2)
        FUNC_MODIFIES(*pmc);

void Parrot_gc_free_pmc_sync(PARROT_INTERP, ARGMOD(PMC *p))
        __attribute__nonnull__(1)
        __attribute__nonnull__(2)
        FUNC_MODIFIES(*p);

void Parrot_gc_free_string_header(PARROT_INTERP, ARGMOD(STRING *s))
        __attribute__nonnull__(1)
        __attribute__nonnull__(2)
        FUNC_MODIFIES(*s);

int Parrot_gc_get_pmc_index(PARROT_INTERP, ARGIN(PMC* pmc))
        __attribute__nonnull__(1)
        __attribute__nonnull__(2);

size_t Parrot_gc_headers_alloc_since_last_collect(PARROT_INTERP)
        __attribute__nonnull__(1);

UINTVAL Parrot_gc_impatient_pmcs(PARROT_INTERP)
        __attribute__nonnull__(1);

void Parrot_gc_initialize(PARROT_INTERP, ARGIN(void *stacktop))
        __attribute__nonnull__(1)
        __attribute__nonnull__(2);

void Parrot_gc_mark_and_sweep(PARROT_INTERP, UINTVAL flags)
        __attribute__nonnull__(1);

size_t Parrot_gc_mem_alloc_since_last_collect(PARROT_INTERP)
        __attribute__nonnull__(1);

void Parrot_gc_merge_header_pools(
    ARGMOD(Interp *dest_interp),
    ARGIN(Interp *source_interp))
        __attribute__nonnull__(1)
        __attribute__nonnull__(2)
        FUNC_MODIFIES(*dest_interp);

PARROT_CANNOT_RETURN_NULL
PARROT_WARN_UNUSED_RESULT
void * Parrot_gc_new_bufferlike_header(PARROT_INTERP, size_t size)
        __attribute__nonnull__(1);

PARROT_WARN_UNUSED_RESULT
PARROT_CANNOT_RETURN_NULL
PMC * Parrot_gc_new_pmc_header(PARROT_INTERP, UINTVAL flags)
        __attribute__nonnull__(1);

PARROT_CANNOT_RETURN_NULL
PARROT_WARN_UNUSED_RESULT
STRING * Parrot_gc_new_string_header(PARROT_INTERP, UINTVAL flags)
        __attribute__nonnull__(1);

void Parrot_gc_pmc_needs_early_collection(PARROT_INTERP, ARGMOD(PMC *pmc))
        __attribute__nonnull__(1)
        __attribute__nonnull__(2)
        FUNC_MODIFIES(*pmc);

PARROT_WARN_UNUSED_RESULT
int Parrot_gc_ptr_in_memory_pool(PARROT_INTERP, ARGIN(void *bufstart))
        __attribute__nonnull__(1)
        __attribute__nonnull__(2);

int Parrot_gc_ptr_is_pmc(PARROT_INTERP, ARGIN(void *ptr))
        __attribute__nonnull__(1)
        __attribute__nonnull__(2);

void Parrot_gc_reallocate_buffer_storage(PARROT_INTERP,
    ARGMOD(Buffer *buffer),
    size_t newsize)
        __attribute__nonnull__(1)
        __attribute__nonnull__(2)
        FUNC_MODIFIES(*buffer);

void Parrot_gc_reallocate_string_storage(PARROT_INTERP,
    ARGMOD(STRING *str),
    size_t newsize)
        __attribute__nonnull__(1)
        __attribute__nonnull__(2)
        FUNC_MODIFIES(*str);

UINTVAL Parrot_gc_total_copied(PARROT_INTERP)
        __attribute__nonnull__(1);

size_t Parrot_gc_total_memory_allocated(PARROT_INTERP)
        __attribute__nonnull__(1);

int Parrot_gc_total_pmcs(PARROT_INTERP)
        __attribute__nonnull__(1);

int Parrot_gc_total_sized_buffers(PARROT_INTERP)
        __attribute__nonnull__(1);

#define ASSERT_ARGS_Parrot_block_GC_mark __attribute__unused__ int _ASSERT_ARGS_CHECK = (\
       PARROT_ASSERT_ARG(interp))
#define ASSERT_ARGS_Parrot_block_GC_sweep __attribute__unused__ int _ASSERT_ARGS_CHECK = (\
       PARROT_ASSERT_ARG(interp))
#define ASSERT_ARGS_Parrot_gc_mark_PMC_alive_fun __attribute__unused__ int _ASSERT_ARGS_CHECK = (\
       PARROT_ASSERT_ARG(interp))
#define ASSERT_ARGS_Parrot_gc_mark_PObj_alive __attribute__unused__ int _ASSERT_ARGS_CHECK = (\
       PARROT_ASSERT_ARG(interp) \
    , PARROT_ASSERT_ARG(obj))
#define ASSERT_ARGS_Parrot_gc_mark_STRING_alive_fun \
     __attribute__unused__ int _ASSERT_ARGS_CHECK = (\
       PARROT_ASSERT_ARG(interp))
#define ASSERT_ARGS_Parrot_is_blocked_GC_mark __attribute__unused__ int _ASSERT_ARGS_CHECK = (\
       PARROT_ASSERT_ARG(interp))
#define ASSERT_ARGS_Parrot_is_blocked_GC_sweep __attribute__unused__ int _ASSERT_ARGS_CHECK = (\
       PARROT_ASSERT_ARG(interp))
#define ASSERT_ARGS_Parrot_unblock_GC_mark __attribute__unused__ int _ASSERT_ARGS_CHECK = (\
       PARROT_ASSERT_ARG(interp))
#define ASSERT_ARGS_Parrot_unblock_GC_sweep __attribute__unused__ int _ASSERT_ARGS_CHECK = (\
       PARROT_ASSERT_ARG(interp))
#define ASSERT_ARGS_Parrot_gc_active_pmcs __attribute__unused__ int _ASSERT_ARGS_CHECK = (\
       PARROT_ASSERT_ARG(interp))
#define ASSERT_ARGS_Parrot_gc_active_sized_buffers \
     __attribute__unused__ int _ASSERT_ARGS_CHECK = (\
       PARROT_ASSERT_ARG(interp))
#define ASSERT_ARGS_Parrot_gc_add_pmc_sync __attribute__unused__ int _ASSERT_ARGS_CHECK = (\
       PARROT_ASSERT_ARG(interp) \
    , PARROT_ASSERT_ARG(pmc))
#define ASSERT_ARGS_Parrot_gc_allocate_buffer_storage_aligned \
     __attribute__unused__ int _ASSERT_ARGS_CHECK = (\
       PARROT_ASSERT_ARG(interp) \
    , PARROT_ASSERT_ARG(buffer))
#define ASSERT_ARGS_Parrot_gc_allocate_fixed_size_storage \
     __attribute__unused__ int _ASSERT_ARGS_CHECK = (\
       PARROT_ASSERT_ARG(interp))
#define ASSERT_ARGS_Parrot_gc_allocate_pmc_attributes \
     __attribute__unused__ int _ASSERT_ARGS_CHECK = (\
       PARROT_ASSERT_ARG(interp) \
    , PARROT_ASSERT_ARG(pmc))
#define ASSERT_ARGS_Parrot_gc_allocate_string_storage \
     __attribute__unused__ int _ASSERT_ARGS_CHECK = (\
       PARROT_ASSERT_ARG(interp) \
    , PARROT_ASSERT_ARG(str))
#define ASSERT_ARGS_Parrot_gc_compact_memory_pool __attribute__unused__ int _ASSERT_ARGS_CHECK = (\
       PARROT_ASSERT_ARG(interp))
#define ASSERT_ARGS_Parrot_gc_completely_unblock __attribute__unused__ int _ASSERT_ARGS_CHECK = (\
       PARROT_ASSERT_ARG(interp))
#define ASSERT_ARGS_Parrot_gc_count_collect_runs __attribute__unused__ int _ASSERT_ARGS_CHECK = (\
       PARROT_ASSERT_ARG(interp))
#define ASSERT_ARGS_Parrot_gc_count_lazy_mark_runs \
     __attribute__unused__ int _ASSERT_ARGS_CHECK = (\
       PARROT_ASSERT_ARG(interp))
#define ASSERT_ARGS_Parrot_gc_count_mark_runs __attribute__unused__ int _ASSERT_ARGS_CHECK = (\
       PARROT_ASSERT_ARG(interp))
#define ASSERT_ARGS_Parrot_gc_destroy_header_pools \
     __attribute__unused__ int _ASSERT_ARGS_CHECK = (\
       PARROT_ASSERT_ARG(interp))
#define ASSERT_ARGS_Parrot_gc_destroy_memory_pools \
     __attribute__unused__ int _ASSERT_ARGS_CHECK = (\
       PARROT_ASSERT_ARG(interp))
#define ASSERT_ARGS_Parrot_gc_finalize __attribute__unused__ int _ASSERT_ARGS_CHECK = (\
       PARROT_ASSERT_ARG(interp))
#define ASSERT_ARGS_Parrot_gc_free_bufferlike_header \
     __attribute__unused__ int _ASSERT_ARGS_CHECK = (\
       PARROT_ASSERT_ARG(interp) \
    , PARROT_ASSERT_ARG(obj))
#define ASSERT_ARGS_Parrot_gc_free_fixed_size_storage \
     __attribute__unused__ int _ASSERT_ARGS_CHECK = (\
       PARROT_ASSERT_ARG(interp) \
    , PARROT_ASSERT_ARG(data))
#define ASSERT_ARGS_Parrot_gc_free_pmc_attributes __attribute__unused__ int _ASSERT_ARGS_CHECK = (\
       PARROT_ASSERT_ARG(interp) \
    , PARROT_ASSERT_ARG(pmc))
#define ASSERT_ARGS_Parrot_gc_free_pmc_header __attribute__unused__ int _ASSERT_ARGS_CHECK = (\
       PARROT_ASSERT_ARG(interp) \
    , PARROT_ASSERT_ARG(pmc))
#define ASSERT_ARGS_Parrot_gc_free_pmc_sync __attribute__unused__ int _ASSERT_ARGS_CHECK = (\
       PARROT_ASSERT_ARG(interp) \
    , PARROT_ASSERT_ARG(p))
#define ASSERT_ARGS_Parrot_gc_free_string_header __attribute__unused__ int _ASSERT_ARGS_CHECK = (\
       PARROT_ASSERT_ARG(interp) \
    , PARROT_ASSERT_ARG(s))
#define ASSERT_ARGS_Parrot_gc_get_pmc_index __attribute__unused__ int _ASSERT_ARGS_CHECK = (\
       PARROT_ASSERT_ARG(interp) \
    , PARROT_ASSERT_ARG(pmc))
#define ASSERT_ARGS_Parrot_gc_headers_alloc_since_last_collect \
     __attribute__unused__ int _ASSERT_ARGS_CHECK = (\
       PARROT_ASSERT_ARG(interp))
#define ASSERT_ARGS_Parrot_gc_impatient_pmcs __attribute__unused__ int _ASSERT_ARGS_CHECK = (\
       PARROT_ASSERT_ARG(interp))
#define ASSERT_ARGS_Parrot_gc_initialize __attribute__unused__ int _ASSERT_ARGS_CHECK = (\
       PARROT_ASSERT_ARG(interp) \
    , PARROT_ASSERT_ARG(stacktop))
#define ASSERT_ARGS_Parrot_gc_mark_and_sweep __attribute__unused__ int _ASSERT_ARGS_CHECK = (\
       PARROT_ASSERT_ARG(interp))
#define ASSERT_ARGS_Parrot_gc_mem_alloc_since_last_collect \
     __attribute__unused__ int _ASSERT_ARGS_CHECK = (\
       PARROT_ASSERT_ARG(interp))
#define ASSERT_ARGS_Parrot_gc_merge_header_pools __attribute__unused__ int _ASSERT_ARGS_CHECK = (\
       PARROT_ASSERT_ARG(dest_interp) \
    , PARROT_ASSERT_ARG(source_interp))
#define ASSERT_ARGS_Parrot_gc_new_bufferlike_header \
     __attribute__unused__ int _ASSERT_ARGS_CHECK = (\
       PARROT_ASSERT_ARG(interp))
#define ASSERT_ARGS_Parrot_gc_new_pmc_header __attribute__unused__ int _ASSERT_ARGS_CHECK = (\
       PARROT_ASSERT_ARG(interp))
#define ASSERT_ARGS_Parrot_gc_new_string_header __attribute__unused__ int _ASSERT_ARGS_CHECK = (\
       PARROT_ASSERT_ARG(interp))
#define ASSERT_ARGS_Parrot_gc_pmc_needs_early_collection \
     __attribute__unused__ int _ASSERT_ARGS_CHECK = (\
       PARROT_ASSERT_ARG(interp) \
    , PARROT_ASSERT_ARG(pmc))
#define ASSERT_ARGS_Parrot_gc_ptr_in_memory_pool __attribute__unused__ int _ASSERT_ARGS_CHECK = (\
       PARROT_ASSERT_ARG(interp) \
    , PARROT_ASSERT_ARG(bufstart))
#define ASSERT_ARGS_Parrot_gc_ptr_is_pmc __attribute__unused__ int _ASSERT_ARGS_CHECK = (\
       PARROT_ASSERT_ARG(interp) \
    , PARROT_ASSERT_ARG(ptr))
#define ASSERT_ARGS_Parrot_gc_reallocate_buffer_storage \
     __attribute__unused__ int _ASSERT_ARGS_CHECK = (\
       PARROT_ASSERT_ARG(interp) \
    , PARROT_ASSERT_ARG(buffer))
#define ASSERT_ARGS_Parrot_gc_reallocate_string_storage \
     __attribute__unused__ int _ASSERT_ARGS_CHECK = (\
       PARROT_ASSERT_ARG(interp) \
    , PARROT_ASSERT_ARG(str))
#define ASSERT_ARGS_Parrot_gc_total_copied __attribute__unused__ int _ASSERT_ARGS_CHECK = (\
       PARROT_ASSERT_ARG(interp))
#define ASSERT_ARGS_Parrot_gc_total_memory_allocated \
     __attribute__unused__ int _ASSERT_ARGS_CHECK = (\
       PARROT_ASSERT_ARG(interp))
#define ASSERT_ARGS_Parrot_gc_total_pmcs __attribute__unused__ int _ASSERT_ARGS_CHECK = (\
       PARROT_ASSERT_ARG(interp))
#define ASSERT_ARGS_Parrot_gc_total_sized_buffers __attribute__unused__ int _ASSERT_ARGS_CHECK = (\
       PARROT_ASSERT_ARG(interp))
/* Don't modify between HEADERIZER BEGIN / HEADERIZER END.  Your changes will be lost. */
/* HEADERIZER END: src/gc/api.c */

void Parrot_gc_inf_init(PARROT_INTERP);

#if defined(NDEBUG) && defined(PARROT_IN_CORE)
#  define Parrot_gc_mark_STRING_alive(interp, obj) \
          do if (! STRING_IS_NULL(obj)) PObj_live_SET(obj); while (0)
#else
#  define Parrot_gc_mark_STRING_alive(interp, obj) Parrot_gc_mark_STRING_alive_fun((interp), (obj))
#endif

#if defined(PARROT_IN_CORE)
#  define Parrot_gc_mark_PMC_alive(interp, obj) \
      do if (!PMC_IS_NULL(obj) && !PObj_live_TEST(obj)) Parrot_gc_mark_PMC_alive_fun((interp), (obj)); \
      while (0)
#else
#  define Parrot_gc_mark_PMC_alive(interp, obj) Parrot_gc_mark_PMC_alive_fun((interp), (obj))
#endif

#endif /* PARROT_GC_API_H_GUARD */

/*
 * Local variables:
 *   c-file-style: "parrot"
 * End:
 * vim: expandtab shiftwidth=4:
 */
