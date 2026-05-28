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
#include <bacnet/basic/object/ms-input.h>
#include <bacnet/bactext.h>
#include <property_test.h>

/**
 * @addtogroup bacnet_tests
 * @{
 */

/**
 * @brief Test
 */
#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(ms_input_tests, testMultistateInput)
#else
static void testMultistateInput(void)
#endif
{
    bool status = false;
    unsigned count = 0;
    uint32_t object_instance = BACNET_MAX_INSTANCE, test_object_instance = 0;
    const int32_t skip_fail_property_list[] = { -1 };

    Multistate_Input_Init();
    object_instance = Multistate_Input_Create(object_instance);
    count = Multistate_Input_Count();
    zassert_true(count == 1, NULL);
    test_object_instance = Multistate_Input_Index_To_Instance(0);
    zassert_equal(object_instance, test_object_instance, NULL);
    bacnet_object_properties_read_write_test(
        OBJECT_MULTI_STATE_INPUT, object_instance,
        Multistate_Input_Property_Lists, Multistate_Input_Read_Property,
        Multistate_Input_Write_Property, skip_fail_property_list);
    bacnet_object_name_ascii_test(
        object_instance, Multistate_Input_Name_Set,
        Multistate_Input_Name_ASCII);
    status = Multistate_Input_Delete(object_instance);
    zassert_true(status, NULL);
}

/**
 * @brief Test state name lookup and set-by-name APIs
 */
#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(ms_input_tests, testMultistateInputByName)
#else
static void testMultistateInputByName(void)
#endif
{
    bool status = false;
    uint32_t object_instance = BACNET_MAX_INSTANCE;
    static const char state_text_list[] = "Off\0On\0Auto\0";

    Multistate_Input_Init();
    object_instance = Multistate_Input_Create(object_instance);
    zassert_not_equal(object_instance, BACNET_MAX_INSTANCE, NULL);

    status =
        Multistate_Input_State_Text_List_Set(object_instance, state_text_list);
    zassert_true(status, NULL);

    zassert_equal(
        Multistate_Input_State_From_Text(object_instance, "On"), 2, NULL);
    zassert_equal(
        Multistate_Input_State_From_Text(object_instance, "Missing"), 0, NULL);

    status = Multistate_Input_Present_Value_By_Name_Set(object_instance, "On");
    zassert_true(status, NULL);
    zassert_equal(Multistate_Input_Present_Value(object_instance), 2, NULL);

    status =
        Multistate_Input_Present_Value_By_Name_Set(object_instance, "Missing");
    zassert_false(status, NULL);

    status = Multistate_Input_Delete(object_instance);
    zassert_true(status, NULL);
}
/**
 * @}
 */

#if defined(CONFIG_ZTEST_NEW_API)
ZTEST_SUITE(ms_input_tests, NULL, NULL, NULL, NULL, NULL);
#else
void test_main(void)
{
    ztest_test_suite(
        ms_input_tests, ztest_unit_test(testMultistateInput),
        ztest_unit_test(testMultistateInputByName));

    ztest_run_test_suite(ms_input_tests);
}
#endif
