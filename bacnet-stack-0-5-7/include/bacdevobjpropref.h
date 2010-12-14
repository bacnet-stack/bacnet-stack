/*####COPYRIGHTBEGIN####
 -------------------------------------------
 Copyright (C) 2008 John Minack

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

#ifndef _BAC_DEV_PROP_REF_H_
#define _BAC_DEV_PROP_REF_H_

typedef struct BACnet_Property_Ref {
    BACNET_PROPERTY_ID propertyIdentifier;
    uint32_t arrayIndex;        /* Optional */
} BACNET_PROPERTY_REF;

typedef struct BACnet_Object_Property_Reference {
    BACNET_OBJECT_ID objectIdentifier;
    BACNET_PROPERTY_ID propertyIdentifier;
    uint32_t arrayIndex;        /* Optional */
} BACNET_OBJECT_PROPERTY_REFERENCE;

typedef struct BACnet_Device_Object_Property_Reference {
    BACNET_OBJECT_ID deviceIndentifier; /* Optional (0, 0) */
    BACNET_OBJECT_PROPERTY_REFERENCE objectPropertyRef;
} BACNET_DEVICE_OBJECT_PROPERTY_REFERENCE;

typedef struct BACnet_Device_Object_Reference {
    BACNET_OBJECT_ID deviceIndentifier; /* Optional (0, 0) */
    BACNET_OBJECT_ID objectIdentifier;
} BACNET_DEVICE_OBJECT_REFERENCE;

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

    /* BACnetPropertyReference */
    int bacapp_encode_property_ref(
        uint8_t * apdu,
        BACNET_PROPERTY_REF * value);

    int bacapp_encode_context_property_ref(
        uint8_t * apdu,
        uint8_t tag_number,
        BACNET_PROPERTY_REF * value);

    int bacapp_decode_property_ref(
        uint8_t * apdu,
        BACNET_PROPERTY_REF * value);

    int bacapp_decode_context_property_ref(
        uint8_t * apdu,
        uint8_t tag_number,
        BACNET_PROPERTY_REF * value);


    /* BACnetDeviceObjectPropertyReference */

    int bacapp_encode_device_obj_property_ref(
        uint8_t * apdu,
        BACNET_DEVICE_OBJECT_PROPERTY_REFERENCE * value);

    int bacapp_encode_context_device_obj_property_ref(
        uint8_t * apdu,
        uint8_t tag_number,
        BACNET_DEVICE_OBJECT_PROPERTY_REFERENCE * value);

    int bacapp_decode_device_obj_property_ref(
        uint8_t * apdu,
        BACNET_DEVICE_OBJECT_PROPERTY_REFERENCE * value);

    int bacapp_decode_context_device_obj_property_ref(
        uint8_t * apdu,
        uint8_t tag_number,
        BACNET_DEVICE_OBJECT_PROPERTY_REFERENCE * value);

    /* BACnetObjectPropertyReference */

    int bacapp_encode_obj_property_ref(
        uint8_t * apdu,
        BACNET_OBJECT_PROPERTY_REFERENCE * value);

    int bacapp_encode_context_obj_property_ref(
        uint8_t * apdu,
        uint8_t tag_number,
        BACNET_OBJECT_PROPERTY_REFERENCE * value);

    int bacapp_decode_obj_property_ref(
        uint8_t * apdu,
        BACNET_OBJECT_PROPERTY_REFERENCE * value);

    int bacapp_decode_context_obj_property_ref(
        uint8_t * apdu,
        uint8_t tag_number,
        BACNET_OBJECT_PROPERTY_REFERENCE * value);

    /* BACnetDeviceObjectReference */

    int bacapp_encode_device_obj_ref(
        uint8_t * apdu,
        BACNET_DEVICE_OBJECT_REFERENCE * value);

    int bacapp_encode_context_device_obj_ref(
        uint8_t * apdu,
        uint8_t tag_number,
        BACNET_DEVICE_OBJECT_REFERENCE * value);

    int bacapp_decode_device_obj_ref(
        uint8_t * apdu,
        BACNET_DEVICE_OBJECT_REFERENCE * value);

    int bacapp_decode_context_device_obj_ref(
        uint8_t * apdu,
        uint8_t tag_number,
        BACNET_DEVICE_OBJECT_REFERENCE * value);


#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif
