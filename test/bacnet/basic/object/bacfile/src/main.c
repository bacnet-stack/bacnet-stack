/**
 * @file
 * @brief Test for BACnet File object
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date January 2023
 * @section LICENSE
 *
 * Copyright (C) 2022 Steve Karg <skarg@users.sourceforge.net>
 *
 * @copyright SPDX-License-Identifier: MIT
 */
#include <zephyr/ztest.h>
#include <bacnet/basic/object/bacfile.h>

/**
 * @addtogroup bacnet_tests
 * @{
 */

/**
 * @brief Test
 */
#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(bacfile_tests, test_BACnet_File_Object)
#else
static void test_BACnet_File_Object(void)
#endif
{
    uint8_t apdu[MAX_APDU] = { 0 };
    int len = 0, test_len = 0;
    BACNET_READ_PROPERTY_DATA rpdata = { 0 };
    BACNET_APPLICATION_DATA_VALUE value = { 0 };
    const int32_t *required_property = NULL;
    const uint32_t instance = 1;

    bacfile_init();
    bacfile_create(1);
    rpdata.application_data = &apdu[0];
    rpdata.application_data_len = sizeof(apdu);
    rpdata.object_type = OBJECT_FILE;
    rpdata.object_instance = instance;
    rpdata.array_index = BACNET_ARRAY_ALL;

    BACfile_Property_Lists(&required_property, NULL, NULL);
    while ((*required_property) >= 0) {
        rpdata.object_property = *required_property;
        len = bacfile_read_property(&rpdata);
        if (len < 0) {
            printf(
                "property %u: failed to read!\n",
                (unsigned)rpdata.object_property);
        }
        zassert_true(len >= 0, NULL);
        if (len >= 0) {
            test_len = bacapp_decode_known_property(
                rpdata.application_data, len, &value, rpdata.object_type,
                rpdata.object_property);
            if (len != test_len) {
                printf(
                    "property %u: failed to decode!\n",
                    (unsigned)rpdata.object_property);
            }
            zassert_equal(len, test_len, NULL);
        }
        required_property++;
    }

    bacfile_cleanup();

    return;
}

/**
 * @brief Test writable PROP_OBJECT_NAME, PROP_DESCRIPTION, and PROP_FILE_TYPE.
 */
