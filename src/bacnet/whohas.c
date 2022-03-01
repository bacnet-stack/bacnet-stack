/*####COPYRIGHTBEGIN####
 -------------------------------------------
 Copyright (C) 2006 Steve Karg

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
#include "bacnet/whohas.h"

/** @file whohas.c  Encode/Decode Who-Has requests */

/** Encode Who-Hasservice  - use -1 for limit for unlimited
 *
 * @param apdu  Pointer to the APDU.
 * @param data  Pointer to the Who-Has application data.
 *
 * @return Bytes encoded. */
int whohas_encode_apdu(uint8_t *apdu, BACNET_WHO_HAS_DATA *data)
{
    int len = 0; /* length of each encoding */
    int apdu_len = 0; /* total length of the apdu, return value */

    if (apdu && data) {
        apdu[0] = PDU_TYPE_UNCONFIRMED_SERVICE_REQUEST;
        apdu[1] = SERVICE_UNCONFIRMED_WHO_HAS; /* service choice */
        apdu_len = 2;
        /* optional limits - must be used as a pair */
        if ((data->low_limit >= 0) &&
            (data->low_limit <= BACNET_MAX_INSTANCE) &&
            (data->high_limit >= 0) &&
            (data->high_limit <= BACNET_MAX_INSTANCE)) {
            len = encode_context_unsigned(&apdu[apdu_len], 0, data->low_limit);
            apdu_len += len;
            len = encode_context_unsigned(&apdu[apdu_len], 1, data->high_limit);
            apdu_len += len;
        }
        if (data->is_object_name) {
            len = encode_context_character_string(
                &apdu[apdu_len], 3, &data->object.name);
            apdu_len += len;
        } else {
            len = encode_context_object_id(&apdu[apdu_len], 2,
                data->object.identifier.type, data->object.identifier.instance);
            apdu_len += len;
        }
    }

    return apdu_len;
}

/** Decode the Who-Has service request only.
 *
 * @param apdu  Pointer to the received data.
 * @param apdu_len  Bytes valid in the receive buffer.
 * @param data  Pointer to the application dta to be filled in.
 *
 * @return Bytes decoded. */
int whohas_decode_service_request(
    uint8_t *apdu, unsigned apdu_len, BACNET_WHO_HAS_DATA *data)
{
    int len = 0;
    uint8_t tag_number = 0;
    uint32_t len_value = 0;
    BACNET_UNSIGNED_INTEGER unsigned_value = 0;
    BACNET_OBJECT_TYPE decoded_type = OBJECT_NONE;

    if (apdu_len && data) {
        /* optional limits - must be used as a pair */
        if (decode_is_context_tag(&apdu[len], 0)) {
            len += decode_tag_number_and_value(
                &apdu[len], &tag_number, &len_value);
            if ((unsigned)len < apdu_len) {
                len += decode_unsigned(&apdu[len], len_value, &unsigned_value);
                if ((unsigned)len < apdu_len) {
                    if (unsigned_value <= BACNET_MAX_INSTANCE) {
                        data->low_limit = unsigned_value;
                    }
                    if (!decode_is_context_tag(&apdu[len], 1)) {
                        return -1;
                    }
                    len += decode_tag_number_and_value(
                        &apdu[len], &tag_number, &len_value);
                    if ((unsigned)len < apdu_len) {
                        len += decode_unsigned(
                            &apdu[len], len_value, &unsigned_value);
                        if (unsigned_value <= BACNET_MAX_INSTANCE) {
                            data->high_limit = unsigned_value;
                        }
                    }
                }
            }
        } else {
            data->low_limit = -1;
            data->high_limit = -1;
        }
        /* object id */
        if ((unsigned)len < apdu_len) {
            if (decode_is_context_tag(&apdu[len], 2)) {
                data->is_object_name = false;
                len += decode_tag_number_and_value(
                    &apdu[len], &tag_number, &len_value);
                if ((unsigned)len < apdu_len) {
                    len += decode_object_id(&apdu[len], &decoded_type,
                        &data->object.identifier.instance);
                    data->object.identifier.type = decoded_type;
                }
            }
            /* object name */
            else if (decode_is_context_tag(&apdu[len], 3)) {
                data->is_object_name = true;
                len += decode_tag_number_and_value(
                    &apdu[len], &tag_number, &len_value);
                if ((unsigned)len < apdu_len) {
                    len += decode_character_string(
                        &apdu[len], len_value, &data->object.name);
                }
            } else {
                /* missing required parameters */
                return -1;
            }
        } else {
            /* message too short */
            return -1;
        }
    }

    return len;
}
