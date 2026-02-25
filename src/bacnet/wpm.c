/**
 * @file
 * @brief API for BACnet WritePropertyMultiple service encoder and decoder
 * @author Krzysztof Malorny <malornykrzysztof@gmail.com>
 * @author Nikola Jelic 2011 <nikola.jelic@euroicc.com>
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date 2011
 * @copyright SPDX-License-Identifier: MIT
 */
#include <stdint.h>
#include <string.h>
/* BACnet Stack defines - first */
#include "bacnet/bacdef.h"
/* BACnet Stack API */
#include "bacnet/bacapp.h"
#include "bacnet/bacdcode.h"
#include "bacnet/bacerror.h"
#include "bacnet/wp.h"
#include "bacnet/wpm.h"

/** @file wpm.c  Encode/Decode BACnet Write Property Multiple APDUs  */

/** Decoding for WritePropertyMultiple service, object ID.
 * @ingroup DSWPM
 * This handler will be invoked by write_property_multiple handler
 * if it has been enabled by a call to apdu_set_confirmed_handler().
 * This function decodes only the first tagged entity, which is
 * an object identifier.  This function will return an error if:
 *   - the tag is not the right value
 *   - the number of bytes is not enough to decode for this entity
 *   - the subsequent tag number is incorrect
 *
 * @param apdu [in] The contents of the APDU buffer.
 * @param apdu_len [in] The length of the APDU buffer.
 * @param wp_data [out] The BACNET_WRITE_PROPERTY_DATA structure
 *    which will contain the response values or error.
 *
 * @return Count of decoded bytes.
 */
int wpm_decode_object_id(
    const uint8_t *apdu,
    uint16_t apdu_size,
    BACNET_WRITE_PROPERTY_DATA *wp_data)
{
    uint32_t object_instance = 0;
    BACNET_OBJECT_TYPE object_type = OBJECT_NONE;
    int len = 0, apdu_len = 0;

    /* check for valid pointer and minimum size */
    if (apdu && (apdu_size > 5)) {
        /* Context tag 0 - Object ID */
        len = bacnet_object_id_context_decode(
            &apdu[apdu_len], apdu_size - apdu_len, 0, &object_type,
            &object_instance);
        if (len < 0) {
            if (wp_data) {
                wp_data->error_code = ERROR_CODE_REJECT_INVALID_TAG;
            }
            return BACNET_STATUS_REJECT;
        } else if (len == 0) {
            if (wp_data) {
                wp_data->error_code =
                    ERROR_CODE_REJECT_MISSING_REQUIRED_PARAMETER;
            }
            return BACNET_STATUS_REJECT;
        }
        apdu_len += len;
        if (wp_data) {
            wp_data->object_type = object_type;
            wp_data->object_instance = object_instance;
        }
        /* just test for the next tag - no need to decode it here */
        /* Context tag 1: sequence of BACnetPropertyValue */
        if (!bacnet_is_opening_tag_number(
                &apdu[len], apdu_size - apdu_len, 1, NULL)) {
            if (wp_data) {
                wp_data->error_code = ERROR_CODE_REJECT_INVALID_TAG;
            }
            return BACNET_STATUS_REJECT;
        }
    } else {
        if (wp_data) {
            wp_data->error_code = ERROR_CODE_REJECT_MISSING_REQUIRED_PARAMETER;
        }
        return BACNET_STATUS_REJECT;
    }

    return (int)len;
}

/** Decoding for an object property.
 *
 * BACnetPropertyValue ::= SEQUENCE {
 *   property-identifier[0] BACnetPropertyIdentifier,
 *   property-array-index[1] Unsigned OPTIONAL,
 *   -- used only with array datatypes
 *   -- if omitted with an array the entire array is referenced
 *   property-value[2] ABSTRACT-SYNTAX.&Type,
 *   -- any datatype appropriate for the specified property
 *   priority[3] Unsigned (1..16) OPTIONAL
 *   -- used only when property is commandable
 * }
 *
 * @param apdu [in] The contents of the APDU buffer.
 * @param apdu_len [in] The length of the APDU buffer.
 * @param wp_data [out] The BACNET_WRITE_PROPERTY_DATA structure.
 *
 * @return Bytes decoded
 */
int wpm_decode_object_property(
    const uint8_t *apdu,
    uint16_t apdu_size,
    BACNET_WRITE_PROPERTY_DATA *wp_data)
{
    uint32_t enum_value = 0;
    BACNET_UNSIGNED_INTEGER unsigned_value = 0;
    int len = 0, apdu_len = 0, i = 0, imax = 0;

    if (!apdu || (apdu_size == 0)) {
        if (wp_data) {
            wp_data->error_code = ERROR_CODE_REJECT_MISSING_REQUIRED_PARAMETER;
        }
        return BACNET_STATUS_REJECT;
    }
    if (wp_data) {
        wp_data->error_code = ERROR_CODE_OTHER;
        wp_data->array_index = BACNET_ARRAY_ALL;
        wp_data->priority = BACNET_MAX_PRIORITY;
        wp_data->application_data_len = 0;
    }
    /* property-identifier[0] BACnetPropertyIdentifier */
    len = bacnet_enumerated_context_decode(
        &apdu[apdu_len], apdu_size - apdu_len, 0, &enum_value);
    if (len <= 0) {
        if (wp_data) {
            wp_data->error_code = ERROR_CODE_REJECT_INVALID_TAG;
        }
        return BACNET_STATUS_REJECT;
    }
    apdu_len += len;
    if (wp_data) {
        wp_data->object_property = enum_value;
    }
    len = bacnet_unsigned_context_decode(
        &apdu[apdu_len], apdu_size - apdu_len, 1, &unsigned_value);
    if (len > 0) {
        /* property-array-index [1] Unsigned OPTIONAL */
        apdu_len += len;
        if (wp_data) {
            wp_data->array_index = unsigned_value;
        }
    } else if (len == 0) {
        /* optional - assume ALL array elements */
        if (wp_data) {
            wp_data->array_index = BACNET_ARRAY_ALL;
        }
    } else {
        if (wp_data) {
            wp_data->error_code = ERROR_CODE_REJECT_INVALID_TAG;
        }
        return BACNET_STATUS_REJECT;
    }
    /* property-value[2] ABSTRACT-SYNTAX.&Type */
    if (!bacnet_is_opening_tag_number(
            &apdu[apdu_len], apdu_size - apdu_len, 2, &len)) {
        if (wp_data) {
            wp_data->error_code = ERROR_CODE_REJECT_INVALID_TAG;
        }
        return BACNET_STATUS_REJECT;
    }
    /* note: evaluate with the tag as the first octet */
    imax = bacnet_enclosed_data_length(&apdu[apdu_len], apdu_size - apdu_len);
    if (imax == BACNET_STATUS_ERROR) {
        if (wp_data) {
            wp_data->error_code = ERROR_CODE_REJECT_INVALID_TAG;
        }
        return BACNET_STATUS_REJECT;
    }
    /* add the tag length */
    apdu_len += len;
    /* copy application data, check max length */
    if (imax > (apdu_size - apdu_len)) {
        imax = (apdu_size - apdu_len);
    }
    for (i = 0; i < imax; i++) {
        if (wp_data) {
            wp_data->application_data[i] = apdu[apdu_len + i];
        }
    }
    if (wp_data) {
        wp_data->application_data_len = imax;
    }
    apdu_len += imax;
    len = bacnet_is_closing_tag_number(
        &apdu[apdu_len], apdu_size - apdu_len, 2, &len);
    if (len > 0) {
        apdu_len += len;
    } else {
        if (wp_data) {
            wp_data->error_code = ERROR_CODE_REJECT_INVALID_TAG;
        }
        return BACNET_STATUS_REJECT;
    }
    /* priority[3] Unsigned (1..16) OPTIONAL */
    len = bacnet_unsigned_context_decode(
        &apdu[apdu_len], apdu_size - apdu_len, 3, &unsigned_value);
    if (len > 0) {
        apdu_len += len;
        if (wp_data) {
            wp_data->priority = unsigned_value;
        }
    } else if (len == 0) {
        /* OPTIONAL - no priority */
        if (wp_data) {
            wp_data->priority = BACNET_NO_PRIORITY;
        }
    } else {
        if (wp_data) {
            wp_data->error_code = ERROR_CODE_REJECT_INVALID_TAG;
        }
        return BACNET_STATUS_REJECT;
    }

    return apdu_len;
}

/**
 * @brief Init the APDU for encoding.
 * @param apdu [in] The APDU buffer, or NULL for length
 * @param invoke_id [in] The ID of the saervice invoked.
 * @return number of bytes encoded
 */
int wpm_encode_apdu_init(uint8_t *apdu, uint8_t invoke_id)
{
    if (apdu) {
        apdu[0] = PDU_TYPE_CONFIRMED_SERVICE_REQUEST;
        apdu[1] = encode_max_segs_max_apdu(0, MAX_APDU);
        apdu[2] = invoke_id;
        apdu[3] = SERVICE_CONFIRMED_WRITE_PROP_MULTIPLE; /* service choice */
    }

    return 4;
}

