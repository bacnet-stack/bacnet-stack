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
#ifndef RPM_H
#define RPM_H

#include <stdint.h>
#include <stdbool.h>
#include "bacenum.h"
#include "bacdef.h"
#include "bacapp.h"

struct BACnet_Read_Access_Data;
typedef struct BACnet_Read_Access_Data {
    BACNET_OBJECT_TYPE object_type;
    uint32_t object_instance;
    /* simple linked list of values */
    BACNET_PROPERTY_REFERENCE *listOfProperties;
    struct BACnet_Read_Access_Data *next;
} BACNET_READ_ACCESS_DATA;


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* encode functions */
/* Start with the Init function, and then add an object,
 then add its properties, and then end the object.
 Continue to add objects and properties as needed
 until the APDU is full.*/

/* RPM */
    int rpm_encode_apdu_init(
        uint8_t * apdu,
        uint8_t invoke_id);

    int rpm_encode_apdu_object_begin(
        uint8_t * apdu,
        BACNET_OBJECT_TYPE object_type,
        uint32_t object_instance);

    int rpm_encode_apdu_object_property(
        uint8_t * apdu,
        BACNET_PROPERTY_ID object_property,
        int32_t array_index);

    int rpm_encode_apdu_object_end(
        uint8_t * apdu);

    int rpm_encode_apdu(
        uint8_t * apdu,
        size_t max_apdu,
        uint8_t invoke_id,
        BACNET_READ_ACCESS_DATA * read_access_data);

/* decode the object portion of the service request only */
    int rpm_decode_object_id(
        uint8_t * apdu,
        unsigned apdu_len,
        BACNET_OBJECT_TYPE * object_type,
        uint32_t * object_instance);

/* is this the end of this object property list? */
    int rpm_decode_object_end(
        uint8_t * apdu,
        unsigned apdu_len);

/* decode the object property portion of the service request only */
    int rpm_decode_object_property(
        uint8_t * apdu,
        unsigned apdu_len,
        BACNET_PROPERTY_ID * object_property,
        int32_t * array_index);

/* RPM Ack - reply from server */
    int rpm_ack_encode_apdu_init(
        uint8_t * apdu,
        uint8_t invoke_id);

    int rpm_ack_encode_apdu_object_begin(
        uint8_t * apdu,
        BACNET_OBJECT_TYPE object_type,
        uint32_t object_instance);

    int rpm_ack_encode_apdu_object_property(
        uint8_t * apdu,
        BACNET_PROPERTY_ID object_property,
        int32_t array_index);

    int rpm_ack_encode_apdu_object_property_value(
        uint8_t * apdu,
        uint8_t * application_data,
        unsigned application_data_len);

    int rpm_ack_encode_apdu_object_property_error(
        uint8_t * apdu,
        BACNET_ERROR_CLASS error_class,
        BACNET_ERROR_CODE error_code);

    int rpm_ack_encode_apdu_object_end(
        uint8_t * apdu);

    int rpm_ack_decode_object_id(
        uint8_t * apdu,
        unsigned apdu_len,
        BACNET_OBJECT_TYPE * object_type,
        uint32_t * object_instance);
/* is this the end of the list of this objects properties values? */
    int rpm_ack_decode_object_end(
        uint8_t * apdu,
        unsigned apdu_len);
    int rpm_ack_decode_object_property(
        uint8_t * apdu,
        unsigned apdu_len,
        BACNET_PROPERTY_ID * object_property,
        int32_t * array_index);
#ifdef TEST
#include "ctest.h"
    int rpm_decode_apdu(
        uint8_t * apdu,
        unsigned apdu_len,
        uint8_t * invoke_id,
        uint8_t ** service_request,
        unsigned *service_request_len);

    int rpm_ack_decode_apdu(
        uint8_t * apdu,
        int apdu_len,   /* total length of the apdu */
        uint8_t * invoke_id,
        uint8_t ** service_request,
        unsigned *service_request_len);

    void testReadPropertyMultiple(
        Test * pTest);
    void testReadPropertyMultipleAck(
        Test * pTest);
#endif

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif
