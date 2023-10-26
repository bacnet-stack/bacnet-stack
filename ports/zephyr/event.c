/*####COPYRIGHTBEGIN####
 -------------------------------------------
 Copyright (C) 2008 John Minack

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
#include <assert.h>
#include "bacnet/event.h"
#include "bacnet/bacdcode.h"
#include "bacnet/npdu.h"
#include "bacnet/timestamp.h"
#include "bacnet/authentication_factor.h"

/** @file event.c  Encode/Decode Event Notifications */

int uevent_notify_encode_apdu(
    uint8_t *apdu, BACNET_EVENT_NOTIFICATION_DATA *data)
{
    int len = 0; /* length of each encoding */
    int apdu_len = 0; /* total length of the apdu, return value */

    if (apdu) {
        apdu[0] = PDU_TYPE_UNCONFIRMED_SERVICE_REQUEST;
        apdu[1] = SERVICE_UNCONFIRMED_EVENT_NOTIFICATION; /* service choice */
        apdu_len = 2;

        len += event_notify_encode_service_request(&apdu[apdu_len], data);

        if (len > 0) {
            apdu_len += len;
        } else {
            apdu_len = 0;
        }
    }

    return apdu_len;
}

int cevent_notify_encode_apdu(
    uint8_t *apdu, uint8_t invoke_id, BACNET_EVENT_NOTIFICATION_DATA *data)
{
    int len = 0; /* length of each encoding */
    int apdu_len = 0; /* total length of the apdu, return value */

    if (apdu) {
        apdu[0] = PDU_TYPE_CONFIRMED_SERVICE_REQUEST;
        apdu[1] = encode_max_segs_max_apdu(0, MAX_APDU);
        apdu[2] = invoke_id;
        apdu[3] = SERVICE_CONFIRMED_EVENT_NOTIFICATION; /* service choice */
        apdu_len = 4;

        len += event_notify_encode_service_request(&apdu[apdu_len], data);

        if (len > 0) {
            apdu_len += len;
        } else {
            apdu_len = 0;
        }
    }

    return apdu_len;
}

