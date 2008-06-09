/*
 * $Id$
 * Copyright (C) 2002-2008, The Perl Foundation
 */

#ifndef PARROT_SMALLOBJECT_H_GUARD
#define PARROT_SMALLOBJECT_H_GUARD

#  include "parrot/parrot.h"

typedef struct Small_Object_Arena {
    size_t                     used;
    size_t                     total_objects;
    struct Small_Object_Arena *prev;
    struct Small_Object_Arena *next;
    void                      *start_objects;
} Small_Object_Arena;

struct Small_Object_Pool;

typedef void (*add_free_object_fn_type)(PARROT_INTERP, struct Small_Object_Pool *, void *);
typedef void * (*get_free_object_fn_type)(PARROT_INTERP, struct Small_Object_Pool *);
typedef void (*alloc_objects_fn_type)(PARROT_INTERP, struct Small_Object_Pool *);
typedef void (*dod_object_fn_type)(PARROT_INTERP, struct Small_Object_Pool *, PObj *);

#if PARROT_GC_GMS
/*
 * all objects have this header in front of the actual
 * object pointer. The prev/next pointers chain all existing
 * objects for one pool (sizeclass) together.
 *
 * XXX this could lead to unaligned FLOATVALs in the adjacent PMC
 *     if that's true either insert a dummy or reorder PMC members
 *     ??? How is that possible?
 */
typedef struct _gc_gms_hdr {
    struct _gc_gms_hdr *prev;
    struct _gc_gms_hdr *next;
    struct _gc_gms_gen *gen;
    void *gc_dummy_align;       /* see above */
} Gc_gms_hdr;

#  define PObj_to_GMSH(o) (((Gc_gms_hdr*)(o))-1)
#  define GMSH_to_PObj(p) ((PObj*) ((p)+1))

/* the structure uses 2 ptrs itself */
#  define GC_GMS_STORE_SIZE (64-2)

typedef struct _gc_gms_hdr_store {
    struct _gc_gms_hdr_store *next;
    Gc_gms_hdr **ptr;                           /* insert location */
    Gc_gms_hdr * (store[GC_GMS_STORE_SIZE]);    /* array of hdr pointers */
} Gc_gms_hdr_store;

typedef struct _gc_gms_hdr_list {
    Gc_gms_hdr_store *first;
    Gc_gms_hdr_store *last;
} Gc_gms_hdr_list;


/*
 * all objects belong to one generation
 */
typedef struct _gc_gms_gen {
    UINTVAL gen_no;                     /* generation number */
    UINTVAL timely_destruct_obj_sofar;  /* sum up to this generation */
    UINTVAL black_color;                /* live color of this generation */
    struct _gc_gms_hdr *first;          /* first header in this generation */
    struct _gc_gms_hdr *last;           /* last header in this generation */
    struct _gc_gms_hdr *fin;            /* need destruction/finalization */
    struct Small_Object_Pool *pool;     /* where this generation belongs to */
    Gc_gms_hdr_list igp;                /* IGPs for this generation */
    UINTVAL n_possibly_dead;            /* overwritten count */
    UINTVAL n_objects;                  /* live objects count */
    struct _gc_gms_gen *prev;
    struct _gc_gms_gen *next;
} Gc_gms_gen;

#endif /* PARROT_GC_GMS */

#if PARROT_GC_IT /* Incremental Tricolor Garbage Collector, PDD09 */

/*
 * Header, a linked list.
 I don't know whether it should be single or double-linked.
 We don't keep track of headers when we aren't marking, so
 they can move freely (so long as the arena can still find them)
 When marking, we add them to the grey list like a queue, and
 remove them from the queue when they are marked. Once marked
 (or if left unmarked), the headers aren't used and don't need
 to be linked together (i don't think).
 */
typedef struct _gc_it_hdr {
    /* struct _gc_it_hdr *prev; */
    struct _gc_it_hdr *next;
    void *data_ptr;
} Gc_it_hdr;

/*
 * a basic structure to facilitate cardmarking.
 * Don't know if I want to make the array a fixed size or not.
 */

typedef struct _gc_it_card {
    UNITVAL * cards;
} Gc_it_card;

/*
 * Here are a few macros and constants defined for us, although
 * they will probably need to change
 */

