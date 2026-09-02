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
ZTEST(bitstring_value_object_tests, test_BitString_Value_Object)
#else
static void test_BitString_Value_Object(void)
#endif
{
    const int32_t skip_fail_property_list[] = { -1 };
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
    status = BitString_Value_Valid_Instance(instance + 1);
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
        instance, BitString_Value_Name_Set, BitString_Value_Name_ASCII);
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
    wpdata.application_data_len =
        bacapp_encode_application_data(wpdata.application_data, &value);
    status = BitString_Value_Write_Property(&wpdata);
    zassert_true(status, NULL);
    /* WP to out-of-service */
    wpdata.object_property = PROP_OUT_OF_SERVICE;
    value.tag = BACNET_APPLICATION_TAG_BOOLEAN;
    value.type.Boolean = false;
    wpdata.application_data_len =
        bacapp_encode_application_data(wpdata.application_data, &value);
    status = BitString_Value_Write_Property(&wpdata);
    zassert_true(status, NULL);
    /* WP to status-flags - read-only */
    wpdata.object_property = PROP_STATUS_FLAGS;
    value.tag = BACNET_APPLICATION_TAG_BIT_STRING;
    bitstring_init(&value.type.Bit_String);
    wpdata.application_data_len =
        bacapp_encode_application_data(wpdata.application_data, &value);
    status = BitString_Value_Write_Property(&wpdata);
    zassert_false(status, NULL);
    zassert_equal(wpdata.error_class, ERROR_CLASS_PROPERTY, NULL);
    zassert_equal(wpdata.error_code, ERROR_CODE_WRITE_ACCESS_DENIED, NULL);
    /* no application data */
    wpdata.application_data_len = 0;
    status = BitString_Value_Write_Property(&wpdata);
    zassert_false(status, NULL);
    /* NULL pointer */
    status = BitString_Value_Write_Property(NULL);
    zassert_false(status, NULL);
    /* set same value */
    BitString_Value_Change_Of_Value_Clear(instance);
    status =
        BitString_Value_Present_Value_Set(instance, &value.type.Bit_String);
    zassert_true(status, NULL);
    status = BitString_Value_Change_Of_Value(instance);
    zassert_false(status, NULL);
    /* set different value */
    bitstring_set_bit(&value.type.Bit_String, 1, true);
    status =
        BitString_Value_Present_Value_Set(instance, &value.type.Bit_String);
    zassert_true(status, NULL);
    status = BitString_Value_Change_Of_Value(instance);
    zassert_true(status, NULL);
    /* COV */
    cov_property_value_list_link(
        value_list, sizeof(value_list) / sizeof(value_list[0]));
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
 * @brief Test writable PROP_OBJECT_NAME and PROP_DESCRIPTION.
 */
