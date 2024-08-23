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
int read_range_encode(uint8_t *apdu, BACNET_READ_RANGE_DATA *data)
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
    uint8_t *apdu, size_t apdu_size, BACNET_READ_RANGE_DATA *data)
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
    uint8_t *apdu, uint8_t invoke_id, BACNET_READ_RANGE_DATA *data)
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
 *  @param apdu  Pointer to the APDU buffer.
 *  @param apdu_len  Bytes valid in the APDU buffer.
 *  @param rrdata  Pointer to the data used for encoding.
 *
 *  @return Bytes encoded.
 */
int rr_decode_service_request(
    uint8_t *apdu, unsigned apdu_len, BACNET_READ_RANGE_DATA *rrdata)
{
    unsigned len = 0;
    unsigned TagLen = 0;
    uint8_t tag_number = 0;
    uint32_t len_value_type = 0;
    BACNET_OBJECT_TYPE type = OBJECT_NONE; /* for decoding */
    uint32_t enum_value;
    BACNET_UNSIGNED_INTEGER unsigned_value;

    /* check for value pointers */
    if ((apdu_len >= 5) && apdu && rrdata) {
        /* Tag 0: Object ID */
        if (!decode_is_context_tag(&apdu[len++], 0)) {
            return -1;
        }
        len += decode_object_id(&apdu[len], &type, &rrdata->object_instance);
        rrdata->object_type = type;
        /* Tag 1: Property ID */
        if (len >= apdu_len) {
            return (-1);
        }
        len += decode_tag_number_and_value(
            &apdu[len], &tag_number, &len_value_type);
        if (tag_number != 1) {
            return -1;
        }
        len += decode_enumerated(&apdu[len], len_value_type, &enum_value);
        rrdata->object_property = (BACNET_PROPERTY_ID)enum_value;
        rrdata->Overhead = RR_OVERHEAD; /* Start with the fixed overhead */

        /* Tag 2: Optional Array Index - set to ALL if not present */
        rrdata->array_index = BACNET_ARRAY_ALL; /* Assuming this is the most
                                                   common outcome... */
        if (len < apdu_len) {
            TagLen = (unsigned)decode_tag_number_and_value(
                &apdu[len], &tag_number, &len_value_type);
            if (tag_number == 2) {
                len += TagLen;
                len += decode_unsigned(
                    &apdu[len], len_value_type, &unsigned_value);
                rrdata->array_index = (BACNET_ARRAY_INDEX)unsigned_value;
                rrdata->Overhead +=
                    RR_INDEX_OVERHEAD; /* Allow for this in the response */
            }
        }
        /* And/or optional range selection- Tags 3, 6 and 7 */
        rrdata->RequestType = RR_READ_ALL; /* Assume the worst to cut out
                                              explicit checking later */
        if (len < apdu_len) {
            /*
             * Note: We pick up the opening tag and then decode the
             * parameter types we recognise. We deal with the count and the
             * closing tag in each case statement even though it might
             * appear that we could do them after the switch statement as
             * common elements. This is so that if we receive a tag we don't
             * recognise, we don't try to decode it blindly and make a mess
             * of it.
             */
            len += decode_tag_number_and_value(
                &apdu[len], &tag_number, &len_value_type);
            switch (tag_number) {
                case 3: /* ReadRange by position */
                    rrdata->RequestType = RR_BY_POSITION;
                    if (len >= apdu_len) {
                        break;
                    }
                    len += decode_tag_number_and_value(
                        &apdu[len], &tag_number, &len_value_type);
                    if (len >= apdu_len) {
                        break;
                    }
                    len += decode_unsigned(
                        &apdu[len], len_value_type, &unsigned_value);
                    rrdata->Range.RefIndex = (uint32_t)unsigned_value;
                    if (len >= apdu_len) {
                        break;
                    }
                    len += decode_tag_number_and_value(
                        &apdu[len], &tag_number, &len_value_type);
                    if (len >= apdu_len) {
                        break;
                    }
                    len += decode_signed(
                        &apdu[len], len_value_type, &rrdata->Count);
                    if (len >= apdu_len) {
                        break;
                    }
                    len += decode_tag_number_and_value(
                        &apdu[len], &tag_number, &len_value_type);
                    break;

                case 6: /* ReadRange by sequence number */
                    rrdata->RequestType = RR_BY_SEQUENCE;
                    if (len >= apdu_len) {
                        break;
                    }
                    len += decode_tag_number_and_value(
                        &apdu[len], &tag_number, &len_value_type);
                    if (len >= apdu_len) {
                        break;
                    }
                    len += decode_unsigned(
                        &apdu[len], len_value_type, &unsigned_value);
                    rrdata->Range.RefSeqNum = (uint32_t)unsigned_value;
                    if (len >= apdu_len) {
                        break;
                    }
                    len += decode_tag_number_and_value(
                        &apdu[len], &tag_number, &len_value_type);
                    if (len >= apdu_len) {
                        break;
                    }
                    len += decode_signed(
                        &apdu[len], len_value_type, &rrdata->Count);
                    if (len >= apdu_len) {
                        break;
                    }
                    len += decode_tag_number_and_value(
                        &apdu[len], &tag_number, &len_value_type);
                    /* Allow for this in the response */
                    rrdata->Overhead += RR_1ST_SEQ_OVERHEAD;
                    break;

                case 7: /* ReadRange by time stamp */
                    rrdata->RequestType = RR_BY_TIME;
                    if (len >= apdu_len) {
                        break;
                    }
                    len += decode_tag_number_and_value(
                        &apdu[len], &tag_number, &len_value_type);
                    if (len >= apdu_len) {
                        break;
                    }
                    len += decode_date(&apdu[len], &rrdata->Range.RefTime.date);
                    if (len >= apdu_len) {
                        break;
                    }
                    len += decode_tag_number_and_value(
                        &apdu[len], &tag_number, &len_value_type);
                    if (len >= apdu_len) {
                        break;
                    }
                    len += decode_bacnet_time(
                        &apdu[len], &rrdata->Range.RefTime.time);
                    if (len >= apdu_len) {
                        break;
                    }
                    len += decode_tag_number_and_value(
                        &apdu[len], &tag_number, &len_value_type);
                    if (len >= apdu_len) {
                        break;
                    }
                    len += decode_signed(
                        &apdu[len], len_value_type, &rrdata->Count);
                    if (len >= apdu_len) {
                        break;
                    }
                    len += decode_tag_number_and_value(
                        &apdu[len], &tag_number, &len_value_type);
                    /* Allow for this in the response */
                    rrdata->Overhead += RR_1ST_SEQ_OVERHEAD;
                    break;

                default: /* If we don't recognise the tag then we do nothing
                          * here and try to return all elements of the array
                          */
                    break;
            }
        }
    } else {
        return (-1);
    }

    return (int)len;
}

