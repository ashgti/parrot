/*
Copyright (C) 2001-2009, Parrot Foundation.
$Id$

=head1 NAME

src/gc/gc_private.h - private header file for the GC subsystem

=head1 DESCRIPTION

This is a private header file for the GC subsystem. It contains definitions
that are only for use in the GC and don't need to be included in the rest of
Parrot.
*/

#ifndef PARROT_GC_PRIVATE_H_GUARD
#define PARROT_GC_PRIVATE_H_GUARD

#include "parrot/settings.h"

#if ! DISABLE_GC_DEBUG
/* Set when walking the system stack. Defined in src/gc/system.c */
extern int CONSERVATIVE_POINTER_CHASING;
#endif

#ifdef __ia64__

#  include <ucontext.h>
extern void *flush_reg_store(void);
#  define BACKING_STORE_BASE 0x80000fff80000000

#  ifdef __hpux
#    include <sys/pstat.h>
#    include <ia64/sys/inline.h>
#  endif /* __hpux */

#endif /* __ia64__ */

/* the percent of used Arena items at which to trace next time through */
#define GC_DEBUG_REPLENISH_LEVEL_FACTOR        0.0
#define GC_DEBUG_UNITS_PER_ALLOC_GROWTH_FACTOR 1
#define REPLENISH_LEVEL_FACTOR                 0.5

/* this factor is totally arbitrary, but gives good timings for stress.pasm */
#define UNITS_PER_ALLOC_GROWTH_FACTOR          1.75

#define POOL_MAX_BYTES                         65536 * 128

#define PMC_HEADERS_PER_ALLOC     4096 * 10 / sizeof (PMC)
#define BUFFER_HEADERS_PER_ALLOC  4096      / sizeof (Buffer)
#define STRING_HEADERS_PER_ALLOC  4096 * 20 / sizeof (STRING)

#define CONSTANT_PMC_HEADERS_PER_ALLOC 4096 / sizeof (PMC)
#define GET_SIZED_POOL_IDX(x) ((x) / sizeof (void *))
#define GC_NUM_INITIAL_FIXED_SIZE_POOLS 128


/* these values are used for the attribute allocator */
#define GC_ATTRIB_POOLS_HEADROOM 8
#define GC_FIXED_SIZE_POOL_SIZE 4096

/* Use the lazy allocator. Since it amortizes arena allocation costs, turn
   this on at the same time that you increase the size of allocated arenas.
   increase *_HEADERS_PER_ALLOC and GC_FIXED_SIZE_POOL_SIZE to be large
   enough to satisfy most startup costs. */
#define GC_USE_LAZY_ALLOCATOR 1

/* Set to 1 if we want to use the fixed-size allocator. Set to 0 if we want
   to allocate these things using mem_sys_allocate instead */
#define GC_USE_FIXED_SIZE_ALLOCATOR 1

/* We're using this here to add an additional pointer to a PObj without
   having to actually add an entire pointer to every PObj-alike structure
   in Parrot. Astute observers may notice that if the PObj is comprised of
   only an INTVAL, then there are some systems where sizeof(PObj*) can be
   larger then sizeof(PObj), thus creating overflow. However PObjs are never
   used by themselves, things like PMCs and STRINGs are cast to PObj in the
   GC, so we should have plenty of space. */
typedef struct GC_MS_PObj_Wrapper {
    size_t flags;
    struct GC_MS_PObj_Wrapper * next_ptr;
} GC_MS_PObj_Wrapper;


typedef enum _gc_sys_type_enum {
    MS,  /*mark and sweep*/
    INF /*infinite memory core*/
} gc_sys_type_enum;

