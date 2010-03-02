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
#ifndef READPROPERTY_H
#define READPROPERTY_H

#include <stdint.h>
#include <stdbool.h>
#include "bacdef.h"
#include "bacenum.h"

typedef struct BACnet_Read_Property_Data {
    BACNET_OBJECT_TYPE object_type;
    uint32_t object_instance;
    BACNET_PROPERTY_ID object_property;
    int32_t array_index;
    uint8_t *application_data;
    int application_data_len;
    BACNET_ERROR_CLASS error_class;
    BACNET_ERROR_CODE error_code;
} BACNET_READ_PROPERTY_DATA;

/** Reads one property for this object type of a given instance.
 * A function template; @see device.c for assignment to object types.
 *
 * @param rp_data [in] Pointer to the BACnet_Read_Property_Data structure,
 *                     which is packed with the information from the RP request.
 * @return The length of the apdu encoded or -1 for error or
 *         -2 for abort message.
 */
typedef int (
    *read_property_function) (
    BACNET_READ_PROPERTY_DATA *rp_data);

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* encode service */
    int rp_encode_apdu(
        uint8_t * apdu,
        uint8_t invoke_id,
        BACNET_READ_PROPERTY_DATA * rpdata);

/* decode the service request only */
    int rp_decode_service_request(
        uint8_t * apdu,
        unsigned apdu_len,
        BACNET_READ_PROPERTY_DATA * rpdata);

    /* method to encode the ack without extra buffer */
    int rp_ack_encode_apdu_init(
        uint8_t * apdu,
        uint8_t invoke_id,
        BACNET_READ_PROPERTY_DATA * rpdata);

    int rp_ack_encode_apdu_object_property_end(
        uint8_t * apdu);

    /* method to encode the ack using extra buffer */
    int rp_ack_encode_apdu(
        uint8_t * apdu,
        uint8_t invoke_id,
        BACNET_READ_PROPERTY_DATA * rpdata);

    int rp_ack_decode_service_request(
        uint8_t * apdu,
        int apdu_len,   /* total length of the apdu */
        BACNET_READ_PROPERTY_DATA * rpdata);


#ifdef TEST
#include "ctest.h"
    int rp_decode_apdu(
        uint8_t * apdu,
        unsigned apdu_len,
        uint8_t * invoke_id,
        BACNET_READ_PROPERTY_DATA * rpdata);

    int rp_ack_decode_apdu(
        uint8_t * apdu,
        int apdu_len,   /* total length of the apdu */
        uint8_t * invoke_id,
        BACNET_READ_PROPERTY_DATA * rpdata);

    void test_ReadProperty(
        Test * pTest);
    void test_ReadPropertyAck(
        Test * pTest);
#endif

#ifdef __cplusplus
}
#endif /* __cplusplus */

/** @defgroup DataShare Data Sharing BIBBs
 * These BIBBs prescribe the BACnet capabilities required to interoperably 
 * perform the data sharing functions enumerated in 22.2.1.1 for the BACnet 
 * devices defined therein.
 */

/** @defgroup DSRP Data Sharing -Read Property Service (DS-RP)
 * @ingroup DataShare
 * 15.5 ReadProperty Service <br>
 * The ReadProperty service is used by a client BACnet-user to request the 
 * value of one property of one BACnet Object. This service allows read access 
 * to any property of any object, whether a BACnet-defined object or not.
 */

#endif
