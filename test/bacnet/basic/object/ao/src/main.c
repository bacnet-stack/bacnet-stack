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
 * @brief Test Analog Output instance validity and property lists
 */
#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(ao_tests, testAnalogOutput_Instance_Properties)
#else
static void testAnalogOutput_Instance_Properties(void)
#endif
{
    const uint32_t instance = 456;
    const uint32_t invalid_instance = instance + 1;
    const int *required = NULL;
    const int *optional = NULL;
    const int *proprietary = NULL;

    Analog_Output_Init();
    zassert_not_equal(
        Analog_Output_Create(instance), BACNET_MAX_INSTANCE, NULL);
    zassert_true(Analog_Output_Valid_Instance(instance), NULL);
    zassert_false(Analog_Output_Valid_Instance(invalid_instance), NULL);

    Analog_Output_Property_Lists(&required, &optional, &proprietary);
    zassert_not_null(required, NULL);
    zassert_not_null(optional, NULL);
    zassert_not_null(proprietary, NULL);
    zassert_equal(required[0] >= 0, true, NULL);

    Analog_Output_Delete(instance);
    zassert_false(Analog_Output_Valid_Instance(instance), NULL);
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
 * @brief Test that the priority-array accessors do not dereference NULL
 *  for an unknown object instance
 */
#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(ao_tests, testAnalogOutput_Priority_Array_Unknown_Instance)
#else
static void testAnalogOutput_Priority_Array_Unknown_Instance(void)
#endif
{
    const uint32_t invalid_instance = 999;

    Analog_Output_Init();

    zassert_false(
        Analog_Output_Priority_Array_Relinquished(invalid_instance, 1), NULL);
    zassert_within(
        Analog_Output_Priority_Array_Value(invalid_instance, 1), 0.0f, 0.001f,
        NULL);

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
 * @brief Test that the COV value list reports the Overridden status flag
 */
#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(ao_tests, testAnalogOutput_COV_Overridden)
#else
static void testAnalogOutput_COV_Overridden(void)
#endif
{
    const uint32_t instance = 988;
    BACNET_PROPERTY_VALUE value_list[2] = { 0 };
    BACNET_BIT_STRING *flags;

    value_list[0].next = &value_list[1];
    value_list[1].next = NULL;
    flags = &value_list[1].value.type.Bit_String;

    Analog_Output_Init();
    zassert_not_equal(
        Analog_Output_Create(instance), BACNET_MAX_INSTANCE, NULL);

    Analog_Output_Change_Of_Value_Clear(instance);
    Analog_Output_Overridden_Set(instance, true);
    zassert_true(Analog_Output_Change_Of_Value(instance), NULL);
    zassert_true(Analog_Output_Encode_Value_List(instance, value_list), NULL);
    zassert_equal(value_list[1].propertyIdentifier, PROP_STATUS_FLAGS, NULL);
    zassert_true(bitstring_bit(flags, STATUS_FLAG_OVERRIDDEN), NULL);

    Analog_Output_Change_Of_Value_Clear(instance);
    Analog_Output_Overridden_Set(instance, false);
    zassert_true(Analog_Output_Change_Of_Value(instance), NULL);
    zassert_true(Analog_Output_Encode_Value_List(instance, value_list), NULL);
    zassert_false(bitstring_bit(flags, STATUS_FLAG_OVERRIDDEN), NULL);

    Analog_Output_Delete(instance);
    Analog_Output_Cleanup();
}

/**
 * @brief Test that a Status_Flags change saves the Present_Value at the
 *  time of the change for the COV value list, and that the saved value
 *  is the new COV_Increment baseline
 */
#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(ao_tests, testAnalogOutput_COV_Status_Flags_Prior_Value)
#else
static void testAnalogOutput_COV_Status_Flags_Prior_Value(void)
#endif
{
    const uint32_t instance = 989;
    BACNET_PROPERTY_VALUE value_list[2] = { 0 };
    BACNET_BIT_STRING *flags;

    value_list[0].next = &value_list[1];
    value_list[1].next = NULL;
    flags = &value_list[1].value.type.Bit_String;

    Analog_Output_Init();
    zassert_not_equal(
        Analog_Output_Create(instance), BACNET_MAX_INSTANCE, NULL);
    Analog_Output_COV_Increment_Set(instance, 2.0f);
    zassert_true(Analog_Output_Present_Value_Set(instance, 10.0f, 8), NULL);
    zassert_true(Analog_Output_Change_Of_Value(instance), NULL);
    Analog_Output_Change_Of_Value_Clear(instance);

    /* a change within COV_Increment is not reported: the value list
       (and the initial notification of a new subscriber) reports the
       value of the last change of value */
    zassert_true(Analog_Output_Present_Value_Set(instance, 11.0f, 8), NULL);
    zassert_false(Analog_Output_Change_Of_Value(instance), NULL);
    zassert_true(Analog_Output_Encode_Value_List(instance, value_list), NULL);
    zassert_equal(value_list[0].propertyIdentifier, PROP_PRESENT_VALUE, NULL);
    zassert_within(value_list[0].value.type.Real, 10.0f, 0.001f, NULL);

    /* Out_Of_Service change: report the Present_Value at that time */
    Analog_Output_Out_Of_Service_Set(instance, true);
    zassert_true(Analog_Output_Change_Of_Value(instance), NULL);
    Analog_Output_Change_Of_Value_Clear(instance);
    /* a later change within COV_Increment does not alter the report */
    zassert_true(Analog_Output_Present_Value_Set(instance, 12.0f, 8), NULL);
    zassert_false(Analog_Output_Change_Of_Value(instance), NULL);
    zassert_true(Analog_Output_Encode_Value_List(instance, value_list), NULL);
    zassert_within(value_list[0].value.type.Real, 11.0f, 0.001f, NULL);
    zassert_true(bitstring_bit(flags, STATUS_FLAG_OUT_OF_SERVICE), NULL);
    /* the baseline is now 11: 13 is equal to COV_Increment from 11 */
    zassert_true(Analog_Output_Present_Value_Set(instance, 13.0f, 8), NULL);
    zassert_true(Analog_Output_Change_Of_Value(instance), NULL);
    Analog_Output_Change_Of_Value_Clear(instance);

    /* Overridden change: report the Present_Value at that time */
    zassert_true(Analog_Output_Present_Value_Set(instance, 14.0f, 8), NULL);
    zassert_false(Analog_Output_Change_Of_Value(instance), NULL);
    Analog_Output_Overridden_Set(instance, true);
    zassert_true(Analog_Output_Change_Of_Value(instance), NULL);
    zassert_true(Analog_Output_Encode_Value_List(instance, value_list), NULL);
    zassert_within(value_list[0].value.type.Real, 14.0f, 0.001f, NULL);
    zassert_true(bitstring_bit(flags, STATUS_FLAG_OVERRIDDEN), NULL);
    Analog_Output_Change_Of_Value_Clear(instance);

    /* Fault change: report the Present_Value at that time */
    zassert_true(Analog_Output_Present_Value_Set(instance, 15.0f, 8), NULL);
    zassert_false(Analog_Output_Change_Of_Value(instance), NULL);
    zassert_true(
        Analog_Output_Reliability_Set(instance, RELIABILITY_NO_SENSOR), NULL);
    zassert_true(Analog_Output_Change_Of_Value(instance), NULL);
    zassert_true(Analog_Output_Encode_Value_List(instance, value_list), NULL);
    zassert_within(value_list[0].value.type.Real, 15.0f, 0.001f, NULL);
    zassert_true(bitstring_bit(flags, STATUS_FLAG_FAULT), NULL);
    Analog_Output_Change_Of_Value_Clear(instance);

    /* a Reliability change without a Fault change does not alter the
       report or the baseline */
    zassert_true(Analog_Output_Present_Value_Set(instance, 16.0f, 8), NULL);
    zassert_true(
        Analog_Output_Reliability_Set(instance, RELIABILITY_OVER_RANGE), NULL);
    zassert_false(Analog_Output_Change_Of_Value(instance), NULL);
    zassert_true(Analog_Output_Encode_Value_List(instance, value_list), NULL);
    zassert_within(value_list[0].value.type.Real, 15.0f, 0.001f, NULL);

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
        ztest_unit_test(testAnalogOutput_Instance_Properties),
        ztest_unit_test(testAnalogOutput_Priority_6_Reserved),
        ztest_unit_test(testAnalogOutput_Priority_Array_Unknown_Instance),
        ztest_unit_test(testAnalogOutput_COV),
        ztest_unit_test(testAnalogOutput_COV_Overridden),
        ztest_unit_test(testAnalogOutput_COV_Status_Flags_Prior_Value));

    ztest_run_test_suite(ao_tests);
}
#endif
