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
#include <bacnet/basic/object/color_object.h>

/**
 * @addtogroup bacnet_tests
 * @{
 */

/**
 * @brief Test
 */
#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(color_object_tests, testColorObject)
#else
static void testColorObject(void)
#endif
{
    uint8_t apdu[MAX_APDU] = { 0 };
    int len = 0;
    int test_len = 0;
    BACNET_READ_PROPERTY_DATA rpdata = { 0 };
    BACNET_APPLICATION_DATA_VALUE value = { 0 };
    const int *pRequired = NULL;
    const int *pOptional = NULL;
    const int *pProprietary = NULL;
    const uint32_t instance = 123;
    BACNET_WRITE_PROPERTY_DATA wpdata = { 0 };
    bool status = false;
    unsigned index;
    const char *test_name = NULL;
    char *sample_name = "sample";

    Color_Init();
    Color_Create(instance);
    status = Color_Valid_Instance(instance);
    zassert_true(status, NULL);
    index = Color_Instance_To_Index(instance);
    zassert_equal(index, 0, NULL);

    rpdata.application_data = &apdu[0];
    rpdata.application_data_len = sizeof(apdu);
    rpdata.object_type = OBJECT_COLOR;
    rpdata.object_instance = instance;
    rpdata.object_property = PROP_OBJECT_IDENTIFIER;

    Color_Property_Lists(&pRequired, &pOptional, &pProprietary);
    while ((*pRequired) >= 0) {
        rpdata.object_property = *pRequired;
        rpdata.array_index = BACNET_ARRAY_ALL;
        len = Color_Read_Property(&rpdata);
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
            status = Color_Write_Property(&wpdata);
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
        len = Color_Read_Property(&rpdata);
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
            status = Color_Write_Property(&wpdata);
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
    len = Color_Read_Property(&rpdata);
    zassert_equal(len, BACNET_STATUS_ERROR, NULL);
    wpdata.object_property = PROP_ALL;
    status = Color_Write_Property(&wpdata);
    zassert_false(status, NULL);
    /* test the ASCII name get/set */
    status = Color_Name_Set(instance, sample_name);
    zassert_true(status, NULL);
    test_name = Color_Name_ASCII(instance);
    zassert_equal(test_name, sample_name, NULL);
    status = Color_Name_Set(instance, NULL);
    zassert_true(status, NULL);
    test_name = Color_Name_ASCII(instance);
    zassert_equal(test_name, NULL, NULL);
    /* cleanup */
    status = Color_Delete(instance);
    zassert_true(status, NULL);

    return;
}
/**
 * @}
 */

#if defined(CONFIG_ZTEST_NEW_API)
ZTEST_SUITE(color_object_tests, NULL, NULL, NULL, NULL, NULL);
#else
void test_main(void)
{
    ztest_test_suite(color_object_tests, ztest_unit_test(testColorObject));

    ztest_run_test_suite(color_object_tests);
}
#endif