typedef struct GC_Subsystem {
    /* Which GC subsystem are we using? See PARROT_GC_DEFAULT_TYPE in
     * include/parrot/settings.h for possible values */
    gc_sys_type_enum sys_type;

    /** Function hooks that each subsystem MUST provide */
    void (*do_gc_mark)(PARROT_INTERP, UINTVAL flags);
    void (*finalize_gc_system) (PARROT_INTERP);
    void (*init_pool)(PARROT_INTERP, struct Fixed_Size_Pool *);

    /*Function hooks that GC systems can CHOOSE to provide if they need them
     *These will be called via the GC API functions Parrot_gc_func_name
     *e.g. read barrier && write barrier hooks can go here later ...*/

    /* Holds system-specific data structures
     * unused right now, but this is where it should go if we need them ...
      union {
      } gc_private;
     */
} GC_Subsystem;

typedef struct Memory_Block {
    size_t free;
    size_t size;
    struct Memory_Block *prev;
    struct Memory_Block *next;
    char *start;
    char *top;
} Memory_Block;

typedef struct Variable_Size_Pool {
    Memory_Block *top_block;
    void (*compact)(PARROT_INTERP, struct Variable_Size_Pool *);
    size_t minimum_block_size;
    size_t total_allocated; /* total bytes allocated to this pool */
    size_t guaranteed_reclaimable;     /* bytes that can definitely be reclaimed*/
    size_t possibly_reclaimable;     /* bytes that can possibly be reclaimed
                                      * (above plus COW-freed bytes) */
    FLOATVAL reclaim_factor; /* minimum percentage we will reclaim */
} Variable_Size_Pool;

typedef struct Fixed_Size_Arena {
    size_t                     used;
    size_t                     total_objects;
    struct Fixed_Size_Arena *prev;
    struct Fixed_Size_Arena *next;
    void                      *start_objects;
} Fixed_Size_Arena;

typedef struct PMC_Attribute_Free_List {
    struct PMC_Attribute_Free_List * next;
} PMC_Attribute_Free_List;

typedef struct PMC_Attribute_Arena {
    struct PMC_Attribute_Arena * next;
    struct PMC_Attribute_Arena * prev;
} PMC_Attribute_Arena;

typedef struct PMC_Attribute_Pool {
    size_t attr_size;
    size_t total_objects;
    size_t objects_per_alloc;
    size_t num_free_objects;
    PMC_Attribute_Free_List * free_list;
    PMC_Attribute_Arena     * top_arena;
#if GC_USE_LAZY_ALLOCATOR
    PMC_Attribute_Free_List * newfree;
    PMC_Attribute_Free_List * newlast;
#endif
} PMC_Attribute_Pool;

/* Tracked resource pool */
typedef struct Fixed_Size_Pool {

    struct Variable_Size_Pool *mem_pool;
   /* Size in bytes of an individual pool item. This size may include
    * a GC-system specific GC header. */
    size_t object_size;

    size_t start_arena_memory;
    size_t end_arena_memory;

    Fixed_Size_Arena *last_Arena;
    GC_MS_PObj_Wrapper * free_list;
    size_t num_free_objects;    /* number of resources in the free pool */
    size_t total_objects;

    PARROT_OBSERVER const char *name;

    size_t objects_per_alloc;

    int skip;
    size_t replenish_level;

    add_free_object_fn_type     add_free_object; /* adds a free object to
                                                    the pool's free list  */
    get_free_object_fn_type     get_free_object; /* gets and removes a free
                                                    object from the pool's
                                                    free list */
    alloc_objects_fn_type       alloc_objects;  /* allocates more objects */
    alloc_objects_fn_type       more_objects;
    gc_object_fn_type           gc_object;

    /* Contains GC system-specific data structures ... unused at the moment,
     * but this is where it should go when we need it ...
    union {
    } gc_private;
    */

#if GC_USE_LAZY_ALLOCATOR
    void *newfree;
    void *newlast;
#endif

} Fixed_Size_Pool;

