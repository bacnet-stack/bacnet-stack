/*
 * Copyright (c) 2023 Legrand North America, LLC.
 *
 * SPDX-License-Identifier: MIT
 */

/* @file
 * @brief test BACnet Time Value encode/decode APIs
 */

#include <zephyr/ztest.h>
#include <bacnet/basic/object/time_value.h>
#include <bacnet/bactext.h>

/**
 * @addtogroup bacnet_tests
 * @{
 */

/**
 * @brief Test Time Value handling
 */
static void testTimeValue(void)
{
    uint8_t apdu[MAX_APDU] = { 0 };
    int len = 0, test_len = 0;
    BACNET_READ_PROPERTY_DATA rpdata = { 0 };
    BACNET_APPLICATION_DATA_VALUE value = {0};
    const int *required_property = NULL;
    const uint32_t instance = 1;

    Time_Value_Init();
    zassert_true(Time_Value_Create(instance), NULL);

    rpdata.application_data = &apdu[0];
    rpdata.application_data_len = sizeof(apdu);
    rpdata.object_type = OBJECT_TIME_VALUE;
    rpdata.object_instance = instance;
    rpdata.array_index = BACNET_ARRAY_ALL;

    Time_Value_Property_Lists(&required_property, NULL, NULL);
    while ((*required_property) >= 0) {
        rpdata.object_property = *required_property;
        len = Time_Value_Read_Property(&rpdata);
        zassert_true(len >= 0, NULL);
        if (len >= 0) {
            test_len = bacapp_decode_known_property(rpdata.application_data,
                len, &value, rpdata.object_type, rpdata.object_property);
            if (len != test_len) {
                printf("property '%s': failed to decode!\n",
                    bactext_property_name(rpdata.object_property));
            }
            zassert_equal(len, test_len, NULL);
        }
        required_property++;
    }
    zassert_true(Time_Value_Delete(instance), NULL);
}
/**
 * @}
 */


void test_main(void)
{
    ztest_test_suite(tv_tests,
     ztest_unit_test(testTimeValue)
     );

    ztest_run_test_suite(tv_tests);
}
