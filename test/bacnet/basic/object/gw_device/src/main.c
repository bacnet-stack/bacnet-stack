/**
 * @file
 * @brief Unit test for the routed Device object functions in gw_device.c
 * @copyright SPDX-License-Identifier: MIT
 */
#include <string.h>
#include <zephyr/ztest.h>
#include <bacnet/bacdcode.h>
#include <bacnet/bacapp.h>
#include <bacnet/reject.h>
#include <bacnet/rp.h>
#include <bacnet/wp.h>
#include <bacnet/basic/object/device.h>

/**
 * @addtogroup bacnet_tests
 * @{
 */

#define GATEWAY_INSTANCE 260001
#define VIRTUAL_DNET 2709
#define DEVICE_COUNT 3

/* not declared in device.h */
int Routed_Device_Read_Property_Local(BACNET_READ_PROPERTY_DATA *rpdata);
bool Routed_Device_Write_Property_Local(BACNET_WRITE_PROPERTY_DATA *wp_data);

static bool Write_Local_Called;
static int Read_Local_Result = 1234;

/* stubs for the base Device object handlers called by gw_device.c */
int Device_Read_Property_Local(BACNET_READ_PROPERTY_DATA *rpdata)
{
    (void)rpdata;
    return Read_Local_Result;
}

bool Device_Write_Property_Local(BACNET_WRITE_PROPERTY_DATA *wp_data)
{
    (void)wp_data;
    Write_Local_Called = true;
    return true;
}

/**
 * @brief Reset the routed Device table and add the gateway and two Devices.
 *  Each Device has the MAC address 0x10 + index, and the current index is 0.
 */
static void test_setup(void)
{
    BACNET_CHARACTER_STRING name = { 0 };
    BACNET_ADDRESS *address;
    uint16_t i;

    Write_Local_Called = false;
    Routed_Device_Table_Reset();
    for (i = 0; i < DEVICE_COUNT; i++) {
        characterstring_init_ansi(&name, i == 0 ? "Gateway" : "Virtual");
        zassert_equal(
            Add_Routed_Device(GATEWAY_INSTANCE + i, &name, "Description"), i,
            NULL);
        address = Get_Routed_Device_Address(i);
        zassert_not_null(address, NULL);
        address->net = (i == 0) ? 0 : VIRTUAL_DNET;
        address->len = 1;
        address->adr[0] = 0x10 + i;
    }
    zassert_true(Set_Routed_Device_Object_Index(0), NULL);
}

#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(gw_device_tests, test_Routed_Device_Table)
#else
static void test_Routed_Device_Table(void)
#endif
{
    BACNET_CHARACTER_STRING name = { 0 };
    DEVICE_OBJECT_DATA *device;
    uint16_t i;
    char long_name[MAX_DEV_NAME_LEN + 8] = { 0 };

    Routed_Device_Table_Reset();
    zassert_equal(Get_Num_Managed_Devices(), 0, NULL);
    zassert_equal(Routed_Device_Object_Index(), 0, NULL);

    /* default name and description */
    zassert_equal(Add_Routed_Device(GATEWAY_INSTANCE, NULL, NULL), 0, NULL);
    zassert_equal(Get_Num_Managed_Devices(), 1, NULL);
    device = Get_Routed_Device_Object(-1);
    zassert_not_null(device, NULL);
    zassert_equal(device->bacObj.mObject_Type, OBJECT_DEVICE, NULL);
    zassert_equal(
        device->bacObj.Object_Instance_Number, GATEWAY_INSTANCE, NULL);
    zassert_equal(strcmp(device->bacObj.Object_Name, "No Name"), 0, NULL);
    zassert_equal(strcmp(device->Description, "No Descr"), 0, NULL);
    zassert_equal(device->Database_Revision, 0, NULL);
    zassert_equal(device->Reinitialize.State, BACNET_REINIT_IDLE, NULL);

    /* the added Device becomes the current Device */
    characterstring_init_ansi(&name, "Virtual");
    zassert_equal(
        Add_Routed_Device(GATEWAY_INSTANCE + 1, &name, "Desc"), 1, NULL);
    zassert_equal(Routed_Device_Object_Index(), 1, NULL);
    zassert_equal(
        Routed_Device_Object_Instance_Number(), GATEWAY_INSTANCE + 1, NULL);

    /* a name that is too long is not stored */
    memset(long_name, 'a', sizeof(long_name) - 1);
    characterstring_init_ansi(&name, long_name);
    zassert_equal(
        Add_Routed_Device(GATEWAY_INSTANCE + 2, &name, NULL), 2, NULL);
    device = Get_Routed_Device_Object(-1);
    zassert_equal(strlen(device->bacObj.Object_Name), 0, NULL);

    /* table is full */
    for (i = Get_Num_Managed_Devices(); i < MAX_NUM_DEVICES; i++) {
        zassert_equal(
            Add_Routed_Device(GATEWAY_INSTANCE + i, NULL, NULL), i, NULL);
    }
    zassert_equal(Get_Num_Managed_Devices(), MAX_NUM_DEVICES, NULL);
    zassert_equal(Add_Routed_Device(1, NULL, NULL), UINT16_MAX, NULL);
    zassert_equal(Get_Num_Managed_Devices(), MAX_NUM_DEVICES, NULL);

    /* reset */
    Routed_Device_Table_Reset();
    zassert_equal(Get_Num_Managed_Devices(), 0, NULL);
    zassert_equal(Routed_Device_Object_Index(), 0, NULL);
}

