/* interpreter.h
 *  Copyright (C) 2001-2009, Parrot Foundation.
 *  SVN Info
 *     $Id$
 *  Overview:
 *     The interpreter API handles running the operations
 */

#ifndef PARROT_INTERPRETER_H_GUARD
#define PARROT_INTERPRETER_H_GUARD

/* These should be visible to embedders. */

/* General flags */
/* &gen_from_enum(interpflags.pasm) */
typedef enum {
    PARROT_NO_FLAGS         = 0x00,
    PARROT_BOUNDS_FLAG      = 0x04,  /* We're tracking byte code bounds */
    PARROT_PROFILE_FLAG     = 0x08,  /* gathering profile information */
    PARROT_GC_DEBUG_FLAG    = 0x10,  /* debugging memory management */

    PARROT_EXTERN_CODE_FLAG = 0x100, /* reusing another interp's code */
    PARROT_DESTROY_FLAG     = 0x200, /* the last interpreter shall cleanup */

    PARROT_IS_THREAD        = 0x1000, /* interpreter is a thread */
    PARROT_THR_COPY_INTERP  = 0x2000, /* thread start copies interp state */
    PARROT_THR_THREAD_POOL  = 0x4000, /* type3 threads */

    PARROT_THR_TYPE_1       = PARROT_IS_THREAD,
    PARROT_THR_TYPE_2       = PARROT_IS_THREAD | PARROT_THR_COPY_INTERP,
    PARROT_THR_TYPE_3       = PARROT_IS_THREAD | PARROT_THR_COPY_INTERP |
                              PARROT_THR_THREAD_POOL

} Parrot_Interp_flag;
/* &end_gen */

/* &gen_from_enum(interpdebug.pasm) */
typedef enum {
    PARROT_NO_DEBUG                 = 0x00,
    PARROT_MEM_STAT_DEBUG_FLAG      = 0x01,  /* memory usage summary */
    PARROT_BACKTRACE_DEBUG_FLAG     = 0x02,  /* print bt in exception */
    PARROT_JIT_DEBUG_FLAG           = 0x04,  /* create jit stabs file */
    PARROT_START_DEBUG_FLAG         = 0x08,
    PARROT_THREAD_DEBUG_FLAG        = 0x10,
    PARROT_EVAL_DEBUG_FLAG          = 0x20,  /* create EVAL_n file */
    PARROT_REG_DEBUG_FLAG           = 0x40,  /* fill I,N with garbage */
    PARROT_CTX_DESTROY_DEBUG_FLAG   = 0x80,  /* ctx of a sub is gone */
    PARROT_ALL_DEBUG_FLAGS          = 0xffff
} Parrot_debug_flags;
/* &end_gen */

/* &gen_from_enum(interptrace.pasm) */
typedef enum {
    PARROT_NO_TRACE                 = 0x00,
    PARROT_TRACE_OPS_FLAG           = 0x01,  /* op execution trace */
    PARROT_TRACE_FIND_METH_FLAG     = 0x02,  /* find_method */
    PARROT_TRACE_SUB_CALL_FLAG      = 0x04,  /* invoke/retcc */
    PARROT_ALL_TRACE_FLAGS          = 0xffff
} Parrot_trace_flags;
/* &end_gen */

/* &gen_from_enum(interpcores.pasm) */
typedef enum {
    PARROT_SLOW_CORE,                       /* slow bounds/trace/profile core */
    PARROT_FUNCTION_CORE    = PARROT_SLOW_CORE,
    PARROT_FAST_CORE        = 0x01,         /* fast DO_OP core */
    PARROT_SWITCH_CORE      = 0x02,         /*   P   = prederef   */
    PARROT_CGP_CORE         = 0x06,         /*  CP                */
    PARROT_CGOTO_CORE       = 0x04,         /*  C    = cgoto      */
    PARROT_EXEC_CORE        = 0x20,         /* TODO Parrot_exec_run variants */
    PARROT_GC_DEBUG_CORE    = 0x40,         /* run GC before each op */
    PARROT_DEBUGGER_CORE    = 0x80,         /* used by parrot debugger */
    PARROT_PROFILING_CORE   = 0x160         /* used by parrot debugger */
} Parrot_Run_core_t;
/* &end_gen */