int event_notify_encode_service_request(
    uint8_t *apdu, BACNET_EVENT_NOTIFICATION_DATA *data)
{
    int len = 0; /* length of each encoding */
    int apdu_len = 0; /* total length of the apdu, return value */

    if (apdu) {
        /* tag 0 - processIdentifier */
        len = encode_context_unsigned(
            &apdu[apdu_len], 0, data->processIdentifier);
        apdu_len += len;
        /* tag 1 - initiatingObjectIdentifier */
        len = encode_context_object_id(&apdu[apdu_len], 1,
            data->initiatingObjectIdentifier.type,
            data->initiatingObjectIdentifier.instance);
        apdu_len += len;

        /* tag 2 - eventObjectIdentifier */
        len = encode_context_object_id(&apdu[apdu_len], 2,
            data->eventObjectIdentifier.type,
            data->eventObjectIdentifier.instance);
        apdu_len += len;

        /* tag 3 - timeStamp */

        len = bacapp_encode_context_timestamp(
            &apdu[apdu_len], 3, &data->timeStamp);
        apdu_len += len;

        /* tag 4 - noticicationClass */

        len = encode_context_unsigned(
            &apdu[apdu_len], 4, data->notificationClass);
        apdu_len += len;

        /* tag 5 - priority */

        len = encode_context_unsigned(&apdu[apdu_len], 5, data->priority);
        apdu_len += len;

        /* tag 6 - eventType */
        len = encode_context_enumerated(&apdu[apdu_len], 6, data->eventType);
        apdu_len += len;

        /* tag 7 - messageText */
        if (data->messageText) {
            len = encode_context_character_string(
                &apdu[apdu_len], 7, data->messageText);
            apdu_len += len;
        }
        /* tag 8 - notifyType */
        len = encode_context_enumerated(&apdu[apdu_len], 8, data->notifyType);
        apdu_len += len;

        switch (data->notifyType) {
            case NOTIFY_ALARM:
            case NOTIFY_EVENT:
                /* tag 9 - ackRequired */

                len = encode_context_boolean(
                    &apdu[apdu_len], 9, data->ackRequired);
                apdu_len += len;

                /* tag 10 - fromState */
                len = encode_context_enumerated(
                    &apdu[apdu_len], 10, data->fromState);
                apdu_len += len;
                break;

            default:
                break;
        }

        /* tag 11 - toState */
        len = encode_context_enumerated(&apdu[apdu_len], 11, data->toState);
        apdu_len += len;

        switch (data->notifyType) {
            case NOTIFY_ALARM:
            case NOTIFY_EVENT:
                /* tag 12 - event values */
                len = encode_opening_tag(&apdu[apdu_len], 12);
                apdu_len += len;

                switch (data->eventType) {
                    case EVENT_CHANGE_OF_BITSTRING:
                        len = encode_opening_tag(&apdu[apdu_len], 0);
                        apdu_len += len;

                        len = encode_context_bitstring(&apdu[apdu_len], 0,
                            &data->notificationParams.changeOfBitstring
                                 .referencedBitString);
                        apdu_len += len;

                        len = encode_context_bitstring(&apdu[apdu_len], 1,
                            &data->notificationParams.changeOfBitstring
                                 .statusFlags);
                        apdu_len += len;

                        len = encode_closing_tag(&apdu[apdu_len], 0);
                        apdu_len += len;
                        break;

                    case EVENT_CHANGE_OF_STATE:
                        len = encode_opening_tag(&apdu[apdu_len], 1);
                        apdu_len += len;

                        len = encode_opening_tag(&apdu[apdu_len], 0);
                        apdu_len += len;

                        len = bacapp_encode_property_state(&apdu[apdu_len],
                            &data->notificationParams.changeOfState.newState);
                        apdu_len += len;

                        len = encode_closing_tag(&apdu[apdu_len], 0);
                        apdu_len += len;

                        len = encode_context_bitstring(&apdu[apdu_len], 1,
                            &data->notificationParams.changeOfState
                                 .statusFlags);
                        apdu_len += len;

                        len = encode_closing_tag(&apdu[apdu_len], 1);
                        apdu_len += len;
                        break;

                    case EVENT_CHANGE_OF_VALUE:
                        len = encode_opening_tag(&apdu[apdu_len], 2);
                        apdu_len += len;

                        len = encode_opening_tag(&apdu[apdu_len], 0);
                        apdu_len += len;

                        switch (data->notificationParams.changeOfValue.tag) {
                            case CHANGE_OF_VALUE_REAL:
                                len = encode_context_real(&apdu[apdu_len], 1,
                                    data->notificationParams.changeOfValue
                                        .newValue.changeValue);
                                apdu_len += len;
                                break;

                            case CHANGE_OF_VALUE_BITS:
                                len =
                                    encode_context_bitstring(&apdu[apdu_len], 0,
                                        &data->notificationParams.changeOfValue
                                             .newValue.changedBits);
                                apdu_len += len;
                                break;

                            default:
                                return 0;
                        }

                        len = encode_closing_tag(&apdu[apdu_len], 0);
                        apdu_len += len;

                        len = encode_context_bitstring(&apdu[apdu_len], 1,
                            &data->notificationParams.changeOfValue
                                 .statusFlags);
                        apdu_len += len;

                        len = encode_closing_tag(&apdu[apdu_len], 2);
                        apdu_len += len;
                        break;

                    case EVENT_COMMAND_FAILURE:

                        len = encode_opening_tag(&apdu[apdu_len], 3);
                        apdu_len += len;

                        len = encode_opening_tag(&apdu[apdu_len], 0);
                        apdu_len += len;

                        switch (data->notificationParams.commandFailure.tag) {
                            case COMMAND_FAILURE_BINARY_PV:
                                len =
                                    encode_application_enumerated(&apdu
                                    [apdu_len],
                                    data->notificationParams.commandFailure.
                                    commandValue.binaryValue);
                                apdu_len += len;
                                break;

                            case COMMAND_FAILURE_UNSIGNED:
                                len =
                                    encode_application_unsigned(&apdu
                                    [apdu_len],
                                    data->notificationParams.commandFailure.
                                    commandValue.unsignedValue);
                                apdu_len += len;
                                break;

                            default:
                                return 0;
                        }

                        len = encode_closing_tag(&apdu[apdu_len], 0);
                        apdu_len += len;

                        len =
                            encode_context_bitstring(&apdu[apdu_len], 1,
                            &data->notificationParams.commandFailure.
                            statusFlags);
                        apdu_len += len;

                        len = encode_opening_tag(&apdu[apdu_len], 2);
                        apdu_len += len;

                        switch (data->notificationParams.commandFailure.tag) {
                            case COMMAND_FAILURE_BINARY_PV:
                                len =
                                    encode_application_enumerated(&apdu
                                    [apdu_len],
                                    data->notificationParams.commandFailure.
                                    feedbackValue.binaryValue);
                                apdu_len += len;
                                break;

                            case COMMAND_FAILURE_UNSIGNED:
                                len =
                                    encode_application_unsigned(&apdu
                                    [apdu_len],
                                    data->notificationParams.commandFailure.
                                    feedbackValue.unsignedValue);
                                apdu_len += len;
                                break;

                            default:
                                return 0;
                        }

                        len = encode_closing_tag(&apdu[apdu_len], 2);
                        apdu_len += len;

                        len = encode_closing_tag(&apdu[apdu_len], 3);
                        apdu_len += len;
                        break;

                    case EVENT_FLOATING_LIMIT:
                        len = encode_opening_tag(&apdu[apdu_len], 4);
                        apdu_len += len;

                        len = encode_context_real(&apdu[apdu_len], 0,
                            data->notificationParams.floatingLimit
                                .referenceValue);
                        apdu_len += len;

                        len = encode_context_bitstring(&apdu[apdu_len], 1,
                            &data->notificationParams.floatingLimit
                                 .statusFlags);
                        apdu_len += len;

                        len = encode_context_real(&apdu[apdu_len], 2,
                            data->notificationParams.floatingLimit
                                .setPointValue);
                        apdu_len += len;

                        len = encode_context_real(&apdu[apdu_len], 3,
                            data->notificationParams.floatingLimit.errorLimit);
                        apdu_len += len;

                        len = encode_closing_tag(&apdu[apdu_len], 4);
                        apdu_len += len;
                        break;

                    case EVENT_OUT_OF_RANGE:
                        len = encode_opening_tag(&apdu[apdu_len], 5);
                        apdu_len += len;

                        len = encode_context_real(&apdu[apdu_len], 0,
                            data->notificationParams.outOfRange.exceedingValue);
                        apdu_len += len;

                        len = encode_context_bitstring(&apdu[apdu_len], 1,
                            &data->notificationParams.outOfRange.statusFlags);
                        apdu_len += len;

                        len = encode_context_real(&apdu[apdu_len], 2,
                            data->notificationParams.outOfRange.deadband);
                        apdu_len += len;

                        len = encode_context_real(&apdu[apdu_len], 3,
                            data->notificationParams.outOfRange.exceededLimit);
                        apdu_len += len;

                        len = encode_closing_tag(&apdu[apdu_len], 5);
                        apdu_len += len;
                        break;

                    case EVENT_CHANGE_OF_LIFE_SAFETY:
                        len = encode_opening_tag(&apdu[apdu_len], 8);
                        apdu_len += len;

                        len = encode_context_enumerated(&apdu[apdu_len], 0,
                            data->notificationParams.changeOfLifeSafety
                                .newState);
                        apdu_len += len;

                        len = encode_context_enumerated(&apdu[apdu_len], 1,
                            data->notificationParams.changeOfLifeSafety
                                .newMode);
                        apdu_len += len;

                        len = encode_context_bitstring(&apdu[apdu_len], 2,
                            &data->notificationParams.changeOfLifeSafety
                                 .statusFlags);
                        apdu_len += len;

                        len = encode_context_enumerated(&apdu[apdu_len], 3,
                            data->notificationParams.changeOfLifeSafety
                                .operationExpected);
                        apdu_len += len;

                        len = encode_closing_tag(&apdu[apdu_len], 8);
                        apdu_len += len;
                        break;

                    case EVENT_BUFFER_READY:
                        len = encode_opening_tag(&apdu[apdu_len], 10);
                        apdu_len += len;

                        len = bacapp_encode_context_device_obj_property_ref(
                            &apdu[apdu_len], 0,
                            &data->notificationParams.bufferReady
                                 .bufferProperty);
                        apdu_len += len;

                        len = encode_context_unsigned(&apdu[apdu_len], 1,
                            data->notificationParams.bufferReady
                                .previousNotification);
                        apdu_len += len;

                        len = encode_context_unsigned(&apdu[apdu_len], 2,
                            data->notificationParams.bufferReady
                                .currentNotification);
                        apdu_len += len;

                        len = encode_closing_tag(&apdu[apdu_len], 10);
                        apdu_len += len;
                        break;
                    case EVENT_UNSIGNED_RANGE:
                        len = encode_opening_tag(&apdu[apdu_len], 11);
                        apdu_len += len;

                        len = encode_context_unsigned(&apdu[apdu_len], 0,
                            data->notificationParams.unsignedRange
                                .exceedingValue);
                        apdu_len += len;

                        len = encode_context_bitstring(&apdu[apdu_len], 1,
                            &data->notificationParams.unsignedRange
                                 .statusFlags);
                        apdu_len += len;

                        len = encode_context_unsigned(&apdu[apdu_len], 2,
                            data->notificationParams.unsignedRange
                                .exceededLimit);
                        apdu_len += len;

                        len = encode_closing_tag(&apdu[apdu_len], 11);
                        apdu_len += len;
                        break;
                    case EVENT_ACCESS_EVENT:
                        len = encode_opening_tag(&apdu[apdu_len], 13);
                        apdu_len += len;

                        len =
                            encode_context_enumerated(&apdu[apdu_len], 0,
                            data->notificationParams.accessEvent.accessEvent);
                        apdu_len += len;

                        len =
                            encode_context_bitstring(&apdu[apdu_len], 1,
                            &data->notificationParams.accessEvent.statusFlags);
                        apdu_len += len;

                        len =
                            encode_context_unsigned(&apdu[apdu_len], 2,
                            data->notificationParams.
                            accessEvent.accessEventTag);
                        apdu_len += len;

                        len =
                            bacapp_encode_context_timestamp(&apdu[apdu_len], 3,
                            &data->notificationParams.
                            accessEvent.accessEventTime);
                        apdu_len += len;

                        len =
                            bacapp_encode_context_device_obj_ref(&apdu
                            [apdu_len], 4,
                            &data->notificationParams.
                            accessEvent.accessCredential);
                        apdu_len += len;

                        if (data->notificationParams.
                            accessEvent.authenticationFactor.format_type <
                            AUTHENTICATION_FACTOR_MAX) {
                            len =
                                bacapp_encode_context_authentication_factor
                                (&apdu[apdu_len], 5,
                                &data->notificationParams.
                                accessEvent.authenticationFactor);
                            apdu_len += len;
                        }

                        len = encode_closing_tag(&apdu[apdu_len], 13);
                        apdu_len += len;
                        break;
                    case EVENT_EXTENDED:
                    default:
                        assert(0);
                        break;
                }
                len = encode_closing_tag(&apdu[apdu_len], 12);
                apdu_len += len;
                break;
            case NOTIFY_ACK_NOTIFICATION:
                /* FIXME: handle this case */
            default:
                break;
        }
    }
    return apdu_len;
}