#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(bitstring_value_object_tests, test_BitString_Value_name_description_write)
#else
static void test_BitString_Value_name_description_write(void)
#endif
{
    uint32_t object_instance = BACNET_MAX_INSTANCE;
    bool status = false;
    int len = 0;
    BACNET_WRITE_PROPERTY_DATA wp_data = { 0 };
    BACNET_READ_PROPERTY_DATA rp_data = { 0 };
    BACNET_APPLICATION_DATA_VALUE value = { 0 };
    BACNET_CHARACTER_STRING cstring = { 0 };
    BACNET_CHARACTER_STRING object_name = { 0 };
    uint8_t apdu[MAX_APDU] = { 0 };
    const char *test_name = "BITSTRING-VALUE-NAME-WP";
    const char *test_description = "BitString Value description written via WP";

    BitString_Value_Cleanup();
    object_instance = BitString_Value_Create(BACNET_MAX_INSTANCE);
    zassert_not_equal(object_instance, BACNET_MAX_INSTANCE, NULL);

    wp_data.object_type = OBJECT_BITSTRING_VALUE;
    wp_data.object_instance = object_instance;
    wp_data.array_index = BACNET_ARRAY_ALL;
    wp_data.priority = BACNET_NO_PRIORITY;

    status = characterstring_init_ansi(&cstring, test_name);
    zassert_true(status, NULL);
    wp_data.object_property = PROP_OBJECT_NAME;
    wp_data.application_data_len =
        encode_application_character_string(wp_data.application_data, &cstring);
    status = BitString_Value_Write_Property(&wp_data);
    zassert_true(status, NULL);

    status = BitString_Value_Object_Name(object_instance, &object_name);
    zassert_true(status, NULL);
    status = characterstring_ansi_same(&object_name, test_name);
    zassert_true(status, NULL);

    rp_data.object_type = OBJECT_BITSTRING_VALUE;
    rp_data.object_instance = object_instance;
    rp_data.object_property = PROP_OBJECT_NAME;
    rp_data.array_index = BACNET_ARRAY_ALL;
    rp_data.application_data = apdu;
    rp_data.application_data_len = sizeof(apdu);
    len = BitString_Value_Read_Property(&rp_data);
    zassert_true(len > 0, NULL);
    len = bacapp_decode_application_data(apdu, len, &value);
    zassert_true(len > 0, NULL);
    zassert_equal(value.tag, BACNET_APPLICATION_TAG_CHARACTER_STRING, NULL);
    status = characterstring_ansi_same(&value.type.Character_String, test_name);
    zassert_true(status, NULL);

    status = characterstring_init_ansi(&cstring, test_description);
    zassert_true(status, NULL);
    wp_data.object_property = PROP_DESCRIPTION;
    wp_data.application_data_len =
        encode_application_character_string(wp_data.application_data, &cstring);
    status = BitString_Value_Write_Property(&wp_data);
    zassert_true(status, NULL);
    zassert_not_null(BitString_Value_Description(object_instance), NULL);
    zassert_equal(
        strcmp(BitString_Value_Description(object_instance), test_description),
        0, NULL);

    rp_data.object_property = PROP_DESCRIPTION;
    len = BitString_Value_Read_Property(&rp_data);
    zassert_true(len > 0, NULL);
    len = bacapp_decode_application_data(apdu, len, &value);
    zassert_true(len > 0, NULL);
    zassert_equal(value.tag, BACNET_APPLICATION_TAG_CHARACTER_STRING, NULL);
    status = characterstring_ansi_same(
        &value.type.Character_String, test_description);
    zassert_true(status, NULL);

    status = BitString_Value_Delete(object_instance);
    zassert_true(status, NULL);
    BitString_Value_Cleanup();
}

/**
 * @brief Test BitString Value Writable_Property_List API
 */
#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(bitstring_value_object_tests, test_BitString_Value_Writable_Properties)
#else
static void test_BitString_Value_Writable_Properties(void)
#endif
{
    const uint32_t instance = 456;
    const uint32_t invalid_instance = instance + 1;
    const int32_t *properties = NULL;
    uint32_t count = 0;
    bool status = false;

    BitString_Value_Init();
    status = BitString_Value_Create(instance);
    zassert_true(status, NULL);

    /* write-enabled (default): list starts with PROP_PRESENT_VALUE */
    zassert_true(BitString_Value_Write_Enabled(instance), NULL);
    BitString_Value_Writable_Property_List(instance, &properties);
    zassert_not_null(properties, NULL);
    count = property_list_count(properties);
    zassert_true(count > 0, NULL);
    zassert_true(property_list_member(properties, PROP_PRESENT_VALUE), NULL);

    /* write-disabled: list skips PROP_PRESENT_VALUE */
    BitString_Value_Write_Disable(instance);
    zassert_false(BitString_Value_Write_Enabled(instance), NULL);
    BitString_Value_Writable_Property_List(instance, &properties);
    zassert_not_null(properties, NULL);
    zassert_false(property_list_member(properties, PROP_PRESENT_VALUE), NULL);

    /* write re-enabled: PROP_PRESENT_VALUE back at head */
    BitString_Value_Write_Enable(instance);
    zassert_true(BitString_Value_Write_Enabled(instance), NULL);
    BitString_Value_Writable_Property_List(instance, &properties);
    zassert_true(property_list_member(properties, PROP_PRESENT_VALUE), NULL);

    /* unknown instance: must return a valid list, not NULL/garbage */
    properties = NULL;
    BitString_Value_Writable_Property_List(invalid_instance, &properties);
    zassert_not_null(properties, NULL);
    count = property_list_count(properties);
    zassert_true(count > 0, NULL);

    /* NULL properties pointer: must not crash */
    BitString_Value_Writable_Property_List(instance, NULL);

    BitString_Value_Delete(instance);
    BitString_Value_Cleanup();
}
/**
 * @}
 */

#if defined(CONFIG_ZTEST_NEW_API)
ZTEST_SUITE(bitstring_value_object_tests, NULL, NULL, NULL, NULL, NULL);
#else
void test_main(void)
{
    ztest_test_suite(
        bitstring_value_object_tests,
        ztest_unit_test(test_BitString_Value_Object),
        ztest_unit_test(test_BitString_Value_name_description_write),
        ztest_unit_test(test_BitString_Value_Writable_Properties));

    ztest_run_test_suite(bitstring_value_object_tests);
}
#endif
