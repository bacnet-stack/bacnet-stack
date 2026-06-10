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
 * @}
 */

#if defined(CONFIG_ZTEST_NEW_API)
ZTEST_SUITE(mso_tests, NULL, NULL, NULL, NULL, NULL);
#else
void test_main(void)
{
    ztest_test_suite(
        mso_tests, ztest_unit_test(testMultistateOutput),
        ztest_unit_test(testMultistateOutputByName));

    ztest_run_test_suite(mso_tests);
}
#endif
