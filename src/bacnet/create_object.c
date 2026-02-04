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
 * @brief Decode one BACnetPropertyValue value
 *
 *  BACnetPropertyValue ::= SEQUENCE {
 *      property-identifier [0] BACnetPropertyIdentifier,
 *      property-array-index [1] Unsigned OPTIONAL,
 *      -- used only with array datatypes
 *      -- if omitted with an array the entire array is referenced
 *      property-value [2] ABSTRACT-SYNTAX.&Type,
 *      -- any datatype appropriate for the specified property
 *      priority [3] Unsigned (1..16) OPTIONAL
 *      -- used only when property is commandable
 *  }
 *
 * @param apdu Pointer to the buffer of encoded value
 * @param apdu_size Size of the buffer holding the encode value
 * @param value Pointer to the data used for decoding one value
 * @return number of bytes decoded or BACNET_STATUS_ERROR on error.
 */
int create_object_decode_initial_value(
    const uint8_t *apdu,
    uint32_t apdu_size,
    BACNET_CREATE_OBJECT_PROPERTY_VALUE *value)
{
    int len = 0;
    int apdu_len = 0;
    uint32_t enumerated_value = 0;
    uint32_t len_value_type = 0;
    BACNET_UNSIGNED_INTEGER unsigned_value = 0;
    BACNET_PROPERTY_ID property_identifier = PROP_ALL;
    int imax = 0;

    if (!apdu) {
        return BACNET_STATUS_ERROR;
    }
    /* property-identifier [0] BACnetPropertyIdentifier */
    len = bacnet_enumerated_context_decode(
        &apdu[apdu_len], apdu_size - apdu_len, 0, &enumerated_value);
    if (len > 0) {
        property_identifier = enumerated_value;
        if (value) {
            value->propertyIdentifier = property_identifier;
        }
        apdu_len += len;
    } else {
        return BACNET_STATUS_ERROR;
    }
    /* property-array-index [1] Unsigned OPTIONAL */
    if (bacnet_is_context_tag_number(
            &apdu[apdu_len], apdu_size - apdu_len, 1, &len, &len_value_type)) {
        apdu_len += len;
        len = bacnet_unsigned_decode(
            &apdu[apdu_len], apdu_size - apdu_len, len_value_type,
            &unsigned_value);
        if (len > 0) {
            if (unsigned_value > UINT32_MAX) {
                return BACNET_STATUS_ERROR;
            } else {
                apdu_len += len;
                if (value) {
                    value->propertyArrayIndex = unsigned_value;
                }
            }
        } else {
            return BACNET_STATUS_ERROR;
        }
    } else {
        if (value) {
            value->propertyArrayIndex = BACNET_ARRAY_ALL;
        }
    }
    /* property-value [2] ABSTRACT-SYNTAX.&Type */
    if (bacnet_is_opening_tag_number(
            &apdu[apdu_len], apdu_size - apdu_len, 2, &len)) {
        /* determine the length of the data within the tags */
        imax =
            bacnet_enclosed_data_length(&apdu[apdu_len], apdu_size - apdu_len);
        if (imax == BACNET_STATUS_ERROR) {
            return BACNET_STATUS_ERROR;
        }
        /* count the opening tag number length after finding enclosed length */
        apdu_len += len;
        if (imax > MAX_APDU) {
            /* not enough size in application_data to store the data chunk */
            return BACNET_STATUS_ERROR;
        } else if (value) {
            /* point to the data from the APDU */
            value->application_data = &apdu[apdu_len];
            value->application_data_len = imax;
        }
        /* add on the data length */
        apdu_len += imax;
        if (!bacnet_is_closing_tag_number(
                &apdu[apdu_len], apdu_size - apdu_len, 2, &len)) {
            return BACNET_STATUS_ERROR;
        }
        /* count the closing tag number length */
        apdu_len += len;
    } else {
        return BACNET_STATUS_ERROR;
    }
    /* priority [3] Unsigned (1..16) OPTIONAL */
    if (bacnet_is_context_tag_number(
            &apdu[apdu_len], apdu_size - apdu_len, 3, &len, &len_value_type)) {
        apdu_len += len;
        len = bacnet_unsigned_decode(
            &apdu[apdu_len], apdu_size - apdu_len, len_value_type,
            &unsigned_value);
        if (len > 0) {
            if (unsigned_value > UINT8_MAX) {
                return BACNET_STATUS_ERROR;
            } else {
                apdu_len += len;
                if (value) {
                    value->priority = unsigned_value;
                }
            }
        } else {
            return BACNET_STATUS_ERROR;
        }
    } else {
        if (value) {
            value->priority = BACNET_NO_PRIORITY;
        }
    }

    return apdu_len;
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
#if BACNET_CREATE_OBJECT_LIST_VALUES_ENABLED
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
#endif
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
    int imax = 0;

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
#if BACNET_CREATE_OBJECT_LIST_VALUES_ENABLED
            memmove(data->application_data, &apdu[apdu_len], (size_t)imax);
#endif
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

/**
 * @brief Initialize the created object with the provided initializers.
 * @param data [in] The Create Object data containing the initial values.
 * @param write_property [in] Function pointer to the Write Property handler.
 * @return true if successful, false on error.
 */
bool create_object_initializer_list_process(
    BACNET_CREATE_OBJECT_DATA *data, write_property_function write_property)
{
    BACNET_WRITE_PROPERTY_DATA wp_data = { 0 };
    BACNET_CREATE_OBJECT_PROPERTY_VALUE value = { 0 };
    int len = 0, apdu_len = 0;
    uint8_t *application_data = NULL;

    if (!data) {
        return false;
    }
    if (!write_property) {
        return false;
    }
    data->first_failed_element_number = 1;
    wp_data.object_type = data->object_type;
    wp_data.object_instance = data->object_instance;
    wp_data.error_class = ERROR_CLASS_PROPERTY;
    wp_data.error_code = ERROR_CODE_SUCCESS;
    while (data->application_data_len > apdu_len) {
#if BACNET_CREATE_OBJECT_LIST_VALUES_ENABLED
        application_data = &data->application_data[apdu_len];
#endif
        len = create_object_decode_initial_value(
            application_data, data->application_data_len - apdu_len, &value);
        if (len <= 0) {
            return false;
        }
        wp_data.object_property = value.propertyIdentifier;
        wp_data.array_index = value.propertyArrayIndex;
        memmove(
            &wp_data.application_data[0], value.application_data,
            (size_t)value.application_data_len);
        wp_data.application_data_len = value.application_data_len;
        wp_data.priority = value.priority;
        if (!write_property_bacnet_array_valid(&wp_data)) {
            return false;
        }
        /* write the property - use the provided function */
        if (!write_property(&wp_data)) {
            /* report the error */
            data->error_class = wp_data.error_class;
            data->error_code = wp_data.error_code;
            return false;
        }
        data->first_failed_element_number++;
        apdu_len += len;
    }

    return true;
}

/**
 * @brief Process the CreateObject request.
 * @param data [in,out] The Create Object data containing the request details.
 * @param object_supported [in] Flag indicating if the object type is supported.
 * @param object_exists [in] Flag indicating if the object already exists.
 * @param create_object [in] Function pointer to the Create Object handler.
 * @param delete_object [in] Function pointer to the Delete Object handler.
 * @param write_property [in] Function pointer to the Write Property handler.
 * @return true if successful, false on error.
 */
bool create_object_process(
    BACNET_CREATE_OBJECT_DATA *data,
    bool object_supported,
    bool object_exists,
    create_object_function create_object,
    delete_object_function delete_object,
    write_property_function write_property)
{
    bool status = false;
    uint32_t object_instance;

    if (!data) {
        return false;
    }
    if (!object_supported) {
        /* The device does not support the specified object type. */
        data->error_class = ERROR_CLASS_OBJECT;
        data->error_code = ERROR_CODE_UNSUPPORTED_OBJECT_TYPE;
    } else if (!create_object) {
        /*  The device supports the object type and may have
            sufficient space, but does not support the creation of the
            object for some other reason.*/
        data->error_class = ERROR_CLASS_OBJECT;
        data->error_code = ERROR_CODE_DYNAMIC_CREATION_NOT_SUPPORTED;
    } else if (object_exists) {
        /* The object being created already exists */
        data->error_class = ERROR_CLASS_OBJECT;
        data->error_code = ERROR_CODE_OBJECT_IDENTIFIER_ALREADY_EXISTS;
    } else {
        if (data->application_data_len) {
            /* The optional 'List of Initial Values' parameter is included */
            object_instance = create_object(data->object_instance);
            if (object_instance == BACNET_MAX_INSTANCE) {
                /* The device cannot allocate the space needed
                for the new object.*/
                data->error_class = ERROR_CLASS_RESOURCES;
                data->error_code = ERROR_CODE_NO_SPACE_FOR_OBJECT;
            } else if (write_property) {
                /* set the created object instance */
                data->object_instance = object_instance;
                /* If the optional 'List of Initial Values' parameter
                    is included, then all properties in the list shall
                    be initialized as indicated. */
                data->error_class = ERROR_CLASS_PROPERTY;
                data->error_code = ERROR_CODE_SUCCESS;
                if (!create_object_initializer_list_process(
                        data, write_property)) {
                    /* initialization failed - remove the object */
                    if (delete_object) {
                        (void)delete_object(object_instance);
                    }
                    if (data->error_code == ERROR_CODE_SUCCESS) {
                        /* A property specified by the Property_Identifier
                            in the List of Initial Values does not support
                            initialization during the CreateObject service.*/
                        data->error_code = ERROR_CODE_WRITE_ACCESS_DENIED;
                    }
                } else {
                    data->first_failed_element_number = 0;
                    status = true;
                }
            } else {
                /* cannot initialize without write property handler */
                data->error_code = ERROR_CODE_WRITE_ACCESS_DENIED;
            }
        } else {
            object_instance = create_object(data->object_instance);
            if (object_instance == BACNET_MAX_INSTANCE) {
                /* The device cannot allocate the space needed
                for the new object.*/
                data->error_class = ERROR_CLASS_RESOURCES;
                data->error_code = ERROR_CODE_NO_SPACE_FOR_OBJECT;
            } else {
                /* required by ACK */
                data->object_instance = object_instance;
                data->first_failed_element_number = 0;
                status = true;
            }
        }
    }

    return status;
}
