/**
 * @file
 * @brief Unit test for object
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date July 2023
 *
 * @copyright SPDX-License-Identifier: MIT
 */
#include <zephyr/ztest.h>
#include <bacnet/basic/object/osv.h>
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
ZTEST(osv_tests, testOctetString_Value)
#else
static void testOctetString_Value(void)
#endif
{
    bool status = false;
    unsigned count = 0, index = 0;
    uint32_t object_instance = 0, test_object_instance = 0;
    const int32_t *writable_properties = NULL;
    const int32_t skip_fail_property_list[] = { -1 };

    OctetString_Value_Init();
    test_object_instance = OctetString_Value_Create(BACNET_MAX_INSTANCE + 1);
    zassert_equal(test_object_instance, BACNET_MAX_INSTANCE, NULL);
    test_object_instance = OctetString_Value_Create(BACNET_MAX_INSTANCE);
    zassert_not_equal(test_object_instance, BACNET_MAX_INSTANCE, NULL);
    status = OctetString_Value_Delete(test_object_instance);
    zassert_true(status, NULL);
    count = OctetString_Value_Count();
    zassert_true(count == 0, NULL);
    test_object_instance = OctetString_Value_Create(object_instance);
    zassert_equal(test_object_instance, object_instance, NULL);
    status = OctetString_Value_Valid_Instance(object_instance);
    zassert_true(status, NULL);
    status = OctetString_Value_Valid_Instance(object_instance - 1);
    zassert_false(status, NULL);
    index = OctetString_Value_Instance_To_Index(object_instance);
    zassert_equal(index, 0, NULL);
    test_object_instance = OctetString_Value_Index_To_Instance(index);
    zassert_equal(object_instance, test_object_instance, NULL);
    count = OctetString_Value_Count();
    zassert_true(count == 1, NULL);
    test_object_instance = OctetString_Value_Index_To_Instance(0);
    zassert_equal(object_instance, test_object_instance, NULL);
    bacnet_object_properties_read_write_test(
        OBJECT_OCTETSTRING_VALUE, object_instance,
        OctetString_Value_Property_Lists, OctetString_Value_Read_Property,
        OctetString_Value_Write_Property, skip_fail_property_list);
    status = OctetString_Value_Name_Set(object_instance, "osv-name");
    zassert_true(status, NULL);
    zassert_equal(
        strcmp(OctetString_Value_Name_ASCII(object_instance), "osv-name"), 0,
        NULL);
    status =
        OctetString_Value_Description_Set(object_instance, "osv-description");
    zassert_true(status, NULL);
    zassert_equal(
        strcmp(
            OctetString_Value_Description(object_instance), "osv-description"),
        0, NULL);
    bacnet_object_name_ascii_test(
        object_instance, OctetString_Value_Name_Set,
        OctetString_Value_Name_ASCII);
    OctetString_Value_Writable_Property_List(
        object_instance, &writable_properties);
    zassert_not_null(writable_properties, NULL);
    status = OctetString_Value_Delete(object_instance);
    zassert_true(status, NULL);
}

/**
 * @brief Regression check for CharacterString writes on Octet String Value.
 */
#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(osv_tests, testOctetString_Value_character_string_write)
#else
static void testOctetString_Value_character_string_write(void)
#endif
{
    uint32_t object_instance = 99;
    bool status = false;
    BACNET_WRITE_PROPERTY_DATA wp_data = { 0 };
    BACNET_READ_PROPERTY_DATA rp_data = { 0 };
    BACNET_APPLICATION_DATA_VALUE value = { 0 };
    BACNET_CHARACTER_STRING cstring = { 0 };
    BACNET_CHARACTER_STRING object_name = { 0 };
    uint8_t apdu[MAX_APDU] = { 0 };
    int len = 0;
    const char *test_name = "OSV-NAME-WP";
    const char *test_description =
        "Octet String Value description written via WP";

    OctetString_Value_Init();
    object_instance = OctetString_Value_Create(object_instance);
    zassert_not_equal(object_instance, BACNET_MAX_INSTANCE, NULL);

    wp_data.object_type = OBJECT_OCTETSTRING_VALUE;
    wp_data.object_instance = object_instance;
    wp_data.array_index = BACNET_ARRAY_ALL;
    wp_data.priority = BACNET_NO_PRIORITY;

    status = characterstring_init_ansi(&cstring, test_name);
    zassert_true(status, NULL);
    wp_data.object_property = PROP_OBJECT_NAME;
    wp_data.application_data_len =
        encode_application_character_string(wp_data.application_data, &cstring);
    status = OctetString_Value_Write_Property(&wp_data);
    zassert_true(status, NULL);

    status = OctetString_Value_Object_Name(object_instance, &object_name);
    zassert_true(status, NULL);
    status = characterstring_ansi_same(&object_name, test_name);
    zassert_true(status, NULL);

    rp_data.object_type = OBJECT_OCTETSTRING_VALUE;
    rp_data.object_instance = object_instance;
    rp_data.object_property = PROP_OBJECT_NAME;
    rp_data.array_index = BACNET_ARRAY_ALL;
    rp_data.application_data = apdu;
    rp_data.application_data_len = sizeof(apdu);
    len = OctetString_Value_Read_Property(&rp_data);
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
    status = OctetString_Value_Write_Property(&wp_data);
    zassert_true(status, NULL);
    zassert_not_null(OctetString_Value_Description(object_instance), NULL);
    zassert_equal(
        strcmp(
            OctetString_Value_Description(object_instance), test_description),
        0, NULL);

    rp_data.object_property = PROP_DESCRIPTION;
    len = OctetString_Value_Read_Property(&rp_data);
    zassert_true(len > 0, NULL);
    len = bacapp_decode_application_data(apdu, len, &value);
    zassert_true(len > 0, NULL);
    zassert_equal(value.tag, BACNET_APPLICATION_TAG_CHARACTER_STRING, NULL);
    status = characterstring_ansi_same(
        &value.type.Character_String, test_description);
    zassert_true(status, NULL);

    status = OctetString_Value_Delete(object_instance);
    zassert_true(status, NULL);
}
/**
 * @}
 */

#if defined(CONFIG_ZTEST_NEW_API)
ZTEST_SUITE(osv_tests, NULL, NULL, NULL, NULL, NULL);
#else
void test_main(void)
{
    ztest_test_suite(
        osv_tests, ztest_unit_test(testOctetString_Value),
        ztest_unit_test(testOctetString_Value_character_string_write));

    ztest_run_test_suite(osv_tests);
}
#endif
