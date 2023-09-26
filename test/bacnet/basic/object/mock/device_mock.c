/**
 * @file
 * @brief mock Device object functions
 * @author Mikhail Antropov
 * @date July 2022
 *
 * SPDX-License-Identifier: MIT
 */
#include <zephyr/ztest.h>
#include <bacnet/bactext.h>
#include <bacnet/basic/object/device.h>
#include <bacnet/version.h>

bool Device_Valid_Object_Name(
    BACNET_CHARACTER_STRING * object_name,
    BACNET_OBJECT_TYPE *object_type,
    uint32_t * object_instance)
{
    (void)object_name;
    (void)object_type;
    (void)object_instance;
    return true;
}

void Device_Inc_Database_Revision(
    void)
{

}

uint32_t Device_Object_Instance_Number(
    void)
{
    return 0;
}

bool Device_Write_Property(
    BACNET_WRITE_PROPERTY_DATA * wp_data)
{
    (void)wp_data;
    return false;
}

void Device_getCurrentDateTime(
        BACNET_DATE_TIME * DateTime)
{
    (void)DateTime;
}

int Device_Read_Property(
    BACNET_READ_PROPERTY_DATA * rpdata)
{
    (void)rpdata;
    return 0;
}

static char *Vendor_Name = BACNET_VENDOR_NAME;
static uint16_t Vendor_Identifier = BACNET_VENDOR_ID;
static char *Model_Name = "GNU";
static char *Application_Software_Version = "1.0";
static const char *BACnet_Version = BACNET_VERSION_TEXT;

uint16_t Device_Vendor_Identifier(void)
{
    return Vendor_Identifier;
}

const char *Device_Vendor_Name(void)
{
    return Vendor_Name;
}

const char *Device_Model_Name(void)
{
    return Model_Name;
}

const char *Device_Application_Software_Version(void)
{
    return Application_Software_Version;
}

uint8_t Device_Protocol_Version(void)
{
    return BACNET_PROTOCOL_VERSION;
}

uint8_t Device_Protocol_Revision(void)
{
    return BACNET_PROTOCOL_REVISION;
}