typedef struct Memory_Pools {
    Variable_Size_Pool *memory_pool;
    Variable_Size_Pool *constant_string_pool;
    struct Fixed_Size_Pool *string_header_pool;
    struct Fixed_Size_Pool *pmc_pool;
    struct Fixed_Size_Pool *constant_pmc_pool;
    struct Fixed_Size_Pool *constant_string_header_pool;
    struct Fixed_Size_Pool **sized_header_pools;
    size_t num_sized;

    PMC_Attribute_Pool **attrib_pools;
    size_t num_attribs;


    /** statistics for GC **/
    size_t  gc_mark_runs;       /* Number of times we've done a mark run*/
    size_t  gc_lazy_mark_runs;  /* Number of successful lazy mark runs */
    size_t  gc_collect_runs;    /* Number of times we've done a memory
                                   compaction */
    size_t  mem_allocs_since_last_collect;      /* The number of memory
                                                 * allocations from the
                                                 * system since the last
                                                 * compaction run */
    size_t  header_allocs_since_last_collect;   /* The number of header
                                                 * blocks allocated from
                                                 * the system since the last
                                                 * GC run */
    size_t  memory_allocated;     /* The total amount of
                                   * allocatable memory
                                   * allocated. Doesn't count
                                   * memory for headers or
                                   * internal structures or
                                   * anything */
    UINTVAL memory_collected;     /* Total amount of memory copied
                                     during collection */
    UINTVAL num_early_gc_PMCs;    /* how many PMCs want immediate destruction */
    UINTVAL num_early_PMCs_seen;  /* how many such PMCs has GC seen */
    PMC* gc_mark_start;           /* first PMC marked during a GC run */
    PMC* gc_mark_ptr;             /* last PMC marked during a GC run */
    PMC* gc_trace_ptr;            /* last PMC trace_children was called on */
    int lazy_gc;                  /* flag that indicates whether we should stop
                                     when we've seen all impatient PMCs */
    /*
     * GC blocking
     */
    UINTVAL gc_mark_block_level;  /* How many outstanding GC block
                                     requests are there? */
    UINTVAL gc_sweep_block_level; /* How many outstanding GC block
                                     requests are there? */
    /*
     * private data for the GC subsystem
     */
    void *  gc_private;           /* gc subsystem data */
} Memory_Pools;


/* HEADERIZER BEGIN: src/gc/system.c */
/* Don't modify between HEADERIZER BEGIN / HEADERIZER END.  Your changes will be lost. */

void trace_system_areas(PARROT_INTERP)
        __attribute__nonnull__(1);

#define ASSERT_ARGS_trace_system_areas __attribute__unused__ int _ASSERT_ARGS_CHECK = (\
       PARROT_ASSERT_ARG(interp))
/* Don't modify between HEADERIZER BEGIN / HEADERIZER END.  Your changes will be lost. */
/* HEADERIZER END: src/gc/system.c */

/* HEADERIZER BEGIN: src/gc/mark_sweep.c */
/* Don't modify between HEADERIZER BEGIN / HEADERIZER END.  Your changes will be lost. */

PARROT_WARN_UNUSED_RESULT
INTVAL contained_in_pool(PARROT_INTERP,
    ARGIN(const Fixed_Size_Pool *pool),
    ARGIN(const void *ptr))
        __attribute__nonnull__(1)
        __attribute__nonnull__(2)
        __attribute__nonnull__(3);

PARROT_WARN_UNUSED_RESULT
PARROT_CANNOT_RETURN_NULL
Fixed_Size_Pool * get_bufferlike_pool(PARROT_INTERP, size_t buffer_size)
        __attribute__nonnull__(1);

PARROT_IGNORABLE_RESULT
int /*@alt void@*/
header_pools_iterate_callback(PARROT_INTERP,
    int flag,
    ARGIN_NULLOK(void *arg),
    NOTNULL(pool_iter_fn func))
        __attribute__nonnull__(1)
        __attribute__nonnull__(4);

void initialize_fixed_size_pools(PARROT_INTERP)
        __attribute__nonnull__(1);

void mark_special(PARROT_INTERP, ARGIN(PMC *obj))
        __attribute__nonnull__(1)
        __attribute__nonnull__(2);

