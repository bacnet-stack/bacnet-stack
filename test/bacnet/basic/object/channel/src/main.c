/**
 * @file
 * @brief Unit test for object
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date July 2023
 * @section LICENSE
 *
 * SPDX-License-Identifier: MIT
 */
#include <zephyr/ztest.h>
#include <bacnet/basic/object/channel.h>
#include <bacnet/bactext.h>
#include <property_test.h>

/**
 * @addtogroup bacnet_tests
 * @{
 */

/**
 * @brief Test
 */
static void test_Channel_ReadProperty(void)
{
    const uint32_t instance = 123;
    unsigned count = 0;
    unsigned index = 0;
    const char *sample_name = "Channel:0";
    const char *test_name = NULL;
    bool status = false;
    const int skip_fail_property_list[] = { -1 };

    Channel_Init();
    Channel_Create(instance);
    status = Channel_Valid_Instance(instance);
    zassert_true(status, NULL);
    index = Channel_Instance_To_Index(instance);
    zassert_equal(index, 0, NULL);
    count = Channel_Count();
    zassert_true(count > 0, NULL);
    /* configure the instance property values */
    /* perform a general test for RP/WP */
    bacnet_object_properties_read_write_test(
        OBJECT_CHANNEL, instance, Channel_Property_Lists, Channel_Read_Property,
        Channel_Write_Property, skip_fail_property_list);
    /* test the ASCII name get/set */
    status = Channel_Name_Set(instance, sample_name);
    zassert_true(status, NULL);
    test_name = Channel_Name_ASCII(instance);
    zassert_equal(test_name, sample_name, NULL);
    status = Channel_Name_Set(instance, NULL);
    zassert_true(status, NULL);
    test_name = Channel_Name_ASCII(instance);
    zassert_equal(test_name, NULL, NULL);
    /* cleanup */
    status = Channel_Delete(instance);
    zassert_true(status, NULL);
}
/**
 * @}
 */

void test_main(void)
{
    ztest_test_suite(channel_tests, ztest_unit_test(test_Channel_ReadProperty));

    ztest_run_test_suite(channel_tests);
}
