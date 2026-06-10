/**
 * @file
 * @brief mock Device object functions
 * @author Mikhail Antropov
 * @date July 2022
 *
 * @copyright SPDX-License-Identifier: MIT
 */
#include <zephyr/ztest.h>
#include <bacnet/bactext.h>
#include <bacnet/basic/object/device.h>

bool Device_Valid_Object_Name(
    const BACNET_CHARACTER_STRING *object_name,
    BACNET_OBJECT_TYPE *object_type,
    uint32_t *object_instance)
{
    (void)object_name;
    (void)object_type;
    (void)object_instance;
    return true;
}

void Device_Inc_Database_Revision(void)
{
}

uint32_t Device_Object_Instance_Number(void)
{
    return 0;
}

bool Device_Write_Property(BACNET_WRITE_PROPERTY_DATA *wp_data)
{
    (void)wp_data;
    return false;
}

void Device_getCurrentDateTime(BACNET_DATE_TIME *DateTime)
{
    (void)DateTime;
}

int Device_Read_Property(BACNET_READ_PROPERTY_DATA *rpdata)
{
    (void)rpdata;
    return 0;
}

bool Device_Valid_Object_Id(
    BACNET_OBJECT_TYPE object_type, uint32_t object_instance)
{
    (void)object_type;
    (void)object_instance;
    return true;
}

bool Device_Value_List_Supported(BACNET_OBJECT_TYPE object_type)
{
    (void)object_type;
    return true;
}

bool Device_COV(BACNET_OBJECT_TYPE object_type, uint32_t object_instance)
{
    (void)object_type;
    (void)object_instance;
    return false;
}

void Device_COV_Clear(BACNET_OBJECT_TYPE object_type, uint32_t object_instance)
{
    (void)object_type;
    (void)object_instance;
}

bool Device_Encode_Value_List(
    BACNET_OBJECT_TYPE object_type,
    uint32_t object_instance,
    BACNET_PROPERTY_VALUE *value_list)
{
    (void)object_type;
    (void)object_instance;
    (void)value_list;
    return true;
}

uint16_t Routed_Device_Object_Index(void)
{
    return 0;
}

bool Set_Routed_Device_Object_Index(uint16_t index)
{
    (void)index;
    return true;
}

uint16_t Get_Num_Managed_Devices(void)
{
    return 1;
}
