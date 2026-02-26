/**
 * @file
 * @brief BACnet GetEventNotification encode and decode functions
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date 2009
 * @copyright SPDX-License-Identifier: GPL-2.0-or-later WITH GCC-exception-2.0
 */
#include <stdint.h>
/* BACnet Stack defines - first */
#include "bacnet/bacdef.h"
/* BACnet Stack API */
#include "bacnet/bacdcode.h"
#include "bacnet/getevent.h"

/**
 * @brief Encode APDU for GetEvent-Request service
 *
 * GetEventInformation-Request ::= SEQUENCE {
 *      last-received-object-identifier [0] BACnetObjectIdentifier OPTIONAL
 * }
 *
 * @param apdu Pointer to a buffer, or NULL for length
 * @param lastReceivedObjectIdentifier  Object identifier
 * @return Bytes encoded.
 */
int getevent_apdu_encode(
    uint8_t *apdu, const BACNET_OBJECT_ID *lastReceivedObjectIdentifier)
{
    int len = 0;
    int apdu_len = 0;

    /* encode optional parameter */
    if (lastReceivedObjectIdentifier) {
        len = encode_context_object_id(
            apdu, 0, lastReceivedObjectIdentifier->type,
            lastReceivedObjectIdentifier->instance);
        apdu_len += len;
    }

    return apdu_len;
}

/**
 * @brief Encode the GetEvent-Request service
 * @param apdu  Pointer to the buffer for encoding into
 * @param apdu_size number of bytes available in the buffer
 * @param data  Pointer to the service data used for encoding values
 * @return number of bytes encoded, or zero if unable to encode or too large
 */
size_t getevent_service_request_encode(
    uint8_t *apdu, size_t apdu_size, const BACNET_OBJECT_ID *data)
{
    size_t apdu_len = 0; /* total length of the apdu, return value */

    apdu_len = getevent_apdu_encode(NULL, data);
    if (apdu_len > apdu_size) {
        apdu_len = 0;
    } else {
        apdu_len = getevent_apdu_encode(apdu, data);
    }

    return apdu_len;
}

/** Encode service
 *
 * @param apdu APDU buffer to encode to.
 * @param invoke_id Invoke ID
 * @param lastReceivedObjectIdentifier  Object identifier
 *
 * @return Bytes encoded.
 */
int getevent_encode_apdu(
    uint8_t *apdu, uint8_t invoke_id, const BACNET_OBJECT_ID *data)
{
    int len = 0; /* length of each encoding */
    int apdu_len = 0; /* total length of the apdu, return value */

    if (apdu) {
        apdu[0] = PDU_TYPE_CONFIRMED_SERVICE_REQUEST;
        apdu[1] = encode_max_segs_max_apdu(0, MAX_APDU);
        apdu[2] = invoke_id;
        apdu[3] = SERVICE_CONFIRMED_GET_EVENT_INFORMATION;
    }
    len = 4;
    apdu_len += len;
    if (apdu) {
        apdu += len;
    }
    len = getevent_apdu_encode(apdu, data);
    apdu_len += len;

    return apdu_len;
}

/** Decode the service request only
 *
 * @param apdu APDU buffer to encode to.
 * @param apdu_size Valid bytes in the buffer.
 * @param lastReceivedObjectIdentifier  Object identifier
 *
 * @return number of bytes decoded, zero if tag mismatch,
 *  or #BACNET_STATUS_ERROR (-1) if malformed
 */
int getevent_decode_service_request(
    const uint8_t *apdu,
    unsigned apdu_size,
    BACNET_OBJECT_ID *lastReceivedObjectIdentifier)
{
    int len = 0;
    BACNET_OBJECT_TYPE *object_type = NULL;
    uint32_t *object_instance = NULL;

    if (lastReceivedObjectIdentifier) {
        object_type = &lastReceivedObjectIdentifier->type;
        object_instance = &lastReceivedObjectIdentifier->instance;
    }
    len = bacnet_object_id_context_decode(
        apdu, apdu_size, 0, object_type, object_instance);

    return len;
}

/**
 * @brief Encode the GetEventInformation-ACK service
 * @param apdu  Pointer to the buffer for encoding into
 * @param max_apdu Maximum number of bytes available in the buffer
 * @param invoke_id Invoke ID
 * @return number of bytes encoded, or zero if unable to encode or too large
 */
