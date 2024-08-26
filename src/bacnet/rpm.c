/**
 * @file
 * @brief BACnet ReadPropertyMultiple -Request and -Ack encode and decode
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @author Peter McShane <petermcs@users.sourceforge.net>
 * @author Roy Schneider <postmaster@overthehill.de>
 * @date 2005
 * @copyright SPDX-License-Identifier: GPL-2.0-or-later WITH GCC-exception-2.0
 */
#include <stdint.h>
/* BACnet Stack defines - first */
#include "bacnet/bacdef.h"
/* BACnet Stack API */
#include "bacnet/bacerror.h"
#include "bacnet/bacdcode.h"
#include "bacnet/bacapp.h"
#include "bacnet/memcopy.h"
#include "bacnet/rpm.h"

/** @file rpm.c  Encode/Decode Read Property Multiple and RPM ACKs  */

#if BACNET_SVC_RPM_A

/**
 * @brief Encode the initial portion of the service
 * @param apdu application data unit buffer for encoding, or NULL for length
 * @param invoke_id  Invoke ID
 * @return number of bytes encoded
 */
int rpm_encode_apdu_init(uint8_t *apdu, uint8_t invoke_id)
{
    if (apdu) {
        apdu[0] = PDU_TYPE_CONFIRMED_SERVICE_REQUEST;
        apdu[1] = encode_max_segs_max_apdu(0, MAX_APDU);
        apdu[2] = invoke_id;
        apdu[3] = SERVICE_CONFIRMED_READ_PROP_MULTIPLE; /* service choice */
    }

    return 4;
}

/** Encode the beginning, including
 *  Object-id and Read-Access of the service.
 *
 * @param apdu application data unit buffer for encoding, or NULL for length
 * @param object_type  Object type to encode
 * @param object_instance  Object instance to encode
 *
 * @return number of bytes encoded
 */
int rpm_encode_apdu_object_begin(
    uint8_t *apdu, BACNET_OBJECT_TYPE object_type, uint32_t object_instance)
{
    int apdu_len = 0; /* total length of the apdu, return value */

    if (apdu) {
        apdu_len =
            encode_context_object_id(&apdu[0], 0, object_type, object_instance);
        /* Tag 1: sequence of ReadAccessSpecification */
        apdu_len += encode_opening_tag(&apdu[apdu_len], 1);
    }

    return apdu_len;
}

/** Encode the object properties of the service.
 *
 * @param apdu application data unit buffer for encoding, or NULL for length
 * @param object_property  Object property.
 * @param array_index Optional array index.
 *
 * @return number of bytes encoded
 */
int rpm_encode_apdu_object_property(
    uint8_t *apdu,
    BACNET_PROPERTY_ID object_property,
    BACNET_ARRAY_INDEX array_index)
{
    int apdu_len = 0; /* total length of the apdu, return value */

    if (apdu) {
        apdu_len = encode_context_enumerated(&apdu[0], 0, object_property);
        /* optional array index */
        if (array_index != BACNET_ARRAY_ALL) {
            apdu_len +=
                encode_context_unsigned(&apdu[apdu_len], 1, array_index);
        }
    }

    return apdu_len;
}

/** Encode the end (closing tag) of the service
 *
 * @param apdu application data unit buffer for encoding, or NULL for length
 *
 * @return number of bytes encoded
 */
int rpm_encode_apdu_object_end(uint8_t *apdu)
{
    int apdu_len = 0; /* total length of the apdu, return value */

    if (apdu) {
        apdu_len = encode_closing_tag(&apdu[0], 1);
    }

    return apdu_len;
}

/**
 * @brief Encode the ReadPropertyMultiple-Request
 * @param apdu application data unit buffer for encoding, or NULL for length
 * @param read_access_data [in] The RPM data to be requested.
 * @return number of bytes encoded
 */