/**
 * @brief Decode the very begin of an object in the APDU.
 * @param apdu [in] The APDU buffer, or NULL for length
 * @param object_type [in] The object type to decode.
 * @param object_instance [in] The object instance.
 * @return number of bytes encoded
 */
int wpm_encode_apdu_object_begin(
    uint8_t *apdu, BACNET_OBJECT_TYPE object_type, uint32_t object_instance)
{
    int apdu_len = 0; /* total length of the apdu, return value */
    int len = 0;

    len = encode_context_object_id(apdu, 0, object_type, object_instance);
    apdu_len += len;
    if (apdu) {
        apdu += len;
    }
    /* Tag 1: sequence of WriteAccessSpecification */
    len = encode_opening_tag(apdu, 1);
    apdu_len += len;

    return apdu_len;
}

/**
 * @brief Encode the very end of an object in the APDU.
 * @param apdu [in] The APDU buffer, or NULL for length
 * @return number of bytes encoded
 */
int wpm_encode_apdu_object_end(uint8_t *apdu)
{
    return encode_closing_tag(apdu, 1);
}

/**
 * @brief Encode the object property into the APDU.
 * @param apdu [in] The APDU buffer, or NULL for length
 * @param wpdata [in] Pointer to the property data.
 * @return number of bytes encoded
 */
int wpm_encode_apdu_object_property(
    uint8_t *apdu, const BACNET_WRITE_PROPERTY_DATA *wpdata)
{
    int apdu_len = 0; /* total length of the apdu, return value */
    int len = 0;

    if (!wpdata) {
        return 0;
    }
    len = encode_context_enumerated(apdu, 0, wpdata->object_property);
    apdu_len += len;
    if (apdu) {
        apdu += len;
    }
    /* optional array index */
    if (wpdata->array_index != BACNET_ARRAY_ALL) {
        len = encode_context_unsigned(apdu, 1, wpdata->array_index);
        apdu_len += len;
        if (apdu) {
            apdu += len;
        }
    }
    len = encode_opening_tag(apdu, 2);
    apdu_len += len;
    if (apdu) {
        apdu += len;
    }
    /* copy the property value */
    apdu_len += wpdata->application_data_len;
    if (apdu) {
        for (len = 0; len < wpdata->application_data_len; len++) {
            *apdu = wpdata->application_data[len];
            apdu++;
        }
    }
    len = encode_closing_tag(apdu, 2);
    apdu_len += len;
    if (apdu) {
        apdu += len;
    }
    /* optional priority */
    if (wpdata->priority != BACNET_NO_PRIORITY) {
        len = encode_context_unsigned(apdu, 3, wpdata->priority);
        apdu_len += len;
    }

    return apdu_len;
}

/**
 * @brief Encode APDU for WritePropertyMultiple-Request
 *
 *  WritePropertyMultiple-Request ::= SEQUENCE {
 *
 *  }
 *
 * @param apdu  Pointer to the buffer, or NULL for length
 * @param data  Pointer to the data to encode.
 * @return number of bytes encoded, or zero on error.
 */
int write_property_multiple_request_encode(
    uint8_t *apdu, BACNET_WRITE_ACCESS_DATA *data)
{
    int len = 0; /* length of each encoding */
    int apdu_len = 0; /* total length of the apdu, return value */
    BACNET_WRITE_ACCESS_DATA *wpm_object; /* current object */
    BACNET_PROPERTY_VALUE *wpm_property; /* current property */
    BACNET_WRITE_PROPERTY_DATA wpdata;

    wpm_object = data;
    while (wpm_object) {
        len = wpm_encode_apdu_object_begin(
            apdu, wpm_object->object_type, wpm_object->object_instance);
        apdu_len += len;
        if (apdu) {
            apdu += len;
        }
        wpm_property = wpm_object->listOfProperties;
        while (wpm_property) {
            wpdata.object_property = wpm_property->propertyIdentifier;
            wpdata.array_index = wpm_property->propertyArrayIndex;
            wpdata.priority = wpm_property->priority;
            /* check length for fitting */
            wpdata.application_data_len =
                bacapp_encode_data(NULL, &wpm_property->value);
            if (wpdata.application_data_len > sizeof(wpdata.application_data)) {
                /* too big for buffer */
                return 0;
            }
            wpdata.application_data_len = bacapp_encode_data(
                wpdata.application_data, &wpm_property->value);
            len = wpm_encode_apdu_object_property(apdu, &wpdata);
            apdu_len += len;
            if (apdu) {
                apdu += len;
            }
            wpm_property = wpm_property->next;
        }
        len = wpm_encode_apdu_object_end(apdu);
        apdu_len += len;
        if (apdu) {
            apdu += len;
        }
        wpm_object = wpm_object->next;
    }

    return apdu_len;
}

