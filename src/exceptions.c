/* exceptions.c
 *  Copyright: 2001-2003 The Perl Foundation.  All Rights Reserved.
 *  CVS Info
 *     $Id$
 *  Overview:
 *     define the internal interpreter exceptions
 *  Data Structure and Algorithms:
 *  History:
 *  Notes:
 *     This is experimental code.
 *     The enum_class of the Exception isn't fixed.
 *     The interface isn't fixed.
 *     Much of this may change in the futore.
 *  References:
 */

#include "parrot/parrot.h"
#include "parrot/exceptions.h"

#include <stdarg.h>

/* Exception Handler */
void
internal_exception(int exitcode, const char *format, ...)
{
    va_list arglist;
    va_start(arglist, format);
    vfprintf(stderr, format, arglist);
    va_end(arglist);
    Parrot_exit(exitcode);
}

/* Panic handler */

#ifndef dumpcore
#define dumpcore() \
  fprintf(stderr, "Sorry, coredump is not yet implemented " \
          "for this platform.\n\n"); \
          exit(1);
#endif

void
do_panic(struct Parrot_Interp *interpreter, const char *message,
         const char *file, int line)
{
    /* Note: we can't format any floats in here--Parrot_sprintf
    ** may panic because of floats.
    ** and we don't use Parrot_sprintf or such, because we are
    ** already in panic --leo
    */
    fprintf(stderr, "Parrot VM: PANIC: %s!\n",
               message ? message : "(no message available)");

    fprintf(stderr, "C file %s, line %d\n",
               file ? file : "(not available)", line);

    if (interpreter) {
        fprintf(stderr, "Parrot file %s, line %d\n",
                   interpreter->current_file &&
                   interpreter->current_file->strstart ?
                   (char *)interpreter->current_file->strstart : "(null)",
                   (int)interpreter->current_line);
    }
    else {
        fprintf(stderr, "Parrot file (not available), ");
        fprintf(stderr, "line (not available)\n");
    }

    fprintf(stderr, "\n\
We highly suggest you notify the Parrot team if you have not been working on\n\
Parrot.  Use bugs6.perl.org or send an e-mail to perl6-internals@perl.org.\n\
Include the entire text of this error message and the text of the script that\n\
generated the error.  If you've made any modifications to Parrot, please\n\
describe them as well.\n\n");

    fprintf(stderr, "Version     : %s\n", PARROT_VERSION);
    fprintf(stderr, "Configured  : %s\n", PARROT_CONFIG_DATE);
    fprintf(stderr, "Architecture: %s\n", PARROT_ARCHNAME);
    fprintf(stderr, "JIT Capable : %s\n", JIT_CAPABLE ? "Yes" : "No");
    if (interpreter)
        fprintf(stderr, "Interp Flags: %#x\n", interpreter->flags);
    else
        fprintf(stderr, "Interp Flags: (no interpreter)\n");
    fprintf(stderr, "Exceptions  : %s\n", "(missing from core)");
    fprintf(stderr, "\nDumping Core...\n");

    dumpcore();
}

void
push_exception(Parrot_Interp interpreter, PMC *handler)
{
    if (handler->vtable->base_type != enum_class_Exception_Handler)
        PANIC("Tried to set_eh a non Exception_Handler");
    stack_push(interpreter, &interpreter->ctx.control_stack, handler,
            STACK_ENTRY_PMC, STACK_CLEANUP_NULL);
}

