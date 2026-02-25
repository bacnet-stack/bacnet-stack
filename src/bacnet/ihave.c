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

    if (apdu) {
        apdu[0] = PDU_TYPE_UNCONFIRMED_SERVICE_REQUEST;
        apdu[1] = SERVICE_UNCONFIRMED_I_HAVE;
    }
    len = 2;
    apdu_len += len;
    if (apdu) {
        apdu += len;
    }
    if (data) {
        /* deviceIdentifier */
        len = encode_application_object_id(
            apdu, data->device_id.type, data->device_id.instance);
        apdu_len += len;
        if (apdu) {
            apdu += len;
        }
        /* objectIdentifier */
        len = encode_application_object_id(
            apdu, data->object_id.type, data->object_id.instance);
        apdu_len += len;
        if (apdu) {
            apdu += len;
        }
        /* objectName */
        len = encode_application_character_string(apdu, &data->object_name);
        apdu_len += len;
    }

    return apdu_len;
}

#if BACNET_SVC_I_HAVE_A

/**
 * Decode the I Have request only
 *
 * I-Have-Request ::= SEQUENCE {
 *   device-identifierBACnetObjectIdentifier,
 *   object-identifierBACnetObjectIdentifier,
 *   object-nameCharacterString
 * }
 *
 * @param apdu  Pointer to the APDU buffer
 * @param apdu_size Number of valid bytes in the buffer
 * @param data  Pointer to the I Have data structure.
 *
 * @return Bytes decoded.
 */
int ihave_decode_service_request(
    const uint8_t *apdu, unsigned apdu_size, BACNET_I_HAVE_DATA *data)
{
    int len = 0, apdu_len = 0;
    BACNET_OBJECT_TYPE decoded_type = OBJECT_NONE; /* for decoding */
    uint32_t decoded_instance = 0; /* for decoding */
    BACNET_CHARACTER_STRING *decoded_string = NULL; /* for decoding */

    if (!apdu || (apdu_size < 2)) {
        return -1;
    }
    /* deviceIdentifier */
    len = bacnet_object_id_application_decode(
        &apdu[apdu_len], apdu_size - apdu_len, &decoded_type,
        &decoded_instance);
    if (len <= 0) {
        return BACNET_STATUS_ERROR;
    } else {
        if (data) {
            data->device_id.type = decoded_type;
            data->device_id.instance = decoded_instance;
        }
    }
    apdu_len += len;
    /* objectIdentifier */
    len = bacnet_object_id_application_decode(
        &apdu[apdu_len], apdu_size - apdu_len, &decoded_type,
        &decoded_instance);
    if (len <= 0) {
        return BACNET_STATUS_ERROR;
    } else {
        if (data) {
            data->object_id.type = decoded_type;
            data->object_id.instance = decoded_instance;
        }
    }
    apdu_len += len;
    /* objectName */
    if (data) {
        decoded_string = &data->object_name;
    }
    len = bacnet_character_string_application_decode(
        &apdu[apdu_len], apdu_size - apdu_len, decoded_string);
    if (len <= 0) {
        return BACNET_STATUS_ERROR;
    } else {
        /* nothing else to do, the string is already decoded in place */
    }
    apdu_len += len;

    return apdu_len;
}

/**
 * Decode the I Have
 *
 * @param apdu  Pointer to the APDU buffer
 * @param apdu_size Number of valid bytes in the buffer
 * @param data  Pointer to the I Have data structure.
 *
 * @return Bytes decoded.
 */
int ihave_decode_apdu(
    const uint8_t *apdu, unsigned apdu_size, BACNET_I_HAVE_DATA *data)
{
    int len = 0;

    if ((!apdu) || (apdu_size < 2)) {
        return BACNET_STATUS_ERROR;
    }
    /* optional checking - most likely was already done prior to this call */
    if (apdu[0] != PDU_TYPE_UNCONFIRMED_SERVICE_REQUEST) {
        return BACNET_STATUS_ERROR;
    }
    if (apdu[1] != SERVICE_UNCONFIRMED_I_HAVE) {
        return BACNET_STATUS_ERROR;
    }
    len = ihave_decode_service_request(&apdu[2], apdu_size - 2, data);

    return len;
}
#endif
