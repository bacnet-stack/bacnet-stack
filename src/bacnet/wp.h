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
#ifndef WRITEPROPERTY_H
#define WRITEPROPERTY_H

#include <stdint.h>
#include <stdbool.h>
#include "bacnet/bacnet_stack_exports.h"
#include "bacnet/bacdcode.h"
#include "bacnet/bacapp.h"

/** @note: write property can have application tagged data, or context tagged data,
   or even complex data types (i.e. opening and closing tag around data).
   It could also have more than one value or element.  */

typedef struct BACnet_Write_Property_Data {
    /* number type first to avoid enum cast warning on = { 0 } */
    uint32_t object_instance;
    BACNET_OBJECT_TYPE object_type;
    BACNET_PROPERTY_ID object_property;
    /* use BACNET_ARRAY_ALL when not setting */
    BACNET_ARRAY_INDEX array_index;
    uint8_t application_data[MAX_APDU];
    int application_data_len;
    uint8_t priority;   /* use BACNET_NO_PRIORITY if no priority */
    BACNET_ERROR_CLASS error_class;
    BACNET_ERROR_CODE error_code;
} BACNET_WRITE_PROPERTY_DATA;

/** Attempts to write a new value to one property for this object type
 *  of a given instance.
 * A function template; @see device.c for assignment to object types.
 * @ingroup ObjHelpers
 *
 * @param wp_data [in] Pointer to the BACnet_Write_Property_Data structure,
 *                     which is packed with the information from the WP request.
 * @return The length of the apdu encoded or -1 for error or
 *         -2 for abort message.
 */
typedef bool(
    *write_property_function) (
    BACNET_WRITE_PROPERTY_DATA * wp_data);

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

    /* encode service */
    BACNET_STACK_EXPORT
    size_t writeproperty_apdu_encode(
        uint8_t *apdu, 
        BACNET_WRITE_PROPERTY_DATA *data);
    BACNET_STACK_EXPORT
    size_t writeproperty_service_request_encode(
        uint8_t *apdu, 
        size_t apdu_size, 
        BACNET_WRITE_PROPERTY_DATA *data);
    BACNET_STACK_EXPORT
    int wp_encode_apdu(
        uint8_t * apdu,
        uint8_t invoke_id,
        BACNET_WRITE_PROPERTY_DATA * wp_data);

    /* decode the service request only */
    BACNET_STACK_EXPORT
    int wp_decode_service_request(
        uint8_t * apdu,
        unsigned apdu_len,
        BACNET_WRITE_PROPERTY_DATA * wp_data);

    BACNET_STACK_EXPORT
    bool write_property_type_valid(
        BACNET_WRITE_PROPERTY_DATA * wp_data,
        BACNET_APPLICATION_DATA_VALUE * value,
        uint8_t expected_tag);
    BACNET_STACK_EXPORT
    bool write_property_string_valid(
        BACNET_WRITE_PROPERTY_DATA * wp_data,
        BACNET_APPLICATION_DATA_VALUE * value,
        size_t len_max);
    BACNET_STACK_EXPORT
    bool write_property_empty_string_valid(
        BACNET_WRITE_PROPERTY_DATA * wp_data,
        BACNET_APPLICATION_DATA_VALUE * value,
        size_t len_max);

#ifdef __cplusplus
}
#endif /* __cplusplus */
/** @defgroup DSWP Data Sharing - Write Property Service (DS-WP)
 * @ingroup DataShare
 * 15.9 WriteProperty Service <br>
 * The WriteProperty service is used by a client BACnet-user to modify the
 * value of a single specified property of a BACnet object. This service
 * potentially allows write access to any property of any object, whether a
 * BACnet-defined object or not. Some implementors may wish to restrict write
 * access to certain properties of certain objects. In such cases, an attempt
 * to modify a restricted property shall result in the return of an error of
 * 'Error Class' PROPERTY and 'Error Code' WRITE_ACCESS_DENIED.
 */
#endif
