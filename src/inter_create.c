/*
Copyright (C) 2001-2008, The Perl Foundation.
$Id$

=head1 NAME

src/inter_create.c - Parrot Interpreter Creation and Destruction

=head1 DESCRIPTION

Create or destroy a Parrot interpreter

=head2 Functions

=over 4

=cut

*/


#include "parrot/parrot.h"
#include "parrot/oplib/core_ops.h"
#include "../compilers/imcc/imc.h"
#include "inter_create.str"

/* HEADERIZER HFILE: include/parrot/interpreter.h */

/* HEADERIZER BEGIN: static */
/* Don't modify between HEADERIZER BEGIN / HEADERIZER END.  Your changes will be lost. */

PARROT_WARN_UNUSED_RESULT
static int is_env_var_set(ARGIN(const char* var))
        __attribute__nonnull__(1);

static void setup_default_compreg(PARROT_INTERP)
        __attribute__nonnull__(1);

/* Don't modify between HEADERIZER BEGIN / HEADERIZER END.  Your changes will be lost. */
/* HEADERIZER END: static */

#if EXEC_CAPABLE
    extern int Parrot_exec_run;
#endif

#if EXEC_CAPABLE
Interp interpre;
#endif

#define ATEXIT_DESTROY

/*

=item C<static int is_env_var_set>

Checks whether the specified environment variable is set.

=cut

*/

PARROT_WARN_UNUSED_RESULT
static int
is_env_var_set(ARGIN(const char* var))
{
    int free_it, retval;
    char* const value = Parrot_getenv(var, &free_it);
    if (value == NULL)
        retval = 0;
    else if (*value == '\0')
        retval = 0;
    else
        retval = !STREQ(value, "0");
    if (free_it)
        mem_sys_free(value);
    return retval;
}

/*

=item C<static void setup_default_compreg>

Setup default compiler for PASM.

=cut

*/

static void
setup_default_compreg(PARROT_INTERP)
{
    STRING * const pasm1 = CONST_STRING(interp, "PASM1");

    /* register the nci compiler object */
    Parrot_compreg(interp, pasm1, (Parrot_compiler_func_t)PDB_compile);
}

/*

=item C<Parrot_Interp make_interpreter>

Create the Parrot interpreter. Allocate memory and clear the registers.

=cut

*/

