/**
 * @file
 * @brief Unit test for object
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date June 2024
 * @copyright SPDX-License-Identifier: MIT
 */
#include <zephyr/ztest.h>
#include <bacnet/bactext.h>
#include <bacnet/cov.h>
#include <bacnet/basic/object/bitstring_value.h>
#include <property_test.h>

/**
 * @addtogroup bacnet_tests
 * @{
 */

/**
 * @brief Test
 */
#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(bitstring_value_object_tests, test_BitString_Value_Object)
#else
static void test_BitString_Value_Object(void)
#endif
{
    const int skip_fail_property_list[] = { -1 };
    const uint32_t instance = 123;
    uint32_t test_instance = 0;
    unsigned test_count = 0;
    unsigned test_index = 0;
    bool status = false;
    BACNET_WRITE_PROPERTY_DATA wpdata = { 0 };
    BACNET_APPLICATION_DATA_VALUE value = { 0 };
    BACNET_PROPERTY_VALUE value_list[2] = { 0 };

    BitString_Value_Init();
    status = BitString_Value_Create(instance);
    zassert_true(status, NULL);
    status = BitString_Value_Valid_Instance(instance);
    zassert_true(status, NULL);
    status = BitString_Value_Valid_Instance(instance+1);
    zassert_false(status, NULL);
    test_count = BitString_Value_Count();
    zassert_equal(test_count, 1, NULL);
    test_instance = BitString_Value_Index_To_Instance(0);
    zassert_equal(test_instance, instance, NULL);
    test_index = BitString_Value_Instance_To_Index(instance);
    zassert_equal(test_index, 0, NULL);
    bacnet_object_properties_read_write_test(
        OBJECT_BINARY_INPUT, instance, BitString_Value_Property_Lists,
        BitString_Value_Read_Property, BitString_Value_Write_Property,
        skip_fail_property_list);
    bacnet_object_name_ascii_test(
        instance,
        BitString_Value_Name_Set,
        BitString_Value_Name_ASCII);
    /* test specific WriteProperty values */
    BitString_Value_Write_Disable(instance);
    status = BitString_Value_Write_Enabled(instance);
    zassert_false(status, NULL);
    BitString_Value_Write_Enable(instance);
    status = BitString_Value_Write_Enabled(instance);
    zassert_true(status, NULL);
    wpdata.object_instance = instance;
    wpdata.object_type = OBJECT_BITSTRING_VALUE;
    wpdata.array_index = BACNET_ARRAY_ALL;
    wpdata.priority = BACNET_NO_PRIORITY;
    wpdata.error_class = ERROR_CLASS_OBJECT;
    wpdata.error_code = ERROR_CODE_UNKNOWN_OBJECT;
    /* WP to present-value */
    wpdata.object_property = PROP_PRESENT_VALUE;
    value.tag = BACNET_APPLICATION_TAG_BIT_STRING;
    bitstring_init(&value.type.Bit_String);
    wpdata.application_data_len = bacapp_encode_application_data(
        wpdata.application_data, &value);
    status = BitString_Value_Write_Property(&wpdata);
    zassert_true(status, NULL);
    /* WP to out-of-service */
    wpdata.object_property = PROP_OUT_OF_SERVICE;
    value.tag = BACNET_APPLICATION_TAG_BOOLEAN;
    value.type.Boolean = false;
    wpdata.application_data_len = bacapp_encode_application_data(
        wpdata.application_data, &value);
    status = BitString_Value_Write_Property(&wpdata);
    zassert_true(status, NULL);
    /* WP to status-flags - read-only */
    wpdata.object_property = PROP_STATUS_FLAGS;
    value.tag = BACNET_APPLICATION_TAG_BIT_STRING;
    bitstring_init(&value.type.Bit_String);
    wpdata.application_data_len = bacapp_encode_application_data(
        wpdata.application_data, &value);
    status = BitString_Value_Write_Property(&wpdata);
    zassert_false(status, NULL);
    zassert_equal(wpdata.error_class, ERROR_CLASS_PROPERTY, NULL);
    zassert_equal(wpdata.error_code, ERROR_CODE_WRITE_ACCESS_DENIED, NULL);
    /* WP to property using priority array */
    wpdata.object_property = PROP_PRESENT_VALUE;
    value.tag = BACNET_APPLICATION_TAG_BIT_STRING;
    wpdata.array_index = 0;
    wpdata.application_data_len = bacapp_encode_application_data(
        wpdata.application_data, &value);
    status = BitString_Value_Write_Property(&wpdata);
    zassert_false(status, NULL);
    zassert_equal(wpdata.error_class, ERROR_CLASS_PROPERTY, NULL);
    zassert_equal(wpdata.error_code, ERROR_CODE_PROPERTY_IS_NOT_AN_ARRAY, NULL);
    /* no application data */
    wpdata.application_data_len = 0;
    status = BitString_Value_Write_Property(&wpdata);
    zassert_false(status, NULL);
    /* NULL pointer */
    status = BitString_Value_Write_Property(NULL);
    zassert_false(status, NULL);
    /* set same value */
    BitString_Value_Change_Of_Value_Clear(instance);
    status = BitString_Value_Present_Value_Set(instance,
        &value.type.Bit_String);
    zassert_true(status, NULL);
    status = BitString_Value_Change_Of_Value(instance);
    zassert_false(status, NULL);
    /* set different value */
    bitstring_set_bit(&value.type.Bit_String, 1, true);
    status = BitString_Value_Present_Value_Set(instance,
        &value.type.Bit_String);
    zassert_true(status, NULL);
    status = BitString_Value_Change_Of_Value(instance);
    zassert_true(status, NULL);
    /* COV */
    cov_property_value_list_link(value_list,
        sizeof(value_list)/sizeof(value_list[0]));
    status = BitString_Value_Encode_Value_List(instance, value_list);
    zassert_true(status, NULL);
    /* delete */
    status = BitString_Value_Delete(instance);
    zassert_true(status, NULL);
    /* create - test that cleanup works */
    status = BitString_Value_Create(instance);
    zassert_true(status, NULL);

    return;
}
/**
 * @}
 */

#if defined(CONFIG_ZTEST_NEW_API)
ZTEST_SUITE(bitstring_value_object_tests, NULL, NULL, NULL, NULL, NULL);
#else
void test_main(void)
{
    ztest_test_suite(bitstring_value_object_tests,
     ztest_unit_test(test_BitString_Value_Object)
     );

    ztest_run_test_suite(bitstring_value_object_tests);
}
#endif
