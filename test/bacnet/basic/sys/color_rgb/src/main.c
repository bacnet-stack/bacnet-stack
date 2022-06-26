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
static void test_color_rgb(void)
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
     ztest_unit_test(test_color_rgb)
     );

    ztest_run_test_suite(color_rgb_tests);
}
