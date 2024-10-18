/*
 * Copyright (c) 2022 Legrand North America, LLC.
 *
 * SPDX-License-Identifier: MIT
 */

/* @file
 * @brief test of bsc-event interface
 */

#include <windows.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
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

#define TIMEOUT_CHILD 400
#define TIMEOUT_MIN 200
#define TIMEOUT_MAX 600
#define TIMEOUT_SLEEP 2
#define WAITTIME_MIN (TIMEOUT_SLEEP * 1000 - 20)
#define WAITTIME_MAX (TIMEOUT_SLEEP * 1000 + 20)
#define MULTIPLE_WAIT_THREADS_NUM 50

DWORD WINAPI child_func(LPVOID lpParam)
{
    BSC_EVENT *event = (BSC_EVENT *)lpParam;
    zassert_not_null(event, NULL);

    while (test_stage != STAGE_WAIT_1) {
        Sleep(1);
    }
    bsc_event_signal(event);

    while (test_stage != STAGE_WAIT_2) {
        Sleep(1);
    }
    bsc_event_signal(event);

    while (test_stage != STAGE_TIMEDWAIT_TIMEOUT) {
        Sleep(1);
    }
    Sleep(TIMEOUT_CHILD);
    bsc_event_signal(event);

    while (test_stage != STAGE_TIMEDWAIT_OK) {
        Sleep(1);
    }

    Sleep(TIMEOUT_CHILD);
    bsc_event_signal(event);

    return 0;
}

static void test_bsc_event1(void)
{
    BSC_EVENT *event;
    HANDLE thread;
    DWORD threadID;
    bool b;
    ULONGLONG time1;
    ULONGLONG time2;
    ULONGLONG timediff;

    test_stage = STAGE_NONE;
    event = bsc_event_init();
    zassert_not_null(event, NULL);

    // run child and wait when child running
    thread = CreateThread(
        NULL, // default security attributes
        0, // default stack size
        (LPTHREAD_START_ROUTINE)child_func, event,
        0, // default creation flags
        &threadID); // receive thread identifier
    zassert_not_null(thread, NULL);

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

    time1 = GetTickCount64();
    bsc_wait(TIMEOUT_SLEEP);
    time2 = GetTickCount64();
    timediff = time2 - time1;
    zassert_true((WAITTIME_MIN < timediff) && (timediff < WAITTIME_MAX), NULL);

    WaitForSingleObject(thread, INFINITE);
    CloseHandle(thread);
    bsc_event_deinit(event);
}

DWORD WINAPI thread_func(LPVOID lpParam)
{
    BSC_EVENT *event = (BSC_EVENT *)lpParam;
    zassert_not_null(event, NULL);
    bsc_event_wait(event);
    return 0;
}

static void test_bsc_event2(void)
{
    int i;
    BSC_EVENT *event;
    DWORD tid[MULTIPLE_WAIT_THREADS_NUM];
    HANDLE thread[MULTIPLE_WAIT_THREADS_NUM];

    event = bsc_event_init();
    zassert_not_null(event, NULL);

    for (i = 0; i < MULTIPLE_WAIT_THREADS_NUM; i++) {
        thread[i] = CreateThread(
            NULL, // default security attributes
            0, // default stack size
            (LPTHREAD_START_ROUTINE)thread_func, event,
            0, // default creation flags
            &tid[i]); // receive thread identifier
        zassert_not_null(thread[i], NULL);
    }

    bsc_wait(1);
    bsc_event_signal(event);

    for (i = 0; i < MULTIPLE_WAIT_THREADS_NUM; i++) {
        WaitForSingleObject(thread[i], INFINITE);
        CloseHandle(thread[i]);
    }

    bsc_event_deinit(event);
}

void test_main(void)
{
    ztest_test_suite(bsc_event_test1, ztest_unit_test(test_bsc_event1));
    ztest_test_suite(bsc_event_test2, ztest_unit_test(test_bsc_event2));
    ztest_run_test_suite(bsc_event_test1);
    ztest_run_test_suite(bsc_event_test2);
}