PARROT_API
PARROT_CANNOT_RETURN_NULL
Parrot_Interp
make_interpreter(ARGIN_NULLOK(Interp *parent), INTVAL flags)
{
    Interp *interp;

    /* Get an empty interpreter from system memory */
#if EXEC_CAPABLE
    if (Parrot_exec_run)
        interp = &interpre;
    else
#endif
        interp = mem_allocate_zeroed_typed(Interp);

    interp->lo_var_ptr = NULL;

    /* the last interpreter (w/o) parent has to cleanup globals
     * so remember parent if any */
    if (parent)
        interp->parent_interpreter = parent;
    else {
        interp->parent_interpreter = NULL;
        PMCNULL                    = NULL;
        /*
         * we need a global mutex to protect the interpreter array
         */
        MUTEX_INIT(interpreter_array_mutex);
    }

    create_initial_context(interp);
    interp->resume_flag = RESUME_INITIAL;

    /* main is called as a Sub too - this will get depth 0 then */
    CONTEXT(interp)->recursion_depth = (UINTVAL)-1;
    interp->recursion_limit = RECURSION_LIMIT;

    /* Must initialize flags here so the GC_DEBUG stuff is available before
     * mem_setup_allocator() is called. */
    interp->flags = flags;

    /* PANIC will fail until this is done */
    interp->piodata = NULL;
    PIO_init(interp);

    if (is_env_var_set("PARROT_GC_DEBUG")) {
#if ! DISABLE_GC_DEBUG
        Interp_flags_SET(interp, PARROT_GC_DEBUG_FLAG);
#else
        fprintf(stderr, "PARROT_GC_DEBUG is set but the binary was compiled "
                "with DISABLE_GC_DEBUG.\n");
#endif
    }

    /* Set up the memory allocation system */
    mem_setup_allocator(interp);
    Parrot_block_GC_mark(interp);
    Parrot_block_GC_sweep(interp);

    /*
     * Set up the string subsystem
     * This also generates the constant string tables
     */
    string_init(interp);

    /* Set up the MMD struct */
    interp->binop_mmd_funcs = NULL;

    /* Go and init the MMD tables */
    Parrot_mmd_add_function(interp, MMD_USER_FIRST - 1, (funcptr_t)NULL);

    /* create caches structure */
    init_object_cache(interp);

    /* initialize classes - this needs mmd func table */
    interp->HLL_info = NULL;
    init_world_once(interp);

    /* context data */
    /* Initialize interpreter's flags */
    PARROT_WARNINGS_off(interp, PARROT_WARNINGS_ALL_FLAG);

    /* same with errors */
    PARROT_ERRORS_off(interp, PARROT_ERRORS_ALL_FLAG);

    /* undefined globals are errors by default */
    PARROT_ERRORS_on(interp, PARROT_ERRORS_GLOBALS_FLAG);

    /* param count mismatch is an error by default */
    PARROT_ERRORS_on(interp, PARROT_ERRORS_PARAM_COUNT_FLAG);

#if 0
    /* TODO not yet - too many test failures */
    PARROT_ERRORS_on(interp, PARROT_ERRORS_RESULT_COUNT_FLAG);
#endif

    /* allocate stack chunk cache */
    stack_system_init(interp);

    /* And a dynamic environment stack */
    interp->dynamic_env = new_stack(interp, "DynamicEnv");

    /* clear context introspection vars */
    CONTEXT(interp)->current_sub    = NULL;
    CONTEXT(interp)->current_cont   = NULL;
    CONTEXT(interp)->current_object = NULL;

    /* Load the core op func and info tables */
    interp->op_lib          = PARROT_CORE_OPLIB_INIT(1);
    interp->op_count        = interp->op_lib->op_count;
    interp->op_func_table   = interp->op_lib->op_func_table;
    interp->op_info_table   = interp->op_lib->op_info_table;
    interp->all_op_libs     = NULL;
    interp->evc_func_table  = NULL;
    interp->save_func_table = NULL;
    interp->code            = NULL;
    interp->profile         = NULL;

    /* create the root set registry */
    interp->DOD_registry    = pmc_new(interp, enum_class_AddrRegistry);

    /* create exceptions list */
    interp->current_runloop_id    = 0;
    interp->current_runloop_level = 0;

    /* register assembler/compilers */
    setup_default_compreg(interp);

    /* setup stdio PMCs */
    PIO_init(interp);

    /* init IMCC compiler */
    imcc_init(interp);

    /* Done. Return and be done with it */

    /* Okay, we've finished doing anything that might trigger GC.
     * Actually, we could enable DOD/GC earlier, but here all setup is
     * done
     */
    Parrot_unblock_GC_mark(interp);
    Parrot_unblock_GC_sweep(interp);

    /* all sys running, init the event and signal stuff
     * the first or "master" interpreter is handling events and signals
     */
    interp->task_queue  = NULL;
    interp->thread_data = NULL;

    Parrot_cx_init_scheduler(interp);

#ifdef ATEXIT_DESTROY
    /*
     * if this is not a threaded interpreter, push the interpreter
     * destruction.
     * Threaded interpreters are destructed when the thread ends
     */
    if (!Interp_flags_TEST(interp, PARROT_IS_THREAD))
        Parrot_on_exit(interp, Parrot_really_destroy, NULL);
#endif

    return interp;
}

/*

=item C<void Parrot_destroy>

Does nothing if C<ATEXIT_DESTROY> is defined. Otherwise calls
C<Parrot_really_destroy()> with exit code 0.

This function is not currently used.

=cut

*/

PARROT_API
void
Parrot_destroy(PARROT_INTERP)
{
#ifdef ATEXIT_DESTROY
    UNUSED(interp);
#else
    Parrot_really_destroy(interp, 0);
#endif
}

/*

=item C<void Parrot_really_destroy>

Waits for any threads to complete, then frees all allocated memory, and
closes any open file handles, etc.

Note that C<exit_code> is ignored.

=cut

*/

