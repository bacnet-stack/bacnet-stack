/*
 * Copyright (c) 2022 Legrand North America, LLC.
 *
 * SPDX-License-Identifier: MIT
 */

/* @file
 * @brief test of bsc-event interface
 */

#define _GNU_SOURCE
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <pthread.h>
#include <time.h>
#include <zephyr/ztest.h>
#include <bacnet/datalink/bsc/bsc-event.h>
#include <unistd.h>

typedef enum {
    STAGE_NONE,
    STAGE_WAIT_1,
    STAGE_WAIT_2,
    STAGE_TIMEDWAIT_TIMEOUT,
    STAGE_TIMEDWAIT_OK,
} TEST_STAGE;
static TEST_STAGE test_stage = STAGE_NONE;

#define TIMEOUT_CHILD 400
#define TIMEOUT_MIN 200
#define TIMEOUT_MAX 600
#define MSEC_PER_SEC 1000
#define USEC_PER_MSEC 1000000
#define TIMEOUT_SLEEP 2
#define WAITTIME_MIN (TIMEOUT_SLEEP * MSEC_PER_SEC - 100)
#define WAITTIME_MAX (TIMEOUT_SLEEP * MSEC_PER_SEC + 100)
#define MULTIPLE_WAIT_THREADS_NUM 50

static void *child_func(void *arg)
{
    BSC_EVENT *event = (BSC_EVENT *)arg;
    zassert_not_null(event, NULL);

    while (test_stage != STAGE_WAIT_1) {
        usleep(10);
    }
    bsc_event_signal(event);

    while (test_stage != STAGE_WAIT_2) {
        usleep(10);
    }
    bsc_event_signal(event);

    while (test_stage != STAGE_TIMEDWAIT_TIMEOUT) {
        usleep(10);
    }
    usleep(1000 * TIMEOUT_CHILD);
    bsc_event_signal(event);

    while (test_stage != STAGE_TIMEDWAIT_OK) {
        usleep(10);
    }
    usleep(1000 * TIMEOUT_CHILD);
    bsc_event_signal(event);

    return NULL;
}

static void test_bsc_event1(void)
{
    BSC_EVENT *event;
    pthread_t tid_child;
    bool b;
    struct timespec t1;
    struct timespec t2;
    int64_t timediff;

    test_stage = STAGE_NONE;
    event = bsc_event_init();
    zassert_not_null(event, NULL);

    // run child and wait when child running
    zassert_equal(
        pthread_create(&tid_child, NULL, &child_func, event), 0, NULL);

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

    clock_gettime(CLOCK_REALTIME, &t1);
    bsc_wait(TIMEOUT_SLEEP);
    clock_gettime(CLOCK_REALTIME, &t2);
    timediff = (t2.tv_sec * MSEC_PER_SEC + t2.tv_nsec / USEC_PER_MSEC) -
        (t1.tv_sec * MSEC_PER_SEC + t1.tv_nsec / USEC_PER_MSEC);
    zassert_true(timediff >= TIMEOUT_SLEEP * MSEC_PER_SEC, NULL);
    pthread_join(tid_child, NULL);
    bsc_event_deinit(event);
}

static void *thread_func(void *arg)
{
    BSC_EVENT *event = (BSC_EVENT *)arg;
    zassert_not_null(event, NULL);
    bsc_event_wait(event);
    return NULL;
}

static void test_bsc_event2(void)
{
    BSC_EVENT *event;
    pthread_t tid[MULTIPLE_WAIT_THREADS_NUM];
    int i;

    event = bsc_event_init();
    zassert_not_null(event, NULL);

    for (i = 0; i < MULTIPLE_WAIT_THREADS_NUM; i++) {
        zassert_equal(
            pthread_create(&tid[i], NULL, &thread_func, event), 0, NULL);
    }

    bsc_wait(1);
    bsc_event_signal(event);

    for (i = 0; i < MULTIPLE_WAIT_THREADS_NUM; i++) {
        pthread_join(tid[i], NULL);
    }

    bsc_event_deinit(event);
}

typedef struct {
    BSC_EVENT *event;
    bool result;
} test_param_t;

static void *thread_func2(void *arg)
{
    test_param_t *p = (test_param_t *)arg;
    zassert_not_null(p->event, NULL);
    // use some big timeout value, 24 hours seems to be enough
    p->result = bsc_event_timedwait(p->event, 24 * 60 * 60 * 1000);
    return NULL;
}

static void test_bsc_event3(void)
{
    BSC_EVENT *event;
    pthread_t tid[MULTIPLE_WAIT_THREADS_NUM];
    test_param_t results[MULTIPLE_WAIT_THREADS_NUM];
    int i;

    event = bsc_event_init();
    zassert_not_null(event, NULL);

    for (i = 0; i < MULTIPLE_WAIT_THREADS_NUM; i++) {
        results[i].event = event;
        results[i].result = false;
        zassert_equal(
            pthread_create(&tid[i], NULL, &thread_func2, &results[i]), 0, NULL);
    }

    bsc_wait(1);
    bsc_event_signal(event);

    for (i = 0; i < MULTIPLE_WAIT_THREADS_NUM; i++) {
        pthread_join(tid[i], NULL);
    }

    for (i = 0; i < MULTIPLE_WAIT_THREADS_NUM; i++) {
        zassert_equal(results[i].result == true, true, NULL);
    }

    bsc_event_deinit(event);
}

void test_main(void)
{
    ztest_test_suite(bsc_event_test1, ztest_unit_test(test_bsc_event1));
    ztest_test_suite(bsc_event_test2, ztest_unit_test(test_bsc_event2));
    ztest_test_suite(bsc_event_test3, ztest_unit_test(test_bsc_event3));
    ztest_run_test_suite(bsc_event_test1);
    ztest_run_test_suite(bsc_event_test2);
    ztest_run_test_suite(bsc_event_test3);
}
