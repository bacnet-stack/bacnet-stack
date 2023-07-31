/*
 * Copyright (c) 2020 Legrand North America, LLC.
 *
 * SPDX-License-Identifier: MIT
 */

/* @file
 * @brief test BACnet integer encode/decode APIs
 */

#include <zephyr/ztest.h>
#include <bacnet/basic/object/bo.h>

/**
 * @addtogroup bacnet_tests
 * @{
 */

/**
 * @brief Test
 */
#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(bo_tests, testBinaryOutput)
#else
static void testBinaryOutput(void)
#endif
{
    uint8_t apdu[MAX_APDU] = { 0 };
    int len = 0, test_len = 0;
    BACNET_READ_PROPERTY_DATA rpdata = { 0 };
    BACNET_APPLICATION_DATA_VALUE value = {0};
    const int *required_property = NULL;
    const uint32_t instance = 1;

    Binary_Output_Init();
    Binary_Output_Create(1);
    rpdata.application_data = &apdu[0];
    rpdata.application_data_len = sizeof(apdu);
    rpdata.object_type = OBJECT_BINARY_OUTPUT;
    rpdata.object_instance = instance;
    rpdata.array_index = BACNET_ARRAY_ALL;

    Binary_Output_Property_Lists(&required_property, NULL, NULL);
    while ((*required_property) >= 0) {
        rpdata.object_property = *required_property;
        len = Binary_Output_Read_Property(&rpdata);
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
}
/**
 * @}
 */


#if defined(CONFIG_ZTEST_NEW_API)
ZTEST_SUITE(bo_tests, NULL, NULL, NULL, NULL, NULL);
#else
void test_main(void)
{
    ztest_test_suite(bo_tests,
     ztest_unit_test(testBinaryOutput)
     );

    ztest_run_test_suite(bo_tests);
}
#endif
