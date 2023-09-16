/*
 * Copyright (c) 2022 Legrand North America, LLC.
 *
 * SPDX-License-Identifier: MIT
 */

/* @file
 * @brief test of bsc-event interface
 */

#include <zephyr/kernel.h>
#include <zephyr/ztest.h>
#include <bacnet/datalink/bsc/bsc-event.h>

typedef enum {
    STAGE_NONE,
    STAGE_WAIT_1,
    STAGE_WAIT_2,
    STAGE_TIMEDWAIT_TIMEOUT,
    STAGE_TIMEDWAIT_OK,
} TEST_STAGE;
static TEST_STAGE test_stage = STAGE_NONE;

#define TIMEOUT_CHILD   400
#define TIMEOUT_MIN     200
#define TIMEOUT_MAX     600
#define TIMEOUT_SLEEP   2
#define WAITTIME_MIN    (TIMEOUT_SLEEP * MSEC_PER_SEC - 20)
#define WAITTIME_MAX    (TIMEOUT_SLEEP * MSEC_PER_SEC + 20)
#define MULTIPLE_WAIT_THREADS_NUM 50

#define STACKSIZE 128
K_KERNEL_STACK_DEFINE(child_stack, STACKSIZE);
K_KERNEL_STACK_ARRAY_DEFINE(
    child_stacks, MULTIPLE_WAIT_THREADS_NUM, STACKSIZE);


static void child_func(void *p1, void *p2, void *p3)
{
    BSC_EVENT *event = (BSC_EVENT *)p1;
    zassert_not_null(event, NULL);

    while (test_stage != STAGE_WAIT_1) {
        k_usleep(10);
    }
    bsc_event_signal(event);

    while (test_stage != STAGE_WAIT_2) {
        k_usleep(10);
    }
    bsc_event_signal(event);

    while (test_stage != STAGE_TIMEDWAIT_TIMEOUT) {
        k_usleep(10);
    }
    k_usleep(1000 * TIMEOUT_CHILD);
    bsc_event_signal(event);

    while (test_stage != STAGE_TIMEDWAIT_OK) {
        k_usleep(10);
    }
    k_usleep(1000 * TIMEOUT_CHILD);
    bsc_event_signal(event);
}

#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(bsc_event_test1, test_bsc_event1)
#else
static void test_bsc_event1(void)
#endif
{
    BSC_EVENT *event;
    k_tid_t tid_child;
    struct k_thread thread;
    bool b;
    int64_t t1;
    int64_t t2;
    int64_t timediff;

    test_stage = STAGE_NONE;
    event = bsc_event_init();
    zassert_not_null(event, NULL);

    // run child and wait when child running
    tid_child = k_thread_create(&thread, child_stack, STACKSIZE, &child_func,
        event, NULL, NULL, -1, K_USER | K_INHERIT_PERMS, K_NO_WAIT);
    zassert_not_null(tid_child, NULL);

    test_stage = STAGE_WAIT_1;
    bsc_event_wait(event);

    test_stage = STAGE_WAIT_2;
    bsc_event_wait(event);

    test_stage = STAGE_TIMEDWAIT_TIMEOUT;
    b = bsc_event_timedwait(event, TIMEOUT_MIN);
    zassert_false(b, NULL);

    test_stage = STAGE_TIMEDWAIT_OK;
    b = bsc_event_timedwait(event, TIMEOUT_MAX);
    zassert_true(b, NULL);

    t1 = k_uptime_get();
    bsc_wait(TIMEOUT_SLEEP);
    t2 = k_uptime_get();
    timediff = t2 - t1;
    zassert_true((WAITTIME_MIN < timediff) && (timediff < WAITTIME_MAX), NULL);

    k_thread_join(&thread, K_FOREVER);

    bsc_event_deinit(event);
}

static void thread_func(void *p1, void *p2, void *p3)
{
    BSC_EVENT *event = (BSC_EVENT *)p1;
    zassert_not_null(event, NULL);
    bsc_event_wait(event);
}

#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(bsc_event_test2, test_bsc_event2)
#else
static void test_bsc_event2(void)
#endif
{
    BSC_EVENT *event;
    k_tid_t tid_child[MULTIPLE_WAIT_THREADS_NUM];
    struct k_thread thread[MULTIPLE_WAIT_THREADS_NUM];
    int i;

    event = bsc_event_init();
    zassert_not_null(event, NULL);

    for(i=0; i<MULTIPLE_WAIT_THREADS_NUM; i++) {
        tid_child[i] = k_thread_create(&thread[i], child_stacks[i], STACKSIZE,
            &thread_func, event, NULL, NULL, -1, K_USER | K_INHERIT_PERMS,
            K_NO_WAIT);
        zassert_not_null(tid_child[i], NULL);
    }

    bsc_wait(1);
    bsc_event_signal(event);

    for(i=0; i<MULTIPLE_WAIT_THREADS_NUM; i++) {
        k_thread_join(&thread[i], K_FOREVER);
    }

    bsc_event_deinit(event);
}

typedef struct
{
    BSC_EVENT *event;
    bool result;
} test_param_t;

static void thread_func2(void *p1, void *p2, void *p3)
{
    test_param_t *p = (test_param_t *)p1;
    zassert_not_null(p->event, NULL);
    // use some big timeout value, 24 hours seems to be enough
    p->result = bsc_event_timedwait(p->event, 24*60*60*1000);
}

#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(bsc_event_test3, test_bsc_event3)
#else
static void test_bsc_event3(void)
#endif
{
    BSC_EVENT *event;
    k_tid_t tid[MULTIPLE_WAIT_THREADS_NUM];
    struct k_thread thread[MULTIPLE_WAIT_THREADS_NUM];
    test_param_t results[MULTIPLE_WAIT_THREADS_NUM];
    int i;

    event = bsc_event_init();
    zassert_not_null(event, NULL);

    for(i=0; i<MULTIPLE_WAIT_THREADS_NUM; i++) {
        results[i].event = event;
        results[i].result = false;
        tid[i] = k_thread_create(&thread[i], child_stacks[i], STACKSIZE,
            &thread_func2, &results[i], NULL, NULL, -1,
            K_USER | K_INHERIT_PERMS, K_NO_WAIT);
        zassert_not_null(tid[i], NULL);
    }

    bsc_wait(1);
    bsc_event_signal(event);

    for(i=0; i<MULTIPLE_WAIT_THREADS_NUM; i++) {
        k_thread_join(&thread[i], K_FOREVER);
    }

    for(i=0; i<MULTIPLE_WAIT_THREADS_NUM; i++) {
        zassert_equal(results[i].result == true, true, NULL);
    }

    bsc_event_deinit(event);
}

#if defined(CONFIG_ZTEST_NEW_API)
ZTEST_SUITE(bsc_event_test1, NULL, NULL, NULL, NULL, NULL);
ZTEST_SUITE(bsc_event_test2, NULL, NULL, NULL, NULL, NULL);
ZTEST_SUITE(bsc_event_test3, NULL, NULL, NULL, NULL, NULL);
#else
void test_main(void)
{
    ztest_test_suite(bsc_event_test1, ztest_unit_test(test_bsc_event1));
    ztest_test_suite(bsc_event_test2, ztest_unit_test(test_bsc_event2));
    ztest_test_suite(bsc_event_test3, ztest_unit_test(test_bsc_event3));
    ztest_run_test_suite(bsc_event_test1);
    ztest_run_test_suite(bsc_event_test2);
    ztest_run_test_suite(bsc_event_test3);
}
#endif
