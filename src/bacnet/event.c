/**
 * @file
 * @brief BACnet EventNotification encode and decode functions
 * @author John Minack <minack@users.sourceforge.net>
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date 2008
 * @copyright SPDX-License-Identifier: GPL-2.0-or-later WITH GCC-exception-2.0
 */
#include <assert.h>
#include "bacnet/event.h"
#include "bacnet/bacdcode.h"
#include "bacnet/npdu.h"
#include "bacnet/timestamp.h"
#include "bacnet/authentication_factor.h"

/** @file event.c  Encode/Decode Event Notifications */

/**
 * @brief Decode the array of complex-event-type notification parameters
 * @param apdu - apdu buffer
 * @param apdu_size - APDU total length
 * @param data - the event data struct to store the results in
 * @return number of apdu bytes decoded, or BACNET_STATUS_ERROR on error.
 */
static int complex_event_type_values_decode(
    const uint8_t *apdu,
    unsigned apdu_size,
    BACNET_EVENT_NOTIFICATION_DATA *data)
{
    int len = 0; /* return value */
    BACNET_PROPERTY_VALUE *value;
    int value_len = 0, tag_len = 0;

#if (BACNET_DECODE_COMPLEX_EVENT_TYPE_PARAMETERS == 1)
    /* we want to extract the values */
    value = data->notificationParams.complexEventType.values;
    bacapp_property_value_list_init(
        value, BACNET_COMPLEX_EVENT_TYPE_MAX_PARAMETERS);
#else
    /* we just want to discard the complex values */
    BACNET_PROPERTY_VALUE dummyValue;
    bacapp_property_value_list_init(&dummyValue, 1);
    value = &dummyValue;
#endif
    while (value != NULL) {
        value_len =
            bacapp_property_value_decode(&apdu[len], apdu_size - len, value);
        if (value_len == BACNET_STATUS_ERROR) {
            return BACNET_STATUS_ERROR;
        } else {
            len += value_len;
        }
        /* end of list? */
        if (bacnet_is_closing_tag_number(
                &apdu[len], apdu_size - len, 6, &tag_len)) {
            len += tag_len;
            value->next = NULL;
            break;
        }
        /* is there another one to decode? */
        value = value->next;
        if (value == NULL) {
            /* out of room to store next value */
            return BACNET_STATUS_ERROR;
        }
    }

    return len;
}

/**
 * @brief Encode the unconfirmed COVNotification service request
 * @param apdu  Pointer to the buffer for encoding into
 * @param data  Pointer to the service data used for encoding values
 * @return number of bytes encoded, or zero if unable to encode
 */
int uevent_notify_encode_apdu(
    uint8_t *apdu, const BACNET_EVENT_NOTIFICATION_DATA *data)
{
    int len = 0; /* length of each encoding */
    int apdu_len = 0; /* total length of the apdu, return value */

    if (apdu) {
        apdu[0] = PDU_TYPE_UNCONFIRMED_SERVICE_REQUEST;
        apdu[1] = SERVICE_UNCONFIRMED_EVENT_NOTIFICATION; /* service choice */
    }
    len = 2;
    apdu_len += len;
    if (apdu) {
        apdu += len;
    }
    len = event_notify_encode_service_request(apdu, data);
    if (len > 0) {
        apdu_len += len;
    } else {
        apdu_len = 0;
    }

    return apdu_len;
}

/**
 * @brief Encode the confirmed COVNotification service request
 * @param apdu  Pointer to the buffer for encoding into
 * @param invoke_id  ID to invoke for notification
 * @param data  Pointer to the service data used for encoding values
 * @return number of bytes encoded, or zero if unable to encode
 */
int cevent_notify_encode_apdu(
    uint8_t *apdu,
    uint8_t invoke_id,
    const BACNET_EVENT_NOTIFICATION_DATA *data)
{
    int len = 0; /* length of each encoding */
    int apdu_len = 0; /* total length of the apdu, return value */

    if (apdu) {
        apdu[0] = PDU_TYPE_CONFIRMED_SERVICE_REQUEST;
        apdu[1] = encode_max_segs_max_apdu(0, MAX_APDU);
        apdu[2] = invoke_id;
        apdu[3] = SERVICE_CONFIRMED_EVENT_NOTIFICATION; /* service choice */
    }
    len = 4;
    apdu_len += len;
    if (apdu) {
        apdu += len;
    }
    len = event_notify_encode_service_request(apdu, data);
    if (len > 0) {
        apdu_len += len;
    } else {
        apdu_len = 0;
    }

    return apdu_len;
}

/**
 * @brief Encode the COVNotification service request
 * @param apdu  Pointer to the buffer for encoding into
 * @param data  Pointer to the service data used for encoding values
 * @return number of bytes encoded, or zero if unable to encode
 */