/* &gen_from_enum(cloneflags.pasm) */
typedef enum {
    PARROT_CLONE_CODE = 0x1,        /* active code segments
                                     * XXX interaction with lexicals
                                     */
    PARROT_CLONE_GLOBALS = 0x2,     /* global stash */
    PARROT_CLONE_RUNOPS = 0x4,      /* runops choice */
    PARROT_CLONE_INTERP_FLAGS = 0x8, /* bounds checking and
                                     * debugging flags */
    PARROT_CLONE_HLL = 0x10,        /* clone HLL setting */
    PARROT_CLONE_CLASSES = 0x20,    /* clone usermade classes */
    PARROT_CLONE_LIBRARIES = 0x40,  /* clone loaded library set */
    /* flags that won't be initially implemented */
    PARROT_CLONE_CC = 0x80,         /* clone current continuation --
                                     * fork()-like cloning (requires
                                     * cloned code segments); probably
                                     * would only work if runloop_level is 1 */

    /* combinations of flags */
    PARROT_CLONE_DEFAULT = 0x7f /* everything but CC */
} Parrot_clone_flags;
/* &end_gen */

struct parrot_interp_t;

/* One of the most common shim arguments is the interpreter itself, so it
 * gets its own macro. */
#define PARROT_INTERP /*@notnull@*/ /*@in@*/ Parrot_Interp interp
#define NULLOK_INTERP /*@null@*/    /*@in@*/ Parrot_Interp interp
#define SHIM_INTERP   /*@unused@*/ /*@null@*/ Parrot_Interp interp_unused __attribute__unused__


#ifdef PARROT_IN_CORE

#define Parrot_Language Parrot_Int
#define Parrot_Vtable struct _vtable*

typedef Parrot_Interp_flag Interp_flags;
typedef Parrot_Run_core_t Run_Cores;

#define Interp_flags_SET(interp, flag)   ((interp)->flags |= (flag))
#define Interp_flags_CLEAR(interp, flag) ((interp)->flags &= ~(flag))
#define Interp_flags_TEST(interp, flag)  ((interp)->flags & (flag))

#define Interp_debug_SET(interp, flag)   ((interp)->debug_flags |= (flag))
#define Interp_debug_CLEAR(interp, flag) ((interp)->debug_flags &= ~(flag))
#define Interp_debug_TEST(interp, flag)  ((interp)->debug_flags & (flag))

#define Interp_trace_SET(interp, flag)   Parrot_pcc_trace_flags_on(interp, interp->ctx, (flag))
#define Interp_trace_CLEAR(interp, flag) Parrot_pcc_trace_flags_off(interp, interp->ctx, (flag))
#define Interp_trace_TEST(interp, flag)  Parrot_pcc_trace_flags_test(interp, interp->ctx, (flag))

#define Interp_core_SET(interp, core)   ((interp)->run_core = (core))
#define Interp_core_TEST(interp, core)  ((interp)->run_core == (core))

#include "parrot/context.h"
#include "parrot/parrot.h"
#include "parrot/warnings.h"

#include "parrot/op.h"
#include "parrot/oplib.h"

#include "parrot/debugger.h"
#include "parrot/multidispatch.h"
#include "parrot/call.h"

typedef struct warnings_t {
    Warnings_classes classes;
} *Warnings;

/* Forward declaration for imc_info_t -- the actual struct is
 * defined in imcc/imc.h */
struct _imc_info_t;


struct _Thread_data;    /* in thread.h */
struct _Caches;         /* caches .h */

