/*
Copyright (C) 2001-2007, The Perl Foundation.
$Id$

=head1 NAME

src/exceptions.c - Exceptions

=head1 DESCRIPTION

Define the internal interpreter exceptions.

=over 4

=item * This is experimental code.

=item * The C<enum_class> of the Exception isn't fixed.

=item * The interface isn't fixed.

=item * Much of this may change in the future.

=back

=head2 Functions

=over 4

=cut

*/

#include "parrot/parrot.h"
#include "parrot/exceptions.h"

#ifdef PARROT_HAS_GLIBC_BACKTRACE
#  include <execinfo.h>
#endif

/* HEADERIZER HFILE: include/parrot/exceptions.h */

/* HEADERIZER BEGIN: static */

PARROT_CAN_RETURN_NULL
PARROT_WARN_UNUSED_RESULT
static opcode_t * create_exception(PARROT_INTERP)
        __attribute__nonnull__(1);

PARROT_WARN_UNUSED_RESULT
static size_t dest2offset(PARROT_INTERP, ARGIN(const opcode_t *dest))
        __attribute__nonnull__(1)
        __attribute__nonnull__(2);

PARROT_WARN_UNUSED_RESULT
PARROT_CAN_RETURN_NULL
static PMC * find_exception_handler(PARROT_INTERP, ARGIN(PMC *exception))
        __attribute__nonnull__(1)
        __attribute__nonnull__(2);

static void run_cleanup_action(PARROT_INTERP, ARGIN(Stack_Entry_t *e))
        __attribute__nonnull__(1)
        __attribute__nonnull__(2);

/* HEADERIZER END: static */

#include <stdarg.h>

/*

=item C<void internal_exception>

Signal a fatal exception.  This involves printing an error message to stderr,
and calling C<Parrot_exit> to invoke exit handlers and exit the process with the
given exitcode.  No error handlers are used, so it is not possible for Parrot
bytecode to intercept a fatal error (cf. C<real_exception>).  Furthermore, no
stack unwinding is done, so the exit handlers run in the current dynamic
environment.

=cut

*/

PARROT_API
PARROT_DOES_NOT_RETURN
void
internal_exception(int exitcode, ARGIN(const char *format), ...)
{
    va_list arglist;
    va_start(arglist, format);
    vfprintf(stderr, format, arglist);
    fprintf(stderr, "\n");
    /* caution against output swap (with PDB_backtrace) */
    fflush(stderr);
    va_end(arglist);
/*
 * RT#45907 get rid of all the internal_exceptions or call them
 *          with an interpreter arg
    Parrot_exit(interp, exitcode);
 */
    exit(exitcode);
}

/* Panic handler */

#ifndef dumpcore
#  define dumpcore() \
     fprintf(stderr, "Sorry, coredump is not yet implemented " \
             "for this platform.\n\n"); \
             exit(EXIT_FAILURE);
#endif

/*

=item C<void do_panic>

Panic handler.

=cut

*/

PARROT_DOES_NOT_RETURN
void
do_panic(NULLOK_INTERP, ARGIN_NULLOK(const char *message),
         ARGIN_NULLOK(const char *file), unsigned int line)
{
    /* Note: we can't format any floats in here--Parrot_sprintf
    ** may panic because of floats.
    ** and we don't use Parrot_sprintf or such, because we are
    ** already in panic --leo
    */
    fprintf(stderr, "Parrot VM: PANIC: %s!\n",
               message ? message : "(no message available)");

    fprintf(stderr, "C file %s, line %u\n",
               file ? file : "(not available)", line);

    fprintf(stderr, "Parrot file (not available), ");
    fprintf(stderr, "line (not available)\n");

    fprintf(stderr, "\n\
We highly suggest you notify the Parrot team if you have not been working on\n\
Parrot.  Use parrotbug (located in parrot's root directory) or send an\n\
e-mail to parrot-porters@perl.org.\n\
Include the entire text of this error message and the text of the script that\n\
generated the error.  If you've made any modifications to Parrot, please\n\
describe them as well.\n\n");

    fprintf(stderr, "Version     : %s\n", PARROT_VERSION);
    fprintf(stderr, "Configured  : %s\n", PARROT_CONFIG_DATE);
    fprintf(stderr, "Architecture: %s\n", PARROT_ARCHNAME);
    fprintf(stderr, "JIT Capable : %s\n", JIT_CAPABLE ? "Yes" : "No");
    if (interp)
        fprintf(stderr, "Interp Flags: %#x\n", (unsigned int)interp->flags);
    else
        fprintf(stderr, "Interp Flags: (no interpreter)\n");
    fprintf(stderr, "Exceptions  : %s\n", "(missing from core)");
    fprintf(stderr, "\nDumping Core...\n");

    dumpcore();
    exit(EXIT_FAILURE);
}

