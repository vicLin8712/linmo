#include <lib/libc.h>
#include <lib/malloc.h>
#include <lib/queue.h>

#include "private/error.h"
#include "private/utils.h"

queue_t *queue_create(int32_t capacity)
{
    /* A capacity of at least 2 is required for an N-1 queue. */
    if (capacity < 2)
        capacity = 2;

    if (!ispowerof2(capacity))
        capacity = nextpowerof2(capacity);

    queue_t *q = calloc(1, sizeof(queue_t));
    if (!q)
        return NULL;

    q->buf = malloc(capacity * sizeof(void *));
    if (!q->buf) {
        free(q);
        return NULL;
    }

    q->size = capacity;
    q->mask = capacity - 1;
    /* head and tail are correctly initialized to 0 by calloc */

    return q;
}

/* Destroys a queue, but only if it is empty.
 * This safety check prevents leaking the pointers stored in the queue.
 */
int32_t queue_destroy(queue_t *q)
{
    /* Refuse to destroy a non-empty queue. */
    if (!q || !queue_is_empty(q))
        return ERR_FAIL;

    free(q->buf);
    free(q);
    return ERR_OK;
}

/* Adds an element to the tail of the queue. */
int32_t queue_enqueue(queue_t *q, void *ptr)
{
    /* The queue can only store up to 'size - 1' items. */
    if (queue_is_full(q))
        return ERR_FAIL;

    q->buf[q->tail] = ptr;
    q->tail = (q->tail + 1) & q->mask;
    return ERR_OK;
}

/* Removes an element from the head of the queue. */
void *queue_dequeue(queue_t *q)
{
    if (queue_is_empty(q))
        return NULL;

    void *item = q->buf[q->head];
    q->head = (q->head + 1) & q->mask;
    return item;
}

/* Returns the element at the head of the queue without removing it. */
void *queue_peek(const queue_t *q)
{
    if (queue_is_empty(q))
        return NULL;
    return q->buf[q->head];
}
