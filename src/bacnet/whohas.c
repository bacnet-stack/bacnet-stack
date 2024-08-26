/**
 * @file
 * @brief BACnet WhoHas-Request encode and decode
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date 2006
 * @copyright SPDX-License-Identifier: GPL-2.0-or-later WITH GCC-exception-2.0
 */
#include <stdint.h>
/* BACnet Stack defines - first */
#include "bacnet/bacdef.h"
/* BACnet Stack API */
#include "bacnet/bacdcode.h"
#include "bacnet/whohas.h"

/**
 * @brief Encode Who-Has-Request  - use -1 as limit for unlimited
 *  Who-Has-Request ::= SEQUENCE {
 *      limits SEQUENCE {
 *          device-instance-range-low-limit [0] Unsigned (0..4194303),
 *          device-instance-range-high-limit [1] Unsigned (0..4194303)
 *      } OPTIONAL,
 *      object CHOICE {
 *          object-identifier [2] BACnetObjectIdentifier,
 *          object-name [3] CharacterString
 *      }
 *  }
 *
 * @param apdu application data unit buffer
 * @param data application data to be encoded
 * @return number of bytes encoded
 */
int bacnet_who_has_request_encode(uint8_t *apdu, BACNET_WHO_HAS_DATA *data)
{
    int len = 0; /* length of each encoding */
    int apdu_len = 0; /* total length of the apdu, return value */

    if (!data) {
        return 0;
    }

    /* optional limits - must be used as a pair */
    if ((data->low_limit >= 0) && (data->low_limit <= BACNET_MAX_INSTANCE) &&
        (data->high_limit >= 0) && (data->high_limit <= BACNET_MAX_INSTANCE)) {
        len = encode_context_unsigned(apdu, 0, data->low_limit);
        apdu_len += len;
        if (apdu) {
            apdu += len;
        }
        len = encode_context_unsigned(apdu, 1, data->high_limit);
        apdu_len += len;
        if (apdu) {
            apdu += len;
        }
    }
    if (data->is_object_name) {
        len = encode_context_character_string(apdu, 3, &data->object.name);
        apdu_len += len;
    } else {
        len = encode_context_object_id(
            apdu, 2, data->object.identifier.type,
            data->object.identifier.instance);
        apdu_len += len;
    }

    return apdu_len;
}

/**
 * @brief Encode Who-Has-Request service
 * @param apdu application data unit buffer
 * @param apdu_size number of bytes available in the buffer
 * @param data application data to be encoded
 * @return number of bytes encoded, or zero if unable to encode or too large
 */
size_t bacnet_who_has_service_request_encode(
    uint8_t *apdu, size_t apdu_size, BACNET_WHO_HAS_DATA *data)
{
    size_t apdu_len = 0; /* total length of the apdu, return value */

    apdu_len = bacnet_who_has_request_encode(NULL, data);
    if (apdu_len > apdu_size) {
        apdu_len = 0;
    } else {
        apdu_len = bacnet_who_has_request_encode(apdu, data);
    }

    return apdu_len;
}

/**
 * @brief Encode Who-Has service  - use -1 for limit for unlimited
 * @param apdu  Pointer to the APDU.
 * @param data  Pointer to the Who-Has application data.
 * @return Bytes encoded.
 */
int whohas_encode_apdu(uint8_t *apdu, BACNET_WHO_HAS_DATA *data)
{
    int len = 0; /* length of each encoding */
    int apdu_len = 0; /* total length of the apdu, return value */

    if (apdu) {
        apdu[0] = PDU_TYPE_UNCONFIRMED_SERVICE_REQUEST;
        apdu[1] = SERVICE_UNCONFIRMED_WHO_HAS; /* service choice */
    }
    len = 2;
    apdu_len += len;
    if (apdu) {
        apdu += len;
    }
    len = bacnet_who_has_request_encode(apdu, data);
    if (len > 0) {
        apdu_len += len;
    } else {
        apdu_len = 0;
    }

    return apdu_len;
}

/**
 * @brief Decode the Who-Has service request only.
 * @param apdu application protocol data unit buffer
 * @param apdu_size number of valid bytes in the receive buffer.
 * @param data application data to be decoded
 * @return number of bytes decoded, or BACNET_STATUS_ERROR on error
 */
int whohas_decode_service_request(
    uint8_t *apdu, unsigned apdu_size, BACNET_WHO_HAS_DATA *data)
{
    int apdu_len = 0; /* total length of the apdu, return value */
    int len = 0;
    BACNET_UNSIGNED_INTEGER unsigned_value = 0;
    BACNET_OBJECT_TYPE decoded_type = OBJECT_NONE;
    uint32_t decoded_instance = 0;
    BACNET_CHARACTER_STRING *char_string = NULL;

    if (apdu_size == 0) {
        /* message too short */
        return BACNET_STATUS_ERROR;
    }
    /* If the 'Device Instance Range Low Limit' parameter is present, then
       the 'Device Instance Range High Limit' parameter shall also be present*/
    /* device-instance-range-low-limit [0] Unsigned OPTIONAL */
    len = bacnet_unsigned_context_decode(
        &apdu[apdu_len], apdu_size - apdu_len, 0, &unsigned_value);
    if (len > 0) {
        apdu_len += len;
        /* optional limits - must be used as a pair */
        if (unsigned_value <= BACNET_MAX_INSTANCE) {
            /* (0..4194303) */
            if (data) {
                data->low_limit = unsigned_value;
            }
        } else {
            /* parameter out of range */
            return BACNET_STATUS_ERROR;
        }
        /* device-instance-range-high-limit [1] Unsigned OPTIONAL */
        len = bacnet_unsigned_context_decode(
            &apdu[apdu_len], apdu_size - apdu_len, 1, &unsigned_value);
        if (len > 0) {
            apdu_len += len;
            if (unsigned_value <= BACNET_MAX_INSTANCE) {
                /* (0..4194303) */
                if (data) {
                    data->high_limit = unsigned_value;
                }
            } else {
                /* parameter out of range */
                return BACNET_STATUS_ERROR;
            }
        } else {
            /* missing required parameters */
            return BACNET_STATUS_ERROR;
        }
    } else if (len == 0) {
        /* If the 'Device Instance Range Low Limit' and
           'Device Instance Range High Limit' parameters are omitted,
           then all devices that receive this message are
           qualified to respond with an I-Have service request. */
        /* use -1 as limit for unlimited */
        if (data) {
            data->low_limit = -1;
            data->high_limit = -1;
        }
    } else {
        /* malformed */
        return BACNET_STATUS_ERROR;
    }
    /* object-identifier [2] BACnetObjectIdentifier, CHOICE */
    len = bacnet_object_id_context_decode(
        &apdu[apdu_len], apdu_size - apdu_len, 2, &decoded_type,
        &decoded_instance);
    if (len > 0) {
        apdu_len += len;
        if (data) {
            data->is_object_name = false;
            data->object.identifier.type = decoded_type;
            data->object.identifier.instance = decoded_instance;
        }
    } else if (len == 0) {
        /* object-name [3] CharacterString, CHOICE */
        if (data) {
            char_string = &data->object.name;
        }
        len = bacnet_character_string_context_decode(
            &apdu[apdu_len], apdu_size - apdu_len, 3, char_string);
        if (len > 0) {
            apdu_len += len;
            if (data) {
                data->is_object_name = true;
            }
        } else {
            /* malformed */
            return BACNET_STATUS_ERROR;
        }
    } else {
        /* missing required parameters */
        return BACNET_STATUS_ERROR;
    }

    return apdu_len;
}