int read_property_multiple_request_encode(
    uint8_t *apdu, BACNET_READ_ACCESS_DATA *data)
{
    int len = 0; /* length of each encoding */
    int apdu_len = 0; /* total length of the apdu, return value */
    BACNET_READ_ACCESS_DATA *rpm_object; /* current object */
    BACNET_PROPERTY_REFERENCE *rpm_property; /* current property */

    rpm_object = data;
    while (rpm_object) {
        len = encode_context_object_id(
            apdu, 0, rpm_object->object_type, rpm_object->object_instance);
        apdu_len += len;
        if (apdu) {
            apdu += len;
        }
        /* Tag 1: sequence of ReadAccessSpecification */
        len = encode_opening_tag(apdu, 1);
        apdu_len += len;
        if (apdu) {
            apdu += len;
        }
        rpm_property = rpm_object->listOfProperties;
        while (rpm_property) {
            len = encode_context_enumerated(
                apdu, 0, rpm_property->propertyIdentifier);
            apdu_len += len;
            if (apdu) {
                apdu += len;
            }
            /* optional array index */
            if (rpm_property->propertyArrayIndex != BACNET_ARRAY_ALL) {
                len = encode_context_unsigned(
                    apdu, 1, rpm_property->propertyArrayIndex);
                apdu_len += len;
                if (apdu) {
                    apdu += len;
                }
            }
            rpm_property = rpm_property->next;
        }
        len = encode_closing_tag(apdu, 1);
        apdu_len += len;
        if (apdu) {
            apdu += len;
        }
        rpm_object = rpm_object->next;
    }

    return apdu_len;
}

/**
 * @brief Encode the ReadPropertyMultiple-Request service
 * @param apdu application data unit buffer for encoding, or NULL for length
 * @param apdu_size number of bytes available in the buffer
 * @param data  Pointer to the service data used for encoding values
 * @return number of bytes encoded, or zero if unable to encode or too large
 */
size_t read_property_multiple_request_service_encode(
    uint8_t *apdu, size_t apdu_size, BACNET_READ_ACCESS_DATA *data)
{
    size_t apdu_len = 0; /* total length of the apdu, return value */

    apdu_len = read_property_multiple_request_encode(NULL, data);
    if (apdu_len > apdu_size) {
        apdu_len = 0;
    } else {
        apdu_len = read_property_multiple_request_encode(apdu, data);
    }

    return apdu_len;
}

/** Encode an RPM request, to be sent.
 *
 * @param apdu application data unit buffer for encoding, or NULL for length
 * @param apdu_size [in] Length of apdu buffer.
 * @param invoke_id [in] The Invoke ID to use for this message.
 * @param data [in] The RPM data to be requested.
 * @return number of bytes encoded, or zero if unable to encode or too large
 */
int rpm_encode_apdu(
    uint8_t *apdu,
    size_t apdu_size,
    uint8_t invoke_id,
    BACNET_READ_ACCESS_DATA *data)
{
    int apdu_len = 0; /* total length of the apdu, return value */
    int len = 0; /* length of the data */

    len = rpm_encode_apdu_init(NULL, invoke_id);
    if (len > apdu_size) {
        return 0;
    } else {
        len = rpm_encode_apdu_init(apdu, invoke_id);
        apdu_len += len;
        if (apdu) {
            apdu += len;
        }
        len = read_property_multiple_request_service_encode(
            apdu, apdu_size - apdu_len, data);
        if (len > 0) {
            apdu_len += len;
        } else {
            apdu_len = 0;
        }
    }

    return apdu_len;
}
#endif

/** Decode the object portion of the service request only. Bails out if
 *  tags are wrong or missing/incomplete.
 *
 * @param apdu application data unit buffer for decoding
 * @param apdu_len [in] Count of valid bytes in the buffer.
 * @param rpmdata [in] The data structure to be filled, or NULL for length
 *
 * @return number of decoded bytes, or negative on failure.
 */
int rpm_decode_object_id(
    uint8_t *apdu, unsigned apdu_len, BACNET_RPM_DATA *rpmdata)
{
    int len = 0;
    BACNET_OBJECT_TYPE type = OBJECT_NONE; /* for decoding */

    /* check for value pointers */
    if (apdu && apdu_len && rpmdata) {
        if (apdu_len < 5) { /* Must be at least 2 tags and an object id */
            rpmdata->error_code = ERROR_CODE_REJECT_MISSING_REQUIRED_PARAMETER;
            return BACNET_STATUS_REJECT;
        }
        /* Tag 0: Object ID */
        if (!decode_is_context_tag(&apdu[len++], 0)) {
            rpmdata->error_code = ERROR_CODE_REJECT_INVALID_TAG;
            return BACNET_STATUS_REJECT;
        }
        len += decode_object_id(&apdu[len], &type, &rpmdata->object_instance);
        rpmdata->object_type = type;
        /* Tag 1: sequence of ReadAccessSpecification */
        if (!decode_is_opening_tag_number(&apdu[len], 1)) {
            rpmdata->error_code = ERROR_CODE_REJECT_INVALID_TAG;
            return BACNET_STATUS_REJECT;
        }
        len++; /* opening tag is only one octet */
    }

    return len;
}