/*

=item C<void push_exception>

Add the exception handler on the stack.

=cut

*/

PARROT_API
void
push_exception(PARROT_INTERP, ARGIN(PMC *handler))
{
    if (handler->vtable->base_type != enum_class_Exception_Handler)
        PANIC(interp, "Tried to set_eh a non Exception_Handler");
    stack_push(interp, &interp->dynamic_env, handler,
               STACK_ENTRY_PMC, STACK_CLEANUP_NULL);
}

/*

=item C<static void run_cleanup_action>

RT#48260: Not yet documented!!!

=cut

*/

static void
run_cleanup_action(PARROT_INTERP, ARGIN(Stack_Entry_t *e))
{
    /*
     * this is called during normal stack_pop of the control
     * stack - run the action subroutine with an INTVAL arg of 0
     */
    PMC * const sub = UVal_pmc(e->entry);
    Parrot_runops_fromc_args(interp, sub, "vI", 0);
}

/*

=item C<void Parrot_push_action>

Push an action handler onto the dynamic environment.

=cut

*/

PARROT_API
void
Parrot_push_action(PARROT_INTERP, ARGIN(PMC *sub))
{
    if (!VTABLE_isa(interp, sub,
                const_string(interp, "Sub"))) {
        real_exception(interp, NULL, 1, "Tried to push a non Sub PMC action");
    }
    stack_push(interp, &interp->dynamic_env, sub,
               STACK_ENTRY_ACTION, run_cleanup_action);
}

/*

=item C<void Parrot_push_mark>

Push a cleanup mark onto the dynamic environment.

=cut

*/

PARROT_API
void
Parrot_push_mark(PARROT_INTERP, INTVAL mark)
{
    stack_push(interp, &interp->dynamic_env, &mark,
               STACK_ENTRY_MARK, STACK_CLEANUP_NULL);
}

/*

=item C<void Parrot_pop_mark>

Pop items off the dynamic environment up to the mark.

=cut

*/

PARROT_API
void
Parrot_pop_mark(PARROT_INTERP, INTVAL mark)
{
    do {
        const Stack_Entry_t * const e
            = stack_entry(interp, interp->dynamic_env, 0);
        if (!e)
            real_exception(interp, NULL, 1,
                           "Mark %ld not found.", (long)mark);
        (void)stack_pop(interp, &interp->dynamic_env,
                        NULL, e->entry_type);
        if (e->entry_type == STACK_ENTRY_MARK) {
            if (UVal_int(e->entry) == mark)
                return;
        }
    } while (1);
}

/*

=item C<static PMC * find_exception_handler>

Find the exception handler for C<exception>.

=cut

*/

PARROT_WARN_UNUSED_RESULT
PARROT_CAN_RETURN_NULL
static PMC *
find_exception_handler(PARROT_INTERP, ARGIN(PMC *exception))
{
    char *m;
    int exit_status, print_location;
    int depth = 0;
    Stack_Entry_t *e;

    /* for now, we don't check the exception class and we don't
     * look for matching handlers.  [this is being redesigned anyway.]
     */
    STRING * const message = VTABLE_get_string_keyed_int(interp, exception, 0);

    /* [RT#45909: replace quadratic search with something linear, hopefully
     * without trashing abstraction layers.  -- rgr, 17-Sep-06.] */
    while ((e = stack_entry(interp, interp->dynamic_env, depth)) != NULL) {
        if (e->entry_type == STACK_ENTRY_PMC) {
            PMC * const handler = UVal_pmc(e->entry);
            if (handler && handler->vtable->base_type ==
                    enum_class_Exception_Handler) {
                return handler;
            }
        }
        depth++;
    }

    /* flush interpreter output to get things printed in order */
    PIO_flush(interp, PIO_STDOUT(interp));
    PIO_flush(interp, PIO_STDERR(interp));
    if (interp->debugger) {
        PIO_flush(interp->debugger, PIO_STDOUT(interp->debugger));
        PIO_flush(interp->debugger, PIO_STDERR(interp->debugger));
    }

    m = string_to_cstring(interp, message);
    exit_status = print_location = 1;
    if (m && *m) {
        fputs(m, stderr);
        if (m[strlen(m)-1] != '\n')
            fprintf(stderr, "%c", '\n');
        string_cstring_free(m);
    }
    else {
        if (m)
            string_cstring_free(m); /* coverity fix, m was allocated but was "\0" */
        /* new block for const assignment */
        {
            const INTVAL severity = VTABLE_get_integer_keyed_int(interp, exception, 2);
            if (severity == EXCEPT_exit) {
                print_location = 0;
                exit_status =
                    (int)VTABLE_get_integer_keyed_int(interp, exception, 1);
            }
            else
                fprintf(stderr, "No exception handler and no message\n");
        }
    }
    /* caution against output swap (with PDB_backtrace) */
    fflush(stderr);
    if (print_location)
        PDB_backtrace(interp);
    /*
     * returning NULL from here returns resume address NULL to the
     * runloop, which will terminate the thread function finally
     *
     * RT#45917 this check should better be in Parrot_exit
     */
    if (interp->thread_data && interp->thread_data->tid) {
        /*
         * we should probably detach the thread here
         */
        return NULL;
    }
    /*
     * only main should run the destroy functions - exit handler chain
     * is freed during Parrot_exit
     */
    Parrot_exit(interp, exit_status);
}

/*

=item C<INTVAL count_exception_handlers>

Return the number of exception handlers on the exeception handler stack.

=cut

*/

PARROT_WARN_UNUSED_RESULT
INTVAL
count_exception_handlers(PARROT_INTERP)
{
    INTVAL stack_depth = 0;
    INTVAL eh_depth = 0;
    Stack_Entry_t *e;

    /* Not all entries in the stack are exception handlers, so iterate over the
     * stack, counting exception handler entries. */
    while ((e = stack_entry(interp, interp->dynamic_env, stack_depth)) != NULL) {
        if (e->entry_type == STACK_ENTRY_PMC) {
            PMC * const handler = UVal_pmc(e->entry);
            if (handler && handler->vtable->base_type ==
                    enum_class_Exception_Handler) {
                eh_depth++;
            }
        }
        stack_depth++;
    }

    return eh_depth;
}

/*

=item C<PMC * get_exception_handler>

Return an exception handler by index into the exeception handler stack.

=cut

*/

PARROT_WARN_UNUSED_RESULT
PARROT_CAN_RETURN_NULL
PMC *
get_exception_handler(PARROT_INTERP, INTVAL target_depth)
{
    INTVAL stack_depth = 0;
    INTVAL eh_depth = 0;
    Stack_Entry_t *e;

    /* Not all entries in the stack are exception handlers, so iterate over the
     * stack, counting exception handler entries. */
    while ((e = stack_entry(interp, interp->dynamic_env, stack_depth)) != NULL) {
        if (e->entry_type == STACK_ENTRY_PMC) {
            PMC * const handler = UVal_pmc(e->entry);
            if (handler && handler->vtable->base_type ==
                    enum_class_Exception_Handler) {
                if (eh_depth == target_depth) {
                    return handler;
                }
                eh_depth++;
            }
        }
        stack_depth++;
    }

    return PMCNULL;
}

/*

=item C<PMC * get_all_exception_handlers>

Return an array of all exception handlers.

=cut

*/

PARROT_WARN_UNUSED_RESULT
PARROT_CANNOT_RETURN_NULL
PMC *
get_all_exception_handlers(PARROT_INTERP)
{
    int depth = 0;
    Stack_Entry_t *e;
    PMC * const all_entries = pmc_new(interp, enum_class_ResizablePMCArray);

    while ((e = stack_entry(interp, interp->dynamic_env, depth)) != NULL) {
        if (e->entry_type == STACK_ENTRY_PMC) {
            PMC * const handler = UVal_pmc(e->entry);
            if (handler && handler->vtable->base_type ==
                    enum_class_Exception_Handler) {
                VTABLE_push_pmc(interp, all_entries, handler);
            }
        }
        depth++;
    }

    return all_entries;
}

/*

=item C<void pop_exception>

Pops the topmost exception handler off the stack.

=cut

*/

PARROT_API
void
pop_exception(PARROT_INTERP)
{
    Stack_entry_type  type;
    Parrot_cont      *cc;

    PMC * const handler
        = (PMC *)stack_peek(interp, interp->dynamic_env, &type);

    if (! handler
            || type != STACK_ENTRY_PMC
            || handler->vtable->base_type != enum_class_Exception_Handler) {
        real_exception(interp, NULL, E_RuntimeError,
                "No exception to pop.");
    }
    cc = PMC_cont(handler);
    if (cc->to_ctx != CONTEXT(interp->ctx)) {
        real_exception(interp, NULL, E_RuntimeError,
                "No exception to pop.");
    }
    (void)stack_pop(interp, &interp->dynamic_env, NULL, STACK_ENTRY_PMC);
}

/*

=item C<PMC* new_c_exception_handler>

Generate an exception handler, that catches PASM level exceptions inside
a C function. This could be a separate class too, for now just a private
flag bit is set.

=cut

*/

PARROT_API
PARROT_WARN_UNUSED_RESULT
PARROT_CANNOT_RETURN_NULL
PMC*
new_c_exception_handler(PARROT_INTERP, ARGIN(Parrot_exception *jb))
{
    PMC * const handler = pmc_new(interp, enum_class_Exception_Handler);
    /*
     * this flag denotes a C exception handler
     */
    PObj_get_FLAGS(handler) |= SUB_FLAG_C_HANDLER;
    VTABLE_set_pointer(interp, handler, jb);
    return handler;
}

/*

=item C<void push_new_c_exception_handler>

Pushes an new C exception handler onto the stack.

=cut

*/

PARROT_API
void
push_new_c_exception_handler(PARROT_INTERP, ARGIN(Parrot_exception *jb))
{
    push_exception(interp, new_c_exception_handler(interp, jb));
}

/*

=item C<opcode_t * throw_exception>

Throw the exception.

=cut

*/

PARROT_API
PARROT_CAN_RETURN_NULL
opcode_t *
throw_exception(PARROT_INTERP, ARGIN(PMC *exception), SHIM(void *dest))
{
    opcode_t *address;
    PMC * const handler = find_exception_handler(interp, exception);

    if (!handler)
        return NULL;
    /* put the handler aka continuation ctx in the interpreter */
    address = VTABLE_invoke(interp, handler, exception);
    /* address = VTABLE_get_pointer(interp, handler); */
    if (PObj_get_FLAGS(handler) & SUB_FLAG_C_HANDLER) {
        /* its a C exception handler */
        Parrot_exception * const jb = (Parrot_exception *) address;
        longjmp(jb->destination, 1);
    }
    /* return the address of the handler */
    return address;
}

/*

=item C<opcode_t * rethrow_exception>

Rethrow the exception.

=cut

*/

PARROT_API
PARROT_WARN_UNUSED_RESULT
PARROT_CAN_RETURN_NULL
opcode_t *
rethrow_exception(PARROT_INTERP, ARGIN(PMC *exception))
{
    PMC *handler;
    opcode_t *address;

    if (exception->vtable->base_type != enum_class_Exception)
        PANIC(interp, "Illegal rethrow");
    handler = find_exception_handler(interp, exception);
    address = VTABLE_invoke(interp, handler, exception);
    /* return the address of the handler */
    return address;
}

/*

=item C<void rethrow_c_exception>

Return back to runloop, assumes exception is still in todo (see RT#45915) and
that this is called from within a handler setup with C<new_c_exception>.

=cut

*/

PARROT_DOES_NOT_RETURN
void
rethrow_c_exception(PARROT_INTERP)
{
    Parrot_exception * const the_exception = interp->exceptions;

    PMC * const exception = PMCNULL;   /* RT#45915 */
    PMC * const handler   = find_exception_handler(interp, exception);

    /* RT#45911 we should only peek for the next handler */
    push_exception(interp, handler);
    /*
     * if there was no user handler, interpreter is already shutdown
     */
    the_exception->resume = VTABLE_get_pointer(interp, handler);
    the_exception->error = VTABLE_get_integer_keyed_int(interp,
            exception, 1);
    the_exception->severity = VTABLE_get_integer_keyed_int(interp,
            exception, 2);
    the_exception->msg = VTABLE_get_string_keyed_int(interp, exception, 0);
    longjmp(the_exception->destination, 1);
}

/*

=item C<static size_t dest2offset>

Translate an absolute bytecode location to an offset used for resuming
after an exception had occurred.

=cut

*/

PARROT_WARN_UNUSED_RESULT
static size_t
dest2offset(PARROT_INTERP, ARGIN(const opcode_t *dest))
{
    size_t offset;
    /* translate an absolute location in byte_code to an offset
     * used for resuming after an exception had occurred
     */
    switch (interp->run_core) {
        case PARROT_SWITCH_CORE:
        case PARROT_SWITCH_JIT_CORE:
        case PARROT_CGP_CORE:
        case PARROT_CGP_JIT_CORE:
            offset = dest - (const opcode_t *)interp->code->prederef.code;
        default:
            offset = dest - interp->code->base.data;
    }
    return offset;
}

/*

=item C<static opcode_t * create_exception>

Create an exception.

=cut

*/

PARROT_CAN_RETURN_NULL
PARROT_WARN_UNUSED_RESULT
static opcode_t *
create_exception(PARROT_INTERP)
{
    PMC *exception;     /* exception object */
    opcode_t *dest;     /* absolute address of handler */
    Parrot_exception * const the_exception = interp->exceptions;

    /*
     * if the exception number is in the range of our known exceptions
     * use the precreated exception
     */
    if (the_exception->error <= E_LAST_PYTHON_E &&
            the_exception->error >= 0) {
        exception = interp->exception_list[the_exception->error];
    }
    else {
        /* create an exception object */
        exception = pmc_new(interp, enum_class_Exception);
        /* exception type */
        VTABLE_set_integer_keyed_int(interp, exception, 1,
                the_exception->error);
    }
    /* exception severity */
    VTABLE_set_integer_keyed_int(interp, exception, 2,
            (INTVAL)the_exception->severity);
    if (the_exception->msg) {
        VTABLE_set_string_keyed_int(interp, exception, 0,
                the_exception->msg);
    }
    /* now fill rest of exception, locate handler and get
     * destination of handler
     */
    dest = throw_exception(interp, exception, the_exception->resume);
    return dest;
}

/*

=item C<size_t handle_exception>

Handle an exception.

=cut

*/

PARROT_API
size_t
handle_exception(PARROT_INTERP)
{
    /* absolute address of handler */
    const opcode_t * const dest = create_exception(interp);

    /* XXX We don't know that dest will be non-NULL, and it's not legal */
    /* to pass NULL to dest2offset. */

    return dest2offset(interp, dest);
}

/*

=item C<void new_internal_exception>

Create a new internal exception buffer, either by allocating it or by
getting one from the free list.

=cut

*/

PARROT_API
void
new_internal_exception(PARROT_INTERP)
{
    Parrot_exception *the_exception;

    if (interp->exc_free_list) {
        the_exception = interp->exc_free_list;
        interp->exc_free_list = the_exception->prev;
    }
    else
        the_exception = mem_allocate_typed(Parrot_exception);
    the_exception->prev = interp->exceptions;
    the_exception->resume = NULL;
    the_exception->msg = NULL;
    interp->exceptions = the_exception;
}

/*

=item C<void free_internal_exception>

Place internal exception buffer back on the free list.

=cut

*/

PARROT_API
void
free_internal_exception(PARROT_INTERP)
{
    Parrot_exception * const e = interp->exceptions;
    interp->exceptions = e->prev;
    e->prev = interp->exc_free_list;
    interp->exc_free_list = e;
}

/*

=item C<void destroy_exception_list>

RT#48260: Not yet documented!!!

=cut

*/

void
destroy_exception_list(PARROT_INTERP)
{
    really_destroy_exception_list(interp->exceptions);
    really_destroy_exception_list(interp->exc_free_list);
}

/*

=item C<void really_destroy_exception_list>

RT#48260: Not yet documented!!!

=cut

*/

void
really_destroy_exception_list(ARGIN(Parrot_exception *e))
{
    while (e != NULL) {
        Parrot_exception * const prev = e->prev;
        mem_sys_free(e);
        e    = prev;
    }
}

/*

=item C<void do_exception>

Called from interrupt code. Does a C<longjmp> in front of the runloop,
which calls C<handle_exception()>, returning the handler address where
execution then resumes.

=item C<void do_str_exception>

=item C<void do_pmc_exception>

*/

PARROT_API
PARROT_DOES_NOT_RETURN
void
do_str_exception(PARROT_INTERP, ARGIN(STRING *msg))
{
    Parrot_exception * const the_exception = interp->exceptions;

    the_exception->error = E_RuntimeError;
    the_exception->severity = EXCEPT_error;
    the_exception->msg = msg;
    the_exception->resume = NULL;
    longjmp(the_exception->destination, 1);
}

PARROT_API
PARROT_DOES_NOT_RETURN
void
do_pmc_exception(PARROT_INTERP, ARGIN(PMC *msg))
{
    Parrot_exception * const the_exception = interp->exceptions;

    the_exception->error = E_RuntimeError;
    the_exception->severity = EXCEPT_error;
    the_exception->msg = VTABLE_get_string(interp, msg);;
    the_exception->resume = NULL;
    longjmp(the_exception->destination, 1);
}

PARROT_API
PARROT_DOES_NOT_RETURN
void
do_exception(PARROT_INTERP, INTVAL severity, long error)
{
    Parrot_exception * const the_exception = interp->exceptions;

    the_exception->error = error;
    the_exception->severity = severity;
    the_exception->msg = NULL;
    the_exception->resume = NULL;
    longjmp(the_exception->destination, 1);
}

/*

=item C<void real_exception>

Throws a real exception, with an error message constructed from the format
string and arguments.  C<ret_addr> is the address from which to resume, if some
handler decides that is appropriate, or zero to make the error non-resumable.
C<exitcode> is a C<exception_type_enum> value.

See also C<internal_exception()>, which signals fatal errors, and
C<throw_exception>, which calls the handler.

=cut

*/

PARROT_API
PARROT_DOES_NOT_RETURN
void
real_exception(PARROT_INTERP, ARGIN_NULLOK(void *ret_addr),
        int exitcode, ARGIN(const char *format), ...)
{
    STRING *msg;
    Parrot_exception * const the_exception = interp->exceptions;

    /*
     * if profiling remember end time of lastop and
     * generate entry for exception
     */
    RunProfile * const profile = interp->profile;
    if (profile && Interp_flags_TEST(interp, PARROT_PROFILE_FLAG)) {
        const FLOATVAL now = Parrot_floatval_time();

        profile->data[profile->cur_op].time += now - profile->starttime;
        profile->cur_op = PARROT_PROF_EXCEPTION;
        profile->starttime = now;
        profile->data[PARROT_PROF_EXCEPTION].numcalls++;
    }

    /*
     * make exception message
     */
    if (strchr(format, '%')) {
        va_list arglist;
        va_start(arglist, format);
        msg = Parrot_vsprintf_c(interp, format, arglist);
        va_end(arglist);
    }
    else
        msg = string_make(interp, format, strlen(format),
                NULL, PObj_external_FLAG);
    /* string_from_cstring(interp, format, strlen(format)); */
    /*
     * FIXME classify errors
     */
    if (!the_exception) {
        PIO_eprintf(interp,
                "real_exception (severity:%d error:%d): %Ss\n"
                "likely reason: argument count mismatch in main "
                "(more than 1 param)\n",
                EXCEPT_error, exitcode, msg);
        /* [what if exitcode is a multiple of 256?] */
        exit(exitcode);
    }
    the_exception->severity = EXCEPT_error;
    the_exception->error = exitcode;
    the_exception->msg = msg;
    the_exception->resume = ret_addr;
    if (Interp_debug_TEST(interp, PARROT_BACKTRACE_DEBUG_FLAG)) {
        PIO_eprintf(interp, "real_exception (severity:%d error:%d): %Ss\n",
            EXCEPT_error, exitcode, msg);
        PDB_backtrace(interp);
    }
    /*
     * reenter runloop
     */
    longjmp(the_exception->destination, 1);
}