int event_notify_decode_service_request(
    uint8_t *apdu, unsigned apdu_len, BACNET_EVENT_NOTIFICATION_DATA *data)
{
    int len = 0; /* return value */
    int section_length = 0;
    BACNET_UNSIGNED_INTEGER unsigned_value = 0;
    uint32_t enum_value = 0;
    uint32_t len_value = 0;
    uint8_t tag_number = 0;

    if (apdu_len && data) {
        /* tag 0 - processIdentifier */
        section_length = bacnet_unsigned_context_decode(
            &apdu[len], apdu_len - len, 0, &unsigned_value);
        if (section_length > 0) {
            len += section_length;
            if (unsigned_value <= UINT32_MAX) {
                data->processIdentifier = (uint32_t)unsigned_value;
            } else {
                return BACNET_STATUS_ERROR;
            }
        } else {
            return BACNET_STATUS_ERROR;
        }
        /* tag 1 - initiatingObjectIdentifier */
        if ((section_length = decode_context_object_id(&apdu[len], 1,
                 &data->initiatingObjectIdentifier.type,
                 &data->initiatingObjectIdentifier.instance)) == -1) {
            return -1;
        } else {
            len += section_length;
        }
        /* tag 2 - eventObjectIdentifier */
        if ((section_length = decode_context_object_id(&apdu[len], 2,
                 &data->eventObjectIdentifier.type,
                 &data->eventObjectIdentifier.instance)) == -1) {
            return -1;
        } else {
            len += section_length;
        }
        /* tag 3 - timeStamp */
        if ((section_length = bacapp_decode_context_timestamp(
                 &apdu[len], 3, &data->timeStamp)) == -1) {
            return -1;
        } else {
            len += section_length;
        }
        /* tag 4 - noticicationClass */
        section_length = bacnet_unsigned_context_decode(
            &apdu[len], apdu_len - len, 4, &unsigned_value);
        if (section_length > 0) {
            len += section_length;
            if (unsigned_value <= UINT32_MAX) {
                data->notificationClass = (uint32_t)unsigned_value;
            } else {
                return BACNET_STATUS_ERROR;
            }
        } else {
            return BACNET_STATUS_ERROR;
        }
        /* tag 5 - priority */
        section_length = bacnet_unsigned_context_decode(
            &apdu[len], apdu_len - len, 5, &unsigned_value);
        if (section_length > 0) {
            len += section_length;
            if (unsigned_value <= UINT8_MAX) {
                data->priority = (uint8_t)unsigned_value;
            } else {
                return BACNET_STATUS_ERROR;
            }
        } else {
            return BACNET_STATUS_ERROR;
        }
        /* tag 6 - eventType */
        if ((section_length = decode_context_enumerated(
                 &apdu[len], 6, &enum_value)) == -1) {
            return -1;
        } else {
            data->eventType = (BACNET_EVENT_TYPE)enum_value;
            len += section_length;
        }
        /* tag 7 - messageText */

        if (decode_is_context_tag(&apdu[len], 7)) {
            if (data->messageText != NULL) {
                if ((section_length = decode_context_character_string(
                         &apdu[len], 7, data->messageText)) == -1) {
                    /*FIXME This is an optional parameter */
                    return -1;
                } else {
                    len += section_length;
                }
            } else {
                return -1;
            }
        } else {
            if (data->messageText != NULL) {
                characterstring_init_ansi(data->messageText, "");
            }
        }

        /* tag 8 - notifyType */
        if ((section_length = decode_context_enumerated(
                 &apdu[len], 8, &enum_value)) == -1) {
            return -1;
        } else {
            data->notifyType = (BACNET_NOTIFY_TYPE)enum_value;
            len += section_length;
        }
        switch (data->notifyType) {
            case NOTIFY_ALARM:
            case NOTIFY_EVENT:
                /* tag 9 - ackRequired */
                section_length =
                    decode_context_boolean2(&apdu[len], 9, &data->ackRequired);
                if (section_length == BACNET_STATUS_ERROR) {
                    return -1;
                }
                len += section_length;

                /* tag 10 - fromState */
                if ((section_length = decode_context_enumerated(
                         &apdu[len], 10, &enum_value)) == -1) {
                    return -1;
                } else {
                    data->fromState = (BACNET_EVENT_STATE)enum_value;
                    len += section_length;
                }
                break;
                /* In cases other than alarm and event
                   there's no data, so do not return an error
                   but continue normally */
            case NOTIFY_ACK_NOTIFICATION:
            default:
                break;
        }
        /* tag 11 - toState */
        if ((section_length = decode_context_enumerated(
                 &apdu[len], 11, &enum_value)) == -1) {
            return -1;
        } else {
            data->toState = (BACNET_EVENT_STATE)enum_value;
            len += section_length;
        }
        /* tag 12 - eventValues */
        switch (data->notifyType) {
            case NOTIFY_ALARM:
            case NOTIFY_EVENT:
                if (decode_is_opening_tag_number(&apdu[len], 12)) {
                    len++;
                } else {
                    return -1;
                }
                if (decode_is_opening_tag_number(
                        &apdu[len], (uint8_t)data->eventType)) {
                    len++;
                } else {
                    return -1;
                }

                switch (data->eventType) {
                    case EVENT_CHANGE_OF_BITSTRING:
                        if (-1 ==
                            (section_length = decode_context_bitstring(
                                 &apdu[len], 0,
                                 &data->notificationParams.changeOfBitstring
                                      .referencedBitString))) {
                            return -1;
                        }
                        len += section_length;

                        if (-1 ==
                            (section_length =
                                    decode_context_bitstring(&apdu[len], 1,
                                        &data->notificationParams
                                             .changeOfBitstring.statusFlags))) {
                            return -1;
                        }
                        len += section_length;

                        break;

                    case EVENT_CHANGE_OF_STATE:
                        if (-1 ==
                            (section_length =
                                    bacapp_decode_context_property_state(
                                        &apdu[len], 0,
                                        &data->notificationParams.changeOfState
                                             .newState))) {
                            return -1;
                        }
                        len += section_length;

                        if (-1 ==
                            (section_length =
                                    decode_context_bitstring(&apdu[len], 1,
                                        &data->notificationParams.changeOfState
                                             .statusFlags))) {
                            return -1;
                        }
                        len += section_length;

                        break;

                    case EVENT_CHANGE_OF_VALUE:
                        if (!decode_is_opening_tag_number(&apdu[len], 0)) {
                            return -1;
                        }
                        len++;

                        if (decode_is_context_tag(
                                &apdu[len], CHANGE_OF_VALUE_BITS)) {
                            if (-1 ==
                                (section_length = decode_context_bitstring(
                                     &apdu[len], 0,
                                     &data->notificationParams.changeOfValue
                                          .newValue.changedBits))) {
                                return -1;
                            }

                            len += section_length;
                            data->notificationParams.changeOfValue.tag =
                                CHANGE_OF_VALUE_BITS;
                        } else if (decode_is_context_tag(
                                       &apdu[len], CHANGE_OF_VALUE_REAL)) {
                            if (-1 ==
                                (section_length = decode_context_real(
                                     &apdu[len], 1,
                                     &data->notificationParams.changeOfValue
                                          .newValue.changeValue))) {
                                return -1;
                            }

                            len += section_length;
                            data->notificationParams.changeOfValue.tag =
                                CHANGE_OF_VALUE_REAL;
                        } else {
                            return -1;
                        }
                        if (!decode_is_closing_tag_number(&apdu[len], 0)) {
                            return -1;
                        }
                        len++;

                        if (-1 ==
                            (section_length =
                                    decode_context_bitstring(&apdu[len], 1,
                                        &data->notificationParams.changeOfValue
                                             .statusFlags))) {
                            return -1;
                        }
                        len += section_length;
                        break;

                    case EVENT_COMMAND_FAILURE:
                        if (!decode_is_opening_tag_number(&apdu[len], 0)) {
                            return -1;
                        }
                        len++;

                        if (-1 == (section_length =
                                decode_tag_number_and_value(&apdu[len],
                                    &tag_number, &len_value))) {
                            return -1;
                        }
                        len += section_length;

                        switch (tag_number) {
                            case BACNET_APPLICATION_TAG_ENUMERATED:
                                if (-1 ==
				    (section_length = decode_enumerated(
					&apdu[len], len_value, &enum_value))) {
                                    return -1;
                                }
                                data->notificationParams.commandFailure
                                      .commandValue.binaryValue = enum_value;
                                break;

                            case BACNET_APPLICATION_TAG_UNSIGNED_INT:
                                if (-1 == (section_length =
                                        decode_unsigned(&apdu[len], len_value,
                                            &data->
                                            notificationParams.commandFailure.
                                            commandValue.unsignedValue))) {
                                    return -1;
                                }
                                break;

                            default:
                                return 0;
                        }
                        len += section_length;

                        if (!decode_is_closing_tag_number(&apdu[len], 0)) {
                            return -1;
                        }
                        len++;

                        if (-1 == (section_length =
                                decode_context_bitstring(&apdu[len], 1,
                                    &data->notificationParams.commandFailure.
                                    statusFlags))) {
                            return -1;
                        }
                        len += section_length;

                        if (!decode_is_opening_tag_number(&apdu[len], 2)) {
                            return -1;
                        }
                        len++;

                        if (-1 == (section_length =
                                decode_tag_number_and_value(&apdu[len],
                                    &tag_number, &len_value))) {
                            return -1;
                        }
                        len += section_length;

                        switch (tag_number) {
                            case BACNET_APPLICATION_TAG_ENUMERATED:
                                if (-1 ==
				    (section_length = decode_enumerated(
					&apdu[len], len_value, &enum_value))) {
                                    return -1;
                                }
                                data->notificationParams.commandFailure
				    .feedbackValue.binaryValue = enum_value;
                                break;

                            case BACNET_APPLICATION_TAG_UNSIGNED_INT:
                                if (-1 == (section_length =
                                        decode_unsigned(&apdu[len], len_value,
                                            &data->
                                            notificationParams.commandFailure.
                                            feedbackValue.unsignedValue))) {
                                    return -1;
                                }
                                break;

                            default:
                                return 0;
                        }
                        len += section_length;

                        if (!decode_is_closing_tag_number(&apdu[len], 2)) {
                            return -1;
                        }
                        len++;

                        break;

                    case EVENT_FLOATING_LIMIT:
                        if (-1 ==
                            (section_length = decode_context_real(&apdu[len], 0,
                                 &data->notificationParams.floatingLimit
                                      .referenceValue))) {
                            return -1;
                        }
                        len += section_length;

                        if (-1 ==
                            (section_length =
                                    decode_context_bitstring(&apdu[len], 1,
                                        &data->notificationParams.floatingLimit
                                             .statusFlags))) {
                            return -1;
                        }
                        len += section_length;
                        if (-1 ==
                            (section_length = decode_context_real(&apdu[len], 2,
                                 &data->notificationParams.floatingLimit
                                      .setPointValue))) {
                            return -1;
                        }
                        len += section_length;

                        if (-1 ==
                            (section_length = decode_context_real(&apdu[len], 3,
                                 &data->notificationParams.floatingLimit
                                      .errorLimit))) {
                            return -1;
                        }
                        len += section_length;
                        break;

                    case EVENT_OUT_OF_RANGE:
                        if (-1 ==
                            (section_length = decode_context_real(&apdu[len], 0,
                                 &data->notificationParams.outOfRange
                                      .exceedingValue))) {
                            return -1;
                        }
                        len += section_length;

                        if (-1 ==
                            (section_length =
                                    decode_context_bitstring(&apdu[len], 1,
                                        &data->notificationParams.outOfRange
                                             .statusFlags))) {
                            return -1;
                        }
                        len += section_length;
                        if (-1 ==
                            (section_length = decode_context_real(&apdu[len], 2,
                                 &data->notificationParams.outOfRange
                                      .deadband))) {
                            return -1;
                        }
                        len += section_length;

                        if (-1 ==
                            (section_length = decode_context_real(&apdu[len], 3,
                                 &data->notificationParams.outOfRange
                                      .exceededLimit))) {
                            return -1;
                        }
                        len += section_length;
                        break;

                    case EVENT_CHANGE_OF_LIFE_SAFETY:
                        if (-1 ==
                            (section_length = decode_context_enumerated(
                                 &apdu[len], 0, &enum_value))) {
                            return -1;
                        }
                        data->notificationParams.changeOfLifeSafety.newState =
                            (BACNET_LIFE_SAFETY_STATE)enum_value;
                        len += section_length;

                        if (-1 ==
                            (section_length = decode_context_enumerated(
                                 &apdu[len], 1, &enum_value))) {
                            return -1;
                        }
                        data->notificationParams.changeOfLifeSafety.newMode =
                            (BACNET_LIFE_SAFETY_MODE)enum_value;
                        len += section_length;

                        if (-1 ==
                            (section_length = decode_context_bitstring(
                                 &apdu[len], 2,
                                 &data->notificationParams.changeOfLifeSafety
                                      .statusFlags))) {
                            return -1;
                        }
                        len += section_length;

                        if (-1 ==
                            (section_length = decode_context_enumerated(
                                 &apdu[len], 3, &enum_value))) {
                            return -1;
                        }
                        data->notificationParams.changeOfLifeSafety
                            .operationExpected =
                            (BACNET_LIFE_SAFETY_OPERATION)enum_value;
                        len += section_length;
                        break;

                    case EVENT_BUFFER_READY:
                        /* Tag 0 - bufferProperty */
                        if (-1 ==
                            (section_length =
                                    bacapp_decode_context_device_obj_property_ref(
                                        &apdu[len], 0,
                                        &data->notificationParams.bufferReady
                                             .bufferProperty))) {
                            return -1;
                        }
                        len += section_length;
                        /* Tag 1 - PreviousNotification */
                        section_length = bacnet_unsigned_context_decode(
                            &apdu[len], apdu_len - len, 1, &unsigned_value);
                        if (section_length > 0) {
                            len += section_length;
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
                        section_length = bacnet_unsigned_context_decode(
                            &apdu[len], apdu_len - len, 2, &unsigned_value);
                        if (section_length > 0) {
                            len += section_length;
                            if (unsigned_value <= UINT32_MAX) {
                                data->notificationParams.bufferReady
                                    .currentNotification =
                                    (uint32_t)unsigned_value;
                            } else {
                                return BACNET_STATUS_ERROR;
                            }
                        } else {
                            return BACNET_STATUS_ERROR;
                        }
                        break;

                    case EVENT_UNSIGNED_RANGE:
                        /* Tag 0 - PreviousNotification */
                        section_length = bacnet_unsigned_context_decode(
                            &apdu[len], apdu_len - len, 0, &unsigned_value);
                        if (section_length > 0) {
                            len += section_length;
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
                            (section_length =
                                    decode_context_bitstring(&apdu[len], 1,
                                        &data->notificationParams.unsignedRange
                                             .statusFlags))) {
                            return -1;
                        }
                        len += section_length;
                        /* Tag 2 - exceededLimit */
                        section_length = bacnet_unsigned_context_decode(
                            &apdu[len], apdu_len - len, 2, &unsigned_value);
                        if (section_length > 0) {
                            len += section_length;
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
			    (section_length = decode_context_enumerated(
				&apdu[len], 0, &enum_value))) {
                            return -1;
                        }
                        data->notificationParams.accessEvent.accessEvent =
			    enum_value;
                        len += section_length;

                        if (-1 == (section_length =
                                decode_context_bitstring(&apdu[len], 1,
                                    &data->notificationParams.
                                    accessEvent.statusFlags))) {
                            return -1;
                        }
                        len += section_length;

                        if (-1 == (section_length =
                                decode_context_unsigned(&apdu[len], 2,
                                    &data->notificationParams.
                                    accessEvent.accessEventTag))) {
                            return -1;
                        }
                        len += section_length;

                        if (-1 == (section_length =
                                bacapp_decode_context_timestamp(&apdu[len], 3,
                                    &data->notificationParams.
                                    accessEvent.accessEventTime))) {
                            return -1;
                        }
                        len += section_length;

                        if (-1 == (section_length =
                                bacapp_decode_context_device_obj_ref(&apdu
                                    [len], 4,
                                    &data->notificationParams.
                                    accessEvent.accessCredential))) {
                            return -1;
                        }
                        len += section_length;

                        if (!decode_is_closing_tag(&apdu[len])) {
                            if (-1 == (section_length =
                                    bacapp_decode_context_authentication_factor
                                    (&apdu[len], 5,
                                        &data->notificationParams.
                                        accessEvent.authenticationFactor))) {
                                return -1;
                            }
                            len += section_length;
                        }
                        break;

                    default:
                        return -1;
                }
                if (decode_is_closing_tag_number(
                        &apdu[len], (uint8_t)data->eventType)) {
                    len++;
                } else {
                    return -1;
                }
                if (decode_is_closing_tag_number(&apdu[len], 12)) {
                    len++;
                } else {
                    return -1;
                }
                break;
                /* In cases other than alarm and event
                   there's no data, so do not return an error
                   but continue normally */
            case NOTIFY_ACK_NOTIFICATION:
            default:
                break;
        }
    }

    return len;
}
