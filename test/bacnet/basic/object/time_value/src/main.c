/**
 * @file
 * @brief Unit test for BACnet Time Value object encode/decode APIs
 * @author Mikhail Antropov <michail.antropov@dsr-corporation.com>
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date June 2023
 *
 * @copyright SPDX-License-Identifier: MIT
 */
#include <zephyr/ztest.h>
#include <bacnet/basic/object/time_value.h>
#include <bacnet/bactext.h>
#include <bacnet/proplist.h>
#include <property_test.h>

/**
 * @addtogroup bacnet_tests
 * @{
 */

/**
 * @brief Test Time Value handling
 */
#ifdef CONFIG_ZTEST_NEW_API
ZTEST(bacnet_tv, testTimeValue)
#else
static void testTimeValue(void)
#endif
{
    bool status = false;
    unsigned count = 0;
    uint32_t object_instance = 0;
    const int32_t skip_fail_property_list[] = { -1 };

    Time_Value_Init();
    object_instance = Time_Value_Create(BACNET_MAX_INSTANCE);
    count = Time_Value_Count();
    zassert_true(count > 0, NULL);
    object_instance = Time_Value_Index_To_Instance(0);
    bacnet_object_properties_read_write_test(
        OBJECT_TIME_VALUE, object_instance, Time_Value_Property_Lists,
        Time_Value_Read_Property, Time_Value_Write_Property,
        skip_fail_property_list);
    bacnet_object_name_ascii_test(
        object_instance, Time_Value_Name_Set, Time_Value_Name_ASCII);
    /* check the delete function */
    status = Time_Value_Delete(object_instance);
    zassert_true(status, NULL);
}
/**
 * @}
 */

/**
 * @brief Test Time Value Writable_Property_List and Write_Enabled APIs
 */
#ifdef CONFIG_ZTEST_NEW_API
ZTEST(bacnet_tv, testTimeValue_Writable_Properties)
#else
static void testTimeValue_Writable_Properties(void)
#endif
{
    const uint32_t instance = 456;
    const uint32_t invalid_instance = instance + 1;
    const int32_t *properties = NULL;
    uint32_t count = 0;

    Time_Value_Init();
    zassert_not_equal(Time_Value_Create(instance), BACNET_MAX_INSTANCE, NULL);

    /* write-enabled (default): list starts with PROP_PRESENT_VALUE */
    zassert_true(Time_Value_Write_Enabled(instance), NULL);
    Time_Value_Writable_Property_List(instance, &properties);
    zassert_not_null(properties, NULL);
    count = property_list_count(properties);
    zassert_true(count > 0, NULL);
    zassert_equal(properties[0], PROP_PRESENT_VALUE, NULL);

    /* write-disabled: list skips PROP_PRESENT_VALUE */
    Time_Value_Write_Disable(instance);
    zassert_false(Time_Value_Write_Enabled(instance), NULL);
    Time_Value_Writable_Property_List(instance, &properties);
    zassert_not_null(properties, NULL);
    zassert_not_equal(properties[0], PROP_PRESENT_VALUE, NULL);

    /* write re-enabled: PROP_PRESENT_VALUE back at head */
    Time_Value_Write_Enable(instance);
    zassert_true(Time_Value_Write_Enabled(instance), NULL);
    Time_Value_Writable_Property_List(instance, &properties);
    zassert_equal(properties[0], PROP_PRESENT_VALUE, NULL);

    /* name and description are writable properties */
    zassert_true(property_list_member(properties, PROP_OBJECT_NAME), NULL);
    zassert_true(property_list_member(properties, PROP_DESCRIPTION), NULL);

    /* unknown instance: must return a valid list, not NULL/garbage */
    properties = NULL;
    Time_Value_Writable_Property_List(invalid_instance, &properties);
    zassert_not_null(properties, NULL);
    count = property_list_count(properties);
    zassert_true(count > 0, NULL);

    /* NULL properties pointer: must not crash */
    Time_Value_Writable_Property_List(instance, NULL);

    Time_Value_Delete(instance);
    Time_Value_Cleanup();
}
/**
 * @}
 */

/**
 * @brief Test writable PROP_OBJECT_NAME and PROP_DESCRIPTION.
 */
#ifdef CONFIG_ZTEST_NEW_API
ZTEST(bacnet_tv, testTimeValue_name_description_write)
#else
static void testTimeValue_name_description_write(void)
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
    const char *test_name = "TIME-VALUE-NAME-WP";
    const char *test_description = "Time value description written via WP";

    Time_Value_Cleanup();
    object_instance = Time_Value_Create(BACNET_MAX_INSTANCE);
    zassert_not_equal(object_instance, BACNET_MAX_INSTANCE, NULL);

    wp_data.object_type = OBJECT_TIME_VALUE;
    wp_data.object_instance = object_instance;
    wp_data.array_index = BACNET_ARRAY_ALL;
    wp_data.priority = BACNET_NO_PRIORITY;

    status = characterstring_init_ansi(&cstring, test_name);
    zassert_true(status, NULL);
    wp_data.object_property = PROP_OBJECT_NAME;
    wp_data.application_data_len =
        encode_application_character_string(wp_data.application_data, &cstring);
    status = Time_Value_Write_Property(&wp_data);
    zassert_true(status, NULL);

    status = Time_Value_Object_Name(object_instance, &object_name);
    zassert_true(status, NULL);
    status = characterstring_ansi_same(&object_name, test_name);
    zassert_true(status, NULL);

    rp_data.object_type = OBJECT_TIME_VALUE;
    rp_data.object_instance = object_instance;
    rp_data.object_property = PROP_OBJECT_NAME;
    rp_data.array_index = BACNET_ARRAY_ALL;
    rp_data.application_data = apdu;
    rp_data.application_data_len = sizeof(apdu);
    len = Time_Value_Read_Property(&rp_data);
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
    status = Time_Value_Write_Property(&wp_data);
    zassert_true(status, NULL);
    zassert_not_null(Time_Value_Description(object_instance), NULL);
    zassert_equal(
        strcmp(Time_Value_Description(object_instance), test_description), 0,
        NULL);

    rp_data.object_property = PROP_DESCRIPTION;
    len = Time_Value_Read_Property(&rp_data);
    zassert_true(len > 0, NULL);
    len = bacapp_decode_application_data(apdu, len, &value);
    zassert_true(len > 0, NULL);
    zassert_equal(value.tag, BACNET_APPLICATION_TAG_CHARACTER_STRING, NULL);
    status = characterstring_ansi_same(
        &value.type.Character_String, test_description);
    zassert_true(status, NULL);

    status = Time_Value_Delete(object_instance);
    zassert_true(status, NULL);
    Time_Value_Cleanup();
}

#ifdef CONFIG_ZTEST_NEW_API
ZTEST_SUITE(bacnet_tv, NULL, NULL, NULL, NULL, NULL);
#else
void test_main(void)
{
    ztest_test_suite(
        tv_tests, ztest_unit_test(testTimeValue),
        ztest_unit_test(testTimeValue_Writable_Properties),
        ztest_unit_test(testTimeValue_name_description_write));

    ztest_run_test_suite(tv_tests);
}
#endif
