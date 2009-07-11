/*
Copyright (C) 2001-2009, Parrot Foundation.
$Id$

=head1 NAME

src/tsq.c - Thread-safe queues

=head1 DESCRIPTION

This file implements thread-safe queues for Parrot.

=head2 Functions

=over 4

=cut

*/

#include "parrot/parrot.h"

/* HEADERIZER HFILE: include/parrot/tsq.h */

/*

=item C<QUEUE_ENTRY * pop_entry(QUEUE *queue)>

Does a synchronized removal of the head entry off the queue and returns it.

=cut

*/

PARROT_CAN_RETURN_NULL
QUEUE_ENTRY *
pop_entry(ARGMOD(QUEUE *queue))
{
    ASSERT_ARGS(pop_entry)
    QUEUE_ENTRY *returnval;
    queue_lock(queue);
    returnval = nosync_pop_entry(queue);
    queue_unlock(queue);
    return returnval;
}

/*

=item C<QUEUE_ENTRY * peek_entry(const QUEUE *queue)>

This does no locking, so the result might have changed by the time you
get the entry, but a synchronized C<pop_entry()> will check again and
return C<NULL> if the queue is empty.

=cut

*/

PARROT_CAN_RETURN_NULL
PARROT_WARN_UNUSED_RESULT
QUEUE_ENTRY *
peek_entry(ARGIN(const QUEUE *queue))
{
    ASSERT_ARGS(peek_entry)
    return queue->head;
}

/*

=item C<QUEUE_ENTRY * nosync_pop_entry(QUEUE *queue)>

Grab an entry off the queue with no synchronization. Internal only,
because it's darned evil and shouldn't be used outside the module. It's
in here so we don't have to duplicate pop code.

=cut

*/

PARROT_CANNOT_RETURN_NULL
QUEUE_ENTRY *
nosync_pop_entry(ARGMOD(QUEUE *queue))
{
    ASSERT_ARGS(nosync_pop_entry)
    QUEUE_ENTRY *returnval;
    if (!queue->head) {
        return NULL;
    }
    returnval = queue->head;
    if (queue->head == queue->tail) {
        queue->head = NULL;
        queue->tail = NULL;
    }
    else {
        queue->head = queue->head->next;
    }
    returnval->next = NULL;
    return returnval;
}

/*

=item C<QUEUE_ENTRY * wait_for_entry(QUEUE *queue)>

Does a synchronized removal of the head entry off the queue, waiting if
necessary until there is an entry, and then returns it.

=cut

*/

PARROT_CAN_RETURN_NULL
QUEUE_ENTRY *
wait_for_entry(ARGMOD(QUEUE *queue))
{
    ASSERT_ARGS(wait_for_entry)
    QUEUE_ENTRY *returnval;

    queue_lock(queue);
    while (queue->head == NULL) {
        queue_wait(queue);
    }
    returnval = nosync_pop_entry(queue);
    queue_unlock(queue);
    return returnval;

}

/*

=item C<void push_entry(QUEUE *queue, QUEUE_ENTRY *entry)>

Does a synchronized insertion of C<entry> onto the tail of the queue.

=cut

*/

void
push_entry(ARGMOD(QUEUE *queue), ARGIN(QUEUE_ENTRY *entry))
{
    ASSERT_ARGS(push_entry)
    queue_lock(queue);
    /* Is there something in the queue? */
    if (queue->tail) {
        queue->tail->next = entry;
        queue->tail = entry;
    }
    else {
        queue->head = entry;
        queue->tail = entry;
    }
    queue_signal(queue);        /* assumes only one waiter */
    queue_unlock(queue);
}

/*

=item C<void unshift_entry(QUEUE *queue, QUEUE_ENTRY *entry)>

Does a synchronized insertion of C<entry> into the head of the queue.

=cut

*/

void
unshift_entry(ARGMOD(QUEUE *queue), ARGIN(QUEUE_ENTRY *entry))
{
    ASSERT_ARGS(unshift_entry)
    QUEUE_ENTRY *cur;

    queue_lock(queue);
    cur = queue->head;
    if (!cur) {
        /* empty just set head */
        queue->head = entry;
        queue->tail = entry;
    }
    else {
        queue->head = entry;
        entry->next = cur;
    }
    queue_signal(queue);
    queue_unlock(queue);
}

/*

=item C<void nosync_insert_entry(QUEUE *queue, QUEUE_ENTRY *entry)>

Inserts a timed event according to C<abstime>. The caller has to hold the
queue mutex.

=cut

*/

