/**************************************************************************
 *
 * Copyright (C) 2011 Krzysztof Malorny <malornykrzysztof@gmail.com>
 * Contributions by Nikola Jelic 2011 <nikola.jelic@euroicc.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 *********************************************************************/
#include <stdint.h>
#include <string.h>
#include "bacnet/bacapp.h"
#include "bacnet/bacenum.h"
#include "bacnet/bacdcode.h"
#include "bacnet/bacdef.h"
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
    uint8_t *apdu, uint16_t apdu_len, BACNET_WRITE_PROPERTY_DATA *wp_data)
{
    uint8_t tag_number = 0;
    uint32_t len_value = 0;
    uint32_t object_instance = 0;
    BACNET_OBJECT_TYPE object_type = OBJECT_NONE;
    uint16_t len = 0;

    if (apdu && (apdu_len > 5)) {
        /* Context tag 0 - Object ID */
        len += decode_tag_number_and_value(&apdu[len], &tag_number, &len_value);
        if ((tag_number == 0) && (apdu_len > len)) {
            apdu_len -= len;
            if (apdu_len >= 4) {
                len += decode_object_id(
                    &apdu[len], &object_type, &object_instance);
                if (wp_data) {
                    wp_data->object_type = object_type;
                    wp_data->object_instance = object_instance;
                }
                apdu_len -= len;
            } else {
                if (wp_data) {
                    wp_data->error_code =
                        ERROR_CODE_REJECT_MISSING_REQUIRED_PARAMETER;
                }
                return BACNET_STATUS_REJECT;
            }
        } else {
            if (wp_data) {
                wp_data->error_code = ERROR_CODE_REJECT_INVALID_TAG;
            }
            return BACNET_STATUS_REJECT;
        }
        /* just test for the next tag - no need to decode it here */
        /* Context tag 1: sequence of BACnetPropertyValue */
        if (apdu_len && !decode_is_opening_tag_number(&apdu[len], 1)) {
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
 * @param apdu [in] The contents of the APDU buffer.
 * @param apdu_len [in] The length of the APDU buffer.
 * @param wp_data [out] The BACNET_WRITE_PROPERTY_DATA structure.
 *
 * @return Bytes decoded
 */
int wpm_decode_object_property(
    uint8_t *apdu, uint16_t apdu_len, BACNET_WRITE_PROPERTY_DATA *wp_data)
{
    uint8_t tag_number = 0;
    uint32_t len_value = 0;
    uint32_t enum_value = 0;
    BACNET_UNSIGNED_INTEGER unsigned_value = 0;
    int len = 0, i = 0, imax = 0;

    if ((apdu) && (apdu_len) && (wp_data)) {
        wp_data->array_index = BACNET_ARRAY_ALL;
        wp_data->priority = BACNET_MAX_PRIORITY;
        wp_data->application_data_len = 0;
        /* tag 0 - Property Identifier */
        len += decode_tag_number_and_value(&apdu[len], &tag_number, &len_value);
        if (tag_number == 0) {
            len += decode_enumerated(&apdu[len], len_value, &enum_value);
            wp_data->object_property = enum_value;
        } else {
            wp_data->error_code = ERROR_CODE_REJECT_INVALID_TAG;
            return BACNET_STATUS_REJECT;
        }

        /* tag 1 - Property Array Index - optional */
        len += decode_tag_number_and_value(&apdu[len], &tag_number, &len_value);
        if (tag_number == 1) {
            len += decode_unsigned(&apdu[len], len_value, &unsigned_value);
            wp_data->array_index = (uint32_t)unsigned_value;

            len += decode_tag_number_and_value(
                &apdu[len], &tag_number, &len_value);
        }
        /* tag 2 - Property Value */
        if ((tag_number == 2) && (decode_is_opening_tag(&apdu[len - 1]))) {
            len--;
            imax = bacapp_data_len(&apdu[len], (unsigned)(apdu_len - len),
                wp_data->object_property);
            len++;

            if (imax != BACNET_STATUS_ERROR) {
                /* copy application data, check max length */
                if (imax > (apdu_len - len)) {
                    imax = (apdu_len - len);
                }
                for (i = 0; i < imax; i++) {
                    wp_data->application_data[i] = apdu[len + i];
                }
                wp_data->application_data_len = imax;
                len += imax;
                if (len < apdu_len) {
                    len += decode_tag_number_and_value(
                        &apdu[len], &tag_number, &len_value);
                    /* closing tag 2 */
                    if ((tag_number != 2) &&
                        (decode_is_closing_tag(&apdu[len - 1]))) {
                        wp_data->error_code = ERROR_CODE_REJECT_INVALID_TAG;
                        return BACNET_STATUS_REJECT;
                    }
                } else {
                    wp_data->error_code = ERROR_CODE_REJECT_INVALID_TAG;
                    return BACNET_STATUS_REJECT;
                }
            } else {
                wp_data->error_code = ERROR_CODE_REJECT_INVALID_TAG;
                return BACNET_STATUS_REJECT;
            }
        } else {
            wp_data->error_code = ERROR_CODE_REJECT_INVALID_TAG;
            return BACNET_STATUS_REJECT;
        }

        /* tag 3 - Priority - optional */
        if (len < apdu_len) {
            len += decode_tag_number_and_value(
                &apdu[len], &tag_number, &len_value);
            if (tag_number == 3) {
                len += decode_unsigned(&apdu[len], len_value, &unsigned_value);
                wp_data->priority = (uint8_t)unsigned_value;
            } else {
                len--;
            }
        }
    } else {
        if (wp_data) {
            wp_data->error_code = ERROR_CODE_REJECT_MISSING_REQUIRED_PARAMETER;
        }
        return BACNET_STATUS_REJECT;
    }

    return len;
}

/** Init the APDU for encoding.
 *
 * @param apdu [in] The APDU buffer.
 * @param invoke_id [in] The ID of the saervice invoked.
 *
 * @return Bytes encoded, usually 4 on success.
 */
int wpm_encode_apdu_init(uint8_t *apdu, uint8_t invoke_id)
{
    int apdu_len = 0; /* total length of the apdu, return value */

    if (apdu) {
        apdu[0] = PDU_TYPE_CONFIRMED_SERVICE_REQUEST;
        apdu[1] = encode_max_segs_max_apdu(0, MAX_APDU);
        apdu[2] = invoke_id;
        apdu[3] = SERVICE_CONFIRMED_WRITE_PROP_MULTIPLE; /* service choice */
        apdu_len = 4;
    }

    return apdu_len;
}

/** Decode the very begin of an object in the APDU.
 *
 * @param apdu [in] The APDU buffer.
 * @param object_type [in] The object type to decode.
 * @param object_instance [in] The object instance.
 *
 * @return Bytes encoded.
 */
int wpm_encode_apdu_object_begin(
    uint8_t *apdu, BACNET_OBJECT_TYPE object_type, uint32_t object_instance)
{
    int apdu_len = 0; /* total length of the apdu, return value */

    if (apdu) {
        apdu_len =
            encode_context_object_id(&apdu[0], 0, object_type, object_instance);
        /* Tag 1: sequence of WriteAccessSpecification */
        apdu_len += encode_opening_tag(&apdu[apdu_len], 1);
    }

    return apdu_len;
}

/** Decode the very end of an object in the APDU.
 *
 * @param apdu [in] The APDU buffer.
 *
 * @return Bytes encoded.
 */
int wpm_encode_apdu_object_end(uint8_t *apdu)
{
    int apdu_len = 0; /* total length of the apdu, return value */

    if (apdu) {
        apdu_len = encode_closing_tag(&apdu[0], 1);
    }

    return apdu_len;
}

/** Encode the object property into the APDU.
 *
 * @param apdu [in] The APDU buffer.
 * @param wpdata [in] Pointer to the property data.
 *
 * @return Bytes encoded.
 */
int wpm_encode_apdu_object_property(
    uint8_t *apdu, BACNET_WRITE_PROPERTY_DATA *wpdata)
{
    int apdu_len = 0; /* total length of the apdu, return value */
    int len = 0;
    int imax;

    if (apdu) {
        apdu_len =
            encode_context_enumerated(&apdu[0], 0, wpdata->object_property);
        /* optional array index */
        if (wpdata->array_index != BACNET_ARRAY_ALL) {
            apdu_len += encode_context_unsigned(
                &apdu[apdu_len], 1, wpdata->array_index);
        }
        apdu_len += encode_opening_tag(&apdu[apdu_len], 2);
        imax = wpdata->application_data_len;
        if (imax > (MAX_APDU - apdu_len - 2)) {
            imax = (MAX_APDU - apdu_len - 2);
        }
        for (len = 0; len < imax; len++) {
            apdu[apdu_len] = wpdata->application_data[len];
            apdu_len++;
        }
        apdu_len += encode_closing_tag(&apdu[apdu_len], 2);
        if (wpdata->priority != BACNET_NO_PRIORITY) {
            if (apdu_len < MAX_APDU) {
                apdu_len += encode_context_unsigned(
                    &apdu[apdu_len], 3, wpdata->priority);
            }
        }
    }

    return apdu_len;
}

/** Encode the request into the APDU.
 *
 * @param apdu [in] The APDU buffer.
 * @param max_apdu [in] Maximum space in the buffer.
 * @param invoke_id [in] Invoked service ID.
 * @param write_access_data [in] Access data.
 *
 * @return Bytes encoded.
 */
int wpm_encode_apdu(uint8_t *apdu,
    size_t max_apdu,
    uint8_t invoke_id,
    BACNET_WRITE_ACCESS_DATA *write_access_data)
{
    int apdu_len = 0;
    int len = 0;
    size_t usize;
    BACNET_WRITE_ACCESS_DATA *wpm_object; /* current object */
    uint8_t apdu_temp[MAX_APDU]; /* temp for data before copy */
    BACNET_PROPERTY_VALUE *wpm_property; /* current property */
    BACNET_WRITE_PROPERTY_DATA
    wpdata; /* for compatibility with wpm_encode_apdu_object_property
               function */

    if (apdu) {
        len = wpm_encode_apdu_init(&apdu[0], invoke_id);
        apdu_len += len;

        wpm_object = write_access_data;

        while (wpm_object) {
            len = wpm_encode_apdu_object_begin(&apdu[apdu_len],
                wpm_object->object_type, wpm_object->object_instance);
            apdu_len += len;
            if (apdu_len >= (int)max_apdu) {
                break;
            }

            wpm_property = wpm_object->listOfProperties;

            while (wpm_property) {
                wpdata.object_property = wpm_property->propertyIdentifier;
                wpdata.array_index = wpm_property->propertyArrayIndex;
                wpdata.priority = wpm_property->priority;
                usize = (size_t)bacapp_encode_data(
                    &apdu_temp[0], &wpm_property->value);
                if (usize > sizeof(wpdata.application_data)) {
                    usize = sizeof(wpdata.application_data);
                }
                wpdata.application_data_len = (int)usize;
                memcpy(&wpdata.application_data[0], &apdu_temp[0], usize);
                len = wpm_encode_apdu_object_property(&apdu[apdu_len], &wpdata);
                apdu_len += len;
                if (apdu_len >= (int)max_apdu) {
                    break;
                }

                wpm_property = wpm_property->next;
            }

            len = wpm_encode_apdu_object_end(&apdu[apdu_len]);
            apdu_len += len;

            wpm_object = wpm_object->next;
        }
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
 * @param apdu [in] The APDU buffer.
 * @param invoke_id [in] Invoked service ID.
 * @param wp_data [in] Data of the invoked property.
 *
 * @return Bytes encoded.
 */
int wpm_error_ack_encode_apdu(
    uint8_t *apdu, uint8_t invoke_id, BACNET_WRITE_PROPERTY_DATA *wp_data)
{
    int len = 0;

    if (apdu) {
        apdu[len++] = PDU_TYPE_ERROR;
        apdu[len++] = invoke_id;
        apdu[len++] = SERVICE_CONFIRMED_WRITE_PROP_MULTIPLE;

        len += encode_opening_tag(&apdu[len], 0);
        len += encode_application_enumerated(&apdu[len], wp_data->error_class);
        len += encode_application_enumerated(&apdu[len], wp_data->error_code);
        len += encode_closing_tag(&apdu[len], 0);

        len += encode_opening_tag(&apdu[len], 1);
        len += encode_context_object_id(
            &apdu[len], 0, wp_data->object_type, wp_data->object_instance);
        len +=
            encode_context_enumerated(&apdu[len], 1, wp_data->object_property);

        if (wp_data->array_index != BACNET_ARRAY_ALL) {
            len += encode_context_unsigned(&apdu[len], 2, wp_data->array_index);
        }
        len += encode_closing_tag(&apdu[len], 1);
    }
    return len;
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
    uint8_t *apdu, uint16_t apdu_size, BACNET_WRITE_PROPERTY_DATA *wp_data)
{
    int len = 0, apdu_len = 0;
    uint8_t *apdu_offset = NULL;
    BACNET_ERROR_CLASS error_class = ERROR_CLASS_SERVICES;
    BACNET_ERROR_CODE error_code = ERROR_CODE_SUCCESS;
    BACNET_OBJECT_PROPERTY_REFERENCE value;

    if (apdu) {
        apdu_offset = apdu;
    }
    if (wp_data) {
        wp_data->error_class = ERROR_CLASS_SERVICES;
        wp_data->error_code = ERROR_CODE_REJECT_PARAMETER_OUT_OF_RANGE;
    }
    /* Context tag 0 - Error */
    if (apdu_size == 0) {
        return 0;
    }
    if (decode_is_opening_tag_number(apdu_offset, 0)) {
        len = 1;
        apdu_len += len;
        if (apdu) {
            apdu_offset = &apdu[apdu_len];
            if (apdu_size > len) {
                apdu_size -= len;
            } else {
                return 0;
            }
        }
    } else {
        return 0;
    }
    len = bacerror_decode_error_class_and_code(
        apdu_offset, apdu_size, &error_class, &error_code);
    if (len > 0) {
        if (wp_data) {
            wp_data->error_class = error_class;
            wp_data->error_code = error_code;
        }
        apdu_len += len;
        if (apdu) {
            apdu_offset = &apdu[apdu_len];
            if (apdu_size > len) {
                apdu_size -= len;
            } else {
                return 0;
            }
        }
    } else {
        return 0;
    }
    if (apdu_size == 0) {
        return 0;
    }
    if (decode_is_closing_tag_number(apdu_offset, 0)) {
        len = 1;
        apdu_len += len;
        if (apdu) {
            apdu_offset = &apdu[apdu_len];
            if (apdu_size > len) {
                apdu_size -= len;
            } else {
                return 0;
            }
        }
    } else {
        return 0;
    }
    /* Context tag 1 - BACnetObjectPropertyReference */
    if (apdu_size == 0) {
        return 0;
    }
    if (decode_is_opening_tag_number(apdu_offset, 1)) {
        len = 1;
        apdu_len += len;
        if (apdu) {
            apdu_offset = &apdu[apdu_len];
            if (apdu_size > len) {
                apdu_size -= len;
            } else {
                return 0;
            }
        }
    } else {
        return 0;
    }
    len = bacapp_decode_obj_property_ref(apdu_offset, apdu_size, &value);
    if (len > 0) {
        if (wp_data) {
            wp_data->object_instance = value.object_identifier.instance;
            wp_data->object_type = value.object_identifier.type;
            wp_data->object_property = value.property_identifier;
            wp_data->array_index = value.property_array_index;
        }
        apdu_len += len;
        if (apdu) {
            apdu_offset = &apdu[apdu_len];
            if (apdu_size > len) {
                apdu_size -= len;
            } else {
                return 0;
            }
        }
    } else {
        return 0;
    }
    if (apdu_size == 0) {
        return 0;
    }
    if (decode_is_closing_tag_number(apdu_offset, 1)) {
        len = 1;
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
    BACNET_WRITE_ACCESS_DATA *array,
    size_t size)
{
    size_t i = 0;

    for (i = 0; i < size; i++) {
        if (i > 0) {
            array[i - 1].next = &array[i];
        }
        array[i].next = NULL;
    }
}
