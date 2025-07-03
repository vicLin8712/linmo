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

/* Initialize a mutex to an unlocked state.
 * @m: Pointer to mutex structure (must be non-NULL)
 *
 * Returns ERR_OK on success, ERR_FAIL on failure
 */
int32_t mo_mutex_init(mutex_t *m);

/* Destroy a mutex and free its resources.
 * Fails if any tasks are currently waiting on the mutex.
 * @m: Pointer to mutex structure (NULL is no-op)
 *
 * Returns ERR_OK on success, ERR_TASK_BUSY if tasks waiting, ERR_FAIL if
 * invalid
 */
int32_t mo_mutex_destroy(mutex_t *m);

/* Acquire mutex lock, blocking if necessary.
 * If the mutex is free, acquires it immediately. If owned by another task,
 * the calling task is placed in the wait queue and blocked until the mutex
 * becomes available. Non-recursive - returns error if caller already owns
 * mutex.
 * @m: Pointer to mutex structure (must be valid)
 *
 * Returns ERR_OK on success, ERR_TASK_BUSY if already owned by caller
 */
int32_t mo_mutex_lock(mutex_t *m);

/* Attempt to acquire mutex lock without blocking.
 * Returns immediately whether the lock was acquired or not.
 * @m: Pointer to mutex structure (must be valid)
 *
 * Returns ERR_OK if acquired, ERR_TASK_BUSY if unavailable, ERR_FAIL if
 * invalid
 */
int32_t mo_mutex_trylock(mutex_t *m);

/* Attempt to acquire mutex lock with timeout.
 * Blocks for up to 'ticks' scheduler ticks waiting for the mutex.
 * @m: Pointer to mutex structure (must be valid)
 * @ticks: Maximum time to wait in scheduler ticks (0 = trylock behavior)
 *
 * Returns ERR_OK if acquired, ERR_TIMEOUT if timed out, ERR_TASK_BUSY if
 * recursive
 */
int32_t mo_mutex_timedlock(mutex_t *m, uint32_t ticks);

/* Release mutex lock.
 * If tasks are waiting, ownership is transferred to the next task in FIFO
 * order. The released task is marked ready but may not run immediately
 * depending on scheduler priority.
 * @m: Pointer to mutex structure (must be valid)
 *
 * Returns ERR_OK on success, ERR_NOT_OWNER if caller doesn't own mutex
 */
int32_t mo_mutex_unlock(mutex_t *m);

/* Check if the current task owns the specified mutex.
 * @m: Pointer to mutex structure
 *
 * Returns true if current task owns mutex, false otherwise
 */
bool mo_mutex_owned_by_current(mutex_t *m);

/* Get the number of tasks currently waiting on the mutex.
 * @m: Pointer to mutex structure
 *
 * Returns Number of waiting tasks, or -1 if mutex is invalid
 */
int32_t mo_mutex_waiting_count(mutex_t *m);

/* Condition Variable Control Block
 *
 * A condition variable allows tasks to wait for arbitrary conditions to become
 * true. Tasks must hold an associated mutex when calling wait operations.
 * The mutex is atomically released during the wait and re-acquired before
 * the wait operation returns.
 */
typedef struct {
    list_t
        *waiters; /* List of 'tcb_t *' blocked on this condition (FIFO order) */
    uint32_t magic; /* Magic number for validation and corruption detection */
} cond_t;

/* Initialize a condition variable.
 * @c: Pointer to condition variable structure (must be non-NULL)
 *
 * Returns ERR_OK on success, ERR_FAIL on failure
 */
int32_t mo_cond_init(cond_t *c);

/* Destroy a condition variable and free its resources.
 * Fails if any tasks are currently waiting on the condition variable.
 * @c: Pointer to condition variable structure (NULL is no-op)
 *
 * Returns ERR_OK on success, ERR_TASK_BUSY if tasks waiting, ERR_FAIL if
 * invalid
 */
int32_t mo_cond_destroy(cond_t *c);

/* Wait on condition variable (atomically releases mutex).
 * The calling task must own the specified mutex. The mutex is atomically
 * released and the task blocks until another task signals the condition.
 * Upon waking, the mutex is re-acquired before returning.
 * @c: Pointer to condition variable structure (must be valid)
 * @m: Pointer to mutex structure that caller must own
 *
 * Returns ERR_OK on success, ERR_NOT_OWNER if caller doesn't own mutex
 */
int32_t mo_cond_wait(cond_t *c, mutex_t *m);

/* Wait on condition variable with timeout.
 * Like mo_cond_wait(), but limits wait time to specified number of ticks.
 * @c: Pointer to condition variable structure (must be valid)
 * @m: Pointer to mutex structure that caller must own
 * @ticks: Maximum time to wait in scheduler ticks (0 = immediate timeout)
 *
 * Returns ERR_OK if signaled, ERR_TIMEOUT if timed out, ERR_NOT_OWNER if not
 * owner
 */
int32_t mo_cond_timedwait(cond_t *c, mutex_t *m, uint32_t ticks);

/* Signal one waiting task.
 * Wakes up the oldest task waiting on the condition variable (FIFO order).
 * The signaled task will attempt to re-acquire the associated mutex.
 * @c: Pointer to condition variable structure (must be valid)
 *
 * Returns ERR_OK on success, ERR_FAIL if invalid
 */
int32_t mo_cond_signal(cond_t *c);

/* Signal all waiting tasks.
 * Wakes up all tasks waiting on the condition variable. All tasks will
 * attempt to re-acquire their associated mutexes, but only one will succeed
 * immediately (others will block on the mutex).
 * @c: Pointer to condition variable structure (must be valid)
 *
 * Returns ERR_OK on success, ERR_FAIL if invalid
 */
int32_t mo_cond_broadcast(cond_t *c);

/* Get the number of tasks currently waiting on the condition variable.
 * @c: Pointer to condition variable structure
 *
 * Returns Number of waiting tasks, or -1 if condition variable is invalid
 */
int32_t mo_cond_waiting_count(cond_t *c);
