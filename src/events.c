/* events.c
 *  Copyright: 2001-2003 The Perl Foundation.  All Rights Reserved.
 *  CVS Info
 *     $Id$
 *  Overview:
 *     Event handling stuff
 *  Data Structure and Algorithms:
 *     An event_thread handles async events for all interpreters.
 *     When events are due, they are placed in per interpreter
 *     task_queues, where they are handled then by the check_event*
 *     opcodes.
 *  History:
 *  Notes:
 *  References:
 *     docs/dev/events.pod
 */

#include "parrot/parrot.h"
#include <assert.h>

/* forward def */
static void* event_thread(void *data);

/*
 * we have exactly one event_queue
 */
static QUEUE* event_queue;
#define TASK_PRIO 10

static void
sig_handler(int signum)
{
    switch (signum) {
#ifdef SIGFPE
        /* we can't handle SIGFPE - just give up, i.e. longjmp */
        case SIGFPE:
            do_exception(0, -signum);
            break;
#endif
        default:
            /*
             * TODO convert signal to event
             * which isn't simple: the only thread and signal safe
             * things we may do are:
             * - set a sigatomic_t global or
             * - sem_post a semaphore
             */

            break;
    }
}

void
Parrot_init_signals(void)
{
    /* quick hack to test signals and exceptions
     * s. t/op/hacks_1.pasm
     */
#ifdef SIGFPE
    Parrot_set_sighandler(SIGFPE, sig_handler);
#endif
#ifdef SIGINT
    /* Parrot_set_sighandler(SIGINT, sig_handler); */
#endif
}

/*
 * init event system for first interpreter
 */
static void
init_events_first(Parrot_Interp interpreter)
{
    Parrot_thread    the_thread;
    /*
     * init event queue - be sure its done only once
     * we could use pthread_once for queue_init
     */
    assert(!event_queue);
    event_queue = queue_init(TASK_PRIO);
    /*
     * we start an event_handler thread
     */
    THREAD_CREATE_DETACHED(the_thread, event_thread, event_queue);
    /*
     * now set some sig handlers
     */
    Parrot_init_signals();
}

/*
 * init events for all interpreters
 */
static void
init_events_all(Parrot_Interp interpreter)
{
    /*
     * create event queue
     */
    interpreter->task_queue = queue_init(0);
}

/*
 * initialize the event system
 */
void
Parrot_init_events(Parrot_Interp interpreter)
{
    if (!interpreter->parent_interpreter) {
        init_events_first(interpreter);
    }
    init_events_all(interpreter);
}

/*
 * create queue entry and insert event into task queue
 */
void
Parrot_schedule_event(Parrot_Interp interpreter, parrot_event* ev)
{
    QUEUE_ENTRY* entry = mem_sys_allocate(sizeof(QUEUE_ENTRY));
    entry->next = NULL;
    ev->interp = interpreter;
    entry->data = ev;
    switch (ev->type) {
        case EVENT_TYPE_TIMER:
            entry->type = QUEUE_ENTRY_TYPE_TIMED_EVENT;
            insert_entry(event_queue, entry);
            break;
        default:
            entry->type = QUEUE_ENTRY_TYPE_EVENT;
            push_entry(event_queue, entry);
            break;
    }
}

/*
 * create a new timer event due at diff from now, repeated at
 * interval running the passed sub
 */
void
Parrot_new_timer_event(Parrot_Interp interpreter, FLOATVAL diff,
        FLOATVAL interval, PMC* sub)
{
    parrot_event* ev = mem_sys_allocate(sizeof(parrot_event));
    FLOATVAL now = Parrot_floatval_time();
    ev->type = EVENT_TYPE_TIMER;
    ev->data = NULL;
    ev->u.timer_event.abs_time = now + diff;
    ev->u.timer_event.interval = interval;
    ev->u.timer_event.sub = sub;
    Parrot_schedule_event(interpreter, ev);
}

void
Parrot_schedule_interp_qentry(Parrot_Interp interpreter, QUEUE_ENTRY* entry)
{
    push_entry(interpreter->task_queue, entry);
    enable_event_checking(interpreter);
}

/*
 * duplicate timed entry and add interval to abstime
 */
