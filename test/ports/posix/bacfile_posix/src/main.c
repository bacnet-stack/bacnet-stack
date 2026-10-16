/**
 * @file
 * @brief Validation tests for POSIX BACnet File Object backend
 * @copyright SPDX-License-Identifier: MIT
 */
#include <zephyr/ztest.h>
#include <unistd.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

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

static bool record_equals(int32_t start, const char *expected)
{
    uint8_t buffer[32] = { 0 };
    size_t len = strlen(expected);

    if (!bacfile_posix_read_record_data(
            test_file, start, 0, buffer, sizeof(buffer))) {
        return false;
    }
    return (strlen((const char *)buffer) == len) &&
        (memcmp(buffer, expected, len) == 0);
}

static bool record_write(int32_t start, const char *data)
{
    return bacfile_posix_write_record_data(
        test_file, start, 0, (const uint8_t *)data, strlen(data));
}

static void test_record_read_write(void)
{
    uint8_t buffer[32] = { 0 };

    remove_test_file();

    /* a missing backing file is created by the first write */
    zassert_true(record_write(0, "record-0"), NULL);
    zassert_true(record_write(-1, "record-1"), NULL);
    zassert_true(record_write(-1, "record-2"), NULL);
    /* same index is used to write and to read */
    zassert_true(record_equals(0, "record-0"), NULL);
    zassert_true(record_equals(1, "record-1"), NULL);
    zassert_true(record_equals(2, "record-2"), NULL);
    /* one past the last record is the end of the file */
    zassert_false(
        bacfile_posix_read_record_data(test_file, 3, 0, buffer, sizeof(buffer)),
        NULL);

    zassert_false(
        bacfile_posix_read_record_data(
            test_file, -1, 0, buffer, sizeof(buffer)),
        NULL);
    zassert_false(
        bacfile_posix_read_record_data(NULL, 1, 0, buffer, sizeof(buffer)),
        NULL);
    zassert_false(
        bacfile_posix_write_record_data(NULL, 1, 0, (const uint8_t *)"a", 1),
        NULL);
    zassert_false(record_write(-2, "a"), NULL);

    remove_test_file();
}

static void test_record_write_multiple_at_start(void)
{
    const char *records[] = { "one", "two", "three" };
    size_t i;

    remove_test_file();

    for (i = 0; i < 3; i++) {
        zassert_true(
            bacfile_posix_write_record_data(
                test_file, 0, i, (const uint8_t *)records[i],
                strlen(records[i])),
            NULL);
    }
    zassert_true(record_equals(0, "one"), NULL);
    zassert_true(record_equals(1, "two"), NULL);
    zassert_true(record_equals(2, "three"), NULL);

    /* writing one record at start 0 does not truncate the file */
    zassert_true(record_write(0, "uno"), NULL);
    zassert_true(record_equals(0, "uno"), NULL);
    zassert_true(record_equals(1, "two"), NULL);
    zassert_true(record_equals(2, "three"), NULL);

    remove_test_file();
}

static void test_record_empty_and_end_of_file(void)
{
    uint8_t buffer[8] = { 0 };

    remove_test_file();

    /* empty file has no records */
    zassert_equal(
        bacfile_posix_write_stream_data(test_file, 0, buffer, 0), 0U, NULL);
    zassert_false(
        bacfile_posix_read_record_data(test_file, 0, 0, buffer, sizeof(buffer)),
        NULL);
    remove_test_file();

    /* an empty record is a record, distinct from the end of the file */
    zassert_true(record_write(-1, "first"), NULL);
    zassert_true(record_write(-1, ""), NULL);
    zassert_true(record_write(-1, "third"), NULL);
    zassert_true(record_equals(1, ""), NULL);
    zassert_true(record_equals(2, "third"), NULL);
    zassert_false(
        bacfile_posix_read_record_data(test_file, 3, 0, buffer, sizeof(buffer)),
        NULL);

    /* a record cannot contain the line terminator */
    zassert_false(record_write(-1, "a\nb"), NULL);
    zassert_false(record_write(0, "a\nb"), NULL);
    zassert_false(record_write(-1, "a\n\n"), NULL);
    /* a single trailing newline is ignored */
    zassert_true(record_write(-1, "hello\n"), NULL);
    zassert_true(record_equals(3, "hello"), NULL);
    zassert_true(record_write(-1, "\n"), NULL);
    zassert_true(record_equals(4, ""), NULL);

    remove_test_file();
}

