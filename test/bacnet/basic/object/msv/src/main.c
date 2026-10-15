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
    const int skip_fail_property_list[] = { -1 };

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
    Multistate_Value_Write_Enable(object_instance);

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

#ifdef UINT64_MAX
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
#else
#warning "UINT64_MAX not supported! skipping 64-bit overflow check"
#endif

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
        ztest_unit_test(testMultistateValue_PresentValueRange));

    ztest_run_test_suite(msv_tests);
}
#endif
