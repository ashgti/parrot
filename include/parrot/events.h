/* events.h
 *  Copyright: 2001-2003 The Perl Foundation.  All Rights Reserved.
 *  CVS Info
 *     $Id$
 *  Overview:
 *     This api will handle parrot events
 *  Data Structure and Algorithms:
 *  History:
 *  Notes:
 *  References:
 */

#if !defined(PARROT_EVENT_H_GUARD)
#define PARROT_EVENT_H_GUARD

typedef void* (*event_func_t)(Parrot_Interp, void*);

typedef enum {
    EVENT_TYPE_NONE,
    EVENT_TYPE_EVENT,
    EVENT_TYPE_IO,
    EVENT_TYPE_MSG,
    EVENT_TYPE_ASYNC_IO,
    EVENT_TYPE_TIMER,
    EVENT_TYPE_CLASS_CHANGED,
    EVENT_TYPE_SIGNAL
} parrot_event_type_enum;

/*
 * any timer event has 2 time fields in front
 */
typedef struct {
    FLOATVAL                    abs_time;
    FLOATVAL                    interval;
    PMC*                        sub;    /* handler sub */
} parrot_timer_event;

typedef struct {
    parrot_event_type_enum      type;
    Parrot_Interp               interp;
    event_func_t                event_func;
    void*                       data;
    union {
        STRING*                 msg;            /* for testing only */
        int                     signal;         /* for EVENT_TYPE_SIGNAL */
        parrot_timer_event      timer_event;    /* for EVENT_TYPE_TIMER */
    } u;
} parrot_event;

struct QUEUE_ENTRY;
void Parrot_schedule_event(Parrot_Interp, parrot_event*);
void Parrot_schedule_interp_event(Parrot_Interp, parrot_event*);
void Parrot_schedule_interp_qentry(Parrot_Interp, struct QUEUE_ENTRY* entry);

#define CHECK_EVENTS(i)  Parrot_do_check_events(i)
#define HANDLE_EVENTS(i) Parrot_do_handle_events(i, 1)

void Parrot_init_signals(void);
void Parrot_init_events(Parrot_Interp);
void Parrot_do_check_events(Parrot_Interp);
void Parrot_do_handle_events(Parrot_Interp, int);

void Parrot_new_timer_event(Parrot_Interp, FLOATVAL, FLOATVAL,PMC*);
void disable_event_checking(Parrot_Interp);
void enable_event_checking(Parrot_Interp);
#endif

/*
 * Local variables:
 * c-indentation-style: bsd
 * c-basic-offset: 4
 * indent-tabs-mode: nil
 * End:
 *
 * vim: expandtab shiftwidth=4:
*/
