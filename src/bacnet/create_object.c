/**
 * @file
 * @brief CreateObject service encode and decode
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
#include "bacnet/create_object.h"

/**
 * @brief Encode one value for CreateObject List-of-Initial-Values
 * @param apdu Pointer to the buffer for encoded values
 * @param offset Offset into the buffer to start encoding
 * @param value Pointer to the property value used for encoding
 * @return Bytes encoded or zero on error.
 */
int create_object_encode_initial_value(
    uint8_t *apdu, int offset, const BACNET_PROPERTY_VALUE *value)
{
    if (apdu) {
        apdu += offset;
    }
    return bacapp_property_value_encode(apdu, value);
}

/**
 * @brief Encode the CreateObject service request
 *
 *  CreateObject-Request ::= SEQUENCE {
 *      object-specifier [0] CHOICE {
 *          object-type [0] BACnetObjectType,
 *          object-identifier [1] BACnetObjectIdentifier
 *      },
 *      list-of-initial-values [1] SEQUENCE OF BACnetPropertyValue OPTIONAL
 *  }
 *
 * @param apdu  Pointer to the buffer for encoded values
 * @param data  Pointer to the service data used for encoding values
 *
 * @return Bytes encoded or zero on error.
 */
int create_object_encode_service_request(
    uint8_t *apdu, BACNET_CREATE_OBJECT_DATA *data)
{
    int len = 0; /* length of each encoding */
    int apdu_len = 0; /* total length of the apdu, return value */

    if (data) {
        /* object-specifier [0] */
        len = encode_opening_tag(apdu, 0);
        apdu_len += len;
        if (apdu) {
            apdu += len;
        }
        if (data->object_instance >= BACNET_MAX_INSTANCE) {
            /* object-type [0] BACnetObjectType */
            len = encode_context_enumerated(apdu, 0, data->object_type);
            apdu_len += len;
            if (apdu) {
                apdu += len;
            }
        } else {
            /* object-identifier [1] BACnetObjectIdentifier */
            len = encode_context_object_id(
                apdu, 1, data->object_type, data->object_instance);
            apdu_len += len;
            if (apdu) {
                apdu += len;
            }
        }
        len = encode_closing_tag(apdu, 0);
        apdu_len += len;
        if (apdu) {
            apdu += len;
        }
        if ((data->application_data_len > 0) &&
            (data->application_data_len <= sizeof(data->application_data))) {
            /* list-of-initial-values [1] OPTIONAL */
            len = encode_opening_tag(apdu, 1);
            apdu_len += len;
            if (apdu) {
                apdu += len;
            }
            len = data->application_data_len;
            if (apdu) {
                memmove(apdu, data->application_data, len);
            }
            apdu_len += len;
            if (apdu) {
                apdu += len;
            }
            len = encode_closing_tag(apdu, 1);
            apdu_len += len;
        }
    }

    return apdu_len;
}

/**
 * @brief Encode the CreateObject service request
 * @param apdu  Pointer to the buffer for encoding into
 * @param apdu_size number of bytes available in the buffer
 * @param data  Pointer to the service data used for encoding values
 * @return number of bytes encoded, or zero if unable to encode or too large
 */
size_t create_object_service_request_encode(
    uint8_t *apdu, size_t apdu_size, BACNET_CREATE_OBJECT_DATA *data)
{
    size_t apdu_len = 0; /* total length of the apdu, return value */

    apdu_len = create_object_encode_service_request(NULL, data);
    if (apdu_len > apdu_size) {
        apdu_len = 0;
    } else {
        apdu_len = create_object_encode_service_request(apdu, data);
    }

    return apdu_len;
}

/**
 * @brief Decode the CreateObject service request
 *
 *  CreateObject-Request ::= SEQUENCE {
 *      object-specifier [0] CHOICE {
 *          object-type [0] BACnetObjectType,
 *          object-identifier [1] BACnetObjectIdentifier
 *      },
 *      list-of-initial-values [1] SEQUENCE OF BACnetPropertyValue OPTIONAL
 *  }
 *
 * @param apdu  Pointer to the buffer for decoding.
 * @param apdu_size  Count of valid bytes in the buffer.
 * @param data  Pointer to the property decoded data to be stored
 *
 * @return Bytes decoded or BACNET_STATUS_REJECT on error.
 */
