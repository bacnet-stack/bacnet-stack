/*####COPYRIGHTBEGIN####
 -------------------------------------------
 Copyright (C) 2005 Steve Karg

 This program is free software; you can redistribute it and/or
 modify it under the terms of the GNU General Public License
 as published by the Free Software Foundation; either version 2
 of the License, or (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program; if not, write to:
 The Free Software Foundation, Inc.
 59 Temple Place - Suite 330
 Boston, MA  02111-1307, USA.

 As a special exception, if other files instantiate templates or
 use macros or inline functions from this file, or you compile
 this file and link it with other works to produce a work based
 on this file, this file does not by itself cause the resulting
 work to be covered by the GNU General Public License. However
 the source code for this file must still be made available in
 accordance with section (3) of the GNU General Public License.

 This exception does not invalidate any other reasons why a work
 based on this file might be covered by the GNU General Public
 License.
 -------------------------------------------
####COPYRIGHTEND####*/
#include <stdbool.h>
#include <stdint.h>
#include "bacnet/bacenum.h"
#include "bacnet/bacdcode.h"
#include "bacnet/bacdef.h"
#include "bacnet/wp.h"

/** @file wp.c  Encode/Decode BACnet Write Property APDUs  */

#if BACNET_SVC_WP_A
/**
 * @brief Initialize the APDU for encode service.
 *
 *  WriteProperty-Request ::= SEQUENCE {
 *      object-identifier [0] BACnetObjectIdentifier,
 *      property-identifier [1] BACnetPropertyIdentifier,
 *      property-array-index [2] Unsigned OPTIONAL,
 *          -- used only with array datatype
 *          -- if omitted with an array the entire
 *          -- array is referenced
 *      property-value [3] ABSTRACT-SYNTAX.&Type,
 *      priority [4] Unsigned (1..16) OPTIONAL
 *          -– used only when property is commandable
 *  }
 *
 * @param apdu  Pointer to the buffer, or NULL for length
 * @param invoke_id  ID of service invoked.
 * @param wpdata  Pointer to the write property data.
 *
 * @return Bytes encoded
 */
int wp_encode_apdu(
    uint8_t *apdu, uint8_t invoke_id, BACNET_WRITE_PROPERTY_DATA *wpdata)
{
    int apdu_len = 0; /* total length of the apdu, return value */
    int len = 0; /* total length of the apdu, return value */

    if (!wpdata) {
        return BACNET_STATUS_ERROR;
    }
    if (apdu) {
        apdu[0] = PDU_TYPE_CONFIRMED_SERVICE_REQUEST;
        apdu[1] = encode_max_segs_max_apdu(0, MAX_APDU);
        apdu[2] = invoke_id;
        apdu[3] = SERVICE_CONFIRMED_WRITE_PROPERTY; /* service choice */
    }
    len = 4;
    apdu_len += len;
    if (apdu) {
        apdu += len;
    }
    len = encode_context_object_id(
        apdu, 0, wpdata->object_type, wpdata->object_instance);
    apdu_len += len;
    if (apdu) {
        apdu += len;
    }
    len = encode_context_enumerated(apdu, 1, wpdata->object_property);
    apdu_len += len;
    if (apdu) {
        apdu += len;
    }
    /* optional array index; ALL is -1 which is assumed when missing */
    if (wpdata->array_index != BACNET_ARRAY_ALL) {
        len = encode_context_unsigned(apdu, 2, wpdata->array_index);
        apdu_len += len;
        if (apdu) {
            apdu += len;
        }
    }
    /* propertyValue */
    len = encode_opening_tag(apdu, 3);
    apdu_len += len;
    if (apdu) {
        apdu += len;
    }
    for (len = 0; len < wpdata->application_data_len; len++) {
        if (apdu) {
            *apdu = wpdata->application_data[len];
            apdu += 1;
        }
        apdu_len++;
    }
    len = encode_closing_tag(apdu, 3);
    apdu_len += len;
    if (apdu) {
        apdu += len;
    }
    /* optional priority - 0 if not set, 1..16 if set */
    if (wpdata->priority != BACNET_NO_PRIORITY) {
        len = encode_context_unsigned(apdu, 4, wpdata->priority);
        apdu_len += len;
    }

    return apdu_len;
}
#endif

/**
 * @brief Decode the service request only
 *
 *  WriteProperty-Request ::= SEQUENCE {
 *      object-identifier [0] BACnetObjectIdentifier,
 *      property-identifier [1] BACnetPropertyIdentifier,
 *      property-array-index [2] Unsigned OPTIONAL,
 *          -- used only with array datatype
 *          -- if omitted with an array the entire
 *          -- array is referenced
 *      property-value [3] ABSTRACT-SYNTAX.&Type,
 *      priority [4] Unsigned (1..16) OPTIONAL
 *          -– used only when property is commandable
 *  }
 *
 * @param apdu  Pointer to the buffer.
 * @param apdu_size  Valid bytes in the buffer
 * @param wpdata  Pointer to the write property data.
 *
 * @return number of bytes decoded, or #BACNET_STATUS_ERROR
 */
