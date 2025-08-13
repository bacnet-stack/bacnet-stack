/* @file
 * @brief tests the BACnet RAM File System (BSRAMFS)
 * @date August 2025
 * @author Steve Karg <Steve Karg <skarg@users.sourceforge.net>
 * @copyright SPDX-License-Identifier: MIT
 */
#include <limits.h>
#include <zephyr/ztest.h>
#include <bacnet/bacstr.h>
#include <bacnet/basic/sys/bsramfs.h>

/**
 * @addtogroup bacnet_tests
 * @{
 */

/**
 * @brief Unit Test for the BACnet static RAM File System (BSRAMFS)
 */
#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(bramfs_tests, test_BSRAMFS_stream)
#else
static void test_BSRAMFS_stream(void)
#endif
{
    struct bacnet_file_sramfs_data file_data[3] = {
        { 0, NULL, "testfile1.txt", NULL },
        { 0, NULL, "testfile2.txt", NULL },
        { 0, NULL, "testfile3.txt", NULL }
    };
    size_t file_size = 0, i = 0;
    uint8_t test_file_data[256] = { 0 };
    /* data less than 256 bytes */
    uint8_t file_data_1[] = {
        "This is a first test file for the BACnet RAM File System (BSRAMFS). "
        "It contains some sample data to be read and written."
    };
    uint8_t file_data_2[] = {
        "This is a second test file for the BACnet RAM File System (BSRAMFS). "
        "It contains some additional sample data to be read and written."
    };
    uint8_t file_data_3[] = { "Small file data" };

    /* Initialize the BSRAMFS */
    bacfile_sramfs_init();
    for (i = 0; i < ARRAY_SIZE(file_data); i++) {
        file_size = bacfile_sramfs_file_size(file_data[i].pathname);
        zassert_equal(
            file_size, 0, "File size should be 0 after initialization");
    }
    /* add static files to the file system */
    file_data[0].data = (char *)file_data_1;
    file_data[0].size = sizeof(file_data_1);
    zassert_true(
        bacfile_sramfs_add(&file_data[0]), "Failed to add file_data[0]");
    file_data[1].data = (char *)file_data_2;
    file_data[1].size = sizeof(file_data_2);
    zassert_true(
        bacfile_sramfs_add(&file_data[1]), "Failed to add file_data[1]");
    file_data[2].data = (char *)file_data_3;
    file_data[2].size = sizeof(file_data_3);
    zassert_true(
        bacfile_sramfs_add(&file_data[2]), "Failed to add file_data[2]");
    /* read back the files and check the data */
    file_size = bacfile_sramfs_read_stream_data(
        file_data[0].pathname, 0, test_file_data, sizeof(test_file_data));
    zassert_equal(file_size, sizeof(file_data_1), "file_size=%zu", file_size);
    zassert_true(
        memcmp(test_file_data, file_data_1, sizeof(file_data_1)) == 0,
        "File data should match!");
}

/**
 * @brief Unit Test for the BACnet RAM File System (BSRAMFS)
 */
#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(bramfs_tests, test_BSRAMFS_records)
#else
static void test_BSRAMFS_records(void)
#endif
{
    struct bacnet_file_sramfs_data file_data[1] = {
        { 0,
          "This is the first record in the file.\0"
          "This is the second record in the file.\0"
          "This is the third record in the file.",
          "testfile.txt", NULL }
    };
    bool status = false;
    const char *pathname = file_data[0].pathname;
    char record_1[MAX_OCTET_STRING_BYTES] = { 0 };
    char record_2[MAX_OCTET_STRING_BYTES] = { 0 };
    char record_3[MAX_OCTET_STRING_BYTES] = { 0 };

    /* Initialize the BSRAMFS */
    bacfile_sramfs_init();

    /* no data in the file - expect failure */
    status = bacfile_sramfs_read_record_data(
        pathname, 0, 0, (uint8_t *)record_1, sizeof(record_1));
    zassert_false(status, "Read record 1 should fail on empty file");

    status = bacfile_sramfs_read_record_data(
        pathname, 0, 1, (uint8_t *)record_2, sizeof(record_2));
    zassert_false(status, "Read record 2 should fail on empty file");

    status = bacfile_sramfs_read_record_data(
        pathname, 0, 2, (uint8_t *)record_3, sizeof(record_3));
    zassert_false(status, "Read record 3 should fail on empty file");

    /* add the static file with records */
    bacfile_sramfs_add(&file_data[0]);

    /* read the first record */
    status = bacfile_sramfs_read_record_data(
        pathname, 0, 0, (uint8_t *)record_1, sizeof(record_1));
    zassert_true(status, "Read record 1 should succeed");
    /* read the second record */
    status = bacfile_sramfs_read_record_data(
        pathname, 0, 1, (uint8_t *)record_2, sizeof(record_2));
    zassert_true(status, "Read record 2 should succeed");
    /* read the third record */
    status = bacfile_sramfs_read_record_data(
        pathname, 0, 2, (uint8_t *)record_3, sizeof(record_3));
    zassert_true(status, "Read record 3 should succeed");
}

/**
 * @}
 */

#if defined(CONFIG_ZTEST_NEW_API)
ZTEST_SUITE(bramfs_tests, NULL, NULL, NULL, NULL, NULL);
#else
void test_main(void)
{
    ztest_test_suite(
        bramfs_tests, ztest_unit_test(test_BSRAMFS_stream),
        ztest_unit_test(test_BSRAMFS_records));

    ztest_run_test_suite(bramfs_tests);
}
#endif
