/*
Copyright (C) 2001-2010, Parrot Foundation.
$Id$

=head1 NAME

src/interp/inter_create.c - Parrot Interpreter Creation and Destruction

=head1 DESCRIPTION

Create or destroy a Parrot interpreter

=head2 Functions

=over 4

=cut

*/


#include "parrot/parrot.h"
#include "parrot/runcore_api.h"
#include "parrot/oplib/core_ops.h"
#include "../compilers/imcc/imc.h"
#include "pmc/pmc_callcontext.h"
#include "../gc/gc_private.h"
#include "inter_create.str"

/* HEADERIZER HFILE: include/parrot/interpreter.h */

/* HEADERIZER BEGIN: static */
/* Don't modify between HEADERIZER BEGIN / HEADERIZER END.  Your changes will be lost. */

PARROT_WARN_UNUSED_RESULT
static int is_env_var_set(PARROT_INTERP, ARGIN(STRING* var))
        __attribute__nonnull__(1)
        __attribute__nonnull__(2);

#define ASSERT_ARGS_is_env_var_set __attribute__unused__ int _ASSERT_ARGS_CHECK = (\
       PARROT_ASSERT_ARG(interp) \
    , PARROT_ASSERT_ARG(var))
/* Don't modify between HEADERIZER BEGIN / HEADERIZER END.  Your changes will be lost. */
/* HEADERIZER END: static */

#define ATEXIT_DESTROY

/*

=item C<static int is_env_var_set(PARROT_INTERP, STRING* var)>

Checks whether the specified environment variable is set.

=cut

*/

PARROT_WARN_UNUSED_RESULT
static int
is_env_var_set(PARROT_INTERP, ARGIN(STRING* var))
{
    ASSERT_ARGS(is_env_var_set)
    int retval;
    char* const value = Parrot_getenv(interp, var);
    if (value == NULL)
        retval = 0;
    else if (*value == '\0')
        retval = 0;
    else
        retval = !STREQ(value, "0");
    return retval;
}

/*

=item C<Parrot_Interp make_interpreter(Interp *parent, INTVAL flags)>

Create the Parrot interpreter. Allocate memory and clear the registers.

=cut

*/

PARROT_EXPORT
PARROT_CANNOT_RETURN_NULL
Parrot_Interp
make_interpreter(ARGIN_NULLOK(Interp *parent), INTVAL flags)
{
    ASSERT_ARGS(make_interpreter)
    int stacktop;
    Interp *interp;

    interp = allocate_interpreter(parent, flags);
    initialize_interpreter(interp, (void*)&stacktop);
    return interp;
}

/*

=item C<Parrot_Interp allocate_interpreter(Interp *parent, INTVAL flags)>

Allocate new interpreter from system memory. Everything is preallocated but not
initialized. Used in next cycle:

    allocate_interpreter
    parseflags
    initialize_interpreter

for overriding subsystems (e.g. GC) which require early initialization.

=cut

*/

PARROT_EXPORT
PARROT_CANNOT_RETURN_NULL
Parrot_Interp
allocate_interpreter(ARGIN_NULLOK(Interp *parent), INTVAL flags)
{
    ASSERT_ARGS(allocate_interpreter)
    int stacktop;
    Interp *interp;

    /* Get an empty interpreter from system memory */
    interp = mem_internal_allocate_zeroed_typed(Interp);

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

    /* Must initialize flags before Parrot_gc_initialize() is called
     * so the GC_DEBUG stuff is available. */
    interp->flags = flags;

    interp->ctx         = PMCNULL;
    interp->resume_flag = RESUME_INITIAL;

    interp->recursion_limit = RECURSION_LIMIT;

    /* PANIC will fail until this is done */
    interp->piodata = NULL;

    /* create exceptions list */
    interp->current_runloop_id    = 0;
    interp->current_runloop_level = 0;

    /* Allocate IMCC info */
    IMCC_INFO(interp) = mem_internal_allocate_zeroed_typed(imc_info_t);

    interp->gc_sys           = mem_internal_allocate_zeroed_typed(GC_Subsystem);
    interp->gc_sys->sys_type = parent
                                    ? parent->gc_sys->sys_type
                                    : PARROT_GC_DEFAULT_TYPE;

    /* Done. Return and be done with it */
    return interp;
}

