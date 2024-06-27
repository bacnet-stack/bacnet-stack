/**
 * @file
 * @brief DeleteObject service encode and decode
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date August 2023
 * @copyright SPDX-License-Identifier: GPL-2.0-or-later WITH GCC-exception-2.0
 */
#include <stdint.h>
#include <stdbool.h>
/* BACnet Stack defines - first */
#include "bacnet/bacdef.h"
/* BACnet Stack API */
#include "bacnet/bacapp.h"
#include "bacnet/bacdcode.h"
#include "bacnet/bacerror.h"
#include "bacnet/delete_object.h"

/**
 * @brief Encode the DeleteObject service request
 *
 *  DeleteObject-Request ::= SEQUENCE {
 *      object-identifier BACnetObjectIdentifier
 *  }
 *
 * @param apdu  Pointer to the buffer for encoded values, or NULL for length
 * @param data  Pointer to the service data used for encoding values
 *
 * @return Bytes encoded or zero on error.
 */
int delete_object_encode_service_request(
    uint8_t *apdu, BACNET_DELETE_OBJECT_DATA *data)
{
    int len = 0; /* length of each encoding */
    int apdu_len = 0; /* total length of the apdu, return value */

    if (data) {
        /* object-identifier BACnetObjectIdentifier */
        len = encode_application_object_id(
            apdu, data->object_type, data->object_instance);
        apdu_len += len;
    }

    return apdu_len;
}

/**
 * @brief Encode the DeleteObject service request
 *
 *  DeleteObject-Request ::= SEQUENCE {
 *      object-identifier BACnetObjectIdentifier
 *  }
 *
 * @param apdu  Pointer to the buffer for encoded values
 * @param apdu_size number of bytes available in the buffer
 * @param data  Pointer to the service data used for encoding values
 * @return number of bytes encoded, or zero if unable to encode or too large
 */
int delete_object_service_request_encode(
    uint8_t *apdu, size_t apdu_size, BACNET_DELETE_OBJECT_DATA *data)
{
    size_t apdu_len = 0; /* total length of the apdu, return value */

    apdu_len = delete_object_encode_service_request(NULL, data);
    if (apdu_len > apdu_size) {
        apdu_len = 0;
    } else {
        apdu_len = delete_object_encode_service_request(apdu, data);
    }

    return apdu_len;
}

/**
 * @brief Decode the DeleteObject service request
 *
 *  DeleteObject-Request ::= SEQUENCE {
 *      object-identifier BACnetObjectIdentifier
 *  }
 *
 * @param apdu  Pointer to the buffer for decoding.
 * @param apdu_size  Count of valid bytes in the buffer.
 * @param data  Pointer to the property decoded data to be stored
 *
 * @return Bytes decoded or BACNET_STATUS_REJECT on error.
 */
int delete_object_decode_service_request(
    uint8_t *apdu, uint32_t apdu_size, BACNET_DELETE_OBJECT_DATA *data)
{
    int len = 0;
    int apdu_len = 0;
    BACNET_OBJECT_TYPE object_type = OBJECT_NONE;
    uint32_t object_instance = 0;

    /* object-identifier BACnetObjectIdentifier */
    len = bacnet_object_id_application_decode(
        &apdu[apdu_len], apdu_size - apdu_len, &object_type, &object_instance);
    if (len == BACNET_STATUS_ERROR) {
        if (data) {
            data->error_code = ERROR_CODE_REJECT_MISSING_REQUIRED_PARAMETER;
        }
        return BACNET_STATUS_REJECT;
    } else if (len == 0) {
        if (data) {
            data->error_code = ERROR_CODE_REJECT_INVALID_TAG;
        }
        return BACNET_STATUS_REJECT;
    } else {
        if ((object_type >= MAX_BACNET_OBJECT_TYPE) ||
            (object_instance > BACNET_MAX_INSTANCE)) {
            if (data) {
                data->error_code = ERROR_CODE_REJECT_PARAMETER_OUT_OF_RANGE;
            }
            return BACNET_STATUS_REJECT;
        }
        if (data) {
            data->object_instance = object_instance;
            data->object_type = object_type;
        }
        apdu_len += len;
    }

    return apdu_len;
}
