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
#include <bacnet/bactext.h>
#include <property_test.h>

/**
 * @addtogroup bacnet_tests
 * @{
 */

/**
 * @brief Test ReadProperty API
 */
static void test_Device_Data_Sharing(void)
{
    bool status = false;
    unsigned count = 0;
    uint32_t object_instance = 1234, test_object_instance = 0;
    const int skip_fail_property_list[] = { -1 };

    Device_Init(NULL);
    count = Device_Count();
    zassert_true(count > 0, NULL);
    Device_Object_Name_ANSI_Init("My Device");
    Device_Set_Object_Instance_Number(object_instance);
    test_object_instance = Device_Index_To_Instance(0);
    zassert_equal(object_instance, test_object_instance, NULL);
    bacnet_object_properties_read_write_test(
        OBJECT_DEVICE, object_instance, Device_Property_Lists,
        Device_Read_Property, Device_Write_Property,
        skip_fail_property_list);
}

/**
 * @brief Test basic API
 */
#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(device_tests, testDevice)
#else
static void testDevice(void)
#endif
{
    bool status = false;
    const char *name = "Patricia";
    BACNET_REINITIALIZE_DEVICE_DATA rd_data;

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

    /* Reinitialize with no device password */
    rd_data.error_class = ERROR_CLASS_DEVICE;
    rd_data.error_code = ERROR_CODE_SUCCESS;
    rd_data.state = BACNET_REINIT_COLDSTART;
    characterstring_init_ansi(&rd_data.password, NULL);
    status = Device_Reinitialize_Password_Set(NULL);
    status = Device_Reinitialize(&rd_data);
    zassert_true(status, NULL);
    zassert_equal(
        rd_data.error_class, ERROR_CLASS_DEVICE, "error-class=%s",
        bactext_error_class_name(rd_data.error_class));
    zassert_equal(
        rd_data.error_code, ERROR_CODE_SUCCESS, "error-code=%s",
        bactext_error_code_name(rd_data.error_code));
    /* Reinitialize with device valid password, service no password */
    status = Device_Reinitialize_Password_Set("valid");
    zassert_true(status, NULL);
    status = characterstring_init_ansi(&rd_data.password, NULL);
    zassert_true(status, NULL);
    status = Device_Reinitialize(&rd_data);
    zassert_false(status, NULL);
    zassert_equal(
        rd_data.error_class, ERROR_CLASS_SECURITY, "error-class=%s",
        bactext_error_class_name(rd_data.error_class));
    zassert_equal(
        rd_data.error_code, ERROR_CODE_PASSWORD_FAILURE, "error-code=%s",
        bactext_error_code_name(rd_data.error_code));
    /* Reinitialize with device valid password, service invalid password */
    status = characterstring_init_ansi(&rd_data.password, "invalid");
    zassert_true(status, NULL);
    status = Device_Reinitialize(&rd_data);
    zassert_false(status, NULL);
    zassert_equal(
        rd_data.error_class, ERROR_CLASS_SECURITY, "error-class=%s",
        bactext_error_class_name(rd_data.error_class));
    zassert_equal(
        rd_data.error_code, ERROR_CODE_PASSWORD_FAILURE, "error-code=%s",
        bactext_error_code_name(rd_data.error_code));
    /* Reinitialize with device valid password, service valid password */
    characterstring_init_ansi(&rd_data.password, "valid");
    status = Device_Reinitialize(&rd_data);
    zassert_true(status, NULL);
    /* Reinitialize with device valid password, service too long password */
    characterstring_init_ansi(&rd_data.password, "abcdefghijklmnopqrstuvwxyz");
    status = Device_Reinitialize(&rd_data);
    zassert_false(status, NULL);
    zassert_equal(
        rd_data.error_class, ERROR_CLASS_SERVICES, "error-class=%s",
        bactext_error_class_name(rd_data.error_class));
    zassert_equal(
        rd_data.error_code, ERROR_CODE_PARAMETER_OUT_OF_RANGE, "error-code=%s",
        bactext_error_code_name(rd_data.error_code));
    /* Reinitialize with device no password, unsupported state */
    status = Device_Reinitialize_Password_Set(NULL);
    zassert_true(status, NULL);
    rd_data.state = BACNET_REINIT_MAX;
    status = Device_Reinitialize(&rd_data);
    zassert_false(status, NULL);
    zassert_equal(
        rd_data.error_class, ERROR_CLASS_SERVICES, "error-class=%s",
        bactext_error_class_name(rd_data.error_class));
    zassert_equal(
        rd_data.error_code, ERROR_CODE_PARAMETER_OUT_OF_RANGE, "error-code=%s",
        bactext_error_code_name(rd_data.error_code));

    return;
}
/**
 * @}
 */

#if defined(CONFIG_ZTEST_NEW_API)
ZTEST_SUITE(device_tests, NULL, NULL, NULL, NULL, NULL);
#else
void test_main(void)
{
    ztest_test_suite(
        device_tests, ztest_unit_test(testDevice),
        ztest_unit_test(test_Device_Data_Sharing));

    ztest_run_test_suite(device_tests);
}
#endif