void Parrot_add_to_free_list(PARROT_INTERP,
    ARGMOD(Fixed_Size_Pool *pool),
    ARGMOD(Fixed_Size_Arena *arena))
        __attribute__nonnull__(1)
        __attribute__nonnull__(2)
        __attribute__nonnull__(3)
        FUNC_MODIFIES(*pool)
        FUNC_MODIFIES(*arena);

void Parrot_append_arena_in_pool(PARROT_INTERP,
    ARGMOD(Fixed_Size_Pool *pool),
    ARGMOD(Fixed_Size_Arena *new_arena),
    size_t size)
        __attribute__nonnull__(1)
        __attribute__nonnull__(2)
        __attribute__nonnull__(3)
        FUNC_MODIFIES(*pool)
        FUNC_MODIFIES(*new_arena);

void Parrot_gc_clear_live_bits(PARROT_INTERP,
    ARGIN(const Fixed_Size_Pool *pool))
        __attribute__nonnull__(1)
        __attribute__nonnull__(2);

PARROT_CANNOT_RETURN_NULL
PMC_Attribute_Pool * Parrot_gc_get_attribute_pool(PARROT_INTERP,
    size_t attrib_size)
        __attribute__nonnull__(1);

PARROT_CANNOT_RETURN_NULL
void * Parrot_gc_get_attributes_from_pool(PARROT_INTERP,
    ARGMOD(PMC_Attribute_Pool * pool))
        __attribute__nonnull__(1)
        __attribute__nonnull__(2)
        FUNC_MODIFIES(* pool);

void Parrot_gc_initialize_fixed_size_pools(PARROT_INTERP,
    size_t init_num_pools)
        __attribute__nonnull__(1);

void Parrot_gc_run_init(PARROT_INTERP)
        __attribute__nonnull__(1);

void Parrot_gc_sweep_pool(PARROT_INTERP, ARGMOD(Fixed_Size_Pool *pool))
        __attribute__nonnull__(1)
        __attribute__nonnull__(2)
        FUNC_MODIFIES(*pool);

int Parrot_gc_trace_root(PARROT_INTERP, Parrot_gc_trace_type trace)
        __attribute__nonnull__(1);

#define ASSERT_ARGS_contained_in_pool __attribute__unused__ int _ASSERT_ARGS_CHECK = (\
       PARROT_ASSERT_ARG(interp) \
    , PARROT_ASSERT_ARG(pool) \
    , PARROT_ASSERT_ARG(ptr))
#define ASSERT_ARGS_get_bufferlike_pool __attribute__unused__ int _ASSERT_ARGS_CHECK = (\
       PARROT_ASSERT_ARG(interp))
#define ASSERT_ARGS_header_pools_iterate_callback __attribute__unused__ int _ASSERT_ARGS_CHECK = (\
       PARROT_ASSERT_ARG(interp) \
    , PARROT_ASSERT_ARG(func))
#define ASSERT_ARGS_initialize_fixed_size_pools __attribute__unused__ int _ASSERT_ARGS_CHECK = (\
       PARROT_ASSERT_ARG(interp))
#define ASSERT_ARGS_mark_special __attribute__unused__ int _ASSERT_ARGS_CHECK = (\
       PARROT_ASSERT_ARG(interp) \
    , PARROT_ASSERT_ARG(obj))
#define ASSERT_ARGS_Parrot_add_to_free_list __attribute__unused__ int _ASSERT_ARGS_CHECK = (\
       PARROT_ASSERT_ARG(interp) \
    , PARROT_ASSERT_ARG(pool) \
    , PARROT_ASSERT_ARG(arena))
#define ASSERT_ARGS_Parrot_append_arena_in_pool __attribute__unused__ int _ASSERT_ARGS_CHECK = (\
       PARROT_ASSERT_ARG(interp) \
    , PARROT_ASSERT_ARG(pool) \
    , PARROT_ASSERT_ARG(new_arena))
#define ASSERT_ARGS_Parrot_gc_clear_live_bits __attribute__unused__ int _ASSERT_ARGS_CHECK = (\
       PARROT_ASSERT_ARG(interp) \
    , PARROT_ASSERT_ARG(pool))