static PMC *
find_exception_handler(Parrot_Interp interpreter, PMC *exception)
{
    PMC *handler;
    STRING *message;
    char *m;
    int exit_status, print_location;
    /* for now, we don't check the exception class and we don't
     * look for matching handlers
     */
    message = VTABLE_get_string_keyed_int(interpreter, exception, 0);
    do {
        Stack_Entry_t *e = stack_entry(interpreter,
                interpreter->ctx.control_stack, 0);
        if (!e)
            break;
        (void)stack_pop(interpreter, &interpreter->ctx.control_stack,
                        NULL, e->entry_type);
        if (e->entry_type == STACK_ENTRY_PMC) {
            handler = e->entry.pmc_val;
            if (handler &&
                    handler->vtable->base_type == enum_class_Exception_Handler) {
                return handler;
            }
        }
    } while (1);
    m = string_to_cstring(interpreter, message);
    exit_status = print_location = 1;
    if (m && *m) {
        fputs(m, stderr);
        if (m[strlen(m)-1] != '\n')
            fprintf(stderr, "%c", '\n');
    }
    else {
        INTVAL severity =
            VTABLE_get_integer_keyed_int(interpreter, exception, 2);
        if (severity == EXCEPT_exit) {
            print_location = 0;
            exit_status =
                (int)VTABLE_get_integer_keyed_int(interpreter, exception, 1);
        }
        else
            fprintf(stderr, "No exception handler and no message\n");
    }
    if (print_location)
        print_pbc_location(interpreter);
    Parrot_exit(exit_status);

    return NULL;
}

void
pop_exception(Parrot_Interp interpreter)
{
    Stack_entry_type type;
    PMC *handler;

    handler = stack_peek(interpreter, interpreter->ctx.control_stack, &type);
    if (type != STACK_ENTRY_PMC ||
            handler->vtable->base_type != enum_class_Exception_Handler)
        return; /* no exception on TOS */
    (void)stack_pop(interpreter, &interpreter->ctx.control_stack, NULL,
                    STACK_ENTRY_PMC);
}

/*
 * generate an exception handler, that catches PASM level exceptions
 * inside a C function
 * This could be a separate class too, for now just a private flag
 * bit is set
 */
PMC*
new_c_exception_handler(Parrot_Interp interpreter, Parrot_exception *jb)
{
    PMC *handler = pmc_new(interpreter, enum_class_Exception_Handler);
    PObj_get_FLAGS(handler) |= PObj_private0_FLAG;
    handler->cache.struct_val = jb;
    return handler;
}

void
push_new_c_exception_handler(Parrot_Interp interpreter, Parrot_exception *jb)
{
    push_exception(interpreter, new_c_exception_handler(interpreter, jb));
}

void *
throw_exception(Parrot_Interp interpreter, PMC *exception, void *dest)
{
    PMC *handler;
    struct Parrot_Sub * cc;

    handler = find_exception_handler(interpreter, exception);
    cc = (struct Parrot_Sub*)PMC_sub(handler);
    /* preserve P5 register */
    VTABLE_set_pmc_keyed_int(interpreter, exception, 3, REG_PMC(5));
#if 0
    /* remember handler */
    key = key_new_cstring(interpreter, "_handler");
    VTABLE_set_pmc_keyed(interpreter, exception, key, handler);
#endif
    /* generate and place return continuation */
    if (dest) {
        VTABLE_set_pmc_keyed_int(interpreter, exception, 4,
                new_ret_continuation_pmc(interpreter, dest));
    }
    /* put the continuation ctx in the interpreter */
    restore_context(interpreter, &cc->ctx);
    /* put exception object in P5 */
    REG_PMC(5) = exception;
    if (PObj_get_FLAGS(handler) & PObj_private0_FLAG) {
        /* its a C exception handler */
        Parrot_exception *jb = (Parrot_exception *) handler->cache.struct_val;
        longjmp(jb->destination, 1);
    }
    /* return the address of the handler */
    return handler->cache.struct_val;
}

void *
rethrow_exception(Parrot_Interp interpreter, PMC *exception)
{
    PMC *handler;
    struct Parrot_Sub * cc;

    if (exception->vtable->base_type != enum_class_Exception)
        PANIC("Illegal rethrow");
    handler = find_exception_handler(interpreter, exception);
    cc = (struct Parrot_Sub*)PMC_sub(handler);
    restore_context(interpreter, &cc->ctx);
    /* put exception object in P5 */
    REG_PMC(5) = exception;
    /* return the address of the handler */
    return handler->cache.struct_val;
}

Parrot_exception the_exception;