int getevent_ack_encode_apdu_init(
    uint8_t *apdu, size_t max_apdu, uint8_t invoke_id)
{
    int len = 0; /* length of each encoding */
    int apdu_len = 0; /* total length of the apdu, return value */

    if (apdu && (max_apdu >= 3)) {
        apdu[0] = PDU_TYPE_COMPLEX_ACK; /* complex ACK service */
        apdu[1] = invoke_id; /* original invoke id from request */
        apdu[2] = SERVICE_CONFIRMED_GET_EVENT_INFORMATION;
    }
    len = 3; /* length of the header */
    apdu_len += len;
    if (apdu) {
        apdu += len;
    }
    /* service ack follows */
    /* Tag 0: listOfEventSummaries */
    len = encode_opening_tag(apdu, 0);
    apdu_len += len;

    return apdu_len;
}

/**
 * @brief Encode one or more GetEventInformation-ACK service data
 *
 * GetEventInformation-ACK ::= SEQUENCE {
 *      list-of-event-summaries [0] SEQUENCE OF SEQUENCE {
 *          object-identifier[0] BACnetObjectIdentifier,
 *          event-state[1] BACnetEventState,
 *          acknowledged-transitions[2] BACnetEventTransitionBits,
 *          event-timestamps[3] SEQUENCE SIZE (3) OF BACnetTimeStamp,
 *          notify-type[4] BACnetNotifyType,
 *          event-enable[5] BACnetEventTransitionBits,
 *          event-priorities[6] SEQUENCE SIZE (3) OF Unsigned
 *      },
 *      more-events [1] Boolean
 * }
 *
 * @param apdu Pointer to the buffer for encoding into
 * @param head Pointer to the head of service data used for encoding values
 * @return number of bytes encoded
 */
int getevent_information_ack_encode(
    uint8_t *apdu, BACNET_GET_EVENT_INFORMATION_DATA *head)
{
    int len = 0, apdu_len = 0;
    BACNET_GET_EVENT_INFORMATION_DATA *data;
    unsigned i = 0; /* counter */

    data = head;
    while (data) {
        /* Tag 0: objectIdentifier */
        len = encode_context_object_id(
            apdu, 0, data->objectIdentifier.type,
            data->objectIdentifier.instance);
        apdu_len += len;
        if (apdu) {
            apdu += len;
        }
        /* Tag 1: eventState */
        len = encode_context_enumerated(apdu, 1, data->eventState);
        apdu_len += len;
        if (apdu) {
            apdu += len;
        }
        /* Tag 2: acknowledgedTransitions */
        len = encode_context_bitstring(apdu, 2, &data->acknowledgedTransitions);
        apdu_len += len;
        if (apdu) {
            apdu += len;
        }
        /* Tag 3: eventTimeStamps */
        len = encode_opening_tag(apdu, 3);
        apdu_len += len;
        if (apdu) {
            apdu += len;
        }
        for (i = 0; i < 3; i++) {
            len = bacapp_encode_timestamp(apdu, &data->eventTimeStamps[i]);
            apdu_len += len;
            if (apdu) {
                apdu += len;
            }
        }
        len = encode_closing_tag(apdu, 3);
        apdu_len += len;
        if (apdu) {
            apdu += len;
        }
        /* Tag 4: notifyType */
        len = encode_context_enumerated(apdu, 4, data->notifyType);
        apdu_len += len;
        if (apdu) {
            apdu += len;
        }
        /* Tag 5: eventEnable */
        len = encode_context_bitstring(apdu, 5, &data->eventEnable);
        apdu_len += len;
        if (apdu) {
            apdu += len;
        }
        /* Tag 6: eventPriorities */
        len = encode_opening_tag(apdu, 6);
        apdu_len += len;
        if (apdu) {
            apdu += len;
        }
        for (i = 0; i < 3; i++) {
            len = encode_application_unsigned(apdu, data->eventPriorities[i]);
            apdu_len += len;
            if (apdu) {
                apdu += len;
            }
        }
        len = encode_closing_tag(apdu, 6);
        apdu_len += len;
        if (apdu) {
            apdu += len;
        }
        data = data->next;
    }

    return apdu_len;
}

/**
 * @brief Encode the GetEventInformation-ACK service data
 * @param apdu  Pointer to the buffer for encoding into
 * @param apdu_size Maximum number of bytes available in the buffer
 * @param get_event_data Pointer to the service data used for encoding values
 * @return number of bytes encoded, or zero if unable to encode or too large
 */
int getevent_ack_encode_apdu_data(
    uint8_t *apdu, size_t apdu_size, BACNET_GET_EVENT_INFORMATION_DATA *data)
{
    size_t apdu_len = 0; /* total length of the apdu, return value */

    apdu_len = getevent_information_ack_encode(NULL, data);
    if (apdu_len > apdu_size) {
        apdu_len = 0;
    } else {
        apdu_len = getevent_information_ack_encode(apdu, data);
    }

    return apdu_len;
}

/**
 * @brief Encode the end of the GetEventInformation-ACK service
 * @param apdu  Pointer to the buffer for encoding into, or NULL for length
 * @param moreEvents More events flag
 * @return number of bytes encoded
 */
int getevent_information_ack_end_encode(uint8_t *apdu, bool moreEvents)
{
    int len = 0, apdu_len = 0;

    len = encode_closing_tag(apdu, 0);
    apdu_len += len;
    if (apdu) {
        apdu += len;
    }
    len = encode_context_boolean(apdu, 1, moreEvents);
    apdu_len += len;

    return apdu_len;
}

/**
 * @brief Encode the end of the GetEventInformation-ACK service
 * @param apdu  Pointer to the buffer for encoding into
 * @param max_apdu Maximum number of bytes available in the buffer
 * @param moreEvents More events flag
 * @return number of bytes encoded, or zero if unable to encode or too large
 */
int getevent_ack_encode_apdu_end(
    uint8_t *apdu, size_t apdu_size, bool moreEvents)
{
    size_t apdu_len = 0; /* total length of the apdu, return value */

    apdu_len = getevent_information_ack_end_encode(NULL, moreEvents);
    if (apdu_len > apdu_size) {
        apdu_len = 0;
    } else {
        apdu_len = getevent_information_ack_end_encode(apdu, moreEvents);
    }

    return apdu_len;
}

/**
 * @brief Decode the GetEventInformation-ACK service
 * @param apdu  Pointer to the buffer for decoding from
 * @param apdu_size Total length of the APDU
 * @param invoke_id Invoke ID
 * @param get_event_data Pointer to one or more service data used for
 *  decoding values
 * @param moreEvents More events flag
 * @return number of bytes decoded, or BACNET_STATUS_ERROR on error
 */