#define GC_IT_FLAGS_PER_CARD    (sizeof(UINTVAL) * 4)
#define GC_IT_SET_FLAG(n, f)    ((f) << (n) * 2)
#define GC_IT_MARK_WHITE (0x0)
#define GC_IT_MARK_GREY  (0x1)
#define GC_IT_MARK_BLACK (0x3)

#define PObj_to_IT_HDR(o) (((Gc_it_hdr*)(o))-1)
#define IT_HDR_to_PObj(p) ((PObj*) ((p)+1))

/*
 * Structure to define individual generations.
 * Initially borrowed from the GC_GMS
 */

typedef struct _gc_it_gen {
    UINTVAL gen_no;                     /* generation number */
    UINTVAL timely_destruct_obj_sofar;  /* sum up to this generation */
    struct _gc_it_hdr *first;           /* first header in this generation */
    struct _gc_it_hdr *last;            /* last header in this generation */
    struct _gc_it_hdr *finalize;        /* need destruction/finalization */
    struct Small_Object_Pool *pool;     /* where this generation belongs to */
    Gc_gms_hdr_list igp;                /* IGPs for this generation */
    UINTVAL n_possibly_dead;            /* overwritten count */
    UINTVAL n_objects;                  /* live objects count */
    struct _gc_gms_gen *prev;
    struct _gc_gms_gen *next;
} Gc_gms_gen;

/*
 * Other structures to help with generational capabilities.
 * Borrowed from GC_GMS, initially.
 */

#  define GC_IT_STORE_SIZE (64-2)

typedef struct _gc_it_hdr_store {
    struct _gc_it_hdr_store *next;
    Gc_it_hdr **ptr;                           /* insert location */
    Gc_it_hdr * (store[GC_IT_STORE_SIZE]);     /* array of hdr pointers */
} Gc_it_hdr_store;

typedef struct _gc_it_hdr_list {
    Gc_it_hdr_store *first;
    Gc_it_hdr_store *last;
} Gc_it_hdr_list;

/*
 * Gc_it_data structure
 * Contains information for the IT GC to operate.
 * global, inserted into the Arenas structure
 * With the exception of the STOP flag, this could easily be some kind of
 * enum.
 */

#define GC_IT_FLAG_NEW_MARK      0x01    /* starting a new scan, from scratch */
#define GC_IT_FLAG_RESUME_MARK   0x02    /* resuming a scan in the middle */
#define GC_IT_FLAG_NEW_SWEEP     0x04    /* reached the end of the mark, begin sweep */
#define GC_IT_FLAG_RESUME_SWEEP  0x08    /* resume sweep phase */
#define GC_IT_FLAG_STOP          0x10    /* indicator to stop in the middle */

/* A private datastructure for the GC. All the global data that we need to
   operate will be stored here. */

typedef struct _gc_it_data {
    UINTVAL item_count;       /* number of items scanned in current run */
    UINTVAL total_count;      /* number of items scanned since beginning of mark phase */
    UINTVAL num_generations;  /* number of generations */
    UINTVAL state;            /* status of the current run */
    UINTVAL flags;            /* flag to determine when the trace needs to pause */
    UINTVAL config;           /* config data to tell how the GC operates */
} Gc_it_data;

/*
 * GC data per pool. Contains information that the GC needs in every pool.
 * This structure is changing rapidly, and some of the things in it are
 * probably unnecessary.
 */
typedef struct _gc_it_pool_data {
    Gc_it_gen * first_gen;  /* linked list of generations, youngest first, i assume */
    Gc_it_gen * last_gen;   /* Most recent generation, or oldest, or whatever */
    struct _gc_it_hdr * marker;         /* limit of list. Do I need this? */
    struct _gc_it_hdr * finalize;       /* items to be finalized on death */
    /* struct _gc_it_hdr * items;          all items not in queue or finalized */
    struct _gc_it_hdr * queue;          /* list of grey items, to mark */
    struct _gc_it_hdr * new_list;       /* list of items created before the end of the scan */
} Gc_it_pool_data;

#endif /* PARROT_GC_IT */

