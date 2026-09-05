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
#include <bacnet/basic/object/bo.h>
#include <property_test.h>

/**
 * @addtogroup bacnet_tests
 * @{
 */

/**
 * @brief Test
 */
#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(bo_tests, testBinaryOutput)
#else
static void testBinaryOutput(void)
#endif
{
    bool status = false;
    unsigned count = 0;
    uint32_t object_instance = BACNET_MAX_INSTANCE, test_object_instance = 0;
    const int32_t skip_fail_property_list[] = { PROP_PRIORITY_ARRAY, -1 };

    Binary_Output_Init();
    object_instance = Binary_Output_Create(object_instance);
    count = Binary_Output_Count();
    zassert_true(count == 1, NULL);
    test_object_instance = Binary_Output_Index_To_Instance(0);
    zassert_equal(object_instance, test_object_instance, NULL);
    bacnet_object_properties_read_write_test(
        OBJECT_BINARY_OUTPUT, object_instance, Binary_Output_Property_Lists,
        Binary_Output_Read_Property, Binary_Output_Write_Property,
        skip_fail_property_list);
    bacnet_object_name_ascii_test(
        object_instance, Binary_Output_Name_Set, Binary_Output_Name_ASCII);
    status = Binary_Output_Delete(object_instance);
    zassert_true(status, NULL);
}

#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(bo_tests, testBinaryOutput_Writable_Properties)
#else
static void testBinaryOutput_Writable_Properties(void)
#endif
{
    const uint32_t instance = 456;
    const int32_t *properties = NULL;
    uint32_t count = 0;
    uint32_t i = 0;
    bool has_object_name = false;
    bool has_description = false;

    Binary_Output_Init();
    zassert_not_equal(
        Binary_Output_Create(instance), BACNET_MAX_INSTANCE, NULL);

    Binary_Output_Writable_Property_List(instance, &properties);
    zassert_not_null(properties, NULL);
    count = property_list_count(properties);
    zassert_true(count > 0, NULL);

    has_object_name = false;
    has_description = false;
    for (i = 0; properties[i] != -1; ++i) {
        if (properties[i] == PROP_OBJECT_NAME) {
            has_object_name = true;
        }
        if (properties[i] == PROP_DESCRIPTION) {
            has_description = true;
        }
    }
    zassert_true(has_object_name, NULL);
    zassert_true(has_description, NULL);

    Binary_Output_Delete(instance);
    Binary_Output_Cleanup();
}
/**
 * @brief Test that changes to Present_Value (priority-array),
 *  Relinquish_Default, and Status_Flags (Out_Of_Service, Reliability/Fault)
 *  set the Change_Of_Value flag when the observable value actually changes,
 *  and that redundant writes do not
 */
#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(bo_tests, testBinaryOutput_COV)
#else
static void testBinaryOutput_COV(void)
#endif
{
    const uint32_t instance = 654;
    bool status = false;

    Binary_Output_Init();
    zassert_not_equal(
        Binary_Output_Create(instance), BACNET_MAX_INSTANCE, NULL);

    /* all priorities relinquished: Present_Value tracks Relinquish_Default,
       which defaults to BINARY_INACTIVE */
    Binary_Output_Change_Of_Value_Clear(instance);
    zassert_false(Binary_Output_Change_Of_Value(instance), NULL);
    zassert_equal(Binary_Output_Present_Value(instance), BINARY_INACTIVE, NULL);

    /* Present_Value via the priority array must set Changed */
    Binary_Output_Change_Of_Value_Clear(instance);
    status = Binary_Output_Present_Value_Set(instance, BINARY_ACTIVE, 8);
    zassert_true(status, NULL);
    zassert_equal(Binary_Output_Present_Value(instance), BINARY_ACTIVE, NULL);
    zassert_true(Binary_Output_Change_Of_Value(instance), NULL);

    /* relinquishing that priority reverts to Relinquish_Default and must
       set Changed again */
    Binary_Output_Change_Of_Value_Clear(instance);
    status = Binary_Output_Present_Value_Relinquish(instance, 8);
    zassert_true(status, NULL);
    zassert_equal(Binary_Output_Present_Value(instance), BINARY_INACTIVE, NULL);
    zassert_true(Binary_Output_Change_Of_Value(instance), NULL);

    /* a Relinquish_Default change must set Changed */
    Binary_Output_Change_Of_Value_Clear(instance);
    status = Binary_Output_Relinquish_Default_Set(instance, BINARY_ACTIVE);
    zassert_true(status, NULL);
    zassert_equal(Binary_Output_Present_Value(instance), BINARY_ACTIVE, NULL);
    zassert_true(Binary_Output_Change_Of_Value(instance), NULL);

    /* setting the same effective value again must not set Changed */
    Binary_Output_Change_Of_Value_Clear(instance);
    status = Binary_Output_Relinquish_Default_Set(instance, BINARY_ACTIVE);
    zassert_true(status, NULL);
    zassert_false(Binary_Output_Change_Of_Value(instance), NULL);

    /* Status_Flags: an Out_Of_Service change must set Changed */
    Binary_Output_Change_Of_Value_Clear(instance);
    Binary_Output_Out_Of_Service_Set(instance, true);
    zassert_true(Binary_Output_Change_Of_Value(instance), NULL);

    /* setting the same Out_Of_Service value again must not set Changed */
    Binary_Output_Change_Of_Value_Clear(instance);
    Binary_Output_Out_Of_Service_Set(instance, true);
    zassert_false(Binary_Output_Change_Of_Value(instance), NULL);
    Binary_Output_Out_Of_Service_Set(instance, false);

    /* Status_Flags: a Fault change via Reliability must set Changed */
    Binary_Output_Change_Of_Value_Clear(instance);
    status = Binary_Output_Reliability_Set(instance, RELIABILITY_NO_SENSOR);
    zassert_true(status, NULL);
    zassert_true(Binary_Output_Change_Of_Value(instance), NULL);

    /* setting the same reliability again must not set Changed */
    Binary_Output_Change_Of_Value_Clear(instance);
    status = Binary_Output_Reliability_Set(instance, RELIABILITY_NO_SENSOR);
    zassert_true(status, NULL);
    zassert_false(Binary_Output_Change_Of_Value(instance), NULL);

    Binary_Output_Delete(instance);
    Binary_Output_Cleanup();
}
/**
 * @}
 */

#if defined(CONFIG_ZTEST_NEW_API)
ZTEST_SUITE(bo_tests, NULL, NULL, NULL, NULL, NULL);
#else
void test_main(void)
{
    ztest_test_suite(
        bo_tests, ztest_unit_test(testBinaryOutput),
        ztest_unit_test(testBinaryOutput_Writable_Properties),
        ztest_unit_test(testBinaryOutput_COV));

    ztest_run_test_suite(bo_tests);
}
#endif
