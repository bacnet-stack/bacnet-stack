/*
 * Copyright (c) 2020 Legrand North America, LLC.
 *
 * SPDX-License-Identifier: MIT
 */

/* @file
 * @brief test BACnet real value encode/decode APIs
 */

#include <zephyr/ztest.h>
#include <bacnet/bacreal.h>
#include <bacnet/bacdef.h>

/**
 * @addtogroup bacnet_tests
 * @{
 */

/**
 * @brief Test BACnet real data type (single precision floating point)
 */
static void testBACreal(void)
{
    float real_value = 3.14159F, test_real_value = 0.0;
    uint8_t apdu[MAX_APDU] = { 0 };
    int len = 0, test_len = 0;

    len = encode_bacnet_real(real_value, &apdu[0]);
    zassert_equal(len, 4, NULL);
    test_len = decode_real(&apdu[0], &test_real_value);
    zassert_equal(test_len, len, NULL);
    zassert_equal(test_real_value, real_value, NULL);
}

/**
 * @brief Test BACnet double data type (double precision floating point)
 */
static void testBACdouble(void)
{
    double double_value = 3.1415927, test_double_value = 0.0;
    uint8_t apdu[MAX_APDU] = { 0 };
    int len = 0, test_len = 0;

    len = encode_bacnet_double(double_value, &apdu[0]);
    zassert_equal(len, 8, NULL);
    test_len = decode_double(&apdu[0], &test_double_value);
    zassert_equal(test_len, len, NULL);
    zassert_equal(test_double_value, double_value, NULL);
}
/**
 * @}
 */


void test_main(void)
{
    ztest_test_suite(bacreal_tests,
     ztest_unit_test(testBACreal),
     ztest_unit_test(testBACdouble)
     );

    ztest_run_test_suite(bacreal_tests);
}
