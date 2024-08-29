/**
 * @file
 * @brief API for BACnetLifeSafetyOperation encoder and decoder
 * @author John Minack <minack@users.sourceforge.net>
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date 2008
 * @copyright SPDX-License-Identifier: GPL-2.0-or-later WITH GCC-exception-2.0
 */
#include <stdint.h>
#include "bacnet/lso.h"
#include "bacnet/bacdcode.h"
#include "bacnet/apdu.h"

/**
 * @brief Encode APDU for LifeSafetyOperation-Request
 * @param apdu  Pointer to the buffer, or NULL for length
 * @param data  Pointer to the data to encode.
 * @return number of bytes encoded, or zero on error.
 */
int life_safety_operation_encode(uint8_t *apdu, const BACNET_LSO_DATA *data)
{
    int len = 0; /* length of each encoding */
    int apdu_len = 0; /* total length of the apdu, return value */

    if (!data) {
        return 0;
    }
    /* tag 0 - requestingProcessId */
    len = encode_context_unsigned(apdu, 0, data->processId);
    apdu_len += len;
    if (apdu) {
        apdu += len;
    }
    /* tag 1 - requestingSource */
    len = encode_context_character_string(
        apdu, 1, &data->requestingSrc);
    apdu_len += len;
    if (apdu) {
        apdu += len;
    }
    /* Operation */
    len = encode_context_enumerated(apdu, 2, data->operation);
    apdu_len += len;
    if (apdu) {
        apdu += len;
    }
    /* Object ID */
    if (data->use_target) {
        len = encode_context_object_id(apdu, 3,
            data->targetObject.type, data->targetObject.instance);
        apdu_len += len;
    }

    return apdu_len;
}

/**
 * @brief Encode APDU for LifeSafetyOperation-Request
 * @param apdu  Pointer to the buffer, or NULL for length
 * @param data  Pointer to the data to encode.
 * @return number of bytes encoded, or zero on error.
 */
int lso_encode_apdu(
    uint8_t *apdu, uint8_t invoke_id, const BACNET_LSO_DATA *data)
{
    int len = 0; /* length of each encoding */
    int apdu_len = 0; /* total length of the apdu, return value */

    if (apdu) {
        apdu[0] = PDU_TYPE_CONFIRMED_SERVICE_REQUEST;
        apdu[1] = encode_max_segs_max_apdu(0, MAX_APDU);
        apdu[2] = invoke_id;
        apdu[3] = SERVICE_CONFIRMED_LIFE_SAFETY_OPERATION;
    }
    len = 4;
    apdu_len += len;
    if (apdu) {
        apdu += len;
    }
    len = life_safety_operation_encode(apdu, data);
    if (len > 0) {
        apdu_len += len;
    } else {
        apdu_len = len;
    }

    return apdu_len;
}

/**
 * @brief Encode the LifeSafetyOperation-Request
 * @param apdu  Pointer to the buffer for encoding into
 * @param apdu_size number of bytes available in the buffer
 * @param data  Pointer to the service data used for encoding values
 * @return number of bytes encoded, or zero if unable to encode or too large
 */
size_t life_safety_operation_request_encode(
    uint8_t *apdu, size_t apdu_size, const BACNET_LSO_DATA *data)
{
    size_t apdu_len = 0;

    apdu_len = life_safety_operation_encode(NULL, data);
    if (apdu_len > apdu_size) {
        apdu_len = 0;
    } else {
        apdu_len = life_safety_operation_encode(apdu, data);
    }

    return apdu_len;
}

int lso_decode_service_request(
    const uint8_t *apdu, unsigned apdu_len, BACNET_LSO_DATA *data)
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