/* Tracked resource pool */
typedef struct Small_Object_Pool {
    Small_Object_Arena *last_Arena;
    /* Size in bytes of an individual pool item. This size may include
     * a GC-system specific GC header.
     * See the macros below.
     */
    size_t object_size;
    size_t objects_per_alloc;
    size_t total_objects;
    size_t num_free_objects;    /* number of resources in the free pool */
    int skip;
    size_t replenish_level;
    void *free_list;
    /* adds a free object to the pool's free list  */
    add_free_object_fn_type     add_free_object;
    get_free_object_fn_type     get_free_object;
    alloc_objects_fn_type       alloc_objects;
    alloc_objects_fn_type       more_objects;
    dod_object_fn_type          dod_object;
    /* gets and removes a free object from the pool's free list */
    /* allocates more objects */
    struct Memory_Pool *mem_pool;
    size_t start_arena_memory;
    size_t end_arena_memory;
    const char *name;
#if PARROT_GC_GMS
    struct _gc_gms_hdr marker;          /* limit of list */
    struct _gc_gms_hdr *black;          /* alive */
    struct _gc_gms_hdr *black_fin;      /* alive, needs destruction */
    struct _gc_gms_hdr *gray;           /* to be scanned */
    struct _gc_gms_hdr *white;          /* unprocessed */
    struct _gc_gms_hdr *white_fin;      /* unprocessed, needs destruction */

    struct _gc_gms_gen *first_gen;      /* linked list of generations */
    struct _gc_gms_gen *last_gen;
#endif
#if PARROT_GC_IT
    struct _gc_it_pool_data gc_it_pool_data; /* Data for use by the IT GC */
#endif
} Small_Object_Pool;

/*
 * macros used in arena scan code to convert from object pointers
 * to arena pointers ...
 */


#if PARROT_GC_GMS
#  define GC_HEADER_SIZE (sizeof (Gc_gms_hdr))
#  define PObj_to_ARENA(o) PObj_to_GMSH(o)
#  define ARENA_to_PObj(p) GMSH_to_PObj((Gc_gms_hdr*)(p))
#else
#  define GC_HEADER_SIZE 0
#  define PObj_to_ARENA(o) (o)
#  define ARENA_to_PObj(p) (p)
#endif


/* HEADERIZER BEGIN: src/gc/smallobject.c */
/* Don't modify between HEADERIZER BEGIN / HEADERIZER END.  Your changes will be lost. */

PARROT_WARN_UNUSED_RESULT
INTVAL contained_in_pool(
    ARGIN(const Small_Object_Pool *pool),
    ARGIN(const void *ptr))
        __attribute__nonnull__(1)
        __attribute__nonnull__(2);

void gc_pmc_ext_pool_init(ARGMOD(Small_Object_Pool *pool))
        __attribute__nonnull__(1)
        FUNC_MODIFIES(*pool);

PARROT_MALLOC
PARROT_CANNOT_RETURN_NULL
Small_Object_Pool * new_small_object_pool(
    size_t object_size,
    size_t objects_per_alloc);

void Parrot_add_to_free_list(PARROT_INTERP,
    ARGMOD(Small_Object_Pool *pool),
    ARGMOD(Small_Object_Arena *arena))
        __attribute__nonnull__(1)
        __attribute__nonnull__(2)
        __attribute__nonnull__(3)
        FUNC_MODIFIES(*pool)
        FUNC_MODIFIES(*arena);

void Parrot_append_arena_in_pool(PARROT_INTERP,
    ARGMOD(Small_Object_Pool *pool),
    ARGMOD(Small_Object_Arena *new_arena),
    size_t size)
        __attribute__nonnull__(1)
        __attribute__nonnull__(2)
        __attribute__nonnull__(3)
        FUNC_MODIFIES(*pool)
        FUNC_MODIFIES(*new_arena);

void Parrot_gc_ms_init(PARROT_INTERP)
        __attribute__nonnull__(1);

int Parrot_is_const_pmc(PARROT_INTERP, ARGIN(const PMC *pmc))
        __attribute__nonnull__(1)
        __attribute__nonnull__(2);

void Parrot_small_object_pool_merge(PARROT_INTERP,
    ARGMOD(Small_Object_Pool *dest),
    ARGMOD(Small_Object_Pool *source))
        __attribute__nonnull__(1)
        __attribute__nonnull__(2)
        __attribute__nonnull__(3)
        FUNC_MODIFIES(*dest)
        FUNC_MODIFIES(*source);

/* Don't modify between HEADERIZER BEGIN / HEADERIZER END.  Your changes will be lost. */
/* HEADERIZER END: src/gc/smallobject.c */

#endif /* PARROT_SMALLOBJECT_H_GUARD */

/*
 * Local variables:
 *   c-file-style: "parrot"
 * End:
 * vim: expandtab shiftwidth=4:
 */
