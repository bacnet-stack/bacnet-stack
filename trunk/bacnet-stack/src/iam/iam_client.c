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
#include "bacdef.h"
#include "npdu.h"
#include "dcc.h"
#include "datalink.h"
#include "device.h"
#include "bacdcode.h"
#include "address.h"

int iam_decode_service_request(uint8_t * apdu,
    uint32_t * pDevice_id,
    unsigned *pMax_apdu, int *pSegmentation, uint16_t * pVendor_id)
{
    int len = 0;
    int apdu_len = 0;           /* total length of the apdu, return value */
    int object_type = 0;        /* should be a Device Object */
    uint32_t object_instance = 0;
    uint8_t tag_number = 0;
    uint32_t len_value = 0;
    uint32_t decoded_value = 0;
    int decoded_integer = 0;

    /* OBJECT ID - object id */
    len =
        decode_tag_number_and_value(&apdu[apdu_len], &tag_number,
        &len_value);
    apdu_len += len;
    if (tag_number != BACNET_APPLICATION_TAG_OBJECT_ID)
        return -1;
    len =
        decode_object_id(&apdu[apdu_len], &object_type, &object_instance);
    apdu_len += len;
    if (object_type != OBJECT_DEVICE)
        return -1;
    if (pDevice_id)
        *pDevice_id = object_instance;
    /* MAX APDU - unsigned */
    len =
        decode_tag_number_and_value(&apdu[apdu_len], &tag_number,
        &len_value);
    apdu_len += len;
    if (tag_number != BACNET_APPLICATION_TAG_UNSIGNED_INT)
        return -1;
    len = decode_unsigned(&apdu[apdu_len], len_value, &decoded_value);
    apdu_len += len;
    if (pMax_apdu)
        *pMax_apdu = decoded_value;
    /* Segmentation - enumerated */
    len =
        decode_tag_number_and_value(&apdu[apdu_len], &tag_number,
        &len_value);
    apdu_len += len;
    if (tag_number != BACNET_APPLICATION_TAG_ENUMERATED)
        return -1;
    len = decode_enumerated(&apdu[apdu_len], len_value, &decoded_integer);
    apdu_len += len;
    if (decoded_integer >= MAX_BACNET_SEGMENTATION)
        return -1;
    if (pSegmentation)
        *pSegmentation = decoded_integer;
    /* Vendor ID - unsigned16 */
    len =
        decode_tag_number_and_value(&apdu[apdu_len], &tag_number,
        &len_value);
    apdu_len += len;
    if (tag_number != BACNET_APPLICATION_TAG_UNSIGNED_INT)
        return -1;
    len = decode_unsigned(&apdu[apdu_len], len_value, &decoded_value);
    apdu_len += len;
    if (decoded_value > 0xFFFF)
        return -1;
    if (pVendor_id)
        *pVendor_id = (uint16_t) decoded_value;

    return apdu_len;
}
