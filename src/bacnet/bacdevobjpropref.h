/**************************************************************************
*
* Copyright (C) 2008 John Minack
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
#ifndef _BAC_DEV_PROP_REF_H_
#define _BAC_DEV_PROP_REF_H_

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "bacnet/bacnet_stack_exports.h"
#include "bacnet/bacdef.h"
#include "bacnet/bacint.h"
#include "bacnet/bacenum.h"

typedef struct BACnetDeviceObjectPropertyReference {
    /* number type first to avoid enum cast warning on = { 0 } */
    BACNET_UNSIGNED_INTEGER arrayIndex;
    BACNET_OBJECT_ID objectIdentifier;
    BACNET_PROPERTY_ID propertyIdentifier;
    BACNET_OBJECT_ID deviceIdentifier;
} BACNET_DEVICE_OBJECT_PROPERTY_REFERENCE;

/** BACnetDeviceObjectReference structure.
 * If the optional deviceIdentifier is not provided, then this refers
 * to an object inside this Device.
 */
typedef struct BACnetDeviceObjectReference {
    BACNET_OBJECT_ID deviceIdentifier;         /**< Optional, for external device. */
    BACNET_OBJECT_ID objectIdentifier;
} BACNET_DEVICE_OBJECT_REFERENCE;



#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

    BACNET_STACK_EXPORT
    int bacapp_encode_device_obj_property_ref(
        uint8_t * apdu,
        BACNET_DEVICE_OBJECT_PROPERTY_REFERENCE * value);

    BACNET_STACK_EXPORT
    int bacapp_encode_context_device_obj_property_ref(
        uint8_t * apdu,
        uint8_t tag_number,
        BACNET_DEVICE_OBJECT_PROPERTY_REFERENCE * value);

    BACNET_STACK_EXPORT
    int bacapp_decode_device_obj_property_ref(
        uint8_t * apdu,
        BACNET_DEVICE_OBJECT_PROPERTY_REFERENCE * value);

    BACNET_STACK_EXPORT
    int bacapp_decode_context_device_obj_property_ref(
        uint8_t * apdu,
        uint8_t tag_number,
        BACNET_DEVICE_OBJECT_PROPERTY_REFERENCE * value);


    BACNET_STACK_EXPORT
    int bacapp_encode_device_obj_ref(
        uint8_t * apdu,
        BACNET_DEVICE_OBJECT_REFERENCE * value);

    BACNET_STACK_EXPORT
    int bacapp_encode_context_device_obj_ref(
        uint8_t * apdu,
        uint8_t tag_number,
        BACNET_DEVICE_OBJECT_REFERENCE * value);

    BACNET_STACK_EXPORT
    int bacapp_decode_device_obj_ref(
        uint8_t * apdu,
        BACNET_DEVICE_OBJECT_REFERENCE * value);

    BACNET_STACK_EXPORT
    int bacapp_decode_context_device_obj_ref(
        uint8_t * apdu,
        uint8_t tag_number,
        BACNET_DEVICE_OBJECT_REFERENCE * value);

#ifdef TEST
#include "ctest.h"
    BACNET_STACK_EXPORT
    void testBACnetDeviceObjectPropertyReference(
        Test * pTest);
#endif

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif
