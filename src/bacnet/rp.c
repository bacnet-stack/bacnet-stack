/**
 * @file
 * @brief BACnet ReadProperty-Request and ReadProperty-ACK encode and decode 
 *  helper functions
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date 2005
 * @copyright SPDX-License-Identifier: GPL-2.0-or-later WITH GCC-exception-2.0
 */
#include <stdint.h>
/* BACnet Stack defines - first */
#include "bacnet/bacdef.h"
/* BACnet Stack API */
#include "bacnet/bacdcode.h"
#include "bacnet/rp.h"

#if BACNET_SVC_RP_A
/**
 * @brief Encode APDU for ReadProperty-Request
 *
 *  ReadProperty-Request ::= SEQUENCE {
 *      object-identifier [0] BACnetObjectIdentifier,
 *      property-identifier [1] BACnetPropertyIdentifier,
 *      property-array-index [2] Unsigned OPTIONAL
 *      -- used only with array datatype
 *      -- if omitted with an array the entire array is referenced
 *  }
 *
 * @param apdu  Pointer to the buffer, or NULL for length
 * @param data  Pointer to the data to encode.
 * @return number of bytes encoded, or zero on error.
 */
int read_property_request_encode(uint8_t *apdu, BACNET_READ_PROPERTY_DATA *data)
{
    int len = 0; /* length of each encoding */
    int apdu_len = 0; /* total length of the apdu, return value */

    if (!data) {
        return 0;
    }
    /* object-identifier [0] BACnetObjectIdentifier */
    if (data->object_type <= BACNET_MAX_OBJECT) {
        len = encode_context_object_id(
            apdu, 0, data->object_type, data->object_instance);
        apdu_len += len;
        if (apdu) {
            apdu += len;
        }
    }
    /* property-identifier [1] BACnetPropertyIdentifier */
    if (data->object_property <= MAX_BACNET_PROPERTY_ID) {
        len = encode_context_enumerated(apdu, 1, data->object_property);
        apdu_len += len;
        if (apdu) {
            apdu += len;
        }
    }
    /* property-array-index [2] Unsigned OPTIONAL */
    if (data->array_index != BACNET_ARRAY_ALL) {
        len = encode_context_unsigned(apdu, 2, data->array_index);
        apdu_len += len;
    }

    return apdu_len;
}

/**
 * @brief Encode the ReadProperty-Request service
 * @param apdu  Pointer to the buffer for encoding into
 * @param apdu_size number of bytes available in the buffer
 * @param data  Pointer to the service data used for encoding values
 * @return number of bytes encoded, or zero if unable to encode or too large
 */
size_t read_property_request_service_encode(
    uint8_t *apdu, size_t apdu_size, BACNET_READ_PROPERTY_DATA *data)
{
    size_t apdu_len = 0; /* total length of the apdu, return value */

    apdu_len = read_property_request_encode(NULL, data);
    if (apdu_len > apdu_size) {
        apdu_len = 0;
    } else {
        apdu_len = read_property_request_encode(apdu, data);
    }

    return apdu_len;
}

/** Encode the service
 *
 * @param apdu  Pointer to the buffer for encoding.
 * @param invoke_id  Invoke ID
 * @param rpdata  Pointer to the property data to be encoded.
 *
 * @return Bytes encoded or zero on error.
 */
int rp_encode_apdu(
    uint8_t *apdu, uint8_t invoke_id, BACNET_READ_PROPERTY_DATA *data)
{
    int len = 0; /* length of each encoding */
    int apdu_len = 0; /* total length of the apdu, return value */

    if (apdu) {
        apdu[0] = PDU_TYPE_CONFIRMED_SERVICE_REQUEST;
        apdu[1] = encode_max_segs_max_apdu(0, MAX_APDU);
        apdu[2] = invoke_id;
        apdu[3] = SERVICE_CONFIRMED_READ_PROPERTY; /* service choice */
    }
    len = 4;
    apdu_len += len;
    if (apdu) {
        apdu += len;
    }
    len = read_property_request_encode(apdu, data);
    apdu_len += len;

    return apdu_len;
}
#endif

/** Decode the service request only
 *
 * @param apdu  Pointer to the buffer for encoding.
 * @param apdu_size  Count of valid bytes in the buffer.
 * @param rpdata  Pointer to the property data to be encoded.
 *
 * @return number of bytes decoded, or #BACNET_STATUS_REJECT
 */
