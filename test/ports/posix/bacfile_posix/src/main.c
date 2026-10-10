/**
 * @file
 * @brief Validation tests for POSIX BACnet File Object backend
 * @copyright SPDX-License-Identifier: MIT
 */
#include <zephyr/ztest.h>
#include <unistd.h>
#include <string.h>
#include <stdint.h>

#include <bacfile-posix.h>

static const char *test_file = "bacfile_posix_validation.tmp";

static void remove_test_file(void)
{
    (void)unlink(test_file);
}

static void test_stream_read_write_append(void)
{
    const uint8_t data1[] = { 'A', 'B', 'C' };
    const uint8_t data2[] = { 'X', 'Y' };
    const uint8_t data3[] = { 'Z' };
    uint8_t buffer[8] = { 0 };
    size_t len = 0;

    remove_test_file();

    len = bacfile_posix_write_stream_data(test_file, 0, data1, sizeof(data1));
    zassert_equal(len, sizeof(data1), NULL);

    len = bacfile_posix_write_stream_data(test_file, 5, data2, sizeof(data2));
    zassert_equal(len, sizeof(data2), NULL);

    zassert_equal(bacfile_posix_file_size(test_file), 7U, NULL);

    len = bacfile_posix_read_stream_data(test_file, 0, buffer, sizeof(buffer));
    zassert_equal(len, 7U, NULL);
    zassert_equal(buffer[0], 'A', NULL);
    zassert_equal(buffer[1], 'B', NULL);
    zassert_equal(buffer[2], 'C', NULL);
    zassert_equal(buffer[5], 'X', NULL);
    zassert_equal(buffer[6], 'Y', NULL);

    len = bacfile_posix_write_stream_data(test_file, -1, data3, sizeof(data3));
    zassert_equal(len, sizeof(data3), NULL);

    zassert_equal(bacfile_posix_file_size(test_file), 8U, NULL);

    memset(buffer, 0, sizeof(buffer));
    len = bacfile_posix_read_stream_data(test_file, 7, buffer, 1);
    zassert_equal(len, 1U, NULL);
    zassert_equal(buffer[0], 'Z', NULL);

    remove_test_file();
}

static void test_file_size_set_grow_and_shrink(void)
{
    const uint8_t data[] = { 1U, 2U, 3U, 4U };
    uint8_t zeros[6] = { 0 };
    size_t len = 0;
    size_t i = 0;

    remove_test_file();

    len = bacfile_posix_write_stream_data(test_file, 0, data, sizeof(data));
    zassert_equal(len, sizeof(data), NULL);
    zassert_equal(bacfile_posix_file_size(test_file), sizeof(data), NULL);

    zassert_true(bacfile_posix_file_size_set(test_file, 10U), NULL);
    zassert_equal(bacfile_posix_file_size(test_file), 10U, NULL);

    len = bacfile_posix_read_stream_data(test_file, 4, zeros, sizeof(zeros));
    zassert_equal(len, sizeof(zeros), NULL);
    for (i = 0; i < sizeof(zeros); i++) {
        zassert_equal(zeros[i], 0U, NULL);
    }

    zassert_true(bacfile_posix_file_size_set(test_file, 2U), NULL);
    zassert_equal(bacfile_posix_file_size(test_file), 2U, NULL);

    memset(zeros, 0, sizeof(zeros));
    len = bacfile_posix_read_stream_data(test_file, 0, zeros, sizeof(zeros));
    zassert_equal(len, 2U, NULL);
    zassert_equal(zeros[0], 1U, NULL);
    zassert_equal(zeros[1], 2U, NULL);

    remove_test_file();
}

static void test_invalid_parameters(void)
{
    const uint8_t data[] = { 9U };
    uint8_t out[] = { 0U };

    remove_test_file();

    zassert_equal(
        bacfile_posix_write_stream_data(test_file, -2, data, sizeof(data)), 0U,
        NULL);
    zassert_equal(
        bacfile_posix_read_stream_data(test_file, -1, out, sizeof(out)), 0U,
        NULL);

    zassert_equal(bacfile_posix_file_size(NULL), 0U, NULL);
    zassert_false(bacfile_posix_file_size_set(NULL, 1U), NULL);
    zassert_equal(
        bacfile_posix_write_stream_data(NULL, 0, data, sizeof(data)), 0U, NULL);
    zassert_equal(
        bacfile_posix_read_stream_data(NULL, 0, out, sizeof(out)), 0U, NULL);

    remove_test_file();
}

static void test_record_read_write(void)
{
    const uint8_t record1[] = "record-1\n";
    const uint8_t record2[] = "record-2\n";
    const uint8_t record2_updated[] = "record-2-upd\n";
    uint8_t buffer[32] = { 0 };

    remove_test_file();

    zassert_true(
        bacfile_posix_write_record_data(
            test_file, 0, 0, record1, sizeof(record1) - 1),
        NULL);
    zassert_true(
        bacfile_posix_write_record_data(
            test_file, -1, 0, record2, sizeof(record2) - 1),
        NULL);

    memset(buffer, 0, sizeof(buffer));
    zassert_true(
        bacfile_posix_read_record_data(
            test_file, 1, 0, buffer, sizeof(record1) - 1),
        NULL);
    zassert_mem_equal(buffer, record1, sizeof(record1) - 1, NULL);

    memset(buffer, 0, sizeof(buffer));
    zassert_true(
        bacfile_posix_read_record_data(
            test_file, 2, 0, buffer, sizeof(record2) - 1),
        NULL);
    zassert_mem_equal(buffer, record2, sizeof(record2) - 1, NULL);

    zassert_true(
        bacfile_posix_write_record_data(
            test_file, 1, 0, record2_updated, sizeof(record2_updated) - 1),
        NULL);

    memset(buffer, 0, sizeof(buffer));
    zassert_true(
        bacfile_posix_read_record_data(
            test_file, 2, 0, buffer, sizeof(record2_updated) - 1),
        NULL);
    zassert_mem_equal(
        buffer, record2_updated, sizeof(record2_updated) - 1, NULL);

    zassert_false(
        bacfile_posix_read_record_data(
            test_file, -1, 0, buffer, sizeof(buffer)),
        NULL);
    zassert_false(
        bacfile_posix_read_record_data(NULL, 1, 0, buffer, sizeof(buffer)),
        NULL);
    zassert_false(
        bacfile_posix_write_record_data(
            NULL, 1, 0, record1, sizeof(record1) - 1),
        NULL);

    remove_test_file();
}

void test_main(void)
{
    ztest_test_suite(
        bacfile_posix_tests, ztest_unit_test(test_stream_read_write_append),
        ztest_unit_test(test_file_size_set_grow_and_shrink),
        ztest_unit_test(test_invalid_parameters),
        ztest_unit_test(test_record_read_write));

    ztest_run_test_suite(bacfile_posix_tests);
}
