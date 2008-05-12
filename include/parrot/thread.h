/* thread.h
 *  Copyright (C) 2001-2007, The Perl Foundation.
 *  SVN Info
 *     $Id$
 *  Overview:
 *     This is the api header for the thread primitives
 *  Data Structure and Algorithms:
 *  History:
 *  Notes:
 *  References:
 */

#ifndef PARROT_THREAD_H_GUARD
#define PARROT_THREAD_H_GUARD

#  include "parrot/parrot.h"

#  define PARROT_HAS_THREADS 1
#  include "parrot/atomic.h"

#ifndef PARROT_SYNC_PRIMITIVES_DEFINED

#  undef  PARROT_HAS_THREADS
#  define PARROT_HAS_THREADS 0

#  define LOCK(m)
#  define UNLOCK(m)
#  define COND_WAIT(c, m)
#  define COND_TIMED_WAIT(c, m, t)
#  define COND_SIGNAL(c)
#  define COND_BROADCAST(c)

#  define MUTEX_INIT(m)
#  define MUTEX_DESTROY(m)

#  define COND_INIT(c)
#  define COND_DESTROY(c)

#  define THREAD_CREATE_DETACHED(t, func, arg)
#  define THREAD_CREATE_JOINABLE(t, func, arg)

#  define JOIN(t, ret)
#  define DETACH(t)

#  define CLEANUP_PUSH(f, a)
#  define CLEANUP_POP(a)

#  define Parrot_mutex int
#  define Parrot_cond int
#  define Parrot_thread int

typedef void (*Cleanup_Handler)(void *);

#  ifndef _STRUCT_TIMESPEC
#    define _STRUCT_TIMESPEC
struct timespec {
    time_t tv_sec;
    long tv_nsec;
};
#  endif /* _STRUCT_TIMESPEC */

#endif /* PARROT_SYNC_PRIMITIVES_DEFINED */

#ifndef YIELD
#  define YIELD
#endif /* YIELD */

typedef enum {
    THREAD_STATE_JOINABLE,              /* default */
    THREAD_STATE_DETACHED = 0x01,       /* i.e. non-joinable */
    THREAD_STATE_JOINED   = 0x02,       /* JOIN was issued */
    THREAD_STATE_FINISHED = 0x04,        /* the thread function has ended */
    THREAD_STATE_NOT_STARTED = 0x08,     /* the thread wasn't started */
    THREAD_STATE_SUSPENDED_GC = 0x10,     /* suspended for GC on request */
    THREAD_STATE_GC_WAKEUP = 0x20,      /* the thread is waiting on its condition
                                           variable, and will do a GC run if
                                           it is woken up and marked as suspended
                                           for GC */
    THREAD_STATE_SUSPEND_GC_REQUESTED = 0x40 /* the thread's event queue
                                                 contains a suspend-for-GC event */
} thread_state_enum;


/*
 * per interpreter thread data structure
 */
typedef struct _Thread_data {
    Parrot_thread       thread;         /* pthread_t or such */
    INTVAL              state;
    int                 wants_shared_gc; /* therad is trying to
                                            do a shared GC run */
    UINTVAL             tid;            /* 0.. n-1 idx in interp array */

    Parrot_Interp       joiner;         /* thread that is trying to join this */

    /* for wr access to interpreter e.g. for DOD/GC
     * if only used for DOD/GC the lock could be in the arena
     * instead here, or in the interpreter, with negative size impact
     * for the non-threaded case
     */
    Parrot_mutex interp_lock;

    /* for waking up the interpreter from various sorts
     * of sleeping
     */
    Parrot_cond  interp_cond;

    /* STM transaction log */
    struct STM_tx_log   *stm_log;

    /* COW'd constant tables */
    Hash             *const_tables;
} Thread_data;

#  define LOCK_INTERPRETER(interp) \
    if ((interp)->thread_data) \
        LOCK((interp)->thread_data->interp_lock)
#  define UNLOCK_INTERPRETER(interp) \
    if ((interp)->thread_data) \
        UNLOCK((interp)->thread_data->interp_lock)

#  define INTERPRETER_LOCK_INIT(interp) \
        do { \
            MUTEX_INIT((interp)->thread_data->interp_lock); \
            COND_INIT((interp)->thread_data->interp_cond); \
        } while (0)
#  define INTERPRETER_LOCK_DESTROY(interp) \
        do { \
            MUTEX_DESTROY((interp)->thread_data->interp_lock); \
            COND_DESTROY((interp)->thread_data->interp_cond); \
        } while (0)


/*
 * this global mutex protects the list of interpreters
 */
VAR_SCOPE Parrot_mutex                  interpreter_array_mutex;
VAR_SCOPE Interp          ** interpreter_array;
VAR_SCOPE size_t                        n_interpreters;

typedef enum {
    THREAD_GC_STAGE_NONE,
    THREAD_GC_STAGE_MARK,
    THREAD_GC_STAGE_SWEEP = THREAD_GC_STAGE_NONE
} thread_gc_stage_enum;

