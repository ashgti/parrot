/* thread.c
 *  Copyright: 2001-2003 The Perl Foundation.  All Rights Reserved.
 *  CVS Info
 *     $Id$
 *  Overview:
 *     Thread handling stuff
 *  Data Structure and Algorithms:
 *     Threads are created by creating new ParrotInterpreter objects.
 *  History:
 *     2003.12.18 leo initial rev
 *  Notes:
 *  References:
 *     classes/parrotinterpreter.pmc
 *     docs/dev/events.pod
 *
 */

#include "parrot/parrot.h"
#include <assert.h>


void Parrot_really_destroy(int exit_code, void *interpreter);
/*
 * the actual thread function
 */
static void*
thread_func(void *arg)
{
    PMC *self = (PMC*) arg;
    UINTVAL tid;

    Parrot_Interp interpreter = PMC_data(self);
    runops(interpreter, (opcode_t *)self->cache.struct_val -
            (opcode_t *)interpreter->code->byte_code);
    /*
     * thread is finito
     */
    LOCK(interpreter_array_mutex);
    interpreter->thread_data->state |= THREAD_STATE_FINISHED;
    tid = interpreter->thread_data->tid;
    if (interpreter != interpreter_array[tid]) {
        UNLOCK(interpreter_array_mutex);
        PANIC("thread finished: interpreter mismatch");
    }
    if ((interpreter->thread_data->state & THREAD_STATE_DETACHED) ||
        (interpreter->thread_data->state & THREAD_STATE_JOINED)) {
        interpreter_array[tid] = NULL;
        Parrot_really_destroy(0, interpreter);
    }
    UNLOCK(interpreter_array_mutex);

    /*
     * TODO pass return value
     */
    return NULL;
}

/*
 * helper functions used also for running plain interpreters
 */

/*
 * copy / clone interpreter packfile / code
 * all resources are created in the destination interpreter
 */
void
pt_clone_code(Parrot_Interp d, Parrot_Interp s)
{
    Interp_flags_SET(d, PARROT_EXTERN_CODE_FLAG);
    d->code = s->code;
}

/*
 * setup code, create ret continuation
 */
void
pt_thread_prepare_for_run(Parrot_Interp d, Parrot_Interp s)
{
    PMC *ret_c;
    pt_clone_code(d, s);
    /*
     * provide a return continuation, so that CPS returns
     * are working - create it in the new interpreters mem space
     */
    ret_c = pmc_new(d, enum_class_RetContinuation);
    d->pmc_reg.registers[1] = ret_c;
}

/*
 * ParrotThread emthods
 *
 * pt_thread_run
 * run the sub PMC in a separate thread using interpreter in dest_interp
 */
int
pt_thread_run(Parrot_Interp interp, PMC* dest_interp, PMC* sub)
{
    Parrot_Interp interpreter = PMC_data(dest_interp);

    PMC *parent = VTABLE_get_pmc_keyed_int(interp, interp->iglobals,
                IGLOBALS_INTERPRETER);

    /*
     * TODO check if thread flags are consistent
     */
    if (interp->flags & PARROT_THR_COPY_INTERP)
        clone_interpreter(dest_interp, parent);
    /*
     * TODO thread pools
     */

    dest_interp->cache.struct_val = sub->cache.struct_val;
    pt_thread_prepare_for_run(interpreter, interp);
    /*
     * set regs according to pdd03
     * P0 = sub, P2 = object
     * XXX P5, P6 are still used to invoke the thread method
     *     these should be the first params though
     */
    REG_PMC(0) = sub;
    REG_PMC(2) = dest_interp;
    /*
     * create a joinable thread
     */
    interpreter->thread_data->state = THREAD_STATE_JOINABLE;
    THREAD_CREATE_JOINABLE(interpreter->thread_data->thread,
            thread_func, dest_interp);
    return 0;
}

/*
 * run a type 1 thread
 * nothing is shared, both interpreters are free running
 * w/o any communication
 */

int
pt_thread_run_1(Parrot_Interp interp, PMC* dest_interp, PMC* sub)
{
    interp->flags |= PARROT_THR_TYPE_1;
    return pt_thread_run(interp, dest_interp, sub);
}

/*
 * run a type 2 thread
 * no shared variables, threads are communicating by sending messages
 */
int
pt_thread_run_2(Parrot_Interp interp, PMC* dest_interp, PMC* sub)
{
    interp->flags |= PARROT_THR_TYPE_2;
    return pt_thread_run(interp, dest_interp, sub);
}

/*
 * run a type 3 thread
 * threads may have shared variables and are managed in a thread pool
 */
int
pt_thread_run_3(Parrot_Interp interp, PMC* dest_interp, PMC* sub)
{
    interp->flags |= PARROT_THR_TYPE_3;
    return pt_thread_run(interp, dest_interp, sub);
}

/*
 * pt_thread_yield
 * religuish the processor
 */

void
pt_thread_yield(void)
{
    YIELD;
}

/*
 * helper, check if tid is valid - caller holds mutex
 * return interpreter for tid
 */
static Parrot_Interp
pt_check_tid(UINTVAL tid, const char *from)
{
    if (tid >= n_interpreters) {
        UNLOCK(interpreter_array_mutex);
        internal_exception(1, "%s: illegal thread tid %d", from, tid);
    }
    if (tid == 0) {
        UNLOCK(interpreter_array_mutex);
        internal_exception(1, "%s: llegal thread tid %d (main)", from, tid);
    }
    if (!interpreter_array[tid]) {
        UNLOCK(interpreter_array_mutex);
        internal_exception(1, "%s: illegal thread tid %d - empty", from, tid);
    }
    return interpreter_array[tid];
}