int rp_decode_service_request(
    uint8_t *apdu, unsigned apdu_size, BACNET_READ_PROPERTY_DATA *data)
{
    int len = 0;
    int apdu_len = 0;
    uint32_t instance = 0;
    BACNET_OBJECT_TYPE type = OBJECT_NONE; /* for decoding */
    uint32_t property = 0; /* for decoding */
    BACNET_UNSIGNED_INTEGER unsigned_value = 0; /* for decoding */

    /* check for value pointers */
    if (!apdu) {
        if (data) {
            data->error_code = ERROR_CODE_REJECT_MISSING_REQUIRED_PARAMETER;
        }
        return BACNET_STATUS_REJECT;
    }
    /* object-identifier [0] BACnetObjectIdentifier */
    len = bacnet_object_id_context_decode(
        &apdu[apdu_len], apdu_size - apdu_len, 0, &type, &instance);
    if (len > 0) {
        if (instance > BACNET_MAX_INSTANCE) {
            if (data) {
                data->error_code = ERROR_CODE_REJECT_PARAMETER_OUT_OF_RANGE;
            }
            return BACNET_STATUS_REJECT;
        }
        apdu_len += len;
        if (data) {
            data->object_type = type;
            data->object_instance = instance;
        }
    } else {
        if (data) {
            data->error_code = ERROR_CODE_REJECT_INVALID_TAG;
        }
        return BACNET_STATUS_REJECT;
    }
    /* property-identifier [1] BACnetPropertyIdentifier */
    len = bacnet_enumerated_context_decode(
        &apdu[apdu_len], apdu_size - apdu_len, 1, &property);
    if (len > 0) {
        apdu_len += len;
        if (data) {
            data->object_property = (BACNET_PROPERTY_ID)property;
        }
    } else {
        if (data) {
            data->error_code = ERROR_CODE_REJECT_INVALID_TAG;
        }
        return BACNET_STATUS_REJECT;
    }
    /* property-array-index [2] Unsigned OPTIONAL */
    len = bacnet_unsigned_context_decode(
        &apdu[apdu_len], apdu_size - apdu_len, 2, &unsigned_value);
    if (len > 0) {
        apdu_len += len;
        if (data) {
            data->array_index = unsigned_value;
        }
    } else {
        /* wrong tag - skip apdu_len increment and go to next field */
        if (data) {
            data->array_index = BACNET_ARRAY_ALL;
        }
    }
    if (apdu_len < apdu_size) {
        /* If something left over now, we have an invalid request */
        if (data) {
            data->error_code = ERROR_CODE_REJECT_TOO_MANY_ARGUMENTS;
        }
        return BACNET_STATUS_REJECT;
    }

    return apdu_len;
}

/**
 * @brief Encode APDU for ReadProperty-ACK
 *
 *  ReadProperty-ACK ::= SEQUENCE {
 *      object-identifier [0] BACnetObjectIdentifier,
 *      property-identifier [1] BACnetPropertyIdentifier,
 *      property-array-index [2] Unsigned OPTIONAL,
 *      -- used only with array datatype
 *      -- if omitted with an array the entire array is referenced
 *      property-value [3]
 *  }
 *
 * @param apdu  Pointer to the buffer, or NULL for length
 * @param data  Pointer to the data to encode.
 * @return number of bytes encoded, or zero on error.
 */
int read_property_ack_encode(uint8_t *apdu, BACNET_READ_PROPERTY_DATA *data)
{
    int len = 0; /* length of each encoding */
    int apdu_len = 0; /* total length of the apdu, return value */

    if (!data) {
        return 0;
    }
    /* object-identifier [0] BACnetObjectIdentifier */
    len = encode_context_object_id(
        apdu, 0, data->object_type, data->object_instance);
    apdu_len += len;
    if (apdu) {
        apdu += len;
    }
    /* property-identifier [1] BACnetPropertyIdentifier */
    len = encode_context_enumerated(apdu, 1, data->object_property);
    apdu_len += len;
    if (apdu) {
        apdu += len;
    }
    /* property-array-index [2] Unsigned OPTIONAL */
    if (data->array_index != BACNET_ARRAY_ALL) {
        len = encode_context_unsigned(apdu, 2, data->array_index);
        apdu_len += len;
        if (apdu) {
            apdu += len;
        }
    }
    /* property-value [3] */
    len = encode_opening_tag(apdu, 3);
    apdu_len += len;
    if (apdu) {
        apdu += len;
    }
    if (apdu) {
        for (len = 0; len < data->application_data_len; len++) {
            apdu[len] = data->application_data[len];
        }
        apdu += data->application_data_len;
    }
    apdu_len += data->application_data_len;
    len = encode_closing_tag(apdu, 3);
    apdu_len += len;

    return apdu_len;
}

