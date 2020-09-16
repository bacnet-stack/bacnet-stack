/*
 * Copyright (c) 2020 Legrand North America, LLC.
 *
 * SPDX-License-Identifier: MIT
 */

/* @file
 * @brief test BACnet integer encode/decode APIs
 */

#include <ztest.h>
#include <bacnet/basic/object/acc.h>

/**
 * @addtogroup bacnet_tests
 * @{
 */

/**
 * @brief Test
 */
static void test_Accumulator(void)
{
#if 0 /*TODO: Refactor implementation to expose for testing */
    uint8_t apdu[MAX_APDU] = { 0 };
    int len = 0;
    int test_len = 0;
    BACNET_READ_PROPERTY_DATA rpdata = {0};
    BACNET_APPLICATION_DATA_VALUE value = {0};
    const int *property = &Properties_Required[0];
    BACNET_UNSIGNED_INTEGER unsigned_value = 1;

    Accumulator_Init();
    rpdata.application_data = &apdu[0];
    rpdata.application_data_len = sizeof(apdu);
    rpdata.object_type = OBJECT_ACCUMULATOR;
    rpdata.object_instance = 1;

    while ((*property) >= 0) {
        rpdata.object_property = *property;
        rpdata.array_index = BACNET_ARRAY_ALL;
        len = Accumulator_Read_Property(&rpdata);
        zassert_not_equal(len, 0, NULL);
        if (IS_CONTEXT_SPECIFIC(rpdata.application_data[0])) {
            test_len = bacapp_decode_context_data(rpdata.application_data,
                len, &value, rpdata.object_property);
        } else {
            test_len = bacapp_decode_application_data(rpdata.application_data,
                len, &value);
        }
        if (len != test_len) {
            printf("property '%s': failed to decode!\n",
                bactext_property_name(rpdata.object_property));
        }
        zassert_equal(len, test_len, NULL);
        property++;
    }
    /* test 1-bit to 64-bit encode/decode of present-value */
    rpdata.object_property = PROP_PRESENT_VALUE;
    while (unsigned_value != BACNET_UNSIGNED_INTEGER_MAX) {
        Accumulator_Present_Value_Set(0, unsigned_value);
        len = Accumulator_Read_Property(&rpdata);
        zassert_not_equal(len, 0, NULL);
        test_len = bacapp_decode_application_data(rpdata.application_data,
            len, &value);
        zassert_equal(len, test_len, NULL);
        unsigned_value |= (unsigned_value<<1);
    }

    return;
#else
    ztest_test_skip();
#endif
}
/**
 * @}
 */


void test_main(void)
{
    ztest_test_suite(acc_tests,
     ztest_unit_test(test_Accumulator)
     );

    ztest_run_test_suite(acc_tests);
}
