/* Lock-free, single-producer, single-consumer (SPSC) ring buffer queue.
 *
 * This queue is implemented as a circular buffer and is designed for
 * pointer-sized elements. It is "lock-free" in the sense that it does not
 * require mutexes for SPSC scenarios, but it is NOT thread-safe for
 * multiple producers or multiple consumers. For multi-producer/consumer
 * use, access must be protected by an external synchronization primitive
 * like a mutex.
 */

#pragma once

#include <lib/libc.h>
#include "private/utils.h"

/* State is managed entirely by head/tail indices. The capacity is always
 * a power of two to allow for efficient bitwise masking instead of a more
 * expensive modulo operation.
 *
 * The effective capacity is 'size - 1' to distinguish the full state from the
 * empty state without extra variables.
 */
typedef struct {
    void **buf;            /* The allocated buffer for pointer elements. */
    int32_t size;          /* The total size of the buffer (a power of 2). */
    int32_t mask;          /* The bitwise mask, equivalent to 'size - 1'. */
    volatile int32_t head; /* The read index. */
    volatile int32_t tail; /* The write index. */
} queue_t;

/* Creates and initializes a new queue.
 * @capacity: The desired minimum capacity. The actual capacity will be rounded
 *           up to the next power of two.
 * Return A pointer to the newly created queue, or NULL on failure.
 */
queue_t *queue_create(int32_t capacity);

/* Destroys a queue and frees its resources.
 * This operation will fail if the queue is not empty, preventing memory
 * leaks of the items contained within the queue.
 * @q: The queue to destroy.
 *
 * Return 0 on success, or a negative error code on failure.
 */
int32_t queue_destroy(queue_t *q);

/* Checks if the queue is empty. */
static inline bool queue_is_empty(const queue_t *q)
{
    return !q || q->head == q->tail;
}

/* Returns the number of elements currently in the queue.
 *
 * If the queue pointer is NULL, it is treated as an empty queue
 * and this function returns 0. This ensures safe behavior even
 * when invalid pointers are passed.
 */
static inline uint32_t queue_count(const queue_t *q)
{
    /* Treat NULL queue as empty to prevent undefined behavior */
    if (unlikely(!q))
        return 0u;
    return (q->tail - q->head + q->size) & q->mask;
}

/* Checks if the queue is full. */
static inline bool queue_is_full(const queue_t *q)
{
    return q && (((q->tail + 1) & q->mask) == q->head);
}

/* Adds an element to the tail of the queue (FIFO). */
int32_t queue_enqueue(queue_t *q, void *ptr);

/* Removes an element from the head of the queue (FIFO). */
void *queue_dequeue(queue_t *q);

/* Returns the element at the head of the queue without removing it. */
void *queue_peek(const queue_t *q);
