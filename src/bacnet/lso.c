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
 *
 * LifeSafetyOperation-Request ::= SEQUENCE {
 *     requesting-process-identifier[0] Unsigned32,
 *     requesting-source[1] CharacterString,
 *     request[2] BACnetLifeSafetyOperation,
 *     object-identifier[3] BACnetObjectIdentifier OPTIONAL
 * }
 *
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
    len = encode_context_character_string(apdu, 1, &data->requestingSrc);
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
        len = encode_context_object_id(
            apdu, 3, data->targetObject.type, data->targetObject.instance);
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

/**
 * @brief Decode the LifeSafetyOperation-Request
 *
 *  * LifeSafetyOperation-Request ::= SEQUENCE {
 *     requesting-process-identifier[0] Unsigned32,
 *     requesting-source[1] CharacterString,
 *     request[2] BACnetLifeSafetyOperation,
 *     object-identifier[3] BACnetObjectIdentifier OPTIONAL
 * }

 * @param apdu  Pointer to the buffer to decode from
 * @param apdu_size number of bytes available in the buffer
 * @param data  Pointer to the data to decode into.
 * @return number of bytes decoded, or BACNET_STATUS_ERROR on error.
 */
int lso_decode_service_request(
    const uint8_t *apdu, unsigned apdu_size, BACNET_LSO_DATA *data)
{
    int len = 0; /* length returned from decoding */
    int apdu_len = 0; /* return value */
    uint32_t operation = 0; /* handles decoded value */
    BACNET_UNSIGNED_INTEGER unsigned_value = 0; /* for decoding*/
    BACNET_CHARACTER_STRING *decoded_string = NULL; /* for decoding */
    BACNET_OBJECT_TYPE object_type = 0;
    uint32_t instance = 0;

    /* check for value pointers */
    if (!apdu && !apdu_size) {
        return 0;
    }
    /* requesting-process-identifier[0] Unsigned32 */
    len = bacnet_unsigned_context_decode(
        &apdu[apdu_len], apdu_size - apdu_len, 0, &unsigned_value);
    if (len <= 0) {
        return BACNET_STATUS_ERROR;
    }
    if (data) {
        data->processId = (uint32_t)unsigned_value;
    }
    apdu_len += len;
    if (data) {
        decoded_string = &data->requestingSrc;
    }
    /* requesting-source[1] CharacterString */
    len = bacnet_character_string_context_decode(
        &apdu[apdu_len], apdu_size - apdu_len, 1, decoded_string);
    if (len <= 0) {
        return BACNET_STATUS_ERROR;
    }
    apdu_len += len;
    /* request[2] BACnetLifeSafetyOperation */
    len = bacnet_enumerated_context_decode(
        &apdu[apdu_len], apdu_size - apdu_len, 2, &operation);
    if (len <= 0) {
        return BACNET_STATUS_ERROR;
    }
    if (operation > LIFE_SAFETY_OP_PROPRIETARY_MAX) {
        return BACNET_STATUS_ERROR;
    }
    if (data) {
        data->operation = (BACNET_LIFE_SAFETY_OPERATION)operation;
    }
    apdu_len += len;
    /* object-identifier[3] BACnetObjectIdentifier OPTIONAL */
    len = bacnet_object_id_context_decode(
        &apdu[apdu_len], apdu_size - apdu_len, 3, &object_type, &instance);
    if (len > 0) {
        if (data) {
            data->targetObject.type = object_type;
            data->targetObject.instance = instance;
            data->use_target = true;
        }
        apdu_len += len;
    } else {
        if (data) {
            data->use_target = false;
            data->targetObject.type = OBJECT_NONE;
            data->targetObject.instance = 0;
        }
    }

    return apdu_len;
}
