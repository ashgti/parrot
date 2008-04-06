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
#define Parrot_block_DOD(interp) \
        { \
            (interp)->arena_base->DOD_block_level++; \
            Parrot_shared_DOD_block(interp); \
        }

#define Parrot_unblock_DOD(interp) \
        if ((interp)->arena_base->DOD_block_level) { \
            (interp)->arena_base->DOD_block_level--; \
            Parrot_shared_DOD_unblock(interp); \
        }

/* Macros for recursively blocking and unblocking GC */
#define Parrot_block_GC(interp) \
        (interp)->arena_base->GC_block_level++

#define Parrot_unblock_GC(interp) \
        if ((interp)->arena_base->GC_block_level) \
            (interp)->arena_base->GC_block_level--

/* Macros for testing if the DOD and GC are blocked */
#define Parrot_is_blocked_DOD(interp) \
        ((interp)->arena_base->DOD_block_level)

#define Parrot_is_blocked_GC(interp) \
        ((interp)->arena_base->GC_block_level)

#define DOD_trace_stack_FLAG    (UINTVAL)(1 << 0)   /* trace system areads and stack */
#define DOD_trace_normal        (UINTVAL)(1 << 0)   /* the same */
#define DOD_lazy_FLAG           (UINTVAL)(1 << 1)   /* timely destruction run */
#define DOD_finish_FLAG         (UINTVAL)(1 << 2)   /* on Parrot exit: mark (almost) all PMCs dead and */
                                                    /* garbage collect. */
#define DOD_no_trace_volatile_roots (UINTVAL)(1 << 3)
            /* trace all but volatile root set, i.e. registers */

/* HEADERIZER BEGIN: src/gc/dod.c */

PARROT_API
void pobject_lives(PARROT_INTERP, ARGMOD(PObj *obj))
        __attribute__nonnull__(1)
        __attribute__nonnull__(2)
        FUNC_MODIFIES(*obj);

void clear_cow(PARROT_INTERP, ARGMOD(Small_Object_Pool *pool), int cleanup)
        __attribute__nonnull__(1)
        __attribute__nonnull__(2)
        FUNC_MODIFIES(*pool);

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

void Parrot_dod_ms_run(PARROT_INTERP, int flags)
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

void trace_mem_block(PARROT_INTERP, size_t lo_var_ptr, size_t hi_var_ptr)
        __attribute__nonnull__(1);

void used_cow(PARROT_INTERP, ARGMOD(Small_Object_Pool *pool), int cleanup)
        __attribute__nonnull__(1)
        __attribute__nonnull__(2)
        FUNC_MODIFIES(*pool);

/* HEADERIZER END: src/gc/dod.c */


/* HEADERIZER BEGIN: src/cpu_dep.c */

void trace_system_areas(PARROT_INTERP)
        __attribute__nonnull__(1);

/* HEADERIZER END: src/cpu_dep.c */


#if ! DISABLE_GC_DEBUG
/* Set when walking the system stack */
extern int CONSERVATIVE_POINTER_CHASING;
#endif


/* GC subsystem init functions */
/* HEADERIZER BEGIN: src/gc/gc_gms.c */

PARROT_API
void Parrot_gc_gms_init(PARROT_INTERP)
        __attribute__nonnull__(1);

PARROT_API
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

/* HEADERIZER END: src/gc/gc_gms.c */

/* HEADERIZER BEGIN: src/gc/gc_ims.c */

void Parrot_dod_ims_wb(PARROT_INTERP, ARGMOD(PMC *agg), ARGMOD(PMC *_new))
        __attribute__nonnull__(1)
        __attribute__nonnull__(2)
        __attribute__nonnull__(3)
        FUNC_MODIFIES(*agg)
        FUNC_MODIFIES(*_new);

void Parrot_gc_ims_init(PARROT_INTERP)
        __attribute__nonnull__(1);

/* HEADERIZER END: src/gc/gc_ims.c */

/*
 * write barrier
 */
#if PARROT_GC_IMS
#  define DOD_WRITE_BARRIER(interp, agg, old, _new) \
    do { \
        if (!PMC_IS_NULL(_new)   && \
                PObj_live_TEST(agg) && \
                (PObj_get_FLAGS(agg) & PObj_custom_GC_FLAG) && \
                !PObj_live_TEST(_new)) { \
            Parrot_dod_ims_wb(interp, agg, _new); \
        } \
    } while (0)

#  define DOD_WRITE_BARRIER_KEY(interp, agg, old, old_key, _new, new_key) \
          DOD_WRITE_BARRIER(interp, agg, old, _new)
#endif

#if PARROT_GC_MS
#  define DOD_WRITE_BARRIER(interp, agg, old, _new)
#  define DOD_WRITE_BARRIER_KEY(interp, agg, old, old_key, _new, new_key)
#endif

#if PARROT_GC_GMS
#  define DOD_WRITE_BARRIER(interp, agg, old, _new) do { \
    UINTVAL gen_agg, gen_new; \
    if (!(_new) || PMC_IS_NULL(_new)) \
        break; \
    gen_agg = PObj_to_GMSH(agg)->gen->gen_no; \
    gen_new = PObj_to_GMSH(_new)->gen->gen_no; \
    if (gen_agg < gen_new) \
        parrot_gc_gms_wb(interp, agg, old, _new); \
} while (0)

#  define DOD_WRITE_BARRIER_KEY(interp, agg, old, old_key, _new, new_key) do { \
    UINTVAL gen_agg, gen_new, gen_key; \
    if (!(_new) || PMC_IS_NULL(_new)) \
        break; \
    gen_agg = PObj_to_GMSH(agg)->gen->gen_no; \
    gen_new = PObj_to_GMSH(_new)->gen->gen_no; \
    gen_key = PObj_to_GMSH(new_key)->gen->gen_no; \
    if (gen_agg < gen_new || gen_agg < gen_key) \
        parrot_gc_gms_wb_key(interp, agg, old, old_key, _new, new_key); \
} while (0)

#endif

#endif /* PARROT_DOD_H_GUARD */

/*
 * Local variables:
 *   c-file-style: "parrot"
 * End:
 * vim: expandtab shiftwidth=4:
 */
