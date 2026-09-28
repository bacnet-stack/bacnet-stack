/**
 * @file
 * @brief Unit test for object
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date April 2024
 * @section LICENSE
 *
 * @copyright SPDX-License-Identifier: MIT
 */
#include <stdio.h>
#include <zephyr/ztest.h>
#include <bacnet/basic/object/msv.h>
#include <bacnet/bactext.h>
#include <bacnet/proplist.h>
#include <bacnet/wp.h>
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
    zassert_true(property_list_member(properties, PROP_PRESENT_VALUE), NULL);

    /* write-disabled: list skips PROP_PRESENT_VALUE */
    Multistate_Value_Write_Disable(instance);
    zassert_false(Multistate_Value_Write_Enabled(instance), NULL);
    Multistate_Value_Writable_Property_List(instance, &properties);
    zassert_not_null(properties, NULL);
    zassert_false(property_list_member(properties, PROP_PRESENT_VALUE), NULL);

    /* write re-enabled: PROP_PRESENT_VALUE back at head */
    Multistate_Value_Write_Enable(instance);
    zassert_true(Multistate_Value_Write_Enabled(instance), NULL);
    Multistate_Value_Writable_Property_List(instance, &properties);
    zassert_true(property_list_member(properties, PROP_PRESENT_VALUE), NULL);

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
 * @brief Regression test for create/delete cleanup of the state-name list.
 * The object must be fully torn down when removed from the object list, and the
 * key list should be empty after cleanup.
 */
#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(msv_tests, testMultistateValue_CreateCleanup)
#else
static void testMultistateValue_CreateCleanup(void)
#endif
{
    const uint32_t instance = 789;
    bool status = false;

    Multistate_Value_Init();
    zassert_not_equal(
        Multistate_Value_Create(instance), BACNET_MAX_INSTANCE, NULL);
    zassert_equal(Multistate_Value_Count(), 1, NULL);
    zassert_true(Multistate_Value_Valid_Instance(instance), NULL);

    status = Multistate_Value_Delete(instance);
    zassert_true(status, NULL);
    zassert_equal(Multistate_Value_Count(), 0, NULL);

    Multistate_Value_Cleanup();
    zassert_equal(Multistate_Value_Count(), 0, NULL);
}
/**
 * @}
 */

/**
 * @brief Regression test for B6: Present_Value must not truncate when
 * Number_Of_States exceeds 255, and Write_Property must reject a value
 * that does not fit in a uint32_t instead of silently wrapping it.
 */
#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(msv_tests, testMultistateValue_PresentValueRange)
#else
static void testMultistateValue_PresentValueRange(void)
#endif
{
    bool status = false;
    uint32_t object_instance = BACNET_MAX_INSTANCE;
    static char state_text_list[4096];
    size_t offset = 0;
    unsigned i;
    const unsigned state_count = 300;
    BACNET_WRITE_PROPERTY_DATA wp_data = { 0 };

    Multistate_Value_Init();
    object_instance = Multistate_Value_Create(object_instance);
    zassert_not_equal(object_instance, BACNET_MAX_INSTANCE, NULL);

    for (i = 1; i <= state_count; i++) {
        offset += (size_t)snprintf(
            &state_text_list[offset], sizeof(state_text_list) - offset, "S%u",
            i);
        state_text_list[offset++] = 0;
    }
    state_text_list[offset++] = 0;
    status =
        Multistate_Value_State_Text_List_Set(object_instance, state_text_list);
    zassert_true(status, NULL);

    /* a state number above 255 must survive storage without truncation */
    status = Multistate_Value_Present_Value_Set(object_instance, 256);
    zassert_true(status, NULL);
    zassert_equal(Multistate_Value_Present_Value(object_instance), 256, NULL);

    /* Write_Property with a value above 255 must not wrap to 0 */
    wp_data.object_type = OBJECT_MULTI_STATE_VALUE;
    wp_data.object_instance = object_instance;
    wp_data.array_index = BACNET_ARRAY_ALL;
    wp_data.priority = BACNET_NO_PRIORITY;
    wp_data.object_property = PROP_PRESENT_VALUE;
    wp_data.application_data_len =
        encode_application_unsigned(wp_data.application_data, 257);
    status = Multistate_Value_Write_Property(&wp_data);
    zassert_true(status, NULL);
    zassert_equal(Multistate_Value_Present_Value(object_instance), 257, NULL);

    /* a 64-bit value whose low 32 bits alias a valid state (2^32 + 2) must
       be rejected, not silently truncated and accepted */
    wp_data.application_data_len = encode_application_unsigned(
        wp_data.application_data, UINT64_C(4294967298));
    status = Multistate_Value_Write_Property(&wp_data);
    zassert_false(status, NULL);
    zassert_equal(wp_data.error_class, ERROR_CLASS_PROPERTY, NULL);
    zassert_equal(wp_data.error_code, ERROR_CODE_VALUE_OUT_OF_RANGE, NULL);
    /* present-value must remain unchanged, not wrapped to 2 */
    zassert_equal(Multistate_Value_Present_Value(object_instance), 257, NULL);

    status = Multistate_Value_Delete(object_instance);
    zassert_true(status, NULL);
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
        ztest_unit_test(testMultistateValue_Writable_Properties),
        ztest_unit_test(testMultistateValue_CreateCleanup),
        ztest_unit_test(testMultistateValue_PresentValueRange));

    ztest_run_test_suite(msv_tests);
}
#endif