#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(gw_device_tests, test_Routed_Device_Index)
#else
static void test_Routed_Device_Index(void)
#endif
{
    DEVICE_OBJECT_DATA *device;
    BACNET_ADDRESS *address;
    BACNET_ADDRESS my_address = { 0 };

    test_setup();
    zassert_true(Set_Routed_Device_Object_Index(1), NULL);
    zassert_equal(Routed_Device_Object_Index(), 1, NULL);
    zassert_false(Set_Routed_Device_Object_Index(MAX_NUM_DEVICES), NULL);
    zassert_equal(Routed_Device_Object_Index(), 1, NULL);

    /* -1 means the current Device */
    device = Get_Routed_Device_Object(-1);
    zassert_not_null(device, NULL);
    zassert_equal(
        device->bacObj.Object_Instance_Number, GATEWAY_INSTANCE + 1, NULL);
    address = Get_Routed_Device_Address(-1);
    zassert_not_null(address, NULL);
    zassert_equal(address->adr[0], 0x11, NULL);
    Routed_Device_Get_My_Address(&my_address);
    zassert_equal(my_address.net, VIRTUAL_DNET, NULL);
    zassert_equal(my_address.adr[0], 0x11, NULL);
    Routed_Device_Get_My_Address(NULL);

    /* a valid index selects the Device */
    device = Get_Routed_Device_Object(2);
    zassert_not_null(device, NULL);
    zassert_equal(Routed_Device_Object_Index(), 2, NULL);
    zassert_equal(
        device->bacObj.Object_Instance_Number, GATEWAY_INSTANCE + 2, NULL);
    address = Get_Routed_Device_Address(0);
    zassert_not_null(address, NULL);
    zassert_equal(Routed_Device_Object_Index(), 0, NULL);
    zassert_equal(address->adr[0], 0x10, NULL);

    /* an invalid index is NULL and does not change the current Device */
    zassert_true(Set_Routed_Device_Object_Index(1), NULL);
    zassert_is_null(Get_Routed_Device_Object(DEVICE_COUNT), NULL);
    zassert_is_null(Get_Routed_Device_Object(-2), NULL);
    zassert_is_null(Get_Routed_Device_Address(DEVICE_COUNT), NULL);
    zassert_is_null(Get_Routed_Device_Address(-2), NULL);
    zassert_equal(Routed_Device_Object_Index(), 1, NULL);
}

#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(gw_device_tests, test_Routed_Device_Address_Lookup)
#else
static void test_Routed_Device_Address_Lookup(void)
#endif
{
    uint8_t adr[MAX_MAC_LEN] = { 0 };

    test_setup();
    /* match by MAC address selects the Device */
    adr[0] = 0x12;
    zassert_true(Routed_Device_Address_Lookup(2, 1, adr), NULL);
    zassert_equal(Routed_Device_Object_Index(), 2, NULL);

    /* mismatch does not change the current Device */
    adr[0] = 0x11;
    zassert_false(Routed_Device_Address_Lookup(1, 1, NULL), NULL);
    zassert_false(Routed_Device_Address_Lookup(0, 1, adr), NULL);
    zassert_equal(Routed_Device_Object_Index(), 2, NULL);

    /* broadcast MAC is an automatic match */
    zassert_true(Routed_Device_Address_Lookup(1, 0, NULL), NULL);
    zassert_equal(Routed_Device_Object_Index(), 1, NULL);

    /* invalid index */
    zassert_false(Routed_Device_Address_Lookup(-1, 0, NULL), NULL);
    zassert_false(Routed_Device_Address_Lookup(DEVICE_COUNT, 0, NULL), NULL);
    zassert_equal(Routed_Device_Object_Index(), 1, NULL);
}

#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(gw_device_tests, test_Routed_Device_GetNext)
#else
static void test_Routed_Device_GetNext(void)
#endif
{
    const int32_t dnet_list[] = { VIRTUAL_DNET, -1 };
    BACNET_ADDRESS dest = { 0 };
    int cursor = 0;

    test_setup();

    /* local network: only the gateway matches, once */
    dest.net = 0;
    dest.len = 0;
    zassert_true(Routed_Device_GetNext(&dest, dnet_list, &cursor), NULL);
    zassert_equal(Routed_Device_Object_Index(), 0, NULL);
    zassert_equal(cursor, -1, NULL);
    zassert_false(Routed_Device_GetNext(&dest, dnet_list, &cursor), NULL);
    zassert_equal(cursor, -1, NULL);

    /* global broadcast: every Device gets a chance */
    dest.net = BACNET_BROADCAST_NETWORK;
    cursor = 0;
    zassert_true(Routed_Device_GetNext(&dest, dnet_list, &cursor), NULL);
    zassert_equal(Routed_Device_Object_Index(), 0, NULL);
    zassert_equal(cursor, 1, NULL);
    zassert_true(Routed_Device_GetNext(&dest, dnet_list, &cursor), NULL);
    zassert_equal(Routed_Device_Object_Index(), 1, NULL);
    zassert_equal(cursor, 2, NULL);
    zassert_true(Routed_Device_GetNext(&dest, dnet_list, &cursor), NULL);
    zassert_equal(Routed_Device_Object_Index(), 2, NULL);
    zassert_equal(cursor, -1, NULL);
    zassert_false(Routed_Device_GetNext(&dest, dnet_list, &cursor), NULL);

    /* virtual network broadcast: only the routed Devices */
    dest.net = VIRTUAL_DNET;
    dest.len = 0;
    cursor = 0;
    zassert_true(Routed_Device_GetNext(&dest, dnet_list, &cursor), NULL);
    zassert_equal(Routed_Device_Object_Index(), 1, NULL);
    zassert_equal(cursor, 2, NULL);
    zassert_true(Routed_Device_GetNext(&dest, dnet_list, &cursor), NULL);
    zassert_equal(Routed_Device_Object_Index(), 2, NULL);
    zassert_equal(cursor, -1, NULL);

    /* virtual network unicast: matches on the MAC address */
    dest.len = 1;
    dest.adr[0] = 0x12;
    cursor = 0;
    zassert_true(Routed_Device_GetNext(&dest, dnet_list, &cursor), NULL);
    zassert_equal(Routed_Device_Object_Index(), 2, NULL);
    zassert_equal(cursor, -1, NULL);
    /* the gateway MAC address is not on the virtual network */
    dest.adr[0] = 0x10;
    cursor = 0;
    zassert_false(Routed_Device_GetNext(&dest, dnet_list, &cursor), NULL);
    zassert_equal(cursor, -1, NULL);
    dest.adr[0] = 0x99;
    cursor = 0;
    zassert_false(Routed_Device_GetNext(&dest, dnet_list, &cursor), NULL);
    zassert_equal(cursor, -1, NULL);

    /* unreachable network */
    dest.net = VIRTUAL_DNET + 1;
    cursor = 0;
    zassert_false(Routed_Device_GetNext(&dest, dnet_list, &cursor), NULL);
    zassert_equal(cursor, -1, NULL);

    /* cursor out of range */
    dest.net = BACNET_BROADCAST_NETWORK;
    dest.len = 0;
    cursor = DEVICE_COUNT;
    zassert_false(Routed_Device_GetNext(&dest, dnet_list, &cursor), NULL);
    zassert_equal(cursor, -1, NULL);
}

