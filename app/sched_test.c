/*
 * Test suite for the RR-cursor based scheduler implementation.
 *
 * This suite verifies the consistency of the O(1) RR-cursor based scheduler
 * data structures, including bitmap bit positions and O(1) task count
 * tracking. It exercises all task states (TASK_READY, TASK_BLOCKED,
 * TASK_SUSPENDED, and TASK_RUNNING) and also verifies correct handling
 * of task cancellation.
 */

#include <linmo.h>

#include "private/error.h"

#define TEST_ASSERT(condition, description)    \
    do {                                       \
        if (condition) {                       \
            printf("PASS: %s\n", description); \
            tests_passed++;                    \
        } else {                               \
            printf("FAIL: %s\n", description); \
            tests_failed++;                    \
        }                                      \
    } while (0)

/* Test results tracking */
static int tests_passed = 0;
static int tests_failed = 0;

void task_normal(void)
{
    for (;;) {
        mo_task_wfi();
    }
}

void task_delay(void)
{
    mo_task_delay(10000);
    mo_task_yield();
    while (1)
        mo_task_wfi();
}

static bool bit_in_bitmap(int prio)
{
    return ((kcb->ready_bitmap & (1U << prio))) ? true : false;
}

static int32_t task_cnt_in_sched(int prio)
{
    return kcb->queue_counts[prio];
}

void test_bitmap_and_taskcnt(void)
{
    printf("\n=== Testing Bitmap and Task Count Consistency ===\n");

    /* Test TASK_READY when created */
    int task_id = mo_task_spawn(task_normal, DEFAULT_STACK_SIZE);
    TEST_ASSERT(bit_in_bitmap(4) == true,
                "Bitmap is consistent when TASK_READY");
    TEST_ASSERT(task_cnt_in_sched(4) == 1,
                "Task count is consistent when TASK_READY");

    /* Test task migration to different priority */
    mo_task_priority(task_id, TASK_PRIO_HIGH);
    TEST_ASSERT(bit_in_bitmap(2) == true && bit_in_bitmap(4) == false,
                "Bitmap is consistent when priority migration");
    TEST_ASSERT(task_cnt_in_sched(2) == 1 && task_cnt_in_sched(4) == 0,
                "Task count is consistent when priority migration");

    /* Test suspend task */
    mo_task_suspend(task_id);
    TEST_ASSERT(bit_in_bitmap(2) == false,
                "Bitmap is consistent when TASK_SUSPENDED");
    TEST_ASSERT(task_cnt_in_sched(2) == 0,
                "Task count is consistent when TASK_SUSPENDED");

    /* Test resume from task suspended */
    mo_task_resume(task_id);
    TEST_ASSERT(bit_in_bitmap(2) == true,
                "Bitmap is consistent when TASK_READY from TASK_SUSPENDED");
    TEST_ASSERT(task_cnt_in_sched(2) == 1,
                "Task count is consistent when TASK_READY from TASK_SUSPENDED");

    /* Test cancelling task */
    mo_task_cancel(task_id);
    TEST_ASSERT(bit_in_bitmap(2) == false,
                "Bitmap is consistent when task canceled");
    TEST_ASSERT(task_cnt_in_sched(2) == 0,
                "Task count is consistent when task canceled");

    /* Test block task by delay; the task_delay will blocked itself when it
     * executed */
    int blocked_task_id = mo_task_spawn(task_delay, DEFAULT_STACK_SIZE);

    /* Migrate delay task to TASK_PRIO_CRIT so that task_delay process can be
     * execute, block itself */
    mo_task_priority(blocked_task_id, TASK_PRIO_CRIT);

    /* Before yield, total task count in priority level 0 should be 2 (test
     * controller and delay task)*/
    TEST_ASSERT(task_cnt_in_sched(0) == 2,
                "Task count is consistent when task canceled");

    /* Yield and delay task will be executed, block itself */
    mo_task_yield();

    /* Only test_controller task in TASK_PRIO_CRIT ready queue */
    TEST_ASSERT(task_cnt_in_sched(0) == 1,
                "Task count is consistent when task blocked");

    mo_task_cancel(blocked_task_id);
}

void test_cursor(void)
{
    /* Check cursor set up when new task is created */
    int task_id = mo_task_spawn(task_normal, DEFAULT_STACK_SIZE);
    TEST_ASSERT(((tcb_t *) (kcb->rr_cursors[4]->data))->id == task_id,
                " Cursor setup successful ");

    /* Check when only one task in ready queue and executing, the new task
     * enqueue will advance cursor to point to enqueued task node for RR
     * consistency; use task priority change normal task to same priority of
     * controller task */
    mo_task_priority(task_id, TASK_PRIO_CRIT);
    TEST_ASSERT(((tcb_t *) (kcb->rr_cursors[0]->data))->id == task_id,
                " Cursor advance successful when new task enqueue into "
                "one-task-existing ready queue. ");
    TEST_ASSERT((kcb->rr_cursors[4]) == NULL,
                " Cursor set to NULL when no task. ");

    mo_task_cancel(task_id);
    TEST_ASSERT(((tcb_t *) (kcb->rr_cursors[0]->data))->id == mo_task_id(),
                " Cursor set successful when cursor-pointed task remove; "
                "cursor advanced. ");
}

/* Print test results */
void print_test_results(void)
{
    printf("\n=== Test Results ===\n");
    printf("Tests passed: %d\n", tests_passed);
    printf("Tests failed: %d\n", tests_failed);
    printf("Total tests: %d\n", tests_passed + tests_failed);

    if (tests_failed == 0) {
        printf("All tests PASSED!\n");
    } else {
        printf("Some tests FAILED!\n");
    }
}

void schedule_test_task(void)
{
    printf("Starting RR-cursor based scheduler test suits...\n");

    test_bitmap_and_taskcnt();
    test_cursor();

    print_test_results();
    printf("RR-cursor based scheduler tests completed successfully.\n");

    /* Test complete - go into low-activity mode */
    while (1)
        mo_task_wfi();
}

int32_t app_main(void)
{
    int test_controller = mo_task_spawn(schedule_test_task, DEFAULT_STACK_SIZE);
    mo_task_priority(test_controller, TASK_PRIO_CRIT);
    /* preemptive scheduling */
    return 1;
}
