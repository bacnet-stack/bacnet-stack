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
#include <bacnet/basic/object/mso.h>
#include <property_test.h>

/**
 * @addtogroup bacnet_tests
 * @{
 */

/**
 * @brief Test
 */
#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(mso_tests, testMultistateOutput)
#else
static void testMultistateOutput(void)
#endif
{
    bool status = false;
    unsigned count = 0;
    uint32_t object_instance = BACNET_MAX_INSTANCE, test_object_instance = 0;
    const int32_t skip_fail_property_list[] = { PROP_PRIORITY_ARRAY, -1 };

    Multistate_Output_Init();
    object_instance = Multistate_Output_Create(object_instance);
    count = Multistate_Output_Count();
    zassert_true(count == 1, NULL);
    test_object_instance = Multistate_Output_Index_To_Instance(0);
    zassert_equal(object_instance, test_object_instance, NULL);
    bacnet_object_properties_read_write_test(
        OBJECT_MULTI_STATE_OUTPUT, object_instance,
        Multistate_Output_Property_Lists, Multistate_Output_Read_Property,
        Multistate_Output_Write_Property, skip_fail_property_list);
    bacnet_object_name_ascii_test(
        object_instance, Multistate_Output_Name_Set,
        Multistate_Output_Name_ASCII);
    status = Multistate_Output_Delete(object_instance);
    zassert_true(status, NULL);
}

/**
 * @brief Test state name lookup and set-by-name APIs
 */
#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(mso_tests, testMultistateOutputByName)
#else
static void testMultistateOutputByName(void)
#endif
{
    bool status = false;
    uint32_t object_instance = BACNET_MAX_INSTANCE;
    static const char state_text_list[] = "Off\0On\0Auto\0";

    Multistate_Output_Init();
    object_instance = Multistate_Output_Create(object_instance);
    zassert_not_equal(object_instance, BACNET_MAX_INSTANCE, NULL);

    status =
        Multistate_Output_State_Text_List_Set(object_instance, state_text_list);
    zassert_true(status, NULL);

    zassert_equal(
        Multistate_Output_State_From_Text(object_instance, "On"), 2, NULL);
    zassert_equal(
        Multistate_Output_State_From_Text(object_instance, "Missing"), 0, NULL);

    status =
        Multistate_Output_Present_Value_By_Name_Set(object_instance, "On", 8);
    zassert_true(status, NULL);
    zassert_equal(Multistate_Output_Present_Value(object_instance), 2, NULL);

    status = Multistate_Output_Present_Value_By_Name_Set(
        object_instance, "Missing", 8);
    zassert_false(status, NULL);

    status = Multistate_Output_Delete(object_instance);
    zassert_true(status, NULL);
}

/**
 * @brief Test that priority 6 (reserved for the Minimum On/Off algorithm
 *  per the BACnet standard's recommended priority assignments) is rejected
 *  by the present-value priority-array primitives, and that other priority
 *  levels are unaffected
 */
#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(mso_tests, testMultistateOutput_Priority_6_Reserved)
#else
static void testMultistateOutput_Priority_6_Reserved(void)
#endif
{
    const uint32_t instance = 789;
    bool status = false;

    Multistate_Output_Init();
    zassert_not_equal(
        Multistate_Output_Create(instance), BACNET_MAX_INSTANCE, NULL);

    /* priority 6 is reserved: Set must be rejected and the slot must stay
       relinquished */
    status = Multistate_Output_Present_Value_Set(instance, 2, 6);
    zassert_false(status, NULL);
    zassert_true(
        Multistate_Output_Priority_Array_Relinquished(instance, 6), NULL);

    /* priority 6 is reserved: Relinquish must be rejected too (nothing to
       relinquish, no false report of a state change) */
    status = Multistate_Output_Present_Value_Relinquish(instance, 6);
    zassert_false(status, NULL);

    /* a non-reserved priority is unaffected by the above */
    status = Multistate_Output_Present_Value_Set(instance, 2, 5);
    zassert_true(status, NULL);
    zassert_false(
        Multistate_Output_Priority_Array_Relinquished(instance, 5), NULL);
    status = Multistate_Output_Present_Value_Relinquish(instance, 5);
    zassert_true(status, NULL);
    zassert_true(
        Multistate_Output_Priority_Array_Relinquished(instance, 5), NULL);

    Multistate_Output_Delete(instance);
    Multistate_Output_Cleanup();
}
/**
 * @brief Test that changes to Present_Value (priority-array),
 *  Relinquish_Default, and Status_Flags (Out_Of_Service, Reliability/Fault)
 *  set the Change_Of_Value flag when the observable value actually changes,
 *  and that redundant writes do not
 */
