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
#include <bacnet/basic/object/ai.h>
#include <property_test.h>

/**
 * @addtogroup bacnet_tests
 * @{
 */

/**
 * @brief Test
 */
#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(ai_tests, testAnalogInput)
#else
static void testAnalogInput(void)
#endif
{
    bool status = false;
    unsigned count = 0;
    uint32_t object_instance = BACNET_MAX_INSTANCE, test_object_instance = 0;
    const int skip_fail_property_list[] = { -1 };

    Analog_Input_Init();
    object_instance = Analog_Input_Create(object_instance);
    count = Analog_Input_Count();
    zassert_true(count == 1, NULL);
    test_object_instance = Analog_Input_Index_To_Instance(0);
    zassert_equal(object_instance, test_object_instance, NULL);
    bacnet_object_properties_read_write_test(
        OBJECT_ANALOG_INPUT, object_instance, Analog_Input_Property_Lists,
        Analog_Input_Read_Property, Analog_Input_Write_Property,
        skip_fail_property_list);
    status = Analog_Input_Delete(object_instance);
    zassert_true(status, NULL);
}
/**
 * @}
 */

#if defined(CONFIG_ZTEST_NEW_API)
ZTEST_SUITE(ai_tests, NULL, NULL, NULL, NULL, NULL);
#else
void test_main(void)
{
    ztest_test_suite(ai_tests, ztest_unit_test(testAnalogInput));

    ztest_run_test_suite(ai_tests);
}
#endif