int event_notify_encode_service_request(
    uint8_t *apdu, const BACNET_EVENT_NOTIFICATION_DATA *data)
{
    int len = 0; /* length of each encoding */
    int apdu_len = 0; /* total length of the apdu, return value */

    if (!data) {
        return 0;
    }
    /* tag 0 - processIdentifier */
    len = encode_context_unsigned(apdu, 0, data->processIdentifier);
    apdu_len += len;
    if (apdu) {
        apdu += len;
    }
    /* tag 1 - initiatingObjectIdentifier */
    len = encode_context_object_id(
        apdu, 1, data->initiatingObjectIdentifier.type,
        data->initiatingObjectIdentifier.instance);
    apdu_len += len;
    if (apdu) {
        apdu += len;
    }
    /* tag 2 - eventObjectIdentifier */
    len = encode_context_object_id(
        apdu, 2, data->eventObjectIdentifier.type,
        data->eventObjectIdentifier.instance);
    apdu_len += len;
    if (apdu) {
        apdu += len;
    }
    /* tag 3 - timeStamp */
    len = bacapp_encode_context_timestamp(apdu, 3, &data->timeStamp);
    apdu_len += len;
    if (apdu) {
        apdu += len;
    }
    /* tag 4 - noticicationClass */
    len = encode_context_unsigned(apdu, 4, data->notificationClass);
    apdu_len += len;
    if (apdu) {
        apdu += len;
    }
    /* tag 5 - priority */
    len = encode_context_unsigned(apdu, 5, data->priority);
    apdu_len += len;
    if (apdu) {
        apdu += len;
    }
    /* tag 6 - eventType */
    len = encode_context_enumerated(apdu, 6, data->eventType);
    apdu_len += len;
    if (apdu) {
        apdu += len;
    }
    /* tag 7 - messageText */
    if (data->messageText) {
        len = encode_context_character_string(apdu, 7, data->messageText);
        apdu_len += len;
        if (apdu) {
            apdu += len;
        }
    }
    /* tag 8 - notifyType */
    len = encode_context_enumerated(apdu, 8, data->notifyType);
    apdu_len += len;
    if (apdu) {
        apdu += len;
    }
    switch (data->notifyType) {
        case NOTIFY_ALARM:
        case NOTIFY_EVENT:
            /* tag 9 - ackRequired */
            len = encode_context_boolean(apdu, 9, data->ackRequired);
            apdu_len += len;
            if (apdu) {
                apdu += len;
            }
            /* tag 10 - fromState */
            len = encode_context_enumerated(apdu, 10, data->fromState);
            apdu_len += len;
            if (apdu) {
                apdu += len;
            }
            break;

        default:
            break;
    }
    /* tag 11 - toState */
    len = encode_context_enumerated(apdu, 11, data->toState);
    apdu_len += len;
    if (apdu) {
        apdu += len;
    }
    switch (data->notifyType) {
        case NOTIFY_ALARM:
        case NOTIFY_EVENT:
            /* tag 12 - event values */
            len = encode_opening_tag(apdu, 12);
            apdu_len += len;
            if (apdu) {
                apdu += len;
            }
            switch (data->eventType) {
                case EVENT_CHANGE_OF_BITSTRING:
                    len = encode_opening_tag(apdu, 0);
                    apdu_len += len;
                    if (apdu) {
                        apdu += len;
                    }
                    len = encode_context_bitstring(
                        apdu, 0,
                        &data->notificationParams.changeOfBitstring
                             .referencedBitString);
                    apdu_len += len;
                    if (apdu) {
                        apdu += len;
                    }
                    len = encode_context_bitstring(
                        apdu, 1,
                        &data->notificationParams.changeOfBitstring
                             .statusFlags);
                    apdu_len += len;
                    if (apdu) {
                        apdu += len;
                    }
                    len = encode_closing_tag(apdu, 0);
                    apdu_len += len;
                    if (apdu) {
                        apdu += len;
                    }
                    break;
                case EVENT_CHANGE_OF_STATE:
                    len = encode_opening_tag(apdu, 1);
                    apdu_len += len;
                    if (apdu) {
                        apdu += len;
                    }
                    len = encode_opening_tag(apdu, 0);
                    apdu_len += len;
                    if (apdu) {
                        apdu += len;
                    }
                    len = bacapp_encode_property_state(
                        apdu, &data->notificationParams.changeOfState.newState);
                    apdu_len += len;
                    if (apdu) {
                        apdu += len;
                    }
                    len = encode_closing_tag(apdu, 0);
                    apdu_len += len;
                    if (apdu) {
                        apdu += len;
                    }
                    len = encode_context_bitstring(
                        apdu, 1,
                        &data->notificationParams.changeOfState.statusFlags);
                    apdu_len += len;
                    if (apdu) {
                        apdu += len;
                    }
                    len = encode_closing_tag(apdu, 1);
                    apdu_len += len;
                    if (apdu) {
                        apdu += len;
                    }
                    break;
                case EVENT_CHANGE_OF_VALUE:
                    len = encode_opening_tag(apdu, 2);
                    apdu_len += len;
                    if (apdu) {
                        apdu += len;
                    }
                    len = encode_opening_tag(apdu, 0);
                    apdu_len += len;
                    if (apdu) {
                        apdu += len;
                    }
                    switch (data->notificationParams.changeOfValue.tag) {
                        case CHANGE_OF_VALUE_REAL:
                            len = encode_context_real(
                                apdu, 1,
                                data->notificationParams.changeOfValue.newValue
                                    .changeValue);
                            apdu_len += len;
                            if (apdu) {
                                apdu += len;
                            }
                            break;
                        case CHANGE_OF_VALUE_BITS:
                            len = encode_context_bitstring(
                                apdu, 0,
                                &data->notificationParams.changeOfValue.newValue
                                     .changedBits);
                            apdu_len += len;
                            if (apdu) {
                                apdu += len;
                            }
                            break;
                        default:
                            return 0;
                    }
                    len = encode_closing_tag(apdu, 0);
                    apdu_len += len;
                    if (apdu) {
                        apdu += len;
                    }
                    len = encode_context_bitstring(
                        apdu, 1,
                        &data->notificationParams.changeOfValue.statusFlags);
                    apdu_len += len;
                    if (apdu) {
                        apdu += len;
                    }
                    len = encode_closing_tag(apdu, 2);
                    apdu_len += len;
                    if (apdu) {
                        apdu += len;
                    }
                    break;
                case EVENT_COMMAND_FAILURE:
                    len = encode_opening_tag(apdu, 3);
                    apdu_len += len;
                    if (apdu) {
                        apdu += len;
                    }
                    len = encode_opening_tag(apdu, 0);
                    apdu_len += len;
                    if (apdu) {
                        apdu += len;
                    }
                    switch (data->notificationParams.commandFailure.tag) {
                        case COMMAND_FAILURE_BINARY_PV:
                            len = encode_application_enumerated(
                                apdu,
                                data->notificationParams.commandFailure
                                    .commandValue.binaryValue);
                            apdu_len += len;
                            if (apdu) {
                                apdu += len;
                            }
                            break;
                        case COMMAND_FAILURE_UNSIGNED:
                            len = encode_application_unsigned(
                                apdu,
                                data->notificationParams.commandFailure
                                    .commandValue.unsignedValue);
                            apdu_len += len;
                            if (apdu) {
                                apdu += len;
                            }
                            break;
                        default:
                            return 0;
                    }
                    len = encode_closing_tag(apdu, 0);
                    apdu_len += len;
                    if (apdu) {
                        apdu += len;
                    }
                    len = encode_context_bitstring(
                        apdu, 1,
                        &data->notificationParams.commandFailure.statusFlags);
                    apdu_len += len;
                    if (apdu) {
                        apdu += len;
                    }
                    len = encode_opening_tag(apdu, 2);
                    apdu_len += len;
                    if (apdu) {
                        apdu += len;
                    }
                    switch (data->notificationParams.commandFailure.tag) {
                        case COMMAND_FAILURE_BINARY_PV:
                            len = encode_application_enumerated(
                                apdu,
                                data->notificationParams.commandFailure
                                    .feedbackValue.binaryValue);
                            apdu_len += len;
                            if (apdu) {
                                apdu += len;
                            }
                            break;
                        case COMMAND_FAILURE_UNSIGNED:
                            len = encode_application_unsigned(
                                apdu,
                                data->notificationParams.commandFailure
                                    .feedbackValue.unsignedValue);
                            apdu_len += len;
                            if (apdu) {
                                apdu += len;
                            }
                            break;
                        default:
                            return 0;
                    }
                    len = encode_closing_tag(apdu, 2);
                    apdu_len += len;
                    if (apdu) {
                        apdu += len;
                    }
                    len = encode_closing_tag(apdu, 3);
                    apdu_len += len;
                    if (apdu) {
                        apdu += len;
                    }
                    break;
                case EVENT_FLOATING_LIMIT:
                    len = encode_opening_tag(apdu, 4);
                    apdu_len += len;
                    if (apdu) {
                        apdu += len;
                    }
                    len = encode_context_real(
                        apdu, 0,
                        data->notificationParams.floatingLimit.referenceValue);
                    apdu_len += len;
                    if (apdu) {
                        apdu += len;
                    }
                    len = encode_context_bitstring(
                        apdu, 1,
                        &data->notificationParams.floatingLimit.statusFlags);
                    apdu_len += len;
                    if (apdu) {
                        apdu += len;
                    }
                    len = encode_context_real(
                        apdu, 2,
                        data->notificationParams.floatingLimit.setPointValue);
                    apdu_len += len;
                    if (apdu) {
                        apdu += len;
                    }
                    len = encode_context_real(
                        apdu, 3,
                        data->notificationParams.floatingLimit.errorLimit);
                    apdu_len += len;
                    if (apdu) {
                        apdu += len;
                    }
                    len = encode_closing_tag(apdu, 4);
                    apdu_len += len;
                    if (apdu) {
                        apdu += len;
                    }
                    break;
                case EVENT_OUT_OF_RANGE:
                    len = encode_opening_tag(apdu, 5);
                    apdu_len += len;
                    if (apdu) {
                        apdu += len;
                    }
                    len = encode_context_real(
                        apdu, 0,
                        data->notificationParams.outOfRange.exceedingValue);
                    apdu_len += len;
                    if (apdu) {
                        apdu += len;
                    }
                    len = encode_context_bitstring(
                        apdu, 1,
                        &data->notificationParams.outOfRange.statusFlags);
                    apdu_len += len;
                    if (apdu) {
                        apdu += len;
                    }
                    len = encode_context_real(
                        apdu, 2, data->notificationParams.outOfRange.deadband);
                    apdu_len += len;
                    if (apdu) {
                        apdu += len;
                    }
                    len = encode_context_real(
                        apdu, 3,
                        data->notificationParams.outOfRange.exceededLimit);
                    apdu_len += len;
                    if (apdu) {
                        apdu += len;
                    }
                    len = encode_closing_tag(apdu, 5);
                    apdu_len += len;
                    if (apdu) {
                        apdu += len;
                    }
                    break;

                case EVENT_CHANGE_OF_LIFE_SAFETY:
                    len = encode_opening_tag(apdu, 8);
                    apdu_len += len;
                    if (apdu) {
                        apdu += len;
                    }
                    len = encode_context_enumerated(
                        apdu, 0,
                        data->notificationParams.changeOfLifeSafety.newState);
                    apdu_len += len;
                    if (apdu) {
                        apdu += len;
                    }
                    len = encode_context_enumerated(
                        apdu, 1,
                        data->notificationParams.changeOfLifeSafety.newMode);
                    apdu_len += len;
                    if (apdu) {
                        apdu += len;
                    }
                    len = encode_context_bitstring(
                        apdu, 2,
                        &data->notificationParams.changeOfLifeSafety
                             .statusFlags);
                    apdu_len += len;
                    if (apdu) {
                        apdu += len;
                    }
                    len = encode_context_enumerated(
                        apdu, 3,
                        data->notificationParams.changeOfLifeSafety
                            .operationExpected);
                    apdu_len += len;
                    if (apdu) {
                        apdu += len;
                    }
                    len = encode_closing_tag(apdu, 8);
                    apdu_len += len;
                    if (apdu) {
                        apdu += len;
                    }
                    break;

                case EVENT_BUFFER_READY:
                    len = encode_opening_tag(apdu, 10);
                    apdu_len += len;
                    if (apdu) {
                        apdu += len;
                    }
                    len = bacapp_encode_context_device_obj_property_ref(
                        apdu, 0,
                        &data->notificationParams.bufferReady.bufferProperty);
                    apdu_len += len;
                    if (apdu) {
                        apdu += len;
                    }
                    len = encode_context_unsigned(
                        apdu, 1,
                        data->notificationParams.bufferReady
                            .previousNotification);
                    apdu_len += len;
                    if (apdu) {
                        apdu += len;
                    }
                    len = encode_context_unsigned(
                        apdu, 2,
                        data->notificationParams.bufferReady
                            .currentNotification);
                    apdu_len += len;
                    if (apdu) {
                        apdu += len;
                    }
                    len = encode_closing_tag(apdu, 10);
                    apdu_len += len;
                    if (apdu) {
                        apdu += len;
                    }
                    break;
                case EVENT_UNSIGNED_RANGE:
                    len = encode_opening_tag(apdu, 11);
                    apdu_len += len;
                    if (apdu) {
                        apdu += len;
                    }
                    len = encode_context_unsigned(
                        apdu, 0,
                        data->notificationParams.unsignedRange.exceedingValue);
                    apdu_len += len;
                    if (apdu) {
                        apdu += len;
                    }
                    len = encode_context_bitstring(
                        apdu, 1,
                        &data->notificationParams.unsignedRange.statusFlags);
                    apdu_len += len;
                    if (apdu) {
                        apdu += len;
                    }
                    len = encode_context_unsigned(
                        apdu, 2,
                        data->notificationParams.unsignedRange.exceededLimit);
                    apdu_len += len;
                    if (apdu) {
                        apdu += len;
                    }
                    len = encode_closing_tag(apdu, 11);
                    apdu_len += len;
                    if (apdu) {
                        apdu += len;
                    }
                    break;
                case EVENT_ACCESS_EVENT:
                    len = encode_opening_tag(apdu, 13);
                    apdu_len += len;
                    if (apdu) {
                        apdu += len;
                    }
                    len = encode_context_enumerated(
                        apdu, 0,
                        data->notificationParams.accessEvent.accessEvent);
                    apdu_len += len;
                    if (apdu) {
                        apdu += len;
                    }
                    len = encode_context_bitstring(
                        apdu, 1,
                        &data->notificationParams.accessEvent.statusFlags);
                    apdu_len += len;
                    if (apdu) {
                        apdu += len;
                    }
                    len = encode_context_unsigned(
                        apdu, 2,
                        data->notificationParams.accessEvent.accessEventTag);
                    apdu_len += len;
                    if (apdu) {
                        apdu += len;
                    }
                    len = bacapp_encode_context_timestamp(
                        apdu, 3,
                        &data->notificationParams.accessEvent.accessEventTime);
                    apdu_len += len;
                    if (apdu) {
                        apdu += len;
                    }
                    len = bacapp_encode_context_device_obj_ref(
                        apdu, 4,
                        &data->notificationParams.accessEvent.accessCredential);
                    apdu_len += len;
                    if (apdu) {
                        apdu += len;
                    }
                    if (data->notificationParams.accessEvent
                            .authenticationFactor.format_type <
                        AUTHENTICATION_FACTOR_MAX) {
                        len = bacapp_encode_context_authentication_factor(
                            apdu, 5,
                            &data->notificationParams.accessEvent
                                 .authenticationFactor);
                        apdu_len += len;
                        if (apdu) {
                            apdu += len;
                        }
                    }
                    len = encode_closing_tag(apdu, 13);
                    apdu_len += len;
                    if (apdu) {
                        apdu += len;
                    }
                    break;
                case EVENT_EXTENDED:
                default:
                    assert(0);
                    break;
            }
            len = encode_closing_tag(apdu, 12);
            apdu_len += len;
            break;
        case NOTIFY_ACK_NOTIFICATION:
            /* FIXME: handle this case */
        default:
            break;
    }

    return apdu_len;
}

