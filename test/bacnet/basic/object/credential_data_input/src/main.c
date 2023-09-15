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
    BACNET_READ_PROPERTY_DATA rpdata;
    /* for decode value data */
    BACNET_APPLICATION_DATA_VALUE value;
    const int *pRequired = NULL;
    const int *pOptional = NULL;
    const int *pProprietary = NULL;
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
        if (len > 0) {
            test_len = bacapp_decode_application_data(rpdata.application_data,
                (uint8_t)rpdata.application_data_len, &value);
            if (len != test_len) {
                printf("property '%s': failed to decode!\n",
                    bactext_property_name(rpdata.object_property));
            }
            if (rpdata.object_property == PROP_PRIORITY_ARRAY) {
                /* FIXME: known fail to decode */
                len = test_len;
            }
            zassert_true(test_len >= 0, NULL);
        } else {
            printf("property '%s': failed to read!\n",
                bactext_property_name(rpdata.object_property));
        }
        pRequired++;
    }
    while ((*pOptional) != -1) {
        rpdata.object_property = *pOptional;
        rpdata.array_index = BACNET_ARRAY_ALL;
        len = Credential_Data_Input_Read_Property(&rpdata);
        zassert_not_equal(len, BACNET_STATUS_ERROR, NULL);
        if (len > 0) {
            test_len = bacapp_decode_application_data(rpdata.application_data,
                (uint8_t)rpdata.application_data_len, &value);
            zassert_true(test_len >= 0, NULL);
        } else {
            printf("property '%s': failed to read!\n",
                bactext_property_name(rpdata.object_property));
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
