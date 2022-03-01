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
#include <stdint.h>
#include "bacnet/lso.h"
#include "bacnet/bacdcode.h"
#include "bacnet/apdu.h"

/** @file lso.c  BACnet Life Safety Operation encode/decode */

int lso_encode_apdu(uint8_t *apdu, uint8_t invoke_id, BACNET_LSO_DATA *data)
{
    int len = 0; /* length of each encoding */
    int apdu_len = 0; /* total length of the apdu, return value */

    if (apdu && data) {
        apdu[0] = PDU_TYPE_CONFIRMED_SERVICE_REQUEST;
        apdu[1] = encode_max_segs_max_apdu(0, MAX_APDU);
        apdu[2] = invoke_id;
        apdu[3] = SERVICE_CONFIRMED_LIFE_SAFETY_OPERATION;
        apdu_len = 4;
        /* tag 0 - requestingProcessId */
        len = encode_context_unsigned(&apdu[apdu_len], 0, data->processId);
        apdu_len += len;
        /* tag 1 - requestingSource */
        len = encode_context_character_string(
            &apdu[apdu_len], 1, &data->requestingSrc);
        apdu_len += len;
        /*
           Operation
         */
        len = encode_context_enumerated(&apdu[apdu_len], 2, data->operation);
        apdu_len += len;
        /*
           Object ID
         */
        if (data->use_target) {
            len = encode_context_object_id(&apdu[apdu_len], 3,
                data->targetObject.type, data->targetObject.instance);
            apdu_len += len;
        }
    }

    return apdu_len;
}

int lso_decode_service_request(
    uint8_t *apdu, unsigned apdu_len, BACNET_LSO_DATA *data)
{
    int len = 0; /* return value */
    int section_length = 0; /* length returned from decoding */
    uint32_t operation = 0; /* handles decoded value */
    BACNET_UNSIGNED_INTEGER unsigned_value = 0;

    /* check for value pointers */
    if (apdu_len && data) {
        /* Tag 0: Object ID          */
        section_length =
            decode_context_unsigned(&apdu[len], 0, &unsigned_value);
        if (section_length == BACNET_STATUS_ERROR) {
            return BACNET_STATUS_ERROR;
        }
        data->processId = (uint32_t)unsigned_value;
        len += section_length;
        section_length = decode_context_character_string(
            &apdu[len], 1, &data->requestingSrc);
        if (section_length == BACNET_STATUS_ERROR) {
            return BACNET_STATUS_ERROR;
        }
        len += section_length;
        section_length = decode_context_enumerated(&apdu[len], 2, &operation);
        if (section_length == BACNET_STATUS_ERROR) {
            return BACNET_STATUS_ERROR;
        }
        data->operation = (BACNET_LIFE_SAFETY_OPERATION)operation;
        len += section_length;
        /*
         ** This is an optional parameter, so don't fail if it doesn't exist
         */
        if (decode_is_context_tag(&apdu[len], 3)) {
            section_length = decode_context_object_id(&apdu[len], 3,
                &data->targetObject.type, &data->targetObject.instance);
            if (section_length == BACNET_STATUS_ERROR) {
                return BACNET_STATUS_ERROR;
            }
            data->use_target = true;
            len += section_length;
        } else {
            data->use_target = false;
            data->targetObject.type = OBJECT_NONE;
            data->targetObject.instance = 0;
        }
        return len;
    }

    return 0;
}
