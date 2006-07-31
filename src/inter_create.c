/*
Copyright (C) 2001-2003, The Perl Foundation.
$Id$

=head1 NAME

src/inter_create.c - Parrot Interpreter Creation and Destruction

=head1 DESCRIPTION

Create or destroy a Parrot interpreter.c

=head2 Functions

=over 4

=cut

*/


#include <assert.h>
#include "parrot/parrot.h"
#include "parrot/oplib/core_ops.h"
#include "../compilers/imcc/imc.h"

#if EXEC_CAPABLE
Interp interpre;
#endif

#define ATEXIT_DESTROY

/*

=item C<static int is_env_var_set(const char* var)>

Checks whether the specified environment variable is set.

=cut

*/

static int
is_env_var_set(const char* var)
{
    int free_it, retval;
    char* value = Parrot_getenv(var, &free_it);
    if (value == NULL)
        retval = 0;
    else if (*value == '\0')
        retval = 0;
    else
        retval = ! (strcmp(value, "0") == 0);
    if (free_it)
        mem_sys_free(value);
    return retval;
}

/*

=item C<static void setup_default_compreg(Parrot_Interp interpreter)>

Setup default compiler for PASM.

=cut

*/

static void
setup_default_compreg(Parrot_Interp interpreter)
{
    STRING *pasm1 = string_from_cstring(interpreter, "PASM1", 0);

    /* register the nci compiler object */
    Parrot_compreg(interpreter, pasm1, (Parrot_compiler_func_t)PDB_compile);
}

/*

=item C<Parrot_Interp
make_interpreter(Parrot_Interp parent, Interp_flags flags)>

Create the Parrot interpreter. Allocate memory and clear the registers.

=cut

*/

void Parrot_really_destroy(Interp *, int exit_code, void *);

