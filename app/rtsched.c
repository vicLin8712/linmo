#include <linmo.h>

/* Task 4: Simple task that prints a message and waits for scheduling */
static void task4(void)
{
    while (1) {
        printf("Task 4 running\n");
        mo_task_wfi(); /* Wait for interrupt to yield control */
    }
}

/* Task 3: Simple task that prints a message and waits for scheduling */
static void task3(void)
{
    while (1) {
        printf("Task 3 running\n");
        mo_task_wfi(); /* Wait for interrupt to yield control */
    }
}

/* Task 2: Prints task ID and an incrementing counter, then waits */
static void task2(void)
{
    int32_t cnt = 300000;

    while (1) {
        printf("[Task %d: %ld]\n", mo_task_id(), cnt++);
        mo_task_wfi(); /* Yield control to scheduler */
    }
}

/* Task 1: Prints task ID and an incrementing counter, then waits */
static void task1(void)
{
    int32_t cnt = 200000;

    while (1) {
        printf("[Task %d: %ld]\n", mo_task_id(), cnt++);
        mo_task_wfi(); /* Yield control to scheduler */
    }
}

/* Task 0: Prints task ID and an incrementing counter, then waits */
static void task0(void)
{
    int32_t cnt = 100000;

    while (1) {
        printf("[Task %d: %ld]\n", mo_task_id(), cnt++);
        mo_task_wfi(); /* Yield control to scheduler */
    }
}

typedef struct {
    unsigned credits;
    unsigned remaining;
} custom_prio_t;

/* A simple credit-based real-time scheduler
 * – Every RT task carries a custom_prio_t record via its tcb_t::rt_prio field.
 * – Each time the scheduler selects a task it decrements "remaining".
 *   When "remaining" reaches zero it is reloaded from "credits" on the task’s
 *   next turn.
 * – The function returns the ID of the selected RT task, or –1 when no RT task
 *   is ready so the kernel should fall back to its round-robin scheduler.
 */
static int32_t custom_sched(void)
{
    static list_node_t *task_node = NULL; /* resume point */

    /* If we have no starting point or we’ve wrapped, begin at head->next */
    if (!task_node)
        task_node = list_next(kcb->tasks->head);

    /* Scan at most one full loop of the list */
    list_node_t *start = task_node;
    do {
        if (!task_node) /* empty list */
            return -1;

        /* Skip head/tail sentinels and NULL-data nodes */
        if (task_node == kcb->tasks->head || task_node == kcb->tasks->tail ||
            !task_node->data) {
            task_node = list_next(task_node);
            continue;
        }

        /* Safe: data is non-NULL here */
        tcb_t *task = (tcb_t *) task_node->data;

        /* READY + RT-eligible ? */
        if (task->state == TASK_READY && task->rt_prio) {
            /* Consume one credit */
            custom_prio_t *cp = (custom_prio_t *) task->rt_prio;
            if (cp->remaining == 0)
                cp->remaining = cp->credits;
            cp->remaining--;

            /* Next time resume with the following node */
            task_node = list_next(task_node);
            if (task_node == kcb->tasks->head || task_node == kcb->tasks->tail)
                task_node = list_next(task_node); /* skip sentinel  */
            return task->id;
        }

        /* Otherwise advance */
        task_node = list_next(task_node);
    } while (task_node != start); /* one full lap */

    /* No READY RT task this cycle */
    task_node = NULL; /* restart next */
    return -1;
}

/* Application Entry Point: Initializes tasks and scheduler
 *
 * Spawns five tasks, assigns real-time priorities to tasks 0, 1, and 2,
 * and sets up the custom credit-based scheduler. Enables preemptive mode.
 */
int32_t app_main(void)
{
    /* Define RT task priorities with initial credit values */
    static custom_prio_t priorities[3] = {
        {.credits = 3, .remaining = 3}, /* Task 0 */
        {.credits = 4, .remaining = 4}, /* Task 1 */
        {.credits = 5, .remaining = 5}, /* Task 2 */
    };

    /* Spawn tasks with default stack size */
    mo_task_spawn(task0, DEFAULT_STACK_SIZE);
    mo_task_spawn(task1, DEFAULT_STACK_SIZE);
    mo_task_spawn(task2, DEFAULT_STACK_SIZE);
    mo_task_spawn(task3, DEFAULT_STACK_SIZE);
    mo_task_spawn(task4, DEFAULT_STACK_SIZE);

    /* Configure custom scheduler and assign RT priorities */
    kcb->rt_sched = custom_sched;
    mo_task_rt_priority(0, &priorities[0]);
    mo_task_rt_priority(1, &priorities[1]);
    mo_task_rt_priority(2, &priorities[2]);

    /* preemptive scheduling */
    return 1;
}
