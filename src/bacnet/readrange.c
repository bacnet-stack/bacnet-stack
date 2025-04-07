/**
 * @file
 * @brief BACnet ReadRange-Request encode and decode helper functions
 * @author Peter Mc Shane <petermcs@users.sourceforge.net>
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date 2009
 * @copyright SPDX-License-Identifier: GPL-2.0-or-later WITH GCC-exception-2.0
 */
#include <stdint.h>
/* BACnet Stack defines - first */
#include "bacnet/bacdef.h"
/* BACnet Stack API */
#include "bacnet/bacdcode.h"
#include "bacnet/readrange.h"

/*
 * ReadRange-Request ::= SEQUENCE {
 *     objectIdentifier   [0] BACnetObjectIdentifier,
 *     propertyIdentifier [1] BACnetPropertyIdentifier,
 *     propertyArrayIndex [2] Unsigned OPTIONAL, -- used only with array
 *     datatype range CHOICE {
 *         byPosition [3] SEQUENCE {
 *             referenceIndex Unsigned,
 *             count          INTEGER
 *         },
 *         -- context tag 4 is deprecated
 *         -- context tag 5 is deprecated
 *         bySequenceNumber [6] SEQUENCE {
 *             referenceIndex Unsigned,
 *             count          INTEGER
 *         },
 *         byTime [7] SEQUENCE {
 *             referenceTime BACnetDateTime,
 *             count         INTEGER
 *         }
 *     } OPTIONAL
 * }
 */

/**
 * @brief Encode ReadRange-Request APDU
 * @param apdu  Pointer to the buffer, or NULL for length
 * @param data  Pointer to the data to encode.
 * @return number of bytes encoded, or zero on error.
 */
int read_range_encode(uint8_t *apdu, const BACNET_READ_RANGE_DATA *data)
{
    int len = 0; /* length of each encoding */
    int apdu_len = 0; /* total length of the apdu, return value */

    if (!data) {
        return 0;
    }
    /* objectIdentifier   [0] BACnetObjectIdentifier */
    len = encode_context_object_id(
        apdu, 0, data->object_type, data->object_instance);
    apdu_len += len;
    if (apdu) {
        apdu += len;
    }
    /* propertyIdentifier [1] BACnetPropertyIdentifier */
    len = encode_context_enumerated(apdu, 1, data->object_property);
    apdu_len += len;
    if (apdu) {
        apdu += len;
    }
    /* propertyArrayIndex [2] Unsigned OPTIONAL */
    if (data->array_index != BACNET_ARRAY_ALL) {
        len = encode_context_unsigned(apdu, 2, data->array_index);
        apdu_len += len;
        if (apdu) {
            apdu += len;
        }
    }
    switch (data->RequestType) {
        case RR_BY_POSITION:
            /* byPosition [3] SEQUENCE */
            len = encode_opening_tag(apdu, 3);
            apdu_len += len;
            if (apdu) {
                apdu += len;
            }
            len = encode_application_unsigned(apdu, data->Range.RefIndex);
            apdu_len += len;
            if (apdu) {
                apdu += len;
            }
            len = encode_application_signed(apdu, data->Count);
            apdu_len += len;
            if (apdu) {
                apdu += len;
            }
            len = encode_closing_tag(apdu, 3);
            apdu_len += len;
            break;
        case RR_BY_SEQUENCE:
            /* bySequenceNumber [6] SEQUENCE */
            len = encode_opening_tag(apdu, 6);
            apdu_len += len;
            if (apdu) {
                apdu += len;
            }
            len = encode_application_unsigned(apdu, data->Range.RefSeqNum);
            apdu_len += len;
            if (apdu) {
                apdu += len;
            }
            len = encode_application_signed(apdu, data->Count);
            apdu_len += len;
            if (apdu) {
                apdu += len;
            }
            len = encode_closing_tag(apdu, 6);
            apdu_len += len;
            break;
        case RR_BY_TIME:
            /* byTime [7] SEQUENCE */
            len = encode_opening_tag(apdu, 7);
            apdu_len += len;
            if (apdu) {
                apdu += len;
            }
            len = encode_application_date(apdu, &data->Range.RefTime.date);
            apdu_len += len;
            if (apdu) {
                apdu += len;
            }
            len = encode_application_time(apdu, &data->Range.RefTime.time);
            apdu_len += len;
            if (apdu) {
                apdu += len;
            }
            len = encode_application_signed(apdu, data->Count);
            apdu_len += len;
            if (apdu) {
                apdu += len;
            }
            len = encode_closing_tag(apdu, 7);
            apdu_len += len;
            break;
        case RR_READ_ALL:
            /* read the whole list - omit the range parameter */
            break;
        default:
            break;
    }

    return apdu_len;
}

