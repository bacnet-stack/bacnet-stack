/*
 * Copyright (c) 2020 Legrand North America, LLC.
 *
 * SPDX-License-Identifier: MIT
 */

/* @file
 * @brief test BACnet integer encode/decode APIs
 */

#include <ztest.h>
#include <bacnet/basic/sys/keylist.h>
#include <bacnet/basic/object/objects.h>

/**
 * @addtogroup bacnet_tests
 * @{
 */

/**
 * @brief Test
 */
#if 0 /*TODO: Change to use external methods */
static void testBACnetObjectsCompare(
    OBJECT_DEVICE_T *pDevice, uint32_t device_id)
{
    zassert_not_null(pDevice, NULL);
    if (pDevice) {
        zassert_not_null(pDevice->Object_List, NULL);
        zassert_equal(pDevice->Object_Identifier.instance, device_id, NULL);
        zassert_equal(pDevice->Object_Identifier.type, OBJECT_DEVICE, NULL);
        zassert_equal(pDevice->Object_Type, OBJECT_DEVICE, NULL);
    }
}
#endif

static void testBACnetObjects(void)
{
#if 0 /*TODO: Change to use external methods */
    uint32_t device_id = 0;
    unsigned test_point = 0;
    const unsigned max_test_points = 20;
    OBJECT_DEVICE_T *pDevice;

    for (test_point = 0; test_point < max_test_points; test_point++) {
        device_id = test_point * (BACNET_MAX_INSTANCE / max_test_points);
        pDevice = objects_device_new(device_id);
        testBACnetObjectsCompare(pDevice, device_id);
        pDevice = objects_device_by_instance(device_id);
        testBACnetObjectsCompare(pDevice, device_id);
    }
    zassert_equal(max_test_points, objects_device_count(), NULL);
    for (test_point = 0; test_point < max_test_points; test_point++) {
        device_id = test_point * (BACNET_MAX_INSTANCE / max_test_points);
        pDevice = objects_device_by_instance(device_id);
        testBACnetObjectsCompare(pDevice, device_id);
    }
    for (test_point = 0; test_point < max_test_points; test_point++) {
        device_id = test_point * (BACNET_MAX_INSTANCE / max_test_points);
        pDevice = objects_device_data(test_point);
        testBACnetObjectsCompare(
            pDevice, Keylist_Key(Device_List, test_point));
    }
    for (test_point = 0; test_point < max_test_points; test_point++) {
        pDevice = objects_device_delete(0);
    }
#else
    ztest_test_skip();
#endif
}
/**
 * @}
 */


void test_main(void)
{
    ztest_test_suite(objects_tests,
     ztest_unit_test(testBACnetObjects)
     );

    ztest_run_test_suite(objects_tests);
}