#define ASSERT_ARGS_Parrot_gc_get_attribute_pool __attribute__unused__ int _ASSERT_ARGS_CHECK = (\
       PARROT_ASSERT_ARG(interp))
#define ASSERT_ARGS_Parrot_gc_get_attributes_from_pool \
     __attribute__unused__ int _ASSERT_ARGS_CHECK = (\
       PARROT_ASSERT_ARG(interp) \
    , PARROT_ASSERT_ARG(pool))
#define ASSERT_ARGS_Parrot_gc_initialize_fixed_size_pools \
     __attribute__unused__ int _ASSERT_ARGS_CHECK = (\
       PARROT_ASSERT_ARG(interp))
#define ASSERT_ARGS_Parrot_gc_run_init __attribute__unused__ int _ASSERT_ARGS_CHECK = (\
       PARROT_ASSERT_ARG(interp))
#define ASSERT_ARGS_Parrot_gc_sweep_pool __attribute__unused__ int _ASSERT_ARGS_CHECK = (\
       PARROT_ASSERT_ARG(interp) \
    , PARROT_ASSERT_ARG(pool))
#define ASSERT_ARGS_Parrot_gc_trace_root __attribute__unused__ int _ASSERT_ARGS_CHECK = (\
       PARROT_ASSERT_ARG(interp))
/* Don't modify between HEADERIZER BEGIN / HEADERIZER END.  Your changes will be lost. */
/* HEADERIZER END: src/gc/mark_sweep.c */

/* HEADERIZER BEGIN: src/gc/pools.c */
/* Don't modify between HEADERIZER BEGIN / HEADERIZER END.  Your changes will be lost. */

PARROT_WARN_UNUSED_RESULT
PARROT_CANNOT_RETURN_NULL
Fixed_Size_Pool * get_bufferlike_pool(PARROT_INTERP, size_t buffer_size)
        __attribute__nonnull__(1);

PARROT_IGNORABLE_RESULT
int /*@alt void@*/
header_pools_iterate_callback(PARROT_INTERP,
    int flag,
    ARGIN_NULLOK(void *arg),
    NOTNULL(pool_iter_fn func))
        __attribute__nonnull__(1)
        __attribute__nonnull__(4);

void initialize_fixed_size_pools(PARROT_INTERP)
        __attribute__nonnull__(1);

/* Don't modify between HEADERIZER BEGIN / HEADERIZER END.  Your changes will be lost. */
/* HEADERIZER END: src/gc/pools.c */

/* HEADERIZER BEGIN: src/gc/alloc_resources.c */
/* Don't modify between HEADERIZER BEGIN / HEADERIZER END.  Your changes will be lost. */

PARROT_CANNOT_RETURN_NULL
PARROT_WARN_UNUSED_RESULT
char * aligned_mem(ARGIN(const Buffer *buffer), ARGIN(char *mem))
        __attribute__nonnull__(1)
        __attribute__nonnull__(2);

PARROT_PURE_FUNCTION
PARROT_WARN_UNUSED_RESULT
size_t aligned_size(ARGIN(const Buffer *buffer), size_t len)
        __attribute__nonnull__(1);

PARROT_CONST_FUNCTION
PARROT_WARN_UNUSED_RESULT
size_t aligned_string_size(size_t len);

void check_buffer_ptr(
    ARGMOD(Buffer * pobj),
    ARGMOD(Variable_Size_Pool * pool))
        __attribute__nonnull__(1)
        __attribute__nonnull__(2)
        FUNC_MODIFIES(* pobj)
        FUNC_MODIFIES(* pool);

void compact_pool(PARROT_INTERP, ARGMOD(Variable_Size_Pool *pool))
        __attribute__nonnull__(1)
        __attribute__nonnull__(2)
        FUNC_MODIFIES(*pool);

void initialize_var_size_pools(PARROT_INTERP)
        __attribute__nonnull__(1);

