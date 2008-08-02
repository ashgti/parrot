/* scheduler.h
 *  Copyright (C) 2007, The Perl Foundation.
 *  SVN Info
 *     $Id$
 *  Overview:
 *     Parrot concurrency scheduler header stuff
 *  Data Structure and Algorithms:
 *  History:
 *  Notes:
 *  References:
 */

#ifndef PARROT_SCHEDULER_H_GUARD
#define PARROT_SCHEDULER_H_GUARD

#include "parrot/parrot.h"

/* HEADERIZER BEGIN: src/scheduler.c */
/* Don't modify between HEADERIZER BEGIN / HEADERIZER END.  Your changes will be lost. */

PARROT_API
void Parrot_cx_add_handler(PARROT_INTERP, ARGIN(PMC *handler))
        __attribute__nonnull__(1)
        __attribute__nonnull__(2);

PARROT_API
void Parrot_cx_add_handler_local(PARROT_INTERP, ARGIN(PMC *handler))
        __attribute__nonnull__(1)
        __attribute__nonnull__(2);

PARROT_API
void Parrot_cx_broadcast_message(PARROT_INTERP,
    ARGIN(STRING *messagetype),
    ARGIN_NULLOK(PMC *data))
        __attribute__nonnull__(1)
        __attribute__nonnull__(2);

PARROT_API
INTVAL Parrot_cx_count_handlers_typed(PARROT_INTERP,
    ARGIN(STRING *handler_type))
        __attribute__nonnull__(1)
        __attribute__nonnull__(2);

PARROT_API
void Parrot_cx_delete_handler_local(PARROT_INTERP,
    ARGIN(STRING *handler_type))
        __attribute__nonnull__(1)
        __attribute__nonnull__(2);

PARROT_API
void Parrot_cx_delete_handler_typed(PARROT_INTERP,
    ARGIN(STRING *handler_type))
        __attribute__nonnull__(1)
        __attribute__nonnull__(2);

PARROT_API
PARROT_CAN_RETURN_NULL
PMC * Parrot_cx_delete_suspend_for_gc(PARROT_INTERP)
        __attribute__nonnull__(1);

PARROT_API
void Parrot_cx_delete_task(PARROT_INTERP, ARGIN(PMC *task))
        __attribute__nonnull__(1)
        __attribute__nonnull__(2);

PARROT_API
PARROT_CAN_RETURN_NULL
PMC * Parrot_cx_find_handler_for_task(PARROT_INTERP, ARGIN(PMC *task))
        __attribute__nonnull__(1)
        __attribute__nonnull__(2);

PARROT_API
PARROT_CAN_RETURN_NULL
PMC * Parrot_cx_find_handler_local(PARROT_INTERP, ARGIN(PMC *task))
        __attribute__nonnull__(1)
        __attribute__nonnull__(2);

PARROT_API
void Parrot_cx_handle_tasks(PARROT_INTERP, ARGMOD(PMC *scheduler))
        __attribute__nonnull__(1)
        __attribute__nonnull__(2)
        FUNC_MODIFIES(*scheduler);

PARROT_API
PARROT_CAN_RETURN_NULL
PMC * Parrot_cx_peek_task(PARROT_INTERP)
        __attribute__nonnull__(1);

PARROT_API
void Parrot_cx_request_suspend_for_gc(PARROT_INTERP)
        __attribute__nonnull__(1);

PARROT_API
void Parrot_cx_runloop_end(PARROT_INTERP)
        __attribute__nonnull__(1);

PARROT_API
void Parrot_cx_schedule_callback(PARROT_INTERP,
    ARGIN(PMC *user_data),
    ARGIN(char *ext_data))
        __attribute__nonnull__(1)
        __attribute__nonnull__(2)
        __attribute__nonnull__(3);

PARROT_API
void Parrot_cx_schedule_repeat(PARROT_INTERP, ARGIN(PMC *task))
        __attribute__nonnull__(1)
        __attribute__nonnull__(2);

PARROT_API
PARROT_WARN_UNUSED_RESULT
PARROT_CAN_RETURN_NULL
opcode_t * Parrot_cx_schedule_sleep(PARROT_INTERP,
    FLOATVAL time,
    ARGIN_NULLOK(opcode_t *next))
        __attribute__nonnull__(1);

PARROT_API
void Parrot_cx_schedule_task(PARROT_INTERP, ARGIN(PMC *task))
        __attribute__nonnull__(1)
        __attribute__nonnull__(2);

PARROT_API
void Parrot_cx_schedule_timer(PARROT_INTERP,
    ARGIN_NULLOK(STRING *type),
    FLOATVAL duration,
    FLOATVAL interval,
    INTVAL repeat,
    ARGIN_NULLOK(PMC *sub))
        __attribute__nonnull__(1);

PARROT_API
void Parrot_cx_send_message(PARROT_INTERP,
    ARGIN(STRING *messagetype),
    ARGIN_NULLOK(PMC *payload))
        __attribute__nonnull__(1)
        __attribute__nonnull__(2);

void Parrot_cx_check_tasks(PARROT_INTERP, ARGMOD(PMC *scheduler))
        __attribute__nonnull__(1)
        __attribute__nonnull__(2)
        FUNC_MODIFIES(*scheduler);

void Parrot_cx_init_scheduler(PARROT_INTERP)
        __attribute__nonnull__(1);

void Parrot_cx_invoke_callback(PARROT_INTERP, ARGIN(PMC *callback))
        __attribute__nonnull__(1)
        __attribute__nonnull__(2);

void Parrot_cx_refresh_task_list(PARROT_INTERP, ARGMOD(PMC *scheduler))
        __attribute__nonnull__(1)
        __attribute__nonnull__(2)
        FUNC_MODIFIES(*scheduler);

void Parrot_cx_runloop_wake(PARROT_INTERP, ARGMOD(PMC *scheduler))
        __attribute__nonnull__(1)
        __attribute__nonnull__(2)
        FUNC_MODIFIES(*scheduler);

void Parrot_cx_timer_invoke(PARROT_INTERP, ARGIN(PMC *timer))
        __attribute__nonnull__(1)
        __attribute__nonnull__(2);

/* Don't modify between HEADERIZER BEGIN / HEADERIZER END.  Your changes will be lost. */
/* HEADERIZER END: src/scheduler.c */

/* Timer PMC interface constants */
/* &gen_from_enum(timer.pasm) */
typedef enum {
    PARROT_TIMER_SEC,
    PARROT_TIMER_USEC,
    PARROT_TIMER_NSEC,
    PARROT_TIMER_REPEAT,
    PARROT_TIMER_INTERVAL,
    PARROT_TIMER_RUNNING,
    PARROT_TIMER_HANDLER,
    PARROT_TIMER_MAX
} parrot_timer_enum_t;
/* &end_gen */


#endif /* PARROT_SCHEDULER_H_GUARD */

/*
 * Local variables:
 *   c-file-style: "parrot"
 * End:
 * vim: expandtab shiftwidth=4:
 */