/**
 * @brief Encode the WritePropertyMultiple-Request service
 * @param apdu  Pointer to the buffer for encoding into
 * @param apdu_size number of bytes available in the buffer
 * @param data  Pointer to the service data used for encoding values
 * @return number of bytes encoded, or zero if unable to encode or
 *  too big for buffer
 */
size_t write_property_multiple_request_service_encode(
    uint8_t *apdu, size_t apdu_size, BACNET_WRITE_ACCESS_DATA *data)
{
    size_t apdu_len = 0; /* total length of the apdu, return value */

    apdu_len = write_property_multiple_request_encode(NULL, data);
    if (apdu_len > apdu_size) {
        /* too big for buffer */
        apdu_len = 0;
    } else {
        apdu_len = write_property_multiple_request_encode(apdu, data);
    }

    return apdu_len;
}

/**
 * @brief Encode the WritePropertyMultiple-Request into the APDU.
 * @param apdu [in] The APDU buffer
 * @param apdu_size [in] Maximum space in the buffer.
 * @param invoke_id [in] Invoked service ID.
 * @param data [in] BACnetWriteAccessData
 * @return number of bytes encoded, or zero if unable to encode or
 *  too big for buffer
 */
int wpm_encode_apdu(
    uint8_t *apdu,
    size_t apdu_size,
    uint8_t invoke_id,
    BACNET_WRITE_ACCESS_DATA *data)
{
    int apdu_len = 0;
    int len = 0;

    len = wpm_encode_apdu_init(NULL, invoke_id);
    if (len > apdu_size) {
        /* too big for buffer */
        return 0;
    }
    len = wpm_encode_apdu_init(apdu, invoke_id);
    apdu_len += len;
    if (apdu) {
        apdu += len;
    }
    len = write_property_multiple_request_service_encode(
        apdu, apdu_size - apdu_len, data);
    if (len > 0) {
        /* too big for buffer */
        apdu_len += len;
    } else {
        return 0;
    }

    return apdu_len;
}

/** Init the APDU for encoding the confiremd write property multiple service.
 *
 * @param apdu [in] The APDU buffer.
 * @param invoke_id [in] Invoked service ID.
 *
 * @return Bytes encoded, usually 3.
 */
int wpm_ack_encode_apdu_init(uint8_t *apdu, uint8_t invoke_id)
{
    int len = 0;

    if (apdu) {
        apdu[len++] = PDU_TYPE_SIMPLE_ACK;
        apdu[len++] = invoke_id;
        apdu[len++] = SERVICE_CONFIRMED_WRITE_PROP_MULTIPLE;
    }

    return len;
}

/** Encode an Error acknowledge in the APDU.
 *
 * @param apdu [in] The APDU buffer, or NULL for length
 * @param wp_data [in] Data of the invoked property.
 *
 * @return number of bytes encoded.
 */
int wpm_error_ack_service_encode(
    uint8_t *apdu, const BACNET_WRITE_PROPERTY_DATA *wp_data)
{
    int len = 0, apdu_len = 0;

    len = encode_opening_tag(apdu, 0);
    apdu_len += len;
    if (apdu) {
        apdu += len;
    }
    len = encode_application_enumerated(apdu, wp_data->error_class);
    apdu_len += len;
    if (apdu) {
        apdu += len;
    }
    len = encode_application_enumerated(apdu, wp_data->error_code);
    apdu_len += len;
    if (apdu) {
        apdu += len;
    }
    len = encode_closing_tag(apdu, 0);
    apdu_len += len;
    if (apdu) {
        apdu += len;
    }
    len = encode_opening_tag(apdu, 1);
    apdu_len += len;
    if (apdu) {
        apdu += len;
    }
    len = encode_context_object_id(
        apdu, 0, wp_data->object_type, wp_data->object_instance);
    apdu_len += len;
    if (apdu) {
        apdu += len;
    }
    len = encode_context_enumerated(apdu, 1, wp_data->object_property);
    apdu_len += len;
    if (apdu) {
        apdu += len;
    }
    if (wp_data->array_index != BACNET_ARRAY_ALL) {
        len = encode_context_unsigned(apdu, 2, wp_data->array_index);
        apdu_len += len;
        if (apdu) {
            apdu += len;
        }
    }
    len = encode_closing_tag(apdu, 1);
    apdu_len += len;

    return apdu_len;
}