PARROT_MALLOC
PARROT_CANNOT_RETURN_NULL
void * mem_allocate(PARROT_INTERP,
    size_t size,
    ARGMOD(Variable_Size_Pool *pool))
        __attribute__nonnull__(1)
        __attribute__nonnull__(3)
        FUNC_MODIFIES(*pool);

void merge_pools(
    ARGMOD(Variable_Size_Pool *dest),
    ARGMOD(Variable_Size_Pool *source))
        __attribute__nonnull__(1)
        __attribute__nonnull__(2)
        FUNC_MODIFIES(*dest)
        FUNC_MODIFIES(*source);

#define ASSERT_ARGS_aligned_mem __attribute__unused__ int _ASSERT_ARGS_CHECK = (\
       PARROT_ASSERT_ARG(buffer) \
    , PARROT_ASSERT_ARG(mem))
#define ASSERT_ARGS_aligned_size __attribute__unused__ int _ASSERT_ARGS_CHECK = (\
       PARROT_ASSERT_ARG(buffer))
#define ASSERT_ARGS_aligned_string_size __attribute__unused__ int _ASSERT_ARGS_CHECK = (0)
#define ASSERT_ARGS_check_buffer_ptr __attribute__unused__ int _ASSERT_ARGS_CHECK = (\
       PARROT_ASSERT_ARG(pobj) \
    , PARROT_ASSERT_ARG(pool))
#define ASSERT_ARGS_compact_pool __attribute__unused__ int _ASSERT_ARGS_CHECK = (\
       PARROT_ASSERT_ARG(interp) \
    , PARROT_ASSERT_ARG(pool))
#define ASSERT_ARGS_initialize_var_size_pools __attribute__unused__ int _ASSERT_ARGS_CHECK = (\
       PARROT_ASSERT_ARG(interp))
#define ASSERT_ARGS_mem_allocate __attribute__unused__ int _ASSERT_ARGS_CHECK = (\
       PARROT_ASSERT_ARG(interp) \
    , PARROT_ASSERT_ARG(pool))
#define ASSERT_ARGS_merge_pools __attribute__unused__ int _ASSERT_ARGS_CHECK = (\
       PARROT_ASSERT_ARG(dest) \
    , PARROT_ASSERT_ARG(source))
/* Don't modify between HEADERIZER BEGIN / HEADERIZER END.  Your changes will be lost. */
/* HEADERIZER END: src/gc/alloc_resources.c */

/* GC subsystem init functions */
/* HEADERIZER BEGIN: src/gc/gc_ms.c */
/* Don't modify between HEADERIZER BEGIN / HEADERIZER END.  Your changes will be lost. */

void Parrot_gc_ms_init(PARROT_INTERP)
        __attribute__nonnull__(1);

#define ASSERT_ARGS_Parrot_gc_ms_init __attribute__unused__ int _ASSERT_ARGS_CHECK = (\
       PARROT_ASSERT_ARG(interp))
/* Don't modify between HEADERIZER BEGIN / HEADERIZER END.  Your changes will be lost. */
/* HEADERIZER END: src/gc/gc_ms.c */

/* HEADERIZER BEGIN: src/gc/gc_inf.c */
/* Don't modify between HEADERIZER BEGIN / HEADERIZER END.  Your changes will be lost. */

void Parrot_gc_inf_init(PARROT_INTERP)
        __attribute__nonnull__(1);

#define ASSERT_ARGS_Parrot_gc_inf_init __attribute__unused__ int _ASSERT_ARGS_CHECK = (\
       PARROT_ASSERT_ARG(interp))
/* Don't modify between HEADERIZER BEGIN / HEADERIZER END.  Your changes will be lost. */
/* HEADERIZER END: src/gc/gc_inf.c */

#endif /* PARROT_GC_PRIVATE_H_GUARD */



/*
 * Local variables:
 *   c-file-style: "parrot"
 * End:
 * vim: expandtab shiftwidth=4:
 */
