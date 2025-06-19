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
 * @deprecated Use getevent_apdu_encode() instead
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
 * @param apdu_len Valid bytes in the buffer.
 * @param lastReceivedObjectIdentifier  Object identifier
 *
 * @return Bytes encoded.
 */
int getevent_decode_service_request(
    const uint8_t *apdu,
    unsigned apdu_len,
    BACNET_OBJECT_ID *lastReceivedObjectIdentifier)
{
    unsigned len = 0;

    /* check for value pointers */
    if (apdu_len && lastReceivedObjectIdentifier) {
        /* Tag 0: Object ID - optional */
        if (!decode_is_context_tag(&apdu[len++], 0)) {
            return -1;
        }
        if (len < apdu_len) {
            len += decode_object_id(
                &apdu[len], &lastReceivedObjectIdentifier->type,
                &lastReceivedObjectIdentifier->instance);
        }
    }

    return (int)len;
}

int getevent_ack_encode_apdu_init(
    uint8_t *apdu, size_t max_apdu, uint8_t invoke_id)
{
    int apdu_len = 0; /* total length of the apdu, return value */

    if (apdu && (max_apdu >= 4)) {
        apdu[0] = PDU_TYPE_COMPLEX_ACK; /* complex ACK service */
        apdu[1] = invoke_id; /* original invoke id from request */
        apdu[2] = SERVICE_CONFIRMED_GET_EVENT_INFORMATION;
        apdu_len = 3;
        /* service ack follows */
        /* Tag 0: listOfEventSummaries */
        apdu_len += encode_opening_tag(&apdu[apdu_len], 0);
    }

    return apdu_len;
}

int getevent_ack_encode_apdu_data(
    uint8_t *apdu,
    size_t max_apdu,
    BACNET_GET_EVENT_INFORMATION_DATA *get_event_data)
{
    int apdu_len = 0; /* total length of the apdu, return value */
    BACNET_GET_EVENT_INFORMATION_DATA *event_data;
    unsigned i = 0; /* counter */

    (void)max_apdu;
    if (apdu) {
        event_data = get_event_data;
        while (event_data) {
            /* Tag 0: objectIdentifier */
            apdu_len += encode_context_object_id(
                &apdu[apdu_len], 0, event_data->objectIdentifier.type,
                event_data->objectIdentifier.instance);
            /* Tag 1: eventState */
            apdu_len += encode_context_enumerated(
                &apdu[apdu_len], 1, event_data->eventState);
            /* Tag 2: acknowledgedTransitions */
            apdu_len += encode_context_bitstring(
                &apdu[apdu_len], 2, &event_data->acknowledgedTransitions);
            /* Tag 3: eventTimeStamps */
            apdu_len += encode_opening_tag(&apdu[apdu_len], 3);
            for (i = 0; i < 3; i++) {
                apdu_len += bacapp_encode_timestamp(
                    &apdu[apdu_len], &event_data->eventTimeStamps[i]);
            }
            apdu_len += encode_closing_tag(&apdu[apdu_len], 3);
            /* Tag 4: notifyType */
            apdu_len += encode_context_enumerated(
                &apdu[apdu_len], 4, event_data->notifyType);
            /* Tag 5: eventEnable */
            apdu_len += encode_context_bitstring(
                &apdu[apdu_len], 5, &event_data->eventEnable);
            /* Tag 6: eventPriorities */
            apdu_len += encode_opening_tag(&apdu[apdu_len], 6);
            for (i = 0; i < 3; i++) {
                apdu_len += encode_application_unsigned(
                    &apdu[apdu_len], event_data->eventPriorities[i]);
            }
            apdu_len += encode_closing_tag(&apdu[apdu_len], 6);
            event_data = event_data->next;
        }
    }

    return apdu_len;
}

int getevent_ack_encode_apdu_end(
    uint8_t *apdu, size_t max_apdu, bool moreEvents)
{
    int apdu_len = 0; /* total length of the apdu, return value */

    (void)max_apdu;
    if (apdu) {
        apdu_len += encode_closing_tag(&apdu[apdu_len], 0);
        apdu_len += encode_context_boolean(&apdu[apdu_len], 1, moreEvents);
    }

    return apdu_len;
}

int getevent_ack_decode_service_request(
    const uint8_t *apdu,
    int apdu_len, /* total length of the apdu */
    BACNET_GET_EVENT_INFORMATION_DATA *get_event_data,
    bool *moreEvents)
{
    uint8_t tag_number = 0;
    uint32_t len_value = 0;
    int len = 0; /* total length of decodes */
    uint32_t enum_value = 0; /* for decoding */
    BACNET_UNSIGNED_INTEGER unsigned_value = 0;
    BACNET_GET_EVENT_INFORMATION_DATA *event_data;
    unsigned i = 0; /* counter */

    /* FIXME: check apdu_len against the len during decode   */
    event_data = get_event_data;
    if (apdu && apdu_len && event_data && moreEvents) {
        if (!decode_is_opening_tag_number(&apdu[len], 0)) {
            return -1;
        }
        len++;
        while (event_data) {
            /* Tag 0: objectIdentifier */
            if (decode_is_context_tag(&apdu[len], 0)) {
                len += decode_tag_number_and_value(
                    &apdu[len], &tag_number, &len_value);
                len += decode_object_id(
                    &apdu[len], &event_data->objectIdentifier.type,
                    &event_data->objectIdentifier.instance);
            } else {
                return -1;
            }
            /* Tag 1: eventState */
            if (decode_is_context_tag(&apdu[len], 1)) {
                len += decode_tag_number_and_value(
                    &apdu[len], &tag_number, &len_value);
                len += decode_enumerated(&apdu[len], len_value, &enum_value);
                if (enum_value < EVENT_STATE_MAX) {
                    event_data->eventState = (BACNET_EVENT_STATE)enum_value;
                } else {
                    return -1;
                }
            } else {
                return -1;
            }
            /* Tag 2: acknowledgedTransitions */
            if (decode_is_context_tag(&apdu[len], 2)) {
                len += decode_tag_number_and_value(
                    &apdu[len], &tag_number, &len_value);
                len += decode_bitstring(
                    &apdu[len], len_value,
                    &event_data->acknowledgedTransitions);
            } else {
                return -1;
            }
            /* Tag 3: eventTimeStamps */
            if (decode_is_opening_tag_number(&apdu[len], 3)) {
                len += decode_tag_number_and_value(
                    &apdu[len], &tag_number, &len_value);
                for (i = 0; i < 3; i++) {
                    len += bacapp_decode_timestamp(
                        &apdu[len], &event_data->eventTimeStamps[i]);
                }
            } else {
                return -1;
            }
            if (decode_is_closing_tag_number(&apdu[len], 3)) {
                len += decode_tag_number_and_value(
                    &apdu[len], &tag_number, &len_value);
            } else {
                return -1;
            }
            /* Tag 4: notifyType */
            if (decode_is_context_tag(&apdu[len], 4)) {
                len += decode_tag_number_and_value(
                    &apdu[len], &tag_number, &len_value);
                len += decode_enumerated(&apdu[len], len_value, &enum_value);
                if (enum_value < NOTIFY_MAX) {
                    event_data->notifyType = (BACNET_NOTIFY_TYPE)enum_value;
                } else {
                    return -1;
                }
            } else {
                return -1;
            }
            /* Tag 5: eventEnable */
            if (decode_is_context_tag(&apdu[len], 5)) {
                len += decode_tag_number_and_value(
                    &apdu[len], &tag_number, &len_value);
                len += decode_bitstring(
                    &apdu[len], len_value, &event_data->eventEnable);
            } else {
                return -1;
            }
            /* Tag 6: eventPriorities */
            if (decode_is_opening_tag_number(&apdu[len], 6)) {
                len += decode_tag_number_and_value(
                    &apdu[len], &tag_number, &len_value);
                for (i = 0; i < 3; i++) {
                    len += decode_tag_number_and_value(
                        &apdu[len], &tag_number, &len_value);
                    len +=
                        decode_unsigned(&apdu[len], len_value, &unsigned_value);
                    event_data->eventPriorities[i] = (uint32_t)unsigned_value;
                }
            } else {
                return -1;
            }
            if (decode_is_closing_tag_number(&apdu[len], 6)) {
                len += decode_tag_number_and_value(
                    &apdu[len], &tag_number, &len_value);
            } else {
                return -1;
            }
            if (decode_is_closing_tag_number(&apdu[len], 0)) {
                len += decode_tag_number_and_value(
                    &apdu[len], &tag_number, &len_value);
                event_data->next = NULL;
            }
            event_data = event_data->next;
        }
        if (decode_is_context_tag(&apdu[len], 1)) {
            len += decode_tag_number_and_value(
                &apdu[len], &tag_number, &len_value);
            if (len_value == 1) {
                *moreEvents = decode_context_boolean(&apdu[len++]);
            } else {
                *moreEvents = false;
            }
        } else {
            return -1;
        }
    }

    return len;
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