void
Parrot_really_destroy(PARROT_INTERP, SHIM(int exit_code), SHIM(void *arg))
{
    /* wait for threads to complete if needed; terminate the event loop */
    if (!interp->parent_interpreter) {
        Parrot_cx_runloop_end(interp);
        pt_join_threads(interp);
    }

    /* if something needs destruction (e.g. closing PIOs)
     * we must destroy it now:
     *
     * Be sure that an async collector hasn't live bits set now, so
     * trigger a finish run
     *
     * Need to turn off DOD blocking, else things stay alive and IO
     * handles aren't closed
     */
    interp->arena_base->DOD_block_level    =
        interp->arena_base->GC_block_level = 0;

    if (Interp_trace_TEST(interp, ~0)) {
        PIO_eprintf(interp, "ParrotIO objects (like stdout and stderr)"
            "are about to be closed, so clearing trace flags.\n");
        Interp_trace_CLEAR(interp, ~0);
    }

    /* Destroys all PMCs, even constants and the ParrotIO objects for
     * std{in, out, err}, so don't be verbose about DOD'ing. */
    if (interp->thread_data)
        interp->thread_data->state |= THREAD_STATE_SUSPENDED_GC;

    Parrot_do_dod_run(interp, GC_finish_FLAG);

#if STM_PROFILE
    if (interp->thread_data && interp->thread_data->stm_log
    && !interp->parent_interpreter
    &&  Interp_debug_TEST(interp, PARROT_THREAD_DEBUG_FLAG))
        Parrot_STM_dump_profile(interp);
#endif

    /*
     * that doesn't get rid of constant PMCs like these in vtable->data
     * so if such a PMC needs destroying, we get a memory leak, like for
     * the SharedRef PMC
     * TODO sweep constants too or special treatment - depends on how
     *      many constant PMCs we'll create
     */

    /* destroy IMCC compiler */
    imcc_destroy(interp);

    /* Now the PIOData gets also cleared */
    PIO_finish(interp);

    /*
     * now all objects that need timely destruction should be finalized
     * so terminate the event loop
     */
    if (!interp->parent_interpreter) {
        PIO_internal_shutdown(interp);
/*        Parrot_kill_event_loop(interp); */
    }

    /* we destroy all child interpreters and the last one too,
     * if the --leak-test commandline was given
     */
    if (! (interp->parent_interpreter ||
                Interp_flags_TEST(interp, PARROT_DESTROY_FLAG)))
        return;

    if (interp->thread_data && interp->thread_data->stm_log) {
        while (Parrot_STM_transaction_depth(interp) > 0) {
            /* XXX */
            fprintf(stderr, "interpreter %p had pending transaction on exit\n",
                    (void *) interp);
            Parrot_STM_abort(interp);
        }
#if STM_PROFILE
        if (interp->parent_interpreter
            && interp->thread_data->state & THREAD_STATE_JOINED)
            Parrot_STM_merge_profile(interp->parent_interpreter,
                                     interp);
#endif
        Parrot_STM_destroy(interp);
    }

    if (interp->parent_interpreter &&
        interp->thread_data &&
        (interp->thread_data->state & THREAD_STATE_JOINED)) {
        Parrot_merge_header_pools(interp->parent_interpreter, interp);
        Parrot_merge_memory_pools(interp->parent_interpreter, interp);
    }

    if (interp->arena_base->finalize_gc_system)
        interp->arena_base->finalize_gc_system(interp);

    /* copies of constant tables */
    Parrot_destroy_constants(interp);

    /* buffer headers, PMCs */
    Parrot_destroy_header_pools(interp);

    /* memory pools in resources */
    Parrot_destroy_memory_pools(interp);

    /* mem subsystem is dead now */
    mem_sys_free(interp->arena_base);
    interp->arena_base = NULL;

    /* cache structure */
    destroy_object_cache(interp);

    /* packfile */
    if (interp->initial_pf)
        PackFile_destroy(interp, interp->initial_pf);

    if (interp->profile) {
        mem_sys_free(interp->profile->data);
        interp->profile->data = NULL;
        mem_sys_free(interp->profile);
        interp->profile = NULL;
    }

    /* deinit op_lib */
    (void) PARROT_CORE_OPLIB_INIT(0);

    stack_destroy(interp->dynamic_env);

    destroy_context(interp);
    destroy_runloop_jump_points(interp);

    if (interp->evc_func_table) {
        mem_sys_free(interp->evc_func_table);
        interp->evc_func_table = NULL;
    }

    /* strings, charsets, encodings - only once */
    string_deinit(interp);

    if (!interp->parent_interpreter) {
        if (interp->thread_data)
            mem_sys_free(interp->thread_data);

        /* free vtables */
        parrot_free_vtables(interp);
        Parrot_mmd_destroy(interp);

        MUTEX_DESTROY(interpreter_array_mutex);
        mem_sys_free(interp);
        /*
         * finally free other globals
         */
        mem_sys_free(interpreter_array);
        interpreter_array = NULL;
    }

    else {
        /* don't free a thread interpreter, if it isn't joined yet */
        if (!interp->thread_data || (
                    interp->thread_data &&
                    (interp->thread_data->state & THREAD_STATE_JOINED))) {
            if (interp->thread_data) {
                mem_sys_free(interp->thread_data);
                interp->thread_data = NULL;
            }
            mem_sys_free(interp);
        }
    }
}

/*

=back

=head1 SEE ALSO

F<include/parrot/interpreter.h>, F<src/interpreter.c>.

=cut

*/

/*
 * Local variables:
 *   c-file-style: "parrot"
 * End:
 * vim: expandtab shiftwidth=4:
 */
