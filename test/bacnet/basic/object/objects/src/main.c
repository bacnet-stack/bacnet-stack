/*
 * Copyright (c) 2020 Legrand North America, LLC.
 *
 * SPDX-License-Identifier: MIT
 */

/* @file
 * @brief test BACnet integer encode/decode APIs
 */

#include <zephyr/ztest.h>
#include <bacnet/basic/sys/keylist.h>
#include <bacnet/basic/object/objects.h>

/**
 * @addtogroup bacnet_tests
 * @{
 */

/**
 * @brief Test
 */
static void testBACnetObjectsCompare(
    OBJECT_DEVICE_T *pDevice, uint32_t expected_device_id)
{
    zassert_not_null(pDevice, NULL);
    if (pDevice) {
        zassert_not_null(pDevice->Object_List, NULL);
        zassert_equal(pDevice->Object_Identifier.instance, expected_device_id, NULL);
        zassert_equal(pDevice->Object_Identifier.type, OBJECT_DEVICE, NULL);
        zassert_equal(pDevice->Object_Type, OBJECT_DEVICE, NULL);
    }
}

#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(objects_tests, testBACnetObjects)
#else
static void testBACnetObjects(void)
#endif
{
    uint32_t device_id = 0;
    unsigned test_point = 0;
    const unsigned max_test_points = 20;
    OBJECT_DEVICE_T *pDevice;


    /* Verify deleting a non-existant object returns the correct value */
    zassert_false(objects_device_delete(0), NULL);

    /* Create devices */
    for (test_point = 0; test_point < max_test_points; test_point++) {
        device_id = test_point * (BACNET_MAX_INSTANCE / max_test_points);
        pDevice = objects_device_new(device_id);
        testBACnetObjectsCompare(pDevice, device_id);

	/* Verify the last created device can be fetched by ID */
        pDevice = objects_device_by_instance(device_id);
        testBACnetObjectsCompare(pDevice, device_id);
    }
    zassert_equal(max_test_points, objects_device_count(), NULL);

    /* Verify each of the expected IDs can be fetched by ID */
    for (test_point = 0; test_point < max_test_points; test_point++) {
        device_id = test_point * (BACNET_MAX_INSTANCE / max_test_points);
        pDevice = objects_device_by_instance(device_id);
        testBACnetObjectsCompare(pDevice, device_id);
    }
    /* Verify each of the expected IDs can be fetched by index */
    for (test_point = 0; test_point < max_test_points; test_point++) {
        device_id = test_point * (BACNET_MAX_INSTANCE / max_test_points);
        pDevice = objects_device_data(test_point);
        testBACnetObjectsCompare(pDevice, device_id);
    }
    /* Delete every object */
    for (test_point = 0; test_point < max_test_points; test_point++) {
        device_id = test_point * (BACNET_MAX_INSTANCE / max_test_points);
        zassert_true(objects_device_delete(0), NULL);
        zassert_equal(objects_device_by_instance(device_id), NULL, NULL);
    }
    zassert_false(objects_device_delete(0), NULL);
}
/**
 * @}
 */


#if defined(CONFIG_ZTEST_NEW_API)
ZTEST_SUITE(objects_tests, NULL, NULL, NULL, NULL, NULL);
#else
void test_main(void)
{
    ztest_test_suite(objects_tests,
     ztest_unit_test(testBACnetObjects)
     );

    ztest_run_test_suite(objects_tests);
}
#endif
