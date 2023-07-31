/*
 * Copyright (c) 2020 Legrand North America, LLC.
 *
 * SPDX-License-Identifier: MIT
 */

/* @file
 * @brief test BACnet integer encode/decode APIs
 */

#include <zephyr/ztest.h>
#include <bacnet/basic/object/access_rights.h>

/**
 * @addtogroup bacnet_tests
 * @{
 */

/**
 * @brief Test
 */
#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(access_rights_tests, testAccessRights)
#else
static void testAccessRights(void)
#endif
{
    uint8_t apdu[MAX_APDU] = { 0 };
    int len = 0, test_len = 0;
    uint32_t decoded_instance = 0;
    BACNET_OBJECT_TYPE decoded_type = 0;
    BACNET_READ_PROPERTY_DATA rpdata;

    Access_Rights_Init();
    rpdata.application_data = &apdu[0];
    rpdata.application_data_len = sizeof(apdu);
    rpdata.object_type = OBJECT_ACCESS_RIGHTS;
    rpdata.object_instance = 1;
    rpdata.object_property = PROP_OBJECT_IDENTIFIER;
    rpdata.array_index = BACNET_ARRAY_ALL;
    len = Access_Rights_Read_Property(&rpdata);
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
ZTEST_SUITE(access_rights_tests, NULL, NULL, NULL, NULL, NULL);
#else
void test_main(void)
{
    ztest_test_suite(access_rights_tests,
     ztest_unit_test(testAccessRights)
     );

    ztest_run_test_suite(access_rights_tests);
}
#endif
