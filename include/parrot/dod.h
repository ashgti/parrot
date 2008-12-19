/* dod.h
 *  Copyright (C) 2001-2007, The Perl Foundation.
 *  SVN Info
 *     $Id$
 *  Overview:
 *     Handles dead object destruction of the various headers
 *  History:
 *     Initial version by Mike Lambert on 2002.05.27
 */

#ifndef PARROT_DOD_H_GUARD
#define PARROT_DOD_H_GUARD

#include "parrot/parrot.h"

/* Macros for recursively blocking and unblocking DOD */
#define Parrot_block_GC_mark(interp) \
        { \
            (interp)->arena_base->DOD_block_level++; \
            Parrot_shared_DOD_block(interp); \
        }

#define Parrot_unblock_GC_mark(interp) \
        if ((interp)->arena_base->DOD_block_level) { \
            (interp)->arena_base->DOD_block_level--; \
            Parrot_shared_DOD_unblock(interp); \
        }

/* Macros for recursively blocking and unblocking GC */
#define Parrot_block_GC_sweep(interp) \
        (interp)->arena_base->GC_block_level++

#define Parrot_unblock_GC_sweep(interp) \
        if ((interp)->arena_base->GC_block_level) \
            (interp)->arena_base->GC_block_level--

/* Macros for testing if the DOD and GC are blocked */
#define Parrot_is_blocked_GC_mark(interp) \
        ((interp)->arena_base->DOD_block_level)

#define Parrot_is_blocked_GC_sweep(interp) \
        ((interp)->arena_base->GC_block_level)

#define GC_trace_stack_FLAG    (UINTVAL)(1 << 0)   /* trace system areas and stack */
#define GC_trace_normal        (UINTVAL)(1 << 0)   /* the same */
#define GC_lazy_FLAG           (UINTVAL)(1 << 1)   /* timely destruction run */
#define GC_finish_FLAG         (UINTVAL)(1 << 2)   /* on Parrot exit: mark (almost) all PMCs dead and */
                                                   /* garbage collect. */

/* HEADERIZER BEGIN: src/gc/dod.c */
/* Don't modify between HEADERIZER BEGIN / HEADERIZER END.  Your changes will be lost. */

PARROT_EXPORT
void object_lives(SHIM_INTERP, ARGMOD(PObj *obj))
        __attribute__nonnull__(2)
        FUNC_MODIFIES(*obj);

PARROT_EXPORT
void pobject_lives(PARROT_INTERP, ARGMOD(PObj *obj))
        __attribute__nonnull__(1)
        __attribute__nonnull__(2)
        FUNC_MODIFIES(*obj);

void clear_cow(PARROT_INTERP, ARGMOD(Small_Object_Pool *pool), int cleanup)
        __attribute__nonnull__(1)
        __attribute__nonnull__(2)
        FUNC_MODIFIES(*pool);

UINTVAL is_PObj(ARGIN(void * ptr))
        __attribute__nonnull__(1);

void Parrot_do_dod_run(PARROT_INTERP, UINTVAL flags)
        __attribute__nonnull__(1);

void Parrot_dod_clear_live_bits(PARROT_INTERP)
        __attribute__nonnull__(1);

void Parrot_dod_free_buffer(SHIM_INTERP,
    ARGMOD(Small_Object_Pool *pool),
    ARGMOD(PObj *b))
        __attribute__nonnull__(2)
        __attribute__nonnull__(3)
        FUNC_MODIFIES(*pool)
        FUNC_MODIFIES(*b);

void Parrot_dod_free_buffer_malloc(SHIM_INTERP,
    SHIM(Small_Object_Pool *pool),
    ARGMOD(PObj *b))
        __attribute__nonnull__(3)
        FUNC_MODIFIES(*b);

void Parrot_dod_free_pmc(PARROT_INTERP,
    SHIM(Small_Object_Pool *pool),
    ARGMOD(PObj *p))
        __attribute__nonnull__(1)
        __attribute__nonnull__(3)
        FUNC_MODIFIES(*p);

void Parrot_dod_free_sysmem(SHIM_INTERP,
    SHIM(Small_Object_Pool *pool),
    ARGMOD(PObj *b))
        __attribute__nonnull__(3)
        FUNC_MODIFIES(*b);

void Parrot_dod_ms_run(PARROT_INTERP, UINTVAL flags)
        __attribute__nonnull__(1);

void Parrot_dod_ms_run_init(PARROT_INTERP)
        __attribute__nonnull__(1);

