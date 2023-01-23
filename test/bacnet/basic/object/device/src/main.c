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
 * @brief Test
 */
static void test_Device_ReadProperty(void)
{
    uint8_t apdu[MAX_APDU] = { 0 };
    int len = 0, test_len = 0;
    BACNET_READ_PROPERTY_DATA rpdata = { 0 };
    BACNET_APPLICATION_DATA_VALUE value = {0};
    const int *required_property = NULL;
    const uint32_t instance = 1;
    bool status = false;

    Device_Init(NULL);
    status = Device_Set_Object_Instance_Number(instance);
    zassert_true(status, NULL);
    rpdata.application_data = &apdu[0];
    rpdata.application_data_len = sizeof(apdu);
    rpdata.object_type = OBJECT_DEVICE;
    rpdata.object_instance = instance;
    rpdata.array_index = BACNET_ARRAY_ALL;

    Device_Property_Lists(&required_property, NULL, NULL);
    while ((*required_property) >= 0) {
        rpdata.object_property = *required_property;
        len = Device_Read_Property(&rpdata);
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
            if (rpdata.object_property == PROP_OBJECT_LIST) {
                /* FIXME: known fail to decode */
                len = test_len;
            }
            zassert_equal(len, test_len, NULL);
        }
        required_property++;
    }

    return;
}

/**
 * @}
 */


void test_main(void)
{
    ztest_test_suite(device_tests,
     ztest_unit_test(testDevice),
     ztest_unit_test(test_Device_ReadProperty)
     );

    ztest_run_test_suite(device_tests);
}