#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(gw_device_tests, test_Routed_Device_Is_Valid_Network)
#else
static void test_Routed_Device_Is_Valid_Network(void)
#endif
{
    const int32_t dnet_list[] = { VIRTUAL_DNET, -1 };

    zassert_true(
        Routed_Device_Is_Valid_Network(BACNET_BROADCAST_NETWORK, dnet_list),
        NULL);
    zassert_true(Routed_Device_Is_Valid_Network(0, dnet_list), NULL);
    zassert_true(Routed_Device_Is_Valid_Network(VIRTUAL_DNET, dnet_list), NULL);
    zassert_false(
        Routed_Device_Is_Valid_Network(VIRTUAL_DNET + 1, dnet_list), NULL);
}

#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(gw_device_tests, test_Routed_Device_Object_Functions)
#else
static void test_Routed_Device_Object_Functions(void)
#endif
{
    BACNET_CHARACTER_STRING name = { 0 };
    DEVICE_OBJECT_DATA *device;

    test_setup();
    zassert_true(Set_Routed_Device_Object_Index(1), NULL);

    zassert_equal(
        Routed_Device_Index_To_Instance(0), GATEWAY_INSTANCE + 1, NULL);
    zassert_equal(
        Routed_Device_Index_To_Instance(5), GATEWAY_INSTANCE + 1, NULL);
    zassert_equal(
        Routed_Device_Object_Instance_Number(), GATEWAY_INSTANCE + 1, NULL);

    /* only the addressed Device is valid */
    zassert_true(
        Routed_Device_Valid_Object_Instance_Number(GATEWAY_INSTANCE + 1), NULL);
    zassert_false(
        Routed_Device_Valid_Object_Instance_Number(GATEWAY_INSTANCE), NULL);
    zassert_false(
        Routed_Device_Valid_Object_Instance_Number(GATEWAY_INSTANCE + 2), NULL);
    zassert_equal(Routed_Device_Object_Index(), 1, NULL);

    zassert_true(Routed_Device_Name(GATEWAY_INSTANCE + 1, &name), NULL);
    zassert_equal(strcmp(characterstring_value(&name), "Virtual"), 0, NULL);
    zassert_false(Routed_Device_Name(GATEWAY_INSTANCE, &name), NULL);

    /* changing the instance only changes the addressed Device */
    zassert_false(
        Routed_Device_Set_Object_Instance_Number(BACNET_MAX_INSTANCE + 1),
        NULL);
    zassert_equal(
        Routed_Device_Object_Instance_Number(), GATEWAY_INSTANCE + 1, NULL);
    zassert_true(Routed_Device_Set_Object_Instance_Number(500), NULL);
    zassert_equal(Routed_Device_Object_Instance_Number(), 500, NULL);
    zassert_true(Routed_Device_Valid_Object_Instance_Number(500), NULL);
    zassert_false(
        Routed_Device_Valid_Object_Instance_Number(GATEWAY_INSTANCE + 1), NULL);
    device = Get_Routed_Device_Object(0);
    zassert_equal(
        device->bacObj.Object_Instance_Number, GATEWAY_INSTANCE, NULL);
    device = Get_Routed_Device_Object(1);
    zassert_equal(device->Database_Revision, 1, NULL);

    /* name and description */
    zassert_true(
        Routed_Device_Set_Object_Name(CHARACTER_UTF8, "Renamed", 7), NULL);
    zassert_equal(strcmp(device->bacObj.Object_Name, "Renamed"), 0, NULL);
    zassert_equal(device->Database_Revision, 2, NULL);
    zassert_false(
        Routed_Device_Set_Object_Name(CHARACTER_UTF8, "x", MAX_DEV_NAME_LEN),
        NULL);
    zassert_false(Routed_Device_Set_Object_Name(CHARACTER_UCS2, "x", 1), NULL);
    zassert_equal(strcmp(device->bacObj.Object_Name, "Renamed"), 0, NULL);
    zassert_true(Routed_Device_Set_Description("New", 3), NULL);
    zassert_equal(strcmp(device->Description, "New"), 0, NULL);
    zassert_false(Routed_Device_Set_Description("x", MAX_DEV_DESC_LEN), NULL);
    zassert_equal(strcmp(device->Description, "New"), 0, NULL);
    Routed_Device_Inc_Database_Revision();
    zassert_equal(device->Database_Revision, 3, NULL);
    device = Get_Routed_Device_Object(0);
    zassert_equal(device->Database_Revision, 0, NULL);
}

