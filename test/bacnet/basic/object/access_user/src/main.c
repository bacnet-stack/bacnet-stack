/*
 * Copyright (c) 2020 Legrand North America, LLC.
 *
 * SPDX-License-Identifier: MIT
 */

/* @file
 * @brief test BACnet integer encode/decode APIs
 */

#include <zephyr/ztest.h>
#include <bacnet/basic/object/access_user.h>

/**
 * @addtogroup bacnet_tests
 * @{
 */

/**
 * @brief Test
 */
#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(access_user_tests, testAccessUser)
#else
static void testAccessUser(void)
#endif
{
    uint8_t apdu[MAX_APDU] = { 0 };
    int len = 0, test_len = 0;
    uint32_t decoded_instance = 0;
    BACNET_OBJECT_TYPE decoded_type = 0;
    BACNET_READ_PROPERTY_DATA rpdata;

    Access_User_Init();
    rpdata.application_data = &apdu[0];
    rpdata.application_data_len = sizeof(apdu);
    rpdata.object_type = OBJECT_ACCESS_USER;
    rpdata.object_instance = 1;
    rpdata.object_property = PROP_OBJECT_IDENTIFIER;
    rpdata.array_index = BACNET_ARRAY_ALL;
    len = Access_User_Read_Property(&rpdata);
    zassert_not_equal(len, 0, NULL);
    test_len = bacnet_object_id_application_decode(
        apdu, len, &decoded_type, &decoded_instance);
    zassert_not_equal(test_len, BACNET_STATUS_ERROR, NULL);
    zassert_equal(decoded_type, rpdata.object_type, NULL);
    zassert_equal(decoded_instance, rpdata.object_instance, NULL);

    return;
}
/**
 * @}
 */

#if defined(CONFIG_ZTEST_NEW_API)
ZTEST_SUITE(access_user_tests, NULL, NULL, NULL, NULL, NULL);
#else
void test_main(void)
{
    ztest_test_suite(access_user_tests, ztest_unit_test(testAccessUser));

    ztest_run_test_suite(access_user_tests);
}
#endif