/**
 * @brief Encode ReadRange-Request service APDU
 * @param apdu  Pointer to the buffer for encoding into
 * @param apdu_size number of bytes available in the buffer
 * @param data  Pointer to the service data used for encoding values
 * @return number of bytes encoded, or zero if unable to encode or too large
 */
size_t read_range_request_encode(
    uint8_t *apdu, size_t apdu_size, const BACNET_READ_RANGE_DATA *data)
{
    size_t apdu_len = 0; /* total length of the apdu, return value */

    apdu_len = read_range_encode(NULL, data);
    if (apdu_len > apdu_size) {
        apdu_len = 0;
    } else {
        apdu_len = read_range_encode(apdu, data);
    }

    return apdu_len;
}

/**
 *  Build a ReadRange request packet.
 *
 *  @param apdu  Pointer to the APDU buffer.
 *  @param invoke_id  Invoke ID
 *  @param rrdata  Pointer to the data used for encoding.
 *
 *  @return Bytes encoded.
 */
int rr_encode_apdu(
    uint8_t *apdu, uint8_t invoke_id, const BACNET_READ_RANGE_DATA *data)
{
    int len = 0; /* length of each encoding */
    int apdu_len = 0; /* total length of the apdu, return value */

    if (apdu) {
        apdu[0] = PDU_TYPE_CONFIRMED_SERVICE_REQUEST;
        apdu[1] = encode_max_segs_max_apdu(0, MAX_APDU);
        apdu[2] = invoke_id;
        apdu[3] = SERVICE_CONFIRMED_READ_RANGE; /* service choice */
    }
    len = 4;
    apdu_len += len;
    if (apdu) {
        apdu += len;
    }
    len = read_range_encode(apdu, data);
    apdu_len += len;

    return apdu_len;
}

/**
 * Decode the received ReadRange request
 *
 *  @param apdu Pointer to the APDU buffer.
 *  @param apdu_size number of bytes in the APDU buffer.
 *  @param data Pointer to the data filled while decoding.
 *  @return Bytes decoded, or #BACNET_STATUS_ERROR
 */
int rr_decode_service_request(
    const uint8_t *apdu, unsigned apdu_size, BACNET_READ_RANGE_DATA *data)
{
    int len = 0, apdu_len = 0;
    uint32_t value32 = 0;
    int32_t signed_value = 0;
    BACNET_OBJECT_TYPE object_type = OBJECT_NONE;
    uint32_t enum_value = 0;
    BACNET_UNSIGNED_INTEGER unsigned_value = 0;
    BACNET_DATE *bdate = NULL;
    BACNET_TIME *btime = NULL;

    /* check for value pointers */
    if (!apdu) {
        return BACNET_STATUS_ERROR;
    }
    /* objectIdentifier   [0] BACnetObjectIdentifier */
    len = bacnet_object_id_context_decode(
        &apdu[apdu_len], apdu_size - apdu_len, 0, &object_type, &value32);
    if (len > 0) {
        apdu_len += len;
        if (data) {
            data->object_type = object_type;
            data->object_instance = value32;
        }
    } else {
        return BACNET_STATUS_ERROR;
    }
    /* propertyIdentifier [1] BACnetPropertyIdentifier */
    len = bacnet_enumerated_context_decode(
        &apdu[apdu_len], apdu_size - apdu_len, 1, &enum_value);
    if (len > 0) {
        apdu_len += len;
        if (data) {
            data->object_property = (BACNET_PROPERTY_ID)enum_value;
            data->Overhead = RR_OVERHEAD; /* Start with the fixed overhead */
        }
    } else {
        return BACNET_STATUS_ERROR;
    }
    /* propertyArrayIndex [2] Unsigned OPTIONAL */
    len = bacnet_unsigned_context_decode(
        &apdu[apdu_len], apdu_size - apdu_len, 2, &unsigned_value);
    if (len > 0) {
        apdu_len += len;
        if (data) {
            data->array_index = (BACNET_ARRAY_INDEX)unsigned_value;
            data->Overhead += RR_INDEX_OVERHEAD;
        }
    } else if (len == 0) {
        /* OPTIONAL missing - skip adding len */
        if (data) {
            data->array_index = BACNET_ARRAY_ALL;
        }
    } else {
        return BACNET_STATUS_ERROR;
    }
    if (bacnet_is_opening_tag_number(
            &apdu[apdu_len], apdu_size - apdu_len, 3, &len)) {
        /*
            byPosition [3] SEQUENCE {
                referenceIndex  Unsigned,
                count           INTEGER
            }
        */
        apdu_len += len;
        if (data) {
            data->RequestType = RR_BY_POSITION;
        }
        len = bacnet_unsigned_application_decode(
            &apdu[apdu_len], apdu_size - apdu_len, &unsigned_value);
        if (len > 0) {
            apdu_len += len;
            if (data) {
                data->Range.RefIndex = (uint32_t)unsigned_value;
            }
        } else {
            return BACNET_STATUS_ERROR;
        }
        len = bacnet_signed_application_decode(
            &apdu[apdu_len], apdu_size - apdu_len, &signed_value);
        if (len > 0) {
            apdu_len += len;
            if (data) {
                data->Count = signed_value;
            }
        } else {
            return BACNET_STATUS_ERROR;
        }
        if (bacnet_is_closing_tag_number(
                &apdu[apdu_len], apdu_size - apdu_len, 3, &len)) {
            apdu_len += len;
        } else {
            return BACNET_STATUS_ERROR;
        }
    } else if (bacnet_is_opening_tag_number(
                   &apdu[apdu_len], apdu_size - apdu_len, 6, &len)) {
        /*
            bySequenceNumber [6] SEQUENCE {
                referenceIndex  Unsigned,
                count           INTEGER
            }
        */
        apdu_len += len;
        if (data) {
            data->RequestType = RR_BY_SEQUENCE;
        }
        len = bacnet_unsigned_application_decode(
            &apdu[apdu_len], apdu_size - apdu_len, &unsigned_value);
        if (len > 0) {
            apdu_len += len;
            if (data) {
                data->Range.RefSeqNum = (uint32_t)unsigned_value;
                data->Overhead += RR_1ST_SEQ_OVERHEAD;
            }
        } else {
            return BACNET_STATUS_ERROR;
        }
        len = bacnet_signed_application_decode(
            &apdu[apdu_len], apdu_size - apdu_len, &signed_value);
        if (len > 0) {
            apdu_len += len;
            if (data) {
                data->Count = signed_value;
            }
        } else {
            return BACNET_STATUS_ERROR;
        }
        if (bacnet_is_closing_tag_number(
                &apdu[apdu_len], apdu_size - apdu_len, 6, &len)) {
            apdu_len += len;
        } else {
            return BACNET_STATUS_ERROR;
        }
    } else if (bacnet_is_opening_tag_number(
                   &apdu[apdu_len], apdu_size - apdu_len, 7, &len)) {
        /*
            byTime [7] SEQUENCE {
                referenceTime   BACnetDateTime,
                count           INTEGER
            }
        */
        apdu_len += len;
        if (data) {
            data->RequestType = RR_BY_TIME;
            bdate = &data->Range.RefTime.date;
            btime = &data->Range.RefTime.time;
        }
        len = bacnet_date_application_decode(
            &apdu[apdu_len], apdu_size - apdu_len, bdate);
        if (len > 0) {
            apdu_len += len;
        } else {
            return BACNET_STATUS_ERROR;
        }
        len = bacnet_time_application_decode(
            &apdu[apdu_len], apdu_size - apdu_len, btime);
        if (len > 0) {
            apdu_len += len;
        } else {
            return BACNET_STATUS_ERROR;
        }
        len = bacnet_signed_application_decode(
            &apdu[apdu_len], apdu_size - apdu_len, &signed_value);
        if (len > 0) {
            apdu_len += len;
            if (data) {
                data->Count = signed_value;
            }
        } else {
            return BACNET_STATUS_ERROR;
        }
        if (bacnet_is_closing_tag_number(
                &apdu[apdu_len], apdu_size - apdu_len, 7, &len)) {
            apdu_len += len;
        } else {
            return BACNET_STATUS_ERROR;
        }
    } else {
        /* OPTIONAL range missing - skip adding len */
        if (data) {
            data->RequestType = RR_READ_ALL;
        }
    }

    return apdu_len;
}

/*
 * ReadRange-ACK ::= SEQUENCE {
 *      objectIdentifier    [0] BACnetObjectIdentifier,
 *      propertyIdentifier  [1] BACnetPropertyIdentifier,
 *      propertyArrayIndex  [2] Unsigned OPTIONAL,
 *      -- used only with array datatype
 *      resultFlags         [3] BACnetResultFlags,
 *      itemCount           [4] Unsigned,
 *      itemData            [5] SEQUENCE OF ABSTRACT-SYNTAX.&TYPE,
 *      firstSequenceNumber [6] Unsigned32 OPTIONAL
 *      -- used only if 'Item Count' > 0 and
 *      -- the request was either of type 'By Sequence Number' or 'By Time'
 * }
 */

/**
 * @brief Encode ReadRange-ACK service APDU
 * @param apdu  Pointer to the buffer, or NULL for length
 * @param data  Pointer to the data to encode.
 * @return number of bytes encoded, or zero on error.
 */
int readrange_ack_encode(uint8_t *apdu, const BACNET_READ_RANGE_DATA *data)
{
    int apdu_len = 0; /* total length of the apdu, return value */
    int len = 0;

    if (!data) {
        return 0;
    }
    len = encode_context_object_id(
        apdu, 0, data->object_type, data->object_instance);
    apdu_len += len;
    if (apdu) {
        apdu += len;
    }
    len = encode_context_enumerated(apdu, 1, data->object_property);
    apdu_len += len;
    if (apdu) {
        apdu += len;
    }
    /* context 2 array index is optional */
    if (data->array_index != BACNET_ARRAY_ALL) {
        len = encode_context_unsigned(apdu, 2, data->array_index);
        apdu_len += len;
        if (apdu) {
            apdu += len;
        }
    }
    /* Context 3 BACnet Result Flags */
    len = encode_context_bitstring(apdu, 3, &data->ResultFlags);
    apdu_len += len;
    if (apdu) {
        apdu += len;
    }
    /* Context 4 Item Count */
    len = encode_context_unsigned(apdu, 4, data->ItemCount);
    apdu_len += len;
    if (apdu) {
        apdu += len;
    }
    /* Context 5 Property list - reading the standard it looks like an
     * empty list still requires an opening and closing tag as the
     * tagged parameter is not optional
     */
    len = encode_opening_tag(apdu, 5);
    apdu_len += len;
    if (apdu) {
        apdu += len;
    }
    if (data->application_data_len > 0) {
        for (len = 0; len < data->application_data_len; len++) {
            if (apdu) {
                apdu[len] = data->application_data[len];
            }
        }
        apdu_len += len;
        if (apdu) {
            apdu += len;
        }
    }
    len = encode_closing_tag(apdu, 5);
    apdu_len += len;
    if (apdu) {
        apdu += len;
    }
    if ((data->ItemCount != 0) && (data->RequestType != RR_BY_POSITION) &&
        (data->RequestType != RR_READ_ALL)) {
        /* Context 6 Sequence number of first item */
        len = encode_context_unsigned(apdu, 6, data->FirstSequence);
        apdu_len += len;
    }

    return apdu_len;
}

/**
 * @brief Encode the ReadRange-ACK service
 * @param apdu  Pointer to the buffer for encoding into, or NULL for length
 * @param apdu_size number of bytes available in the buffer
 * @param data  Pointer to the service data to be encoded
 * @return number of bytes encoded, or zero if unable to encode or too large
 */
size_t readrange_ack_service_encode(
    uint8_t *apdu, size_t apdu_size, const BACNET_READ_RANGE_DATA *data)
{
    size_t apdu_len = 0; /* total length of the apdu, return value */

    apdu_len = readrange_ack_encode(NULL, data);
    if (apdu_len > apdu_size) {
        apdu_len = 0;
    } else {
        apdu_len = readrange_ack_encode(apdu, data);
    }

    return apdu_len;
}

/**
 * Build a ReadRange response packet
 *
 * @param apdu  Pointer to the buffer.
 * @param invoke_id original invoke id for request
 * @param data  Pointer to the property data to be encoded
 * @return number of bytes encoded
 */
int rr_ack_encode_apdu(
    uint8_t *apdu, uint8_t invoke_id, const BACNET_READ_RANGE_DATA *data)
{
    int apdu_len = 0; /* total length of the apdu, return value */
    int len = 0;

    if (apdu) {
        apdu[0] = PDU_TYPE_COMPLEX_ACK; /* complex ACK service */
        apdu[1] = invoke_id; /* original invoke id from request */
        apdu[2] = SERVICE_CONFIRMED_READ_RANGE; /* service choice */
    }
    len = 3;
    apdu_len += len;
    if (apdu) {
        apdu += len;
    }
    len = readrange_ack_encode(apdu, data);
    apdu_len += len;

    return apdu_len;
}

/**
 * Decode the received ReadRange response
 *
 *  @param apdu Pointer to the APDU buffer.
 *  @param apdu_size Number of bytes in the APDU buffer.
 *  @param data Pointer to the data filled while decoding (can be NULL).
 *  @return number of bytes decoded, or #BACNET_STATUS_ERROR
 */
int rr_ack_decode_service_request(
    uint8_t *apdu, int apdu_size, BACNET_READ_RANGE_DATA *data)
{
    int apdu_len = 0;
    int len = 0;
    int data_len = 0;
    BACNET_OBJECT_TYPE object_type = OBJECT_NONE;
    uint32_t value32 = 0;
    BACNET_UNSIGNED_INTEGER unsigned_value;
    BACNET_BIT_STRING *bitstring = NULL;

    /* Check apdu_len against the len during decode. */
    if (!apdu) {
        return BACNET_STATUS_ERROR;
    }
    /* objectIdentifier    [0] BACnetObjectIdentifier */
    len = bacnet_object_id_context_decode(
        &apdu[apdu_len], apdu_size - apdu_len, 0, &object_type, &value32);
    if (len > 0) {
        apdu_len += len;
        if (data) {
            data->object_type = object_type;
            data->object_instance = value32;
        }
    } else {
        return BACNET_STATUS_ERROR;
    }
    /* propertyIdentifier [1] BACnetPropertyIdentifier */
    len = bacnet_enumerated_context_decode(
        &apdu[apdu_len], apdu_size - apdu_len, 1, &value32);
    if (len > 0) {
        apdu_len += len;
        if (data) {
            data->object_property = (BACNET_PROPERTY_ID)value32;
        }
    } else {
        return BACNET_STATUS_ERROR;
    }
    /* propertyArrayIndex [2] Unsigned OPTIONAL */
    len = bacnet_unsigned_context_decode(
        &apdu[apdu_len], apdu_size - apdu_len, 2, &unsigned_value);
    if (len > 0) {
        apdu_len += len;
        if (data) {
            data->array_index = (BACNET_ARRAY_INDEX)unsigned_value;
        }
    } else if (len == 0) {
        /* OPTIONAL missing - skip adding len */
        if (data) {
            data->array_index = BACNET_ARRAY_ALL;
        }
    } else {
        return BACNET_STATUS_ERROR;
    }
    /* resultFlags [3] BACnetResultFlags */
    if (data) {
        bitstring = &data->ResultFlags;
    }
    len = bacnet_bitstring_context_decode(
        &apdu[apdu_len], apdu_size - apdu_len, 3, bitstring);
    if (len > 0) {
        apdu_len += len;
    } else {
        return BACNET_STATUS_ERROR;
    }
    /* itemCount [4] Unsigned */
    len = bacnet_unsigned_context_decode(
        &apdu[apdu_len], apdu_size - apdu_len, 4, &unsigned_value);
    if (len > 0) {
        apdu_len += len;
        if (data) {
            data->ItemCount = (uint32_t)unsigned_value;
        }
    } else {
        return BACNET_STATUS_ERROR;
    }
    /* itemData [5] SEQUENCE OF ABSTRACT-SYNTAX.&TYPE */
    if (!bacnet_is_opening_tag_number(
            &apdu[apdu_len], apdu_size - apdu_len, 5, &len)) {
        return BACNET_STATUS_ERROR;
    }
    /* determine the length of the data blob
       note: APDU must include the opening tag in order to find
       the matching closing tag */
    data_len =
        bacnet_enclosed_data_length(&apdu[apdu_len], apdu_size - apdu_len);
    if (data_len == BACNET_STATUS_ERROR) {
        return BACNET_STATUS_ERROR;
    }
    /* count the opening tag number length AFTER getting the data length */
    apdu_len += len;
    /* sanity check */
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
            &apdu[apdu_len], apdu_size - apdu_len, 5, &len)) {
        return BACNET_STATUS_ERROR;
    }
    /* count the closing tag number length */
    apdu_len += len;
    /* firstSequenceNumber [6] Unsigned32 OPTIONAL
        -- used only if 'Item Count' > 0 and
        -- the request was either of type 'By Sequence Number'
        -- or 'By Time' */
    if (apdu_len < apdu_size) {
        len = bacnet_unsigned_context_decode(
            &apdu[apdu_len], apdu_size - apdu_len, 6, &unsigned_value);
        if (len > 0) {
            apdu_len += len;
            if (data) {
                data->FirstSequence = (uint32_t)unsigned_value;
            }
        } else if (len == 0) {
            /* OPTIONAL missing - skip adding len */
            if (data) {
                data->FirstSequence = 0;
            }
        } else {
            return BACNET_STATUS_ERROR;
        }
    }

    return apdu_len;
}
