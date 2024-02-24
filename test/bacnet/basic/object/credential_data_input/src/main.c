/*
 * Copyright (c) 2020 Legrand North America, LLC.
 *
 * SPDX-License-Identifier: MIT
 */

/* @file
 * @brief test BACnet integer encode/decode APIs
 */

#include <zephyr/ztest.h>
#include <bacnet/basic/object/credential_data_input.h>
#include <bacnet/bactext.h>

/**
 * @addtogroup bacnet_tests
 * @{
 */

/**
 * @brief Test
 */
#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(credential_data_input_tests, testCredentialDataInput)
#else
static void testCredentialDataInput(void)
#endif
{
    uint8_t apdu[MAX_APDU] = { 0 };
    int len = 0;
    int test_len = 0;
    BACNET_READ_PROPERTY_DATA rpdata = { 0 };
    BACNET_WRITE_PROPERTY_DATA wpdata = { 0 };
    BACNET_APPLICATION_DATA_VALUE value = { 0 };
    const int *pRequired = NULL;
    const int *pOptional = NULL;
    const int *pProprietary = NULL;
    bool status = false;
    unsigned count = 0;
    uint32_t object_instance = 0;

    Credential_Data_Input_Init();
    count = Credential_Data_Input_Count();
    zassert_true(count > 0, NULL);
    object_instance = Credential_Data_Input_Index_To_Instance(0);
    rpdata.application_data = &apdu[0];
    rpdata.application_data_len = sizeof(apdu);
    rpdata.object_type = OBJECT_CREDENTIAL_DATA_INPUT;
    rpdata.object_instance = object_instance;
    Credential_Data_Input_Property_Lists(&pRequired, &pOptional, &pProprietary);
    while ((*pRequired) != -1) {
        rpdata.object_property = *pRequired;
        rpdata.array_index = BACNET_ARRAY_ALL;
        len = Credential_Data_Input_Read_Property(&rpdata);
        zassert_not_equal(len, BACNET_STATUS_ERROR, NULL);
        if (len >= 0) {
            test_len = bacapp_decode_known_property(rpdata.application_data,
                (uint8_t)rpdata.application_data_len, &value,
                rpdata.object_type, rpdata.object_property);
            if (len != test_len) {
                printf("property '%s': failed to decode! %d!=%d\n",
                    bactext_property_name(rpdata.object_property), test_len,
                    len);
            }
            if ((rpdata.object_property == PROP_PRESENT_VALUE) ||
                (rpdata.object_property == PROP_UPDATE_TIME) ||
                (rpdata.object_property == PROP_SUPPORTED_FORMATS)) {
                /* FIXME: known fail to decode */
                test_len = len;
            }
            zassert_true(test_len == len, NULL);
            /* check WriteProperty properties */
            wpdata.object_type = rpdata.object_type;
            wpdata.object_instance = rpdata.object_instance;
            wpdata.object_property = rpdata.object_property;
            wpdata.array_index = rpdata.array_index;
            memcpy(&wpdata.application_data, rpdata.application_data, MAX_APDU);
            wpdata.application_data_len = len;
            wpdata.error_code = ERROR_CODE_SUCCESS;
            status = Credential_Data_Input_Write_Property(&wpdata);
            if (!status) {
                /* verify WriteProperty property is known */
                zassert_not_equal(wpdata.error_code,
                    ERROR_CODE_UNKNOWN_PROPERTY,
                    "property '%s': WriteProperty Unknown!\n",
                    bactext_property_name(rpdata.object_property));
            }
        } else {
            printf("property '%s': failed to read(%d)!\n",
                bactext_property_name(rpdata.object_property),len);
        }
        pRequired++;
    }
    while ((*pOptional) != -1) {
        rpdata.object_property = *pOptional;
        rpdata.array_index = BACNET_ARRAY_ALL;
        len = Credential_Data_Input_Read_Property(&rpdata);
        zassert_not_equal(len, BACNET_STATUS_ERROR, NULL);
        if (len > 0) {
            test_len = bacapp_decode_known_property(rpdata.application_data,
                (uint8_t)rpdata.application_data_len, &value,
                rpdata.object_type, rpdata.object_property);
            zassert_true(test_len == len, NULL);
            /* check WriteProperty properties */
            wpdata.object_type = rpdata.object_type;
            wpdata.object_instance = rpdata.object_instance;
            wpdata.object_property = rpdata.object_property;
            wpdata.array_index = rpdata.array_index;
            memcpy(&wpdata.application_data, rpdata.application_data, MAX_APDU);
            wpdata.application_data_len = len;
            wpdata.error_code = ERROR_CODE_SUCCESS;
            status = Credential_Data_Input_Write_Property(&wpdata);
            if (!status) {
                /* verify WriteProperty property is known */
                zassert_not_equal(wpdata.error_code,
                    ERROR_CODE_UNKNOWN_PROPERTY,
                    "property '%s': WriteProperty Unknown!\n",
                    bactext_property_name(rpdata.object_property));
            }
        } else {
            printf("property '%s': failed to read(%d)!\n",
                bactext_property_name(rpdata.object_property),len);
        }
        pOptional++;
    }
}
/**
 * @}
 */


#if defined(CONFIG_ZTEST_NEW_API)
ZTEST_SUITE(credential_data_input_tests, NULL, NULL, NULL, NULL, NULL);
#else
void test_main(void)
{
    ztest_test_suite(credential_data_input_tests,
     ztest_unit_test(testCredentialDataInput)
     );

    ztest_run_test_suite(credential_data_input_tests);
}
#endif
