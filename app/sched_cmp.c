#include <linmo.h>

#ifndef TEST_SCENARIO
#define TEST_SCENARIO 0
#endif

#ifndef DURATION
#define DURATION 40000 /* Execution duration(ms) */
#endif

/* Test scenario for O(1) and O(n) scheduler */

/* Comparison of new and old scheduler test suit */
static const struct {
    const char *name;
    uint32_t task_count;
    int task_active_ratio;
} perf_tests[] = {
    {"Minimal Active", 500, 2}, /* 2% tasks available */
    {"Moderate Active", 500, 4}, {"Heavy Active", 500, 20},
    {"Stress Test", 500, 50},    {"Full Load Test", 500, 100},
};


/* Test suit for O(1) and O(n) scheduler */
uint32_t test_start_time, max_schedule_time;

int end_task_id;

/* Default task */
void task_normal(void)
{
    while (1) {
        if (mo_uptime() - test_start_time > DURATION) {
            /* Wake up end task */
            mo_task_resume(end_task_id);
        }

        /* Start record max schedule time after 3000ms(3s) */
        if (mo_uptime() - test_start_time > 3000)
            max_schedule_time = (each_schedule_time > max_schedule_time)
                                    ? each_schedule_time
                                    : max_schedule_time;

        mo_task_wfi();
    }
}

/* Random task */
void task_random(void)
{
    int cnt = random() % 100;
    while (cnt--) {
        /* Start record max schedule time after 3000ms(3s) */
        if (mo_uptime() - test_start_time > 3000)
            max_schedule_time = (each_schedule_time > max_schedule_time)
                                    ? each_schedule_time
                                    : max_schedule_time;

        mo_task_wfi();
    }
    mo_task_suspend(mo_task_id());
}

void tasks_init(void)
{
    for (uint32_t i = 0; i < perf_tests[TEST_SCENARIO].task_count; i++) {
        /* Active ratio */
        bool active =
            ((random() % 100 < perf_tests[TEST_SCENARIO].task_active_ratio));

        int task_id = 0;

        /* task count ratio of normal:random tasks 3:1*/
        if (random() % 4 != 0)
            task_id = mo_task_spawn(task_normal, DEFAULT_STACK_SIZE);
        else
            task_id = mo_task_spawn(task_random, DEFAULT_STACK_SIZE);

        /* Suspend if task is not active */
        if (!active)
            mo_task_suspend(task_id);
    }
}

void run_scheduler_performance_evaluation(void)
{
    printf("=== Linmo Enhanced Scheduler Performance Evaluation ===\n");

    /* Old scheduler performance */
#ifdef OLD
    printf("\nRunning test: %s for old scheduler \n",
           perf_tests[TEST_SCENARIO].name);
    printf("Task count: %d \n", perf_tests[TEST_SCENARIO].task_count);
    printf("Task active ratio: %d \n",
           perf_tests[TEST_SCENARIO].task_active_ratio);

    schedule_cnt = 0;
    schedule_time = 0;

    /* Suspend this task */
    test_start_time = mo_uptime();
    mo_task_suspend(end_task_id);

    /* Print result when task resumed */
    printf("\nOld scheduler avg scheduling time: %d ns\n",
           ((schedule_time * 1000) / (schedule_cnt)));
    printf("Maximum schedule time: %d ns\n", max_schedule_time);
    printf("END TEST \n");

    while (1)
        mo_task_wfi();

#else
    /* New scheduler performance */
    printf("\nRunning test: %s for new scheduler \n",
           perf_tests[TEST_SCENARIO].name);

    printf("Task count: %d \n", perf_tests[TEST_SCENARIO].task_count);
    schedule_cnt = 0;
    schedule_time = 0;

    /* Setup first task node of task list for old scheduler iteration */
    /* Suspend this task */
    test_start_time = mo_uptime();
    mo_task_suspend(end_task_id);

    /* Print result when task resumed */
    printf("\nNew scheduler avg scheduling time: %d ns\n",
           (int) ((schedule_time * 1000) / (schedule_cnt)));
    printf("Maximum schedule time: %d ns\n", max_schedule_time);
    printf("END TEST \n");

    while (1)
        mo_task_wfi();
#endif
}

int32_t app_main(void)
{
    end_task_id =
        mo_task_spawn(run_scheduler_performance_evaluation, DEFAULT_STACK_SIZE);
    mo_task_priority(end_task_id, TASK_PRIO_CRIT);
    mo_logger_flush();
    tasks_init();

#ifdef OLD
    kcb->task_current = kcb->tasks->head->next;
#endif
    /* preemptive mode */
    return 1;
}
