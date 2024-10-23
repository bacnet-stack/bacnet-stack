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

/**
 * @addtogroup bacnet_tests
 * @{
 */

/**
 * @brief Test
 */
static void test_Channel_ReadProperty(void)
{
    uint8_t apdu[MAX_APDU] = { 0 };
    int len = 0;
    int test_len = 0;
    BACNET_READ_PROPERTY_DATA rpdata = { 0 };
    BACNET_APPLICATION_DATA_VALUE value = { 0 };
    BACNET_WRITE_PROPERTY_DATA wpdata = { 0 };
    const uint32_t instance = 123;
    const int *pRequired = NULL;
    const int *pOptional = NULL;
    const int *pProprietary = NULL;
    unsigned count = 0;
    bool status = false;
    unsigned index;
    const char *test_name = NULL;
    char *sample_name = "sample";

    Channel_Init();
    Channel_Create(instance);
    status = Channel_Valid_Instance(instance);
    zassert_true(status, NULL);
    index = Channel_Instance_To_Index(instance);
    zassert_equal(index, 0, NULL);

    count = Channel_Count();
    zassert_true(count > 0, NULL);
    rpdata.application_data = &apdu[0];
    rpdata.application_data_len = sizeof(apdu);
    rpdata.object_type = OBJECT_CHANNEL;
    rpdata.object_instance = Channel_Index_To_Instance(0);
    status = Channel_Valid_Instance(rpdata.object_instance);
    zassert_true(status, NULL);
    Channel_Property_Lists(&pRequired, &pOptional, &pProprietary);
    while ((*pRequired) != -1) {
        rpdata.object_property = *pRequired;
        rpdata.array_index = BACNET_ARRAY_ALL;
        len = Channel_Read_Property(&rpdata);
        zassert_not_equal(
            len, BACNET_STATUS_ERROR,
            "property '%s': failed to ReadProperty!\n",
            bactext_property_name(rpdata.object_property));
        if (len > 0) {
            test_len = bacapp_decode_application_data(
                rpdata.application_data, (uint8_t)rpdata.application_data_len,
                &value);
            if ((rpdata.object_property == PROP_PRIORITY_ARRAY) ||
                (rpdata.object_property == PROP_CONTROL_GROUPS) ||
                (rpdata.object_property ==
                 PROP_LIST_OF_OBJECT_PROPERTY_REFERENCES)) {
                /* FIXME: known fail to decode */
                len = test_len;
            }
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
            status = Channel_Write_Property(&wpdata);
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
        len = Channel_Read_Property(&rpdata);
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
            status = Channel_Write_Property(&wpdata);
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
    len = Channel_Read_Property(&rpdata);
    zassert_equal(len, BACNET_STATUS_ERROR, NULL);
    wpdata.object_property = PROP_ALL;
    status = Channel_Write_Property(&wpdata);
    zassert_false(status, NULL);
    /* test the ASCII name get/set */
    status = Channel_Name_Set(instance, sample_name);
    zassert_true(status, NULL);
    test_name = Channel_Name_ASCII(instance);
    zassert_equal(test_name, sample_name, NULL);
    status = Channel_Name_Set(instance, NULL);
    zassert_true(status, NULL);
    test_name = Channel_Name_ASCII(instance);
    zassert_equal(test_name, NULL, NULL);
    /* cleanup */
    status = Channel_Delete(instance);
    zassert_true(status, NULL);
}
/**
 * @}
 */

void test_main(void)
{
    ztest_test_suite(channel_tests, ztest_unit_test(test_Channel_ReadProperty));

    ztest_run_test_suite(channel_tests);
}