Parrot_Interp
make_interpreter(Parrot_Interp parent, Interp_flags flags)
{
    Interp *interpreter;
#if EXEC_CAPABLE
    extern int Parrot_exec_run;
#endif

    /* Get an empty interpreter from system memory */
#if EXEC_CAPABLE
    if (Parrot_exec_run)
        interpreter = &interpre;
    else
#endif
        interpreter = mem_sys_allocate_zeroed(sizeof(Interp));

    /*
     * the last interpreter (w/o) parent has to cleanup globals
     * so remember parent if any
     */
    SET_NULL(interpreter->lo_var_ptr);
    if (parent) {
        interpreter->parent_interpreter = parent;
    }
    else {
        SET_NULL(interpreter->parent_interpreter);
        /*
         * we need a global mutex to protect the interpreter array
         */
        MUTEX_INIT(interpreter_array_mutex);
    }
    create_initial_context(interpreter);
    interpreter->resume_flag = RESUME_INITIAL;
    /* main is called as a Sub too - this will get depth 0 then */
    CONTEXT(interpreter->ctx)->recursion_depth = -1;
    interpreter->recursion_limit = 1000;

    /* Must initialize flags here so the GC_DEBUG stuff is available before
     * mem_setup_allocator() is called. */
    interpreter->flags = flags;

    /* PANIC will fail until this is done */
    SET_NULL(interpreter->piodata);
    PIO_init(interpreter);

    if (is_env_var_set("PARROT_GC_DEBUG")) {
#if ! DISABLE_GC_DEBUG
        Interp_flags_SET(interpreter, PARROT_GC_DEBUG_FLAG);
#else
        fprintf(stderr, "PARROT_GC_DEBUG is set but the binary was compiled "
                "with DISABLE_GC_DEBUG.\n");
#endif
    }

    /* Set up the memory allocation system */
    mem_setup_allocator(interpreter);
    Parrot_block_DOD(interpreter);
    Parrot_block_GC(interpreter);

    /*
     * Set up the string subsystem
     * This also generates the constant string tables
     */
    string_init(interpreter);

    /* Set up the MMD struct */
    interpreter->binop_mmd_funcs = NULL;

    /* Go and init the MMD tables */
    mmd_add_function(interpreter, MMD_USER_FIRST - 1, (funcptr_t)0);

    /* create caches structure */
    init_object_cache(interpreter);

    /* initialize classes - this needs mmd func table */
    SET_NULL(interpreter->HLL_info);
    Parrot_init(interpreter);

    /* context data */
    /* Initialize interpreter's flags */
    PARROT_WARNINGS_off(interpreter, PARROT_WARNINGS_ALL_FLAG);

    /* same with errors */
    PARROT_ERRORS_off(interpreter, PARROT_ERRORS_ALL_FLAG);
    /* undefined globals are errors by default */
    PARROT_ERRORS_on(interpreter, PARROT_ERRORS_GLOBALS_FLAG);
    /* param count mismatch is an error by default */
    PARROT_ERRORS_on(interpreter, PARROT_ERRORS_PARAM_COUNT_FLAG );
#if 0
    /* TODO not yet - too many test failures */
    PARROT_ERRORS_on(interpreter, PARROT_ERRORS_RESULT_COUNT_FLAG );
#endif

    /* allocate stack chunk cache */
    stack_system_init(interpreter);
    /* Set up the initial register chunks */
    setup_register_stacks(interpreter);

    /* Need a user stack */
    CONTEXT(interpreter->ctx)->user_stack = new_stack(interpreter, "User");

    /* And a control stack */
    CONTEXT(interpreter->ctx)->control_stack = new_stack(interpreter, "Control");

    /* clear context introspection vars */
    SET_NULL_P(CONTEXT(interpreter->ctx)->current_sub, PMC*);
    SET_NULL_P(CONTEXT(interpreter->ctx)->current_cont, PMC*);
    SET_NULL_P(CONTEXT(interpreter->ctx)->current_object, PMC*);

    /* Load the core op func and info tables */
    interpreter->op_lib = PARROT_CORE_OPLIB_INIT(1);
    interpreter->op_count = interpreter->op_lib->op_count;
    interpreter->op_func_table = interpreter->op_lib->op_func_table;
    interpreter->op_info_table = interpreter->op_lib->op_info_table;
    SET_NULL_P(interpreter->all_op_libs, op_lib_t **);
    SET_NULL_P(interpreter->evc_func_table, op_func_t *);
    SET_NULL_P(interpreter->save_func_table, op_func_t *);

    SET_NULL_P(interpreter->code, struct PackFile *);
    SET_NULL_P(interpreter->profile, ProfData *);

    /* null out the root set registry */
    SET_NULL_P(interpreter->DOD_registry, PMC *);

    /* create exceptions list */
    interpreter->current_runloop_level = 0;
    interpreter->current_runloop_id = 0;
    Parrot_init_exceptions(interpreter);

    /* register assembler/compilers */
    setup_default_compreg(interpreter);

    /* setup stdio PMCs */
    PIO_init(interpreter);
    /* init builtin function struct */
    Parrot_init_builtins(interpreter);
    /* init IMCC compiler */
    imcc_init(interpreter);

    /* Done. Return and be done with it */

    /* Okay, we've finished doing anything that might trigger GC.
     * Actually, we could enable DOD/GC earlier, but here all setup is
     * done
     */
    Parrot_unblock_DOD(interpreter);
    Parrot_unblock_GC(interpreter);

    /* all sys running, init the event and signal stuff
     * the first or "master" interpreter is handling events and signals
     */
    SET_NULL_P(interpreter->task_queue, QUEUE*);
    SET_NULL_P(interpreter->thread_data, _Thread_data *);

    Parrot_init_events(interpreter);

#ifdef ATEXIT_DESTROY
    /*
     * if this is not a threaded interpreter, push the interpreter
     * destruction.
     * Threaded interpreters are destructed when the thread ends
     */
    if (!Interp_flags_TEST(interpreter, PARROT_IS_THREAD))
        Parrot_on_exit(interpreter, Parrot_really_destroy, NULL);
#endif

    return interpreter;
}

/*

=item C<void
Parrot_destroy(Interp *interpreter)>

Does nothing if C<ATEXIT_DESTROY> is defined. Otherwise calls
C<Parrot_really_destroy()> with exit code 0.

This function is not currently used.

=cut

*/

