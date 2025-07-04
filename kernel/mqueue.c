/* message queues backed by the generic queue_t */

#include <lib/malloc.h>
#include <lib/queue.h>

#include <sys/mqueue.h>
#include <sys/task.h>

#include "private/error.h"
#include "private/utils.h"

mq_t *mo_mq_create(uint16_t max_items)
{
    mq_t *mq = malloc(sizeof *mq);
    if (unlikely(!mq))
        return NULL;

    mq->q = queue_create(max_items);
    if (unlikely(!mq->q)) {
        free(mq);
        return NULL;
    }
    return mq;
}

int32_t mo_mq_destroy(mq_t *mq)
{
    if (unlikely(!mq))
        return ERR_OK; /* Destroying NULL is no-op */

    if (unlikely(!mq->q))
        return ERR_FAIL; /* Invalid mqueue state */

    CRITICAL_ENTER();

    if (unlikely(queue_count(mq->q) != 0)) { /* refuse to destroy non-empty q */
        CRITICAL_LEAVE();
        return ERR_MQ_NOTEMPTY;
    }

    /* Safe to destroy now - no need to hold critical section */
    CRITICAL_LEAVE();

    queue_destroy(mq->q);
    free(mq);

    return ERR_OK;
}

int32_t mo_mq_enqueue(mq_t *mq, message_t *msg)
{
    if (unlikely(!mq || !mq->q || !msg))
        return ERR_FAIL;

    int32_t rc;

    CRITICAL_ENTER();
    rc = queue_enqueue(mq->q, msg);
    CRITICAL_LEAVE();

    return rc; /* 0 on success, −1 on full */
}

/* remove oldest message (FIFO) */
message_t *mo_mq_dequeue(mq_t *mq)
{
    if (unlikely(!mq || !mq->q))
        return NULL;

    message_t *msg;

    CRITICAL_ENTER();
    msg = queue_dequeue(mq->q);
    CRITICAL_LEAVE();

    return msg; /* NULL when queue is empty */
}

/* inspect head without removing */
message_t *mo_mq_peek(mq_t *mq)
{
    if (unlikely(!mq || !mq->q))
        return NULL;

    message_t *msg;

    CRITICAL_ENTER();
    msg = queue_peek(mq->q);
    CRITICAL_LEAVE();

    return msg; /* NULL when queue is empty */
}
