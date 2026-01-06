/**************************************************************************
 *
 * Copyright (C) 2005 Steve Karg
 *
 * SPDX-License-Identifier: GPL-2.0-or-later WITH GCC-exception-2.0
 *
 *********************************************************************/
#ifndef DEVICE_H
#define DEVICE_H

#include <stdbool.h>
#include <stdint.h>
#include "bacnet/bacdef.h"
#include "bacnet/bacenum.h"
#include "bacnet/wp.h"
#include "bacnet/readrange.h"

typedef unsigned (*object_count_function)(void);
typedef uint32_t (*object_index_to_instance_function)(unsigned index);
typedef char *(*object_name_function)(uint32_t object_instance);

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

void Device_Object_Function_Set(
    BACNET_OBJECT_TYPE object_type,
    object_count_function count_function,
    object_index_to_instance_function index_function,
    object_name_function name_function);

void Device_Init(void);

void Device_Property_Lists(
    const int32_t **pRequired,
    const int32_t **pOptional,
    const int32_t **pProprietary);

uint32_t Device_Object_Instance_Number(void);
bool Device_Set_Object_Instance_Number(uint32_t object_id);
bool Device_Valid_Object_Instance_Number(uint32_t object_id);
unsigned Device_Object_List_Count(void);
bool Device_Object_List_Identifier(
    uint32_t array_index, BACNET_OBJECT_TYPE *object_type, uint32_t *instance);

BACNET_DEVICE_STATUS Device_System_Status(void);
void Device_Set_System_Status(BACNET_DEVICE_STATUS status);

const char *Device_Vendor_Name(void);

uint16_t Device_Vendor_Identifier(void);

const char *Device_Model_Name(void);
bool Device_Set_Model_Name(const char *name, size_t length);

const char *Device_Firmware_Revision(void);

const char *Device_Application_Software_Version(void);
bool Device_Set_Application_Software_Version(const char *name, size_t length);

bool Device_Set_Object_Name(const char *name, size_t length);
const char *Device_Object_Name(void);

const char *Device_Description(void);
bool Device_Set_Description(const char *name, size_t length);

const char *Device_Location(void);
bool Device_Set_Location(const char *name, size_t length);

/* some stack-centric constant values - no set methods */
uint8_t Device_Protocol_Version(void);
uint8_t Device_Protocol_Revision(void);
BACNET_SEGMENTATION Device_Segmentation_Supported(void);

uint8_t Device_Database_Revision(void);
void Device_Set_Database_Revision(uint8_t revision);

bool Device_Valid_Object_Name(
    const char *object_name,
    BACNET_OBJECT_TYPE *object_type,
    uint32_t *object_instance);
char *Device_Valid_Object_Id(
    BACNET_OBJECT_TYPE object_type, uint32_t object_instance);

int Device_Encode_Property_APDU(
    uint8_t *apdu,
    uint32_t object_instance,
    BACNET_PROPERTY_ID property,
    uint32_t array_index,
    BACNET_ERROR_CLASS *error_class,
    BACNET_ERROR_CODE *error_code);

bool Device_Write_Property(
    BACNET_WRITE_PROPERTY_DATA *wp_data,
    BACNET_ERROR_CLASS *error_class,
    BACNET_ERROR_CODE *error_code);

bool DeviceGetRRInfo(
    uint32_t Object, /* Which particular object - obviously not important for
                        device object */

    BACNET_PROPERTY_ID Property, /* Which property */

    RR_PROP_INFO *pInfo, /* Where to put the information */

    BACNET_ERROR_CLASS *error_class,
    BACNET_ERROR_CODE *error_code);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif
