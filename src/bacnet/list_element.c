/**
 * @file
 * @brief AddListElement and RemoveListElement service encode and decode
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date December 2022
 * @copyright SPDX-License-Identifier: GPL-2.0-or-later WITH GCC-exception-2.0
 */
#include <stdint.h>
#include <stdbool.h>
/* BACnet Stack defines - first */
#include "bacnet/bacdef.h"
/* BACnet Stack API */
#include "bacnet/bacdcode.h"
#include "bacnet/bacerror.h"
#include "bacnet/list_element.h"

/**
 * @brief Encode the Add/Remove ListElement service request APDU
 *
 *  AddListElement-Request ::= SEQUENCE {
 *      object-identifier       [0] BACnetObjectIdentifier,
 *      property-identifier     [1] BACnetPropertyIdentifier,
 *      property-array-index    [2] Unsigned OPTIONAL,
 *      -- used only with array datatype
 *      list-of-elements        [3] ABSTRACT-SYNTAX.&Type
 *  }
 *  RemoveListElement-Request ::= SEQUENCE {
 *      object-identifier       [0] BACnetObjectIdentifier,
 *      property-identifier     [1] BACnetPropertyIdentifier,
 *      property-array-index    [2] Unsigned OPTIONAL,
 *      -- used only with array datatype
 *      list-of-elements        [3] ABSTRACT-SYNTAX.&Type
 *  }
 *
 * @param apdu  Pointer to the buffer for encoding.
 * @param list_element  Pointer to the property data to be encoded.
 *
 * @return Bytes encoded or zero on error.
 */
int list_element_encode_service_request(
    uint8_t *apdu, const BACNET_LIST_ELEMENT_DATA *list_element)
{
    int len = 0; /* length of each encoding */
    int apdu_len = 0; /* total length of the apdu, return value */

    if (list_element) {
        len = encode_context_object_id(
            apdu, 0, list_element->object_type, list_element->object_instance);
        apdu_len += len;
        if (apdu) {
            apdu += len;
        }
        len = encode_context_enumerated(apdu, 1, list_element->object_property);
        apdu_len += len;
        if (apdu) {
            apdu += len;
        }
        if (list_element->array_index != BACNET_ARRAY_ALL) {
            /* optional array index */
            len = encode_context_unsigned(apdu, 2, list_element->array_index);
            apdu_len += len;
            if (apdu) {
                apdu += len;
            }
        }
        len = encode_opening_tag(apdu, 3);
        apdu_len += len;
        if (apdu) {
            apdu += len;
        }
        for (len = 0; len < list_element->application_data_len; len++) {
            if (apdu) {
                *apdu = list_element->application_data[len];
                apdu++;
            }
        }
        apdu_len += len;
        len = encode_closing_tag(apdu, 3);
        apdu_len += len;
    }

    return apdu_len;
}

/**
 * @brief Encode the Add/Remove ListElement service request only
 * @param apdu  Pointer to the buffer for encoding into
 * @param apdu_size number of bytes available in the buffer
 * @param data  Pointer to the service data used for encoding values
 * @return number of bytes encoded, or zero if unable to encode or too large
 */
size_t list_element_service_request_encode(
    uint8_t *apdu, size_t apdu_size, const BACNET_LIST_ELEMENT_DATA *data)
{
    size_t apdu_len = 0; /* total length of the apdu, return value */

    apdu_len = list_element_encode_service_request(NULL, data);
    if (apdu_len > apdu_size) {
        apdu_len = 0;
    } else {
        apdu_len = list_element_encode_service_request(apdu, data);
    }

    return apdu_len;
}

/**
 * @brief Decode the Add/Remove ListElement service request only
 * @param apdu  Pointer to the buffer for decoding.
 * @param apdu_size  Count of valid bytes in the buffer.
 * @param list_element  Pointer to the property decoded data to be stored
 *
 * @return Bytes decoded or BACNET_STATUS_REJECT on error.
 */