/*
 * return back to runloop, assumes exception is still in REG_PMC(5)
 * and that this is called from within a handler setup with
 * new_c_exception
 */
void
rethrow_c_exception(Parrot_Interp interpreter)
{
    PMC *exception, *handler, *p5;

    p5 = VTABLE_get_pmc_keyed_int(interpreter, REG_PMC(5), 3);
    exception = REG_PMC(5);
    REG_PMC(5) = p5;
    handler = find_exception_handler(interpreter, exception);
    /* XXX we should only peek for the next handler */
    push_exception(interpreter, handler);
    /*
     * if there was no user handler, interpreter is already shutdown
     */
    the_exception.resume = handler->cache.struct_val;
    the_exception.error = VTABLE_get_integer_keyed_int(interpreter,
            exception, 1);
    the_exception.severity = VTABLE_get_integer_keyed_int(interpreter,
            exception, 2);
    the_exception.msg = VTABLE_get_string_keyed_int(interpreter, exception, 0);
    longjmp(the_exception.destination, 1);
}

static size_t
dest2offset(Parrot_Interp interpreter, opcode_t *dest)
{
    size_t offset;
    /* translate an absolute location in byte_code to an offset
     * used for resuming after an exception had occured
     */
    switch (interpreter->run_core) {
        case PARROT_PREDEREF_CORE:
        case PARROT_SWITCH_CORE:
        case PARROT_CGP_CORE:
            offset = (void **)dest - interpreter->prederef.code;
        default:
            offset = dest - interpreter->code->byte_code;
    }
    return offset;
}

static opcode_t *
create_exception(Parrot_Interp interpreter)
{
    PMC *exception;     /* exception object */
    opcode_t *dest;     /* absolute address of handler */


    /* create an exception object */
    exception = pmc_new(interpreter, enum_class_Exception);
    /* exception type */
    VTABLE_set_integer_keyed_int(interpreter, exception, 1,
            the_exception.error);
    /* exception severity */
    VTABLE_set_integer_keyed_int(interpreter, exception, 2,
            (INTVAL)the_exception.severity);
    if (the_exception.msg) {
        VTABLE_set_string_keyed_int(interpreter, exception, 0,
                the_exception.msg);
    }
    /* now fill rest of exception, locate handler and get
     * destination of handler
     */
    dest = throw_exception(interpreter, exception, the_exception.resume);
    return dest;
}

size_t
handle_exception(Parrot_Interp interpreter)
{
    opcode_t *dest;     /* absolute address of handler */

    dest = create_exception(interpreter);
    return dest2offset(interpreter, dest);
}

/*
 * do_exception:
 * - called from interrupt code
 * - does a longjmp in front of the runloop, which calls
 * - handle_exception(): returning the handler address
 * - where execution then resumes
 */
void
do_exception(exception_severity severity, long error)
{
    the_exception.error = error;
    the_exception.severity = severity;
    the_exception.msg = NULL;
    the_exception.resume = NULL;
    longjmp(the_exception.destination, 1);
}

/*
 * instead of internal_exception this throws a real exception
 */
void
real_exception(struct Parrot_Interp *interpreter, void *ret_addr,
        int exitcode,  const char *format, ...)
{
    STRING *msg;


    /*
     * if profiling remember end time of lastop and
     * generate entry for exception
     */
    if (interpreter->profile &&
            Interp_flags_TEST(interpreter, PARROT_PROFILE_FLAG)) {
        RunProfile *profile = interpreter->profile;
        FLOATVAL now = Parrot_floatval_time();
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
        msg = Parrot_vsprintf_c(interpreter, format, arglist);
        va_end(arglist);
    }
    else
        msg = string_make(interpreter, format, strlen(format),
                NULL, PObj_external_FLAG, NULL);
    /*
     * FIXME classify errors
     */
    the_exception.error = exitcode;
    the_exception.severity = EXCEPT_error;
    the_exception.msg = msg;
    the_exception.resume = ret_addr;
    /*
     * reenter runloop
     */
    longjmp(the_exception.destination, 1);
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