typedef struct _Prederef_branch {       /* item for recording branches */
    size_t offs;                        /* offset in code */
    void  *op;                          /* opcode at that position */
} Prederef_branch;

typedef struct _Prederef {
    void **code;                        /* prederefed code */
    Prederef_branch *branches;          /* list of branches in code */
    size_t n_branches;                  /* entries in that list */
    size_t n_allocated;                 /* allocated size of it */
} Prederef;

/*
 * Get Context from interpeter.
 */
#define CONTEXT(interp)         Parrot_pcc_get_context_struct((interp), (interp)->ctx)

/*
 * Helper macros to fetch fields from context.
 *
 * Not considered as part of public API. Should be replaced with proper accessor
 * functions to manipulate Context.
 */
#define CURRENT_CONTEXT(interp) ((interp)->ctx)


typedef struct _context_mem {
    void **free_list;               /* array of free-lists, per size free slots */
    int n_free_slots;               /* amount of allocated */
} context_mem;

struct _handler_node_t; /* forward def - exit.h */

/* The actual interpreter structure */
struct parrot_interp_t {
    PMC           *ctx;                       /* current Context */

    struct Memory_Pools *mem_pools;                /* Pointer to this interpreter's
                                               * arena */

    struct GC_Subsystem *gc_sys;              /*functions and data specific
                                                  to current GC subsystem*/

    PMC *gc_registry;                         /* root set of registered PMCs */

    PMC     *class_hash;                      /* Hash of classes */
    VTABLE **vtables;                         /* array of vtable ptrs */
    int      n_vtable_max;                    /* highest used type */
    int      n_vtable_alloced;                /* alloced vtable space */

    struct _ParrotIOData   *piodata;          /* interpreter's IO system */

    op_lib_t  *op_lib;                        /* Opcode library */
    size_t     op_count;                      /* The number of ops */
    op_info_t *op_info_table;                 /* Opcode info table
                                               * (name, nargs, arg types) */

    op_func_t *op_func_table;                  /* opcode dispatch table
                                                * (functions, labels, or nothing
                                                * (e.g. switched core), which
                                                * the interpreter is currently
                                                * running */

    op_func_t *evc_func_table;                /* event check opcode dispatch */
    op_func_t *save_func_table;               /* for restoring op_func_table */

    int         n_libs;                       /* count of libs below */
    op_lib_t  **all_op_libs;                  /* all loaded opcode libraries */

    INTVAL flags;                             /* Various interpreter flags that
                                               * signal that runops should do
                                               * something */

    UINTVAL debug_flags;                      /* debug settings */

    struct runcore_t  *run_core;              /* type of core to run the ops */
    struct runcore_t **cores;                 /* array of known runcores */
    UINTVAL            num_cores;             /* number of known runcores */

    INTVAL resume_flag;
    size_t resume_offset;

    PackFile_ByteCode *code;                  /* The code we are executing */
    struct PackFile          *initial_pf;     /* first created PF  */

    struct _imc_info_t *imc_info;             /* imcc data */

    const char *output_file;                  /* where to write output */

    PDB_t  *pdb;                              /* debug /trace system */

    PMC * dynamic_env;                        /* Dynamic environment stack */

    void *lo_var_ptr;                         /* Pointer to memory on runops
                                               * system stack */

    Interp *parent_interpreter;

    /* per interpreter global vars */
    INTVAL world_inited;                      /* world_init_once() is done */

    UINTVAL hash_seed;                        /* STRING hash seed */

    PMC *iglobals;                      /* FixedPMCArray of PMCs, containing: */
    /* 0:   PMC *Parrot_base_classname_hash; hash containing name->base_type */
    /* 1:   PMC *Parrot_compreg_hash;    hash containing assembler/compilers */
    /* 2:   PMC *Argv;                   list of argv */
    /* 3:   PMC *NCI func hash           hash of NCI funcs */
    /* 4:   PMC *ParrotInterpreter       that's me */
    /* 5:   PMC *Dyn_libs                dynamically loaded ParrotLibrary */
    /* 6:   PMC *Config_Hash             Hash of config settings  */
    /* 7:   PMC *Lib_Paths               LoL of search paths  */
    /* 8:   PMC *PBC_Libs                Hash of load_bytecode cde */
    /* 9:   PMC *Executable              String PMC with name from argv[0]. */


