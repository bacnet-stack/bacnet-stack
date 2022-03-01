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
#include <stdint.h>
#include "bacnet/bacenum.h"
#include "bacnet/bacdcode.h"
#include "bacnet/bacdef.h"
#include "bacnet/wp.h"

/** @file wp.c  Encode/Decode BACnet Write Property APDUs  */

#if BACNET_SVC_WP_A
/** Initialize the APDU for encode service.
 *
 * @param apdu  Pointer to the buffer.
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
    int imax = 0; /* maximum application data length */

    if (apdu) {
        apdu[0] = PDU_TYPE_CONFIRMED_SERVICE_REQUEST;
        apdu[1] = encode_max_segs_max_apdu(0, MAX_APDU);
        apdu[2] = invoke_id;
        apdu[3] = SERVICE_CONFIRMED_WRITE_PROPERTY; /* service choice */
        apdu_len = 4;
        len = encode_context_object_id(
            &apdu[apdu_len], 0, wpdata->object_type, wpdata->object_instance);
        apdu_len += len;
        len = encode_context_enumerated(
            &apdu[apdu_len], 1, wpdata->object_property);
        apdu_len += len;
        /* optional array index; ALL is -1 which is assumed when missing */
        if (wpdata->array_index != BACNET_ARRAY_ALL) {
            len = encode_context_unsigned(
                &apdu[apdu_len], 2, wpdata->array_index);
            apdu_len += len;
        }
        /* propertyValue */
        len = encode_opening_tag(&apdu[apdu_len], 3);
        apdu_len += len;
        imax = wpdata->application_data_len;
        if (imax > (MAX_APDU - 2 /*closing*/ - apdu_len)) {
            imax = MAX_APDU - 2 - apdu_len;
        }
        for (len = 0; len < imax; len++) {
            apdu[apdu_len + len] = wpdata->application_data[len];
        }
        apdu_len += imax;
        len = encode_closing_tag(&apdu[apdu_len], 3);
        apdu_len += len;
        /* optional priority - 0 if not set, 1..16 if set */
        if (wpdata->priority != BACNET_NO_PRIORITY) {
            len = encode_context_unsigned(&apdu[apdu_len], 4, wpdata->priority);
            apdu_len += len;
        }
    }

    return apdu_len;
}
#endif

/** Decode the service request only
 *
 * FIXME: there could be various error messages returned
 * using unique values less than zero.
 *
 * @param apdu  Pointer to the buffer.
 * @param apdu_len  Valid bytes in the buffer
 * @param wpdata  Pointer to the write property data.
 *
 * @return Bytes encoded or a negative value as error.
 */
int wp_decode_service_request(
    uint8_t *apdu, unsigned apdu_len, BACNET_WRITE_PROPERTY_DATA *wpdata)
{
    int len = 0;
    int tag_len = 0;
    uint8_t tag_number = 0;
    uint32_t len_value_type = 0;
    BACNET_OBJECT_TYPE type = OBJECT_NONE; /* for decoding */
    uint32_t property = 0; /* for decoding */
    BACNET_UNSIGNED_INTEGER unsigned_value = 0;
    int i = 0; /* loop counter */
    int imax = 0; /* max application data length */

    /* check for value pointers */
    if (apdu_len && wpdata) {
        /* Tag 0: Object ID          */
        if (!decode_is_context_tag(&apdu[len++], 0)) {
            return -1;
        }
        len += decode_object_id(&apdu[len], &type, &wpdata->object_instance);
        wpdata->object_type = type;
        /* Tag 1: Property ID */
        len += decode_tag_number_and_value(
            &apdu[len], &tag_number, &len_value_type);
        if (tag_number != 1) {
            return -1;
        }
        len += decode_enumerated(&apdu[len], len_value_type, &property);
        wpdata->object_property = (BACNET_PROPERTY_ID)property;
        /* Tag 2: Optional Array Index */
        /* note: decode without incrementing len so we can check for opening tag
         */
        tag_len = decode_tag_number_and_value(
            &apdu[len], &tag_number, &len_value_type);
        if (tag_number == 2) {
            len += tag_len;
            len += decode_unsigned(&apdu[len], len_value_type, &unsigned_value);
            wpdata->array_index = unsigned_value;
        } else {
            wpdata->array_index = BACNET_ARRAY_ALL;
        }
        /* Tag 3: opening context tag */
        if (!decode_is_opening_tag_number(&apdu[len], 3)) {
            return -1;
        }
        /* determine the length of the data blob */
        imax = bacapp_data_len(
            &apdu[len], apdu_len - len, (BACNET_PROPERTY_ID)property);
        if (imax == BACNET_STATUS_ERROR) {
            return -2;
        }
        /* a tag number of 3 is not extended so only one octet */
        len++;
        /* copy the data from the APDU */
        if (imax > (MAX_APDU - len - 1 /*closing*/)) {
            imax = (MAX_APDU - len - 1);
        }
        for (i = 0; i < imax; i++) {
            wpdata->application_data[i] = apdu[len + i];
        }
        wpdata->application_data_len = imax;
        /* add on the data length */
        len += imax;
        if (!decode_is_closing_tag_number(&apdu[len], 3)) {
            return -2;
        }
        /* a tag number of 3 is not extended so only one octet */
        len++;
        /* Tag 4: optional Priority - assumed MAX if not explicitly set */
        wpdata->priority = BACNET_MAX_PRIORITY;
        if ((unsigned)len < apdu_len) {
            tag_len = decode_tag_number_and_value(
                &apdu[len], &tag_number, &len_value_type);
            if (tag_number == 4) {
                len += tag_len;
                len = decode_unsigned(
                    &apdu[len], len_value_type, &unsigned_value);
                if ((unsigned_value >= BACNET_MIN_PRIORITY) &&
                    (unsigned_value <= BACNET_MAX_PRIORITY)) {
                    wpdata->priority = (uint8_t)unsigned_value;
                } else {
                    return -5;
                }
            }
        }
    }

    return len;
}

/**
 * @brief simple validation of value tag for Write Property argument
 * @param wp_data - #BACNET_WRITE_PROPERTY_DATA data, including
 *  requested data and space for the reply, or error response.
 * @param value - #BACNET_APPLICATION_DATA_VALUE data, for the tag
 * @param expected_tag - the tag that is expected for this property value
 */
bool write_property_type_valid(
    BACNET_WRITE_PROPERTY_DATA * wp_data,
    BACNET_APPLICATION_DATA_VALUE * value,
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
 * @param expected_tag - the tag that is expected for this property value
 */
bool write_property_string_valid(
    BACNET_WRITE_PROPERTY_DATA * wp_data,
    BACNET_APPLICATION_DATA_VALUE * value,
    int len_max)
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
            } else if (characterstring_length(&value->type.Character_String) >
                (uint16_t)len_max) {
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
 * @param expected_tag - the tag that is expected for this property value
 */
bool write_property_empty_string_valid(
    BACNET_WRITE_PROPERTY_DATA * wp_data,
    BACNET_APPLICATION_DATA_VALUE * value,
    int len_max)
{
    bool valid = false;

    if (value && (value->tag == BACNET_APPLICATION_TAG_CHARACTER_STRING)) {
        if (characterstring_encoding(&value->type.Character_String) ==
            CHARACTER_ANSI_X34) {
            if (characterstring_length(&value->type.Character_String) >
                (uint16_t)len_max) {
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
