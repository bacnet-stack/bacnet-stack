/**************************************************************************
*
* Copyright (C) 2012 Steve Karg <skarg@users.sourceforge.net>
*
* Permission is hereby granted, free of charge, to any person obtaining
* a copy of this software and associated documentation files (the
* "Software"), to deal in the Software without restriction, including
* without limitation the rights to use, copy, modify, merge, publish,
* distribute, sublicense, and/or sell copies of the Software, and to
* permit persons to whom the Software is furnished to do so, subject to
* the following conditions:
*
* The above copyright notice and this permission notice shall be included
* in all copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*********************************************************************/
#ifndef BACNET_OBJECTS_H
#define BACNET_OBJECTS_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "bacnet/bacnet_stack_exports.h"
#include "bacnet/bacdef.h"
#include "bacnet/bacstr.h"
#include "bacnet/bacenum.h"

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
    OBJECT_DEVICE_T *objects_device_delete(int index);
    
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

