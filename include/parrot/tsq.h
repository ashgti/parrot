/* tsq.h
 *  Copyright (C) 2001-2007, The Perl Foundation.
 *  SVN Info
 *     $Id$
 *  Overview:
 *     Defines the thread-safe queue system
 *  Data Structure and Algorithms:
 *  History:
 *  Notes:
 *  References:
 */

#ifndef PARROT_TSQ_H_GUARD
#define PARROT_TSQ_H_GUARD

#include "parrot/config.h"
#include "parrot/thread.h"


typedef struct QUEUE_ENTRY QUEUE_ENTRY;
typedef struct QUEUE QUEUE;

typedef enum {
    QUEUE_ENTRY_TYPE_NONE,
    QUEUE_ENTRY_TYPE_EVENT,
    QUEUE_ENTRY_TYPE_TIMED_EVENT
} queue_entry_type_enum;

struct QUEUE_ENTRY {
    void *data;
    queue_entry_type_enum type;
    QUEUE_ENTRY *next;
};

struct QUEUE {
    QUEUE_ENTRY *head;
    QUEUE_ENTRY *tail;
    UINTVAL max_prio;
    Parrot_mutex queue_mutex;
    Parrot_cond queue_condition;
};

/* HEADERIZER BEGIN: src/tsq.c */

void insert_entry(ARGINOUT(QUEUE *queue), ARGIN(QUEUE_ENTRY *entry))
        __attribute__nonnull__(1)
        __attribute__nonnull__(2);

void nosync_insert_entry(ARGINOUT(QUEUE *queue), ARGIN(QUEUE_ENTRY *entry))
        __attribute__nonnull__(1)
        __attribute__nonnull__(2);

PARROT_CAN_RETURN_NULL
QUEUE_ENTRY * nosync_pop_entry(ARGINOUT(QUEUE *queue))
        __attribute__nonnull__(1);

PARROT_CAN_RETURN_NULL
PARROT_WARN_UNUSED_RESULT
QUEUE_ENTRY * peek_entry(ARGINOUT(QUEUE *queue))
        __attribute__nonnull__(1);

PARROT_CAN_RETURN_NULL
QUEUE_ENTRY * pop_entry(ARGINOUT(QUEUE *queue))
        __attribute__nonnull__(1);

void push_entry(ARGINOUT(QUEUE *queue), ARGIN(QUEUE_ENTRY *entry))
        __attribute__nonnull__(1)
        __attribute__nonnull__(2);

void queue_broadcast(ARGINOUT(QUEUE *queue))
        __attribute__nonnull__(1);

void queue_destroy(ARGINOUT(QUEUE *queue))
        __attribute__nonnull__(1);

PARROT_CAN_RETURN_NULL
PARROT_MALLOC
QUEUE* queue_init(UINTVAL prio);

void queue_lock(ARGINOUT(QUEUE *queue))
        __attribute__nonnull__(1);

void queue_signal(ARGINOUT(QUEUE *queue))
        __attribute__nonnull__(1);

void queue_timedwait(
    ARGINOUT(QUEUE *queue),
    ARGIN(const struct timespec *abs_time))
        __attribute__nonnull__(1)
        __attribute__nonnull__(2);

void queue_unlock(ARGINOUT(QUEUE *queue))
        __attribute__nonnull__(1);

void queue_wait(ARGINOUT(QUEUE *queue))
        __attribute__nonnull__(1);

void unshift_entry(ARGINOUT(QUEUE *queue), ARGIN(QUEUE_ENTRY *entry))
        __attribute__nonnull__(1)
        __attribute__nonnull__(2);

PARROT_CAN_RETURN_NULL
QUEUE_ENTRY * wait_for_entry(ARGINOUT(QUEUE *queue))
        __attribute__nonnull__(1);

/* HEADERIZER END: src/tsq.c */

#endif /* PARROT_TSQ_H_GUARD */

/*
 * Local variables:
 *   c-file-style: "parrot"
 * End:
 * vim: expandtab shiftwidth=4:
 */