/*
 * join (wait for) a joinable thread
 */
void*
pt_thread_join(UINTVAL tid)
{
    Parrot_Interp interpreter;
    int state;

    LOCK(interpreter_array_mutex);
    interpreter = pt_check_tid(tid, "join");
    if (interpreter->thread_data->state == THREAD_STATE_JOINABLE ||
            interpreter->thread_data->state == THREAD_STATE_FINISHED) {
        void *retval;
        interpreter->thread_data->state |= THREAD_STATE_JOINED;
        UNLOCK(interpreter_array_mutex);
        JOIN(interpreter->thread_data->thread, retval);
        LOCK(interpreter_array_mutex);
        UNLOCK(interpreter_array_mutex);
        return retval;
    }
    /*
     * when here thread was in wrong state
     */
    state = interpreter->thread_data->state;
    UNLOCK(interpreter_array_mutex);
    internal_exception(1, "join: illegal thread state %d tid %d", state, tid);
    return NULL;
}

/*
 * possibly wait for other running threads - called when destructing
 * the passed interpreter
 */
void
pt_join_threads(Parrot_Interp interpreter)
{
    size_t i;

    /*
     * if no threads where started - fine
     */
    if (!n_interpreters) {
        return;
    }
    /*
     * only the first interpreter waits for other threads
     */
    if (interpreter != interpreter_array[0]) {
        return;
    }
    LOCK(interpreter_array_mutex);

    for (i = 1; i < n_interpreters; ++i) {
        Parrot_Interp thread_interp = interpreter_array[i];
        if (thread_interp == NULL)
            continue;
        if (thread_interp->thread_data->state == THREAD_STATE_JOINABLE ||
            (thread_interp->thread_data->state & THREAD_STATE_FINISHED)) {

            void *retval;
            thread_interp->thread_data->state |= THREAD_STATE_JOINED;
            UNLOCK(interpreter_array_mutex);
            JOIN(thread_interp->thread_data->thread, retval);
            LOCK(interpreter_array_mutex);
        }
    }
    UNLOCK(interpreter_array_mutex);
    MUTEX_DESTROY(interpreter_array_mutex);
    return;
}

/*
 * detach (make non-joinable) thread
 */
void
pt_thread_detach(UINTVAL tid)
{
    Parrot_Interp interpreter;

    LOCK(interpreter_array_mutex);
    interpreter = pt_check_tid(tid, "detach");
    /*
     * if interpreter is joinable, we detach em
     */
    if (interpreter->thread_data->state == THREAD_STATE_JOINABLE ||
            interpreter->thread_data->state == THREAD_STATE_FINISHED) {
        DETACH(interpreter->thread_data->thread);
        interpreter->thread_data->state |= THREAD_STATE_DETACHED;
        if (interpreter->thread_data->state & THREAD_STATE_FINISHED) {
            interpreter_array[tid] = NULL;
        }
    }
    UNLOCK(interpreter_array_mutex);
}

/*
 * kill a thread
 */
void
pt_thread_kill(UINTVAL tid)
{
    Parrot_Interp interpreter;

    LOCK(interpreter_array_mutex);
    interpreter = pt_check_tid(tid, "kill");
    /*
     * if interpreter is joinable, we detach em
     */
    if (interpreter->thread_data->state == THREAD_STATE_JOINABLE) {
        DETACH(interpreter->thread_data->thread);
        interpreter->thread_data->state |= THREAD_STATE_DETACHED;
    }
    if (interpreter->thread_data->state & THREAD_STATE_FINISHED) {
        UNLOCK(interpreter_array_mutex);
        return;
    }
    UNLOCK(interpreter_array_mutex);
    /*
     * schedule a terminate event for that interpreter
     */
    Parrot_new_terminate_event(interpreter);
}

/*
 * threaded interpreter book-keeping
 */

/*
 * all threaded interpreters are stored in an array
 * assumes that caller holds LOCK
 */
void
pt_add_to_interpreters(Parrot_Interp interpreter, Parrot_Interp new_interp)
{
    size_t i;

    new_interp->thread_data = mem_sys_allocate_zeroed(sizeof(Thread_data));
    INTERPRETER_LOCK_INIT(new_interp);
    if (n_interpreters == 0) {
        /*
         * first time - add master interpreter and thread
         */
        interpreter_array = mem_sys_allocate(2 * sizeof(Parrot_Interp));
        interpreter_array[0] = interpreter;
        interpreter_array[1] = new_interp;
        interpreter->thread_data =
            mem_sys_allocate_zeroed(sizeof(Thread_data));
        INTERPRETER_LOCK_INIT(interpreter);
        interpreter->thread_data->tid = 0;
        new_interp ->thread_data->tid = 1;
        n_interpreters = 2;
        return;
    }
    /*
     * look for an empty slot
     */
    for (i = 0; i < n_interpreters; ++i) {
        if (interpreter_array[i] == NULL) {
            interpreter_array[i] = new_interp;
            new_interp->thread_data->tid = i;
            return;
        }
    }
    /*
     * need to resize
     */
    interpreter_array = mem_sys_realloc(interpreter_array,
            (n_interpreters + 1) * sizeof(Parrot_Interp));
    interpreter_array[n_interpreters] = new_interp;
    new_interp->thread_data->tid = n_interpreters;
    ++n_interpreters;
}

/*
 * Local variables:
 * c-indentation-style: bsd
 * c-basic-offset: 4
 * indent-tabs-mode: nil
 * End:
 *
 * vim: expandtab shiftwidth=4:
*/