void Parrot_dod_profile_end(PARROT_INTERP, int what)
        __attribute__nonnull__(1);

void Parrot_dod_profile_start(PARROT_INTERP)
        __attribute__nonnull__(1);

void Parrot_dod_sweep(PARROT_INTERP, ARGMOD(Small_Object_Pool *pool))
        __attribute__nonnull__(1)
        __attribute__nonnull__(2)
        FUNC_MODIFIES(*pool);

int Parrot_dod_trace_children(PARROT_INTERP, size_t how_many)
        __attribute__nonnull__(1);

void Parrot_dod_trace_pmc_data(PARROT_INTERP, ARGIN(PMC *p))
        __attribute__nonnull__(1)
        __attribute__nonnull__(2);

int Parrot_dod_trace_root(PARROT_INTERP, int trace_stack)
        __attribute__nonnull__(1);

void Parrot_free_pmc_ext(PARROT_INTERP, ARGMOD(PMC *p))
        __attribute__nonnull__(1)
        __attribute__nonnull__(2)
        FUNC_MODIFIES(*p);

void set_PObj(ARGMOD(void * ptr))
        __attribute__nonnull__(1)
        FUNC_MODIFIES(* ptr);

void trace_mem_block(PARROT_INTERP, size_t lo_var_ptr, size_t hi_var_ptr)
        __attribute__nonnull__(1);

void used_cow(PARROT_INTERP, ARGMOD(Small_Object_Pool *pool), int cleanup)
        __attribute__nonnull__(1)
        __attribute__nonnull__(2)
        FUNC_MODIFIES(*pool);

/* Don't modify between HEADERIZER BEGIN / HEADERIZER END.  Your changes will be lost. */
/* HEADERIZER END: src/gc/dod.c */


/* HEADERIZER BEGIN: src/cpu_dep.c */
/* Don't modify between HEADERIZER BEGIN / HEADERIZER END.  Your changes will be lost. */

void trace_system_areas(PARROT_INTERP)
        __attribute__nonnull__(1);

/* Don't modify between HEADERIZER BEGIN / HEADERIZER END.  Your changes will be lost. */
/* HEADERIZER END: src/cpu_dep.c */


#if ! DISABLE_GC_DEBUG
/* Set when walking the system stack */
extern int CONSERVATIVE_POINTER_CHASING;
#endif


/* GC subsystem init functions */
/* HEADERIZER BEGIN: src/gc/gc_gms.c */
/* Don't modify between HEADERIZER BEGIN / HEADERIZER END.  Your changes will be lost. */

PARROT_EXPORT
void Parrot_gc_gms_init(PARROT_INTERP)
        __attribute__nonnull__(1);

PARROT_EXPORT
void parrot_gc_gms_pobject_lives(PARROT_INTERP, ARGMOD(PObj *obj))
        __attribute__nonnull__(1)
        __attribute__nonnull__(2)
        FUNC_MODIFIES(*obj);

void parrot_gc_gms_wb(PARROT_INTERP,
    ARGIN(PMC *agg),
    ARGIN(void *old),
    ARGIN(void *_new))
        __attribute__nonnull__(1)
        __attribute__nonnull__(2)
        __attribute__nonnull__(3)
        __attribute__nonnull__(4);

void parrot_gc_gms_wb_key(PARROT_INTERP,
    ARGIN(PMC *agg),
    ARGIN(void *old),
    ARGIN(void *old_key),
    ARGIN(void *_new),
    ARGIN(void *new_key))
        __attribute__nonnull__(1)
        __attribute__nonnull__(2)
        __attribute__nonnull__(3)
        __attribute__nonnull__(4)
        __attribute__nonnull__(5)
        __attribute__nonnull__(6);

/* Don't modify between HEADERIZER BEGIN / HEADERIZER END.  Your changes will be lost. */
/* HEADERIZER END: src/gc/gc_gms.c */

/* HEADERIZER BEGIN: src/gc/gc_ims.c */
/* Don't modify between HEADERIZER BEGIN / HEADERIZER END.  Your changes will be lost. */

void Parrot_dod_ims_wb(PARROT_INTERP, ARGMOD(PMC *agg), ARGMOD(PMC *_new))
        __attribute__nonnull__(1)
        __attribute__nonnull__(2)
        __attribute__nonnull__(3)
        FUNC_MODIFIES(*agg)
        FUNC_MODIFIES(*_new);