    PMC *HLL_info;                            /* HLL names and types */
    PMC *HLL_namespace;                       /* cache of HLL toplevel ns */

    PMC *root_namespace;                      /* namespace hash */
    PMC *scheduler;                           /* concurrency scheduler */

    MMD_Cache *op_mmd_cache;                  /* MMD cache for builtins. */

    struct _Caches * caches;                  /* see caches.h */

    STRING     **const_cstring_table;         /* CONST_STRING(x) items */
    Hash        *const_cstring_hash;          /* cache of const_string items */

    struct QUEUE* task_queue;                 /* per interpreter queue */
    struct _handler_node_t *exit_handler_list;/* exit.c */
    int sleeping;                             /* used during sleep in events */

    struct parrot_runloop_t *current_runloop;   /* internal runloop jump point stack */
    struct parrot_runloop_t *runloop_jmp_free_list; /* and free list */

    int current_runloop_level;                /* for reentering run loop */
    int current_runloop_id;

    struct _Thread_data *thread_data;         /* thread specific items */

    UINTVAL recursion_limit;                  /* Sub call resursion limit */

    /* during a call sequencer the caller fills these objects
     * inside the invoke these get moved to the context structure */
    PMC *current_cont;                        /* the return continuation PMC */
    PMC *current_object;                      /* invocant, if a method call */
};

/* typedef struct parrot_interp_t Interp;    done in parrot.h so that
                                             interpreter.h's prereq headers can
                                             use 'Interp' */

typedef enum {
    RESUME_NONE         = 0x00,
    RESUME_RESTART      = 0x01,
    RESUME_ISJ          = 0x02,
    RESUME_INITIAL      = 0x04
} resume_flag_enum;

/* &gen_from_enum(iglobals.pasm) */
typedef enum {
    IGLOBALS_CLASSNAME_HASH,
    IGLOBALS_COMPREG_HASH,
    IGLOBALS_ARGV_LIST,
    IGLOBALS_NCI_FUNCS,
    IGLOBALS_INTERPRETER,       /* this interpreter as ParrotInterpreter PMC */
    IGLOBALS_DYN_LIBS,          /* Hash of ParrotLibrary loaded dynamic ext */
    IGLOBALS_CONFIG_HASH,
    IGLOBALS_LIB_PATHS,         /* LoL of search paths and dynamic ext */
    IGLOBALS_PBC_LIBS,          /* Hash of load_bytecode cde */
    IGLOBALS_EXECUTABLE,        /* How Parrot was invoked (from argv[0]) */

    IGLOBALS_SIZE
} iglobals_enum;
/* &end_gen */

#define PCONST(i) PF_CONST(interp->code, (i))
#define PNCONST   PF_NCONST(interp->code)

/* TODO - Make this a config option */
/* Splint complains about PMCNULL's storage, so don't use it. */
#ifdef S_SPLINT_S
#  define PARROT_CATCH_NULL 0
#else
#  define PARROT_CATCH_NULL 1
#endif

#if PARROT_CATCH_NULL
PARROT_DATA PMC    *PMCNULL;    /* Holds single Null PMC */
#else
#  define PMCNULL         ((PMC *)NULL)
#endif /* PARROT_CATCH_NULL */

/* Maybe PMC_IS_NULL(interp, pmc) ? */
#if PARROT_CATCH_NULL
#  define PMC_IS_NULL(pmc) ((pmc) == PMCNULL || (pmc) == NULL)
#else
#  define PMC_IS_NULL(pmc) (pmc) == NULL
#endif