int wp_decode_service_request(
    uint8_t *apdu, unsigned apdu_size, BACNET_WRITE_PROPERTY_DATA *wpdata)
{
    int len = 0;
    int apdu_len = 0;
    uint32_t instance = 0;
    BACNET_OBJECT_TYPE type = OBJECT_NONE; /* for decoding */
    uint32_t property = 0; /* for decoding */
    BACNET_UNSIGNED_INTEGER unsigned_value = 0;
    int i = 0; /* loop counter */
    int imax = 0; /* max application data length */

    /* check for value pointers */
    if (!apdu) {
        return BACNET_STATUS_ERROR;
    }
    /* object-identifier [0] BACnetObjectIdentifier */
    len = bacnet_object_id_context_decode(
        &apdu[apdu_len], apdu_size - apdu_len, 0, &type, &instance);
    if (len > 0) {
        if (instance > BACNET_MAX_INSTANCE) {
            return BACNET_STATUS_ERROR;
        }
        apdu_len += len;
        if (wpdata) {
            wpdata->object_type = type;
            wpdata->object_instance = instance;
        }
    } else {
        return BACNET_STATUS_ERROR;
    }
    /* property-identifier [1] BACnetPropertyIdentifier */
    len = bacnet_enumerated_context_decode(
        &apdu[apdu_len], apdu_size - apdu_len, 1, &property);
    if (len > 0) {
        apdu_len += len;
        if (wpdata) {
            wpdata->object_property = (BACNET_PROPERTY_ID)property;
        }
    } else {
        return BACNET_STATUS_ERROR;
    }
    /* property-array-index [2] Unsigned OPTIONAL */
    len = bacnet_unsigned_context_decode(
        &apdu[apdu_len], apdu_size - apdu_len, 2, &unsigned_value);
    if (len > 0) {
        apdu_len += len;
        if (wpdata) {
            wpdata->array_index = unsigned_value;
        }
    } else {
        /* wrong tag - skip apdu_len increment and go to next field */
        if (wpdata) {
            wpdata->array_index = BACNET_ARRAY_ALL;
        }
    }
    /* property-value [3] ABSTRACT-SYNTAX.&Type */
    if (!bacnet_is_opening_tag_number(
            &apdu[apdu_len], apdu_size - apdu_len, 3, &len)) {
        return BACNET_STATUS_ERROR;
    }
    /* determine the length of the data blob */
    imax = bacapp_data_len(
        &apdu[apdu_len], apdu_size - apdu_len, (BACNET_PROPERTY_ID)property);
    if (imax == BACNET_STATUS_ERROR) {
        return BACNET_STATUS_ERROR;
    }
    /* count the opening tag number length */
    apdu_len += len;
    /* copy the data from the APDU */
    if (imax > MAX_APDU) {
        /* not enough size in application_data to store the data chunk */
        return BACNET_STATUS_ERROR;
    } else if (wpdata) {
        for (i = 0; i < imax; i++) {
            wpdata->application_data[i] = apdu[apdu_len + i];
        }
        wpdata->application_data_len = imax;
    }
    /* add on the data length */
    apdu_len += imax;
    if (!bacnet_is_closing_tag_number(
            &apdu[apdu_len], apdu_size - apdu_len, 3, &len)) {
        return BACNET_STATUS_ERROR;
    }
    /* count the closing tag number length */
    apdu_len += len;
    /* priority [4] Unsigned (1..16) OPTIONAL */
    /* assumed MAX priority if not explicitly set */
    if (wpdata) {
        wpdata->priority = BACNET_MAX_PRIORITY;
    }
    if ((unsigned)apdu_len < apdu_size) {
        len = bacnet_unsigned_context_decode(
            &apdu[apdu_len], apdu_len - apdu_size, 4, &unsigned_value);
        if (len > 0) {
            apdu_len += len;
            if ((unsigned_value >= BACNET_MIN_PRIORITY) &&
                (unsigned_value <= BACNET_MAX_PRIORITY)) {
                if (wpdata) {
                    wpdata->priority = (uint8_t)unsigned_value;
                }
            } else {
                return BACNET_STATUS_ERROR;
            }
        } else {
            return BACNET_STATUS_ERROR;
        }
    }

    return apdu_len;
}

