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
#include "bacnet/bacenum.h"
#include "bacnet/bacdcode.h"
#include "bacnet/bacdef.h"
#include "bacnet/bacerror.h"

/** @file bacerror.c  Encode/Decode BACnet Errors */

/* encode service */
int bacerror_encode_apdu(uint8_t *apdu,
    uint8_t invoke_id,
    BACNET_CONFIRMED_SERVICE service,
    BACNET_ERROR_CLASS error_class,
    BACNET_ERROR_CODE error_code)
{
    int apdu_len = 0; /* total length of the apdu, return value */

    if (apdu) {
        apdu[0] = PDU_TYPE_ERROR;
        apdu[1] = invoke_id;
        apdu[2] = service;
        apdu_len = 3;
        /* service parameters */
        apdu_len += encode_application_enumerated(&apdu[apdu_len], error_class);
        apdu_len += encode_application_enumerated(&apdu[apdu_len], error_code);
    }

    return apdu_len;
}

#if !BACNET_SVC_SERVER
/* decode the application class and code */
int bacerror_decode_error_class_and_code(uint8_t *apdu,
    unsigned apdu_len,
    BACNET_ERROR_CLASS *error_class,
    BACNET_ERROR_CODE *error_code)
{
    int len = 0;
    uint8_t tag_number = 0;
    uint32_t len_value_type = 0;
    uint32_t decoded_value = 0;

    if (apdu_len) {
        /* error class */
        len += decode_tag_number_and_value(
            &apdu[len], &tag_number, &len_value_type);
        if (tag_number != BACNET_APPLICATION_TAG_ENUMERATED) {
            return 0;
        }
        len += decode_enumerated(&apdu[len], len_value_type, &decoded_value);
        if (error_class) {
            *error_class = (BACNET_ERROR_CLASS)decoded_value;
        }
        /* error code */
        len += decode_tag_number_and_value(
            &apdu[len], &tag_number, &len_value_type);
        if (tag_number != BACNET_APPLICATION_TAG_ENUMERATED) {
            return 0;
        }
        len += decode_enumerated(&apdu[len], len_value_type, &decoded_value);
        if (error_code) {
            *error_code = (BACNET_ERROR_CODE)decoded_value;
        }
    }

    return len;
}

/* decode the service request only */
int bacerror_decode_service_request(uint8_t *apdu,
    unsigned apdu_len,
    uint8_t *invoke_id,
    BACNET_CONFIRMED_SERVICE *service,
    BACNET_ERROR_CLASS *error_class,
    BACNET_ERROR_CODE *error_code)
{
    int len = 0;

    if (apdu_len > 2) {
        if (invoke_id) {
            *invoke_id = apdu[0];
        }
        if (service) {
            *service = (BACNET_CONFIRMED_SERVICE)apdu[1];
        }
        /* decode the application class and code */
        len = bacerror_decode_error_class_and_code(
            &apdu[2], apdu_len - 2, error_class, error_code);
    }

    return len;
}
#endif
