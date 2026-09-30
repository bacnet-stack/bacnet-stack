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
#include <bacnet/basic/object/ao.h>
#include <property_test.h>

/**
 * @addtogroup bacnet_tests
 * @{
 */

/**
 * @brief Test
 */
#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(ao_tests, testAnalogOutput)
#else
static void testAnalogOutput(void)
#endif
{
    bool status = false;
    unsigned count = 0;
    uint32_t object_instance = BACNET_MAX_INSTANCE, test_object_instance = 0;
    const int skip_fail_property_list[] = { -1 };

    Analog_Output_Init();
    object_instance = Analog_Output_Create(object_instance);
    count = Analog_Output_Count();
    zassert_true(count == 1, NULL);
    test_object_instance = Analog_Output_Index_To_Instance(0);
    zassert_equal(object_instance, test_object_instance, NULL);
    bacnet_object_properties_read_write_test(
        OBJECT_ANALOG_OUTPUT, object_instance, Analog_Output_Property_Lists,
        Analog_Output_Read_Property, Analog_Output_Write_Property,
        skip_fail_property_list);
    bacnet_object_name_ascii_test(
        object_instance, Analog_Output_Name_Set, Analog_Output_Name_ASCII);
    status = Analog_Output_Delete(object_instance);
    zassert_true(status, NULL);
}

/**
 * @brief Test Analog Output Writable_Property_List API
 */
#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(ao_tests, testAnalogOutput_Writable_Properties)
#else
static void testAnalogOutput_Writable_Properties(void)
#endif
{
    const uint32_t instance = 456;
    const uint32_t invalid_instance = instance + 1;
    const int32_t *properties = NULL;
    uint32_t count = 0;

    Analog_Output_Init();
    zassert_not_equal(
        Analog_Output_Create(instance), BACNET_MAX_INSTANCE, NULL);

    Analog_Output_Delete(instance);
    Analog_Output_Cleanup();
}

/**
 * @brief Test that priority 6 (reserved for the Minimum On/Off algorithm
 *  per the BACnet standard's recommended priority assignments) is rejected
 *  by the present-value priority-array primitives, and that other priority
 *  levels are unaffected
 */
#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(ao_tests, testAnalogOutput_Priority_6_Reserved)
#else
static void testAnalogOutput_Priority_6_Reserved(void)
#endif
{
    const uint32_t instance = 789;
    bool status = false;

    Analog_Output_Init();
    zassert_not_equal(
        Analog_Output_Create(instance), BACNET_MAX_INSTANCE, NULL);

    /* priority 6 is reserved: Set must be rejected and the slot must stay
       relinquished */
    status = Analog_Output_Present_Value_Set(instance, 42.0f, 6);
    zassert_false(status, NULL);
    zassert_true(Analog_Output_Priority_Array_Relinquished(instance, 6), NULL);

    /* priority 6 is reserved: Relinquish must be rejected too (nothing to
       relinquish, no false report of a state change) */
    status = Analog_Output_Present_Value_Relinquish(instance, 6);
    zassert_false(status, NULL);

    /* a non-reserved priority is unaffected by the above */
    status = Analog_Output_Present_Value_Set(instance, 42.0f, 5);
    zassert_true(status, NULL);
    zassert_false(Analog_Output_Priority_Array_Relinquished(instance, 5), NULL);
    status = Analog_Output_Present_Value_Relinquish(instance, 5);
    zassert_true(status, NULL);
    zassert_true(Analog_Output_Priority_Array_Relinquished(instance, 5), NULL);

    Analog_Output_Delete(instance);
    Analog_Output_Cleanup();
}
/**
 * @brief Test that changes to Present_Value (priority-array),
 *  Relinquish_Default, and Status_Flags (Out_Of_Service, Reliability/Fault)
 *  set the Change_Of_Value flag when the observable value actually changes,
 *  and that redundant writes do not
 */