/**
 * @brief Decode the end portion of the service request only.
 * @param apdu application data unit buffer for decoding
 * @param apdu_len [in] Count of valid bytes in the buffer.
 * @return number of decoded bytes, or negative on failure.
 */
int rpm_decode_object_end(uint8_t *apdu, unsigned apdu_len)
{
    int len = 0; /* total length of the apdu, return value */

    if (apdu && apdu_len) {
        if (decode_is_closing_tag_number(apdu, 1) == true) {
            len = 1;
        }
    }

    return len;
}

/**
 * @brief Decode the object property portion of the service request only
 *
 *  BACnetPropertyReference ::= SEQUENCE {
 *      propertyIdentifier [0] BACnetPropertyIdentifier,
 *      propertyArrayIndex [1] Unsigned OPTIONAL
 *      --used only with array datatype
 *      -- if omitted with an array the entire array is referenced
 *  }
 *
 * @param apdu application data unit buffer for decoding
 * @param apdu_len  Count of received bytes.
 * @param rpmdata  Pointer to the data structure to be filled.
 * @return number of decoded bytes, or negative on failure.
 */
int rpm_decode_object_property(
    uint8_t *apdu, unsigned apdu_len, BACNET_RPM_DATA *rpmdata)
{
    int len = 0;
    int option_len = 0;
    uint8_t tag_number = 0;
    uint32_t len_value_type = 0;
    uint32_t property = 0; /* for decoding */
    BACNET_UNSIGNED_INTEGER unsigned_value = 0; /* for decoding */

    /* check for valid pointers */
    if (apdu && apdu_len && rpmdata) {
        /* Tag 0: propertyIdentifier */
        if (!IS_CONTEXT_SPECIFIC(apdu[len])) {
            rpmdata->error_code = ERROR_CODE_REJECT_INVALID_TAG;
            return BACNET_STATUS_REJECT;
        }

        len += decode_tag_number_and_value(
            &apdu[len], &tag_number, &len_value_type);
        if (tag_number != 0) {
            rpmdata->error_code = ERROR_CODE_REJECT_INVALID_TAG;
            return BACNET_STATUS_REJECT;
        }
        /* Should be at least the unsigned value + 1 tag left */
        if ((len + len_value_type) >= apdu_len) {
            rpmdata->error_code = ERROR_CODE_REJECT_MISSING_REQUIRED_PARAMETER;
            return BACNET_STATUS_REJECT;
        }
        len += decode_enumerated(&apdu[len], len_value_type, &property);
        rpmdata->object_property = (BACNET_PROPERTY_ID)property;
        /* Assume most probable outcome */
        rpmdata->array_index = BACNET_ARRAY_ALL;
        /* Tag 1: Optional propertyArrayIndex */
        if (IS_CONTEXT_SPECIFIC(apdu[len]) && !IS_CLOSING_TAG(apdu[len])) {
            option_len = decode_tag_number_and_value(
                &apdu[len], &tag_number, &len_value_type);
            if (tag_number == 1) {
                len += option_len;
                /* Should be at least the unsigned array index + 1 tag left */
                if ((len + len_value_type) >= apdu_len) {
                    rpmdata->error_code =
                        ERROR_CODE_REJECT_MISSING_REQUIRED_PARAMETER;
                    return BACNET_STATUS_REJECT;
                }
                len += decode_unsigned(
                    &apdu[len], len_value_type, &unsigned_value);
                rpmdata->array_index = unsigned_value;
            }
        }
    }

    return len;
}

/**
 * @brief Encode the acknowledge for a RPM.
 * @param apdu application data unit buffer for encoding, or NULL for length
 * @param invoke_id [in] Invoke Id.
 * @return number of bytes encoded
 */
int rpm_ack_encode_apdu_init(uint8_t *apdu, uint8_t invoke_id)
{
    if (apdu) {
        apdu[0] = PDU_TYPE_COMPLEX_ACK; /* complex ACK service */
        apdu[1] = invoke_id; /* original invoke id from request */
        apdu[2] = SERVICE_CONFIRMED_READ_PROP_MULTIPLE; /* service choice */
    }

    return 3;
}

/**
 * @brief Encode the object type for an acknowledge of a RPM.
 * @param apdu application data unit buffer for encoding, or NULL for length
 * @param rpmdata [in] Pointer to the data used to fill in the APDU.
 * @return number of bytes encoded
 */
int rpm_ack_encode_apdu_object_begin(uint8_t *apdu, BACNET_RPM_DATA *rpmdata)
{
    int apdu_len = 0; /* total length of the apdu, return value */
    int len = 0;

    /* Tag 0: objectIdentifier */
    len = encode_context_object_id(
        apdu, 0, rpmdata->object_type, rpmdata->object_instance);
    apdu_len += len;
    if (apdu) {
        apdu += len;
    }
    /* Tag 1: listOfResults */
    len = encode_opening_tag(apdu, 1);
    apdu_len += len;

    return apdu_len;
}

/**
 * @brief Encode the object property for an acknowledge of a RPM.
 * @param apdu application data unit buffer for encoding, or NULL for length
 * @param object_property [in] Object property ID.
 * @param array_index  Optional array index
 * @return number of bytes encoded
 */
int rpm_ack_encode_apdu_object_property(
    uint8_t *apdu,
    BACNET_PROPERTY_ID object_property,
    BACNET_ARRAY_INDEX array_index)
{
    int apdu_len = 0; /* total length of the apdu, return value */
    int len = 0;

    /* Tag 2: propertyIdentifier */
    len = encode_context_enumerated(apdu, 2, object_property);
    apdu_len += len;
    if (apdu) {
        apdu += len;
    }
    /* Tag 3: optional propertyArrayIndex */
    if (array_index != BACNET_ARRAY_ALL) {
        len = encode_context_unsigned(apdu, 3, array_index);
        apdu_len += len;
    }

    return apdu_len;
}

/**
 * @brief Encode the object property value for an acknowledge of a RPM.
 * @param apdu application data unit buffer for encoding, or NULL for length
 * @param application_data [in] Pointer to the application data used to fill in
 * the APDU.
 * @param application_data_len [in] Length of the application data.
 * @return number of bytes encoded
 */
int rpm_ack_encode_apdu_object_property_value(
    uint8_t *apdu, uint8_t *application_data, unsigned application_data_len)
{
    int apdu_len = 0; /* total length of the apdu, return value */
    int len = 0;

    /* Tag 4: propertyValue */
    len = encode_opening_tag(apdu, 4);
    apdu_len += len;
    if (apdu) {
        apdu += len;
    }
    if (application_data != apdu) {
        /* skip application data already in the application data buffer */
        for (len = 0; len < application_data_len; len++) {
            if (apdu) {
                apdu[len] = application_data[len];
            }
        }
    }
    apdu_len += application_data_len;
    if (apdu) {
        apdu += application_data_len;
    }
    len = encode_closing_tag(apdu, 4);
    apdu_len += len;

    return apdu_len;
}

/**
 * @brief Encode the object property error for an acknowledge of a RPM.
 * @param apdu application data unit buffer for encoding, or NULL for length
 * @param error_class [in] Error Class
 * @param error_code [in] Error Code
 * @return number of bytes encoded
 */
int rpm_ack_encode_apdu_object_property_error(
    uint8_t *apdu, BACNET_ERROR_CLASS error_class, BACNET_ERROR_CODE error_code)
{
    int apdu_len = 0; /* total length of the apdu, return value */
    int len = 0;

    /* Tag 5: propertyAccessError */
    len = encode_opening_tag(apdu, 5);
    apdu_len += len;
    if (apdu) {
        apdu += len;
    }
    len = encode_application_enumerated(apdu, error_class);
    apdu_len += len;
    if (apdu) {
        apdu += len;
    }
    len = encode_application_enumerated(apdu, error_code);
    apdu_len += len;
    if (apdu) {
        apdu += len;
    }
    len = encode_closing_tag(apdu, 5);
    apdu_len += len;

    return apdu_len;
}

