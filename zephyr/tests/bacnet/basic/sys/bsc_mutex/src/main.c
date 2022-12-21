/*
 * Copyright (c) 2022 Legrand North America, LLC.
 *
 * SPDX-License-Identifier: MIT
*/

/* @file
 * @brief test of bsc-mutex interface
*/

#include <zephyr.h>
#include "ztest.h"
#include "bacnet/datalink/bsc/bsc-mutex.h"

static bool child_running = false;
static int counter = 0;

#define MUTEX_RECURSIVE_DEEP    10

#define STACKSIZE 128
K_KERNEL_STACK_DEFINE(child_stack, STACKSIZE);

static void child_func(void *p1, void *p2, void *p3)
{
    BSC_MUTEX *mutex = (BSC_MUTEX *)p1;
    unsigned long i;

    zassert_not_null(mutex, NULL);

    bsc_mutex_lock(mutex);
    child_running = true;
    k_usleep(100);

     for (i = 0; i < MUTEX_RECURSIVE_DEEP; ++i) {
         bsc_mutex_lock(mutex);
         counter++;
         k_usleep(100);
    }

     for (i = 0; i < MUTEX_RECURSIVE_DEEP; ++i) {
         bsc_mutex_unlock(mutex);
         counter++;
         k_usleep(100);
    }

    bsc_mutex_unlock(mutex);
}

static void test_bsc_mutex(void)
{
    BSC_MUTEX *mutex;
    k_tid_t tid_child;
    struct k_thread thread;
    struct k_mutex *m;

    child_running = false;
    counter = 0;

    mutex = bsc_mutex_init();
    zassert_not_null(mutex, NULL);

    // check: is this real mutex
    m = (struct k_mutex*)bsc_mutex_native(mutex);
    zassert_not_null(m, NULL);
    zassert_equal(k_mutex_lock(m, K_FOREVER), 0, NULL);
    zassert_equal(k_mutex_unlock(m), 0, NULL);

    // run child and wait when child running
    tid_child = k_thread_create(&thread, child_stack, STACKSIZE, &child_func,
        mutex, NULL, NULL, -1, K_USER | K_INHERIT_PERMS, K_NO_WAIT);
    zassert_not_null(tid_child, NULL);
    while (!child_running) {
        k_usleep(10);
    }

    bsc_mutex_lock(mutex);
    zassert_equal(counter, 20, NULL);
    bsc_mutex_unlock(mutex);

    k_thread_join(&thread, K_FOREVER);
    bsc_mutex_deinit(mutex);
}

static void child_func2(void *p1, void *p2, void *p3)
{
    unsigned long i;

    bsc_global_mutex_lock();
    child_running = true;
    k_usleep(100);

    for (i = 0; i < MUTEX_RECURSIVE_DEEP; ++i) {
        bsc_global_mutex_lock();
        counter++;
        k_usleep(100);
    }
    for (i = 0; i < MUTEX_RECURSIVE_DEEP; ++i) {
       bsc_global_mutex_unlock();
       counter++;
       k_usleep(100);
   }

   bsc_global_mutex_unlock();
}

static void test_bsc_mutex_global(void)
{
    k_tid_t tid_child;
    struct k_thread thread;

    child_running = false;
    counter = 0;

    // run child and wait when child running
    tid_child = k_thread_create(&thread, child_stack, STACKSIZE, &child_func2,
        NULL, NULL, NULL, -1, K_USER | K_INHERIT_PERMS, K_NO_WAIT);
    zassert_not_null(tid_child, NULL);
    while (!child_running) {
        k_usleep(10);
    }

    bsc_global_mutex_lock();
    zassert_equal(counter, 20, NULL);
    bsc_global_mutex_unlock();

    k_thread_join(&thread, K_FOREVER);
}

void test_main(void)
{
    ztest_test_suite(bsc_mutex_test,
        ztest_unit_test(test_bsc_mutex),
        ztest_unit_test(test_bsc_mutex_global));
    ztest_run_test_suite(bsc_mutex_test);
}