void
nosync_insert_entry(ARGMOD(QUEUE *queue), ARGIN(QUEUE_ENTRY *entry))
{
    ASSERT_ARGS(nosync_insert_entry)
    QUEUE_ENTRY *cur = queue->head;
    QUEUE_ENTRY *prev;
    parrot_event *event;
    FLOATVAL abs_time;

    PARROT_ASSERT(entry->type == QUEUE_ENTRY_TYPE_TIMED_EVENT);
    /*
     * empty queue - just insert
     */
    if (!cur) {
        queue->head = entry;
        queue->tail = entry;
        return;
    }

    prev     = NULL;
    event    = (parrot_event *)entry->data;
    abs_time = event->u.timer_event.abs_time;

    while (cur && cur->type == QUEUE_ENTRY_TYPE_TIMED_EVENT) {
        const parrot_event * const cur_event = (parrot_event *)cur->data;
        if (abs_time > cur_event->u.timer_event.abs_time) {
            prev = cur;
            cur = cur->next;
        }
        else
            break;
    }
    if (!prev)
        queue->head = entry;
    else {
        prev->next = entry;
        if (prev == queue->tail)
            queue->tail = entry;
    }
    entry->next = cur;
}

/*

=item C<void insert_entry(QUEUE *queue, QUEUE_ENTRY *entry)>

Does a synchronized insert of C<entry>.

=cut

*/

void
insert_entry(ARGMOD(QUEUE *queue), ARGIN(QUEUE_ENTRY *entry))
{
    ASSERT_ARGS(insert_entry)
    queue_lock(queue);
    nosync_insert_entry(queue, entry);
    queue_signal(queue);
    queue_unlock(queue);
}

/*

=item C<void queue_lock(QUEUE *queue)>

Locks the queue's mutex.

=cut

*/

void
queue_lock(ARGMOD(QUEUE *queue))
{
    ASSERT_ARGS(queue_lock)
    LOCK(queue->queue_mutex);
}

/*

=item C<void queue_unlock(QUEUE *queue)>

Unlocks the queue's mutex.

=cut

*/

void
queue_unlock(ARGMOD(QUEUE *queue))
{
    ASSERT_ARGS(queue_unlock)
    UNLOCK(queue->queue_mutex);
}

/*

=item C<void queue_broadcast(QUEUE *queue)>

This function wakes up I<every> thread waiting on the queue.

=cut

*/

void
queue_broadcast(ARGMOD(QUEUE *queue))
{
    ASSERT_ARGS(queue_broadcast)
    COND_BROADCAST(queue->queue_condition);
}

/*

=item C<void queue_signal(QUEUE *queue)>

=cut

*/

void
queue_signal(ARGMOD(QUEUE *queue))
{
    ASSERT_ARGS(queue_signal)
    COND_SIGNAL(queue->queue_condition);
}

/*

=item C<void queue_wait(QUEUE *queue)>

Instructs the queue to wait.

=cut

*/

void
queue_wait(ARGMOD(QUEUE *queue))
{
    ASSERT_ARGS(queue_wait)
    COND_WAIT(queue->queue_condition, queue->queue_mutex);
}

/*

=item C<void queue_timedwait(QUEUE *queue, const struct timespec *abs_time)>

Instructs the queue to wait for C<abs_time> seconds (?).

=cut

*/

void
queue_timedwait(ARGMOD(QUEUE *queue), ARGIN(const struct timespec *abs_time))
{
    ASSERT_ARGS(queue_timedwait)
    COND_TIMED_WAIT(queue->queue_condition, queue->queue_mutex, abs_time);
}

/*

=item C<QUEUE* queue_init(UINTVAL prio)>

Initializes the queue, setting C<prio> as the queue's priority.

=cut

*/

PARROT_CAN_RETURN_NULL
PARROT_MALLOC
QUEUE*
queue_init(UINTVAL prio)
{
    ASSERT_ARGS(queue_init)
    QUEUE * const queue = mem_allocate_typed(QUEUE);

    queue->head = queue->tail = NULL;
    queue->max_prio = prio;
    COND_INIT(queue->queue_condition);
    MUTEX_INIT(queue->queue_mutex);
    return queue;
}

/*

=item C<void queue_destroy(QUEUE *queue)>

Destroys the queue, raising an exception if it is not empty.

=cut

*/

void
queue_destroy(ARGMOD(QUEUE *queue))
{
    ASSERT_ARGS(queue_destroy)
    if (peek_entry(queue))
        exit_fatal(1, "Queue not empty on destroy");

    COND_DESTROY(queue->queue_condition);
    MUTEX_DESTROY(queue->queue_mutex);
    mem_sys_free(queue);
}

/*

=back

=head1 SEE ALSO

F<include/parrot/tsq.h>.

=cut

*/


/*
 * Local variables:
 *   c-file-style: "parrot"
 * End:
 * vim: expandtab shiftwidth=4:
 */