/*

=item C<Parrot_Interp initialize_interpreter(PARROT_INTERP, void *stacktop)>

Initialize previously allocated interpreter.

=cut

*/

PARROT_EXPORT
PARROT_CANNOT_RETURN_NULL
Parrot_Interp
initialize_interpreter(PARROT_INTERP, ARGIN(void *stacktop))
{
    ASSERT_ARGS(initialize_interpreter)

    /* Set up the memory allocation system */
    Parrot_gc_initialize(interp, stacktop);
    Parrot_block_GC_mark(interp);
    Parrot_block_GC_sweep(interp);

    interp->ctx         = PMCNULL;
    interp->resume_flag = RESUME_INITIAL;

    interp->recursion_limit = RECURSION_LIMIT;

    /* PANIC will fail until this is done */
    interp->piodata = NULL;
    Parrot_io_init(interp);

    /*
     * Set up the string subsystem
     * This also generates the constant string tables
     */
    Parrot_str_init(interp);

    /* Set up MMD; MMD cache for builtins. */
    interp->op_mmd_cache = Parrot_mmd_cache_create(interp);

    /* create caches structure */
    init_object_cache(interp);

    /* initialize classes - this needs mmd func table */
    interp->HLL_info = NULL;

    Parrot_initialize_core_vtables(interp);
    init_world_once(interp);

    /* context data */
    if (is_env_var_set(interp, CONST_STRING(interp, "PARROT_GC_DEBUG"))) {
#if ! DISABLE_GC_DEBUG
        Interp_flags_SET(interp, PARROT_GC_DEBUG_FLAG);
#else
        fprintf(stderr, "PARROT_GC_DEBUG is set but the binary was compiled "
                "with DISABLE_GC_DEBUG.\n");
#endif
    }

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

    create_initial_context(interp);

    /* clear context introspection vars */
    Parrot_pcc_set_sub(interp, CURRENT_CONTEXT(interp), NULL);
    Parrot_pcc_set_continuation(interp, CURRENT_CONTEXT(interp), NULL); /* TODO Use PMCNULL */
    Parrot_pcc_set_object(interp, CURRENT_CONTEXT(interp), NULL);

    /* initialize built-in runcores */
    Parrot_runcore_init(interp);

    /* Load the core op func and info tables */
    interp->op_lib          = PARROT_CORE_OPLIB_INIT(interp, 1);
    interp->op_count        = interp->op_lib->op_count;
    interp->op_func_table   = interp->op_lib->op_func_table;
    interp->op_info_table   = interp->op_lib->op_info_table;
    interp->all_op_libs     = NULL;
    interp->evc_func_table  = NULL;
    interp->save_func_table = NULL;
    interp->code            = NULL;

    /* create the root set registry */
    interp->gc_registry     = Parrot_pmc_new(interp, enum_class_AddrRegistry);

    /* And a dynamic environment stack */
    /* TODO: We should really consider removing this (TT #876) */
    interp->dynamic_env = Parrot_pmc_new(interp, enum_class_ResizablePMCArray);

    /* create exceptions list */
    interp->current_runloop_id    = 0;
    interp->current_runloop_level = 0;

    /* setup stdio PMCs */
    Parrot_io_init(interp);

    /* init IMCC compiler */
    imcc_init(interp);

    /* Done. Return and be done with it */

    /* Okay, we've finished doing anything that might trigger GC.
     * Actually, we could enable GC earlier, but here all setup is
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

=item C<void Parrot_destroy(PARROT_INTERP)>

Does nothing if C<ATEXIT_DESTROY> is defined. Otherwise calls
C<Parrot_really_destroy()> with exit code 0.

This function is not currently used.

=cut

*/

PARROT_EXPORT
void
Parrot_destroy(PARROT_INTERP)
{
    ASSERT_ARGS(Parrot_destroy)
#ifdef ATEXIT_DESTROY
    UNUSED(interp);
#else
    Parrot_really_destroy(interp, 0);
#endif
}

/*

=item C<void Parrot_really_destroy(PARROT_INTERP, int exit_code, void *arg)>

Waits for any threads to complete, then frees all allocated memory, and
closes any open file handles, etc.

Note that C<exit_code> is ignored.

=cut

*/

void
Parrot_really_destroy(PARROT_INTERP, SHIM(int exit_code), SHIM(void *arg))
{
    ASSERT_ARGS(Parrot_really_destroy)
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
     * Need to turn off GC blocking, else things stay alive and IO
     * handles aren't closed
     */
    Parrot_gc_completely_unblock(interp);

    if (Interp_trace_TEST(interp, ~0)) {
        Parrot_io_eprintf(interp, "FileHandle objects (like stdout and stderr)"
            "are about to be closed, so clearing trace flags.\n");
        Interp_trace_CLEAR(interp, ~0);
    }

    /* Destroys all PMCs, even constants and the FileHandle objects for
     * std{in, out, err}, so don't be verbose about GC'ing. */
    if (interp->thread_data)
        interp->thread_data->state |= THREAD_STATE_SUSPENDED_GC;

    Parrot_gc_mark_and_sweep(interp, GC_finish_FLAG);

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
    Parrot_io_finish(interp);

    /* deinit runcores and dynamic op_libs */
    if (!interp->parent_interpreter)
        Parrot_runcore_destroy(interp);

    /*
     * now all objects that need timely destruction should be finalized
     * so terminate the event loop
     */
 /*   if (!interp->parent_interpreter) {
        PIO_internal_shutdown(interp);
        Parrot_kill_event_loop(interp);
    }
  */

    /* we destroy all child interpreters and the last one too,
     * if the --leak-test commandline was given */
    if (! (interp->parent_interpreter
    ||    Interp_flags_TEST(interp, PARROT_DESTROY_FLAG)))
        return;

    if (interp->parent_interpreter
    &&  interp->thread_data
    && (interp->thread_data->state & THREAD_STATE_JOINED)) {
        Parrot_gc_destroy_child_interp(interp->parent_interpreter, interp);
    }

    /* MMD cache */
    Parrot_mmd_cache_destroy(interp, interp->op_mmd_cache);

    /* copies of constant tables */
    Parrot_destroy_constants(interp);

    destroy_runloop_jump_points(interp);

    /* packfile */
    if (interp->initial_pf)
        PackFile_destroy(interp, interp->initial_pf);
    /* cache structure */
    destroy_object_cache(interp);

    if (interp->evc_func_table) {
        mem_gc_free(interp, interp->evc_func_table);
        interp->evc_func_table = NULL;
    }

    /* strings, charsets, encodings - only once */
    Parrot_str_finish(interp);

    PARROT_CORE_OPLIB_INIT(interp, 0);

    if (!interp->parent_interpreter) {
        if (interp->thread_data)
            mem_internal_free(interp->thread_data);

        /* free vtables */
        parrot_free_vtables(interp);

        /* Finalyze GC */
        Parrot_gc_finalize(interp);

        MUTEX_DESTROY(interpreter_array_mutex);
        mem_internal_free(interp);

        /* finally free other globals */
        mem_internal_free(interpreter_array);
        interpreter_array = NULL;
    }

    else {
        /* don't free a thread interpreter, if it isn't joined yet */
        if (!interp->thread_data
        || (interp->thread_data
        && (interp->thread_data->state & THREAD_STATE_JOINED))) {
            if (interp->thread_data) {
                mem_internal_free(interp->thread_data);
                interp->thread_data = NULL;
            }

            parrot_free_vtables(interp);

            /* Finalyze GC */
            Parrot_gc_finalize(interp);

            mem_internal_free(interp);
        }
    }
}


/*

=back

=head1 SEE ALSO

L<include/parrot/interpreter.h>, L<src/interp/interpreter.c>.

=cut

*/

/*
 * Local variables:
 *   c-file-style: "parrot"
 * End:
 * vim: expandtab shiftwidth=4:
 */
