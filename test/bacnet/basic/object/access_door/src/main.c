/*
 * Copyright (c) 2020 Legrand North America, LLC.
 *
 * SPDX-License-Identifier: MIT
 */

/* @file
 * @brief test BACnet access_door object APIs
 */

#include <zephyr/ztest.h>
#include <bacnet/basic/object/access_door.h>

/**
 * @addtogroup bacnet_tests
 * @{
 */

/**
 * @brief Test
 */
#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(tests_object_access_door, test_object_access_door)
#else
static void test_object_access_door(void)
#endif
{
    uint8_t apdu[MAX_APDU] = { 0 };
    int len = 0;
    int test_len = 0;
    BACNET_READ_PROPERTY_DATA rpdata;
    /* for decode value data */
    BACNET_APPLICATION_DATA_VALUE value = { 0 };
    const int *pRequired = NULL;
    const int *pOptional = NULL;
    const int *pProprietary = NULL;
    unsigned count = 0;
    uint32_t object_instance = 0;

    Access_Door_Init();
    count = Access_Door_Count();
    zassert_true(count > 0, NULL);
    object_instance = Access_Door_Index_To_Instance(0);
    rpdata.application_data = &apdu[0];
    rpdata.application_data_len = sizeof(apdu);
    rpdata.object_type = OBJECT_ACCESS_DOOR;
    rpdata.object_instance = object_instance;
    Access_Door_Property_Lists(&pRequired, &pOptional, &pProprietary);
    while ((*pRequired) != -1) {
        rpdata.object_property = *pRequired;
        rpdata.array_index = BACNET_ARRAY_ALL;
        len = Access_Door_Read_Property(&rpdata);
        zassert_not_equal(len, BACNET_STATUS_ERROR, NULL);
        if (len > 0) {
            test_len = bacapp_decode_application_data(
                rpdata.application_data, (uint8_t)rpdata.application_data_len,
                &value);
            zassert_true(test_len >= 0, NULL);
        }
        pRequired++;
    }
    while ((*pOptional) != -1) {
        rpdata.object_property = *pOptional;
        rpdata.array_index = BACNET_ARRAY_ALL;
        len = Access_Door_Read_Property(&rpdata);
        zassert_not_equal(len, BACNET_STATUS_ERROR, NULL);
        if (len > 0) {
            test_len = bacapp_decode_application_data(
                rpdata.application_data, (uint8_t)rpdata.application_data_len,
                &value);
            zassert_true(test_len >= 0, NULL);
        }
        pOptional++;
    }

    return;
}
/**
 * @}
 */

#if defined(CONFIG_ZTEST_NEW_API)
ZTEST_SUITE(tests_object_access_door, NULL, NULL, NULL, NULL, NULL);
#else
void test_main(void)
{
    ztest_test_suite(
        tests_object_access_door, ztest_unit_test(test_object_access_door));

    ztest_run_test_suite(tests_object_access_door);
}
#endif
