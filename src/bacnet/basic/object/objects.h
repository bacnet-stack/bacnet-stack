/**
 * @file
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date 2012
 * @brief API for basic BACnet device object list management implementation
 * @copyright SPDX-License-Identifier: MIT
 */
#ifndef BACNET_BASIC_OBJECT_OBJECTS_H
#define BACNET_BASIC_OBJECT_OBJECTS_H
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
/* BACnet Stack defines - first */
#include "bacnet/bacdef.h"
/* BACnet Stack API */
#include "bacnet/bacstr.h"

typedef struct object_device_t {
    BACNET_OBJECT_ID Object_Identifier;
    BACNET_CHARACTER_STRING Object_Name;
    BACNET_OBJECT_TYPE Object_Type;
    BACNET_DEVICE_STATUS System_Status;
    BACNET_CHARACTER_STRING Vendor_Name;
    uint16_t Vendor_Identifier;
    BACNET_CHARACTER_STRING Model_Name;
    BACNET_CHARACTER_STRING Firmware_Revision;
    BACNET_CHARACTER_STRING Application_Software_Version;
    BACNET_CHARACTER_STRING Location;
    BACNET_CHARACTER_STRING Description;
    uint8_t Protocol_Version;
    uint8_t Protocol_Revision;
    BACNET_BIT_STRING Protocol_Services_Supported;
    BACNET_BIT_STRING Protocol_Object_Types_Supported;
    OS_Keylist Object_List;
    uint32_t Max_APDU_Length_Accepted;
    BACNET_SEGMENTATION Segmentation_Supported;
    uint32_t APDU_Timeout;
    uint8_t Number_Of_APDU_Retries;
    uint32_t Database_Revision;
} OBJECT_DEVICE_T;

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

    BACNET_STACK_EXPORT
    bool objects_device_delete(int index);

    BACNET_STACK_EXPORT
    OBJECT_DEVICE_T *objects_device_new(uint32_t device_instance);

    BACNET_STACK_EXPORT
    OBJECT_DEVICE_T *objects_device_by_instance(uint32_t device_instance);

    BACNET_STACK_EXPORT
    OBJECT_DEVICE_T *objects_device_data(int index);

    BACNET_STACK_EXPORT
    int objects_device_count(void);

    BACNET_STACK_EXPORT
    uint32_t objects_device_id(int index);

    BACNET_STACK_EXPORT
    void objects_init(void);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif

