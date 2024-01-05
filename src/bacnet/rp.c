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
#include "bacnet/rp.h"

/** @file rp.c  Encode/Decode Read Property and RP ACKs */

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
 * @param apdu_len  Count of valid bytes in the buffer.
 * @param rpdata  Pointer to the property data to be encoded.
 *
 * @return Bytes decoded or zero on error.
 */
int rp_decode_service_request(
    uint8_t *apdu, unsigned apdu_len, BACNET_READ_PROPERTY_DATA *rpdata)
{
    unsigned len = 0;
    uint8_t tag_number = 0;
    uint32_t len_value_type = 0;
    BACNET_OBJECT_TYPE type = OBJECT_NONE; /* for decoding */
    uint32_t property = 0; /* for decoding */
    BACNET_UNSIGNED_INTEGER unsigned_value = 0; /* for decoding */

    /* check for value pointers */
    if (rpdata) {
        /* Must have at least 2 tags, an object id and a property identifier
         * of at least 1 byte in length to have any chance of parsing */
        if (apdu_len < 7) {
            rpdata->error_code = ERROR_CODE_REJECT_MISSING_REQUIRED_PARAMETER;
            return BACNET_STATUS_REJECT;
        }

        /* Tag 0: Object ID          */
        if (!decode_is_context_tag(&apdu[len++], 0)) {
            rpdata->error_code = ERROR_CODE_REJECT_INVALID_TAG;
            return BACNET_STATUS_REJECT;
        }
        len += decode_object_id(&apdu[len], &type, &rpdata->object_instance);
        rpdata->object_type = type;
        /* Tag 1: Property ID */
        len += decode_tag_number_and_value(
            &apdu[len], &tag_number, &len_value_type);
        if (tag_number != 1) {
            rpdata->error_code = ERROR_CODE_REJECT_INVALID_TAG;
            return BACNET_STATUS_REJECT;
        }
        len += decode_enumerated(&apdu[len], len_value_type, &property);
        rpdata->object_property = (BACNET_PROPERTY_ID)property;
        /* Tag 2: Optional Array Index */
        if (len < apdu_len) {
            len += decode_tag_number_and_value(
                &apdu[len], &tag_number, &len_value_type);
            if ((tag_number == 2) && (len < apdu_len)) {
                len += decode_unsigned(
                    &apdu[len], len_value_type, &unsigned_value);
                rpdata->array_index = (BACNET_ARRAY_INDEX)unsigned_value;
            } else {
                rpdata->error_code = ERROR_CODE_REJECT_INVALID_TAG;
                return BACNET_STATUS_REJECT;
            }
        } else {
            rpdata->array_index = BACNET_ARRAY_ALL;
        }
    }

    if (len < apdu_len) {
        /* If something left over now, we have an invalid request */
        if (rpdata) {
            rpdata->error_code = ERROR_CODE_REJECT_TOO_MANY_ARGUMENTS;
        }
        return BACNET_STATUS_REJECT;
    }

    return (int)len;
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
 * @param apdu  Pointer to the buffer for encoding.
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
 * @param apdu_len [in] The total length of the apdu.
 * @param rpdata [out] The structure holding the partially decoded result.
 * @return Number of decoded bytes (could be less than apdu_len),
 * 			or -1 on decoding error.
 */
int rp_ack_decode_service_request(uint8_t *apdu,
    int apdu_len, /* total length of the apdu */
    BACNET_READ_PROPERTY_DATA *rpdata)
{
    uint8_t tag_number = 0;
    uint32_t len_value_type = 0;
    int tag_len = 0; /* length of tag decode */
    int len = 0; /* total length of decodes */
    BACNET_OBJECT_TYPE object_type = OBJECT_NONE; /* object type */
    uint32_t property = 0; /* for decoding */
    BACNET_UNSIGNED_INTEGER unsigned_value = 0; /* for decoding */

    /* Check basics. */
    if (apdu && (apdu_len >= 8 /*minimum*/)) {
        /* Tag 0: Object ID */
        if (!decode_is_context_tag(&apdu[0], 0)) {
            return -1;
        }
        len = 1;
        len += decode_object_id(
            &apdu[len], &object_type, &rpdata->object_instance);
        rpdata->object_type = object_type;
        /* Tag 1: Property ID */
        if (len >= apdu_len) {
            return -1;
        }
        len += decode_tag_number_and_value(
            &apdu[len], &tag_number, &len_value_type);
        if (tag_number != 1) {
            return -1;
        }
        if (len >= apdu_len) {
            return -1;
        }
        len += decode_enumerated(&apdu[len], len_value_type, &property);
        rpdata->object_property = (BACNET_PROPERTY_ID)property;
        /* Tag 2: Optional Array Index */
        if (len >= apdu_len) {
            return -1;
        }
        tag_len = decode_tag_number_and_value(
            &apdu[len], &tag_number, &len_value_type);
        if (tag_number == 2) {
            len += tag_len;
            len += decode_unsigned(&apdu[len], len_value_type, &unsigned_value);
            rpdata->array_index = (BACNET_ARRAY_INDEX)unsigned_value;
        } else {
            rpdata->array_index = BACNET_ARRAY_ALL;
        }
        /* Tag 3: opening context tag */
        if (len >= apdu_len) {
            return -1;
        }
        if (decode_is_opening_tag_number(&apdu[len], 3)) {
            /* a tag number of 3 is not extended so only one octet */
            len++;
            /* don't decode the application tag number or its data here */
            rpdata->application_data = &apdu[len];
            /* Just to ensure we do not create a wrapped over value here. */
            if (len < apdu_len) {
                rpdata->application_data_len =
                    apdu_len - len - 1 /*closing tag */;
            } else {
                rpdata->application_data_len = 0;
            }
            /* len includes the data and the closing tag */
            len = apdu_len;
        } else {
            return -1;
        }
    } else {
        return -1;
    }

    return len;
}
#endif