#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(gw_device_tests, test_Routed_Device_Read_Property_Local)
#else
static void test_Routed_Device_Read_Property_Local(void)
#endif
{
    BACNET_READ_PROPERTY_DATA rpdata = { 0 };
    BACNET_CHARACTER_STRING value = { 0 };
    BACNET_UNSIGNED_INTEGER unsigned_value = 0;
    BACNET_OBJECT_TYPE object_type = OBJECT_NONE;
    uint32_t instance = 0;
    uint8_t apdu[MAX_APDU] = { 0 };
    int len;

    test_setup();
    zassert_true(Set_Routed_Device_Object_Index(1), NULL);
    rpdata.application_data = apdu;
    rpdata.application_data_len = sizeof(apdu);
    rpdata.object_type = OBJECT_DEVICE;
    rpdata.object_instance = GATEWAY_INSTANCE + 1;
    rpdata.array_index = BACNET_ARRAY_ALL;

    rpdata.object_property = PROP_OBJECT_IDENTIFIER;
    len = Routed_Device_Read_Property_Local(&rpdata);
    zassert_true(len > 0, NULL);
    len =
        bacnet_object_id_application_decode(apdu, len, &object_type, &instance);
    zassert_true(len > 0, NULL);
    zassert_equal(object_type, OBJECT_DEVICE, NULL);
    zassert_equal(instance, GATEWAY_INSTANCE + 1, NULL);

    rpdata.object_property = PROP_OBJECT_NAME;
    len = Routed_Device_Read_Property_Local(&rpdata);
    zassert_true(len > 0, NULL);
    len = bacnet_character_string_application_decode(apdu, len, &value);
    zassert_true(len > 0, NULL);
    zassert_equal(strcmp(characterstring_value(&value), "Virtual"), 0, NULL);

    rpdata.object_property = PROP_DESCRIPTION;
    len = Routed_Device_Read_Property_Local(&rpdata);
    zassert_true(len > 0, NULL);
    len = bacnet_character_string_application_decode(apdu, len, &value);
    zassert_true(len > 0, NULL);
    zassert_equal(
        strcmp(characterstring_value(&value), "Description"), 0, NULL);

    Routed_Device_Inc_Database_Revision();
    rpdata.object_property = PROP_DATABASE_REVISION;
    len = Routed_Device_Read_Property_Local(&rpdata);
    zassert_true(len > 0, NULL);
    len = bacnet_unsigned_application_decode(apdu, len, &unsigned_value);
    zassert_true(len > 0, NULL);
    zassert_equal(unsigned_value, 1, NULL);

    /* other properties are handed to the base Device handler */
    rpdata.object_property = PROP_VENDOR_NAME;
    zassert_equal(
        Routed_Device_Read_Property_Local(&rpdata), Read_Local_Result, NULL);

    /* invalid request */
    zassert_equal(Routed_Device_Read_Property_Local(NULL), 0, NULL);
    rpdata.application_data_len = 0;
    zassert_equal(Routed_Device_Read_Property_Local(&rpdata), 0, NULL);
    rpdata.application_data_len = sizeof(apdu);
    rpdata.application_data = NULL;
    zassert_equal(Routed_Device_Read_Property_Local(&rpdata), 0, NULL);
}

/**
 * @brief Encode the value, and write the property of the current Device
 */
static bool write_value(
    BACNET_WRITE_PROPERTY_DATA *wpdata,
    BACNET_PROPERTY_ID property,
    BACNET_APPLICATION_DATA_VALUE *value)
{
    wpdata->object_type = OBJECT_DEVICE;
    wpdata->object_instance = Routed_Device_Object_Instance_Number();
    wpdata->object_property = property;
    wpdata->array_index = BACNET_ARRAY_ALL;
    wpdata->application_data_len =
        bacapp_encode_application_data(wpdata->application_data, value);
    return Routed_Device_Write_Property_Local(wpdata);
}