/** Encode an Error acknowledge in the APDU.
 *
 * @param apdu [in] The APDU buffer, or NULL for length
 * @param invoke_id [in] Invoked service ID.
 * @param wp_data [in] Data of the invoked property.
 *
 * @return number of bytes encoded.
 */
int wpm_error_ack_encode_apdu(
    uint8_t *apdu, uint8_t invoke_id, const BACNET_WRITE_PROPERTY_DATA *wp_data)
{
    int len = 0, apdu_len = 0;

    if (apdu) {
        apdu[0] = PDU_TYPE_ERROR;
        apdu[1] = invoke_id;
        apdu[2] = SERVICE_CONFIRMED_WRITE_PROP_MULTIPLE;
    }
    len = 3;
    apdu_len += len;
    if (apdu) {
        apdu += len;
    }
    len = wpm_error_ack_service_encode(apdu, wp_data);
    apdu_len += len;

    return apdu_len;
}

#if !BACNET_SVC_SERVER
/** Decoding for WritePropertyMultiple Error
 * @ingroup DSWPM
 *  WritePropertyMultiple-Error ::= SEQUENCE {
 *      error-type [0] Error,
 *      first-failed-write-attempt [1] BACnetObjectPropertyReference
 *  }
 *
 * @param apdu [in] The contents of the APDU buffer.
 * @param apdu_size [in] The size of the APDU buffer.
 * @param wp_data [out] The BACNET_WRITE_PROPERTY_DATA structure
 *    which will contain the response values or error.
 *
 * @return Count of decoded bytes.
 */
int wpm_error_ack_decode_apdu(
    const uint8_t *apdu,
    uint16_t apdu_size,
    BACNET_WRITE_PROPERTY_DATA *wp_data)
{
    int len = 0, apdu_len = 0;
    BACNET_ERROR_CLASS error_class = ERROR_CLASS_SERVICES;
    BACNET_ERROR_CODE error_code = ERROR_CODE_SUCCESS;
    BACNET_OBJECT_PROPERTY_REFERENCE value;

    if (!apdu) {
        if (wp_data) {
            wp_data->error_class = ERROR_CLASS_SERVICES;
            wp_data->error_code = ERROR_CODE_REJECT_MISSING_REQUIRED_PARAMETER;
        }
        return 0;
    }
    if (wp_data) {
        wp_data->error_class = ERROR_CLASS_SERVICES;
        wp_data->error_code = ERROR_CODE_REJECT_PARAMETER_OUT_OF_RANGE;
    }
    if (apdu_size == 0) {
        return 0;
    }
    /* Context tag 0 - Error */
    if (bacnet_is_opening_tag_number(
            &apdu[apdu_len], apdu_size - apdu_len, 0, &len)) {
        apdu_len += len;
    } else {
        return 0;
    }
    len = bacerror_decode_error_class_and_code(
        &apdu[apdu_len], apdu_size - apdu_len, &error_class, &error_code);
    if (len > 0) {
        if (wp_data) {
            wp_data->error_class = error_class;
            wp_data->error_code = error_code;
        }
        apdu_len += len;
    } else {
        return 0;
    }
    if (apdu_size == 0) {
        return 0;
    }
    if (bacnet_is_closing_tag_number(
            &apdu[apdu_len], apdu_size - apdu_len, 0, &len)) {
        apdu_len += len;
    } else {
        return 0;
    }
    /* Context tag 1 - BACnetObjectPropertyReference */
    if (bacnet_is_opening_tag_number(
            &apdu[apdu_len], apdu_size - apdu_len, 1, &len)) {
        apdu_len += len;
    } else {
        return 0;
    }
    len = bacapp_decode_obj_property_ref(
        &apdu[apdu_len], apdu_size - apdu_len, &value);
    if (len > 0) {
        if (wp_data) {
            wp_data->object_instance = value.object_identifier.instance;
            wp_data->object_type = value.object_identifier.type;
            wp_data->object_property = value.property_identifier;
            wp_data->array_index = value.property_array_index;
        }
        apdu_len += len;
    } else {
        return 0;
    }
    if (bacnet_is_closing_tag_number(
            &apdu[apdu_len], apdu_size - apdu_len, 1, &len)) {
        apdu_len += len;
    } else {
        return 0;
    }

    return apdu_len;
}
#endif

/**
 * @brief Convert an array of BACnetWriteAccessData to linked list
 * @param array pointer to element zero of the array
 * @param size number of elements in the array
 */
void wpm_write_access_data_link_array(
    BACNET_WRITE_ACCESS_DATA *array, size_t size)
{
    size_t i = 0;

    for (i = 0; i < size; i++) {
        if (i > 0) {
            array[i - 1].next = &array[i];
        }
        array[i].next = NULL;
    }
}