void
Parrot_destroy(Interp *interpreter)
{
#ifdef ATEXIT_DESTROY
    UNUSED(interpreter);
#else
    Parrot_really_destroy(0, (void*) interpreter);
#endif
}

/*

=item C<void
Parrot_really_destroy(int exit_code, void *vinterp)>

Waits for any threads to complete, then frees all allocated memory, and
closes any open file handles, etc.

Note that C<exit_code> is ignored.

=cut

*/

void
Parrot_really_destroy(Interp *interpreter, int exit_code, void *arg)
{
    UNUSED(exit_code);
    UNUSED(arg);

    /*
     * wait for threads to complete if needed
     */
    if (!interpreter->parent_interpreter) {
        pt_join_threads(interpreter);
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
    interpreter->arena_base->DOD_block_level =
        interpreter->arena_base->GC_block_level = 0;
    Parrot_do_dod_run(interpreter, DOD_finish_FLAG);

    /*
     * that doesn't get rid of constant PMCs like these in vtable->data
     * so if such a PMC needs destroy, we got a memory leak, like for
     * the SharedRef PMC
     * TODO sweep constants too or special treatment - depends on how
     *      many constant PMCs we'll create
     */

    /* Now the PIOData gets also cleared */
    PIO_finish(interpreter);

    /*
     * now all objects that need timely destruction should be finalized
     * so terminate the event loop
     */
    if (!interpreter->parent_interpreter) {
        PIO_internal_shutdown(interpreter);
        Parrot_kill_event_loop();
    }

    /* we destroy all child interpreters and the last one too,
     * if the --leak-test commandline was given
     */
    if (! (interpreter->parent_interpreter ||
                Interp_flags_TEST(interpreter, PARROT_DESTROY_FLAG)))
        return;

    if (interpreter->arena_base->de_init_gc_system)
        interpreter->arena_base->de_init_gc_system(interpreter);
    /* buffer headers, PMCs */
    Parrot_destroy_header_pools(interpreter);
    /* memory pools in resources */
    Parrot_destroy_memory_pools(interpreter);
    /* mem subsystem is dead now */
    mem_sys_free(interpreter->arena_base);
    interpreter->arena_base = NULL;
    /* cache structure */
    mem_sys_free(interpreter->caches);
    /* packfile */

    if (!Interp_flags_TEST(interpreter, PARROT_EXTERN_CODE_FLAG))  {
        struct PackFile *pf = interpreter->initial_pf;
        if (pf)
            PackFile_destroy(interpreter, pf);
    }

    /* free vtables */
    parrot_free_vtables(interpreter);
    mmd_destroy(interpreter);


    if (interpreter->profile) {
        mem_sys_free(interpreter->profile->data);
        interpreter->profile->data = NULL;
        mem_sys_free(interpreter->profile);
        interpreter->profile = NULL;
    }

    /* deinit op_lib */
    (void) PARROT_CORE_OPLIB_INIT(0);

    stack_destroy(CONTEXT(interpreter->ctx)->user_stack);
    stack_destroy(CONTEXT(interpreter->ctx)->control_stack);

    destroy_context(interpreter);

    /* predefined exceptions */
    mem_sys_free(interpreter->exception_list);
    if (interpreter->evc_func_table)
        mem_sys_free(interpreter->evc_func_table);
    /* strings, charsets, encodings - only once */
    string_deinit(interpreter);
    if (!interpreter->parent_interpreter) {
        MUTEX_DESTROY(interpreter_array_mutex);
        mem_sys_free(interpreter);
        /*
         * finally free other globals
         */
        mem_sys_free(interpreter_array);
        interpreter_array = NULL;
    }

    else {
        /* don't free a thread interpreter, if it isn't joined yet */
        if (!interpreter->thread_data || (
                    interpreter->thread_data &&
                    (interpreter->thread_data->state & THREAD_STATE_JOINED))) {
            if (interpreter->thread_data ) {
                mem_sys_free(interpreter->thread_data);
                interpreter->thread_data = NULL;
            }
            mem_sys_free(interpreter);
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
 * c-indentation-style: bsd
 * c-basic-offset: 4
 * indent-tabs-mode: nil
 * End:
 *
 * vim: expandtab shiftwidth=4:
*/