int getevent_ack_decode_service_request(
    const uint8_t *apdu,
    int apdu_size,
    BACNET_GET_EVENT_INFORMATION_DATA *get_event_data,
    bool *moreEvents)
{
    int len = 0, apdu_len = 0;
    uint32_t enum_value = 0; /* for decoding */
    BACNET_UNSIGNED_INTEGER unsigned_value = 0;
    BACNET_OBJECT_TYPE *object_type;
    uint32_t *object_instance;
    BACNET_BIT_STRING *bitstring_value;
    BACNET_TIMESTAMP *timestamp_value;
    BACNET_GET_EVENT_INFORMATION_DATA *data;
    unsigned i = 0; /* counter */

    if (!apdu) {
        return BACNET_STATUS_ERROR;
    }
    if (apdu_size == 0) {
        return BACNET_STATUS_ERROR;
    }
    if (!bacnet_is_opening_tag_number(apdu, apdu_size - apdu_len, 0, &len)) {
        return BACNET_STATUS_ERROR;
    }
    apdu_len += len;
    data = get_event_data;
    while (apdu_len < apdu_size) {
        if (data) {
            object_type = &data->objectIdentifier.type;
            object_instance = &data->objectIdentifier.instance;
        } else {
            object_type = NULL;
            object_instance = NULL;
        }
        /* Tag 0: objectIdentifier */
        len = bacnet_object_id_context_decode(
            &apdu[apdu_len], apdu_size - apdu_len, 0, object_type,
            object_instance);
        if (len <= 0) {
            return BACNET_STATUS_ERROR;
        }
        apdu_len += len;
        /* Tag 1: eventState */
        len = bacnet_enumerated_context_decode(
            &apdu[apdu_len], apdu_size - apdu_len, 1, &enum_value);
        if (len > 0) {
            if (data) {
                if (enum_value < EVENT_STATE_MAX) {
                    data->eventState = (BACNET_EVENT_STATE)enum_value;
                } else {
                    return BACNET_STATUS_ERROR;
                }
            }
        } else {
            return BACNET_STATUS_ERROR;
        }
        apdu_len += len;
        /* Tag 2: acknowledgedTransitions */
        if (data) {
            bitstring_value = &data->acknowledgedTransitions;
        } else {
            bitstring_value = NULL;
        }
        len = bacnet_bitstring_context_decode(
            &apdu[apdu_len], apdu_size - apdu_len, 2, bitstring_value);
        if (len <= 0) {
            return BACNET_STATUS_ERROR;
        }
        apdu_len += len;
        /* Tag 3: eventTimeStamps */
        if (!bacnet_is_opening_tag_number(
                &apdu[apdu_len], apdu_size - apdu_len, 3, &len)) {
            return BACNET_STATUS_ERROR;
        }
        apdu_len += len;
        for (i = 0; i < 3; i++) {
            if (data) {
                timestamp_value = &data->eventTimeStamps[i];
            } else {
                timestamp_value = NULL;
            }
            len = bacnet_timestamp_decode(
                &apdu[apdu_len], apdu_size - apdu_len, timestamp_value);
            if (len <= 0) {
                return BACNET_STATUS_ERROR;
            }
            apdu_len += len;
        }
        if (!bacnet_is_closing_tag_number(
                &apdu[apdu_len], apdu_size - apdu_len, 3, &len)) {
            return BACNET_STATUS_ERROR;
        }
        apdu_len += len;
        /* Tag 4: notifyType */
        len = bacnet_enumerated_context_decode(
            &apdu[apdu_len], apdu_size - apdu_len, 4, &enum_value);
        if (len > 0) {
            if (enum_value < NOTIFY_MAX) {
                if (data) {
                    data->notifyType = (BACNET_NOTIFY_TYPE)enum_value;
                }
            } else {
                return BACNET_STATUS_ERROR;
            }
        } else {
            return BACNET_STATUS_ERROR;
        }
        apdu_len += len;
        /* Tag 5: eventEnable */
        if (data) {
            bitstring_value = &data->eventEnable;
        } else {
            bitstring_value = NULL;
        }
        len = bacnet_bitstring_context_decode(
            &apdu[apdu_len], apdu_size - apdu_len, 5, bitstring_value);
        if (len <= 0) {
            return BACNET_STATUS_ERROR;
        }
        apdu_len += len;
        /* Tag 6: eventPriorities */
        if (!bacnet_is_opening_tag_number(
                &apdu[apdu_len], apdu_size - apdu_len, 6, &len)) {
            return BACNET_STATUS_ERROR;
        }
        apdu_len += len;
        for (i = 0; i < 3; i++) {
            len = bacnet_unsigned_application_decode(
                &apdu[apdu_len], apdu_size - apdu_len, &unsigned_value);
            if (len > 0) {
                if (unsigned_value <= UINT32_MAX) {
                    if (data) {
                        data->eventPriorities[i] = (uint32_t)unsigned_value;
                    }
                } else {
                    return BACNET_STATUS_ERROR;
                }
            } else {
                return BACNET_STATUS_ERROR;
            }
            apdu_len += len;
        }
        if (!bacnet_is_closing_tag_number(
                &apdu[apdu_len], apdu_size - apdu_len, 6, &len)) {
            return BACNET_STATUS_ERROR;
        }
        apdu_len += len;
        if (bacnet_is_closing_tag_number(
                &apdu[apdu_len], apdu_size - apdu_len, 0, &len)) {
            if (data) {
                data->next = NULL; /* mark end of the list */
            }
            apdu_len += len;
            break;
        }
        if (data) {
            data = data->next;
        }
    }
    /* Tag 1: moreEvents */
    len = bacnet_boolean_context_decode(
        &apdu[apdu_len], apdu_size - apdu_len, 1, moreEvents);
    if (len <= 0) {
        return BACNET_STATUS_ERROR; /* malformed */
    }
    apdu_len += len;

    return apdu_len;
}

/**
 * @brief Convert an array of GetEventInformation-Request to linked list
 * @param array pointer to element zero of the array
 * @param size number of elements in the array
 */
void getevent_information_link_array(
    BACNET_GET_EVENT_INFORMATION_DATA *array, size_t size)
{
    size_t i = 0;

    for (i = 0; i < size; i++) {
        if (i > 0) {
            array[i - 1].next = &array[i];
        }
        array[i].next = NULL;
    }
}