#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(gw_device_tests, test_Routed_Device_Write_Property_Local)
#else
static void test_Routed_Device_Write_Property_Local(void)
#endif
{
    BACNET_WRITE_PROPERTY_DATA wpdata = { 0 };
    BACNET_APPLICATION_DATA_VALUE value = { 0 };
    DEVICE_OBJECT_DATA *device;
    char long_name[MAX_DEV_NAME_LEN + 8] = { 0 };
    bool status;

    test_setup();
    zassert_true(Set_Routed_Device_Object_Index(1), NULL);
    device = Get_Routed_Device_Object(1);

    /* Object_Identifier */
    value.tag = BACNET_APPLICATION_TAG_OBJECT_ID;
    value.type.Object_Id.type = OBJECT_DEVICE;
    value.type.Object_Id.instance = 777;
    status = write_value(&wpdata, PROP_OBJECT_IDENTIFIER, &value);
    zassert_true(status, NULL);
    zassert_equal(device->bacObj.Object_Instance_Number, 777, NULL);
    zassert_equal(
        Get_Routed_Device_Object(0)->bacObj.Object_Instance_Number,
        GATEWAY_INSTANCE, NULL);
    zassert_true(Set_Routed_Device_Object_Index(1), NULL);

    value.type.Object_Id.type = OBJECT_ANALOG_INPUT;
    status = write_value(&wpdata, PROP_OBJECT_IDENTIFIER, &value);
    zassert_false(status, NULL);
    zassert_equal(wpdata.error_class, ERROR_CLASS_PROPERTY, NULL);
    zassert_equal(wpdata.error_code, ERROR_CODE_VALUE_OUT_OF_RANGE, NULL);
    zassert_equal(device->bacObj.Object_Instance_Number, 777, NULL);

    value.tag = BACNET_APPLICATION_TAG_UNSIGNED_INT;
    value.type.Unsigned_Int = 1;
    status = write_value(&wpdata, PROP_OBJECT_IDENTIFIER, &value);
    zassert_false(status, NULL);
    zassert_equal(wpdata.error_code, ERROR_CODE_INVALID_DATA_TYPE, NULL);

    /* Object_Name */
    value.tag = BACNET_APPLICATION_TAG_CHARACTER_STRING;
    characterstring_init_ansi(&value.type.Character_String, "Written");
    status = write_value(&wpdata, PROP_OBJECT_NAME, &value);
    zassert_true(status, NULL);
    zassert_equal(strcmp(device->bacObj.Object_Name, "Written"), 0, NULL);
    zassert_equal(
        strcmp(Get_Routed_Device_Object(0)->bacObj.Object_Name, "Gateway"), 0,
        NULL);
    zassert_true(Set_Routed_Device_Object_Index(1), NULL);

    characterstring_init_ansi(&value.type.Character_String, "");
    status = write_value(&wpdata, PROP_OBJECT_NAME, &value);
    zassert_false(status, NULL);
    zassert_equal(wpdata.error_code, ERROR_CODE_VALUE_OUT_OF_RANGE, NULL);

    memset(long_name, 'a', sizeof(long_name) - 1);
    characterstring_init_ansi(&value.type.Character_String, long_name);
    status = write_value(&wpdata, PROP_OBJECT_NAME, &value);
    zassert_false(status, NULL);
    zassert_equal(wpdata.error_class, ERROR_CLASS_RESOURCES, NULL);
    zassert_equal(
        wpdata.error_code, ERROR_CODE_NO_SPACE_TO_WRITE_PROPERTY, NULL);
    zassert_equal(strcmp(device->bacObj.Object_Name, "Written"), 0, NULL);

    /* the longest name that fits is one less than the buffer size */
    memset(long_name, 'b', MAX_DEV_NAME_LEN - 1);
    long_name[MAX_DEV_NAME_LEN - 1] = 0;
    characterstring_init_ansi(&value.type.Character_String, long_name);
    status = write_value(&wpdata, PROP_OBJECT_NAME, &value);
    zassert_true(status, NULL);
    zassert_equal(strcmp(device->bacObj.Object_Name, long_name), 0, NULL);

    /* a name that does not fit is rejected and the old name is kept */
    memset(long_name, 'c', MAX_DEV_NAME_LEN);
    long_name[MAX_DEV_NAME_LEN] = 0;
    characterstring_init_ansi(&value.type.Character_String, long_name);
    status = write_value(&wpdata, PROP_OBJECT_NAME, &value);
    zassert_false(status, NULL);
    zassert_equal(wpdata.error_class, ERROR_CLASS_RESOURCES, NULL);
    zassert_equal(
        wpdata.error_code, ERROR_CODE_NO_SPACE_TO_WRITE_PROPERTY, NULL);
    zassert_equal(device->bacObj.Object_Name[0], 'b', NULL);
    characterstring_init_ansi(&value.type.Character_String, "Written");
    zassert_true(write_value(&wpdata, PROP_OBJECT_NAME, &value), NULL);

    value.tag = BACNET_APPLICATION_TAG_UNSIGNED_INT;
    value.type.Unsigned_Int = 1;
    status = write_value(&wpdata, PROP_OBJECT_NAME, &value);
    zassert_false(status, NULL);
    zassert_equal(wpdata.error_code, ERROR_CODE_INVALID_DATA_TYPE, NULL);

    /* other properties are handed to the base Device handler */
    zassert_false(Write_Local_Called, NULL);
    status = write_value(&wpdata, PROP_DESCRIPTION, &value);
    zassert_true(status, NULL);
    zassert_true(Write_Local_Called, NULL);

    /* data that cannot be decoded */
    wpdata.object_property = PROP_OBJECT_NAME;
    wpdata.application_data[0] = 0x75; /* character string, length 5 */
    wpdata.application_data[1] = 0x00;
    wpdata.application_data_len = 2;
    status = Routed_Device_Write_Property_Local(&wpdata);
    zassert_false(status, NULL);
    zassert_equal(wpdata.error_class, ERROR_CLASS_PROPERTY, NULL);
}

