/*
 * Copyright (c) 2020 Legrand North America, LLC.
 *
 * SPDX-License-Identifier: MIT
 */

/* @file
 * @brief test BACnet integer encode/decode APIs
 */

#include <zephyr/ztest.h>
#include <bacnet/property.h>

/**
 * @addtogroup bacnet_tests
 * @{
 */

/**
 * @brief Test
 */
#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(property_tests, testPropList)
#else
void testPropList(void)
#endif
{
    unsigned i = 0, j = 0;
    unsigned count = 0;
    BACNET_PROPERTY_ID property = MAX_BACNET_PROPERTY_ID;
    unsigned object_id = 0, object_name = 0, object_type = 0;
    struct special_property_list_t property_list = { 0 };

    for (i = 0; i < OBJECT_PROPRIETARY_MIN; i++) {
        count = property_list_special_count((BACNET_OBJECT_TYPE)i, PROP_ALL);
        zassert_true(count >= 3, NULL);
        object_id = 0;
        object_name = 0;
        object_type = 0;
        for (j = 0; j < count; j++) {
            property = property_list_special_property(
                (BACNET_OBJECT_TYPE)i, PROP_ALL, j);
            if (property == PROP_OBJECT_TYPE) {
                object_type++;
            }
            if (property == PROP_OBJECT_IDENTIFIER) {
                object_id++;
            }
            if (property == PROP_OBJECT_NAME) {
                object_name++;
            }
        }
        zassert_equal(object_type, 1, NULL);
        zassert_equal(object_id, 1, NULL);
        zassert_equal(object_name, 1, NULL);
        /* test member function */
        property_list_special((BACNET_OBJECT_TYPE)i, &property_list);
        zassert_true(
            property_list_member(
                property_list.Required.pList, PROP_OBJECT_TYPE), NULL);
        zassert_true(
            property_list_member(
                property_list.Required.pList, PROP_OBJECT_IDENTIFIER), NULL);
        zassert_true(
            property_list_member(
                property_list.Required.pList, PROP_OBJECT_NAME), NULL);
    }
}
/**
 * @}
 */


#if defined(CONFIG_ZTEST_NEW_API)
ZTEST_SUITE(property_tests, NULL, NULL, NULL, NULL, NULL);
#else
void test_main(void)
{
    ztest_test_suite(property_tests,
     ztest_unit_test(testPropList)
     );

    ztest_run_test_suite(property_tests);
}
#endif
