/**
 * @file
 * @brief test POSIX filename API
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date 2007
 * @copyright SPDX-License-Identifier: MIT
 */
#include <zephyr/ztest.h>
#include <bacnet/basic/sys/filename.h>

/**
 * @addtogroup bacnet_tests
 * @{
 */

/**
 * @brief Test
 */
#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(filename_tests, testFilename)
#else
static void testFilename(void)
#endif
{
    const char *data1 = "c:\\Joshua\\run";
    const char *data2 = "/home/Anna/run";
    const char *data3 = "c:\\Program Files\\Christopher\\run.exe";
    const char *data4 = "//Mary/data/run";
    const char *data5 = "bin\\run";
    const char *data6 = "run.exe";
    const char *filename = NULL;

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
    filename = filename_remove_path(data6);
    zassert_equal(strcmp("run.exe", filename), 0, NULL);

    return;
}

#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(filename_tests, testFilenameValid)
#else
static void testFilenameValid(void)
#endif
{
    const char *data0 = "";
    const char *data1 = "c:\\Joshua\\run";
    const char *data2 = "/home/Anna/run";
    const char *data3 = "c:\\Program Files\\Christopher\\run.exe";
    const char *data4 = "//Mary/data/run";
    const char *data5 = "bin\\\\run";
    const char *data6 = "bin/./run";
    const char *data7 = "bin/../run";
    const char *data_valid = "certs/mycert.pem";
    bool valid = false;

    valid = filename_path_valid(NULL);
    zassert_false(valid, NULL);
    valid = filename_path_valid(data0);
    zassert_false(valid, NULL);
    valid = filename_path_valid(data1);
    zassert_false(valid, NULL);
    valid = filename_path_valid(data2);
    zassert_false(valid, NULL);
    valid = filename_path_valid(data3);
    zassert_false(valid, NULL);
    valid = filename_path_valid(data4);
    zassert_false(valid, NULL);
    valid = filename_path_valid(data5);
    zassert_false(valid, NULL);
    valid = filename_path_valid(data6);
    zassert_false(valid, NULL);
    valid = filename_path_valid(data7);
    zassert_false(valid, NULL);
    valid = filename_path_valid(data_valid);
    zassert_true(valid, NULL);

    return;
}

/**
 * @}
 */

#if defined(CONFIG_ZTEST_NEW_API)
ZTEST_SUITE(filename_tests, NULL, NULL, NULL, NULL, NULL);
#else
void test_main(void)
{
    ztest_test_suite(
        filename_tests, ztest_unit_test(testFilename),
        ztest_unit_test(testFilenameValid));

    ztest_run_test_suite(filename_tests);
}
#endif
