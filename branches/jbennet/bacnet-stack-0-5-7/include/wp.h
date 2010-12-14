/*####COPYRIGHTBEGIN####
 -------------------------------------------
 Copyright (C) 2005 Steve Karg

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
#ifndef WRITEPROPERTY_H
#define WRITEPROPERTY_H

#include <stdint.h>
#include <stdbool.h>
#include "bacdcode.h"
#include "bacapp.h"
#include "apdu.h"

/** @note: write property can have application tagged data, or context tagged data,
   or even complex data types (i.e. opening and closing tag around data).
   It could also have more than one value or element.  */

typedef struct BACnet_Write_Property_Data {
    BACNET_OBJECT_TYPE object_type;
    uint32_t object_instance;
    BACNET_PROPERTY_ID object_property;
    int32_t array_index;        /* use BACNET_ARRAY_ALL when not setting */
    uint8_t *application_data;  /* why should we copy buffer here ? we copy the pointer */
    int application_data_len;
    uint8_t priority;   /* use BACNET_NO_PRIORITY if no priority */
    BACNET_ERROR_CLASS error_class;
    BACNET_ERROR_CODE error_code;
} BACNET_WRITE_PROPERTY_DATA;

/* Forward declaration of session object */
struct bacnet_session_object;

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
    struct bacnet_session_object * sess,
    BACNET_WRITE_PROPERTY_DATA * wp_data);

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

    /* encode service */
    int wp_encode_apdu(
        uint8_t * apdu,
        int max_apdu_len,
        BACNET_WRITE_PROPERTY_DATA * wp_data);

    /* decode the service request only */
    int wp_decode_service_request(
        uint8_t * apdu,
        unsigned apdu_len,
        BACNET_WRITE_PROPERTY_DATA * wp_data);

#ifdef TEST
#include "ctest.h"
    int wp_decode_apdu(
        uint8_t * apdu,
        unsigned apdu_len,
        uint8_t * invoke_id,
        BACNET_WRITE_PROPERTY_DATA * wp_data);

    void test_ReadProperty(
        Test * pTest);
    void test_ReadPropertyAck(
        Test * pTest);
#endif

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