int create_object_decode_service_request(
    const uint8_t *apdu, uint32_t apdu_size, BACNET_CREATE_OBJECT_DATA *data)
{
    int len = 0;
    int apdu_len = 0;
    BACNET_OBJECT_TYPE object_type = OBJECT_NONE;
    uint32_t object_instance = 0;
    uint32_t enumerated_value = 0;
    int imax = 0, i = 0;

    /* object-specifier [0] CHOICE */
    if (!bacnet_is_opening_tag_number(
            &apdu[apdu_len], apdu_size - apdu_len, 0, &len)) {
        if (data) {
            data->error_code = ERROR_CODE_REJECT_INVALID_TAG;
        }
        return BACNET_STATUS_REJECT;
    }
    apdu_len += len;
    /* CHOICE of Tag [0] or [1] */
    /* object-identifier [1] BACnetObjectIdentifier */
    len = bacnet_object_id_context_decode(
        &apdu[apdu_len], apdu_size - apdu_len, 1, &object_type,
        &object_instance);
    if (len > 0) {
        if ((object_type >= MAX_BACNET_OBJECT_TYPE) ||
            (object_instance >= BACNET_MAX_INSTANCE)) {
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
    } else {
        /* object-type [0] BACnetObjectType */
        len = bacnet_enumerated_context_decode(
            &apdu[apdu_len], apdu_size - apdu_len, 0, &enumerated_value);
        if (len > 0) {
            if (enumerated_value >= MAX_BACNET_OBJECT_TYPE) {
                if (data) {
                    data->error_code = ERROR_CODE_REJECT_PARAMETER_OUT_OF_RANGE;
                }
                return BACNET_STATUS_REJECT;
            }
            if (data) {
                data->object_instance = BACNET_MAX_INSTANCE;
                data->object_type = enumerated_value;
            }
            apdu_len += len;
        } else {
            if (data) {
                data->error_code = ERROR_CODE_REJECT_INVALID_TAG;
            }
            return BACNET_STATUS_REJECT;
        }
    }
    if (!bacnet_is_closing_tag_number(
            &apdu[apdu_len], apdu_size - apdu_len, 0, &len)) {
        if (data) {
            data->error_code = ERROR_CODE_REJECT_INVALID_TAG;
        }
        return BACNET_STATUS_REJECT;
    }
    apdu_len += len;
    /* list-of-initial-values [1] SEQUENCE OF BACnetPropertyValue OPTIONAL */
    if (bacnet_is_opening_tag_number(
            &apdu[apdu_len], apdu_size - apdu_len, 1, &len)) {
        /* determine the length of the data within the tags */
        imax =
            bacnet_enclosed_data_length(&apdu[apdu_len], apdu_size - apdu_len);
        if (imax == BACNET_STATUS_ERROR) {
            if (data) {
                data->error_code = ERROR_CODE_REJECT_INVALID_TAG;
            }
            return BACNET_STATUS_REJECT;
        }
        /* count the opening tag number length after finding enclosed length */
        apdu_len += len;
        if (imax > MAX_APDU) {
            /* not enough size in application_data to store the data chunk */
            if (data) {
                data->error_code = ERROR_CODE_REJECT_BUFFER_OVERFLOW;
            }
            return BACNET_STATUS_REJECT;
        } else if (data) {
            /* copy the data from the APDU */
            for (i = 0; i < imax; i++) {
                data->application_data[i] = apdu[apdu_len + i];
            }
            data->application_data_len = imax;
        }
        /* add on the data length */
        apdu_len += imax;
        if (!bacnet_is_closing_tag_number(
                &apdu[apdu_len], apdu_size - apdu_len, 1, &len)) {
            if (data) {
                data->error_code = ERROR_CODE_REJECT_INVALID_TAG;
            }
            return BACNET_STATUS_REJECT;
        }
        /* count the closing tag number length */
        apdu_len += len;
    }

    return apdu_len;
}

/**
 * @brief Encode a CreateObject-ACK APDU service data
 *
 *  CreateObject-ACK ::= BACnetObjectIdentifier
 *
 * @param apdu  Pointer to the buffer for encoding, or NULL for length
 * @param data  Pointer to the property data to be encoded.
 * @return number of bytes encoded
 */
int create_object_ack_service_encode(
    uint8_t *apdu, const BACNET_CREATE_OBJECT_DATA *data)
{
    /* BACnetObjectIdentifier */
    return encode_application_object_id(
        apdu, data->object_type, data->object_instance);
}

/**
 * @brief Encode a CreateObject-ACK APDU
 *
 *  CreateObject-ACK ::= BACnetObjectIdentifier
 *
 * @param apdu  Pointer to the buffer for encoding, or NULL for length
 * @param invoke_id original invoke id from request
 * @param data  Pointer to the property data to be encoded.
 * @return number of bytes encoded
 */
int create_object_ack_encode(
    uint8_t *apdu, uint8_t invoke_id, const BACNET_CREATE_OBJECT_DATA *data)
{
    int apdu_len = 3; /* total length of the apdu, return value */

    if (apdu) {
        /* service */
        apdu[0] = PDU_TYPE_COMPLEX_ACK;
        /* original invoke id from request */
        apdu[1] = invoke_id;
        /* service choice */
        apdu[2] = SERVICE_CONFIRMED_CREATE_OBJECT;
        apdu += apdu_len;
    }
    apdu_len += create_object_ack_service_encode(apdu, data);

    return apdu_len;
}

/**
 * @brief Decoding for CreateObject-ACK APDU service data
 *  CreateObject-ACK ::= BACnetObjectIdentifier
 *
 * @param apdu  Pointer to the buffer for decoding.
 * @param apdu_size  size of the buffer for decoding.
 * @param data  Pointer to the property data to be encoded.
 * @return Bytes encoded or #BACNET_STATUS_ERROR on error.
 */
int create_object_ack_service_decode(
    const uint8_t *apdu, uint16_t apdu_size, BACNET_CREATE_OBJECT_DATA *data)
{
    int apdu_len = 0;
    BACNET_OBJECT_TYPE object_type = OBJECT_NONE;
    uint32_t object_instance = 0;

    apdu_len = bacnet_object_id_application_decode(
        apdu, apdu_size, &object_type, &object_instance);
    if (apdu_len <= 0) {
        apdu_len = BACNET_STATUS_ERROR;
    } else {
        if (data) {
            data->object_instance = object_instance;
            data->object_type = object_type;
        }
    }

    return apdu_len;
}

/**
 * @brief Encode a CreateObject-Error ACK APDU
 *
 * CreateObject-Error ::= SEQUENCE {
 *      error-type [0] Error,
 *      first-failed-element-number [1] Unsigned
 * }
 *
 * @param apdu  Pointer to the buffer for encoding.
 * @param data  Pointer to the property data to be encoded.
 * @return Bytes encoded or zero on error.
 */
int create_object_error_ack_service_encode(
    uint8_t *apdu, const BACNET_CREATE_OBJECT_DATA *data)
{
    int len = 0; /* length of each encoding */
    int apdu_len = 0; /* total length of the apdu, return value */

    len = encode_opening_tag(apdu, 0);
    apdu_len += len;
    if (apdu) {
        apdu += len;
    }
    len = encode_application_enumerated(apdu, data->error_class);
    apdu_len += len;
    if (apdu) {
        apdu += len;
    }
    len = encode_application_enumerated(apdu, data->error_code);
    apdu_len += len;
    if (apdu) {
        apdu += len;
    }
    len = encode_closing_tag(apdu, 0);
    apdu_len += len;
    if (apdu) {
        apdu += len;
    }
    len = encode_context_unsigned(apdu, 1, data->first_failed_element_number);
    apdu_len += len;

    return apdu_len;
}

/**
 * @brief Encode a CreateObject Error acknowledge in the APDU.
 * @param apdu [in] The APDU buffer.
 * @param invoke_id [in] Invoked service ID.
 * @param data [in] Data of the invoked property.
 * @return number of bytes encoded
 */
int create_object_error_ack_encode(
    uint8_t *apdu, uint8_t invoke_id, const BACNET_CREATE_OBJECT_DATA *data)
{
    int len = 3;

    if (apdu) {
        apdu[0] = PDU_TYPE_ERROR;
        apdu[1] = invoke_id;
        apdu[2] = SERVICE_CONFIRMED_CREATE_OBJECT;
        apdu += len;
    }
    len += create_object_error_ack_service_encode(apdu, data);

    return len;
}

/**
 * @brief Decode a CreateObject-Error ACK APDU
 *
 * CreateObject-Error ::= SEQUENCE {
 *      error-type [0] Error,
 *      first-failed-element-number [1] Unsigned
 * }
 *
 * @param apdu  Pointer to the buffer for decoding.
 * @param apdu_size  size of the buffer for decoding.
 * @param data  Pointer to the property data to be encoded.
 * @return Bytes encoded or BACNET_STATUS_REJECT on error.
 */
int create_object_error_ack_service_decode(
    const uint8_t *apdu, uint16_t apdu_size, BACNET_CREATE_OBJECT_DATA *data)
{
    int len = 0, apdu_len = 0;
    BACNET_ERROR_CLASS error_class = ERROR_CLASS_SERVICES;
    BACNET_ERROR_CODE error_code = ERROR_CODE_SUCCESS;
    BACNET_UNSIGNED_INTEGER first_failed_element_number = 0;

    if (!apdu) {
        return BACNET_STATUS_REJECT;
    }
    if (data) {
        data->first_failed_element_number = 0;
        data->error_class = ERROR_CLASS_SERVICES;
        data->error_code = ERROR_CODE_REJECT_PARAMETER_OUT_OF_RANGE;
    }
    /* Opening Context tag 0 - Error */
    if (bacnet_is_opening_tag_number(apdu, apdu_size, 0, &len)) {
        apdu_len += len;
        apdu += len;
    } else {
        return BACNET_STATUS_REJECT;
    }
    len = bacerror_decode_error_class_and_code(
        apdu, apdu_size - apdu_len, &error_class, &error_code);
    if (len > 0) {
        if (data) {
            data->error_class = error_class;
            data->error_code = error_code;
        }
        apdu_len += len;
        apdu += len;
    } else {
        return BACNET_STATUS_REJECT;
    }
    /* Closing Context tag 0 - Error */
    if (bacnet_is_closing_tag_number(apdu, apdu_size - apdu_len, 0, &len)) {
        apdu_len += len;
        apdu += len;
    } else {
        return BACNET_STATUS_REJECT;
    }
    len = bacnet_unsigned_context_decode(
        apdu, apdu_size - apdu_len, 1, &first_failed_element_number);
    if (len > 0) {
        if (data) {
            data->first_failed_element_number = first_failed_element_number;
        }
        apdu_len += len;
    } else {
        return BACNET_STATUS_REJECT;
    }

    return apdu_len;
}
