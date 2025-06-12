/* Mutex using binary semaphore Mutex */

#include <linmo.h>

#include "private/error.h"

/* Test configuration */
#define MAX_ITERATIONS 5
#define COOPERATION_YIELDS 3 /* Number of yields between iterations */

/* Shared resources protected by binary semaphore */
static sem_t *binary_mutex = NULL;
static int shared_counter = 0;
static int task_a_count = 0, task_b_count = 0;
static int critical_section_violations = 0;
static int currently_in_critical_section = 0;

/* Enhanced Task A */
void task_a(void)
{
    printf("Task A (ID %d) starting...\n", mo_task_id());

    for (int i = 0; i < MAX_ITERATIONS; i++) {
        printf("Task A: Requesting mutex (iteration %d)\n", i + 1);
        mo_sem_wait(binary_mutex);

        /* === CRITICAL SECTION START === */
        if (currently_in_critical_section != 0) {
            printf("Task A: VIOLATION - Multiple tasks in critical section!\n");
            critical_section_violations++;
        }
        currently_in_critical_section = mo_task_id();

        printf("Task A: Entering critical section\n");
        int old_counter = shared_counter;

        /* Simulate work with yields instead of delays */
        for (int work = 0; work < 3; work++)
            mo_task_yield();

        shared_counter = old_counter + 1;
        task_a_count++;
        printf("Task A: Updated counter: %d -> %d\n", old_counter,
               shared_counter);

        if (currently_in_critical_section != mo_task_id()) {
            printf("Task A: VIOLATION - Critical section corrupted!\n");
            critical_section_violations++;
        }
        currently_in_critical_section = 0;
        /* === CRITICAL SECTION END === */

        mo_sem_signal(binary_mutex);
        printf("Task A: Released mutex\n");

        /* Cooperative scheduling */
        for (int j = 0; j < COOPERATION_YIELDS; j++)
            mo_task_yield();
    }

    printf("Task A completed %d iterations\n", task_a_count);

    /* Keep running to prevent panic */
    while (1) {
        for (int i = 0; i < 10; i++)
            mo_task_yield();
    }
}

/* Enhanced Task B */
void task_b(void)
{
    printf("Task B (ID %d) starting...\n", mo_task_id());

    for (int i = 0; i < MAX_ITERATIONS; i++) {
        printf("Task B: Trying trylock (iteration %d)\n", i + 1);

        /* Try non-blocking first */
        int32_t trylock_result = mo_sem_trywait(binary_mutex);
        if (trylock_result != ERR_OK) {
            printf("Task B: Mutex busy, using blocking wait\n");
            mo_sem_wait(binary_mutex);
        } else {
            printf("Task B: Trylock succeeded\n");
        }

        /* === CRITICAL SECTION START === */
        if (currently_in_critical_section != 0) {
            printf("Task B: VIOLATION - Multiple tasks in critical section!\n");
            critical_section_violations++;
        }
        currently_in_critical_section = mo_task_id();

        printf("Task B: Entering critical section\n");
        int old_counter = shared_counter;

        /* Simulate work */
        for (int work = 0; work < 3; work++)
            mo_task_yield();

        shared_counter = old_counter + 10;
        task_b_count++;
        printf("Task B: Updated counter: %d -> %d\n", old_counter,
               shared_counter);

        if (currently_in_critical_section != mo_task_id()) {
            printf("Task B: VIOLATION - Critical section corrupted!\n");
            critical_section_violations++;
        }
        currently_in_critical_section = 0;
        /* === CRITICAL SECTION END === */

        mo_sem_signal(binary_mutex);
        printf("Task B: Released mutex\n");

        /* Cooperative scheduling */
        for (int j = 0; j < COOPERATION_YIELDS; j++)
            mo_task_yield();
    }

    printf("Task B completed %d iterations\n", task_b_count);

    /* Keep running to prevent panic */
    while (1) {
        for (int i = 0; i < 10; i++)
            mo_task_yield();
    }
}

/* Simple monitor task */
void monitor_task(void)
{
    printf("Monitor starting...\n");

    int cycles = 0;

    while (cycles < 50) { /* Monitor for reasonable time */
        cycles++;

        /* Check progress every few cycles */
        if (cycles % 10 == 0) {
            printf("Monitor: A=%d, B=%d, Counter=%d, Violations=%d\n",
                   task_a_count, task_b_count, shared_counter,
                   critical_section_violations);
        }

        /* Check if both tasks completed */
        if (task_a_count >= MAX_ITERATIONS && task_b_count >= MAX_ITERATIONS) {
            printf("Monitor: Both tasks completed successfully\n");
            break;
        }

        /* Yield to let other tasks run */
        for (int i = 0; i < 5; i++)
            mo_task_yield();
    }

    /* Final report */
    printf("\n=== FINAL RESULTS ===\n");
    printf("Task A iterations: %d\n", task_a_count);
    printf("Task B iterations: %d\n", task_b_count);
    printf("Final shared counter: %d\n", shared_counter);
    printf("Expected counter: %d\n", task_a_count * 1 + task_b_count * 10);
    printf("Critical section violations: %d\n", critical_section_violations);

    /* Test validation */
    bool fairness_ok = (task_a_count > 0 && task_b_count > 0);
    bool mutex_ok = (critical_section_violations == 0);
    bool data_ok = (shared_counter == (task_a_count * 1 + task_b_count * 10));

    printf("\nTest Results:\n");
    printf("Fairness: %s\n", fairness_ok ? "PASS" : "FAIL");
    printf("Mutual Exclusion: %s\n", mutex_ok ? "PASS" : "FAIL");
    printf("Data Consistency: %s\n", data_ok ? "PASS" : "FAIL");
    printf("Overall: %s\n",
           (fairness_ok && mutex_ok && data_ok) ? "PASS" : "FAIL");

    printf("Binary semaphore mutex test completed.\n");

    /* Keep running */
    while (1) {
        for (int i = 0; i < 20; i++)
            mo_task_yield();
    }
}

/* Simple idle task */
void idle_task(void)
{
    while (1)
        mo_task_yield(); /* Use yield instead of WFI */
}

/* Application entry point */
int32_t app_main(void)
{
    printf("Binary Semaphore Test Starting...\n");

    /* Create binary semaphore */
    binary_mutex = mo_sem_create(10, 1);
    if (!binary_mutex) {
        printf("FATAL: Failed to create binary semaphore\n");
        return false;
    }

    printf("Binary semaphore created successfully\n");

    /* Create tasks */
    int32_t task_a_id = mo_task_spawn(task_a, 1024);
    int32_t task_b_id = mo_task_spawn(task_b, 1024);
    int32_t monitor_id = mo_task_spawn(monitor_task, 1024);
    int32_t idle_id = mo_task_spawn(idle_task, 512);

    if (task_a_id < 0 || task_b_id < 0 || monitor_id < 0 || idle_id < 0) {
        printf("FATAL: Failed to create tasks\n");
        return false;
    }

    printf("Tasks created: A=%d, B=%d, Monitor=%d, Idle=%d\n", (int) task_a_id,
           (int) task_b_id, (int) monitor_id, (int) idle_id);

    printf("Starting test...\n");
    return true; /* Enable preemptive scheduling */
}
