/**
 * @file
 * @brief CreateObject and DeleteObject service encode and decode
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date August 2023
 * @section LICENSE
 *
 * Copyright (C) 2023 Steve Karg <skarg@users.sourceforge.net>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later WITH GCC-exception-2.0
 */
#include <stdint.h>
#include <stdbool.h>
#include "bacnet/bacapp.h"
#include "bacnet/bacenum.h"
#include "bacnet/bacdcode.h"
#include "bacnet/bacdef.h"
#include "bacnet/bacerror.h"
#include "bacnet/create_object.h"

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
    BACNET_PROPERTY_VALUE *value = NULL; /* value in list */

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
        if (data->list_of_initial_values) {
            /* list-of-initial-values [1] OPTIONAL */
            len = encode_opening_tag(apdu, 1);
            apdu_len += len;
            if (apdu) {
                apdu += len;
            }
            /* the first value includes a pointer to the next value, etc */
            value = data->list_of_initial_values;
            while (value != NULL) {
                /* SEQUENCE OF BACnetPropertyValue */
                len = bacapp_property_value_encode(apdu, value);
                apdu_len += len;
                if (apdu) {
                    apdu += len;
                }
                /* is there another one to encode? */
                /* FIXME: check to see if there is room in the APDU */
                value = value->next;
            }
            len = encode_closing_tag(apdu, 1);
            apdu_len += len;
        }
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
 * @param apdu_len  Count of valid bytes in the buffer.
 * @param data  Pointer to the property decoded data to be stored
 *
 * @return Bytes decoded or BACNET_STATUS_REJECT on error.
 */
int create_object_decode_service_request(
    uint8_t *apdu, uint32_t apdu_size, BACNET_CREATE_OBJECT_DATA *data)
{
    int len = 0;
    int apdu_len = BACNET_STATUS_REJECT;
    BACNET_OBJECT_TYPE object_type = OBJECT_NONE;
    uint32_t object_instance = 0;
    uint32_t enumerated_value = 0;

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
    len = bacnet_object_id_context_decode(&apdu[apdu_len], apdu_size - apdu_len,
        1, &object_type, &object_instance);
    if (len != BACNET_STATUS_ERROR) {
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
        if (len != BACNET_STATUS_ERROR) {
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
            &apdu[apdu_len], apdu_size - apdu_len, 0, &len)) {
        apdu_len += len;
        len = bacapp_property_value_decode(&apdu[apdu_len],
            apdu_size - apdu_len, data->list_of_initial_values);
        if (len == BACNET_STATUS_ERROR) {
            if (data) {
                data->error_code = ERROR_CODE_REJECT_INVALID_TAG;
            }
            return BACNET_STATUS_REJECT;
        }
        apdu_len += len;
        if (!bacnet_is_closing_tag_number(
                &apdu[apdu_len], apdu_size - apdu_len, 0, &len)) {
            if (data) {
                data->error_code = ERROR_CODE_REJECT_INVALID_TAG;
            }
            return BACNET_STATUS_REJECT;
        }
        apdu_len += len;
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
int create_object_error_ack_encode(
    uint8_t *apdu, BACNET_CREATE_OBJECT_DATA *data)
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
 * @brief Decoding for AddListElement or RemoveListElement Error Ack
 *  AddListElement-Error ::= SEQUENCE {
 *      error-type [0] Error,
 *      first-failed-element-number [1] UNSIGNED
 *  }
 *  RemoveListElement-Error ::= SEQUENCE {
 *      error-type [0] Error,
 *      first-failed-element-number [1] UNSIGNED
 *  }
 *
 * @param apdu  Pointer to the buffer for decoding.
 * @param apdu_size  size of the buffer for decoding.
 * @param data  Pointer to the property data to be encoded.
 * @return Bytes encoded or zero on error.
 */
int create_object_error_ack_decode(
    uint8_t *apdu, uint16_t apdu_size, BACNET_CREATE_OBJECT_DATA *data)
{
    int len = 0, apdu_len = 0;
    BACNET_ERROR_CLASS error_class = ERROR_CLASS_SERVICES;
    BACNET_ERROR_CODE error_code = ERROR_CODE_SUCCESS;
    BACNET_UNSIGNED_INTEGER first_failed_element_number = 0;

    if (!apdu) {
        return 0;
    }
    if (data) {
        data->first_failed_element_number = 0;
        data->error_class = ERROR_CLASS_SERVICES;
        data->error_code = ERROR_CODE_REJECT_PARAMETER_OUT_OF_RANGE;
    }
    if (apdu_size < apdu_len) {
        return 0;
    }
    /* Opening Context tag 0 - Error */
    if (decode_is_opening_tag_number(apdu, 0)) {
        /* opening tag 0 is 1 byte */
        len = 1;
        apdu_len += len;
        apdu += len;
    } else {
        return 0;
    }
    if (apdu_size < apdu_len) {
        return 0;
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
        return 0;
    }
    if (apdu_size < apdu_len) {
        return 0;
    }
    /* Closing Context tag 0 - Error */
    if (decode_is_closing_tag_number(apdu, 0)) {
        /* closing tag 0 is 1 byte */
        len = 1;
        apdu_len += len;
        apdu += len;
    } else {
        return 0;
    }
    if (apdu_size < apdu_len) {
        return 0;
    }
    len = bacnet_unsigned_context_decode(
        apdu, apdu_size - apdu_len, 1, &first_failed_element_number);
    if (len > 0) {
        if (data) {
            data->first_failed_element_number = first_failed_element_number;
        }
        apdu_len += len;
    } else {
        return 0;
    }

    return apdu_len;
}
