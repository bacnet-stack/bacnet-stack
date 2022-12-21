/*
 * Copyright (c) 2022 Legrand North America, LLC.
 *
 * SPDX-License-Identifier: MIT
*/

/* @file
 * @brief test of bsc-mutex interface
*/

#define _GNU_SOURCE
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

static void *child_func(void* arg)
{
    BSC_MUTEX *mutex = (BSC_MUTEX *)arg;
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
  
    return NULL;
}

static void test_bsc_mutex(void)
{
    BSC_MUTEX *mutex;
    pthread_t tid_child;
    pthread_mutex_t *m;

    child_running = false;
    counter = 0;

    mutex = bsc_mutex_init();
    zassert_not_null(mutex, NULL);

    // check: is this real mutex
    m = (pthread_mutex_t*)bsc_mutex_native(mutex);
    zassert_not_null(m, NULL);
    zassert_equal(pthread_mutex_lock(m), 0, NULL);
    zassert_equal(pthread_mutex_unlock(m), 0, NULL);

    // run child and wait when child running
    zassert_equal(
        pthread_create(&tid_child, NULL, &child_func, mutex), 0, NULL);
    while (!child_running) {
        usleep(10);
    }

    bsc_mutex_lock(mutex);
    zassert_equal(counter, 20, NULL);
    bsc_mutex_unlock(mutex);

    pthread_join(tid_child, NULL);
    bsc_mutex_deinit(mutex);
}

static void *child_func2(void* arg)
{
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
    pthread_t tid_child;

    child_running = false;
    counter = 0;

    // run child and wait when child running
    zassert_equal(
        pthread_create(&tid_child, NULL, &child_func2, NULL), 0, NULL);
    while (!child_running) {
        usleep(10);
    }

    bsc_global_mutex_lock();
    zassert_equal(counter, 20, NULL);
    bsc_global_mutex_unlock();

    pthread_join(tid_child, NULL);
}

void test_main(void)
{
    ztest_test_suite(bsc_mutex_test,
        ztest_unit_test(test_bsc_mutex),
        ztest_unit_test(test_bsc_mutex_global));
    ztest_run_test_suite(bsc_mutex_test);
}
