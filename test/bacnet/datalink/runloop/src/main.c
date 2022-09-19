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

static int runloop_counter = 0;

void runloop_func(BSC_SOCKET_CTX *ctx)
{
    runloop_counter++;
}

static void test_simple(void)
{
    BSC_SC_RET ret;
    BSC_SOCKET_CTX ctx;
    int old_counter;
    runloop_counter = 0;
    ret = bsc_runloop_start();
    zassert_equal(ret, BSC_SC_SUCCESS, NULL);
    ret = bsc_runloop_reg(&ctx, runloop_func);
    sleep(4);
    zassert_equal(runloop_counter >=3, true, NULL);
    old_counter = runloop_counter;
    bsc_runloop_schedule();
    usleep(100);
    zassert_equal(runloop_counter - old_counter, 1 ,0 );
    bsc_runloop_unreg(&ctx);
    bsc_runloop_stop();
}

void test_main(void)
{
    // Tests must not be run in parallel threads!
    // Thats why tests functions are in different suites.


    ztest_test_suite(runloop_test_1, ztest_unit_test(test_simple));
    ztest_run_test_suite(runloop_test_1);
}