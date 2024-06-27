/**
 * @file
 * @brief BACnet error encode and decode helper functions
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date 2005
 * @copyright SPDX-License-Identifier: GPL-2.0-or-later WITH GCC-exception-2.0
 */
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
#include <stdint.h>
/* BACnet Stack defines - first */
#include "bacnet/bacdef.h"
/* BACnet Stack API */
#include "bacnet/bacdcode.h"
#include "bacnet/bacerror.h"

/** @file bacerror.c  Encode/Decode BACnet Errors */

/**
 * @brief Encodes BACnet Error class and code values into a PDU
 *  From clause 21. FORMAL DESCRIPTION OF APPLICATION PROTOCOL DATA UNITS
 * 
 *      Error ::= SEQUENCE {
 *          -- NOTE: The valid combinations of error-class and error-code 
 *          -- are defined in Clause 18.
 *          error-class ENUMERATED,
 *          error-code ENUMERATED
 *      }
 * 
 * @param apdu - buffer for the data to be encoded, or NULL for length
 * @param invoke_id - invokeID to be encoded
 * @param service - BACnet service to be encoded
 * @param error_class - #BACNET_ERROR_CLASS value to be encoded
 * @param error_code - #BACNET_ERROR_CODE value to be encoded
 * @return number of bytes encoded
 */
int bacerror_encode_apdu(uint8_t *apdu,
    uint8_t invoke_id,
    BACNET_CONFIRMED_SERVICE service,
    BACNET_ERROR_CLASS error_class,
    BACNET_ERROR_CODE error_code)
{
    /* length of the specific element of the PDU */
    int len = 0;
    /* total length of the apdu, return value */
    int apdu_len = 0; 

    if (apdu) {
        apdu[0] = PDU_TYPE_ERROR;
        apdu[1] = invoke_id;
        apdu[2] = service;
    }
    len = 3;
    apdu_len = len;
    if (apdu) {
        apdu += len;
    }
    /* service parameters */
    len = encode_application_enumerated(apdu, error_class);
    apdu_len += len;
    if (apdu) {
        apdu += len;
    }
    len = encode_application_enumerated(apdu, error_code);
    apdu_len += len;

    return apdu_len;
}

#if !BACNET_SVC_SERVER
/**
 * @brief Decodes from bytes a BACnet Error service APDU
 *  From clause 21. FORMAL DESCRIPTION OF APPLICATION PROTOCOL DATA UNITS
 * 
 *  Error ::= SEQUENCE {
 *      -- NOTE: The valid combinations of error-class and error-code 
 *      -- are defined in Clause 18.
 *      error-class ENUMERATED,
 *      error-code ENUMERATED
 *  }
 *
 * @param apdu - buffer of data to be decoded
 * @param apdu_size - number of bytes in the buffer
 * @param error_class - decoded #BACNET_ERROR_CLASS value
 * @param error_code - decoded #BACNET_ERROR_CODE value
 *
 * @return number of bytes decoded, or #BACNET_STATUS_ERROR (-1) if malformed
 */
int bacerror_decode_error_class_and_code(uint8_t *apdu,
    unsigned apdu_size,
    BACNET_ERROR_CLASS *error_class,
    BACNET_ERROR_CODE *error_code)
{
    int apdu_len = 0;
    int tag_len = 0;
    uint32_t decoded_value = 0;

    if (apdu) {
        /* error class */
        tag_len = bacnet_enumerated_application_decode(
            &apdu[apdu_len], apdu_size-apdu_len, &decoded_value);
        if (tag_len <= 0) {
            return BACNET_STATUS_ERROR;
        }
        if (error_class) {
            *error_class = (BACNET_ERROR_CLASS)decoded_value;
        }
        apdu_len += tag_len;
        /* error code */
        tag_len = bacnet_enumerated_application_decode(
            &apdu[apdu_len], apdu_size - apdu_len, &decoded_value);
        if (tag_len <= 0) {
            return BACNET_STATUS_ERROR;
        }
        if (error_code) {
            *error_code = (BACNET_ERROR_CODE)decoded_value;
        }
        apdu_len += tag_len;
    }

    return apdu_len;
}

/**
 * @brief Decodes from bytes a BACnet Error service
 * @param apdu - buffer of data to be decoded
 * @param apdu_size - number of bytes in the buffer
 * @param invoke_id - decoded invokeID
 * @param service - decoded BACnet service
 * @param error_class - decoded #BACNET_ERROR_CLASS value
 * @param error_code - decoded #BACNET_ERROR_CODE value
 *
 * @return number of bytes decoded, or #BACNET_STATUS_ERROR (-1) if malformed
 */
int bacerror_decode_service_request(uint8_t *apdu,
    unsigned apdu_size,
    uint8_t *invoke_id,
    BACNET_CONFIRMED_SERVICE *service,
    BACNET_ERROR_CLASS *error_class,
    BACNET_ERROR_CODE *error_code)
{
    int apdu_len = BACNET_STATUS_ERROR;
    int len = 0;

    if (apdu && (apdu_size > 2)) {
        if (invoke_id) {
            *invoke_id = apdu[0];
        }
        if (service) {
            *service = (BACNET_CONFIRMED_SERVICE)apdu[1];
        }
        len = 2;
        apdu_len = len;
        /* decode the application class and code */
        len = bacerror_decode_error_class_and_code(
            &apdu[apdu_len], apdu_size - apdu_len, error_class, error_code);
        if (len > 0) {
            apdu_len += len;
        } else {
            apdu_len = BACNET_STATUS_ERROR;
        }
    }

    return apdu_len;
}
#endif