#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(ao_tests, testAnalogOutput_COV)
#else
static void testAnalogOutput_COV(void)
#endif
{
    const uint32_t instance = 987;
    bool status = false;

    Analog_Output_Init();
    zassert_not_equal(
        Analog_Output_Create(instance), BACNET_MAX_INSTANCE, NULL);

    /* Present_Value via the priority array must set Changed */
    Analog_Output_Change_Of_Value_Clear(instance);
    zassert_false(Analog_Output_Change_Of_Value(instance), NULL);
    status = Analog_Output_Present_Value_Set(instance, 20.0f, 8);
    zassert_true(status, NULL);
    zassert_within(Analog_Output_Present_Value(instance), 20.0f, 0.001f, NULL);
    zassert_true(Analog_Output_Change_Of_Value(instance), NULL);

    /* relinquishing that priority reverts to Relinquish_Default and must
       set Changed again */
    Analog_Output_Change_Of_Value_Clear(instance);
    status = Analog_Output_Present_Value_Relinquish(instance, 8);
    zassert_true(status, NULL);
    zassert_true(Analog_Output_Change_Of_Value(instance), NULL);

    /* all priorities relinquished: a Relinquish_Default change beyond
       COV_Increment (default 1.0) must set Changed */
    Analog_Output_Change_Of_Value_Clear(instance);
    status = Analog_Output_Relinquish_Default_Set(instance, 5.0f);
    zassert_true(status, NULL);
    zassert_within(Analog_Output_Present_Value(instance), 5.0f, 0.001f, NULL);
    zassert_true(Analog_Output_Change_Of_Value(instance), NULL);

    /* a Relinquish_Default change within COV_Increment must not set
       Changed */
    Analog_Output_Change_Of_Value_Clear(instance);
    status = Analog_Output_Relinquish_Default_Set(instance, 5.5f);
    zassert_true(status, NULL);
    zassert_false(Analog_Output_Change_Of_Value(instance), NULL);

    /* Status_Flags: an Out_Of_Service change must set Changed */
    Analog_Output_Change_Of_Value_Clear(instance);
    Analog_Output_Out_Of_Service_Set(instance, true);
    zassert_true(Analog_Output_Change_Of_Value(instance), NULL);

    /* setting the same Out_Of_Service value again must not set Changed */
    Analog_Output_Change_Of_Value_Clear(instance);
    Analog_Output_Out_Of_Service_Set(instance, true);
    zassert_false(Analog_Output_Change_Of_Value(instance), NULL);
    Analog_Output_Out_Of_Service_Set(instance, false);

    /* Status_Flags: a Fault change via Reliability must set Changed */
    Analog_Output_Change_Of_Value_Clear(instance);
    status = Analog_Output_Reliability_Set(instance, RELIABILITY_NO_SENSOR);
    zassert_true(status, NULL);
    zassert_true(Analog_Output_Change_Of_Value(instance), NULL);

    /* setting the same reliability again must not set Changed */
    Analog_Output_Change_Of_Value_Clear(instance);
    status = Analog_Output_Reliability_Set(instance, RELIABILITY_NO_SENSOR);
    zassert_true(status, NULL);
    zassert_false(Analog_Output_Change_Of_Value(instance), NULL);

    Analog_Output_Delete(instance);
    Analog_Output_Cleanup();
}
/**
 * @}
 */

#if defined(CONFIG_ZTEST_NEW_API)
ZTEST_SUITE(ao_tests, NULL, NULL, NULL, NULL, NULL);
#else
void test_main(void)
{
    ztest_test_suite(
        ao_tests, ztest_unit_test(testAnalogOutput),
        ztest_unit_test(testAnalogOutput_Writable_Properties),
        ztest_unit_test(testAnalogOutput_Priority_6_Reserved),
        ztest_unit_test(testAnalogOutput_COV));

    ztest_run_test_suite(ao_tests);
}
#endif