PARROT_DATA STRING *STRINGNULL; /* a single Null STRING */

#define STRING_IS_NULL(s) ((s) == STRINGNULL || (s) == NULL)
#define STRING_IS_EMPTY(s) !(int)(s)->strlen

/* &gen_from_def(sysinfo.pasm) prefix(SYSINFO_) */

#define PARROT_INTSIZE               1
#define PARROT_FLOATSIZE             2
#define PARROT_POINTERSIZE           3
#define PARROT_OS                    4
#define PARROT_OS_VERSION            5
#define PARROT_OS_VERSION_NUMBER     6
#define CPU_ARCH                     7
#define CPU_TYPE                     8
#define PARROT_INTMAX                9
#define PARROT_INTMIN               10

/* &end_gen */

typedef opcode_t *(*native_func_t)(PARROT_INTERP,
                                   opcode_t * cur_opcode,
                                   opcode_t * start_code);

VAR_SCOPE native_func_t run_native;

typedef PMC *(*Parrot_compiler_func_t)(PARROT_INTERP,
                                       const char * program);

/* HEADERIZER BEGIN: src/interp/inter_create.c */
/* Don't modify between HEADERIZER BEGIN / HEADERIZER END.  Your changes will be lost. */

PARROT_EXPORT
PARROT_CANNOT_RETURN_NULL
Parrot_Interp make_interpreter(ARGIN_NULLOK(Interp *parent), INTVAL flags);

PARROT_EXPORT
void Parrot_destroy(PARROT_INTERP)
        __attribute__nonnull__(1);

void Parrot_really_destroy(PARROT_INTERP,
    NULLOK(int exit_code),
    SHIM(void *arg))
        __attribute__nonnull__(1);

#define ASSERT_ARGS_make_interpreter __attribute__unused__ int _ASSERT_ARGS_CHECK = (0)
#define ASSERT_ARGS_Parrot_destroy __attribute__unused__ int _ASSERT_ARGS_CHECK = (\
       PARROT_ASSERT_ARG(interp))
#define ASSERT_ARGS_Parrot_really_destroy __attribute__unused__ int _ASSERT_ARGS_CHECK = (\
       PARROT_ASSERT_ARG(interp))
/* Don't modify between HEADERIZER BEGIN / HEADERIZER END.  Your changes will be lost. */
/* HEADERIZER END: src/interp/inter_create.c */

/* HEADERIZER BEGIN: src/interp/inter_cb.c */
/* Don't modify between HEADERIZER BEGIN / HEADERIZER END.  Your changes will be lost. */

PARROT_EXPORT
void Parrot_callback_C(
    ARGIN(char *external_data),
    ARGMOD_NULLOK(PMC *user_data))
        __attribute__nonnull__(1)
        FUNC_MODIFIES(*user_data);

PARROT_EXPORT
void Parrot_callback_D(
    ARGMOD(PMC *user_data),
    ARGMOD_NULLOK(char *external_data))
        __attribute__nonnull__(1)
        FUNC_MODIFIES(*user_data)
        FUNC_MODIFIES(*external_data);

PARROT_EXPORT
PARROT_CANNOT_RETURN_NULL
PARROT_WARN_UNUSED_RESULT
PMC* Parrot_make_cb(PARROT_INTERP,
    ARGMOD(PMC* sub),
    ARGIN(PMC* user_data),
    ARGIN(STRING *cb_signature))
        __attribute__nonnull__(1)
        __attribute__nonnull__(2)
        __attribute__nonnull__(3)
        __attribute__nonnull__(4)
        FUNC_MODIFIES(* sub);

PARROT_EXPORT
void Parrot_run_callback(PARROT_INTERP,
    ARGMOD(PMC* user_data),
    ARGIN(char* external_data))
        __attribute__nonnull__(1)
        __attribute__nonnull__(2)
        __attribute__nonnull__(3)
        FUNC_MODIFIES(* user_data);