#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(gw_device_tests, test_Routed_Device_Service_Approval)
#else
static void test_Routed_Device_Service_Approval(void)
#endif
{
    uint8_t apdu[MAX_APDU] = { 0 };

    test_setup();
    /* the gateway supports everything */
    zassert_equal(
        Routed_Device_Service_Approval(
            SERVICE_SUPPORTED_DEVICE_COMMUNICATION_CONTROL, 0, apdu, 7),
        0, NULL);
    zassert_true(Set_Routed_Device_Object_Index(1), NULL);
    zassert_equal(
        Routed_Device_Service_Approval(
            SERVICE_SUPPORTED_REINITIALIZE_DEVICE, 0, apdu, 7),
        0, NULL);
    zassert_equal(
        Routed_Device_Service_Approval(
            SERVICE_SUPPORTED_READ_PROPERTY, 0, apdu, 7),
        0, NULL);
    /* DCC is rejected for a routed Device */
    zassert_equal(
        Routed_Device_Service_Approval(
            SERVICE_SUPPORTED_DEVICE_COMMUNICATION_CONTROL, 0, apdu, 7),
        3, NULL);
    zassert_equal(apdu[0], PDU_TYPE_REJECT, NULL);
    zassert_equal(apdu[1], 7, NULL);
    zassert_equal(apdu[2], REJECT_REASON_UNRECOGNIZED_SERVICE, NULL);
    zassert_equal(
        Routed_Device_Service_Approval(
            SERVICE_SUPPORTED_DEVICE_COMMUNICATION_CONTROL, 0, NULL, 7),
        1, NULL);
}

/**
 * @}
 */

#if defined(CONFIG_ZTEST_NEW_API)
ZTEST_SUITE(gw_device_tests, NULL, NULL, NULL, NULL, NULL);
#else
void test_main(void)
{
    ztest_test_suite(
        gw_device_tests, ztest_unit_test(test_Routed_Device_Table),
        ztest_unit_test(test_Routed_Device_Index),
        ztest_unit_test(test_Routed_Device_Address_Lookup),
        ztest_unit_test(test_Routed_Device_GetNext),
        ztest_unit_test(test_Routed_Device_Is_Valid_Network),
        ztest_unit_test(test_Routed_Device_Object_Functions),
        ztest_unit_test(test_Routed_Device_Read_Property_Local),
        ztest_unit_test(test_Routed_Device_Write_Property_Local),
        ztest_unit_test(test_Routed_Device_Service_Approval));

    ztest_run_test_suite(gw_device_tests);
}
#endif