/**
 * @brief Encode the COVNotification service request
 * @param apdu  Pointer to the buffer for encoding into
 * @param apdu_size number of bytes available in the buffer
 * @param data  Pointer to the service data used for encoding values
 * @return number of bytes encoded, or zero if unable to encode or too large
 */
size_t read_property_ack_service_encode(
    uint8_t *apdu, size_t apdu_size, BACNET_READ_PROPERTY_DATA *data)
{
    size_t apdu_len = 0; /* total length of the apdu, return value */

    apdu_len = read_property_ack_encode(NULL, data);
    if (apdu_len > apdu_size) {
        apdu_len = 0;
    } else {
        apdu_len = read_property_ack_encode(apdu, data);
    }

    return apdu_len;
}

/** Alternate method to encode the ack without extra buffer.
 *
 * @param apdu  Pointer to the buffer for encoding.
 * @param invoke_id  Invoke Id
 * @param rpdata  Pointer to the property data to be encoded.
 *
 * @return Bytes decoded or zero on error.
 */
int rp_ack_encode_apdu_init(
    uint8_t *apdu, uint8_t invoke_id, BACNET_READ_PROPERTY_DATA *rpdata)
{
    int len = 0; /* length of each encoding */
    int apdu_len = 0; /* total length of the apdu, return value */

    if (apdu) {
        apdu[0] = PDU_TYPE_COMPLEX_ACK; /* complex ACK service */
        apdu[1] = invoke_id; /* original invoke id from request */
        apdu[2] = SERVICE_CONFIRMED_READ_PROPERTY; /* service choice */
    }
    len = 3;
    apdu_len += len;
    if (apdu) {
        apdu += len;
    }
    /* service ack follows */
    len = encode_context_object_id(
        apdu, 0, rpdata->object_type, rpdata->object_instance);
    apdu_len += len;
    if (apdu) {
        apdu += len;
    }
    len = encode_context_enumerated(apdu, 1, rpdata->object_property);
    apdu_len += len;
    if (apdu) {
        apdu += len;
    }
    /* context 2 array index is optional */
    if (rpdata->array_index != BACNET_ARRAY_ALL) {
        len = encode_context_unsigned(apdu, 2, rpdata->array_index);
        apdu_len += len;
        if (apdu) {
            apdu += len;
        }
    }
    len = encode_opening_tag(apdu, 3);
    apdu_len += len;

    return apdu_len;
}

/** Encode the closing tag for the object property.
 *  Note: Encode the application tagged data yourself.
 *
 * @param apdu  Pointer to the buffer for encoding.
 * @param invoke_id  Invoke Id
 * @param rpdata  Pointer to the property data to be encoded.
 *
 * @return Bytes encoded or zero on error.
 */
int rp_ack_encode_apdu_object_property_end(uint8_t *apdu)
{
    int apdu_len = 0; /* total length of the apdu, return value */

    if (apdu) {
        apdu_len = encode_closing_tag(&apdu[0], 3);
    }

    return apdu_len;
}

/** Encode the acknowledge.
 *
 * @param apdu  Pointer to the buffer for encoding, or NULL for length
 * @param invoke_id  Invoke Id
 * @param rpdata  Pointer to the property data to be encoded.
 *
 * @return Bytes encoded or zero on error.
 */