#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(mso_tests, testMultistateOutput_COV)
#else
static void testMultistateOutput_COV(void)
#endif
{
    const uint32_t instance = 654;
    bool status = false;

    Multistate_Output_Init();
    zassert_not_equal(
        Multistate_Output_Create(instance), BACNET_MAX_INSTANCE, NULL);

    /* all priorities relinquished: Present_Value tracks Relinquish_Default,
       which defaults to state 1 */
    Multistate_Output_Change_Of_Value_Clear(instance);
    zassert_false(Multistate_Output_Change_Of_Value(instance), NULL);
    zassert_equal(Multistate_Output_Present_Value(instance), 1, NULL);

    /* Present_Value via the priority array must set Changed */
    Multistate_Output_Change_Of_Value_Clear(instance);
    status = Multistate_Output_Present_Value_Set(instance, 3, 8);
    zassert_true(status, NULL);
    zassert_equal(Multistate_Output_Present_Value(instance), 3, NULL);
    zassert_true(Multistate_Output_Change_Of_Value(instance), NULL);

    /* relinquishing that priority reverts to Relinquish_Default and must
       set Changed again */
    Multistate_Output_Change_Of_Value_Clear(instance);
    status = Multistate_Output_Present_Value_Relinquish(instance, 8);
    zassert_true(status, NULL);
    zassert_equal(Multistate_Output_Present_Value(instance), 1, NULL);
    zassert_true(Multistate_Output_Change_Of_Value(instance), NULL);

    /* a Relinquish_Default change must set Changed */
    Multistate_Output_Change_Of_Value_Clear(instance);
    status = Multistate_Output_Relinquish_Default_Set(instance, 2);
    zassert_true(status, NULL);
    zassert_equal(Multistate_Output_Present_Value(instance), 2, NULL);
    zassert_true(Multistate_Output_Change_Of_Value(instance), NULL);

    /* setting the same effective value again must not set Changed */
    Multistate_Output_Change_Of_Value_Clear(instance);
    status = Multistate_Output_Relinquish_Default_Set(instance, 2);
    zassert_true(status, NULL);
    zassert_false(Multistate_Output_Change_Of_Value(instance), NULL);

    /* Status_Flags: an Out_Of_Service change must set Changed */
    Multistate_Output_Change_Of_Value_Clear(instance);
    Multistate_Output_Out_Of_Service_Set(instance, true);
    zassert_true(Multistate_Output_Change_Of_Value(instance), NULL);

    /* setting the same Out_Of_Service value again must not set Changed */
    Multistate_Output_Change_Of_Value_Clear(instance);
    Multistate_Output_Out_Of_Service_Set(instance, true);
    zassert_false(Multistate_Output_Change_Of_Value(instance), NULL);
    Multistate_Output_Out_Of_Service_Set(instance, false);

    /* Status_Flags: a Fault change via Reliability must set Changed */
    Multistate_Output_Change_Of_Value_Clear(instance);
    status = Multistate_Output_Reliability_Set(instance, RELIABILITY_NO_SENSOR);
    zassert_true(status, NULL);
    zassert_true(Multistate_Output_Change_Of_Value(instance), NULL);

    /* setting the same reliability again must not set Changed */
    Multistate_Output_Change_Of_Value_Clear(instance);
    status = Multistate_Output_Reliability_Set(instance, RELIABILITY_NO_SENSOR);
    zassert_true(status, NULL);
    zassert_false(Multistate_Output_Change_Of_Value(instance), NULL);

    Multistate_Output_Delete(instance);
    Multistate_Output_Cleanup();
}
/**
 * @}
 */

#if defined(CONFIG_ZTEST_NEW_API)
ZTEST_SUITE(mso_tests, NULL, NULL, NULL, NULL, NULL);
#else
void test_main(void)
{
    ztest_test_suite(
        mso_tests, ztest_unit_test(testMultistateOutput),
        ztest_unit_test(testMultistateOutputByName),
        ztest_unit_test(testMultistateOutput_Priority_6_Reserved),
        ztest_unit_test(testMultistateOutput_COV));

    ztest_run_test_suite(mso_tests);
}
#endif
