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
#include <bacnet/basic/object/ao.h>
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
ZTEST(ao_tests, testAnalogOutput)
#else
static void testAnalogOutput(void)
#endif
{
    bool status = false;
    unsigned count = 0;
    uint32_t object_instance = BACNET_MAX_INSTANCE, test_object_instance = 0;
    const int32_t skip_fail_property_list[] = { -1 };

    Analog_Output_Init();
    object_instance = Analog_Output_Create(object_instance);
    count = Analog_Output_Count();
    zassert_true(count == 1, NULL);
    test_object_instance = Analog_Output_Index_To_Instance(0);
    zassert_equal(object_instance, test_object_instance, NULL);
    bacnet_object_properties_read_write_test(
        OBJECT_ANALOG_OUTPUT, object_instance, Analog_Output_Property_Lists,
        Analog_Output_Read_Property, Analog_Output_Write_Property,
        skip_fail_property_list);
    bacnet_object_name_ascii_test(
        object_instance, Analog_Output_Name_Set, Analog_Output_Name_ASCII);
    status = Analog_Output_Delete(object_instance);
    zassert_true(status, NULL);
}

/**
 * @brief Test writable PROP_OBJECT_NAME and PROP_DESCRIPTION.
 */
#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(ao_tests, testAnalogOutput_name_description_write)
#else
static void testAnalogOutput_name_description_write(void)
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
    const char *test_name = "AO-NAME-WP";
    const char *test_description = "Analog output description written via WP";

    Analog_Output_Cleanup();
    object_instance = Analog_Output_Create(BACNET_MAX_INSTANCE);
    zassert_not_equal(object_instance, BACNET_MAX_INSTANCE, NULL);

    wp_data.object_type = OBJECT_ANALOG_OUTPUT;
    wp_data.object_instance = object_instance;
    wp_data.array_index = BACNET_ARRAY_ALL;
    wp_data.priority = BACNET_NO_PRIORITY;

    status = characterstring_init_ansi(&cstring, test_name);
    zassert_true(status, NULL);
    wp_data.object_property = PROP_OBJECT_NAME;
    wp_data.application_data_len =
        encode_application_character_string(wp_data.application_data, &cstring);
    status = Analog_Output_Write_Property(&wp_data);
    zassert_true(status, NULL);

    status = Analog_Output_Object_Name(object_instance, &object_name);
    zassert_true(status, NULL);
    status = characterstring_ansi_same(&object_name, test_name);
    zassert_true(status, NULL);

    rp_data.object_type = OBJECT_ANALOG_OUTPUT;
    rp_data.object_instance = object_instance;
    rp_data.object_property = PROP_OBJECT_NAME;
    rp_data.array_index = BACNET_ARRAY_ALL;
    rp_data.application_data = apdu;
    rp_data.application_data_len = sizeof(apdu);
    len = Analog_Output_Read_Property(&rp_data);
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
    status = Analog_Output_Write_Property(&wp_data);
    zassert_true(status, NULL);
    zassert_not_null(Analog_Output_Description(object_instance), NULL);
    zassert_equal(
        strcmp(Analog_Output_Description(object_instance), test_description), 0,
        NULL);

    rp_data.object_property = PROP_DESCRIPTION;
    len = Analog_Output_Read_Property(&rp_data);
    zassert_true(len > 0, NULL);
    len = bacapp_decode_application_data(apdu, len, &value);
    zassert_true(len > 0, NULL);
    zassert_equal(value.tag, BACNET_APPLICATION_TAG_CHARACTER_STRING, NULL);
    status = characterstring_ansi_same(
        &value.type.Character_String, test_description);
    zassert_true(status, NULL);

    status = Analog_Output_Delete(object_instance);
    zassert_true(status, NULL);
    Analog_Output_Cleanup();
}

/**
 * @brief Test Analog Output Writable_Property_List API
 */
#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(ao_tests, testAnalogOutput_Writable_Properties)
#else
static void testAnalogOutput_Writable_Properties(void)
#endif
{
    const uint32_t instance = 456;
    const uint32_t invalid_instance = instance + 1;
    const int32_t *properties = NULL;
    uint32_t count = 0;

    Analog_Output_Init();
    zassert_not_equal(
        Analog_Output_Create(instance), BACNET_MAX_INSTANCE, NULL);

    /* valid instance: list is non-NULL and -1-terminated */
    Analog_Output_Writable_Property_List(instance, &properties);
    zassert_not_null(properties, NULL);
    count = property_list_count(properties);
    zassert_true(count > 0, NULL);

    /* unknown instance: must still return a valid list, not NULL/garbage */
    properties = NULL;
    Analog_Output_Writable_Property_List(invalid_instance, &properties);
    zassert_not_null(properties, NULL);

    /* NULL properties pointer: must not crash */
    Analog_Output_Writable_Property_List(instance, NULL);

    Analog_Output_Delete(instance);
    Analog_Output_Cleanup();
}
/**
 * @}
 */

#if defined(CONFIG_ZTEST_NEW_API)
ZTEST_SUITE(ao_tests, NULL, NULL, NULL, NULL, NULL);
#else
void test_main(void)
{
    ztest_test_suite(
        ao_tests, ztest_unit_test(testAnalogOutput),
        ztest_unit_test(testAnalogOutput_name_description_write),
        ztest_unit_test(testAnalogOutput_Writable_Properties));

    ztest_run_test_suite(ao_tests);
}
#endif
