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
#include <bacnet/basic/object/msv.h>
#include <bacnet/bactext.h>
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
ZTEST(msv_tests, testMultistateValue)
#else
static void testMultistateValue(void)
#endif
{
    bool status = false;
    unsigned count = 0;
    uint32_t object_instance = BACNET_MAX_INSTANCE, test_object_instance = 0;
    const int32_t skip_fail_property_list[] = { -1 };

    Multistate_Value_Init();
    object_instance = Multistate_Value_Create(object_instance);
    count = Multistate_Value_Count();
    zassert_true(count == 1, NULL);
    test_object_instance = Multistate_Value_Index_To_Instance(0);
    zassert_equal(object_instance, test_object_instance, NULL);
    bacnet_object_properties_read_write_test(
        OBJECT_MULTI_STATE_VALUE, object_instance,
        Multistate_Value_Property_Lists, Multistate_Value_Read_Property,
        Multistate_Value_Write_Property, skip_fail_property_list);
    bacnet_object_name_ascii_test(
        object_instance, Multistate_Value_Name_Set,
        Multistate_Value_Name_ASCII);
    status = Multistate_Value_Delete(object_instance);
    zassert_true(status, NULL);
}

/**
 * @brief Test state name lookup and set-by-name APIs
 */
#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(msv_tests, testMultistateValueByName)
#else
static void testMultistateValueByName(void)
#endif
{
    bool status = false;
    uint32_t object_instance = BACNET_MAX_INSTANCE;
    static const char state_text_list[] = "Off\0On\0Auto\0";

    Multistate_Value_Init();
    object_instance = Multistate_Value_Create(object_instance);
    zassert_not_equal(object_instance, BACNET_MAX_INSTANCE, NULL);

    status =
        Multistate_Value_State_Text_List_Set(object_instance, state_text_list);
    zassert_true(status, NULL);

    zassert_equal(
        Multistate_Value_State_From_Text(object_instance, "On"), 2, NULL);
    zassert_equal(
        Multistate_Value_State_From_Text(object_instance, "Missing"), 0, NULL);

    status = Multistate_Value_Present_Value_By_Name_Set(object_instance, "On");
    zassert_true(status, NULL);
    zassert_equal(Multistate_Value_Present_Value(object_instance), 2, NULL);

    status =
        Multistate_Value_Present_Value_By_Name_Set(object_instance, "Missing");
    zassert_false(status, NULL);

    status = Multistate_Value_Delete(object_instance);
    zassert_true(status, NULL);
}
/**
 * @}
 */

/**
 * @brief Test Multi-State Value Writable_Property_List and Write_Enabled APIs
 */
#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(msv_tests, testMultistateValue_Writable_Properties)
#else
static void testMultistateValue_Writable_Properties(void)
#endif
{
    const uint32_t instance = 456;
    const uint32_t invalid_instance = instance + 1;
    const int32_t *properties = NULL;
    uint32_t count = 0;

    Multistate_Value_Init();
    zassert_not_equal(
        Multistate_Value_Create(instance), BACNET_MAX_INSTANCE, NULL);

    /* write-enabled (default): list starts with PROP_PRESENT_VALUE */
    zassert_true(Multistate_Value_Write_Enabled(instance), NULL);
    Multistate_Value_Writable_Property_List(instance, &properties);
    zassert_not_null(properties, NULL);
    count = property_list_count(properties);
    zassert_true(count > 0, NULL);
    zassert_equal(properties[0], PROP_PRESENT_VALUE, NULL);

    /* write-disabled: list skips PROP_PRESENT_VALUE */
    Multistate_Value_Write_Disable(instance);
    zassert_false(Multistate_Value_Write_Enabled(instance), NULL);
    Multistate_Value_Writable_Property_List(instance, &properties);
    zassert_not_null(properties, NULL);
    zassert_not_equal(properties[0], PROP_PRESENT_VALUE, NULL);

    /* write re-enabled: PROP_PRESENT_VALUE back at head */
    Multistate_Value_Write_Enable(instance);
    zassert_true(Multistate_Value_Write_Enabled(instance), NULL);
    Multistate_Value_Writable_Property_List(instance, &properties);
    zassert_equal(properties[0], PROP_PRESENT_VALUE, NULL);

    /* unknown instance: must return a valid list, not NULL/garbage */
    properties = NULL;
    Multistate_Value_Writable_Property_List(invalid_instance, &properties);
    zassert_not_null(properties, NULL);
    count = property_list_count(properties);
    zassert_true(count > 0, NULL);

    /* NULL properties pointer: must not crash */
    Multistate_Value_Writable_Property_List(instance, NULL);

    Multistate_Value_Delete(instance);
    Multistate_Value_Cleanup();
}
/**
 * @}
 */

#if defined(CONFIG_ZTEST_NEW_API)
ZTEST_SUITE(msv_tests, NULL, NULL, NULL, NULL, NULL);
#else
void test_main(void)
{
    ztest_test_suite(
        msv_tests, ztest_unit_test(testMultistateValue),
        ztest_unit_test(testMultistateValueByName),
        ztest_unit_test(testMultistateValue_Writable_Properties));

    ztest_run_test_suite(msv_tests);
}
#endif
