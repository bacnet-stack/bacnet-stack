/*####COPYRIGHTBEGIN####
 -------------------------------------------
 Copyright (C) 2008 Steve Karg

 This program is free software; you can redistribute it and/or
 modify it under the terms of the GNU General Public License
 as published by the Free Software Foundation; either version 2
 of the License, or (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program; if not, write to:
 The Free Software Foundation, Inc.
 59 Temple Place - Suite 330
 Boston, MA  02111-1307, USA.

 As a special exception, if other files instantiate templates or
 use macros or inline functions from this file, or you compile
 this file and link it with other works to produce a work based
 on this file, this file does not by itself cause the resulting
 work to be covered by the GNU General Public License. However
 the source code for this file must still be made available in
 accordance with section (3) of the GNU General Public License.

 This exception does not invalidate any other reasons why a work
 based on this file might be covered by the GNU General Public
 License.
 -------------------------------------------
####COPYRIGHTEND####*/
#ifndef OBJECTS_H
#define OBJECTS_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "bacdef.h"
#include "bacstr.h"
#include "bacenum.h"

typedef union BACnetScale_t {
    float Float;
    int32_t Integer;
} BACNET_SCALE_T;

/* structures to hold the data gotten by ReadProperty from the device */
typedef struct object_accumulator_t {
    BACNET_OBJECT_ID Object_Identifier;
    BACNET_CHARACTER_STRING Object_Name;
    BACNET_OBJECT_TYPE Object_Type;
    uint32_t Present_Value;
    BACNET_STATUS_FLAGS Status_Flags;
    BACNET_EVENT_STATE Event_State;
    bool Out_Of_Service;
    BACNET_SCALE_T Scale;
    BACNET_ENGINEERING_UNITS Units;
    uint32_t Max_Pres_Value;
} OBJECT_ACCUMULATOR_T;

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


#endif
