/*
 * Copyright (c) 2020 Legrand North America, LLC.
 *
 * SPDX-License-Identifier: MIT
 */

/* @file
 * @brief test BACnet integer encode/decode APIs
 */

#include <zephyr/ztest.h>
#include <bacnet/basic/sys/sbuf.h>

/**
 * @addtogroup bacnet_tests
 * @{
 */

/**
 * @brief Test
 */
#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(sbuf_tests, testStaticBuffer)
#else
static void testStaticBuffer(void)
#endif
{
    STATIC_BUFFER sbuffer;
    char *data1 = "Joshua";
    char *data2 = "Anna";
    char *data3 = "Christopher";
    char *data4 = "Mary";
    char data_buffer[480] = "";
    char test_data_buffer[480] = "";
    char *data;
    unsigned count;

    sbuf_init(&sbuffer, NULL, 0);
    zassert_true(sbuf_empty(&sbuffer), NULL);
    zassert_equal(sbuf_data(&sbuffer), NULL, NULL);
    zassert_equal(sbuf_size(&sbuffer), 0, NULL);
    zassert_equal(sbuf_count(&sbuffer), 0, NULL);
    zassert_false(sbuf_append(&sbuffer, data1, strlen(data1)), NULL);

    sbuf_init(&sbuffer, data_buffer, sizeof(data_buffer));
    zassert_true(sbuf_empty(&sbuffer), NULL);
    zassert_equal(sbuf_data(&sbuffer), data_buffer, NULL);
    zassert_equal(sbuf_size(&sbuffer), sizeof(data_buffer), NULL);
    zassert_equal(sbuf_count(&sbuffer), 0, NULL);

    zassert_true(sbuf_append(&sbuffer, data1, strlen(data1)), NULL);
    zassert_true(sbuf_append(&sbuffer, data2, strlen(data2)), NULL);
    zassert_true(sbuf_append(&sbuffer, data3, strlen(data3)), NULL);
    zassert_true(sbuf_append(&sbuffer, data4, strlen(data4)), NULL);
    strcat(test_data_buffer, data1);
    strcat(test_data_buffer, data2);
    strcat(test_data_buffer, data3);
    strcat(test_data_buffer, data4);
    zassert_equal(sbuf_count(&sbuffer), strlen(test_data_buffer), NULL);

    data = sbuf_data(&sbuffer);
    count = sbuf_count(&sbuffer);
    zassert_equal(memcmp(data, test_data_buffer, count), 0, NULL);
    zassert_equal(count, strlen(test_data_buffer), NULL);

    zassert_true(sbuf_truncate(&sbuffer, 0), NULL);
    zassert_equal(sbuf_count(&sbuffer), 0, NULL);
    zassert_equal(sbuf_size(&sbuffer), sizeof(data_buffer), NULL);
    zassert_true(sbuf_append(&sbuffer, data4, strlen(data4)), NULL);
    data = sbuf_data(&sbuffer);
    count = sbuf_count(&sbuffer);
    zassert_equal(memcmp(data, data4, count), 0, NULL);
    zassert_equal(count, strlen(data4), NULL);

    return;
}
/**
 * @}
 */

#if defined(CONFIG_ZTEST_NEW_API)
ZTEST_SUITE(sbuf_tests, NULL, NULL, NULL, NULL, NULL);
#else
void test_main(void)
{
    ztest_test_suite(sbuf_tests, ztest_unit_test(testStaticBuffer));

    ztest_run_test_suite(sbuf_tests);
}
#endif