void Parrot_gc_ims_init(PARROT_INTERP)
        __attribute__nonnull__(1);

/* Don't modify between HEADERIZER BEGIN / HEADERIZER END.  Your changes will be lost. */
/* HEADERIZER END: src/gc/gc_ims.c */

#if PARROT_GC_IT
/* HEADERIZER BEGIN: src/gc/gc_it.c */
/* Don't modify between HEADERIZER BEGIN / HEADERIZER END.  Your changes will be lost. */

PARROT_EXPORT
void gc_it_add_free_object(PARROT_INTERP,
    ARGMOD(struct Small_Object_Pool *pool),
    ARGMOD(void *to_add))
        __attribute__nonnull__(1)
        __attribute__nonnull__(2)
        __attribute__nonnull__(3)
        FUNC_MODIFIES(*pool)
        FUNC_MODIFIES(*to_add);

PARROT_EXPORT
void gc_it_alloc_objects(PARROT_INTERP,
    ARGMOD(struct Small_Object_Pool *pool))
        __attribute__nonnull__(1)
        __attribute__nonnull__(2)
        FUNC_MODIFIES(*pool);

PARROT_WARN_UNUSED_RESULT
PARROT_CANNOT_RETURN_NULL
PARROT_EXPORT
void * gc_it_get_free_object(PARROT_INTERP,
    ARGMOD(struct Small_Object_Pool *pool))
        __attribute__nonnull__(1)
        __attribute__nonnull__(2)
        FUNC_MODIFIES(*pool);

PARROT_EXPORT
void gc_it_more_objects(PARROT_INTERP, ARGMOD(Small_Object_Pool *pool))
        __attribute__nonnull__(1)
        __attribute__nonnull__(2)
        FUNC_MODIFIES(*pool);

PARROT_EXPORT
void Parrot_gc_it_deinit(PARROT_INTERP)
        __attribute__nonnull__(1);

PARROT_EXPORT
void Parrot_gc_it_init(PARROT_INTERP)
        __attribute__nonnull__(1);

PARROT_EXPORT
void Parrot_gc_it_pool_init(PARROT_INTERP, ARGMOD(Small_Object_Pool *pool))
        __attribute__nonnull__(1)
        __attribute__nonnull__(2)
        FUNC_MODIFIES(*pool);

PARROT_EXPORT
void Parrot_gc_it_run(PARROT_INTERP, int flags)
        __attribute__nonnull__(1);

PARROT_INLINE
void gc_it_add_free_header(SHIM_INTERP,
    ARGMOD(struct Small_Object_Pool * pool),
    ARGMOD(struct Gc_it_hdr * hdr))
        __attribute__nonnull__(2)
        __attribute__nonnull__(3)
        FUNC_MODIFIES(* pool)
        FUNC_MODIFIES(* hdr);

PARROT_WARN_UNUSED_RESULT
PARROT_INLINE
UINTVAL gc_it_get_card_mark(ARGMOD(Gc_it_hdr *hdr))
        __attribute__nonnull__(1)
        FUNC_MODIFIES(*hdr);

void gc_it_mark_threaded(SHIM_INTERP);
UINTVAL gc_it_ptr_get_aggregate(ARGIN(void * const ptr))
        __attribute__nonnull__(1);

void gc_it_ptr_set_aggregate(ARGMOD(void *ptr), unsigned char flag)
        __attribute__nonnull__(1)
        FUNC_MODIFIES(*ptr);

PARROT_INLINE
void gc_it_set_card_mark(ARGMOD(Gc_it_hdr *hdr), UINTVAL flag)
        __attribute__nonnull__(1)
        FUNC_MODIFIES(*hdr);

void gc_it_trace_normal(PARROT_INTERP)
        __attribute__nonnull__(1);

void gc_it_trace_threaded(SHIM_INTERP);

UINTVAL gc_it_pmc_dead(ARGIN(PMC *p))
        __attribute__nonnull__(1);
/* Don't modify between HEADERIZER BEGIN / HEADERIZER END.  Your changes will be lost. */
/* HEADERIZER END: src/gc/gc_it.c */
#endif

/*
 * write barrier
 */

#if PARROT_GC_IMS
#  define GC_WRITE_BARRIER(interp, agg, old, _new) \
    do { \
        if (!PMC_IS_NULL(_new)   && \
                PObj_live_TEST(agg) && \
                (PObj_get_FLAGS(agg) & PObj_custom_GC_FLAG) && \
                !PObj_live_TEST(_new)) { \
            Parrot_dod_ims_wb((interp), (agg), (_new)); \
        } \
    } while (0)

