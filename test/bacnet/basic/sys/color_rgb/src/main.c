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
#include <ztest.h>
#include <bacnet/basic/sys/color_rgb.h>

/**
 * @addtogroup bacnet_tests
 * @{
 */

/**
 * Unit Test for sRGB to CIE xy
 */
static void test_color_rgb_xy_unit(
    uint8_t red, uint8_t green, uint8_t blue,
    float x_coordinate, float y_coordinate,
    uint8_t brightness)
{
    float test_x_coordinate = 0.0, test_y_coordinate = 0.0;
    uint8_t test_brightness = 0;
    uint8_t test_red = 0, test_green = 0, test_blue = 0;

    printf("test value:(%u,%u,%u)=(%.3f,%.3f,%u)\n",
        (unsigned)red, (unsigned)green, (unsigned)blue,
        x_coordinate, y_coordinate, (unsigned)brightness);
    color_rgb_to_xy(red, green, blue, &test_x_coordinate, &test_y_coordinate,
        &test_brightness);
    color_rgb_from_xy(&test_red, &test_green, &test_blue,
        x_coordinate, y_coordinate, brightness);
    printf("calculated:(%u,%u,%u)=(%.3f,%.3f,%u)\n",
        (unsigned)test_red, (unsigned)test_green, (unsigned)test_blue,
        test_x_coordinate, test_y_coordinate, (unsigned)test_brightness);
    //zassert_equal(x_coordinate, test_x_coordinate, NULL);
    //zassert_equal(y_coordinate, test_y_coordinate, NULL);
    //zassert_equal(brightness, test_brightness, NULL);
    //zassert_equal(red, test_red, NULL);
    //zassert_equal(green, test_green, NULL);
    //zassert_equal(blue, test_blue, NULL);
}

/**
 * Unit Test for sRGB to CIE xy
 */
static void test_color_rgb_xy(void)
{
    test_color_rgb_xy_unit(0, 0, 0, 0.0, 0.0, 0);
    test_color_rgb_xy_unit(255, 255, 255, 0.323, 0.329, 255);
    test_color_rgb_xy_unit(0, 0, 255, 0.136, 0.04, 12);
    test_color_rgb_xy_unit(0, 255, 0, 0.172, 0.747, 170);
    test_color_rgb_xy_unit(255, 0, 0, 0.701, 0.299, 72);
    test_color_rgb_xy_unit(128, 0, 0, 0.701, 0.299, 16);
}

/**
* Unit Test for sRGB to CIE xy
*/
static void test_color_rgb_ascii(void)
{
    unsigned count = color_rgb_count();
    zassert_true(count > 0, NULL);
    const char *name, *test_name;
    uint8_t red, green, blue;
    uint8_t test_red, test_green, test_blue;
    unsigned test_index;
    float x_coordinate;
    float y_coordinate;
    float brightness;

    for (unsigned i = 0; i < count; i++) {
        name = color_rgb_from_index(i, &red, &green, &blue);
        zassert_not_null(name, NULL);
        test_index = color_rgb_from_ascii(&test_red, &test_green, &test_blue,
            name);
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


void test_main(void)
{
    ztest_test_suite(color_rgb_tests,
     ztest_unit_test(test_color_rgb_ascii),
     ztest_unit_test(test_color_rgb_xy)
     );

    ztest_run_test_suite(color_rgb_tests);
}
