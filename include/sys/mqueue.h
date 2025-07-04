#pragma once

#include <lib/queue.h>

/* Message container */
typedef struct {
    void *payload; /* pointer to user-allocated buffer */
    uint16_t type; /* user-defined discriminator */
    uint16_t size; /* payload size in bytes */
} message_t;

/* Message-queue descriptor */
typedef struct {
    queue_t *q; /* FIFO of (message_t *) pointers */
} mq_t;

mq_t *mo_mq_create(uint16_t size);
int32_t mo_mq_destroy(mq_t *mq);
int32_t mo_mq_enqueue(mq_t *mq, message_t *m);
message_t *mo_mq_dequeue(mq_t *mq);
message_t *mo_mq_peek(mq_t *mq);

static inline int32_t mo_mq_items(mq_t *mq)
{
    /* Add NULL safety to prevent crashes */
    if (unlikely(!mq || !mq->q))
        return 0;
    return queue_count(mq->q);
}