int list_element_decode_service_request(
    uint8_t *apdu, unsigned apdu_size, BACNET_LIST_ELEMENT_DATA *list_element)
{
    int len = 0, application_data_len = 0, apdu_len = 0;
    BACNET_OBJECT_TYPE object_type = OBJECT_NONE;
    uint32_t object_instance = 0;
    uint32_t property = 0;
    BACNET_UNSIGNED_INTEGER unsigned_value = 0;

    /* Must have at least 2 tags, an object id and a property identifier
     * of at least 1 byte in length to have any chance of parsing */
    if (apdu_size < 7) {
        if (list_element) {
            list_element->error_code =
                ERROR_CODE_REJECT_MISSING_REQUIRED_PARAMETER;
        }
        return BACNET_STATUS_REJECT;
    }

    /* Tag 0: Object ID          */
    len = bacnet_object_id_context_decode(
        &apdu[apdu_len], apdu_size - apdu_len, 0, &object_type,
        &object_instance);
    if (len <= 0) {
        if (list_element) {
            list_element->error_code = ERROR_CODE_REJECT_INVALID_TAG;
        }
        return BACNET_STATUS_REJECT;
    }
    apdu_len += len;
    if (list_element) {
        list_element->object_type = object_type;
        list_element->object_instance = object_instance;
    }
    /* Tag 1: Property ID */
    len = bacnet_enumerated_context_decode(
        &apdu[apdu_len], apdu_size - apdu_len, 1, &property);
    if (len <= 0) {
        if (list_element) {
            list_element->error_code = ERROR_CODE_REJECT_INVALID_TAG;
        }
        return BACNET_STATUS_REJECT;
    }
    if (list_element) {
        list_element->object_property = (BACNET_PROPERTY_ID)property;
    }
    apdu_len += len;
    /* Tag 2: Optional Array Index */
    len = bacnet_unsigned_context_decode(
        &apdu[apdu_len], apdu_size - apdu_len, 2, &unsigned_value);
    if (len > 0) {
        if (list_element) {
            list_element->array_index = (BACNET_ARRAY_INDEX)unsigned_value;
        }
        apdu_len += len;
    } else if (len == 0) {
        /* optional, so not an error if not present */
        if (list_element) {
            list_element->array_index = BACNET_ARRAY_ALL;
        }
    } else {
        if (list_element) {
            list_element->error_code = ERROR_CODE_REJECT_INVALID_TAG;
        }
        return BACNET_STATUS_REJECT;
    }
    /* Tag 3: opening context tag */
    if (bacnet_is_opening_tag_number(
            &apdu[apdu_len], apdu_size - apdu_len, 3, &len)) {
        application_data_len =
            bacnet_enclosed_data_length(&apdu[apdu_len], apdu_size - apdu_len);
        if (application_data_len < 0) {
            if (list_element) {
                list_element->error_code = ERROR_CODE_REJECT_INVALID_TAG;
            }
            return BACNET_STATUS_REJECT;
        }
        /* add the tag length */
        apdu_len += len;
        /* postpone the application data */
        if (list_element) {
            list_element->application_data = &apdu[apdu_len];
            list_element->application_data_len = application_data_len;
        }
        apdu_len += application_data_len;
    } else {
        if (list_element) {
            list_element->error_code = ERROR_CODE_REJECT_INVALID_TAG;
        }
        return BACNET_STATUS_REJECT;
    }
    if (bacnet_is_closing_tag_number(
            &apdu[apdu_len], apdu_size - apdu_len, 3, &len)) {
        apdu_len += len;
    } else {
        if (list_element) {
            list_element->error_code = ERROR_CODE_REJECT_INVALID_TAG;
        }
        return BACNET_STATUS_REJECT;
    }
    if (apdu_len < apdu_size) {
        /* If something left over now, we have an invalid request */
        if (list_element) {
            list_element->error_code = ERROR_CODE_REJECT_TOO_MANY_ARGUMENTS;
        }
        return BACNET_STATUS_REJECT;
    }

    return (int)apdu_len;
}

/**
 * @brief Encode a AddListElement-Error or RemoveListElement-Error APDU
 *  AddListElement-Error ::= SEQUENCE {
 *      error-type [0] Error,
 *      first-failed-element-number [1] UNSIGNED
 *  }
 *  RemoveListElement-Error ::= SEQUENCE {
 *      error-type [0] Error,
 *      first-failed-element-number [1] UNSIGNED
 *  }
 * @param apdu  Pointer to the buffer for encoding.
 * @param list_element  Pointer to the property data to be encoded.
 * @return Bytes encoded or zero on error.
 */
int list_element_error_ack_encode(
    uint8_t *apdu, const BACNET_LIST_ELEMENT_DATA *list_element)
{
    int len = 0; /* length of each encoding */
    int apdu_len = 0; /* total length of the apdu, return value */

    len = encode_opening_tag(apdu, 0);
    apdu_len += len;
    if (apdu) {
        apdu += len;
    }
    len = encode_application_enumerated(apdu, list_element->error_class);
    apdu_len += len;
    if (apdu) {
        apdu += len;
    }
    len = encode_application_enumerated(apdu, list_element->error_code);
    apdu_len += len;
    if (apdu) {
        apdu += len;
    }
    len = encode_closing_tag(apdu, 0);
    apdu_len += len;
    if (apdu) {
        apdu += len;
    }
    len = encode_context_unsigned(
        apdu, 1, list_element->first_failed_element_number);
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
 * @param list_element  Pointer to the property data to be encoded.
 * @return Bytes encoded or zero on error.
 */
int list_element_error_ack_decode(
    const uint8_t *apdu,
    uint16_t apdu_size,
    BACNET_LIST_ELEMENT_DATA *list_element)
{
    int len = 0, apdu_len = 0;
    BACNET_ERROR_CLASS error_class = ERROR_CLASS_SERVICES;
    BACNET_ERROR_CODE error_code = ERROR_CODE_SUCCESS;
    BACNET_UNSIGNED_INTEGER first_failed_element_number = 0;

    if (!apdu) {
        return 0;
    }
    if (list_element) {
        list_element->first_failed_element_number = 0;
        list_element->error_class = ERROR_CLASS_SERVICES;
        list_element->error_code = ERROR_CODE_REJECT_PARAMETER_OUT_OF_RANGE;
    }
    if (apdu_size < apdu_len) {
        return 0;
    }
    /* Opening Context tag 0 - Error */
    if (bacnet_is_opening_tag_number(apdu, apdu_size, 0, &len)) {
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
        if (list_element) {
            list_element->error_class = error_class;
            list_element->error_code = error_code;
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
    if (bacnet_is_closing_tag_number(apdu, apdu_size - apdu_len, 0, &len)) {
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
        if (list_element) {
            list_element->first_failed_element_number =
                first_failed_element_number;
        }
        apdu_len += len;
    } else {
        return 0;
    }

    return apdu_len;
}