/*
 * ReadRange-ACK ::= SEQUENCE {
 *     objectIdentifier    [0] BACnetObjectIdentifier,
 *     propertyIdentifier  [1] BACnetPropertyIdentifier,
 *     propertyArrayIndex  [2] Unsigned OPTIONAL,  -- used only with
 * array datatype resultFlags         [3] BACnetResultFlags, itemCount [4]
 * Unsigned, itemData            [5] SEQUENCE OF ABSTRACT-SYNTAX.&TYPE,
 *     firstSequenceNumber [6] Unsigned32 OPTIONAL -- used only if 'Item
 * Count' > 0 and the request was either of
 *                                                  -- type 'By Sequence
 * Number' or 'By Time'
 * }
 */

/**
 * Build a ReadRange response packet
 *
 * @param apdu  Pointer to the buffer.
 * @param invoke_id  ID invoked.
 * @param rrdata  Pointer to the read range data structure used for
 * encoding.
 *
 * @return The count of encoded bytes.
 */
int rr_ack_encode_apdu(
    uint8_t *apdu, uint8_t invoke_id, BACNET_READ_RANGE_DATA *rrdata)
{
    int imax = 0;
    int len = 0; /* length of each encoding */
    int apdu_len = 0; /* total length of the apdu, return value */

    if (apdu) {
        apdu[0] = PDU_TYPE_COMPLEX_ACK; /* complex ACK service */
        apdu[1] = invoke_id; /* original invoke id from request */
        apdu[2] = SERVICE_CONFIRMED_READ_RANGE; /* service choice */
        apdu_len = 3;
        /* service ack follows */
        apdu_len += encode_context_object_id(
            &apdu[apdu_len], 0, rrdata->object_type, rrdata->object_instance);
        apdu_len += encode_context_enumerated(
            &apdu[apdu_len], 1, rrdata->object_property);
        /* context 2 array index is optional */
        if (rrdata->array_index != BACNET_ARRAY_ALL) {
            apdu_len += encode_context_unsigned(
                &apdu[apdu_len], 2, rrdata->array_index);
        }
        /* Context 3 BACnet Result Flags */
        apdu_len +=
            encode_context_bitstring(&apdu[apdu_len], 3, &rrdata->ResultFlags);
        /* Context 4 Item Count */
        apdu_len +=
            encode_context_unsigned(&apdu[apdu_len], 4, rrdata->ItemCount);
        /* Context 5 Property list - reading the standard it looks like an
         * empty list still requires an opening and closing tag as the
         * tagged parameter is not optional
         */
        apdu_len += encode_opening_tag(&apdu[apdu_len], 5);
        if (rrdata->ItemCount != 0) {
            imax = rrdata->application_data_len;
            if (imax > (MAX_APDU - apdu_len - 2 /*closing*/)) {
                imax = (MAX_APDU - apdu_len - 2);
            }
            for (len = 0; len < imax; len++) {
                apdu[apdu_len++] = rrdata->application_data[len];
            }
        }
        apdu_len += encode_closing_tag(&apdu[apdu_len], 5);

        if ((rrdata->ItemCount != 0) &&
            (rrdata->RequestType != RR_BY_POSITION) &&
            (rrdata->RequestType != RR_READ_ALL)) {
            /* Context 6 Sequence number of first item */
            if (apdu_len < (MAX_APDU - 4)) {
                apdu_len += encode_context_unsigned(
                    &apdu[apdu_len], 6, rrdata->FirstSequence);
            }
        }
    }

    return apdu_len;
}

