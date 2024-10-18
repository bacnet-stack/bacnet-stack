/**
 * @file
 * @brief Unit test for object
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date April 2024
 * @section LICENSE
 *
 * SPDX-License-Identifier: MIT
 */
#include <zephyr/ztest.h>
#include <bacnet/basic/object/av.h>
#include <property_test.h>

/**
 * @addtogroup bacnet_tests
 * @{
 */

/**
 * @brief Test
 */
#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(av_tests, testAnalog_Value)
#else
static void testAnalog_Value(void)
#endif
{
    bool status = false;
    unsigned count = 0;
    uint32_t object_instance = BACNET_MAX_INSTANCE, test_object_instance = 0;
    const int skip_fail_property_list[] = { -1 };

    Analog_Value_Init();
    object_instance = Analog_Value_Create(object_instance);
    count = Analog_Value_Count();
    zassert_true(count == 1, NULL);
    test_object_instance = Analog_Value_Index_To_Instance(0);
    zassert_equal(object_instance, test_object_instance, NULL);
    bacnet_object_properties_read_write_test(
        OBJECT_ANALOG_VALUE, object_instance, Analog_Value_Property_Lists,
        Analog_Value_Read_Property, Analog_Value_Write_Property,
        skip_fail_property_list);
    bacnet_object_name_ascii_test(
        object_instance, Analog_Value_Name_Set, Analog_Value_Name_ASCII);
    status = Analog_Value_Delete(object_instance);
    zassert_true(status, NULL);
}
/**
 * @}
 */

#if defined(CONFIG_ZTEST_NEW_API)
ZTEST_SUITE(av_tests, NULL, NULL, NULL, NULL, NULL);
#else
void test_main(void)
{
    ztest_test_suite(av_tests, ztest_unit_test(testAnalog_Value));

    ztest_run_test_suite(av_tests);
}
#endif