typedef struct _Shared_gc_info {
    thread_gc_stage_enum  gc_stage;
    Parrot_cond           gc_cond;
    int                   num_reached;

    Parrot_atomic_integer gc_block_level;
} Shared_gc_info;

/* TODO use thread pools instead */
VAR_SCOPE Shared_gc_info *shared_gc_info;

typedef struct _Sync {
    Parrot_Interp owner;                /* that interpreter, that owns
                                           the arena, where the PMC is in */
    Parrot_mutex pmc_lock;              /* for wr access to PMCs content */
} Sync;

/* HEADERIZER BEGIN: src/thread.c */
/* Don't modify between HEADERIZER BEGIN / HEADERIZER END.  Your changes will be lost. */

PARROT_API
void Parrot_shared_DOD_block(PARROT_INTERP)
        __attribute__nonnull__(1);

PARROT_API
void Parrot_shared_DOD_unblock(PARROT_INTERP)
        __attribute__nonnull__(1);

void pt_add_to_interpreters(PARROT_INTERP,
    ARGIN_NULLOK(Parrot_Interp new_interp))
        __attribute__nonnull__(1);

void pt_clone_code(Parrot_Interp d, Parrot_Interp s);
void pt_clone_globals(Parrot_Interp d, Parrot_Interp s);
void pt_DOD_mark_root_finished(PARROT_INTERP)
        __attribute__nonnull__(1);

void pt_DOD_start_mark(PARROT_INTERP)
        __attribute__nonnull__(1);

void pt_DOD_stop_mark(PARROT_INTERP)
        __attribute__nonnull__(1);

void pt_free_pool(PARROT_INTERP)
        __attribute__nonnull__(1);

void pt_join_threads(PARROT_INTERP)
        __attribute__nonnull__(1);

PARROT_CAN_RETURN_NULL
PMC * pt_shared_fixup(PARROT_INTERP, ARGMOD(PMC *pmc))
        __attribute__nonnull__(1)
        __attribute__nonnull__(2)
        FUNC_MODIFIES(*pmc);

void pt_suspend_self_for_gc(PARROT_INTERP)
        __attribute__nonnull__(1);

void pt_thread_detach(UINTVAL tid);
PARROT_CAN_RETURN_NULL
PMC* pt_thread_join(NOTNULL(Parrot_Interp parent), UINTVAL tid)
        __attribute__nonnull__(1);

void pt_thread_kill(UINTVAL tid);
void pt_thread_prepare_for_run(Parrot_Interp d, Parrot_Interp s);
int pt_thread_run(PARROT_INTERP,
    ARGOUT(PMC *dest_interp),
    ARGIN(PMC *sub),
    ARGIN_NULLOK(PMC *arg))
        __attribute__nonnull__(1)
        __attribute__nonnull__(2)
        __attribute__nonnull__(3)
        FUNC_MODIFIES(*dest_interp);

int pt_thread_run_1(PARROT_INTERP,
    ARGOUT(PMC* dest_interp),
    ARGIN(PMC* sub),
    ARGIN(PMC *arg))
        __attribute__nonnull__(1)
        __attribute__nonnull__(2)
        __attribute__nonnull__(3)
        __attribute__nonnull__(4)
        FUNC_MODIFIES(* dest_interp);

int pt_thread_run_2(PARROT_INTERP,
    ARGOUT(PMC* dest_interp),
    ARGIN(PMC* sub),
    ARGIN(PMC *arg))
        __attribute__nonnull__(1)
        __attribute__nonnull__(2)
        __attribute__nonnull__(3)
        __attribute__nonnull__(4)
        FUNC_MODIFIES(* dest_interp);

int pt_thread_run_3(PARROT_INTERP,
    ARGOUT(PMC* dest_interp),
    ARGIN(PMC* sub),
    ARGIN(PMC *arg))
        __attribute__nonnull__(1)
        __attribute__nonnull__(2)
        __attribute__nonnull__(3)
        __attribute__nonnull__(4)
        FUNC_MODIFIES(* dest_interp);

void pt_thread_wait_with(PARROT_INTERP, ARGMOD(Parrot_mutex *mutex))
        __attribute__nonnull__(1)
        __attribute__nonnull__(2)
        FUNC_MODIFIES(*mutex);

void pt_thread_yield(void);
PARROT_CAN_RETURN_NULL
PMC * pt_transfer_sub(
    ARGOUT(Parrot_Interp d),
    ARGIN(Parrot_Interp s),
    ARGIN(PMC *sub))
        __attribute__nonnull__(1)
        __attribute__nonnull__(2)
        __attribute__nonnull__(3)
        FUNC_MODIFIES(d);

/* Don't modify between HEADERIZER BEGIN / HEADERIZER END.  Your changes will be lost. */
/* HEADERIZER END: src/thread.c */

#endif /* PARROT_THREAD_H_GUARD */

/*
 * Local variables:
 *   c-file-style: "parrot"
 * End:
 * vim: expandtab shiftwidth=4:
 */
