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

/**
 * @addtogroup bacnet_tests
 * @{
 */

/**
 * @brief Test
 */
static void testCredentialDataInput(void)
{
    uint8_t apdu[MAX_APDU] = { 0 };
    int len = 0;
    uint32_t len_value = 0;
    uint8_t tag_number = 0;
    uint32_t decoded_instance = 0;
    BACNET_OBJECT_TYPE decoded_type = 0;
    BACNET_READ_PROPERTY_DATA rpdata;

    Credential_Data_Input_Init();
    rpdata.application_data = &apdu[0];
    rpdata.application_data_len = sizeof(apdu);
    rpdata.object_type = OBJECT_CREDENTIAL_DATA_INPUT;
    rpdata.object_instance = 1;
    rpdata.object_property = PROP_OBJECT_IDENTIFIER;
    rpdata.array_index = BACNET_ARRAY_ALL;
    len = Credential_Data_Input_Read_Property(&rpdata);
    zassert_not_equal(len, 0, NULL);
    len = decode_tag_number_and_value(&apdu[0], &tag_number, &len_value);
    zassert_equal(tag_number, BACNET_APPLICATION_TAG_OBJECT_ID, NULL);
    len = decode_object_id(&apdu[len], &decoded_type, &decoded_instance);
    zassert_equal(decoded_type, rpdata.object_type, NULL);
    zassert_equal(decoded_instance, rpdata.object_instance, NULL);

    return;
}
/**
 * @}
 */


void test_main(void)
{
    ztest_test_suite(credential_data_input_tests,
     ztest_unit_test(testCredentialDataInput)
     );

    ztest_run_test_suite(credential_data_input_tests);
}