#define ASSERT_ARGS_Parrot_callback_C __attribute__unused__ int _ASSERT_ARGS_CHECK = (\
       PARROT_ASSERT_ARG(external_data))
#define ASSERT_ARGS_Parrot_callback_D __attribute__unused__ int _ASSERT_ARGS_CHECK = (\
       PARROT_ASSERT_ARG(user_data))
#define ASSERT_ARGS_Parrot_make_cb __attribute__unused__ int _ASSERT_ARGS_CHECK = (\
       PARROT_ASSERT_ARG(interp) \
    , PARROT_ASSERT_ARG(sub) \
    , PARROT_ASSERT_ARG(user_data) \
    , PARROT_ASSERT_ARG(cb_signature))
#define ASSERT_ARGS_Parrot_run_callback __attribute__unused__ int _ASSERT_ARGS_CHECK = (\
       PARROT_ASSERT_ARG(interp) \
    , PARROT_ASSERT_ARG(user_data) \
    , PARROT_ASSERT_ARG(external_data))
/* Don't modify between HEADERIZER BEGIN / HEADERIZER END.  Your changes will be lost. */
/* HEADERIZER END: src/interp/inter_cb.c */

/* HEADERIZER BEGIN: src/interp/inter_misc.c */
/* Don't modify between HEADERIZER BEGIN / HEADERIZER END.  Your changes will be lost. */

PARROT_EXPORT
PARROT_WARN_UNUSED_RESULT
INTVAL interpinfo(PARROT_INTERP, INTVAL what)
        __attribute__nonnull__(1);

PARROT_EXPORT
PARROT_WARN_UNUSED_RESULT
PARROT_CAN_RETURN_NULL
PMC* interpinfo_p(PARROT_INTERP, INTVAL what)
        __attribute__nonnull__(1);

PARROT_EXPORT
PARROT_WARN_UNUSED_RESULT
PARROT_CANNOT_RETURN_NULL
STRING* interpinfo_s(PARROT_INTERP, INTVAL what)
        __attribute__nonnull__(1);

PARROT_EXPORT
PARROT_CANNOT_RETURN_NULL
void * Parrot_compile_file(PARROT_INTERP,
    ARGIN(const char *fullname),
    ARGOUT(STRING **error))
        __attribute__nonnull__(1)
        __attribute__nonnull__(2)
        __attribute__nonnull__(3)
        FUNC_MODIFIES(*error);

PARROT_EXPORT
void Parrot_compreg(PARROT_INTERP,
    ARGIN(STRING *type),
    NOTNULL(Parrot_compiler_func_t func))
        __attribute__nonnull__(1)
        __attribute__nonnull__(2)
        __attribute__nonnull__(3);

PARROT_EXPORT
void Parrot_mark_method_writes(PARROT_INTERP,
    int type,
    ARGIN(const char *name))
        __attribute__nonnull__(1)
        __attribute__nonnull__(3);

PARROT_EXPORT
void register_nci_method(PARROT_INTERP,
    const int type,
    ARGIN(void *func),
    ARGIN(const char *name),
    ARGIN(const char *proto))
        __attribute__nonnull__(1)
        __attribute__nonnull__(3)
        __attribute__nonnull__(4)
        __attribute__nonnull__(5);

PARROT_EXPORT
void register_raw_nci_method_in_ns(PARROT_INTERP,
    const int type,
    ARGIN(void *func),
    ARGIN(STRING *name))
        __attribute__nonnull__(1)
        __attribute__nonnull__(3)
        __attribute__nonnull__(4);

PARROT_WARN_UNUSED_RESULT
INTVAL sysinfo_i(SHIM_INTERP, INTVAL info_wanted);

PARROT_CANNOT_RETURN_NULL
PARROT_WARN_UNUSED_RESULT
STRING * sysinfo_s(PARROT_INTERP, INTVAL info_wanted)
        __attribute__nonnull__(1);

