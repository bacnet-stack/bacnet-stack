/*
 * Copyright (c) 2020 Legrand North America, LLC.
 *
 * SPDX-License-Identifier: MIT
 */

/* @file
 * @brief test BACnet integer encode/decode APIs
 */

#include <zephyr/ztest.h>
#include <bacnet/basic/object/device.h>

/**
 * @addtogroup bacnet_tests
 * @{
 */

/**
 * @brief Test
 */
static void testDevice(void)
{
    bool status = false;
    const char *name = "Patricia";

    status = Device_Set_Object_Instance_Number(0);
    zassert_equal(Device_Object_Instance_Number(), 0, NULL);
    zassert_true(status, NULL);
    status = Device_Set_Object_Instance_Number(BACNET_MAX_INSTANCE);
    zassert_equal(Device_Object_Instance_Number(), BACNET_MAX_INSTANCE, NULL);
    zassert_true(status, NULL);
    status = Device_Set_Object_Instance_Number(BACNET_MAX_INSTANCE / 2);
    zassert_equal(
        Device_Object_Instance_Number(), (BACNET_MAX_INSTANCE / 2), NULL);
    zassert_true(status, NULL);
    status = Device_Set_Object_Instance_Number(BACNET_MAX_INSTANCE + 1);
    zassert_not_equal(
        Device_Object_Instance_Number(), (BACNET_MAX_INSTANCE + 1), NULL);
    zassert_false(status, NULL);

    Device_Set_System_Status(STATUS_NON_OPERATIONAL, true);
    zassert_equal(Device_System_Status(), STATUS_NON_OPERATIONAL, NULL);

    zassert_equal(Device_Vendor_Identifier(), BACNET_VENDOR_ID, NULL);

    Device_Set_Model_Name(name, strlen(name));
    zassert_equal(strcmp(Device_Model_Name(), name), 0, NULL);

    return;
}
/**
 * @}
 */


void test_main(void)
{
    ztest_test_suite(device_tests,
     ztest_unit_test(testDevice)
     );

    ztest_run_test_suite(device_tests);
}
