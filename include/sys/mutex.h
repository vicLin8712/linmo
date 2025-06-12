#pragma once

/* Lightweight (non-recursive) mutexes and POSIX-style condition
 * variables.
 * - Mutex : binary semaphore + owner field
 * - Condition variable : FIFO list of blocked TCBs
 *
 * A mutex is a synchronization primitive that guarantees mutual exclusion:
 * the first thread that calls mo_mutex_lock() becomes the owner and may access
 * the protected resource, while every subsequent thread that calls
 * mo_mutex_lock() is blocked and placed on the mutex's wait list until the
 * owner calls mo_mutex_unlock() or a specified timeout expires.
 *
 * Because mo_mutex_lock() may block, both locking and unlocking operations must
 * be invoked from a context in which the thread scheduler is allowed to run --
 * i.e., from user or kernel thread context, not from an interrupt handler.
 */

#include <lib/list.h>
#include <sys/semaphore.h>

/* Magic numbers for validation */
#define MUTEX_MAGIC 0x4D555458 /* "MUTX" */
#define COND_MAGIC 0x434F4E44  /* "COND" */

/* Mutex Control Block
 * This implementation is self-contained and does not rely on a semaphore. It
 * tracks ownership via a task ID and maintains its own queue of waiting tasks.
 */
typedef struct {
    list_t *waiters;    /* list of 'tcb_t *' blocked on this mutex */
    uint16_t owner_tid; /* 0 if unlocked, otherwise task ID of owner */
    uint32_t magic;     /* Magic number for validation */
} mutex_t;

/* Initialize a mutex to an unlocked state */
int32_t mo_mutex_init(mutex_t *m);

/* Destroy a mutex (fails if tasks are waiting) */
int32_t mo_mutex_destroy(mutex_t *m);

/* Acquire mutex lock (blocking) */
int32_t mo_mutex_lock(mutex_t *m);

/* Attempt to acquire mutex lock without blocking */
int32_t mo_mutex_trylock(mutex_t *m);

/* Attempt to acquire mutex lock with timeout */
int32_t mo_mutex_timedlock(mutex_t *m, uint32_t ticks);

/* Release mutex lock */
int32_t mo_mutex_unlock(mutex_t *m);

/* Check if current task owns the mutex */
bool mo_mutex_owned_by_current(mutex_t *m);

/* Get the number of tasks waiting on the mutex */
int32_t mo_mutex_waiting_count(mutex_t *m);

/* Condition Variable Control Block
 * A condition variable allows tasks to wait for an arbitrary condition to
 * become true while a mutex is held.
 */
typedef struct {
    list_t *waiters; /* list of 'tcb_t *' blocked on this condition */
    uint32_t magic;  /* Magic number for validation */
} cond_t;

/* Initialize a condition variable */
int32_t mo_cond_init(cond_t *c);

/* Destroy a condition variable (fails if tasks are waiting) */
int32_t mo_cond_destroy(cond_t *c);

/* Wait on condition variable (atomically releases mutex) */
int32_t mo_cond_wait(cond_t *c, mutex_t *m);

/* Wait on condition variable with timeout */
int32_t mo_cond_timedwait(cond_t *c, mutex_t *m, uint32_t ticks);

/* Signal one waiting task */
int32_t mo_cond_signal(cond_t *c);

/* Signal all waiting tasks */
int32_t mo_cond_broadcast(cond_t *c);

/* Get the number of tasks waiting on the condition variable */
int32_t mo_cond_waiting_count(cond_t *c);
