/**
 * @file
 * @brief Unit test for object
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date April 2024
 * @section LICENSE
 *
 * @copyright SPDX-License-Identifier: MIT
 */
#include <zephyr/ztest.h>
#include <bacnet/basic/object/ai.h>
#include <bacnet/proplist.h>
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
    const int32_t skip_fail_property_list[] = { -1 };

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
    bacnet_object_name_ascii_test(
        object_instance, Analog_Input_Name_Set, Analog_Input_Name_ASCII);
    status = Analog_Input_Delete(object_instance);
    zassert_true(status, NULL);
}

/**
 * @brief Test Analog Input Writable_Property_List API
 */
#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(ai_tests, testAnalogInput_Writable_Properties)
#else
static void testAnalogInput_Writable_Properties(void)
#endif
{
    const uint32_t instance = 456;
    const uint32_t invalid_instance = instance + 1;
    const int32_t *properties = NULL;
    uint32_t count = 0;

    Analog_Input_Init();
    zassert_not_equal(Analog_Input_Create(instance), BACNET_MAX_INSTANCE, NULL);

    /* valid instance: list is non-NULL and -1-terminated */
    Analog_Input_Writable_Property_List(instance, &properties);
    zassert_not_null(properties, NULL);
    count = property_list_count(properties);
    zassert_true(count > 0, NULL);

    /* unknown instance: must still return a valid list, not NULL/garbage */
    properties = NULL;
    Analog_Input_Writable_Property_List(invalid_instance, &properties);
    zassert_not_null(properties, NULL);

    /* NULL properties pointer: must not crash */
    Analog_Input_Writable_Property_List(instance, NULL);

    Analog_Input_Delete(instance);
    Analog_Input_Cleanup();
}
/**
 * @}
 */

#if defined(CONFIG_ZTEST_NEW_API)
ZTEST_SUITE(ai_tests, NULL, NULL, NULL, NULL, NULL);
#else
void test_main(void)
{
    ztest_test_suite(
        ai_tests, ztest_unit_test(testAnalogInput),
        ztest_unit_test(testAnalogInput_Writable_Properties));

    ztest_run_test_suite(ai_tests);
}
#endif