/**
 * @brief Encode the end tag for an acknowledge of a RPM.
 * @param apdu application data unit buffer for encoding, or NULL for length
 * @return number of bytes encoded
 */
int rpm_ack_encode_apdu_object_end(uint8_t *apdu)
{
    return encode_closing_tag(apdu, 1);
}

#if BACNET_SVC_RPM_A
/**
 * @brief Decode the ReadPropertyMultiple-Ack object-identifier
 * @param apdu application data unit buffer for decoding
 * @param apdu_size size of the application data unit buffer
 * @param object_type [out] The object type of the object identifier.
 * @param object_instance [out] The object instance of the object identifier.
 * @return Number of bytes decoded, or negative on error.
 */
int rpm_ack_decode_object_id(
    uint8_t *apdu,
    unsigned apdu_size,
    BACNET_OBJECT_TYPE *object_type,
    uint32_t *object_instance)
{
    int apdu_len = 0, len = 0;

    /* check for valid buffer and length */
    if (apdu && apdu_size) {
        /* Tag 0: objectIdentifier */
        len = bacnet_object_id_context_decode(
            &apdu[apdu_len], apdu_size - apdu_len, 0, object_type,
            object_instance);
        if (len <= 0) {
            return BACNET_STATUS_ERROR;
        }
        apdu_len += len;
        /* Tag 1: listOfResults */
        len = bacnet_is_opening_tag_number(
            &apdu[apdu_len], apdu_size - apdu_len, 1, &len);
        if (len <= 0) {
            return BACNET_STATUS_ERROR;
        }
        apdu_len += len;
    }

    return apdu_len;
}

/**
 * @brief Decode the ReadPropertyMultiple-Ack closing tag
 * @param apdu application data unit buffer for decoding
 * @param apdu_size size of the application data unit buffer
 * @return Number of bytes decoded, or negative on error.
 */
int rpm_ack_decode_object_end(uint8_t *apdu, unsigned apdu_size)
{
    int apdu_len = 0; /* total length of the apdu, return value */
    int len = 0;

    if (bacnet_is_closing_tag_number(apdu, apdu_size, 1, &len)) {
        apdu_len += len;
    } else {
        return BACNET_STATUS_ERROR;
    }

    return apdu_len;
}

/**
 * @brief Decode the ReadPropertyMultiple-Ack object property
 * @param apdu application data unit buffer for decoding
 * @param apdu_size size of the application data unit buffer
 * @param object_property [out] The object property of the object identifier.
 * @param array_index [out] The array index of the object property.
 * @return Number of bytes decoded, or negative on error.
 */
int rpm_ack_decode_object_property(
    uint8_t *apdu,
    unsigned apdu_size,
    BACNET_PROPERTY_ID *object_property,
    BACNET_ARRAY_INDEX *array_index)
{
    int len = 0;
    int apdu_len = 0;
    BACNET_UNSIGNED_INTEGER unsigned_value = 0; /* for decoding */
    uint32_t enum_value = 0; /* for decoding */

    /* check for valid pointers */
    if (!apdu) {
        return BACNET_STATUS_ERROR;
    }
    if (apdu_size == 0) {
        return BACNET_STATUS_ERROR;
    }
    /* Tag 2: propertyIdentifier */
    len = bacnet_enumerated_context_decode(
        &apdu[apdu_len], apdu_size - apdu_len, 2, &enum_value);
    if (len > 0) {
        apdu_len += len;
        if (object_property) {
            *object_property = (BACNET_PROPERTY_ID)enum_value;
        }
    } else {
        return BACNET_STATUS_ERROR;
    }
    /* Tag 3: Optional propertyArrayIndex */
    len = bacnet_unsigned_context_decode(
        &apdu[apdu_len], apdu_size - apdu_len, 3, &unsigned_value);
    if (len > 0) {
        apdu_len += len;
        if (unsigned_value < UINT32_MAX) {
            if (array_index) {
                *array_index = (BACNET_ARRAY_INDEX)unsigned_value;
            }
        } else {
            return BACNET_STATUS_ERROR;
        }
    } else {
        /* optional - assume ALL array elements */
        if (array_index) {
            *array_index = BACNET_ARRAY_ALL;
        }
    }

    return apdu_len;
}