/**
 * @brief Encode the EventNotification service request
 * @param apdu  Pointer to the buffer for encoding into
 * @param apdu_size number of bytes available in the buffer
 * @param data  Pointer to the service data used for encoding values
 * @return number of bytes encoded, or zero if unable to encode or too large
 */
size_t event_notification_service_request_encode(
    uint8_t *apdu, size_t apdu_size, const BACNET_EVENT_NOTIFICATION_DATA *data)
{
    size_t apdu_len = 0; /* total length of the apdu, return value */

    apdu_len = event_notify_encode_service_request(NULL, data);
    if (apdu_len > apdu_size) {
        apdu_len = 0;
    } else {
        apdu_len = event_notify_encode_service_request(apdu, data);
    }

    return apdu_len;
}

/**
 * @brief Decode the EventNotification service request only.
 * @details Confirmed and Unconfirmed are the same encoding
 *  UnconfirmedEventNotification-Request ::= SEQUENCE {
 *  ConfirmedEventNotification-Request ::= SEQUENCE {
 *      process-identifier[0] Unsigned32,
 *      initiating-device-identifier[1] BACnetObjectIdentifier,
 *      event-object-identifier[2] BACnetObjectIdentifier,
 *      timestamp[3] BACnetTimeStamp,
 *      notification-class[4] Unsigned,
 *      priority[5] Unsigned8,
 *      event-type[6] BACnetEventType,
 *      message-text[7] CharacterString OPTIONAL,
 *      notify-type[8] BACnetNotifyType,
 *      ack-required[9] Boolean OPTIONAL,
 *      from-state[10] BACnetEventState OPTIONAL,
 *      to-state[11] BACnetEventState,
 *      event-values[12] BACnetNotificationParameters OPTIONAL
 *  }
 *
 * @param apdu  Pointer to the buffer.
 * @param apdu_size  Number of valid bytes in the buffer.
 * @param data  Pointer to the data to store the decoded values, or NULL
 *
 * @return Bytes decoded or BACNET_STATUS_ERROR on error.
 */