static QUEUE_ENTRY*
dup_entry_interval(QUEUE_ENTRY* entry, FLOATVAL now)
{
    parrot_event *event;
    QUEUE_ENTRY *new_entry;

    new_entry = mem_sys_allocate(sizeof(QUEUE_ENTRY));
    new_entry->next = NULL;
    new_entry->type = entry->type;
    new_entry->data = mem_sys_allocate(sizeof(parrot_event));
    mem_sys_memcopy(new_entry->data, entry->data, sizeof(parrot_event));
    event = new_entry->data;
    event->u.timer_event.abs_time = now + event->u.timer_event.interval;
    return new_entry;
}
/*
 * The event_thread is started by the first interpreter.
 * It handles all events for all interpreters.
 */
static void*
event_thread(void *data)
{
    QUEUE* event_q = (QUEUE*) data;
    parrot_event* event;
    QUEUE_ENTRY *entry;

    LOCK(event_q->queue_mutex);
    while (1) {
        entry = peek_entry(event_q);
        if (!entry) {
            /* wait infinite until entry arrives */
            queue_wait(event_q);
        }
        else if (entry->type == QUEUE_ENTRY_TYPE_TIMED_EVENT) {
            /* do a_timedwait for entry */
            struct timespec abs_time;
            FLOATVAL when;
            event = (parrot_event* )entry->data;
            when = event->u.timer_event.abs_time;
            abs_time.tv_sec = (time_t) when;
            abs_time.tv_nsec = (when - abs_time.tv_sec) * (1000L*1000L*1000L);

            queue_timedwait(event_q, &abs_time);
        }
        else {
            /* we shouldn't get here probably */
            internal_exception(1, "Spurious event is event queue");
        }
        /*
         * one or more entries arrived - we hold the mutex again
         * so we have to use the nonsyc_pop_entry to pop off event entries
         */
        while (( entry = peek_entry(event_q))) {
            FLOATVAL now;

            event = NULL;
            switch (entry->type) {
                case QUEUE_ENTRY_TYPE_EVENT:
                    entry = nosync_pop_entry(event_q);
                    event = entry->data;
                    break;
                case QUEUE_ENTRY_TYPE_TIMED_EVENT:
                    event = entry->data;
                    now = Parrot_floatval_time();
                    /*
                     * if the timer_event isn't due yet, ignore the event
                     * (we were signalled on insert of the event)
                     * wait until we get at it again when time has elapsed
                     */
                    if (now < event->u.timer_event.abs_time)
                        goto again;
                    entry = nosync_pop_entry(event_q);
                    /*
                     * if event is repeated dup and reinsert it
                     */
                    if (event->u.timer_event.interval) {
                        nosync_insert_entry(event_q,
                                dup_entry_interval(entry, now));
                    }
                    break;
                default:
                    internal_exception(1, "Unknown queue entry");
            }
            /*
             * TODO check for a stop event to do cleanup
             */
            /*
             * now insert entry in interpreter task queue
             */
            assert(event);
            if (event->interp) {
                Parrot_schedule_interp_qentry(event->interp, entry);
            }
            else {
                /*
                 * TODO broadcast or deliver to first interp
                 */
            }
        } /* while events */
again:

    } /* forever */
    /*
     * not reached yet
     */
    UNLOCK(event_q->queue_mutex);
}

/*
 * explicitely sync called by the check_event opcode from run loops
 */
void
Parrot_do_check_events(Parrot_Interp interpreter)
{
    if (peek_entry(interpreter->task_queue))
        Parrot_do_handle_events(interpreter, 0);
}

/*
 * called by the check_event__ opcode from run loops or from above
 * When called from the check_events__ opcode, we have to restore
 * the op_func_table
 */
void
Parrot_do_handle_events(Parrot_Interp interpreter, int restore)
{
    QUEUE_ENTRY *entry;
    parrot_event* event;

    if (restore)
        disable_event_checking(interpreter);
    if (!peek_entry(interpreter->task_queue))
        return;
    while (peek_entry(interpreter->task_queue)) {
        entry = pop_entry(interpreter->task_queue);
        event = (parrot_event* )entry->data;
        mem_sys_free(entry);

        switch (event->type) {
            case EVENT_TYPE_TIMER:
                /* run ops, save registers */
                Parrot_runops_fromc_save(interpreter, event->u.timer_event.sub);
                break;
            default:
                fprintf(stderr, "Unhandled event type %d\n", event->type);
                break;
        }
        mem_sys_free(event);
    }
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
