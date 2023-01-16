/*
 * Copyright (c) 2020 Legrand North America, LLC.
 *
 * SPDX-License-Identifier: MIT
 */

/* @file
 * @brief test of runloop interface
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <pthread.h>
#include <ztest.h>
#include <bacnet/datalink/bsc/bsc-runloop.h>
#include <unistd.h>

#ifdef ZEPHYR_SLEEP
#define sleep(sec) k_msleep(1000 * (sec))
#define usleep k_usleep
#endif

static int runloop_counter = 0;

void runloop_func(void *ctx)
{
    runloop_counter++;
}

static void test_simple(void)
{
    BSC_SC_RET ret;
    BSC_RUNLOOP *rl;
    void* ctx = (void*) &ret;
    int old_counter;
    runloop_counter = 0;
    ret = bsc_runloop_start(bsc_global_runloop());
    zassert_equal(ret, BSC_SC_SUCCESS, NULL);
    ret = bsc_runloop_reg(bsc_global_runloop(), &ctx, runloop_func);
    sleep(4);
    zassert_equal(runloop_counter >=3, true, NULL);
    old_counter = runloop_counter;
    bsc_runloop_schedule(bsc_global_runloop());
    usleep(500);
    zassert_equal(runloop_counter - old_counter >= 1, true ,0 );
    bsc_runloop_unreg(bsc_global_runloop(),&ctx);
    bsc_runloop_stop(bsc_global_runloop());
    runloop_counter = 0;
    rl = bsc_local_runloop_alloc();
    ret = bsc_runloop_start(rl);
    zassert_equal(ret, BSC_SC_SUCCESS, NULL);
    ret = bsc_runloop_reg(rl, &ctx, runloop_func);
    sleep(4);
    zassert_equal(runloop_counter >=3, true, NULL);
    old_counter = runloop_counter;
    bsc_runloop_schedule(rl);
    usleep(100);
    zassert_equal(runloop_counter - old_counter, 1 ,0 );
    bsc_runloop_unreg(rl,&ctx);
    bsc_runloop_stop(rl);
    bsc_local_runloop_free(rl);
}

void test_main(void)
{
    // Tests must not be run in parallel threads!
    // Thats why tests functions are in different suites.

    ztest_test_suite(runloop_test_1, ztest_unit_test(test_simple));
    ztest_run_test_suite(runloop_test_1);
}