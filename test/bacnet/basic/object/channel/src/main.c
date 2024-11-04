/**
 * @file
 * @brief Unit test for object
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date July 2023
 * @section LICENSE
 *
 * SPDX-License-Identifier: MIT
 */
#include <zephyr/ztest.h>
#include <bacnet/basic/object/channel.h>
#include <bacnet/bactext.h>
#include <property_test.h>

/**
 * @addtogroup bacnet_tests
 * @{
 */

/**
 * @brief Test
 */
static void test_Channel_Property_Read_Write(void)
{
    const uint32_t instance = 123;
    unsigned count = 0;
    unsigned index = 0;
    const char *sample_name = "Channel:0";
    const char *test_name = NULL;
    uint32_t test_instance = 0;
    bool status = false;
    const int skip_fail_property_list[] = { -1 };
    BACNET_CHANNEL_VALUE channel_value = { 0 };
    BACNET_WRITE_PROPERTY_DATA wp_data = { 0 };
    BACNET_APPLICATION_DATA_VALUE value = { 0 };
    BACNET_DEVICE_OBJECT_PROPERTY_REFERENCE member = { 0 };

    Channel_Init();
    Channel_Create(instance);
    status = Channel_Valid_Instance(instance);
    zassert_true(status, NULL);
    status = Channel_Valid_Instance(instance - 1);
    zassert_false(status, NULL);
    index = Channel_Instance_To_Index(instance);
    zassert_equal(index, 0, NULL);
    test_instance = Channel_Index_To_Instance(index);
    zassert_equal(instance, test_instance, NULL);
    count = Channel_Count();
    zassert_true(count > 0, NULL);
    /* configure the instance property values and test API for lists */
    member.deviceIdentifier.type = OBJECT_DEVICE;
    member.deviceIdentifier.instance = 0;
    member.objectIdentifier.type = OBJECT_ANALOG_OUTPUT;
    member.objectIdentifier.instance = 1;
    member.propertyIdentifier = PROP_PRESENT_VALUE;
    member.arrayIndex = BACNET_ARRAY_ALL;
    index = Channel_Reference_List_Member_Element_Add(instance, &member);
    zassert_not_equal(index, 0, NULL);
    status =
        Channel_Reference_List_Member_Element_Set(instance, index, &member);
    zassert_true(status, NULL);
    status = Channel_Control_Groups_Element_Set(instance, 1, 1);
    zassert_true(status, NULL);
    member.deviceIdentifier.type = OBJECT_DEVICE;
    member.deviceIdentifier.instance = 0;
    member.objectIdentifier.type = OBJECT_BINARY_OUTPUT;
    member.objectIdentifier.instance = 1;
    member.propertyIdentifier = PROP_PRESENT_VALUE;
    member.arrayIndex = BACNET_ARRAY_ALL;
    index = Channel_Reference_List_Member_Element_Add(instance, &member);
    zassert_not_equal(index, 0, NULL);
    member.deviceIdentifier.type = OBJECT_DEVICE;
    member.deviceIdentifier.instance = 0;
    member.objectIdentifier.type = OBJECT_MULTI_STATE_OUTPUT;
    member.objectIdentifier.instance = 1;
    member.propertyIdentifier = PROP_PRESENT_VALUE;
    member.arrayIndex = BACNET_ARRAY_ALL;
    index = Channel_Reference_List_Member_Element_Add(instance, &member);
    zassert_not_equal(index, 0, NULL);
    member.deviceIdentifier.type = OBJECT_DEVICE;
    member.deviceIdentifier.instance = 0;
    member.objectIdentifier.type = OBJECT_LIGHTING_OUTPUT;
    member.objectIdentifier.instance = 1;
    member.propertyIdentifier = PROP_PRESENT_VALUE;
    member.arrayIndex = BACNET_ARRAY_ALL;
    index = Channel_Reference_List_Member_Element_Add(instance, &member);
    zassert_not_equal(index, 0, NULL);
    member.deviceIdentifier.type = OBJECT_DEVICE;
    member.deviceIdentifier.instance = 0;
    member.objectIdentifier.type = OBJECT_COLOR;
    member.objectIdentifier.instance = 1;
    member.propertyIdentifier = PROP_PRESENT_VALUE;
    member.arrayIndex = BACNET_ARRAY_ALL;
    index = Channel_Reference_List_Member_Element_Add(instance, &member);
    zassert_not_equal(index, 0, NULL);
    member.deviceIdentifier.type = OBJECT_DEVICE;
    member.deviceIdentifier.instance = 0;
    member.objectIdentifier.type = OBJECT_COLOR_TEMPERATURE;
    member.objectIdentifier.instance = 1;
    member.propertyIdentifier = PROP_PRESENT_VALUE;
    member.arrayIndex = BACNET_ARRAY_ALL;
    index = Channel_Reference_List_Member_Element_Add(instance, &member);
    zassert_not_equal(index, 0, NULL);
    /* perform a general test for RP/WP */
    bacnet_object_properties_read_write_test(
        OBJECT_CHANNEL, instance, Channel_Property_Lists, Channel_Read_Property,
        Channel_Write_Property, skip_fail_property_list);
    /* test the ASCII name get/set */
    status = Channel_Name_Set(instance, sample_name);
    zassert_true(status, NULL);
    test_name = Channel_Name_ASCII(instance);
    zassert_equal(test_name, sample_name, NULL);
    status = Channel_Name_Set(instance, NULL);
    zassert_true(status, NULL);
    test_name = Channel_Name_ASCII(instance);
    zassert_equal(test_name, NULL, NULL);

    /* test specific WriteProperty values - common configuration */
    wp_data.object_type = OBJECT_CHANNEL;
    wp_data.object_instance = instance;
    wp_data.array_index = BACNET_ARRAY_ALL;
    wp_data.priority = BACNET_MAX_PRIORITY;
    /* specific WriteProperty value */
    wp_data.object_property = PROP_PRESENT_VALUE;
    value.tag = BACNET_APPLICATION_TAG_CHANNEL_VALUE;
    value.type.Channel_Value.tag = BACNET_APPLICATION_TAG_REAL;
    value.type.Channel_Value.type.Real = 3.14159;
    wp_data.application_data_len =
        bacapp_encode_application_data(wp_data.application_data, &value);
    status = Channel_Write_Property(&wp_data);
    zassert_true(status, NULL);
    /* specific WriteProperty value */
    wp_data.object_property = PROP_OUT_OF_SERVICE;
    value.tag = BACNET_APPLICATION_TAG_BOOLEAN;
    value.type.Boolean = true;
    wp_data.application_data_len =
        bacapp_encode_application_data(wp_data.application_data, &value);
    status = Channel_Write_Property(&wp_data);
    zassert_true(status, NULL);
    /* specific WriteProperty value */
    wp_data.object_property = PROP_CHANNEL_NUMBER;
    value.tag = BACNET_APPLICATION_TAG_UNSIGNED_INT;
    value.type.Unsigned_Int = 123;
    wp_data.application_data_len =
        bacapp_encode_application_data(wp_data.application_data, &value);
    status = Channel_Write_Property(&wp_data);
    zassert_true(status, NULL);
    value.type.Unsigned_Int = UINT16_MAX + 1;
    wp_data.application_data_len =
        bacapp_encode_application_data(wp_data.application_data, &value);
    status = Channel_Write_Property(&wp_data);
    zassert_false(status, NULL);
    /* specific WriteProperty value */
    wp_data.object_property = PROP_CONTROL_GROUPS;
    wp_data.array_index = 1;
    value.tag = BACNET_APPLICATION_TAG_UNSIGNED_INT;
    /* min valid value */
    value.type.Unsigned_Int = 0;
    wp_data.application_data_len =
        bacapp_encode_application_data(wp_data.application_data, &value);
    status = Channel_Write_Property(&wp_data);
    zassert_true(status, NULL);
    /* max valid value */
    value.type.Unsigned_Int = UINT16_MAX;
    wp_data.application_data_len =
        bacapp_encode_application_data(wp_data.application_data, &value);
    status = Channel_Write_Property(&wp_data);
    zassert_true(status, NULL);
    /* array size - read-only */
    wp_data.array_index = 0;
    status = Channel_Write_Property(&wp_data);
    zassert_false(status, NULL);
    /* out-of-range value */
    wp_data.array_index = 1;
    value.type.Unsigned_Int = UINT16_MAX + 1;
    wp_data.application_data_len =
        bacapp_encode_application_data(wp_data.application_data, &value);
    status = Channel_Write_Property(&wp_data);
    zassert_false(status, NULL);
    /* invalid data type for Array element */
    wp_data.array_index = 1;
    value.type.Real = 3.14159f;
    value.tag = BACNET_APPLICATION_TAG_REAL;
    wp_data.application_data_len =
        bacapp_encode_application_data(wp_data.application_data, &value);
    status = Channel_Write_Property(&wp_data);
    zassert_false(status, NULL);
    /* invalid data type for Array size */
    wp_data.array_index = 0;
    status = Channel_Write_Property(&wp_data);
    zassert_false(status, NULL);
    /* invalid array-index - probably */
    wp_data.array_index = BACNET_ARRAY_ALL - 1;
    value.tag = BACNET_APPLICATION_TAG_UNSIGNED_INT;
    value.type.Unsigned_Int = 0;
    wp_data.application_data_len =
        bacapp_encode_application_data(wp_data.application_data, &value);
    status = Channel_Write_Property(&wp_data);
    zassert_false(status, NULL);
    /* specific WriteProperty value */
    wp_data.array_index = 1;
    wp_data.priority = BACNET_MAX_PRIORITY;
    wp_data.object_property = PROP_LIST_OF_OBJECT_PROPERTY_REFERENCES;
    value.tag = BACNET_APPLICATION_TAG_DEVICE_OBJECT_PROPERTY_REFERENCE;
    value.type.Device_Object_Property_Reference.objectIdentifier.type =
        OBJECT_ANALOG_OUTPUT;
    value.type.Device_Object_Property_Reference.objectIdentifier.instance = 1;
    value.type.Device_Object_Property_Reference.propertyIdentifier =
        PROP_PRESENT_VALUE;
    value.type.Device_Object_Property_Reference.arrayIndex = BACNET_ARRAY_ALL;
    wp_data.application_data_len =
        bacapp_encode_application_data(wp_data.application_data, &value);
    status = Channel_Write_Property(&wp_data);
    zassert_true(status, NULL);
    wp_data.array_index = 0;
    status = Channel_Write_Property(&wp_data);
    zassert_false(status, NULL);
    wp_data.array_index = BACNET_ARRAY_ALL - 1;
    status = Channel_Write_Property(&wp_data);
    zassert_false(status, NULL);
    /* read-only property */
    wp_data.array_index = BACNET_ARRAY_ALL;
    wp_data.priority = BACNET_MAX_PRIORITY;
    wp_data.object_property = PROP_OBJECT_TYPE;
    value.tag = BACNET_APPLICATION_TAG_ENUMERATED;
    value.type.Enumerated = OBJECT_ANALOG_INPUT;
    wp_data.application_data_len =
        bacapp_encode_application_data(wp_data.application_data, &value);
    status = Channel_Write_Property(&wp_data);
    zassert_equal(wp_data.error_class, ERROR_CLASS_PROPERTY, NULL);
    zassert_equal(wp_data.error_code, ERROR_CODE_WRITE_ACCESS_DENIED, NULL);
    zassert_false(status, NULL);
    /* present-value API */
    channel_value.tag = BACNET_APPLICATION_TAG_REAL;
    channel_value.type.Real = 3.14159f;
    status = Channel_Present_Value_Set(instance, 1, &channel_value);
    zassert_true(status, NULL);
    /* cleanup */
    status = Channel_Delete(instance);
    zassert_true(status, NULL);
    Channel_Cleanup();
}
/**
 * @}
 */

void test_main(void)
{
    ztest_test_suite(
        channel_tests, ztest_unit_test(test_Channel_Property_Read_Write));

    ztest_run_test_suite(channel_tests);
}
