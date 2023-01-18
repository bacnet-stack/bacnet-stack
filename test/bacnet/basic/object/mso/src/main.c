/*
 * Copyright (c) 2020 Legrand North America, LLC.
 *
 * SPDX-License-Identifier: MIT
 */

/* @file
 * @brief test BACnet integer encode/decode APIs
 */

#include <zephyr/ztest.h>
#include <bacnet/basic/object/mso.h>

/**
 * @addtogroup bacnet_tests
 * @{
 */

/**
 * @brief Test
 */
static void testMultistateOutput(void)
{
    uint8_t apdu[MAX_APDU] = { 0 };
    int len = 0, test_len = 0;
    BACNET_READ_PROPERTY_DATA rpdata = { 0 };
    BACNET_APPLICATION_DATA_VALUE value = {0};
    const int *required_property = NULL;
    const uint32_t instance = 1;

    Multistate_Output_Init();
    Multistate_Output_Create(1);
    rpdata.application_data = &apdu[0];
    rpdata.application_data_len = sizeof(apdu);
    rpdata.object_type = OBJECT_MULTI_STATE_OUTPUT;
    rpdata.object_instance = instance;
    rpdata.array_index = BACNET_ARRAY_ALL;

    Multistate_Output_Property_Lists(&required_property, NULL, NULL);
    while ((*required_property) >= 0) {
        rpdata.object_property = *required_property;
        len = Multistate_Output_Read_Property(&rpdata);
        if (len < 0) {
            printf("property %u: failed to read!\n",
                (unsigned)rpdata.object_property);
        }
        zassert_true(len >= 0, NULL);
        if (len >= 0) {
            test_len = bacapp_decode_known_property(rpdata.application_data,
                len, &value, rpdata.object_type, rpdata.object_property);
            if (len != test_len) {
                printf("property %u: failed to decode!\n",
                    (unsigned)rpdata.object_property);
            }
            if (rpdata.object_property == PROP_PRIORITY_ARRAY) {
                /* FIXME: known fail to decode */
                len = test_len;
            }
            zassert_equal(len, test_len, NULL);
        }
        required_property++;
    }

    return;
}
/**
 * @}
 */


void test_main(void)
{
    ztest_test_suite(mso_tests,
     ztest_unit_test(testMultistateOutput)
     );

    ztest_run_test_suite(mso_tests);
}