int event_notify_decode_service_request(
    const uint8_t *apdu,
    unsigned apdu_size,
    BACNET_EVENT_NOTIFICATION_DATA *data)
{
    int apdu_len = 0; /* return value */
    int len = 0;
    BACNET_UNSIGNED_INTEGER unsigned_value = 0;
    BACNET_OBJECT_ID object_id = { 0 };
    BACNET_TIMESTAMP timestamp_value = { 0 };
    BACNET_CHARACTER_STRING *cstring = NULL;
    BACNET_BIT_STRING *bstring = NULL;
    BACNET_PROPERTY_STATE *property_state = NULL;
    BACNET_NOTIFY_TYPE notify_type = NOTIFY_MAX;
    BACNET_EVENT_TYPE event_type = EVENT_NONE;
    bool boolean_value = false;
    float real_value = 0.0f;
    uint32_t enum_value = 0;

    if (apdu_size) {
        /* process-identifier[0] Unsigned32 */
        len = bacnet_unsigned_context_decode(
            &apdu[apdu_len], apdu_size - apdu_len, 0, &unsigned_value);
        if (len > 0) {
            apdu_len += len;
            if (unsigned_value <= UINT32_MAX) {
                if (data) {
                    data->processIdentifier = (uint32_t)unsigned_value;
                }
            } else {
                return BACNET_STATUS_ERROR;
            }
        } else {
            return BACNET_STATUS_ERROR;
        }
        /* initiating-device-identifier[1] BACnetObjectIdentifier */
        len = bacnet_object_id_context_decode(
            &apdu[apdu_len], apdu_size - apdu_len, 1, &object_id.type,
            &object_id.instance);
        if (len > 0) {
            apdu_len += len;
            if (data) {
                data->initiatingObjectIdentifier.type = object_id.type;
                data->initiatingObjectIdentifier.instance = object_id.instance;
            }
        } else {
            return BACNET_STATUS_ERROR;
        }
        /* event-object-identifier[2] BACnetObjectIdentifier */
        len = bacnet_object_id_context_decode(
            &apdu[apdu_len], apdu_size - apdu_len, 2, &object_id.type,
            &object_id.instance);
        if (len > 0) {
            apdu_len += len;
            if (data) {
                data->eventObjectIdentifier.type = object_id.type;
                data->eventObjectIdentifier.instance = object_id.instance;
            }
        } else {
            return BACNET_STATUS_ERROR;
        }
        /* timestamp[3] BACnetTimeStamp */
        len = bacnet_timestamp_context_decode(
            &apdu[apdu_len], apdu_size - apdu_len, 3, &timestamp_value);
        if (len > 0) {
            apdu_len += len;
            if (data) {
                bacapp_timestamp_copy(&data->timeStamp, &timestamp_value);
            }
        } else {
            return BACNET_STATUS_ERROR;
        }
        /* notification-class[4] Unsigned */
        len = bacnet_unsigned_context_decode(
            &apdu[apdu_len], apdu_size - apdu_len, 4, &unsigned_value);
        if (len > 0) {
            apdu_len += len;
            if (unsigned_value <= UINT32_MAX) {
                data->notificationClass = (uint32_t)unsigned_value;
            } else {
                return BACNET_STATUS_ERROR;
            }
        } else {
            return BACNET_STATUS_ERROR;
        }
        /* priority[5] Unsigned8 */
        len = bacnet_unsigned_context_decode(
            &apdu[apdu_len], apdu_size - apdu_len, 5, &unsigned_value);
        if (len > 0) {
            apdu_len += len;
            if (unsigned_value <= UINT8_MAX) {
                data->priority = (uint8_t)unsigned_value;
            } else {
                return BACNET_STATUS_ERROR;
            }
        } else {
            return BACNET_STATUS_ERROR;
        }
        /* event-type[6] BACnetEventType */
        len = bacnet_enumerated_context_decode(
            &apdu[apdu_len], apdu_size - apdu_len, 6, &enum_value);
        if (len > 0) {
            apdu_len += len;
            if (enum_value <= EVENT_PROPRIETARY_MAX) {
                event_type = (BACNET_EVENT_TYPE)enum_value;
                if (data) {
                    data->eventType = event_type;
                }
            } else {
                return BACNET_STATUS_ERROR;
            }
        } else {
            return BACNET_STATUS_ERROR;
        }
        /* message-text[7] CharacterString OPTIONAL */
        if (data) {
            cstring = data->messageText;
        } else {
            cstring = NULL;
        }
        len = bacnet_character_string_context_decode(
            &apdu[apdu_len], apdu_size - apdu_len, 7, cstring);
        if (len > 0) {
            apdu_len += len;
        } else if (len == 0) {
            /* OPTIONAL - set default */
            characterstring_init_ansi(cstring, "");
        } else {
            return BACNET_STATUS_ERROR;
        }
        /* notify-type[8] BACnetNotifyType */
        len = bacnet_enumerated_context_decode(
            &apdu[apdu_len], apdu_size - apdu_len, 8, &enum_value);
        if (len > 0) {
            apdu_len += len;
            if (data) {
                data->notifyType = (BACNET_NOTIFY_TYPE)enum_value;
                if (enum_value >= NOTIFY_MAX) {
                    enum_value = NOTIFY_MAX;
                }
                notify_type = enum_value;
            }
        } else {
            return BACNET_STATUS_ERROR;
        }
        if ((notify_type == NOTIFY_ALARM) || (notify_type == NOTIFY_EVENT)) {
            /* ack-required[9] Boolean OPTIONAL */
            len = bacnet_boolean_context_decode(
                &apdu[apdu_len], apdu_size - apdu_len, 9, &boolean_value);
            if (len > 0) {
                apdu_len += len;
                if (data) {
                    data->ackRequired = boolean_value;
                }
            } else if (len == 0) {
                /* OPTIONAL - set default */
                if (data) {
                    data->ackRequired = false;
                }
            } else {
                return BACNET_STATUS_ERROR;
            }
            /* from-state[10] BACnetEventState OPTIONAL */
            len = bacnet_enumerated_context_decode(
                &apdu[apdu_len], apdu_size - apdu_len, 10, &enum_value);
            if (len > 0) {
                apdu_len += len;
                if (data) {
                    data->fromState = (BACNET_EVENT_STATE)enum_value;
                }
            } else if (len == 0) {
                /* OPTIONAL - set default out of range */
                if (data) {
                    data->fromState = EVENT_STATE_MAX;
                }
            } else {
                return BACNET_STATUS_ERROR;
            }
        }
        /* to-state[11] BACnetEventState */
        len = bacnet_enumerated_context_decode(
            &apdu[apdu_len], apdu_size - apdu_len, 11, &enum_value);
        if (len > 0) {
            apdu_len += len;
            if (data) {
                data->toState = (BACNET_EVENT_STATE)enum_value;
            }
        } else {
            return BACNET_STATUS_ERROR;
        }
    }
    if ((notify_type == NOTIFY_ALARM) || (notify_type == NOTIFY_EVENT)) {
        /* event-values[12] BACnetNotificationParameters OPTIONAL */
        if (bacnet_is_opening_tag_number(
                &apdu[apdu_len], apdu_size - apdu_len, 12, &len)) {
            apdu_len += len;
        } else {
            return BACNET_STATUS_ERROR;
        }
        if (event_type >= EVENT_PROPRIETARY_MIN) {
            /* complex-event-type [6] SEQUENCE OF BACnetPropertyValue */
            if (bacnet_is_opening_tag_number(
                    &apdu[apdu_len], apdu_size - apdu_len,
                    EVENT_COMPLEX_EVENT_TYPE, &len)) {
                apdu_len += len;
                len = complex_event_type_values_decode(
                    &apdu[apdu_len], apdu_size - apdu_len, data);
                if (apdu_len < 0) {
                    return BACNET_STATUS_ERROR;
                }
                if (bacnet_is_closing_tag_number(
                        &apdu[apdu_len], apdu_size - apdu_len,
                        EVENT_COMPLEX_EVENT_TYPE, &len)) {
                    apdu_len += len;
                } else {
                    return BACNET_STATUS_ERROR;
                }
            } else {
                return BACNET_STATUS_ERROR;
            }
        } else if (bacnet_is_opening_tag_number(
                       &apdu[apdu_len], apdu_size - apdu_len,
                       (uint8_t)event_type, &len)) {
            /* BACnetNotificationParameters */
            apdu_len += len;
            switch (event_type) {
                case EVENT_CHANGE_OF_BITSTRING:
                    /* referenced-bitstring[0] BitString */
                    if (data) {
                        bstring = &data->notificationParams.changeOfBitstring
                                       .referencedBitString;
                    } else {
                        bstring = NULL;
                    }
                    len = bacnet_bitstring_context_decode(
                        &apdu[apdu_len], apdu_size - apdu_len, 0, bstring);
                    if (len > 0) {
                        apdu_len += len;
                    } else {
                        return BACNET_STATUS_ERROR;
                    }
                    /* status-flags[1] BACnetStatusFlags */
                    if (data) {
                        bstring = &data->notificationParams.changeOfBitstring
                                       .statusFlags;
                    } else {
                        bstring = NULL;
                    }
                    len = bacnet_bitstring_context_decode(
                        &apdu[apdu_len], apdu_size - apdu_len, 1, bstring);
                    if (len > 0) {
                        apdu_len += len;
                    } else {
                        return BACNET_STATUS_ERROR;
                    }
                    break;
                case EVENT_CHANGE_OF_STATE:
                    /* new-state[0] BACnetEventState */
                    if (data) {
                        property_state =
                            &data->notificationParams.changeOfState.newState;
                    } else {
                        property_state = NULL;
                    }
                    len = bacapp_decode_context_property_state(
                        &apdu[apdu_len], 0, property_state);
                    if (len > 0) {
                        apdu_len += len;
                    } else {
                        return BACNET_STATUS_ERROR;
                    }
                    /* status-flags[1] BACnetStatusFlags */
                    if (data) {
                        bstring =
                            &data->notificationParams.changeOfState.statusFlags;
                    } else {
                        bstring = NULL;
                    }
                    len = bacnet_bitstring_context_decode(
                        &apdu[apdu_len], apdu_size - apdu_len, 1, bstring);
                    if (len > 0) {
                        apdu_len += len;
                    } else {
                        return BACNET_STATUS_ERROR;
                    }
                    break;
                case EVENT_CHANGE_OF_VALUE:
                    /* new-value [0] CHOICE */
                    if (bacnet_is_opening_tag_number(
                            &apdu[apdu_len], apdu_size - apdu_len, 0, &len)) {
                        apdu_len += len;
                    } else {
                        return BACNET_STATUS_ERROR;
                    }
                    /* changed-bits[0] BitString */
                    if (data) {
                        bstring = &data->notificationParams.changeOfValue
                                       .newValue.changedBits;
                    } else {
                        bstring = NULL;
                    }
                    len = bacnet_bitstring_context_decode(
                        &apdu[apdu_len], apdu_size - apdu_len, 0, bstring);
                    if (len > 0) {
                        apdu_len += len;
                        if (data) {
                            data->notificationParams.changeOfValue.tag =
                                CHANGE_OF_VALUE_BITS;
                        }
                    } else if (len < 0) {
                        return BACNET_STATUS_ERROR;
                    } else {
                        /* changed-value[1] Real */
                        len = bacnet_real_context_decode(
                            &apdu[apdu_len], apdu_size - apdu_len, 1,
                            &real_value);
                        if (len > 0) {
                            apdu_len += len;
                            if (data) {
                                data->notificationParams.changeOfValue.newValue
                                    .changeValue = real_value;
                                data->notificationParams.changeOfValue.tag =
                                    CHANGE_OF_VALUE_REAL;
                            }
                        } else {
                            return BACNET_STATUS_ERROR;
                        }
                    }
                    if (bacnet_is_closing_tag_number(
                            &apdu[apdu_len], apdu_size - apdu_len, 0, &len)) {
                        apdu_len += len;
                    } else {
                        return BACNET_STATUS_ERROR;
                    }
                    /* status-flags [1] BACnetStatusFlags*/
                    if (data) {
                        bstring =
                            &data->notificationParams.changeOfValue.statusFlags;
                    } else {
                        bstring = NULL;
                    }
                    len = bacnet_bitstring_context_decode(
                        &apdu[apdu_len], apdu_size - apdu_len, 1, bstring);
                    if (len > 0) {
                        apdu_len += len;
                    } else {
                        return BACNET_STATUS_ERROR;
                    }
                    break;
                case EVENT_COMMAND_FAILURE:
                    /* command-value[0] ABSTRACT-SYNTAX.&Type,
                       -- depends on ref property */
                    if (bacnet_is_opening_tag_number(
                            &apdu[apdu_len], apdu_size - apdu_len, 0, &len)) {
                        apdu_len += len;
                    } else {
                        return BACNET_STATUS_ERROR;
                    }
                    len = bacnet_enumerated_application_decode(
                        &apdu[apdu_len], apdu_size - apdu_len, &enum_value);
                    if (len > 0) {
                        apdu_len += len;
                        if (data) {
                            data->notificationParams.commandFailure.commandValue
                                .binaryValue = enum_value;
                            data->notificationParams.commandFailure.tag =
                                COMMAND_FAILURE_BINARY_PV;
                        }
                    } else if (len < 0) {
                        return BACNET_STATUS_ERROR;
                    }
                    if (len == 0) {
                        len = bacnet_unsigned_application_decode(
                            &apdu[apdu_len], apdu_size - apdu_len,
                            &unsigned_value);
                        if (len > 0) {
                            apdu_len += len;
                            if (data) {
                                data->notificationParams.commandFailure
                                    .commandValue.unsignedValue =
                                    unsigned_value;
                                data->notificationParams.commandFailure.tag =
                                    COMMAND_FAILURE_UNSIGNED;
                            }
                        } else {
                            return BACNET_STATUS_ERROR;
                        }
                    }
                    if (bacnet_is_closing_tag_number(
                            &apdu[apdu_len], apdu_size - apdu_len, 0, &len)) {
                        apdu_len += len;
                    } else {
                        return BACNET_STATUS_ERROR;
                    }
                    /* status-flags[1] BACnetStatusFlags */
                    if (data) {
                        bstring = &data->notificationParams.commandFailure
                                       .statusFlags;
                    } else {
                        bstring = NULL;
                    }
                    len = bacnet_bitstring_context_decode(
                        &apdu[apdu_len], apdu_size - apdu_len, 1, bstring);
                    if (len > 0) {
                        apdu_len += len;
                    } else {
                        return BACNET_STATUS_ERROR;
                    }
                    /*  feedback-value[2] ABSTRACT-SYNTAX.&Type
                        -- depends on ref property */
                    if (bacnet_is_opening_tag_number(
                            &apdu[apdu_len], apdu_size - apdu_len, 2, &len)) {
                        apdu_len += len;
                    } else {
                        return BACNET_STATUS_ERROR;
                    }
                    if (data->notificationParams.commandFailure.tag ==
                        COMMAND_FAILURE_BINARY_PV) {
                        len = bacnet_enumerated_application_decode(
                            &apdu[apdu_len], apdu_size - apdu_len, &enum_value);
                        if (len > 0) {
                            apdu_len += len;
                            if (data) {
                                data->notificationParams.commandFailure
                                    .feedbackValue.binaryValue = enum_value;
                            }
                        } else if (len < 0) {
                            return BACNET_STATUS_ERROR;
                        }
                    } else {
                        len = bacnet_unsigned_application_decode(
                            &apdu[apdu_len], apdu_size - apdu_len,
                            &unsigned_value);
                        if (len > 0) {
                            apdu_len += len;
                            if (data) {
                                data->notificationParams.commandFailure
                                    .feedbackValue.unsignedValue =
                                    unsigned_value;
                            }
                        } else {
                            return BACNET_STATUS_ERROR;
                        }
                    }
                    if (bacnet_is_closing_tag_number(
                            &apdu[apdu_len], apdu_size - apdu_len, 2, &len)) {
                        apdu_len += len;
                    } else {
                        return BACNET_STATUS_ERROR;
                    }
                    break;
                case EVENT_FLOATING_LIMIT:
                    if (-1 ==
                        (len = decode_context_real(
                             &apdu[apdu_len], 0,
                             &data->notificationParams.floatingLimit
                                  .referenceValue))) {
                        return -1;
                    }
                    apdu_len += len;

                    if (-1 ==
                        (len = decode_context_bitstring(
                             &apdu[apdu_len], 1,
                             &data->notificationParams.floatingLimit
                                  .statusFlags))) {
                        return -1;
                    }
                    apdu_len += len;
                    if (-1 ==
                        (len = decode_context_real(
                             &apdu[apdu_len], 2,
                             &data->notificationParams.floatingLimit
                                  .setPointValue))) {
                        return -1;
                    }
                    apdu_len += len;

                    if (-1 ==
                        (len = decode_context_real(
                             &apdu[apdu_len], 3,
                             &data->notificationParams.floatingLimit
                                  .errorLimit))) {
                        return -1;
                    }
                    apdu_len += len;
                    break;

                case EVENT_OUT_OF_RANGE:
                    if (-1 ==
                        (len = decode_context_real(
                             &apdu[apdu_len], 0,
                             &data->notificationParams.outOfRange
                                  .exceedingValue))) {
                        return -1;
                    }
                    apdu_len += len;

                    if (-1 ==
                        (len = decode_context_bitstring(
                             &apdu[apdu_len], 1,
                             &data->notificationParams.outOfRange
                                  .statusFlags))) {
                        return -1;
                    }
                    apdu_len += len;
                    if (-1 ==
                        (len = decode_context_real(
                             &apdu[apdu_len], 2,
                             &data->notificationParams.outOfRange.deadband))) {
                        return -1;
                    }
                    apdu_len += len;

                    if (-1 ==
                        (len = decode_context_real(
                             &apdu[apdu_len], 3,
                             &data->notificationParams.outOfRange
                                  .exceededLimit))) {
                        return -1;
                    }
                    apdu_len += len;
                    break;

                case EVENT_CHANGE_OF_LIFE_SAFETY:
                    if (-1 ==
                        (len = decode_context_enumerated(
                             &apdu[apdu_len], 0, &enum_value))) {
                        return -1;
                    }
                    data->notificationParams.changeOfLifeSafety.newState =
                        (BACNET_LIFE_SAFETY_STATE)enum_value;
                    apdu_len += len;

                    if (-1 ==
                        (len = decode_context_enumerated(
                             &apdu[apdu_len], 1, &enum_value))) {
                        return -1;
                    }
                    data->notificationParams.changeOfLifeSafety.newMode =
                        (BACNET_LIFE_SAFETY_MODE)enum_value;
                    apdu_len += len;

                    if (-1 ==
                        (len = decode_context_bitstring(
                             &apdu[apdu_len], 2,
                             &data->notificationParams.changeOfLifeSafety
                                  .statusFlags))) {
                        return -1;
                    }
                    apdu_len += len;

                    if (-1 ==
                        (len = decode_context_enumerated(
                             &apdu[apdu_len], 3, &enum_value))) {
                        return -1;
                    }
                    data->notificationParams.changeOfLifeSafety
                        .operationExpected =
                        (BACNET_LIFE_SAFETY_OPERATION)enum_value;
                    apdu_len += len;
                    break;

                case EVENT_BUFFER_READY:
                    /* Tag 0 - bufferProperty */
                    len =
                        bacnet_device_object_property_reference_context_decode(
                            &apdu[apdu_len], apdu_size - apdu_len, 0,
                            &data->notificationParams.bufferReady
                                 .bufferProperty);
                    if (len <= 0) {
                        return -1;
                    }
                    apdu_len += len;
                    /* Tag 1 - PreviousNotification */
                    len = bacnet_unsigned_context_decode(
                        &apdu[apdu_len], apdu_size - apdu_len, 1,
                        &unsigned_value);
                    if (len > 0) {
                        apdu_len += len;
                        if (unsigned_value <= UINT32_MAX) {
                            data->notificationParams.bufferReady
                                .previousNotification =
                                (uint32_t)unsigned_value;
                        } else {
                            return BACNET_STATUS_ERROR;
                        }
                    } else {
                        return BACNET_STATUS_ERROR;
                    }
                    /* Tag 2 - currentNotification */
                    len = bacnet_unsigned_context_decode(
                        &apdu[apdu_len], apdu_size - apdu_len, 2,
                        &unsigned_value);
                    if (len > 0) {
                        apdu_len += len;
                        if (unsigned_value <= UINT32_MAX) {
                            data->notificationParams.bufferReady
                                .currentNotification = (uint32_t)unsigned_value;
                        } else {
                            return BACNET_STATUS_ERROR;
                        }
                    } else {
                        return BACNET_STATUS_ERROR;
                    }
                    break;

                case EVENT_UNSIGNED_RANGE:
                    /* Tag 0 - PreviousNotification */
                    len = bacnet_unsigned_context_decode(
                        &apdu[apdu_len], apdu_size - apdu_len, 0,
                        &unsigned_value);
                    if (len > 0) {
                        apdu_len += len;
                        if (unsigned_value <= UINT32_MAX) {
                            data->notificationParams.unsignedRange
                                .exceedingValue = (uint32_t)unsigned_value;
                        } else {
                            return BACNET_STATUS_ERROR;
                        }
                    } else {
                        return BACNET_STATUS_ERROR;
                    }
                    /* Tag 1 - statusFlags */
                    if (-1 ==
                        (len = decode_context_bitstring(
                             &apdu[apdu_len], 1,
                             &data->notificationParams.unsignedRange
                                  .statusFlags))) {
                        return -1;
                    }
                    apdu_len += len;
                    /* Tag 2 - exceededLimit */
                    len = bacnet_unsigned_context_decode(
                        &apdu[apdu_len], apdu_size - apdu_len, 2,
                        &unsigned_value);
                    if (len > 0) {
                        apdu_len += len;
                        if (unsigned_value <= UINT32_MAX) {
                            data->notificationParams.unsignedRange
                                .exceededLimit = (uint32_t)unsigned_value;
                        } else {
                            return BACNET_STATUS_ERROR;
                        }
                    } else {
                        return BACNET_STATUS_ERROR;
                    }
                    break;

                case EVENT_ACCESS_EVENT:
                    if (-1 ==
                        (len = decode_context_enumerated(
                             &apdu[apdu_len], 0, &enum_value))) {
                        return -1;
                    }
                    data->notificationParams.accessEvent.accessEvent =
                        enum_value;
                    apdu_len += len;
                    if (-1 ==
                        (len = decode_context_bitstring(
                             &apdu[apdu_len], 1,
                             &data->notificationParams.accessEvent
                                  .statusFlags))) {
                        return -1;
                    }
                    apdu_len += len;

                    if (-1 ==
                        (len = decode_context_unsigned(
                             &apdu[apdu_len], 2,
                             &data->notificationParams.accessEvent
                                  .accessEventTag))) {
                        return -1;
                    }
                    apdu_len += len;

                    if (-1 ==
                        (len = bacapp_decode_context_timestamp(
                             &apdu[apdu_len], 3,
                             &data->notificationParams.accessEvent
                                  .accessEventTime))) {
                        return -1;
                    }
                    apdu_len += len;

                    if (-1 ==
                        (len = bacapp_decode_context_device_obj_ref(
                             &apdu[apdu_len], 4,
                             &data->notificationParams.accessEvent
                                  .accessCredential))) {
                        return -1;
                    }
                    apdu_len += len;

                    if (!decode_is_closing_tag(&apdu[apdu_len])) {
                        if (-1 ==
                            (len = bacapp_decode_context_authentication_factor(
                                 &apdu[apdu_len], 5,
                                 &data->notificationParams.accessEvent
                                      .authenticationFactor))) {
                            return -1;
                        }
                        apdu_len += len;
                    }
                    break;

                default:
                    return -1;
            }
            if (bacnet_is_closing_tag_number(
                    &apdu[apdu_len], apdu_size - apdu_len, (uint8_t)event_type,
                    &len)) {
                apdu_len += len;
            } else {
                return BACNET_STATUS_ERROR;
            }
            if (bacnet_is_closing_tag_number(
                    &apdu[apdu_len], apdu_size - apdu_len, 12, &len)) {
                apdu_len += len;
            } else {
                return BACNET_STATUS_ERROR;
            }
        }
    }

    return apdu_len;
}
