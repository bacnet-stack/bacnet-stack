/*
 * Copyright (c) 2020 Legrand North America, LLC.
 *
 * SPDX-License-Identifier: MIT
 */

/* @file
 * @brief test BACnet integer encode/decode APIs
 */

#include <ztest.h>
#include <bacnet/basic/sys/filename.h>

/**
 * @addtogroup bacnet_tests
 * @{
 */

/**
 * @brief Test
 */
static void testFilename(void)
{
    char *data1 = "c:\\Joshua\\run";
    char *data2 = "/home/Anna/run";
    char *data3 = "c:\\Program Files\\Christopher\\run.exe";
    char *data4 = "//Mary/data/run";
    char *data5 = "bin\\run";
    char *filename = NULL;

    filename = filename_remove_path(data1);
    zassert_equal(strcmp("run", filename), 0, NULL);
    filename = filename_remove_path(data2);
    zassert_equal(strcmp("run", filename), 0, NULL);
    filename = filename_remove_path(data3);
    zassert_equal(strcmp("run.exe", filename), 0, NULL);
    filename = filename_remove_path(data4);
    zassert_equal(strcmp("run", filename), 0, NULL);
    filename = filename_remove_path(data5);
    zassert_equal(strcmp("run", filename), 0, NULL);

    return;
}
/**
 * @}
 */


void test_main(void)
{
    ztest_test_suite(filename_tests,
     ztest_unit_test(testFilename)
     );

    ztest_run_test_suite(filename_tests);
}
