/*
 * Copyright (c) 2020 Legrand North America, LLC.
 *
 * SPDX-License-Identifier: MIT
 */

/* @file
 * @brief test BACnet integer encode/decode APIs
 */

#include <zephyr/ztest.h>
#include <bacnet/memcopy.h>

/**
 * @addtogroup bacnet_tests
 * @{
 */

/**
 * @brief Test
 */
#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(memcopy_tests, test_memcopy)
#else
static void test_memcopy(void)
#endif
{
    char *data1 = "Joshua";
    char *data2 = "Anna";
    char buffer[480] = "";
    char big_buffer[480] = "";
    size_t len = 0;

    len = memcopy(&buffer[0], &data1[0], 0, sizeof(data1), sizeof(buffer));
    zassert_equal(len, sizeof(data1), NULL);
    zassert_equal(memcmp(&buffer[0], &data1[0], len), 0, NULL);
    len = memcopy(&buffer[0], &data2[0], len, sizeof(data2), sizeof(buffer));
    zassert_equal(len, sizeof(data2), NULL);
    len = memcopy(
        &buffer[0], &big_buffer[0], 1, sizeof(big_buffer), sizeof(buffer));
    zassert_equal(len, 0, NULL);
}
/**
 * @}
 */

#if defined(CONFIG_ZTEST_NEW_API)
ZTEST_SUITE(memcopy_tests, NULL, NULL, NULL, NULL, NULL);
#else
void test_main(void)
{
    ztest_test_suite(memcopy_tests, ztest_unit_test(test_memcopy));

    ztest_run_test_suite(memcopy_tests);
}
#endif
