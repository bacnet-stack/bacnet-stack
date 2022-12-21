/*
 * Copyright (c) 2022 Legrand North America, LLC.
 *
 * SPDX-License-Identifier: MIT
 */

/* @file
* @brief test of bsc-mutex interface
*/

#include "stdlib.h"
#include "string.h"
#include "stdio.h"
#include "pthread.h"
#include "ztest.h"
#include "bacnet/datalink/bsc/bsc-mutex.h"
#include "unistd.h"

static bool child_running = false;
static int counter = 0;

#define MUTEX_RECURSIVE_DEEP    10

DWORD WINAPI child_func( LPVOID lpParam )
{
    BSC_MUTEX *mutex = (BSC_MUTEX *)lpParam;
    unsigned long i;

    zassert_not_null(mutex, NULL);

    bsc_mutex_lock(mutex);
    child_running = true;
    usleep(100);

     for (i = 0; i < MUTEX_RECURSIVE_DEEP; ++i) {
         bsc_mutex_lock(mutex);
         counter++;
         usleep(100);
    }

     for (i = 0; i < MUTEX_RECURSIVE_DEEP; ++i) {
         bsc_mutex_unlock(mutex);
         counter++;
         usleep(100);
    }

    bsc_mutex_unlock(mutex);
  
    return 0;
}

static void test_bsc_mutex(void)
{
    BSC_MUTEX *mutex;
    HANDLE thread;
    DWORD threadID;
    HANDLE m;

    child_running = false;
    counter = 0;

    mutex = bsc_mutex_init();
    zassert_not_null(mutex, NULL);

    // check: is this real mutex
    m = (HANDLE)bsc_mutex_native(mutex);
    zassert_not_null(m, NULL);
    zassert_equal(WaitForSingleObject(m, INFINITE), WAIT_OBJECT_0, NULL);
    zassert_equal(ReleaseMutex(m), 0, NULL);

    // run child and wait when child running
    thread = CreateThread( 
                     NULL,       // default security attributes
                     0,          // default stack size
                     (LPTHREAD_START_ROUTINE) child_func,
                     mutex,
                     0,          // default creation flags
                     &ThreadID); // receive thread identifier
    zassert_not_null(thread, NULL);
    while (!child_running) {
        usleep(10);
    }

    bsc_mutex_lock(mutex);
    zassert_equal(counter, 20, NULL);
    bsc_mutex_unlock(mutex);

    WaitForSingleObject(thread, INFINITE);
    CloseHandle(thread);

    bsc_mutex_deinit(mutex);
}

DWORD WINAPI child_func2( LPVOID lpParam )
{
    UNUSED(lpParam)
    unsigned long i;

    bsc_global_mutex_lock();
    child_running = true;
    usleep(100);

    for (i = 0; i < MUTEX_RECURSIVE_DEEP; ++i) {
        bsc_global_mutex_lock();
        counter++;
        usleep(100);
    }
    for (i = 0; i < MUTEX_RECURSIVE_DEEP; ++i) {
       bsc_global_mutex_unlock();
       counter++;
       usleep(100);
   }

   bsc_global_mutex_unlock();
 
   return NULL;
}

static void test_bsc_mutex_global(void)
{
    HANDLE thread;
    DWORD threadID;

    child_running = false;
    counter = 0;

    // run child and wait when child running
    thread = CreateThread( 
                     NULL,       // default security attributes
                     0,          // default stack size
                     (LPTHREAD_START_ROUTINE) child_func2,
                     NULL,
                     0,          // default creation flags
                     &ThreadID); // receive thread identifier
    zassert_not_null(thread, NULL);
    while (!child_running) {
        usleep(10);
    }

    bsc_global_mutex_lock();
    zassert_equal(counter, 20, NULL);
    bsc_global_mutex_unlock();

    WaitForSingleObject(thread, INFINITE);
    CloseHandle(thread);
}

void test_main(void)
{
    ztest_test_suite(bsc_mutex_test,
        ztest_unit_test(test_bsc_mutex),
        ztest_unit_test(test_bsc_mutex_global));
    ztest_run_test_suite(bsc_mutex_test);
}