#define ASSERT_ARGS_interpinfo __attribute__unused__ int _ASSERT_ARGS_CHECK = (\
       PARROT_ASSERT_ARG(interp))
#define ASSERT_ARGS_interpinfo_p __attribute__unused__ int _ASSERT_ARGS_CHECK = (\
       PARROT_ASSERT_ARG(interp))
#define ASSERT_ARGS_interpinfo_s __attribute__unused__ int _ASSERT_ARGS_CHECK = (\
       PARROT_ASSERT_ARG(interp))
#define ASSERT_ARGS_Parrot_compile_file __attribute__unused__ int _ASSERT_ARGS_CHECK = (\
       PARROT_ASSERT_ARG(interp) \
    , PARROT_ASSERT_ARG(fullname) \
    , PARROT_ASSERT_ARG(error))
#define ASSERT_ARGS_Parrot_compreg __attribute__unused__ int _ASSERT_ARGS_CHECK = (\
       PARROT_ASSERT_ARG(interp) \
    , PARROT_ASSERT_ARG(type) \
    , PARROT_ASSERT_ARG(func))
#define ASSERT_ARGS_Parrot_mark_method_writes __attribute__unused__ int _ASSERT_ARGS_CHECK = (\
       PARROT_ASSERT_ARG(interp) \
    , PARROT_ASSERT_ARG(name))
#define ASSERT_ARGS_register_nci_method __attribute__unused__ int _ASSERT_ARGS_CHECK = (\
       PARROT_ASSERT_ARG(interp) \
    , PARROT_ASSERT_ARG(func) \
    , PARROT_ASSERT_ARG(name) \
    , PARROT_ASSERT_ARG(proto))
#define ASSERT_ARGS_register_raw_nci_method_in_ns __attribute__unused__ int _ASSERT_ARGS_CHECK = (\
       PARROT_ASSERT_ARG(interp) \
    , PARROT_ASSERT_ARG(func) \
    , PARROT_ASSERT_ARG(name))
#define ASSERT_ARGS_sysinfo_i __attribute__unused__ int _ASSERT_ARGS_CHECK = (0)
#define ASSERT_ARGS_sysinfo_s __attribute__unused__ int _ASSERT_ARGS_CHECK = (\
       PARROT_ASSERT_ARG(interp))
/* Don't modify between HEADERIZER BEGIN / HEADERIZER END.  Your changes will be lost. */
/* HEADERIZER END: src/interp/inter_misc.c */


/* interpreter.c */
void runops_int(Interp *, size_t offset);
void exec_init_prederef(PARROT_INTERP,
    void *prederef_arena);
void prepare_for_run(PARROT_INTERP);
PARROT_EXPORT void dynop_register(PARROT_INTERP, PMC *op_lib);

/* interpreter.pmc */
void clone_interpreter(Parrot_Interp dest, Parrot_Interp self, INTVAL flags);

void Parrot_setup_event_func_ptrs(PARROT_INTERP);

PARROT_EXPORT void disable_event_checking(PARROT_INTERP);
PARROT_EXPORT void enable_event_checking(PARROT_INTERP);

#else /* !PARROT_IN_CORE */

typedef void * *(*native_func_t)(PARROT_INTERP,
                                 void *cur_opcode,
                                 void *start_code);

#endif   /* PARROT_IN_CORE */

#ifndef PMC_IS_NULL
#  define PMC_IS_NULL(pmc) PMC_is_null(NULL, (pmc))
#endif
#ifndef STRING_IS_NULL
#  define STRING_IS_NULL(s) ((s) == NULL || STRING_is_null(NULL, (s))
#endif

#endif   /* PARROT_INTERPRETER_H_GUARD */

/*
 * Local variables:
 *   c-file-style: "parrot"
 * End:
 * vim: expandtab shiftwidth=4:
 */