/**
 * @brief simple validation of value tag for Write Property argument
 * @param wp_data - #BACNET_WRITE_PROPERTY_DATA data, including
 *  requested data and space for the reply, or error response.
 * @param value - #BACNET_APPLICATION_DATA_VALUE data, for the tag
 * @param expected_tag - the tag that is expected for this property value
 * @return true if the expected tag matches the value tag
 */
bool write_property_type_valid(BACNET_WRITE_PROPERTY_DATA *wp_data,
    BACNET_APPLICATION_DATA_VALUE *value,
    uint8_t expected_tag)
{
    /* assume success */
    bool valid = true;

    if (value && (value->tag != expected_tag)) {
        valid = false;
        if (wp_data) {
            wp_data->error_class = ERROR_CLASS_PROPERTY;
            wp_data->error_code = ERROR_CODE_INVALID_DATA_TYPE;
        }
    }

    return (valid);
}

/**
 * @brief simple validation of character string value for Write Property
 * @param wp_data - #BACNET_WRITE_PROPERTY_DATA data, including
 *  requested data and space for the reply, or error response.
 * @param value - #BACNET_APPLICATION_DATA_VALUE data, for the tag
 * @param len_max - max length accepted for a character string, or 0=unchecked
 * @return true if the character string value is valid
 */
bool write_property_string_valid(BACNET_WRITE_PROPERTY_DATA *wp_data,
    BACNET_APPLICATION_DATA_VALUE *value,
    size_t len_max)
{
    bool valid = false;

    if (value && (value->tag == BACNET_APPLICATION_TAG_CHARACTER_STRING)) {
        if (characterstring_encoding(&value->type.Character_String) ==
            CHARACTER_ANSI_X34) {
            if (characterstring_length(&value->type.Character_String) == 0) {
                if (wp_data) {
                    wp_data->error_class = ERROR_CLASS_PROPERTY;
                    wp_data->error_code = ERROR_CODE_VALUE_OUT_OF_RANGE;
                }
            } else if (!characterstring_printable(
                           &value->type.Character_String)) {
                /* assumption: non-empty also means must be "printable" */
                if (wp_data) {
                    wp_data->error_class = ERROR_CLASS_PROPERTY;
                    wp_data->error_code = ERROR_CODE_VALUE_OUT_OF_RANGE;
                }
            } else if ((len_max > 0) && (characterstring_length(
                &value->type.Character_String) > len_max)) {
                if (wp_data) {
                    wp_data->error_class = ERROR_CLASS_RESOURCES;
                    wp_data->error_code = ERROR_CODE_NO_SPACE_TO_WRITE_PROPERTY;
                }
            } else {
                /* It's all good! */
                valid = true;
            }
        } else {
            if (wp_data) {
                wp_data->error_class = ERROR_CLASS_PROPERTY;
                wp_data->error_code = ERROR_CODE_CHARACTER_SET_NOT_SUPPORTED;
            }
        }
    } else {
        if (wp_data) {
            wp_data->error_class = ERROR_CLASS_PROPERTY;
            wp_data->error_code = ERROR_CODE_INVALID_DATA_TYPE;
        }
    }

    return (valid);
}

/**
 * @brief simple validation of character string value for Write Property
 *  for character strings which can be empty
 * @param wp_data - #BACNET_WRITE_PROPERTY_DATA data, including
 *  requested data and space for the reply, or error response.
 * @param value - #BACNET_APPLICATION_DATA_VALUE data, for the tag
 * @param len_max - max length accepted for a character string, or 0=unchecked
 * @return true if the character string value is valid
 */
bool write_property_empty_string_valid(BACNET_WRITE_PROPERTY_DATA *wp_data,
    BACNET_APPLICATION_DATA_VALUE *value,
    size_t len_max)
{
    bool valid = false;

    if (value && (value->tag == BACNET_APPLICATION_TAG_CHARACTER_STRING)) {
        if (characterstring_encoding(&value->type.Character_String) ==
            CHARACTER_ANSI_X34) {
            if ((len_max > 0) && (characterstring_length(
                &value->type.Character_String) > len_max)) {
                if (wp_data) {
                    wp_data->error_class = ERROR_CLASS_RESOURCES;
                    wp_data->error_code = ERROR_CODE_NO_SPACE_TO_WRITE_PROPERTY;
                }
            } else {
                /* It's all good! */
                valid = true;
            }
        } else {
            if (wp_data) {
                wp_data->error_class = ERROR_CLASS_PROPERTY;
                wp_data->error_code = ERROR_CODE_CHARACTER_SET_NOT_SUPPORTED;
            }
        }
    } else {
        if (wp_data) {
            wp_data->error_class = ERROR_CLASS_PROPERTY;
            wp_data->error_code = ERROR_CODE_INVALID_DATA_TYPE;
        }
    }

    return (valid);
}