/**
 * @brief Decode the RPM Ack and call the ReadProperty-ACK function to
 *  process each property value of the reply.
 *
 *  ReadAccessResult ::= SEQUENCE {
 *      object-identifier [0] BACnetObjectIdentifier,
 *      list-of-results [1] SEQUENCE OF SEQUENCE {
 *          property-identifier [2] BACnetPropertyIdentifier,
 *          property-array-index [3] Unsigned OPTIONAL,
 *          -- used only with array datatype
 *          -- if omitted with an array the entire
 *          -- array is referenced
 *          read-result CHOICE {
 *              property-value [4] ABSTRACT-SYNTAX.&Type,
 *              property-access-error [5] Error
 *          }
 *      }
 *  }
 *
 * @param apdu [in] Buffer of bytes received.
 * @param apdu_len [in] Count of valid bytes in the buffer.
 * @param device_id [in] The device ID of the device that replied.
 * @param rp_data [in] The data structure to be filled.
 * @param callback [in] The function to call for each property value.
 */
void rpm_ack_object_property_process(
    uint8_t *apdu,
    unsigned apdu_len,
    uint32_t device_id,
    BACNET_READ_PROPERTY_DATA *rp_data,
    read_property_ack_process callback)
{
    int len = 0;
    uint16_t application_data_len;
    uint32_t error_value = 0; /* decoded error value */

    if (!apdu) {
        return;
    }
    if (!rp_data) {
        return;
    }
    while (apdu_len) {
        /*  object-identifier [0] BACnetObjectIdentifier */
        /*      list-of-results [1] SEQUENCE OF SEQUENCE */
        len = rpm_ack_decode_object_id(
            apdu, apdu_len, &rp_data->object_type, &rp_data->object_instance);
        if (len <= 0) {
            /* malformed */
            return;
        }
        apdu_len -= len;
        apdu += len;
        while (apdu_len) {
            len = rpm_ack_decode_object_property(
                apdu, apdu_len, &rp_data->object_property,
                &rp_data->array_index);
            if (len <= 0) {
                /* malformed */
                return;
            }
            apdu_len -= len;
            apdu += len;
            if (bacnet_is_opening_tag_number(apdu, apdu_len, 4, &len)) {
                application_data_len =
                    bacnet_enclosed_data_length(apdu, apdu_len);
                /* propertyValue */
                apdu_len -= len;
                apdu += len;
                if (application_data_len) {
                    rp_data->application_data_len = application_data_len;
                    rp_data->application_data = apdu;
                    apdu_len -= application_data_len;
                    apdu += application_data_len;
                }
                if (bacnet_is_closing_tag_number(apdu, apdu_len, 4, &len)) {
                    apdu_len -= len;
                    apdu += len;
                } else {
                    /* malformed */
                    return;
                }
                rp_data->error_class = ERROR_CLASS_PROPERTY;
                rp_data->error_code = ERROR_CODE_SUCCESS;
                if (callback) {
                    callback(device_id, rp_data);
                }
            } else if (bacnet_is_opening_tag_number(apdu, apdu_len, 5, &len)) {
                apdu_len -= len;
                apdu += len;
                /* property-access-error */
                len = bacnet_enumerated_application_decode(
                    apdu, apdu_len, &error_value);
                if (len > 0) {
                    rp_data->error_class = (BACNET_ERROR_CLASS)error_value;
                    apdu_len -= len;
                    apdu += len;
                } else {
                    /* malformed */
                    return;
                }
                len = bacnet_enumerated_application_decode(
                    apdu, apdu_len, &error_value);
                if (len > 0) {
                    rp_data->error_code = (BACNET_ERROR_CODE)error_value;
                    apdu_len -= len;
                    apdu += len;
                } else {
                    /* malformed */
                    return;
                }
                if (bacnet_is_closing_tag_number(apdu, apdu_len, 5, &len)) {
                    apdu_len -= len;
                    apdu += len;
                } else {
                    /* malformed */
                    return;
                }
                if (callback) {
                    callback(device_id, rp_data);
                }
            }
        }
        len = rpm_decode_object_end(apdu, apdu_len);
        if (len) {
            apdu_len -= len;
            apdu += len;
        }
    }
}
#endif
