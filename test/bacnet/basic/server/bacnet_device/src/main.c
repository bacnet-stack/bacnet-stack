/**
 * @file
 * @brief test BACnet Device object APIs
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date 2004
 * @copyright SPDX-License-Identifier: MIT
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
    const uint32_t instance = 123;
    unsigned count;
    bool status = false;
    uint32_t test_instance;
    const int32_t skip_fail_property_list[] = { -1 };

    Device_Init(NULL);
    status = Device_Set_Object_Instance_Number(instance);
    zassert_true(status, NULL);
    test_instance = Device_Object_Instance_Number();
    zassert_equal(test_instance, instance, NULL);
    status = Device_Valid_Object_Instance_Number(BACNET_MAX_INSTANCE);
    zassert_false(status, NULL);
    count = Device_Count();
    zassert_equal(count, 1, NULL);
    test_instance = Device_Index_To_Instance(0);
    zassert_equal(test_instance, instance, NULL);
    /* perform a general test for RP/WP */
    bacnet_object_properties_read_write_test(
        OBJECT_DEVICE, instance, Device_Property_Lists, Device_Read_Property,
        Device_Write_Property, skip_fail_property_list);
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
    BACNET_REINITIALIZE_DEVICE_DATA rd_data = { 0 };
    BACNET_READ_PROPERTY_DATA rpdata = { 0 };
    BACNET_CREATE_OBJECT_DATA create_data = { 0 };
    BACNET_DELETE_OBJECT_DATA delete_data = { 0 };
    BACNET_LIST_ELEMENT_DATA list_data = { 0 };
    BACNET_PROPERTY_ID property = 0;
    BACNET_OBJECT_TYPE object_type = OBJECT_NONE;
    uint32_t object_instance = 0;
    BACNET_CHARACTER_STRING char_string = { 0 }, test_char_string = { 0 };
    uint8_t uuid[16] = { 0 }, test_uuid[16] = { 0 };
    BACNET_TIMESTAMP time_of_restart = { 0 }, test_time_of_restart = { 0 };
    BACNET_DATE_TIME date_time = { 0 }, test_date_time = { 0 };
    const char *name_string = NULL;
    unsigned count = 0, test_count = 0;
    int len = 0, time_diff = 0;

    Device_Init(NULL);
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
    rd_data.error_class = ERROR_CLASS_DEVICE;
    rd_data.error_code = ERROR_CODE_SUCCESS;
    rd_data.state = BACNET_REINIT_STARTBACKUP;
    status = Device_Reinitialize(&rd_data);
    zassert_false(status, NULL);
    zassert_equal(
        rd_data.error_class, ERROR_CLASS_SERVICES, "error-class=%s",
        bactext_error_class_name(rd_data.error_class));
    rd_data.error_class = ERROR_CLASS_DEVICE;
    rd_data.error_code = ERROR_CODE_SUCCESS;
    rd_data.state = BACNET_REINIT_ENDBACKUP;
    status = Device_Reinitialize(&rd_data);
    zassert_false(status, NULL);
    zassert_equal(
        rd_data.error_class, ERROR_CLASS_SERVICES, "error-class=%s",
        bactext_error_class_name(rd_data.error_class));
    rd_data.error_class = ERROR_CLASS_DEVICE;
    rd_data.error_code = ERROR_CODE_SUCCESS;
    rd_data.state = BACNET_REINIT_STARTRESTORE;
    status = Device_Reinitialize(&rd_data);
    zassert_false(status, NULL);
    zassert_equal(
        rd_data.error_class, ERROR_CLASS_SERVICES, "error-class=%s",
        bactext_error_class_name(rd_data.error_class));
    rd_data.error_class = ERROR_CLASS_DEVICE;
    rd_data.error_code = ERROR_CODE_SUCCESS;
    rd_data.state = BACNET_REINIT_ENDRESTORE;
    status = Device_Reinitialize(&rd_data);
    zassert_false(status, NULL);
    zassert_equal(
        rd_data.error_class, ERROR_CLASS_SERVICES, "error-class=%s",
        bactext_error_class_name(rd_data.error_class));
    rd_data.error_class = ERROR_CLASS_DEVICE;
    rd_data.error_code = ERROR_CODE_SUCCESS;
    rd_data.state = BACNET_REINIT_ABORTRESTORE;
    status = Device_Reinitialize(&rd_data);
    zassert_false(status, NULL);
    zassert_equal(
        rd_data.error_class, ERROR_CLASS_SERVICES, "error-class=%s",
        bactext_error_class_name(rd_data.error_class));
    rd_data.error_class = ERROR_CLASS_DEVICE;
    rd_data.error_code = ERROR_CODE_SUCCESS;
    rd_data.state = BACNET_REINIT_ACTIVATE_CHANGES;
    status = Device_Reinitialize(&rd_data);
    zassert_true(status, NULL);
    zassert_equal(
        rd_data.state, Device_Reinitialized_State(), "state=%d", rd_data.state);
    Device_Reinitialize_State_Set(BACNET_REINIT_IDLE);
    zassert_equal(Device_Reinitialized_State(), BACNET_REINIT_IDLE, NULL);
    /* test Object_List API */
    count = Device_Object_List_Count();
    zassert_true(count > 0, NULL);
    status = Device_Object_List_Identifier(0, &object_type, &object_instance);
    zassert_false(status, NULL);
    status = Device_Object_List_Identifier(1, &object_type, &object_instance);
    zassert_true(status, NULL);
    status = Device_Valid_Object_Id(object_type, object_instance);
    zassert_true(status, NULL);
    object_type = OBJECT_DEVICE;
    object_instance = Device_Object_Instance_Number();
    for (property = 0; property < MAX_BACNET_PROPERTY_ID; property++) {
        if (Device_Objects_Property_List_Member(
                object_type, object_instance, property)) {
            continue;
        }
        if ((property == PROP_ALL) || (property == PROP_REQUIRED) ||
            (property == PROP_OPTIONAL) || (property == PROP_PROPERTY_LIST)) {
            continue;
        }
        rpdata.object_property = property;
        rpdata.array_index = BACNET_ARRAY_ALL;
        len = Device_Read_Property(&rpdata);
        zassert_equal(
            len, BACNET_STATUS_ERROR,
            "property '%s' array_index=ALL: Missing.\n",
            bactext_property_name(rpdata.object_property));
        /* shrink the number space and skip proprietary range values */
        if (property == PROP_RESERVED_RANGE_MAX) {
            property = PROP_RESERVED_RANGE_MIN2 - 1;
        }
        /* shrink the number space to known values */
        if (property == PROP_RESERVED_RANGE_LAST) {
            break;
        }
    }
    /* test Object Name API */
    status = Device_Set_Object_Name(NULL);
    zassert_false(status, NULL);
    characterstring_init_ansi(&char_string, "Teddy");
    status = Device_Set_Object_Name(&char_string);
    zassert_true(status, NULL);
    Device_Object_Name(Device_Object_Instance_Number(), &test_char_string);
    status = characterstring_same(&char_string, &test_char_string);
    zassert_true(status, NULL);
    Device_Object_Name_ANSI_Init("Tuxedo");
    name_string = Device_Object_Name_ANSI();
    zassert_not_null(name_string, NULL);
    zassert_equal(strcmp(name_string, "Tuxedo"), 0, NULL);
    status = Device_Object_Name_Copy(
        OBJECT_DEVICE, Device_Object_Instance_Number(), &test_char_string);
    zassert_true(status, NULL);
    status = characterstring_same(&char_string, &test_char_string);
    /* UUID tests */
    Device_UUID_Get(NULL, 0);
    Device_UUID_Set(NULL, 0);
    Device_UUID_Get(test_uuid, sizeof(test_uuid));
    for (int i = 0; i < 16; i++) {
        zassert_equal(test_uuid[i], 0, NULL);
    }
    Device_UUID_Init();
    Device_UUID_Get(uuid, sizeof(uuid));
    status = false;
    for (int i = 0; i < 16; i++) {
        if (uuid[i] != test_uuid[i]) {
            status = true;
        }
    }
    zassert_true(status, NULL);
    Device_UUID_Set(test_uuid, sizeof(test_uuid));
    Device_UUID_Get(uuid, sizeof(uuid));
    for (int i = 0; i < 16; i++) {
        zassert_equal(uuid[i], test_uuid[i], NULL);
    }
    name_string = Device_Vendor_Name();
    zassert_not_null(name_string, NULL);
    zassert_true(strlen(name_string) > 0, NULL);
    status = Device_Set_Vendor_Name(NULL, 0);
    zassert_true(status, NULL);
    name_string = Device_Firmware_Revision();
    zassert_not_null(name_string, NULL);
    zassert_true(strlen(name_string) > 0, NULL);
    status = Device_Set_Firmware_Revision(NULL, 0);
    zassert_true(status, NULL);
    name_string = Device_Description();
    zassert_not_null(name_string, NULL);
    zassert_true(strlen(name_string) > 0, NULL);
    status = Device_Set_Description(NULL, 0);
    zassert_true(status, NULL);
    name_string = Device_Location();
    zassert_not_null(name_string, NULL);
    zassert_true(strlen(name_string) > 0, NULL);
    status = Device_Set_Location(NULL, 0);
    zassert_true(status, NULL);
    name_string = Device_Serial_Number();
    zassert_not_null(name_string, NULL);
    zassert_true(strlen(name_string) > 0, NULL);
    status = Device_Serial_Number_Set(NULL, 0);
    zassert_true(status, NULL);
    Device_Set_Time_Of_Restart(NULL);
    time_of_restart.tag = TIME_STAMP_TIME;
    time_of_restart.value.time.hour = 1;
    time_of_restart.value.time.min = 2;
    time_of_restart.value.time.sec = 3;
    time_of_restart.value.time.hundredths = 4;
    Device_Set_Time_Of_Restart(&time_of_restart);
    Device_Time_Of_Restart(&test_time_of_restart);
    status = bacapp_timestamp_same(&time_of_restart, &test_time_of_restart);
    zassert_true(status, NULL);
    Device_Set_Database_Revision(0);
    zassert_equal(Device_Database_Revision(), 0, NULL);
    Device_Inc_Database_Revision();
    zassert_equal(Device_Database_Revision(), 1, NULL);
    Device_Inc_Database_Revision();
    zassert_equal(Device_Database_Revision(), 2, NULL);
    Device_getCurrentDateTime(&date_time);
    datetime_local(&date_time.date, &date_time.time, NULL, NULL);
    time_diff = datetime_compare(&date_time, &test_date_time);
    zassert_equal(time_diff, 0, NULL);
    Device_UTC_Offset_Set(-60);
    time_diff = Device_UTC_Offset();
    zassert_equal(time_diff, -60, NULL);
    status = Device_Daylight_Savings_Status();
    zassert_false(status, NULL);

    /* Add/Remove List Elements */
    list_data.object_type = OBJECT_DEVICE;
    list_data.object_instance = Device_Object_Instance_Number();
    list_data.object_property = PROP_ACTIVE_COV_SUBSCRIPTIONS;
    list_data.array_index = BACNET_ARRAY_ALL;
    list_data.application_data = NULL;
    list_data.application_data_len = 0;
    list_data.first_failed_element_number = 0;
    list_data.error_class = ERROR_CLASS_DEVICE;
    list_data.error_code = ERROR_CODE_SUCCESS;
    len = Device_Add_List_Element(&list_data);
    zassert_true(len < 0, NULL);
    list_data.object_type = OBJECT_ANALOG_VALUE;
    list_data.object_instance = BACNET_MAX_INSTANCE;
    len = Device_Add_List_Element(&list_data);
    zassert_true(len < 0, NULL);
    list_data.object_instance = 1;
    len = Device_Add_List_Element(&list_data);
    zassert_true(len < 0, NULL);
    zassert_equal(
        list_data.error_code, ERROR_CODE_UNKNOWN_OBJECT, "error-code=%s",
        bactext_error_code_name(list_data.error_code));
    Device_Remove_List_Element(&list_data);
    zassert_equal(
        list_data.error_code, ERROR_CODE_UNKNOWN_OBJECT, "error-code=%s",
        bactext_error_code_name(list_data.error_code));
    list_data.object_type = OBJECT_ANALOG_INPUT;
    len = Device_Add_List_Element(&list_data);
    zassert_true(len < 0, NULL);
    zassert_equal(
        list_data.error_code, ERROR_CODE_UNKNOWN_OBJECT, "error-code=%s",
        bactext_error_code_name(list_data.error_code));

    /* COV API tests */
    status = Device_COV(OBJECT_ANALOG_VALUE, BACNET_MAX_INSTANCE);
    zassert_false(status, NULL);
    Device_COV_Clear(OBJECT_ANALOG_VALUE, BACNET_MAX_INSTANCE);
    status = Device_Encode_Value_List(
        OBJECT_ANALOG_VALUE, BACNET_MAX_INSTANCE, NULL);
    zassert_false(status, NULL);
    /* create/delete object tests */
    count = Device_Object_List_Count();
    create_data.object_type = OBJECT_ANALOG_VALUE;
    create_data.object_instance = BACNET_MAX_INSTANCE;
    create_data.list_of_initial_values = NULL;
    create_data.error_class = ERROR_CLASS_DEVICE;
    create_data.error_code = ERROR_CODE_SUCCESS;
    create_data.first_failed_element_number = 0;
    status = Device_Create_Object(&create_data);
    zassert_true(status, NULL);
    zassert_equal(create_data.error_code, ERROR_CODE_SUCCESS, NULL);
    zassert_equal(create_data.object_instance, 1, NULL);
    test_count = Device_Object_List_Count();
    zassert_equal(count + 1, test_count, NULL);
    delete_data.object_type = create_data.object_type;
    delete_data.object_instance = create_data.object_instance;
    delete_data.error_class = ERROR_CLASS_DEVICE;
    delete_data.error_code = ERROR_CODE_SUCCESS;
    status = Device_Delete_Object(&delete_data);
    zassert_true(status, NULL);
    zassert_equal(delete_data.error_code, ERROR_CODE_SUCCESS, NULL);
    test_count = Device_Object_List_Count();
    zassert_equal(count, test_count, NULL);
    status = Device_Delete_Object(&delete_data);
    zassert_false(status, NULL);
    /* known object without delete */
    delete_data.object_type = OBJECT_DEVICE;
    delete_data.object_instance = Device_Object_Instance_Number();
    delete_data.error_class = ERROR_CLASS_DEVICE;
    delete_data.error_code = ERROR_CODE_SUCCESS;
    status = Device_Delete_Object(&delete_data);
    zassert_false(status, NULL);
    delete_data.object_type = MAX_BACNET_OBJECT_TYPE;
    status = Device_Delete_Object(&delete_data);
    zassert_false(status, NULL);
    /* known object without create */
    create_data.object_type = OBJECT_DEVICE;
    create_data.object_instance = BACNET_MAX_INSTANCE;
    create_data.list_of_initial_values = NULL;
    create_data.error_class = ERROR_CLASS_DEVICE;
    create_data.error_code = ERROR_CODE_SUCCESS;
    create_data.first_failed_element_number = 0;
    status = Device_Create_Object(&create_data);
    zassert_false(status, NULL);
    create_data.object_type = MAX_BACNET_OBJECT_TYPE;
    status = Device_Create_Object(&create_data);
    zassert_false(status, NULL);
    /* COV value list */
    status = Device_Value_List_Supported(OBJECT_DEVICE);
    zassert_false(status, NULL);
    /* object timers */
    Device_Timer(1000);
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
