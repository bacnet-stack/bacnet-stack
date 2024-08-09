/**
 * @file
 * @brief API for I-Have service encode and decode
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date 2006
 * @copyright SPDX-License-Identifier: GPL-2.0-or-later WITH GCC-exception-2.0
 */
#include <stdint.h>
/* BACnet Stack defines - first */
#include "bacnet/bacdef.h"
/* BACnet Stack API */
#include "bacnet/bacdcode.h"
#include "bacnet/ihave.h"

/**
 * Encode the I Have request
 *
 * @param apdu  Pointer to the APDU buffer
 * @param data  Pointer to the I Have data structure.
 *
 * @return Bytes encoded.
 */
int ihave_encode_apdu(uint8_t *apdu, const BACNET_I_HAVE_DATA *data)
{
    int len = 0; /* length of each encoding */
    int apdu_len = 0; /* total length of the apdu, return value */

    if (apdu && data) {
        apdu[0] = PDU_TYPE_UNCONFIRMED_SERVICE_REQUEST;
        apdu[1] = SERVICE_UNCONFIRMED_I_HAVE;
        apdu_len = 2;
        /* deviceIdentifier */
        len = encode_application_object_id(
            &apdu[apdu_len], data->device_id.type, data->device_id.instance);
        apdu_len += len;
        /* objectIdentifier */
        len = encode_application_object_id(
            &apdu[apdu_len], data->object_id.type, data->object_id.instance);
        apdu_len += len;
        /* objectName */
        len = encode_application_character_string(
            &apdu[apdu_len], &data->object_name);
        apdu_len += len;
    }

    return apdu_len;
}

#if BACNET_SVC_I_HAVE_A

/**
 * Decode the I Have request only
 *
 * @param apdu  Pointer to the APDU buffer
 * @param apdu_len  Valid bytes in the buffer
 * @param data  Pointer to the I Have data structure.
 *
 * @return Bytes decoded.
 */
int ihave_decode_service_request(
    const uint8_t *apdu, unsigned apdu_len, BACNET_I_HAVE_DATA *data)
{
    int len = 0;
    uint8_t tag_number = 0;
    uint32_t len_value = 0;
    BACNET_OBJECT_TYPE decoded_type = OBJECT_NONE; /* for decoding */

    if ((apdu_len >= 2) && data) {
        /* deviceIdentifier */
        len += decode_tag_number_and_value(&apdu[len], &tag_number, &len_value);
        if (tag_number == BACNET_APPLICATION_TAG_OBJECT_ID) {
            len += decode_object_id(
                &apdu[len], &decoded_type, &data->device_id.instance);
            data->device_id.type = decoded_type;
        } else {
            return -1;
        }
        /* objectIdentifier */
        if ((unsigned)len >= apdu_len) {
            return -1;
        }
        len += decode_tag_number_and_value(&apdu[len], &tag_number, &len_value);
        if (tag_number == BACNET_APPLICATION_TAG_OBJECT_ID) {
            len += decode_object_id(
                &apdu[len], &decoded_type, &data->object_id.instance);
            data->object_id.type = decoded_type;
        } else {
            return -1;
        }
        /* objectName */
        if ((unsigned)len >= apdu_len) {
            return -1;
        }
        len += decode_tag_number_and_value(&apdu[len], &tag_number, &len_value);
        if (tag_number == BACNET_APPLICATION_TAG_CHARACTER_STRING) {
            len += decode_character_string(
                &apdu[len], len_value, &data->object_name);
        } else {
            return -1;
        }
    } else {
        return -1;
    }

    return len;
}

/**
 * Decode the I Have
 *
 * @param apdu  Pointer to the APDU buffer
 * @param apdu_len  Valid bytes in the buffer
 * @param data  Pointer to the I Have data structure.
 *
 * @return Bytes decoded.
 */
int ihave_decode_apdu(
    const uint8_t *apdu, unsigned apdu_len, BACNET_I_HAVE_DATA *data)
{
    int len = 0;

    if ((!apdu) || (apdu_len < 2)) {
        return -1;
    }
    /* optional checking - most likely was already done prior to this call */
    if (apdu[0] != PDU_TYPE_UNCONFIRMED_SERVICE_REQUEST) {
        return -1;
    }
    if (apdu[1] != SERVICE_UNCONFIRMED_I_HAVE) {
        return -1;
    }
    len = ihave_decode_service_request(&apdu[2], apdu_len - 2, data);

    return len;
}
#endif
