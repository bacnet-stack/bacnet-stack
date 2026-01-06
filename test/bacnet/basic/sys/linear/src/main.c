/**
 * @file
 * @brief test linear interpolation APIs
 * @date 2010
 *
 * @section LICENSE
 * Copyright (c) 2010 Steve Karg <skarg@users.sourceforge.net>
 *
 * @copyright SPDX-License-Identifier: MIT
 */
#include <zephyr/ztest.h>
#include <bacnet/basic/sys/linear.h>

/**
 * @addtogroup bacnet_tests
 * @{
 */

/**
 * Unit Test for linear interpolation of floating point values, rounded
 */
void testLinearInterpolateRound(void)
{
    uint16_t x2 = 0;
    uint16_t y1 = 0;
    uint16_t y2 = 0;
    uint16_t y3 = 0;
    uint16_t x2_test = 0;

    y2 = linear_interpolate_round(1, 1, 65535, 1, 100);
    zassert_equal(y2, 1, NULL);
    y2 = linear_interpolate_round(1, 1, 65535, 100, 1);
    zassert_equal(y2, 100, NULL);

    y2 = linear_interpolate_round(1, 65535, 65535, 1, 100);
    zassert_equal(y2, 100, NULL);
    y2 = linear_interpolate_round(1, 65535, 65535, 100, 1);
    zassert_equal(y2, 1, NULL);

    y2 = linear_interpolate_round(1, (65535 / 2), 65535, 1, 100);
    zassert_equal(y2, 50, NULL);

    y2 = linear_interpolate_round(1, (65535 / 4), 65535, 1, 100);
    zassert_equal(y2, 26, NULL);

    y2 = linear_interpolate_round(1, ((65535 * 3) / 4), 65535, 1, 100);
    zassert_equal(y2, 75, NULL);

    y2 = linear_interpolate_round(1, 1, 100, 1, 65535);
    zassert_equal(y2, 1, NULL);

    y2 = linear_interpolate_round(1, 100, 100, 1, 65535);
    zassert_equal(y2, 65535, NULL);

    y2 = linear_interpolate_round(1, 100 / 2, 100, 1, 65535);
    zassert_equal(y2, 32437, NULL);

    /* scaling from percent to steps and back */
    for (x2 = 1; x2 <= 100; x2++) {
        y2 = linear_interpolate_round(1, x2, 100, 1, 65535);
        x2_test = linear_interpolate_round(1, y2, 65535, 1, 100);
        zassert_equal(x2, x2_test, NULL);
    }

    /* test for low-trim, high-trim and scaling from percent to steps */
    for (x2 = 1; x2 <= 100; x2++) {
        y1 = linear_interpolate_round(1, 20, 100, 1, 65535);
        y3 = linear_interpolate_round(1, 80, 100, 1, 65535);
        y2 = linear_interpolate_round(1, x2, 100, y1, y3);
        x2_test = linear_interpolate_round(y1, y2, y3, 1, 100);
        zassert_equal(x2, x2_test, "x2=%hu x2_test=%hu\n", x2, x2_test);
    }

    y2 = linear_interpolate_round(1, 1, 65535, 20, 80);
    zassert_equal(y2, 20, NULL);
    y2 = linear_interpolate_round(1, 1, 65535, 80, 20);
    zassert_equal(y2, 80, NULL);
    y2 = linear_interpolate_round(1, 65535, 65535, 20, 80);
    zassert_equal(y2, 80, NULL);
    y2 = linear_interpolate_round(1, 65535, 65535, 80, 20);
    zassert_equal(y2, 20, NULL);
}

/**
 * Unit Test for linear interpolation of integers
 */
void testLinearInterpolateInt(void)
{
    uint16_t y2 = 0;

    y2 = linear_interpolate_int(1, 1, 65535, 1, 100);
    zassert_equal(y2, 1, NULL);
    y2 = linear_interpolate_int(1, 1, 65535, 100, 1);
    zassert_equal(y2, 100, NULL);

    y2 = linear_interpolate_int(1, 65535, 65535, 1, 100);
    zassert_equal(y2, 100, NULL);
    y2 = linear_interpolate_int(1, 65535, 65535, 100, 1);
    zassert_equal(y2, 1, NULL);

    y2 = linear_interpolate_int(1, (65535 / 4), 65535, 1, 100);
    zassert_equal(y2, 25, NULL);

    y2 = linear_interpolate_int(1, (65535 / 2), 65535, 1, 100);
    zassert_equal(y2, 50, NULL);

    y2 = linear_interpolate_int(1, ((65535 * 3) / 4), 65535, 1, 100);
    zassert_equal(y2, 75, NULL);

    y2 = linear_interpolate_int(1, 1, 100, 1, 65535);
    zassert_equal(y2, 1, NULL);

    y2 = linear_interpolate_int(1, 100, 100, 1, 65535);
    zassert_equal(y2, 65535, NULL);

    y2 = linear_interpolate_int(1, 100 / 2, 100, 1, 65535);
    zassert_equal(y2, 32437, NULL);
}

/**
 * @}
 */

#if defined(CONFIG_ZTEST_NEW_API)
ZTEST_SUITE(Linear_Interpolate, NULL, NULL, NULL, NULL, NULL);
#else
void test_main(void)
{
    ztest_test_suite(
        Linear_Interpolate, ztest_unit_test(testLinearInterpolateRound),
        ztest_unit_test(testLinearInterpolateInt));

    ztest_run_test_suite(Linear_Interpolate);
}
#endif