/**
 * Decode the received ReadRange response
 *
 *  @param apdu  Pointer to the APDU buffer.
 *  @param apdu_len  Bytes valid in the APDU buffer.
 *  @param rrdata  Pointer to the data filled while decoding.
 *
 *  @return Bytes decoded.
 */
int rr_ack_decode_service_request(uint8_t *apdu,
    int apdu_len, /* total length of the apdu */
    BACNET_READ_RANGE_DATA *rrdata)
{
    uint8_t tag_number = 0;
    uint32_t len_value_type = 0;
    int tag_len = 0; /* length of tag decode */
    int len = 0; /* total length of decodes */
    int start_len;
    BACNET_OBJECT_TYPE object_type = OBJECT_NONE; /* object type */
    uint32_t property = 0; /* for decoding */
    BACNET_UNSIGNED_INTEGER unsigned_value;

    /* Check apdu_len against the len during decode. */
    if (apdu && (apdu_len >= 5 /* minimum */)) {
        /* Tag 0: Object ID */
        if (!decode_is_context_tag(&apdu[0], 0)) {
            return -1;
        }
        len = 1;
        len += decode_object_id(
            &apdu[len], &object_type, &rrdata->object_instance);
        rrdata->object_type = object_type;

        /* Tag 1: Property ID */
        if (len >= apdu_len) {
            return -1;
        }
        len += decode_tag_number_and_value(
            &apdu[len], &tag_number, &len_value_type);
        if (tag_number != 1) {
            return -1;
        }
        len += decode_enumerated(&apdu[len], len_value_type, &property);
        rrdata->object_property = (BACNET_PROPERTY_ID)property;

        /* Tag 2: Optional Array Index */
        if (len >= apdu_len) {
            return -1;
        }
        tag_len = decode_tag_number_and_value(
            &apdu[len], &tag_number, &len_value_type);
        if (tag_number == 2) {
            len += tag_len;
            len += decode_unsigned(&apdu[len], len_value_type, &unsigned_value);
            rrdata->array_index = (BACNET_ARRAY_INDEX)unsigned_value;
        } else {
            rrdata->array_index = BACNET_ARRAY_ALL;
        }

        /* Tag 3: Result Flags */
        if (len >= apdu_len) {
            return -1;
        }
        len += decode_tag_number_and_value(
            &apdu[len], &tag_number, &len_value_type);
        if (tag_number != 3) {
            return -1;
        }
        if (len >= apdu_len) {
            return -1;
        }
        len +=
            decode_bitstring(&apdu[len], len_value_type, &rrdata->ResultFlags);

        /* Tag 4: Item count */
        if (len >= apdu_len) {
            return -1;
        }
        len += decode_tag_number_and_value(
            &apdu[len], &tag_number, &len_value_type);
        if (tag_number != 4) {
            return -1;
        }
        if (len >= apdu_len) {
            return -1;
        }
        len += decode_unsigned(&apdu[len], len_value_type, &unsigned_value);
        rrdata->ItemCount = (uint32_t)unsigned_value;
        if (len >= apdu_len) {
            return -1;
        }
        if (decode_is_opening_tag_number(&apdu[len], 5)) {
            len++; /* A tag number of 5 is not extended so only one octet
                    * Setup the start position and length of the data
                    * returned from the request don't decode the application
                    * tag number or its data here. */
            rrdata->application_data = &apdu[len];
            start_len = len;
            while (len < apdu_len) {
                if (IS_CONTEXT_SPECIFIC(apdu[len]) &&
                    (decode_is_closing_tag_number(&apdu[len], 5))) {
                    rrdata->application_data_len = len - start_len;
                    len++; /* Step over single byte closing tag */
                    break;
                } else {
                    /* Don't care about tag number, just skipping over
                     * anyway */
                    len += decode_tag_number_and_value(
                        &apdu[len], NULL, &len_value_type);
                    len += len_value_type; /* Skip over data value as well */
                    if (len >= apdu_len) { /* APDU is exhausted so we have
                                            * failed to find closing tag */
                        return (-1);
                    }
                }
            }
        } else {
            return -1;
        }
        if (len < apdu_len) { /* Still something left to look at? */
            /* Tag 6: Item count */
            len += decode_tag_number_and_value(
                &apdu[len], &tag_number, &len_value_type);
            if (tag_number != 6) {
                return -1;
            }
            if (len < apdu_len) {
                len += decode_unsigned(
                    &apdu[len], len_value_type, &unsigned_value);
                rrdata->FirstSequence = (uint32_t)unsigned_value;
            }
        }
    }

    return len;
}
