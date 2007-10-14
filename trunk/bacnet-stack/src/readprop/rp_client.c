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
#include "bacenum.h"
#include "bacdcode.h"
#include "bacdef.h"
#include "rp.h"

/* encode service */
int rp_encode_apdu(uint8_t * apdu,
    uint8_t invoke_id, BACNET_READ_PROPERTY_DATA * data)
{
    int len = 0;                /* length of each encoding */
    int apdu_len = 0;           /* total length of the apdu, return value */

    if (apdu) {
        apdu[0] = PDU_TYPE_CONFIRMED_SERVICE_REQUEST;
        apdu[1] = encode_max_segs_max_apdu(0, MAX_APDU);
        apdu[2] = invoke_id;
        apdu[3] = SERVICE_CONFIRMED_READ_PROPERTY;      /* service choice */
        apdu_len = 4;
        len = encode_context_object_id(&apdu[apdu_len], 0,
            data->object_type, data->object_instance);
        apdu_len += len;
        len = encode_context_enumerated(&apdu[apdu_len], 1,
            data->object_property);
        apdu_len += len;
        /* optional array index */
        if (data->array_index != BACNET_ARRAY_ALL) {
            len = encode_context_unsigned(&apdu[apdu_len], 2,
                data->array_index);
            apdu_len += len;
        }
    }

    return apdu_len;
}

int rp_ack_decode_service_request(uint8_t * apdu, int apdu_len, /* total length of the apdu */
    BACNET_READ_PROPERTY_DATA * data)
{
    uint8_t tag_number = 0;
    uint32_t len_value_type = 0;
    int tag_len = 0;            /* length of tag decode */
    int len = 0;                /* total length of decodes */
    int object = 0, property = 0;       /* for decoding */
    uint32_t array_value = 0;   /* for decoding */

    /* FIXME: check apdu_len against the len during decode   */
    /* Tag 0: Object ID */
    if (!decode_is_context_tag(&apdu[0], 0))
        return -1;
    len = 1;
    len += decode_object_id(&apdu[len], &object, &data->object_instance);
    data->object_type = (BACNET_OBJECT_TYPE)object;
    /* Tag 1: Property ID */
    len += decode_tag_number_and_value(&apdu[len],
        &tag_number, &len_value_type);
    if (tag_number != 1)
        return -1;
    len += decode_enumerated(&apdu[len], len_value_type, &property);
    data->object_property = (BACNET_PROPERTY_ID)property;
    /* Tag 2: Optional Array Index */
    tag_len = decode_tag_number_and_value(&apdu[len],
        &tag_number, &len_value_type);
    if (tag_number == 2) {
        len += tag_len;
        len += decode_unsigned(&apdu[len], len_value_type, &array_value);
        data->array_index = array_value;
    } else
        data->array_index = BACNET_ARRAY_ALL;
    /* Tag 3: opening context tag */
    if (decode_is_opening_tag_number(&apdu[len], 3)) {
        /* a tag number of 3 is not extended so only one octet */
        len++;
        /* don't decode the application tag number or its data here */
        data->application_data = &apdu[len];
        data->application_data_len = apdu_len - len - 1 /*closing tag */ ;
    } else
        return -1;

    return len;
}
