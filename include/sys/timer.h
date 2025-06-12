#pragma once

#include <types.h>

/* Defines how a timer behaves after it expires. */
typedef enum {
    TIMER_DISABLED = 0,  /**< The timer is created but not running. */
    TIMER_ONESHOT = 1,   /**< The timer fires once, then disables itself. */
    TIMER_AUTORELOAD = 2 /* re-arms itself automatically after firing. */
} timer_mode_t;

/* Timer Control Block.
 *
 * This internal structure holds the state of a single software timer. It is
 * managed entirely by the kernel; user code interacts with it via its ID.
 */
typedef struct {
    /* Core timer data */
    uint32_t deadline_ticks; /**< Expiration time in absolute system ticks. */
    uint32_t period_ms;      /**< Reload period in milliseconds. */
    uint16_t id;             /**< Unique handle assigned by the kernel. */
    uint8_t mode;            /**< Current operating mode (from timer_mode_t). */
    uint8_t _reserved;       /**< Padding for alignment. */

    /* Callback and argument - accessed on expiration */
    void *(*callback)(void *arg); /**< execute upon timer expiry. */
    void *arg; /**< A user-defined argument passed to the callback. */
} timer_t;

/* Creates a new software timer.
 *
 * The timer is created in a DISABLED state and must be started with
 * 'mo_timer_start()'.
 *
 * @callback  : The function to execute upon expiry. Cannot be NULL.
 * @period_ms : The timer's period in milliseconds. Must be greater than 0.
 * @arg       : A user-defined argument to be passed to the callback.
 *
 * Return A positive timer ID on success, or a negative error code on failure.
 */
int32_t mo_timer_create(void *(*callback)(void *arg),
                        uint32_t period_ms,
                        void *arg);

/* Destroys a software timer and frees its resources.
 *
 * If the timer is active, it will be cancelled before being destroyed.
 * @id The ID of the timer to destroy, as returned by 'mo_timer_create'.
 *
 * Return ERR_OK on success, or ERR_FAIL if the ID is not found.
 */
int32_t mo_timer_destroy(uint16_t id);

/* Starts or restarts a software timer.
 *
 * This function arms the timer and adds it to the active list. If the timer
 * was already running, its deadline is recalculated and it is rescheduled.
 * @id   : The ID of the timer to start.
 * @mode : The desired mode (TIMER_ONESHOT or TIMER_AUTORELOAD).
 *
 * Return ERR_OK on success, or ERR_FAIL if the ID or mode is invalid.
 */
int32_t mo_timer_start(uint16_t id, uint8_t mode);

/* Cancels a running software timer.
 *
 * This function disarms the timer and removes it from the active list. The
 * timer object itself is not destroyed and can be restarted later.
 * @id : The ID of the timer to cancel.
 *
 * Return ERR_OK on success, or ERR_FAIL if the timer is not found or not
 * running.
 */
int32_t mo_timer_cancel(uint16_t id);

/* Convert milliseconds to system ticks.
 *
 * F_TIMER is the scheduler tick frequency (in Hz), which must be defined at
 * build-time. This calculation is performed with 64-bit integers to prevent
 * overflow with large millisecond values.
 */
#define MS_TO_TICKS(ms) \
    ((uint32_t) (((uint64_t) (ms) * (uint64_t) (F_TIMER)) / 1000U))