static void test_record_no_trailing_newline(void)
{
    const uint8_t data[] = { 'a', 'a', '\n', 'b', 'b' };
    uint8_t buffer[8] = { 0 };

    remove_test_file();

    zassert_equal(
        bacfile_posix_write_stream_data(test_file, 0, data, sizeof(data)),
        sizeof(data), NULL);
    /* last line without a newline is still a record */
    zassert_true(record_equals(0, "aa"), NULL);
    zassert_true(record_equals(1, "bb"), NULL);
    zassert_false(
        bacfile_posix_read_record_data(test_file, 2, 0, buffer, sizeof(buffer)),
        NULL);
    /* append does not join the new record to the last line */
    zassert_true(record_write(-1, "cc"), NULL);
    zassert_true(record_equals(1, "bb"), NULL);
    zassert_true(record_equals(2, "cc"), NULL);

    remove_test_file();
}

static void test_record_write_past_end(void)
{
    remove_test_file();

    zassert_true(record_write(-1, "aa"), NULL);
    /* start equal to the record count is a write at the end */
    zassert_true(record_write(1, "bb"), NULL);
    zassert_true(record_equals(1, "bb"), NULL);
    /* start past the end extends the file with empty records */
    zassert_true(record_write(4, "ee"), NULL);
    zassert_true(record_equals(2, ""), NULL);
    zassert_true(record_equals(3, ""), NULL);
    zassert_true(record_equals(4, "ee"), NULL);

    remove_test_file();
}

static void test_record_long_line(void)
{
    uint8_t data[600];
    uint8_t buffer[700] = { 0 };

    remove_test_file();

    memset(data, 'x', sizeof(data));
    zassert_true(
        bacfile_posix_write_record_data(test_file, -1, 0, data, sizeof(data)),
        NULL);
    zassert_true(record_write(-1, "next"), NULL);
    zassert_true(record_equals(1, "next"), NULL);
    zassert_true(
        bacfile_posix_read_record_data(test_file, 0, 0, buffer, sizeof(buffer)),
        NULL);
    zassert_mem_equal(buffer, data, sizeof(data), NULL);

    remove_test_file();
}

static void test_record_count(void)
{
    const uint8_t data[] = { 'a', '\n', '\n', 'b' };

    remove_test_file();

    zassert_equal(bacfile_posix_record_count(test_file), 0U, NULL);
    zassert_true(record_write(-1, "one"), NULL);
    zassert_equal(bacfile_posix_record_count(test_file), 1U, NULL);
    zassert_true(record_write(-1, ""), NULL);
    zassert_equal(bacfile_posix_record_count(test_file), 2U, NULL);
    zassert_true(record_write(3, "four"), NULL);
    zassert_equal(bacfile_posix_record_count(test_file), 4U, NULL);
    zassert_equal(bacfile_posix_record_count(NULL), 0U, NULL);

    remove_test_file();
    /* the last line without a newline is a record */
    zassert_equal(
        bacfile_posix_write_stream_data(test_file, 0, data, sizeof(data)),
        sizeof(data), NULL);
    zassert_equal(bacfile_posix_record_count(test_file), 3U, NULL);

    remove_test_file();
}

void test_main(void)
{
    ztest_test_suite(
        bacfile_posix_tests, ztest_unit_test(test_stream_read_write_append),
        ztest_unit_test(test_invalid_parameters),
        ztest_unit_test(test_record_read_write),
        ztest_unit_test(test_record_write_multiple_at_start),
        ztest_unit_test(test_record_empty_and_end_of_file),
        ztest_unit_test(test_record_no_trailing_newline),
        ztest_unit_test(test_record_write_past_end),
        ztest_unit_test(test_record_long_line),
        ztest_unit_test(test_record_count));

    ztest_run_test_suite(bacfile_posix_tests);
}