#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(bacfile_tests, test_BACnet_File_Object_name_description_file_type_write)
#else
static void test_BACnet_File_Object_name_description_file_type_write(void)
#endif
{
    uint32_t object_instance = 1;
    uint8_t apdu[MAX_APDU] = { 0 };
    BACNET_WRITE_PROPERTY_DATA wp_data = { 0 };
    BACNET_READ_PROPERTY_DATA rp_data = { 0 };
    BACNET_APPLICATION_DATA_VALUE value = { 0 };
    BACNET_CHARACTER_STRING char_string = { 0 };
    BACNET_CHARACTER_STRING object_name = { 0 };
    bool status = false;
    int len = 0;
    const char *test_name = "FILE-NAME-WP";
    const char *test_description = "/tmp/file-object.txt";
    const char *test_file_type = "text/plain";

    bacfile_init();
    object_instance = bacfile_create(object_instance);
    zassert_not_equal(object_instance, BACNET_MAX_INSTANCE, NULL);

    rp_data.object_type = OBJECT_FILE;
    rp_data.object_instance = object_instance;
    rp_data.array_index = BACNET_ARRAY_ALL;
    rp_data.application_data = apdu;
    rp_data.application_data_len = sizeof(apdu);
    rp_data.object_property = PROP_DESCRIPTION;
    len = bacfile_read_property(&rp_data);
    zassert_true(len > 0, NULL);
    len = bacapp_decode_application_data(apdu, len, &value);
    zassert_true(len > 0, NULL);
    zassert_equal(value.tag, BACNET_APPLICATION_TAG_CHARACTER_STRING, NULL);
    zassert_equal(value.type.Character_String.length, 0U, NULL);

    wp_data.object_type = OBJECT_FILE;
    wp_data.object_instance = object_instance;
    wp_data.array_index = BACNET_ARRAY_ALL;
    wp_data.priority = BACNET_NO_PRIORITY;

    status = characterstring_init_ansi(&char_string, test_name);
    zassert_true(status, NULL);
    wp_data.object_property = PROP_OBJECT_NAME;
    wp_data.application_data_len = encode_application_character_string(
        wp_data.application_data, &char_string);
    status = bacfile_write_property(&wp_data);
    zassert_true(status, NULL);

    status = bacfile_object_name(object_instance, &object_name);
    zassert_true(status, NULL);
    status = characterstring_ansi_same(&object_name, test_name);
    zassert_true(status, NULL);

    rp_data.object_type = OBJECT_FILE;
    rp_data.object_instance = object_instance;
    rp_data.object_property = PROP_OBJECT_NAME;
    rp_data.array_index = BACNET_ARRAY_ALL;
    rp_data.application_data = apdu;
    rp_data.application_data_len = sizeof(apdu);
    len = bacfile_read_property(&rp_data);
    zassert_true(len > 0, NULL);
    len = bacapp_decode_application_data(apdu, len, &value);
    zassert_true(len > 0, NULL);
    zassert_equal(value.tag, BACNET_APPLICATION_TAG_CHARACTER_STRING, NULL);
    status = characterstring_ansi_same(&value.type.Character_String, test_name);
    zassert_true(status, NULL);

    status = characterstring_init_ansi(&char_string, test_description);
    zassert_true(status, NULL);
    wp_data.object_property = PROP_DESCRIPTION;
    wp_data.application_data_len = encode_application_character_string(
        wp_data.application_data, &char_string);
    status = bacfile_write_property(&wp_data);
    zassert_true(status, NULL);

    rp_data.object_property = PROP_DESCRIPTION;
    rp_data.application_data = apdu;
    rp_data.application_data_len = sizeof(apdu);
    len = bacfile_read_property(&rp_data);
    zassert_true(len > 0, NULL);
    len = bacapp_decode_application_data(apdu, len, &value);
    zassert_true(len > 0, NULL);
    zassert_equal(value.tag, BACNET_APPLICATION_TAG_CHARACTER_STRING, NULL);
    status = characterstring_ansi_same(
        &value.type.Character_String, test_description);
    zassert_true(status, NULL);

    status = characterstring_init_ansi(&char_string, test_file_type);
    zassert_true(status, NULL);
    wp_data.object_property = PROP_FILE_TYPE;
    wp_data.application_data_len = encode_application_character_string(
        wp_data.application_data, &char_string);
    status = bacfile_write_property(&wp_data);
    zassert_true(status, NULL);
    zassert_not_null(bacfile_file_type(object_instance), NULL);
    zassert_true(
        strcmp(bacfile_file_type(object_instance), test_file_type) == 0, NULL);

    rp_data.object_property = PROP_FILE_TYPE;
    rp_data.application_data = apdu;
    rp_data.application_data_len = sizeof(apdu);
    len = bacfile_read_property(&rp_data);
    zassert_true(len > 0, NULL);
    len = bacapp_decode_application_data(apdu, len, &value);
    zassert_true(len > 0, NULL);
    zassert_equal(value.tag, BACNET_APPLICATION_TAG_CHARACTER_STRING, NULL);
    status =
        characterstring_ansi_same(&value.type.Character_String, test_file_type);
    zassert_true(status, NULL);

    bacfile_cleanup();
}
/**
 * @}
 */

#if defined(CONFIG_ZTEST_NEW_API)
ZTEST_SUITE(bacfile_tests, NULL, NULL, NULL, NULL, NULL);
#else
void test_main(void)
{
    ztest_test_suite(
        bacfile_tests, ztest_unit_test(test_BACnet_File_Object),
        ztest_unit_test(
            test_BACnet_File_Object_name_description_file_type_write));

    ztest_run_test_suite(bacfile_tests);
}
#endif