int rp_ack_encode_apdu(
    uint8_t *apdu, uint8_t invoke_id, BACNET_READ_PROPERTY_DATA *rpdata)
{
    int imax = 0;
    int len = 0; /* length of each encoding */
    int apdu_len = 0; /* total length of the apdu, return value */

    if (rpdata) {
        /* Do the initial encoding */
        len = rp_ack_encode_apdu_init(apdu, invoke_id, rpdata);
        apdu_len += len;
        if (apdu) {
            apdu += len;
        }
        imax = rpdata->application_data_len;
        for (len = 0; len < imax; len++) {
            if (apdu) {
                apdu[len] = rpdata->application_data[len];
            }
        }
        apdu_len += len;
        if (apdu) {
            apdu += len;
        }
        len = encode_closing_tag(apdu, 3);
        apdu_len += len;
    }

    return apdu_len;
}

#if BACNET_SVC_RP_A
/** Decode the ReadProperty reply and store the result for one Property in a
 *  BACNET_READ_PROPERTY_DATA structure.
 *  This leaves the value(s) in the application_data buffer to be decoded later;
 *  the application_data field points into the apdu buffer (is not allocated).
 *
 * @param apdu [in] The apdu portion of the ACK reply.
 * @param apdu_size [in] The total length of the apdu.
 * @param rpdata [out] The structure holding the partially decoded result.
 * @return Number of decoded bytes (could be less than apdu_len),
 * 			or -1 on decoding error.
 */
int rp_ack_decode_service_request(uint8_t *apdu,
    int apdu_size,
    BACNET_READ_PROPERTY_DATA *data)
{
    int apdu_len = 0; /* return value */
    int len = 0;
    uint32_t instance = 0;
    BACNET_OBJECT_TYPE type = OBJECT_NONE; /* for decoding */
    uint32_t property = 0; /* for decoding */
    BACNET_UNSIGNED_INTEGER unsigned_value = 0; /* for decoding */
    int data_len = 0;

    if (!apdu) {
        return -BACNET_STATUS_ERROR;
    }
    /* object-identifier [0] BACnetObjectIdentifier */
    len = bacnet_object_id_context_decode(
        &apdu[apdu_len], apdu_size - apdu_len, 0, &type, &instance);
    if (len > 0) {
        if (instance > BACNET_MAX_INSTANCE) {
            return BACNET_STATUS_ERROR;
        }
        apdu_len += len;
        if (data) {
            data->object_type = type;
            data->object_instance = instance;
        }
    } else {
        return BACNET_STATUS_ERROR;
    }
    /* property-identifier [1] BACnetPropertyIdentifier */
    len = bacnet_enumerated_context_decode(
        &apdu[apdu_len], apdu_size - apdu_len, 1, &property);
    if (len > 0) {
        apdu_len += len;
        if (data) {
            data->object_property = (BACNET_PROPERTY_ID)property;
        }
    } else {
        return BACNET_STATUS_ERROR;
    }
    /* property-array-index [2] Unsigned OPTIONAL */
    len = bacnet_unsigned_context_decode(
        &apdu[apdu_len], apdu_size - apdu_len, 2, &unsigned_value);
    if (len > 0) {
        apdu_len += len;
        if (data) {
            data->array_index = unsigned_value;
        }
    } else {
        /* wrong tag - skip apdu_len increment and go to next field */
        if (data) {
            data->array_index = BACNET_ARRAY_ALL;
        }
    }
    /* property-value [3] ABSTRACT-SYNTAX.&Type */
    if (!bacnet_is_opening_tag_number(
            &apdu[apdu_len], apdu_size - apdu_len, 3, &len)) {
        return BACNET_STATUS_ERROR;
    }
    /* determine the length of the data blob */
    data_len = bacnet_enclosed_data_length(&apdu[apdu_len], 
        apdu_size - apdu_len);
    if (data_len == BACNET_STATUS_ERROR) {
        return BACNET_STATUS_ERROR;
    }
    /* count the opening tag number length */
    apdu_len += len;
    if (data_len > MAX_APDU) {
        /* not enough size in application_data to store the data chunk */
        return BACNET_STATUS_ERROR;
    } else if (data) {
        /* don't decode the application tag number or its data here */
        data->application_data = &apdu[apdu_len];
        data->application_data_len = data_len;
    }
    apdu_len += data_len;
    if (!bacnet_is_closing_tag_number(
            &apdu[apdu_len], apdu_size - apdu_len, 3, &len)) {
        return BACNET_STATUS_ERROR;
    }
    /* count the closing tag number length */
    apdu_len += len;

    return apdu_len;
}
#endif
