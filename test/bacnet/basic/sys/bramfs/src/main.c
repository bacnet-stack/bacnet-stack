/* @file
 * @brief tests the BACnet RAM File System (BRAMFS)
 * @date August 2025
 * @author Steve Karg <Steve Karg <skarg@users.sourceforge.net>
 * @copyright SPDX-License-Identifier: MIT
 */
#include <limits.h>
#include <zephyr/ztest.h>
#include <bacnet/basic/sys/bramfs.h>

/**
 * @addtogroup bacnet_tests
 * @{
 */

/**
 * @brief Unit Test for the BACnet RAM File System (BRAMFS)
 */
#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(bramfs_tests, testBRAMFS)
#else
static void testBRAMFS(void)
#endif
{
    const char *pathname = "testfile.txt";
    size_t file_size = 0;
    uint8_t null_file_data[256] = { 0 };
    uint8_t test_file_data[256] = { 0 };
    /* data less than 256 bytes */
    uint8_t file_data[] = {
        "This is a test file for the BACnet RAM File System (BRAMFS). "
        "It contains some sample data to be read and written."
    };
    uint8_t file_data_2[] = {
        "This is a second test file for the BACnet RAM File System (BRAMFS). "
        "It contains some additional sample data to be read and written."
    };
    uint8_t file_data_small[] = { "Small file data" };
    int32_t fileStartPosition = 0;

    /* Initialize the BRAMFS */
    bacfile_ramfs_init();

    file_size = bacfile_ramfs_file_size(pathname);
    zassert_equal(file_size, 0, "File size should be 0 after initialization");
    zassert_true(
        bacfile_ramfs_file_size_set(pathname, sizeof(null_file_data)),
        "Failed to set file size");
    file_size = bacfile_ramfs_file_size(pathname);
    zassert_equal(
        file_size, sizeof(null_file_data),
        "File size should be 256 after setting");
    file_size = bacfile_ramfs_read_stream_data(
        pathname, 0, test_file_data, sizeof(test_file_data));
    zassert_equal(
        file_size, sizeof(test_file_data), "file_size=%zu", file_size);
    zassert_true(
        memcmp(test_file_data, null_file_data, sizeof(test_file_data)) == 0,
        "File data should be zeroed out initially");
    file_size = bacfile_ramfs_write_stream_data(
        pathname, 0, file_data, sizeof(file_data));
    zassert_equal(file_size, sizeof(file_data), "file_size=%zu", file_size);
    file_size = bacfile_ramfs_read_stream_data(
        pathname, 0, test_file_data, sizeof(test_file_data));
    zassert_equal(file_size, sizeof(file_data), "file_size=%zu", file_size);
    zassert_true(
        memcmp(test_file_data, file_data, sizeof(file_data)) == 0,
        "File data should match written data");
    /* append data to the end of the file */
    fileStartPosition = -1;
    file_size = bacfile_ramfs_write_stream_data(
        pathname, fileStartPosition, file_data_2, sizeof(file_data_2));
    zassert_equal(file_size, sizeof(file_data_2), "file_size=%zu", file_size);
    file_size = bacfile_ramfs_read_stream_data(
        pathname, sizeof(file_data), test_file_data, sizeof(test_file_data));
    zassert_equal(file_size, sizeof(file_data_2), "file_size=%zu", file_size);
    zassert_true(
        memcmp(test_file_data, file_data_2, sizeof(file_data_2)) == 0,
        "File data should match appended data");
    /* write a smaller file */
    fileStartPosition = 0;
    file_size = bacfile_ramfs_write_stream_data(
        pathname, fileStartPosition, file_data_small, sizeof(file_data_small));
    zassert_equal(
        file_size, sizeof(file_data_small), "file_size=%zu", file_size);
    file_size = bacfile_ramfs_read_stream_data(
        pathname, 0, test_file_data, sizeof(test_file_data));
    zassert_equal(
        file_size, sizeof(file_data_small), "file_size=%zu", file_size);
    zassert_true(
        memcmp(test_file_data, file_data_small, sizeof(file_data_small)) == 0,
        "File data should match smaller written data");
    file_size = bacfile_ramfs_file_size(pathname);
    zassert_equal(
        file_size, sizeof(file_data_small),
        "File size should be %u after shrinking", sizeof(file_data_small));
    /* shrink the file by writing zero at zero */
    zassert_true(
        bacfile_ramfs_file_size_set(pathname, 0),
        "Failed to set file size to 0");
    file_size = bacfile_ramfs_file_size(pathname);
    zassert_equal(file_size, 0, "File size should be 0 after shrinking");
}
/**
 * @}
 */

#if defined(CONFIG_ZTEST_NEW_API)
ZTEST_SUITE(bramfs_tests, NULL, NULL, NULL, NULL, NULL);
#else
void test_main(void)
{
    ztest_test_suite(bramfs_tests, ztest_unit_test(testBRAMFS));

    ztest_run_test_suite(bramfs_tests);
}
#endif
