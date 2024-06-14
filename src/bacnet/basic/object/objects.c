/**
 * @file
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date 2008
 * @brief A basic BACnet device object list management implementation
 * @copyright SPDX-License-Identifier: GPL-2.0-or-later WITH GCC-exception-2.0
 */
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "bacnet/basic/sys/keylist.h"
#include "bacnet/basic/object/objects.h"

/* list of devices */
static OS_Keylist Device_List = NULL;

void objects_init(void)
{
    if (!Device_List) {
        Device_List = Keylist_Create();
    }
}

int objects_device_count(void)
{
    objects_init();
    return Keylist_Count(Device_List);
}

uint32_t objects_device_id(int index)
{
    return Keylist_Key(Device_List, index);
}

OBJECT_DEVICE_T *objects_device_data(int index)
{
    objects_init();
    return Keylist_Data_Index(Device_List, index);
}

OBJECT_DEVICE_T *objects_device_by_instance(uint32_t device_instance)
{
    objects_init();
    return Keylist_Data(Device_List, device_instance);
}

OBJECT_DEVICE_T *objects_device_new(uint32_t device_instance)
{
    OBJECT_DEVICE_T *pDevice = NULL;
    KEY key = device_instance;

    objects_init();
    if (Device_List) {
        /* does this device already exist? */
        pDevice = Keylist_Data(Device_List, key);
        if (pDevice) {
            memset(pDevice, 0, sizeof(OBJECT_DEVICE_T));
        } else {
            pDevice = calloc(1, sizeof(OBJECT_DEVICE_T));
            if (pDevice) {
                pDevice->Object_Identifier.type = OBJECT_DEVICE;
                pDevice->Object_Identifier.instance = device_instance;
                pDevice->Object_Type = OBJECT_DEVICE;
                pDevice->Object_List = Keylist_Create();
                Keylist_Data_Add(Device_List, key, pDevice);
            }
        }
    }

    return pDevice;
}

bool objects_device_delete(int index)
{
    bool result = false;
    OBJECT_DEVICE_T *pDevice = NULL;
    BACNET_OBJECT_ID *pObject;

    objects_init();
    if (Device_List) {
        pDevice = Keylist_Data_Delete_By_Index(Device_List, index);
        if (pDevice) {
            if (pDevice->Object_List) {
                do {
                    pObject =
                        Keylist_Data_Delete_By_Index(pDevice->Object_List, 0);
                    /* free any dynamic memory used */
                    if (pObject) {
                        free(pObject);
                    }
                } while (pObject);
                Keylist_Delete(pDevice->Object_List);
            }
            free(pDevice);
            result = true;
        }
    }
    return result;
}
