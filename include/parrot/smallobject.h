/*
 * $Id$
 * Copyright (C) 2002-2008, The Perl Foundation
 */

#ifndef PARROT_SMALLOBJECT_H_GUARD
#define PARROT_SMALLOBJECT_H_GUARD

#  include "parrot/parrot.h"

#define GC_DEBUG_REPLENISH_LEVEL_FACTOR        0.0
#define GC_DEBUG_UNITS_PER_ALLOC_GROWTH_FACTOR 1
#define REPLENISH_LEVEL_FACTOR                 0.3

/* this factor is totally arbitrary, but gives good timings for stress.pasm */
#define UNITS_PER_ALLOC_GROWTH_FACTOR          1.75

#define POOL_MAX_BYTES                         65536 * 128

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

/* Switches and modes */

#  define GC_IT_DEBUG             0

#  define GC_IT_INCREMENT_MODE    0
#  define GC_IT_BATCH_MODE        1
#  define GC_IT_SERIAL_MODE       1
#  define GC_IT_PARALLEL_MODE     0

#  ifdef GC_IT_PARALLEL_MODE
#    define GC_IT_THREAD_MAX 4
#  endif

/* minimum number of items to scan in a single increment. If we haven't reached
   this minimum, go back and do another increment. */
#  define GC_IT_ITEMS_MARKED_MIN  20

/* behavior macros to determine where the incremental mode breaks. Each
   increment runs from the current state until the next "break", or until
   the end if there are no more breaks. Some states might require several
   increments before completion and moving to the next state. */
#  define GC_IT_BREAK_AFTER_0
#  define GC_IT_BREAK_AFTER_1
#  define GC_IT_BREAK_AFTER_2
#  define GC_IT_BREAK_AFTER_3
#  define GC_IT_BREAK_AFTER_4
#  define GC_IT_BREAK_AFTER_5
#  define GC_IT_BREAK_AFTER_6

/* Macros that are useful outside src/gc/gc_it.c */

#  define GC_IT_MAX_IN_ARENA 65535

/*
 * GC_IT Header, a linked list.
 * Contains a link to the pool/arena (don't know which) that contains this item
 * Contains a link to the next header, to form linked lists.
 * Contains the number of the card and the flag that represents this item in
 * the arena.
 */

typedef struct Gc_it_hdr {
    struct Gc_it_hdr           *next;
    struct {
        unsigned short flag;
        unsigned short agg;
    }                           data;
} Gc_it_hdr;

#  define GC_IT_CARD_WHITE  0x00     /* Item is dead */
#  define GC_IT_CARD_UNUSED 0x01
#  define GC_IT_CARD_BLACK  0x03     /* Item is completely alive */
#  define GC_IT_CARD_FREE   0x02     /* items which are newly created and should
                                       not be scanned until the next mark */

#  define PObj_to_IT_HDR(o) (((Gc_it_hdr*)(o))-1)
#  define IT_HDR_to_PObj(p) ((PObj*)(((Gc_it_hdr*)(p))+1))
#  define cPObj_to_IT_HDR(o) (((const Gc_it_hdr*)(o))-1)

/*
 * GC States
 * Determines which phase of the run is currently being performed.
 */

typedef enum Gc_it_state {
    GC_IT_READY = 0,
    GC_IT_START_MARK,    /* starting a mark, initialize everything */
    GC_IT_MARK_ROOTS,    /* finding root objects */
    GC_IT_RESUME_MARK,   /* iterating over queue items, tree-at-a-time */
    GC_IT_END_MARK,      /* The mark is over, do cleanup, if any */
    GC_IT_SWEEP_PMCS,    /* sweep pmc pools */
    GC_IT_SWEEP_BUFFERS, /* sweep through all buffers, after everything else */
    GC_IT_FINAL_CLEANUP  /* do any necessary cleanup after the GC run is over */
} Gc_it_state;

/* A private datastructure for the GC. All the global data that we need to
   operate will be stored here. */

typedef struct Gc_it_data {
    UINTVAL item_count;       /* number of items scanned in current run */
    UINTVAL total_count;      /* number of items scanned since beginning of mark phase */
    UINTVAL num_generations;  /* number of generations */
    Gc_it_state state;        /* status of the current run */
    UINTVAL num_threads;      /* number of currently active threads */
    Gc_it_hdr *root_queue;    /* queue for temporary storage of root items */
    Gc_it_hdr *queue;         /* list of grey items, to mark */
    Gc_it_hdr *marked;        /* List of items already marked this phase. */
} Gc_it_data;

#endif /* PARROT_GC_IT */

/* Tracked resource pool */
typedef struct Small_Object_Pool {
    Small_Object_Arena *last_Arena;
    void               *free_list;
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
} Small_Object_Pool;

/*
 * macros used in arena-traversal code to move between GC headers and their
 * associated objects, and from one adjacent object to the next in an arena
 */


#if PARROT_GC_GMS
#  define GC_HEADER_SIZE (sizeof (Gc_gms_hdr))
#  define PObj_to_ARENA(o) PObj_to_GMSH(o)
#  define ARENA_to_PObj(p) GMSH_to_PObj((Gc_gms_hdr*)(p))
#elif PARROT_GC_IT
#  define GC_HEADER_SIZE (sizeof (Gc_it_hdr))
#  define PObj_to_ARENA(o) PObj_to_IT_HDR(o)
#  define ARENA_to_PObj(p) IT_HDR_to_PObj(p)
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
