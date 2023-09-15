/*
 * Copyright (c) 2020 Legrand North America, LLC.
 *
 * SPDX-License-Identifier: MIT
 */

/* @file
 * @brief test BACnet integer encode/decode APIs
 */

#include <zephyr/ztest.h>
#include <bacnet/basic/object/access_credential.h>

/**
 * @addtogroup bacnet_tests
 * @{
 */

/**
 * @brief Test
 */
#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(access_credential_tests, testAccessCredential)
#else
static void testAccessCredential(void)
#endif
{
    uint8_t apdu[MAX_APDU] = { 0 };
    int len = 0;
    int test_len = 0;
    BACNET_READ_PROPERTY_DATA rpdata = { 0 };
    BACNET_APPLICATION_DATA_VALUE value = { 0 };
    BACNET_APPLICATION_DATA_VALUE value2 = { 0 };
    const int *required_property = NULL;
    bool status = false;

    Access_Credential_Init();
    rpdata.application_data = &apdu[0];
    rpdata.application_data_len = sizeof(apdu);
    rpdata.object_type = OBJECT_ACCESS_CREDENTIAL;
    rpdata.object_instance = Access_Credential_Index_To_Instance(0);
    status = Access_Credential_Valid_Instance(rpdata.object_instance);
    zassert_true(status, NULL);
    Access_Credential_Property_Lists(&required_property, NULL, NULL);
    while ((*required_property) >= 0) {
        rpdata.object_property = *required_property;
        rpdata.array_index = BACNET_ARRAY_ALL;
        len = Access_Credential_Read_Property(&rpdata);
        zassert_true(len >= 0, NULL);
        if (len >= 0) {
            if (IS_CONTEXT_SPECIFIC(rpdata.application_data[0])) {
                test_len = bacapp_decode_context_data(rpdata.application_data,
                    len, &value, rpdata.object_property);
            } else {
                test_len = bacapp_decode_application_data(
                    rpdata.application_data, len, &value);
                if (test_len < len) {
                    test_len += bacapp_decode_application_data(
                        rpdata.application_data + test_len, len - test_len,
                        &value2);
                }
            }
            if (len != test_len) {
                fprintf(stderr, "property '%d': failed to decode!\n",
                    rpdata.object_property);
            }
            zassert_true(len == test_len, NULL);
        }
        required_property++;
    }

    return;
}
/**
 * @}
 */


#if defined(CONFIG_ZTEST_NEW_API)
ZTEST_SUITE(access_credential_tests, NULL, NULL, NULL, NULL, NULL);
#else
void test_main(void)
{
    ztest_test_suite(
        access_credential_tests, ztest_unit_test(testAccessCredential));

    ztest_run_test_suite(access_credential_tests);
}
#endif
