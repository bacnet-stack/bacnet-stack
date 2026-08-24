/**
 * @file
 * @brief Unit test for object
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date June 2022
 *
 * @copyright SPDX-License-Identifier: MIT
 */
#include <zephyr/ztest.h>
#include <bacnet/bactext.h>
#include <bacnet/basic/object/color_temperature.h>

/**
 * @addtogroup bacnet_tests
 * @{
 */

/**
 * @brief Test
 */
#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(color_temperature_tests, testColorTemperature)
#else
static void testColorTemperature(void)
#endif
{
    uint8_t apdu[MAX_APDU] = { 0 };
    int len = 0;
    int test_len = 0;
    BACNET_READ_PROPERTY_DATA rpdata = { 0 };
    BACNET_APPLICATION_DATA_VALUE value = { 0 };
    const int32_t *pRequired = NULL;
    const int32_t *pOptional = NULL;
    const int32_t *pProprietary = NULL;
    const uint32_t instance = 123;
    BACNET_WRITE_PROPERTY_DATA wpdata = { 0 };
    bool status = false;
    const char *test_name = NULL;
    char *sample_name = "sample";
    BACNET_CHARACTER_STRING char_string = { 0 };
    BACNET_CHARACTER_STRING desc_string = { 0 };

    Color_Temperature_Init();
    Color_Temperature_Create(instance);

    rpdata.application_data = &apdu[0];
    rpdata.application_data_len = sizeof(apdu);
    rpdata.object_type = OBJECT_COLOR_TEMPERATURE;
    rpdata.object_instance = instance;
    rpdata.object_property = PROP_OBJECT_IDENTIFIER;

    Color_Temperature_Property_Lists(&pRequired, &pOptional, &pProprietary);
    while ((*pRequired) >= 0) {
        rpdata.object_property = *pRequired;
        rpdata.array_index = BACNET_ARRAY_ALL;
        len = Color_Temperature_Read_Property(&rpdata);
        zassert_not_equal(
            len, BACNET_STATUS_ERROR,
            "property '%s': failed to ReadProperty!\n",
            bactext_property_name(rpdata.object_property));
        if (len >= 0) {
            test_len = bacapp_decode_known_property(
                rpdata.application_data, len, &value, rpdata.object_type,
                rpdata.object_property);
            zassert_equal(
                len, test_len, "property '%s': failed to decode!\n",
                bactext_property_name(rpdata.object_property));
            /* check WriteProperty properties */
            wpdata.object_type = rpdata.object_type;
            wpdata.object_instance = rpdata.object_instance;
            wpdata.object_property = rpdata.object_property;
            wpdata.array_index = rpdata.array_index;
            memcpy(&wpdata.application_data, rpdata.application_data, MAX_APDU);
            wpdata.application_data_len = len;
            wpdata.error_code = ERROR_CODE_SUCCESS;
            status = Color_Temperature_Write_Property(&wpdata);
            if (!status) {
                /* verify WriteProperty property is known */
                zassert_not_equal(
                    wpdata.error_code, ERROR_CODE_UNKNOWN_PROPERTY,
                    "property '%s': WriteProperty Unknown!\n",
                    bactext_property_name(rpdata.object_property));
            }
        }
        pRequired++;
    }
    while ((*pOptional) != -1) {
        rpdata.object_property = *pOptional;
        rpdata.array_index = BACNET_ARRAY_ALL;
        len = Color_Temperature_Read_Property(&rpdata);
        zassert_not_equal(
            len, BACNET_STATUS_ERROR,
            "property '%s': failed to ReadProperty!\n",
            bactext_property_name(rpdata.object_property));
        if (len > 0) {
            test_len = bacapp_decode_application_data(
                rpdata.application_data, (uint8_t)rpdata.application_data_len,
                &value);
            zassert_equal(
                len, test_len, "property '%s': failed to decode!\n",
                bactext_property_name(rpdata.object_property));
            /* check WriteProperty properties */
            wpdata.object_type = rpdata.object_type;
            wpdata.object_instance = rpdata.object_instance;
            wpdata.object_property = rpdata.object_property;
            wpdata.array_index = rpdata.array_index;
            memcpy(&wpdata.application_data, rpdata.application_data, MAX_APDU);
            wpdata.application_data_len = len;
            wpdata.error_code = ERROR_CODE_SUCCESS;
            status = Color_Temperature_Write_Property(&wpdata);
            if (!status) {
                /* verify WriteProperty property is known */
                zassert_not_equal(
                    wpdata.error_code, ERROR_CODE_UNKNOWN_PROPERTY,
                    "property '%s': WriteProperty Unknown!\n",
                    bactext_property_name(rpdata.object_property));
            }
        }
        pOptional++;
    }
    /* check for unsupported property - use ALL */
    rpdata.object_property = PROP_ALL;
    len = Color_Temperature_Read_Property(&rpdata);
    zassert_equal(len, BACNET_STATUS_ERROR, NULL);
    wpdata.object_property = PROP_ALL;
    status = Color_Temperature_Write_Property(&wpdata);
    zassert_false(status, NULL);
    /* test the ANSI name/description get/set */
    status = Color_Temperature_Name_Set(instance, sample_name);
    zassert_true(status, NULL);
    test_name = Color_Temperature_Name_ASCII(instance);
    zassert_equal(test_name, sample_name, NULL);
    status = Color_Temperature_Object_Name(instance, &char_string);
    zassert_true(status, NULL);
    zassert_equal(
        strcmp(characterstring_value_const(&char_string), sample_name), 0,
        NULL);
    status = Color_Temperature_Name_Set(instance, NULL);
    zassert_true(status, NULL);
    status = Color_Temperature_Object_Name(instance, &char_string);
    zassert_true(status, NULL);
    zassert_equal(
        strcmp(
            characterstring_value_const(&char_string), "COLOR-TEMPERATURE-123"),
        0, NULL);
    zassert_equal(strcmp(Color_Temperature_Description(instance), ""), 0, NULL);
    status = Color_Temperature_Description_Set(instance, "sample desc");
    zassert_true(status, NULL);
    zassert_equal(
        strcmp(Color_Temperature_Description(instance), "sample desc"), 0,
        NULL);
    status = Color_Temperature_Description_Set(instance, NULL);
    zassert_true(status, NULL);
    zassert_equal(strcmp(Color_Temperature_Description(instance), ""), 0, NULL);
    char_string.length = 0;
    characterstring_init_ansi(&char_string, "written name");
    wpdata.object_type = OBJECT_COLOR_TEMPERATURE;
    wpdata.object_instance = instance;
    wpdata.object_property = PROP_OBJECT_NAME;
    wpdata.array_index = BACNET_ARRAY_ALL;
    wpdata.application_data_len = encode_application_character_string(
        wpdata.application_data, &char_string);
    wpdata.error_class = ERROR_CLASS_PROPERTY;
    wpdata.error_code = ERROR_CODE_SUCCESS;
    status = Color_Temperature_Write_Property(&wpdata);
    zassert_true(status, NULL);
    test_name = Color_Temperature_Name_ASCII(instance);
    zassert_equal(strcmp(test_name, "written name"), 0, NULL);
    desc_string.length = 0;
    characterstring_init_ansi(&desc_string, "written description");
    wpdata.object_property = PROP_DESCRIPTION;
    wpdata.application_data_len = encode_application_character_string(
        wpdata.application_data, &desc_string);
    wpdata.error_class = ERROR_CLASS_PROPERTY;
    wpdata.error_code = ERROR_CODE_SUCCESS;
    status = Color_Temperature_Write_Property(&wpdata);
    zassert_true(status, NULL);
    zassert_equal(
        strcmp(Color_Temperature_Description(instance), "written description"),
        0, NULL);
    /* cleanup */
    status = Color_Temperature_Delete(instance);
    zassert_true(status, NULL);

    return;
}
/**
 * @}
 */

#if defined(CONFIG_ZTEST_NEW_API)
ZTEST_SUITE(color_temperature_tests, NULL, NULL, NULL, NULL, NULL);
#else
void test_main(void)
{
    ztest_test_suite(
        color_temperature_tests, ztest_unit_test(testColorTemperature));

    ztest_run_test_suite(color_temperature_tests);
}
#endif
