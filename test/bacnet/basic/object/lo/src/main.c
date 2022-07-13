/*
 * Copyright (c) 2020 Legrand North America, LLC.
 *
 * SPDX-License-Identifier: MIT
 */

/* @file
 * @brief test BACnet integer encode/decode APIs
 */

#include <ztest.h>
#include <bacnet/bactext.h>
#include <bacnet/basic/object/lo.h>

/**
 * @addtogroup bacnet_tests
 * @{
 */

/**
 * @brief Test
 */
static void testLightingOutput(void)
{
    uint8_t apdu[MAX_APDU] = { 0 };
    int len = 0, test_len = 0;
    uint32_t len_value = 0;
    uint8_t tag_number = 0;
    BACNET_OBJECT_TYPE decoded_type = 0;
    uint32_t decoded_instance = 0;
    BACNET_READ_PROPERTY_DATA rpdata;
    BACNET_APPLICATION_DATA_VALUE value = {0};
    const int *required_property = NULL;
    const uint32_t instance = 1;

    Lighting_Output_Init();
    rpdata.application_data = &apdu[0];
    rpdata.application_data_len = sizeof(apdu);
    rpdata.object_type = OBJECT_LIGHTING_OUTPUT;
    rpdata.object_instance = instance;
    rpdata.array_index = BACNET_ARRAY_ALL;

    Lighting_Output_Property_Lists(&required_property, NULL, NULL);
    while ((*required_property) >= 0) {
        rpdata.object_property = *required_property;
        len = Lighting_Output_Read_Property(&rpdata);
        zassert_true(len >= 0, NULL);
        if (len >= 0) {
            test_len = bacapp_decode_known_property(rpdata.application_data,
                len, &value, rpdata.object_type, rpdata.object_property);
            if (len != test_len) {
                printf("property '%s': failed to decode!\n",
                    bactext_property_name(rpdata.object_property));
            }
            if ((rpdata.object_property == PROP_LIGHTING_COMMAND) ||
                (rpdata.object_property == PROP_PRIORITY_ARRAY) ||
                (rpdata.object_property == PROP_TRACKING_VALUE)) {
                /* FIXME: how to decode the complex data? */
                test_len = len;
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
    ztest_test_suite(lo_tests,
     ztest_unit_test(testLightingOutput)
     );

    ztest_run_test_suite(lo_tests);
}
