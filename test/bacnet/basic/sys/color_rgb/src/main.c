/* @file
 * @brief test BACnet integer encode/decode APIs
 * @date June 2022
 * @brief tests sRGB to and from from CIE xy and brightness API
 *
 * @section LICENSE
 * Copyright (c) 2022 Steve Karg <skarg@users.sourceforge.net>
 *
 * SPDX-License-Identifier: MIT
 */
#include <stdint.h>
#include <stdbool.h>
#include <math.h>
#include <zephyr/ztest.h>
#include <bacnet/basic/sys/color_rgb.h>

/**
 * @addtogroup bacnet_tests
 * @{
 */

/**
 * @brief compare two floating point values to 3 decimal places
 *
 * @param x1 - first comparison value
 * @param x2 - second comparison value
 * @return true if the value is the same to 3 decimal points
 */
static bool is_float_equal(float x1, float x2)
{
    return fabs(x1 - x2) < 0.001;
}

/**
 * Unit Test for sRGB to CIE xy
 */
static void test_color_rgb_xy_gamma_unit(
    uint8_t red,
    uint8_t green,
    uint8_t blue,
    float x_coordinate,
    float y_coordinate,
    uint8_t brightness)
{
    float test_x_coordinate = 0.0, test_y_coordinate = 0.0;
    uint8_t test_brightness = 0;
    uint8_t test_red = 0, test_green = 0, test_blue = 0;

    /* functions with gamma correction */
    color_rgb_to_xy_gamma(
        red, green, blue, &test_x_coordinate, &test_y_coordinate,
        &test_brightness);
    color_rgb_from_xy_gamma(
        &test_red, &test_green, &test_blue, x_coordinate, y_coordinate,
        brightness);
    zassert_true(
        is_float_equal(x_coordinate, test_x_coordinate), "(x=%.3f,test_x=%.3f)",
        x_coordinate, test_x_coordinate);
    zassert_true(
        is_float_equal(y_coordinate, test_y_coordinate), "(y=%.3f,test_y=%.3f)",
        y_coordinate, test_y_coordinate);
    zassert_equal(
        brightness, test_brightness, "b=%u, test_b=%u", brightness,
        test_brightness);
}

/**
 * Unit Test for sRGB to CIE xy
 */
static void test_color_rgb_xy_unit(
    uint8_t red,
    uint8_t green,
    uint8_t blue,
    float x_coordinate,
    float y_coordinate,
    uint8_t brightness)
{
    float test_x_coordinate = 0.0, test_y_coordinate = 0.0;
    uint8_t test_brightness = 0;
    uint8_t test_red = 0, test_green = 0, test_blue = 0;

    color_rgb_to_xy(
        red, green, blue, &test_x_coordinate, &test_y_coordinate,
        &test_brightness);
    color_rgb_from_xy(
        &test_red, &test_green, &test_blue, x_coordinate, y_coordinate,
        brightness);
    zassert_true(
        is_float_equal(x_coordinate, test_x_coordinate), "(x=%.3f,test_x=%.3f)",
        x_coordinate, test_x_coordinate);
    zassert_true(
        is_float_equal(y_coordinate, test_y_coordinate), "(y=%.3f,test_y=%.3f)",
        y_coordinate, test_y_coordinate);
    zassert_equal(
        brightness, test_brightness, "b=%u, test_b=%u", brightness,
        test_brightness);
}

/**
 * Unit Test for sRGB to CIE xy
 */
#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(color_rgb_tests, test_color_rgb_xy)
#else
static void test_color_rgb_xy(void)
#endif
{
    uint8_t red, green, blue;

    /* functions without gamma correction */
    color_rgb_from_ascii(&red, &green, &blue, "black");
    test_color_rgb_xy_unit(red, green, blue, 0.0, 0.0, 0);
    color_rgb_from_ascii(&red, &green, &blue, "white");
    test_color_rgb_xy_unit(red, green, blue, 0.313, 0.329, 255);
    color_rgb_from_ascii(&red, &green, &blue, "blue");
    test_color_rgb_xy_unit(red, green, blue, 0.157, 0.017, 5);
    color_rgb_from_ascii(&red, &green, &blue, "green");
    test_color_rgb_xy_unit(red, green, blue, 0.115, 0.826, 95);
    color_rgb_from_ascii(&red, &green, &blue, "red");
    test_color_rgb_xy_unit(red, green, blue, 0.735, 0.265, 59);
    color_rgb_from_ascii(&red, &green, &blue, "maroon");
    test_color_rgb_xy_unit(red, green, blue, 0.735, 0.265, 29);

    /* functions with gamma correction */
    color_rgb_from_ascii(&red, &green, &blue, "black");
    test_color_rgb_xy_gamma_unit(red, green, blue, 0.0, 0.0, 0);
    color_rgb_from_ascii(&red, &green, &blue, "white");
    test_color_rgb_xy_gamma_unit(red, green, blue, 0.313, 0.329, 255);
    color_rgb_from_ascii(&red, &green, &blue, "blue");
    test_color_rgb_xy_gamma_unit(red, green, blue, 0.157, 0.017, 5);
    color_rgb_from_ascii(&red, &green, &blue, "green");
    test_color_rgb_xy_gamma_unit(red, green, blue, 0.115, 0.826, 40);
    color_rgb_from_ascii(&red, &green, &blue, "red");
    test_color_rgb_xy_gamma_unit(red, green, blue, 0.735, 0.265, 59);
    color_rgb_from_ascii(&red, &green, &blue, "maroon");
    test_color_rgb_xy_gamma_unit(red, green, blue, 0.735, 0.265, 12);
}

/**
 * Unit Test for sRGB to CIE xy
 */
#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(color_rgb_tests, test_color_rgb_ascii)
#else
static void test_color_rgb_ascii(void)
#endif
{
    unsigned count = color_rgb_count();
    zassert_true(count > 0, NULL);
    const char *name, *test_name;
    uint8_t red, green, blue;
    uint8_t test_red, test_green, test_blue;
    unsigned test_index;

    for (unsigned i = 0; i < count; i++) {
        name = color_rgb_from_index(i, &red, &green, &blue);
        zassert_not_null(name, NULL);
        test_index =
            color_rgb_from_ascii(&test_red, &test_green, &test_blue, name);
        zassert_equal(i, test_index, NULL);
        zassert_equal(red, test_red, NULL);
        zassert_equal(green, test_green, NULL);
        zassert_equal(blue, test_blue, NULL);
        test_name = color_rgb_to_ascii(red, green, blue);
        zassert_not_null(test_name, NULL);
    }
}

/**
 * @}
 */

#if defined(CONFIG_ZTEST_NEW_API)
ZTEST_SUITE(color_rgb_tests, NULL, NULL, NULL, NULL, NULL);
#else
void test_main(void)
{
    ztest_test_suite(
        color_rgb_tests, ztest_unit_test(test_color_rgb_ascii),
        ztest_unit_test(test_color_rgb_xy));

    ztest_run_test_suite(color_rgb_tests);
}
#endif
