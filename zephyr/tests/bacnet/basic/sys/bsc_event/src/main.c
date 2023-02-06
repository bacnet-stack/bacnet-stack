/*
 * Copyright (c) 2022 Legrand North America, LLC.
 *
 * SPDX-License-Identifier: MIT
 */

/* @file
 * @brief test of bsc-event interface
 */

#include <zephyr/kernel.h>
#include <ztest.h>
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

#define STACKSIZE 128
K_KERNEL_STACK_DEFINE(child_stack, STACKSIZE);

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

static void test_bsc_event(void)
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

    bsc_event_reset(event);
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

void test_main(void)
{
    ztest_test_suite(bsc_event_test, ztest_unit_test(test_bsc_event));
    ztest_run_test_suite(bsc_event_test);
}