/*

=item C<void Parrot_init_exceptions>

Create exception objects.

=cut

*/

void
Parrot_init_exceptions(PARROT_INTERP)
{
    int i;

    interp->exception_list = (PMC **)mem_sys_allocate(
            sizeof (PMC*) * (E_LAST_PYTHON_E + 1));
    for (i = 0; i <= E_LAST_PYTHON_E; ++i) {
        PMC * const ex = pmc_new(interp, enum_class_Exception);
        interp->exception_list[i] = ex;
        VTABLE_set_integer_keyed_int(interp, ex, 1, i);
    }
}



/*

=item C<void Parrot_confess>

A better version of assert() that gives a backtrace if possible.

=cut

*/

PARROT_API
PARROT_DOES_NOT_RETURN
void
Parrot_confess(ARGIN(const char *cond), ARGIN(const char *file), unsigned int line)
{
    fprintf(stderr, "%s:%u: failed assertion '%s'\n", file, line, cond);
    Parrot_print_backtrace();
    abort();
}

/*

=item C<void Parrot_print_backtrace>

RT#48260: Not yet documented!!!

=cut

*/

void
Parrot_print_backtrace(void)
{
#ifdef PARROT_HAS_GLIBC_BACKTRACE
#  define BACKTRACE_DEPTH 32
/*#  define BACKTRACE_VERBOSE */
    /* stolen from http://www.delorie.com/gnu/docs/glibc/libc_665.html */
    void *array[BACKTRACE_DEPTH];
    size_t i;
#  ifndef BACKTRACE_VERBOSE
    int ident;
    char *caller;
    size_t callerLength;
    size_t j;
#  endif

    const size_t size = backtrace(array, BACKTRACE_DEPTH);
    char ** const strings = backtrace_symbols(array, size);

    fprintf(stderr,
            "Backtrace - Obtained %zd stack frames (max trace depth is %d).\n",
            size, BACKTRACE_DEPTH);
#  ifndef BACKTRACE_VERBOSE
    for (i = 0; i < size; i++) {
        /* always ident */
        ident = 2;  /* initial indent */
        ident += 2 * i; /* nesting depth */
        fprintf(stderr, "%*s", ident, "");

        /* if the caller was an anon function then strchr won't
        find a '(' in the string and will return NULL */
        caller = strchr(strings[i], '(');
        if (caller) {
            /* skip over the '(' */
            caller++;
            /* find the end of the symbol name */
            callerLength = abs(strchr(caller, '+') - caller);
            /* print just the symbol name */
            for (j = 0; j < callerLength; j++) {
                fputc(caller[j], stderr);
            }
            fprintf(stderr, "\n");
        }
        else {
            fprintf(stderr, "(unknown)\n");
        }
    }
#  else
    for (i = 0; i < size; i++)
        fprintf(stderr, "%s\n", strings[i]);
#  endif

    free(strings);

#  undef BACKTRACE_DEPTH
#endif /* ifdef PARROT_HAS_GLIBC_BACKTRACE */
}


/*

=back

=head1 SEE ALSO

F<include/parrot/exceptions.h>.

=cut

*/


/*
 * Local variables:
 *   c-file-style: "parrot"
 * End:
 * vim: expandtab shiftwidth=4:
 */