#  define GC_WRITE_BARRIER_KEY(interp, agg, old, old_key, _new, new_key) \
          GC_WRITE_BARRIER((interp), (agg), (old), (_new))
#endif

#if PARROT_GC_MS
#  define GC_WRITE_BARRIER(interp, agg, old, _new)
#  define GC_WRITE_BARRIER_KEY(interp, agg, old, old_key, _new, new_key)
#endif

#if PARROT_GC_GMS
#  define GC_WRITE_BARRIER(interp, agg, old, _new) do { \
    UINTVAL gen_agg, gen_new; \
    if (!(_new) || PMC_IS_NULL(_new)) \
        break; \
    gen_agg = PObj_to_GMSH(agg)->gen->gen_no; \
    gen_new = PObj_to_GMSH(_new)->gen->gen_no; \
    if (gen_agg < gen_new) \
        parrot_gc_gms_wb((interp), (agg), (old), (_new)); \
} while (0)

#  define GC_WRITE_BARRIER_KEY(interp, agg, old, old_key, _new, new_key) do { \
    UINTVAL gen_agg, gen_new, gen_key; \
    if (!(_new) || PMC_IS_NULL(_new)) \
        break; \
    gen_agg = PObj_to_GMSH(agg)->gen->gen_no; \
    gen_new = PObj_to_GMSH(_new)->gen->gen_no; \
    gen_key = PObj_to_GMSH(new_key)->gen->gen_no; \
    if (gen_agg < gen_new || gen_agg < gen_key) \
        parrot_gc_gms_wb_key((interp), (agg), (old), (old_key), (_new), (new_key)); \
} while (0)

#endif

#if PARROT_GC_IT
    /* The aggregate is obviously being used somewhere, so make sure it's
       marked. pobject_lives will short circuit if it's already been marked.
       mark the new item too. */
#  define GC_WRITE_BARRIER(interp, agg, old, _new) do { \
    if ((agg)) \
        pobject_lives((interp), (PObj*)(agg)); \
    if ((_new)) \
        pobject_lives((interp), (PObj*)(_new)); \
} while (0)
    /* Mark the aggregate, the new object and the new_key, they are all
       apparently being used and I want to make sure they don't get lost */
#  define GC_WRITE_BARRIER_KEY(interp, agg, old, old_key, _new, new_key) do {\
    if ((agg)) \
        pobject_lives((interp), (PObj*)(agg)); \
    if ((_new)) \
        pobject_lives((interp), (PObj*)(_new)); \
    if ((new_key)) \
        pobject_lives((interp), (PObj*)(new_key)); \
} while (0)
#endif

/* Macros for doing common things with the GC_IT */

#define GC_IT_MARK_NODE_GREY(gc_data, hdr) do { \
    (hdr)->next = (gc_data)->queue; \
    (gc_data)->queue = (hdr); \
} while (0)

#define GC_IT_ADD_TO_QUEUE(gc_data, hdr) do {\
    (hdr)->next = (gc_data)->queue; \
    (gc_data)->queue = (hdr); \
} while (0)

#define GC_IT_ADD_TO_ROOT_QUEUE(gc_data, hdr) do {\
    (hdr)->next = (gc_data)->root_queue; \
    (gc_data)->root_queue = (hdr); \
} while (0)

#define GC_IT_ADD_TO_FREE_LIST(pool, hdr) do { \
    (hdr)->next       = (Gc_it_hdr *)((pool)->free_list); \
    (pool)->free_list = (void *)(hdr); \
} while (0)

#define GC_IT_POP_HDR_FROM_LIST(list, hdr, type) do {\
    (hdr)       = (Gc_it_hdr *)(list); \
    (list)      = (type)((hdr)->next); \
    (hdr)->next = NULL; \
} while (0)

#define GC_IT_HDR_FROM_INDEX(p, a, i) \
    (Gc_it_hdr*)(((char*)((a)->start_objects))+((p)->object_size*(i)))

#define GC_IT_HDR_HAS_PARENT_POOL(hdr, pool) \
    ((hdr)->parent_arena->parent_pool == (pool))

#endif /* PARROT_DOD_H_GUARD */

/*
 * Local variables:
 *   c-file-style: "parrot"
 * End:
 * vim: expandtab shiftwidth=4:
 */
