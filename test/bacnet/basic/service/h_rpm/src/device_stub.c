/**
 * @file
 * @brief Stub implementation for device-related functions needed by h_rpm
 *  tests
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @copyright SPDX-License-Identifier: MIT
 */
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
/* BACnet Stack defines - first */
#include "bacnet/bacdef.h"
/* BACnet Stack API */
#include "bacnet/basic/object/device.h"
#include "bacnet/basic/object/netport.h"

#define STUB_PROPERTY_PROPRIETARY 512

/* property lists that contain the Property_List property */
static const int32_t Stub_Required[] = { PROP_OBJECT_IDENTIFIER,
                                         PROP_PROPERTY_LIST, PROP_OBJECT_TYPE };
static const int32_t Stub_Optional[] = { PROP_DESCRIPTION, PROP_PROPERTY_LIST };
static const int32_t Stub_Proprietary[] = { PROP_PROPERTY_LIST,
                                            STUB_PROPERTY_PROPRIETARY };
static const int32_t Stub_Only_Property_List[] = { PROP_PROPERTY_LIST };

/* when true, the optional list only contains Property_List */
bool Stub_Optional_Only_Property_List = false;

void Device_Objects_Property_List(
    BACNET_OBJECT_TYPE object_type,
    uint32_t object_instance,
    struct special_property_list_t *pPropertyList)
{
    (void)object_type;
    (void)object_instance;
    pPropertyList->Required.pList = Stub_Required;
    pPropertyList->Required.count = sizeof(Stub_Required) / sizeof(int32_t);
    if (Stub_Optional_Only_Property_List) {
        pPropertyList->Optional.pList = Stub_Only_Property_List;
        pPropertyList->Optional.count =
            sizeof(Stub_Only_Property_List) / sizeof(int32_t);
    } else {
        pPropertyList->Optional.pList = Stub_Optional;
        pPropertyList->Optional.count = sizeof(Stub_Optional) / sizeof(int32_t);
    }
    pPropertyList->Proprietary.pList = Stub_Proprietary;
    pPropertyList->Proprietary.count =
        sizeof(Stub_Proprietary) / sizeof(int32_t);
}

bool Device_Valid_Object_Id(
    BACNET_OBJECT_TYPE object_type, uint32_t object_instance)
{
    return (object_type == OBJECT_ANALOG_INPUT) && (object_instance == 1);
}

uint32_t Device_Object_Instance_Number(void)
{
    return 1;
}

uint32_t Network_Port_Index_To_Instance(unsigned find_index)
{
    (void)find_index;
    return 1;
}

/* every property reads as the application-tagged unsigned integer 1 */
int Device_Read_Property(BACNET_READ_PROPERTY_DATA *rpdata)
{
    rpdata->application_data[0] = 0x21;
    rpdata->application_data[1] = 0x01;

    return 2;
}
