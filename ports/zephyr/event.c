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
    BACNET_TAG tag;
    int tag_len;

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
        section_length = bacnet_object_id_context_decode(&apdu[len],
            apdu_len - len, 1, &data->initiatingObjectIdentifier.type,
            &data->initiatingObjectIdentifier.instance);
        if (section_length <= 0) {
            return BACNET_STATUS_ERROR;
        } else {
            len += section_length;
        }
        /* tag 2 - eventObjectIdentifier */
        section_length = bacnet_object_id_context_decode(&apdu[len],
            apdu_len - len, 2, &data->eventObjectIdentifier.type,
            &data->eventObjectIdentifier.instance);
        if (section_length <= 0) {
            return BACNET_STATUS_ERROR;
        } else {
            len += section_length;
        }
        /* tag 3 - timeStamp */
        section_length = bacnet_timestamp_context_decode(&apdu[len],
            apdu_len - len, 3, &data->timeStamp);
        if (section_length <= 0) {
            return BACNET_STATUS_ERROR;
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
        section_length = bacnet_enumerated_context_decode(
            &apdu[len], apdu_len - len, 6, &enum_value);
        if (section_length <= 0) {
            return BACNET_STATUS_ERROR;
        } else {
            data->eventType = (BACNET_EVENT_TYPE)enum_value;
            len += section_length;
        }
        /* tag 7 - messageText */

        if (bacnet_is_context_tag_number(&apdu[len], apdu_len - len, 7, NULL)) {
            if (data->messageText != NULL) {
                section_length = bacnet_character_string_context_decode(
                    &apdu[len], apdu_len - len, 7, data->messageText);
                    /*FIXME This is an optional parameter */
                if (section_length > 0) {
                    len += section_length;
                } else {
                    return BACNET_STATUS_ERROR;
                }
            } else {
                return BACNET_STATUS_ERROR;
            }
        } else {
            if (data->messageText != NULL) {
                characterstring_init_ansi(data->messageText, "");
            }
        }

        /* tag 8 - notifyType */
        section_length = bacnet_enumerated_context_decode(
            &apdu[len], apdu_len - len, 8, &enum_value);
        if (section_length <= 0) {
            return BACNET_STATUS_ERROR;
        } else {
            data->notifyType = (BACNET_NOTIFY_TYPE)enum_value;
            len += section_length;
        }
        switch (data->notifyType) {
            case NOTIFY_ALARM:
            case NOTIFY_EVENT:
                /* tag 9 - ackRequired */
                section_length = bacnet_boolean_context_decode(
                        &apdu[len], apdu_len - len, 9, &data->ackRequired);
                if (section_length == BACNET_STATUS_ERROR) {
                    return BACNET_STATUS_ERROR;
                }
                len += section_length;

                /* tag 10 - fromState */
                section_length = bacnet_enumerated_context_decode(
                    &apdu[len], apdu_len - len, 10, &enum_value);
                if (section_length <= 0) {
                    return BACNET_STATUS_ERROR;
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
        section_length = bacnet_enumerated_context_decode(
            &apdu[len], apdu_len - len, 11, &enum_value);
        if (section_length <= 0) {
            return BACNET_STATUS_ERROR;
        } else {
            data->toState = (BACNET_EVENT_STATE)enum_value;
            len += section_length;
        }
        /* tag 12 - eventValues */
        switch (data->notifyType) {
            case NOTIFY_ALARM:
            case NOTIFY_EVENT:
                if (bacnet_is_opening_tag_number(
                        &apdu[len], apdu_len - len, 12, &tag_len)) {
                    len += tag_len;
                } else {
                    return BACNET_STATUS_ERROR;
                }
                if (bacnet_is_opening_tag_number(&apdu[len], apdu_len - len,
                        (uint8_t)data->eventType, &tag_len)) {
                    len += tag_len;
                } else {
                    return BACNET_STATUS_ERROR;
                }

                switch (data->eventType) {
                    case EVENT_CHANGE_OF_BITSTRING:
                        section_length = bacnet_bitstring_context_decode(
                            &apdu[len], apdu_len - len, 0,
                            &data->notificationParams.changeOfBitstring
                                .referencedBitString);
                        if (section_length <= 0) {
                            return BACNET_STATUS_ERROR;
                        }
                        len += section_length;

                        section_length = bacnet_bitstring_context_decode(
                            &apdu[len], apdu_len - len, 1,
                            &data->notificationParams
                                .changeOfBitstring.statusFlags);
                        if (section_length <= 0) {
                            return BACNET_STATUS_ERROR;
                        }
                        len += section_length;

                        break;

                    case EVENT_CHANGE_OF_STATE:
                        section_length = bacapp_decode_context_property_state(
                            &apdu[len], 0,
                            &data->notificationParams.changeOfState.newState);
                        if (section_length <= 0) {
                            return BACNET_STATUS_ERROR;
                        }
                        len += section_length;

                        section_length = bacnet_bitstring_context_decode(
                            &apdu[len], apdu_len - len, 1,
                            &data->notificationParams.changeOfState
                                .statusFlags);
                        if (section_length <= 0) {
                            return BACNET_STATUS_ERROR;
                        }
                        len += section_length;

                        break;

                    case EVENT_CHANGE_OF_VALUE:
                        if (!bacnet_is_opening_tag_number(
                                &apdu[len], apdu_len - len, 0, &tag_len)) {
                            return BACNET_STATUS_ERROR;
                        }
                        len += tag_len;

                        if (bacnet_is_context_tag_number(&apdu[len],
                            apdu_len - len, CHANGE_OF_VALUE_BITS, NULL)) {

                            section_length = bacnet_bitstring_context_decode(
                                &apdu[len], apdu_len - len, 0,
                                &data->notificationParams.changeOfValue
                                    .newValue.changedBits);
                            if (section_length <= 0) {
                                return BACNET_STATUS_ERROR;
                            }

                            len += section_length;
                            data->notificationParams.changeOfValue.tag =
                                CHANGE_OF_VALUE_BITS;
                        } else if (bacnet_is_context_tag_number(&apdu[len],
                                apdu_len - len, CHANGE_OF_VALUE_REAL, NULL)) {
                            section_length = bacnet_real_context_decode(
                                &apdu[len], apdu_len - len, 1,
                                &data->notificationParams.changeOfValue
                                    .newValue.changeValue);
                            if (section_length <= 0) {
                                return BACNET_STATUS_ERROR;
                            }

                            len += section_length;
                            data->notificationParams.changeOfValue.tag =
                                CHANGE_OF_VALUE_REAL;
                        } else {
                            return BACNET_STATUS_ERROR;
                        }
                        if (!bacnet_is_closing_tag_number(
                                &apdu[len], apdu_len - len, 0, &tag_len)) {
                            return BACNET_STATUS_ERROR;
                        }
                        len += tag_len;

                        section_length = bacnet_bitstring_context_decode(
                            &apdu[len], apdu_len - len, 1,
                            &data->notificationParams.changeOfValue
                                .statusFlags);
                        if (section_length <= 0) {
                            return BACNET_STATUS_ERROR;
                        }
                        len += section_length;
                        break;

                    case EVENT_COMMAND_FAILURE:
                        if (!bacnet_is_opening_tag_number(
                                &apdu[len], apdu_len - len, 0, &tag_len)) {
                            return BACNET_STATUS_ERROR;
                        }
                        len += tag_len;

                        section_length = bacnet_tag_decode(
                            &apdu[len], apdu_len - len, &tag);
                        if (section_length <= 0) {
                            return BACNET_STATUS_ERROR;
                        }
                        len += section_length;

                        switch (tag.number) {
                            case BACNET_APPLICATION_TAG_ENUMERATED:
                                section_length = bacnet_enumerated_decode(
                                    &apdu[len], apdu_len - len,
                                    tag.len_value_type, &enum_value);
                                if (section_length <= 0) {
                                    return BACNET_STATUS_ERROR;
                                }
                                data->notificationParams.commandFailure
                                      .commandValue.binaryValue = enum_value;
                                break;

                            case BACNET_APPLICATION_TAG_UNSIGNED_INT:
                                section_length = bacnet_unsigned_decode(
                                    &apdu[len], apdu_len - len,
                                    tag.len_value_type,
                                    &data->notificationParams.commandFailure.
                                        commandValue.unsignedValue);
                                if (section_length <= 0) {
                                    return BACNET_STATUS_ERROR;
                                }
                                break;

                            default:
                                return 0;
                        }
                        len += section_length;

                        if (!bacnet_is_closing_tag_number(
                                &apdu[len], apdu_len - len, 0, &tag_len)) {
                            return BACNET_STATUS_ERROR;
                        }
                        len += tag_len;

                        section_length = bacnet_bitstring_context_decode(
                            &apdu[len], apdu_len - len, 1,
                            &data->notificationParams.commandFailure
                                .statusFlags);
                        if (section_length <= 0) {
                            return BACNET_STATUS_ERROR;
                        }
                        len += section_length;

                        if (!bacnet_is_opening_tag_number(
                                &apdu[len], apdu_len - len, 2, &tag_len)) {
                            return BACNET_STATUS_ERROR;
                        }
                        len += tag_len;

                        section_length = bacnet_tag_decode(
                            &apdu[len], apdu_len - len, &tag);
                        if (section_length <= 0) {
                            return BACNET_STATUS_ERROR;
                        }
                        len += section_length;

                        switch (tag.number) {
                            case BACNET_APPLICATION_TAG_ENUMERATED:
                                section_length = bacnet_enumerated_decode(
                                    &apdu[len], apdu_len - len,
                                    tag.len_value_type, &enum_value);
                                if (section_length <= 0) {
                                    return BACNET_STATUS_ERROR;
                                }
                                data->notificationParams.commandFailure
                                    .feedbackValue.binaryValue = enum_value;
                                break;

                            case BACNET_APPLICATION_TAG_UNSIGNED_INT:
                                section_length = bacnet_unsigned_context_decode(
                                    &apdu[len], apdu_len - len,
                                    tag.len_value_type,
                                    &data->notificationParams.commandFailure.
                                        feedbackValue.unsignedValue);
                                if (section_length <= 0) {
                                    return BACNET_STATUS_ERROR;
                                }
                                break;

                            default:
                                return 0;
                        }
                        len += section_length;

                        if (!bacnet_is_closing_tag_number(
                                &apdu[len], apdu_len - len, 2, &tag_len)) {
                            return BACNET_STATUS_ERROR;
                        }
                        len += tag_len;

                        break;

                    case EVENT_FLOATING_LIMIT:
                        section_length = bacnet_real_context_decode(
                            &apdu[len], apdu_len - len, 0,
                            &data->notificationParams.floatingLimit
                                .referenceValue);
                        if (section_length <= 0) {
                            return BACNET_STATUS_ERROR;
                        }
                        len += section_length;

                        section_length = bacnet_bitstring_context_decode(
                            &apdu[len], apdu_len - len, 1,
                            &data->notificationParams.floatingLimit
                                .statusFlags);
                        if (section_length <= 0) {
                            return BACNET_STATUS_ERROR;
                        }
                        len += section_length;

                        section_length = bacnet_real_context_decode(
                            &apdu[len], apdu_len - len, 2,
                            &data->notificationParams.floatingLimit
                                .setPointValue);
                        if (section_length <= 0) {
                            return BACNET_STATUS_ERROR;
                        }
                        len += section_length;

                        section_length = bacnet_real_context_decode(
                            &apdu[len], apdu_len - len, 3,
                            &data->notificationParams.floatingLimit
                                .errorLimit);
                        if (section_length <= 0) {
                            return BACNET_STATUS_ERROR;
                        }
                        len += section_length;
                        break;

                    case EVENT_OUT_OF_RANGE:
                        section_length = bacnet_real_context_decode(
                            &apdu[len], apdu_len - len, 0,
                            &data->notificationParams.outOfRange
                                .exceedingValue);
                        if (section_length <= 0) {
                            return BACNET_STATUS_ERROR;
                        }
                        len += section_length;

                        section_length = bacnet_bitstring_context_decode(
                            &apdu[len], apdu_len - len, 1,
                            &data->notificationParams.outOfRange
                                .statusFlags);
                        if (section_length <= 0) {
                            return BACNET_STATUS_ERROR;
                        }
                        len += section_length;

                        section_length = bacnet_real_context_decode(
                            &apdu[len], apdu_len - len, 2,
                            &data->notificationParams.outOfRange.deadband);
                        if (section_length <= 0) {
                            return BACNET_STATUS_ERROR;
                        }
                        len += section_length;

                        section_length = bacnet_real_context_decode(
                            &apdu[len], apdu_len - len, 3,
                            &data->notificationParams.outOfRange
                                .exceededLimit);
                        if (section_length <= 0) {
                            return BACNET_STATUS_ERROR;
                        }
                        len += section_length;
                        break;

                    case EVENT_CHANGE_OF_LIFE_SAFETY:
                        section_length = bacnet_enumerated_context_decode(
                            &apdu[len], apdu_len - len, 0, &enum_value);
                        if (section_length <= 0) {
                            return BACNET_STATUS_ERROR;
                        }
                        data->notificationParams.changeOfLifeSafety.newState =
                            (BACNET_LIFE_SAFETY_STATE)enum_value;
                        len += section_length;

                        section_length = bacnet_enumerated_context_decode(
                            &apdu[len], apdu_len - len, 1, &enum_value);
                        if (section_length <= 0) {
                            return BACNET_STATUS_ERROR;
                        }
                        data->notificationParams.changeOfLifeSafety.newMode =
                            (BACNET_LIFE_SAFETY_MODE)enum_value;
                        len += section_length;

                        section_length = bacnet_bitstring_context_decode(
                            &apdu[len], apdu_len - len, 2,
                            &data->notificationParams.changeOfLifeSafety
                                .statusFlags);
                        if (section_length <= 0) {
                            return BACNET_STATUS_ERROR;
                        }
                        len += section_length;

                        section_length = bacnet_enumerated_context_decode(
                            &apdu[len], apdu_len - len, 3, &enum_value);
                        if (section_length <= 0) {
                            return BACNET_STATUS_ERROR;
                        }
                        data->notificationParams.changeOfLifeSafety
                            .operationExpected =
                            (BACNET_LIFE_SAFETY_OPERATION)enum_value;
                        len += section_length;
                        break;

                    case EVENT_BUFFER_READY:
                        /* Tag 0 - bufferProperty */
                        section_length =
                            bacapp_decode_context_device_obj_property_ref(
                                &apdu[len], 0,
                                &data->notificationParams.bufferReady
                                    .bufferProperty);
                        if (section_length <= 0) {
                            return BACNET_STATUS_ERROR;
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
                        section_length = bacnet_bitstring_context_decode(
                            &apdu[len], apdu_len - len, 1,
                            &data->notificationParams.unsignedRange
                                .statusFlags);
                        if (section_length <= 0) {
                            return BACNET_STATUS_ERROR;
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
                        section_length = bacnet_enumerated_context_decode(
				            &apdu[len], apdu_len - len, 0, &enum_value);
                        if (section_length <= 0) {
                            return BACNET_STATUS_ERROR;
                        }
                        data->notificationParams.accessEvent.accessEvent =
                            enum_value;
                        len += section_length;

                        section_length = bacnet_bitstring_context_decode(
                            &apdu[len], apdu_len - len, 1,
                            &data->notificationParams.
                                accessEvent.statusFlags);
                        if (section_length <= 0) {
                            return BACNET_STATUS_ERROR;
                        }
                        len += section_length;

                        section_length = bacnet_unsigned_context_decode(
                            &apdu[len], apdu_len - len, 2,
                            &data->notificationParams.
                                accessEvent.accessEventTag);
                        if (section_length <= 0) {
                            return BACNET_STATUS_ERROR;
                        }
                        len += section_length;

                        section_length = bacnet_timestamp_context_decode(
                            &apdu[len], apdu_len - len, 3,
                            &data->notificationParams.
                                accessEvent.accessEventTime);
                        if (section_length <= 0) {
                            return BACNET_STATUS_ERROR;
                        }
                        len += section_length;

                        section_length = bacapp_decode_context_device_obj_ref(
                            &apdu[len], 4,&data->notificationParams.
                                accessEvent.accessCredential);
                        if (section_length <= 0) {
                            return BACNET_STATUS_ERROR;
                        }
                        len += section_length;

                        if (!bacnet_is_closing_tag(
                                &apdu[len], apdu_len - len)) {
                            section_length =
                                bacapp_decode_context_authentication_factor(
                                    &apdu[len], 5,
                                    &data->notificationParams.
                                        accessEvent.authenticationFactor);
                            if (section_length <= 0) {
                                return BACNET_STATUS_ERROR;
                            }
                            len += section_length;
                        }
                        break;

                    default:
                        return BACNET_STATUS_ERROR;
                }
                if (bacnet_is_closing_tag_number(&apdu[len], apdu_len - len,
                        (uint8_t)data->eventType, &tag_len)) {
                    len += tag_len;
                } else {
                    return BACNET_STATUS_ERROR;
                }
                if (bacnet_is_closing_tag_number(
                        &apdu[len], apdu_len - len, 12, &tag_len)) {
                    len += tag_len;
                } else {
                    return BACNET_STATUS_ERROR;
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
