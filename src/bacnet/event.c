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
#include "bacnet/bacapp.h"
#include "bacnet/bactext.h"

/** @file event.c  Encode/Decode Event Notifications */

/**
 * @brief Encode the unconfirmed EventNotification service request
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
 * @brief Encode the ConfirmedEventNotification service request
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

#if BACNET_EVENT_CHANGE_OF_STATUS_FLAGS_ENABLED
/**
 * @brief Encode the EXTENDED event parameter
 * @param apdu  Pointer to the buffer for encoding into
 * @param data  Pointer to the service data used for encoding values
 * @return number of bytes encoded, or zero if unable to encode
 */
static int event_extended_parameter_encode(
    uint8_t *apdu, const BACNET_EVENT_EXTENDED_PARAMETER *value)
{
    int apdu_len = 0; /* total length of the apdu, return value */

    if (!value) {
        return 0;
    }
    switch (value->tag) {
        case BACNET_APPLICATION_TAG_NULL:
            if (apdu) {
                apdu[0] = value->tag;
            }
            apdu_len++;
            break;
        case BACNET_APPLICATION_TAG_BOOLEAN:
            apdu_len = encode_application_boolean(apdu, value->type.Boolean);
            break;
        case BACNET_APPLICATION_TAG_UNSIGNED_INT:
            apdu_len =
                encode_application_unsigned(apdu, value->type.Unsigned_Int);
            break;
        case BACNET_APPLICATION_TAG_SIGNED_INT:
            apdu_len = encode_application_signed(apdu, value->type.Signed_Int);
            break;
        case BACNET_APPLICATION_TAG_REAL:
            apdu_len = encode_application_real(apdu, value->type.Real);
            break;
        case BACNET_APPLICATION_TAG_DOUBLE:
            apdu_len = encode_application_double(apdu, value->type.Double);
            break;
        case BACNET_APPLICATION_TAG_OCTET_STRING:
            apdu_len =
                encode_application_octet_string(apdu, value->type.Octet_String);
            break;
        case BACNET_APPLICATION_TAG_CHARACTER_STRING:
            apdu_len = encode_application_character_string(
                apdu, value->type.Character_String);
            break;
        case BACNET_APPLICATION_TAG_BIT_STRING:
            apdu_len =
                encode_application_bitstring(apdu, value->type.Bit_String);
            break;
        case BACNET_APPLICATION_TAG_ENUMERATED:
            apdu_len =
                encode_application_enumerated(apdu, value->type.Enumerated);
            break;
        case BACNET_APPLICATION_TAG_DATE:
            apdu_len = encode_application_date(apdu, &value->type.Date);
            break;
        case BACNET_APPLICATION_TAG_TIME:
            apdu_len = encode_application_time(apdu, &value->type.Time);
            break;
        case BACNET_APPLICATION_TAG_OBJECT_ID:
            apdu_len = encode_application_object_id(
                apdu, value->type.Object_Id.type,
                value->type.Object_Id.instance);
            break;
        case BACNET_APPLICATION_TAG_PROPERTY_VALUE:
            apdu_len = bacapp_property_value_context_encode(
                apdu, 0, value->type.Property_Value);
            break;
        default:
            break;
    }

    return apdu_len;
}
#endif

/**
 * @brief Decode the EXTENDED event parameter from a buffer
 * @param apdu - the APDU buffer
 * @param apdu_size - the size of the APDU buffer
 * @param value - BACnetDeviceObjectPropertyValue to decode into
 * @return number of bytes decoded or BACNET_STATUS_ERROR on failure.
 */
static int event_extended_parameter_decode(
    const uint8_t *apdu,
    uint32_t apdu_size,
    BACNET_EVENT_EXTENDED_PARAMETER *value)
{
    int len, apdu_len = 0;
    BACNET_TAG tag = { 0 };

    if (!value) {
        return 0;
    }
    len = bacnet_tag_decode(&apdu[apdu_len], apdu_size - apdu_len, &tag);
    if (len > 0) {
        apdu_len += len;
        if (tag.application) {
            switch (tag.number) {
                case BACNET_APPLICATION_TAG_NULL:
                    len = 0;
                    break;
                case BACNET_APPLICATION_TAG_BOOLEAN:
                    value->type.Boolean = decode_boolean(tag.len_value_type);
                    len = 0;
                    break;
                case BACNET_APPLICATION_TAG_UNSIGNED_INT:
                    len = bacnet_unsigned_decode(
                        &apdu[apdu_len], apdu_size - apdu_len,
                        tag.len_value_type, &value->type.Unsigned_Int);
                    break;
                case BACNET_APPLICATION_TAG_SIGNED_INT:
                    len = bacnet_signed_decode(
                        &apdu[apdu_len], apdu_size - apdu_len,
                        tag.len_value_type, &value->type.Signed_Int);
                    break;
                case BACNET_APPLICATION_TAG_REAL:
                    len = bacnet_real_decode(
                        &apdu[apdu_len], apdu_size - apdu_len,
                        tag.len_value_type, &value->type.Real);
                    break;
                case BACNET_APPLICATION_TAG_DOUBLE:
                    len = bacnet_double_decode(
                        &apdu[apdu_len], apdu_size - apdu_len,
                        tag.len_value_type, &value->type.Double);
                    break;
                case BACNET_APPLICATION_TAG_OCTET_STRING:
                    len = bacnet_octet_string_decode(
                        &apdu[apdu_len], apdu_size - apdu_len,
                        tag.len_value_type, value->type.Octet_String);
                    break;
                case BACNET_APPLICATION_TAG_CHARACTER_STRING:
                    len = bacnet_character_string_decode(
                        &apdu[apdu_len], apdu_size - apdu_len,
                        tag.len_value_type, value->type.Character_String);
                    break;
                case BACNET_APPLICATION_TAG_BIT_STRING:
                    len = bacnet_bitstring_decode(
                        &apdu[apdu_len], apdu_size - apdu_len,
                        tag.len_value_type, value->type.Bit_String);
                    break;
                case BACNET_APPLICATION_TAG_ENUMERATED:
                    len = bacnet_enumerated_decode(
                        &apdu[apdu_len], apdu_size - apdu_len,
                        tag.len_value_type, &value->type.Enumerated);
                    break;
                case BACNET_APPLICATION_TAG_DATE:
                    len = bacnet_date_decode(
                        &apdu[apdu_len], apdu_size - apdu_len,
                        tag.len_value_type, &value->type.Date);
                    break;
                case BACNET_APPLICATION_TAG_TIME:
                    len = bacnet_time_decode(
                        &apdu[apdu_len], apdu_size - apdu_len,
                        tag.len_value_type, &value->type.Time);
                    break;
                case BACNET_APPLICATION_TAG_OBJECT_ID:
                    len = bacnet_object_id_decode(
                        &apdu[apdu_len], apdu_size - apdu_len,
                        tag.len_value_type, &value->type.Object_Id.type,
                        &value->type.Object_Id.instance);
                    break;
                default:
                    len = 0;
                    break;
            }
            if ((len == 0) && (tag.number != BACNET_APPLICATION_TAG_NULL) &&
                (tag.number != BACNET_APPLICATION_TAG_BOOLEAN) &&
                (tag.number != BACNET_APPLICATION_TAG_OCTET_STRING)) {
                /* tags that have a valid zero length */
                /* indicate that we were not able to decode the value */
                value->tag = MAX_BACNET_APPLICATION_TAG;
            } else {
                value->tag = tag.number;
                if (len >= 0) {
                    apdu_len += len;
                } else {
                    apdu_len = BACNET_STATUS_ERROR;
                }
            }
        } else if (tag.opening) {
            switch (tag.number) {
                case 0:
                    len = bacapp_property_value_decode(
                        &apdu[apdu_len], apdu_size - apdu_len,
                        value->type.Property_Value);
                    if (len > 0) {
                        apdu_len += len;
                        value->tag = BACNET_APPLICATION_TAG_PROPERTY_VALUE;
                        if (bacnet_is_closing_tag_number(
                                &apdu[apdu_len], apdu_size - apdu_len,
                                tag.number, &len)) {
                            apdu_len += len;
                        } else {
                            apdu_len = BACNET_STATUS_ERROR;
                        }
                    } else {
                        apdu_len = BACNET_STATUS_ERROR;
                    }
                    break;
                default:
                    value->tag = MAX_BACNET_APPLICATION_TAG;
                    break;
            }
        }
    }

    return apdu_len;
}

#if BACNET_EVENT_CHANGE_OF_DISCRETE_VALUE_ENABLED
/**
 * @brief Encode the DISCRETE_VALUE event
 * @param apdu  Pointer to the buffer for encoding into
 * @param data  Pointer to the DISCRETE_VALUE used for encoding values
 * @return number of bytes encoded, or zero if unable to encode
 */
static int event_discrete_value_encode(
    uint8_t *apdu, const BACNET_EVENT_DISCRETE_VALUE *value)
{
    int apdu_len = 0; /* total length of the apdu, return value */

    if (!value) {
        return 0;
    }
    switch (value->tag) {
        case BACNET_APPLICATION_TAG_BOOLEAN:
            apdu_len = encode_application_boolean(apdu, value->type.Boolean);
            break;
        case BACNET_APPLICATION_TAG_UNSIGNED_INT:
            apdu_len =
                encode_application_unsigned(apdu, value->type.Unsigned_Int);
            break;
        case BACNET_APPLICATION_TAG_SIGNED_INT:
            apdu_len = encode_application_signed(apdu, value->type.Signed_Int);
            break;
        case BACNET_APPLICATION_TAG_OCTET_STRING:
            apdu_len =
                encode_application_octet_string(apdu, value->type.Octet_String);
            break;
        case BACNET_APPLICATION_TAG_CHARACTER_STRING:
            apdu_len = encode_application_character_string(
                apdu, value->type.Character_String);
            break;
        case BACNET_APPLICATION_TAG_ENUMERATED:
            apdu_len =
                encode_application_enumerated(apdu, value->type.Enumerated);
            break;
        case BACNET_APPLICATION_TAG_DATE:
            apdu_len = encode_application_date(apdu, &value->type.Date);
            break;
        case BACNET_APPLICATION_TAG_TIME:
            apdu_len = encode_application_time(apdu, &value->type.Time);
            break;
        case BACNET_APPLICATION_TAG_OBJECT_ID:
            apdu_len = encode_application_object_id(
                apdu, value->type.Object_Id.type,
                value->type.Object_Id.instance);
            break;
        case BACNET_APPLICATION_TAG_DATETIME:
            apdu_len =
                bacapp_encode_context_datetime(apdu, 0, &value->type.Date_Time);
            break;
        default:
            break;
    }

    return apdu_len;
}
#endif

/**
 * @brief Decode a DISCRETE_VALUE from a buffer
 * @param apdu - the APDU buffer
 * @param apdu_size - the size of the APDU buffer
 * @param value - DISCRETE_VALUE to decode into
 * @return number of bytes decoded or BACNET_STATUS_ERROR on failure.
 */
static int event_discrete_value_decode(
    const uint8_t *apdu, uint32_t apdu_size, BACNET_EVENT_DISCRETE_VALUE *value)
{
    int len, apdu_len = 0;
    BACNET_TAG tag = { 0 };

    if (!value) {
        return 0;
    }
    len = bacnet_tag_decode(&apdu[apdu_len], apdu_size - apdu_len, &tag);
    if (len > 0) {
        apdu_len += len;
        if (tag.application) {
            switch (tag.number) {
                case BACNET_APPLICATION_TAG_BOOLEAN:
                    len = 0;
                    value->type.Boolean = decode_boolean(tag.len_value_type);
                    break;
                case BACNET_APPLICATION_TAG_UNSIGNED_INT:
                    len = bacnet_unsigned_decode(
                        &apdu[apdu_len], apdu_size - apdu_len,
                        tag.len_value_type, &value->type.Unsigned_Int);
                    break;
                case BACNET_APPLICATION_TAG_SIGNED_INT:
                    len = bacnet_signed_decode(
                        &apdu[apdu_len], apdu_size - apdu_len,
                        tag.len_value_type, &value->type.Signed_Int);
                    break;
                case BACNET_APPLICATION_TAG_OCTET_STRING:
                    len = bacnet_octet_string_decode(
                        &apdu[apdu_len], apdu_size - apdu_len,
                        tag.len_value_type, value->type.Octet_String);
                    break;
                case BACNET_APPLICATION_TAG_CHARACTER_STRING:
                    len = bacnet_character_string_decode(
                        &apdu[apdu_len], apdu_size - apdu_len,
                        tag.len_value_type, value->type.Character_String);
                    break;
                case BACNET_APPLICATION_TAG_ENUMERATED:
                    len = bacnet_enumerated_decode(
                        &apdu[apdu_len], apdu_size - apdu_len,
                        tag.len_value_type, &value->type.Enumerated);
                    break;
                case BACNET_APPLICATION_TAG_DATE:
                    len = bacnet_date_decode(
                        &apdu[apdu_len], apdu_size - apdu_len,
                        tag.len_value_type, &value->type.Date);
                    break;
                case BACNET_APPLICATION_TAG_TIME:
                    len = bacnet_time_decode(
                        &apdu[apdu_len], apdu_size - apdu_len,
                        tag.len_value_type, &value->type.Time);
                    break;
                case BACNET_APPLICATION_TAG_OBJECT_ID:
                    len = bacnet_object_id_decode(
                        &apdu[apdu_len], apdu_size - apdu_len,
                        tag.len_value_type, &value->type.Object_Id.type,
                        &value->type.Object_Id.instance);
                    break;
                default:
                    len = 0;
                    break;
            }
            if ((len == 0) && (tag.number != BACNET_APPLICATION_TAG_NULL) &&
                (tag.number != BACNET_APPLICATION_TAG_BOOLEAN) &&
                (tag.number != BACNET_APPLICATION_TAG_OCTET_STRING)) {
                /* tags that have a valid zero length */
                /* indicate that we were not able to decode the value */
                value->tag = MAX_BACNET_APPLICATION_TAG;
            } else {
                value->tag = tag.number;
                if (len >= 0) {
                    apdu_len += len;
                } else {
                    apdu_len = BACNET_STATUS_ERROR;
                }
            }
        } else if (tag.opening) {
            switch (tag.number) {
                case 0:
                    len = bacnet_datetime_decode(
                        &apdu[apdu_len], apdu_size - apdu_len,
                        &value->type.Date_Time);
                    if (len > 0) {
                        apdu_len += len;
                        value->tag = BACNET_APPLICATION_TAG_DATETIME;
                        if (bacnet_is_closing_tag_number(
                                &apdu[apdu_len], apdu_size - apdu_len,
                                tag.number, &len)) {
                            apdu_len += len;
                        } else {
                            apdu_len = BACNET_STATUS_ERROR;
                        }
                    } else {
                        apdu_len = BACNET_STATUS_ERROR;
                    }
                    break;
                default:
                    value->tag = MAX_BACNET_APPLICATION_TAG;
                    break;
            }
        }
    }

    return apdu_len;
}

/**
 * @brief Encode the EventNotification service request
 * @param apdu  Pointer to the buffer for encoding into
 * @param data  Pointer to the service data used for encoding values
 * @return number of bytes encoded, or zero if unable to encode
 */
int event_notify_encode_service_request(
    uint8_t *apdu, const BACNET_EVENT_NOTIFICATION_DATA *data)
{
    int len = 0; /* length of each encoding */
    int apdu_len = 0; /* total length of the apdu, return value */
    const BACNET_PROPERTY_VALUE *value = NULL;

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
#if BACNET_EVENT_DOUBLE_OUT_OF_RANGE_ENABLED
                case EVENT_DOUBLE_OUT_OF_RANGE:
                    /* double-out-of-range[14] SEQUENCE */
                    len = encode_opening_tag(apdu, 14);
                    apdu_len += len;
                    if (apdu) {
                        apdu += len;
                    }
                    /* exceeding-value[0] Double */
                    len = encode_context_double(
                        apdu, 0,
                        data->notificationParams.doubleOutOfRange
                            .exceedingValue);
                    apdu_len += len;
                    if (apdu) {
                        apdu += len;
                    }
                    /* status-flags[1] BACnetStatusFlags */
                    len = encode_context_bitstring(
                        apdu, 1,
                        &data->notificationParams.doubleOutOfRange.statusFlags);
                    apdu_len += len;
                    if (apdu) {
                        apdu += len;
                    }
                    /* deadband[2] Double */
                    len = encode_context_double(
                        apdu, 2,
                        data->notificationParams.doubleOutOfRange.deadband);
                    apdu_len += len;
                    if (apdu) {
                        apdu += len;
                    }
                    /* exceeded-limit[3] Double */
                    len = encode_context_double(
                        apdu, 3,
                        data->notificationParams.doubleOutOfRange
                            .exceededLimit);
                    apdu_len += len;
                    if (apdu) {
                        apdu += len;
                    }
                    len = encode_closing_tag(apdu, 14);
                    apdu_len += len;
                    if (apdu) {
                        apdu += len;
                    }
                    break;
#endif
#if BACNET_EVENT_SIGNED_OUT_OF_RANGE_ENABLED
                case EVENT_SIGNED_OUT_OF_RANGE:
                    /* signed-out-of-range[15] SEQUENCE */
                    len = encode_opening_tag(apdu, 15);
                    apdu_len += len;
                    if (apdu) {
                        apdu += len;
                    }
                    /* exceeding-value[0] Integer */
                    len = encode_context_signed(
                        apdu, 0,
                        data->notificationParams.signedOutOfRange
                            .exceedingValue);
                    apdu_len += len;
                    if (apdu) {
                        apdu += len;
                    }
                    /* status-flags[1] BACnetStatusFlags */
                    len = encode_context_bitstring(
                        apdu, 1,
                        &data->notificationParams.signedOutOfRange.statusFlags);
                    apdu_len += len;
                    if (apdu) {
                        apdu += len;
                    }
                    /* deadband[2] Unsigned */
                    len = encode_context_unsigned(
                        apdu, 2,
                        data->notificationParams.signedOutOfRange.deadband);
                    apdu_len += len;
                    if (apdu) {
                        apdu += len;
                    }
                    /* exceeded-limit[3] Integer */
                    len = encode_context_signed(
                        apdu, 3,
                        data->notificationParams.signedOutOfRange
                            .exceededLimit);
                    apdu_len += len;
                    if (apdu) {
                        apdu += len;
                    }
                    len = encode_closing_tag(apdu, 15);
                    apdu_len += len;
                    if (apdu) {
                        apdu += len;
                    }
                    break;
#endif
#if BACNET_EVENT_UNSIGNED_OUT_OF_RANGE_ENABLED
                case EVENT_UNSIGNED_OUT_OF_RANGE:
                    /* unsigned-out-of-range[16] SEQUENCE */
                    len = encode_opening_tag(apdu, EVENT_UNSIGNED_OUT_OF_RANGE);
                    apdu_len += len;
                    if (apdu) {
                        apdu += len;
                    }
                    /* exceeding-value[0] Unsigned */
                    len = encode_context_unsigned(
                        apdu, 0,
                        data->notificationParams.unsignedOutOfRange
                            .exceedingValue);
                    apdu_len += len;
                    if (apdu) {
                        apdu += len;
                    }
                    /* status-flags[1] BACnetStatusFlags */
                    len = encode_context_bitstring(
                        apdu, 1,
                        &data->notificationParams.unsignedOutOfRange
                             .statusFlags);
                    apdu_len += len;
                    if (apdu) {
                        apdu += len;
                    }
                    /* deadband[2] Unsigned */
                    len = encode_context_unsigned(
                        apdu, 2,
                        data->notificationParams.unsignedOutOfRange.deadband);
                    apdu_len += len;
                    if (apdu) {
                        apdu += len;
                    }
                    /* exceeded-limit[3] Unsigned */
                    len = encode_context_unsigned(
                        apdu, 3,
                        data->notificationParams.unsignedOutOfRange
                            .exceededLimit);
                    apdu_len += len;
                    if (apdu) {
                        apdu += len;
                    }
                    len = encode_closing_tag(apdu, EVENT_UNSIGNED_OUT_OF_RANGE);
                    apdu_len += len;
                    if (apdu) {
                        apdu += len;
                    }
                    break;
#endif
#if BACNET_EVENT_CHANGE_OF_CHARACTERSTRING_ENABLED
                case EVENT_CHANGE_OF_CHARACTERSTRING:
                    len = encode_opening_tag(
                        apdu, EVENT_CHANGE_OF_CHARACTERSTRING);
                    apdu_len += len;
                    if (apdu) {
                        apdu += len;
                    }
                    /* changed-value [0] CharacterString */
                    len = encode_context_character_string(
                        apdu, 0,
                        data->notificationParams.changeOfCharacterstring
                            .changedValue);
                    apdu_len += len;
                    if (apdu) {
                        apdu += len;
                    }
                    /* status-flags[1] BACnetStatusFlags */
                    len = encode_context_bitstring(
                        apdu, 1,
                        &data->notificationParams.changeOfCharacterstring
                             .statusFlags);
                    apdu_len += len;
                    if (apdu) {
                        apdu += len;
                    }
                    /* alarm-value [2] CharacterString */
                    len = encode_context_character_string(
                        apdu, 2,
                        data->notificationParams.changeOfCharacterstring
                            .alarmValue);
                    apdu_len += len;
                    if (apdu) {
                        apdu += len;
                    }
                    len = encode_closing_tag(
                        apdu, EVENT_CHANGE_OF_CHARACTERSTRING);
                    apdu_len += len;
                    if (apdu) {
                        apdu += len;
                    }
                    break;
#endif
#if BACNET_EVENT_CHANGE_OF_STATUS_FLAGS_ENABLED
                case EVENT_CHANGE_OF_STATUS_FLAGS:
                    len =
                        encode_opening_tag(apdu, EVENT_CHANGE_OF_STATUS_FLAGS);
                    apdu_len += len;
                    if (apdu) {
                        apdu += len;
                    }
                    /* present-value [0] ABSTRACT-SYNTAX.&Type OPTIONAL */
                    if (data->notificationParams.changeOfStatusFlags
                            .presentValue.tag !=
                        BACNET_APPLICATION_TAG_EMPTYLIST) {
                        len = encode_opening_tag(apdu, 0);
                        apdu_len += len;
                        if (apdu) {
                            apdu += len;
                        }
                        len = event_extended_parameter_encode(
                            apdu,
                            &data->notificationParams.changeOfStatusFlags
                                 .presentValue);
                        apdu_len += len;
                        if (apdu) {
                            apdu += len;
                        }
                        len = encode_closing_tag(apdu, 0);
                        apdu_len += len;
                        if (apdu) {
                            apdu += len;
                        }
                    }
                    /* referenced-flags[1] BACnetStatusFlags */
                    len = encode_context_bitstring(
                        apdu, 1,
                        &data->notificationParams.changeOfStatusFlags
                             .referencedFlags);
                    apdu_len += len;
                    if (apdu) {
                        apdu += len;
                    }
                    len =
                        encode_closing_tag(apdu, EVENT_CHANGE_OF_STATUS_FLAGS);
                    apdu_len += len;
                    if (apdu) {
                        apdu += len;
                    }
                    break;
#endif
#if BACNET_EVENT_CHANGE_OF_RELIABILITY_ENABLED
                case EVENT_CHANGE_OF_RELIABILITY:
                    len = encode_opening_tag(apdu, EVENT_CHANGE_OF_RELIABILITY);
                    apdu_len += len;
                    if (apdu) {
                        apdu += len;
                    }
                    /* reliability [0] BACnetReliability */
                    len = encode_context_enumerated(
                        apdu, 0,
                        data->notificationParams.changeOfReliability
                            .reliability);
                    apdu_len += len;
                    if (apdu) {
                        apdu += len;
                    }
                    /* status-flags [1] BACnetStatusFlags */
                    len = encode_context_bitstring(
                        apdu, 1,
                        &data->notificationParams.changeOfReliability
                             .statusFlags);
                    apdu_len += len;
                    if (apdu) {
                        apdu += len;
                    }
                    /* property-values [2] SEQUENCE OF BACnetPropertyValue */
                    len = encode_opening_tag(apdu, 2);
                    apdu_len += len;
                    if (apdu) {
                        apdu += len;
                    }
                    value = data->notificationParams.changeOfReliability
                                .propertyValues;
                    while (value) {
                        len = bacapp_property_value_encode(apdu, value);
                        apdu_len += len;
                        if (apdu) {
                            apdu += len;
                        }
                        value = value->next;
                    }
                    len = encode_closing_tag(apdu, 2);
                    apdu_len += len;
                    if (apdu) {
                        apdu += len;
                    }
                    len = encode_closing_tag(apdu, EVENT_CHANGE_OF_RELIABILITY);
                    apdu_len += len;
                    if (apdu) {
                        apdu += len;
                    }
                    break;
#endif
#if BACNET_EVENT_CHANGE_OF_DISCRETE_VALUE_ENABLED
                case EVENT_CHANGE_OF_DISCRETE_VALUE:
                    /* change-of-discrete-value [21] SEQUENCE */
                    len = encode_opening_tag(
                        apdu, EVENT_CHANGE_OF_DISCRETE_VALUE);
                    apdu_len += len;
                    if (apdu) {
                        apdu += len;
                    }
                    /* new-value [0] CHOICE */
                    len = encode_opening_tag(apdu, 0);
                    apdu_len += len;
                    if (apdu) {
                        apdu += len;
                    }
                    len = event_discrete_value_encode(
                        apdu,
                        &data->notificationParams.changeOfDiscreteValue
                             .newValue);
                    apdu_len += len;
                    if (apdu) {
                        apdu += len;
                    }
                    len = encode_closing_tag(apdu, 0);
                    apdu_len += len;
                    if (apdu) {
                        apdu += len;
                    }
                    /* status-flags[1] BACnetStatusFlags */
                    len = encode_context_bitstring(
                        apdu, 1,
                        &data->notificationParams.changeOfDiscreteValue
                             .statusFlags);
                    apdu_len += len;
                    if (apdu) {
                        apdu += len;
                    }
                    len = encode_closing_tag(
                        apdu, EVENT_CHANGE_OF_DISCRETE_VALUE);
                    apdu_len += len;
                    if (apdu) {
                        apdu += len;
                    }
                    break;
#endif
#if BACNET_EVENT_CHANGE_OF_TIMER_ENABLED
                case EVENT_CHANGE_OF_TIMER:
                    /* change-of-timer [22] SEQUENCE */
                    len = encode_opening_tag(apdu, EVENT_CHANGE_OF_TIMER);
                    apdu_len += len;
                    if (apdu) {
                        apdu += len;
                    }
                    /* new-state [0] BACnetTimerState */
                    len = encode_context_enumerated(
                        apdu, 0,
                        data->notificationParams.changeOfTimer.newState);
                    apdu_len += len;
                    if (apdu) {
                        apdu += len;
                    }
                    /* status-flags [1] BACnetStatusFlags */
                    len = encode_context_bitstring(
                        apdu, 1,
                        &data->notificationParams.changeOfTimer.statusFlags);
                    apdu_len += len;
                    if (apdu) {
                        apdu += len;
                    }
                    /* update-time [2] BACnetDateTime */
                    len = bacapp_encode_context_datetime(
                        apdu, 2,
                        &data->notificationParams.changeOfTimer.updateTime);
                    apdu_len += len;
                    if (apdu) {
                        apdu += len;
                    }
                    /* last-state-change [3] BACnetTimerTransition OPTIONAL */
                    if (data->notificationParams.changeOfTimer.lastStateChange <
                        TIMER_TRANSITION_MAX) {
                        len = encode_context_enumerated(
                            apdu, 3,
                            data->notificationParams.changeOfTimer
                                .lastStateChange);
                        apdu_len += len;
                        if (apdu) {
                            apdu += len;
                        }
                    }
                    /* initial-timeout [4] Unsigned OPTIONAL */
                    if (data->notificationParams.changeOfTimer.initialTimeout >
                        0) {
                        len = encode_context_enumerated(
                            apdu, 4,
                            data->notificationParams.changeOfTimer
                                .initialTimeout);
                        apdu_len += len;
                        if (apdu) {
                            apdu += len;
                        }
                    }
                    /* expiration-time [5] BACnetDateTime OPTIONAL */
                    if (!datetime_wildcard(
                            &data->notificationParams.changeOfTimer
                                 .expirationTime)) {
                        len = bacapp_encode_context_datetime(
                            apdu, 5,
                            &data->notificationParams.changeOfTimer
                                 .expirationTime);
                        apdu_len += len;
                        if (apdu) {
                            apdu += len;
                        }
                    }
                    len = encode_closing_tag(apdu, EVENT_CHANGE_OF_TIMER);
                    apdu_len += len;
                    if (apdu) {
                        apdu += len;
                    }
                    break;
#endif
                case EVENT_NONE:
                    len = encode_opening_tag(apdu, EVENT_NONE);
                    apdu_len += len;
                    if (apdu) {
                        apdu += len;
                    }
                    len = encode_closing_tag(apdu, EVENT_NONE);
                    apdu_len += len;
                    if (apdu) {
                        apdu += len;
                    }
                    break;
#if BACNET_EVENT_EXTENDED_ENABLED
                case EVENT_EXTENDED:
                    len = encode_opening_tag(apdu, EVENT_EXTENDED);
                    apdu_len += len;
                    if (apdu) {
                        apdu += len;
                    }
                    /* vendor-id [0] Unsigned16 */
                    len = encode_context_unsigned(
                        apdu, 0, data->notificationParams.extended.vendorID);
                    apdu_len += len;
                    if (apdu) {
                        apdu += len;
                    }
                    /* extended-event-type [1] Unsigned */
                    len = encode_context_unsigned(
                        apdu, 1,
                        data->notificationParams.extended.extendedEventType);
                    apdu_len += len;
                    if (apdu) {
                        apdu += len;
                    }
                    /* parameters [2] SEQUENCE OF CHOICE */
                    len = encode_opening_tag(apdu, 2);
                    apdu_len += len;
                    if (apdu) {
                        apdu += len;
                    }
                    len = event_extended_parameter_encode(
                        apdu, &data->notificationParams.extended.parameters);
                    apdu_len += len;
                    if (apdu) {
                        apdu += len;
                    }
                    len = encode_closing_tag(apdu, 2);
                    apdu_len += len;
                    if (apdu) {
                        apdu += len;
                    }
                    len = encode_closing_tag(apdu, EVENT_EXTENDED);
                    apdu_len += len;
                    if (apdu) {
                        apdu += len;
                    }
                    break;
#endif
                default:
                    if (data->eventType >= EVENT_PROPRIETARY_MIN &&
                        data->eventType <= EVENT_PROPRIETARY_MAX) {
                        len =
                            encode_opening_tag(apdu, EVENT_COMPLEX_EVENT_TYPE);
                        apdu_len += len;
                        if (apdu) {
                            apdu += len;
                        }
#if BACNET_DECODE_COMPLEX_EVENT_TYPE_PARAMETERS
                        value =
                            data->notificationParams.complexEventType.values;
#endif
                        while (value) {
                            len = bacapp_property_value_encode(apdu, value);
                            apdu_len += len;
                            if (apdu) {
                                apdu += len;
                            }
                            value = value->next;
                        }
                        len =
                            encode_closing_tag(apdu, EVENT_COMPLEX_EVENT_TYPE);
                        apdu_len += len;
                        if (apdu) {
                            apdu += len;
                        }
                    } else {
                        /* FIXME: add or enable an encoder for event type */
                        assert(0);
                    }
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
    int len = 0, tag_len = 0;
    BACNET_UNSIGNED_INTEGER unsigned_value = 0;
    BACNET_OBJECT_ID object_id = { 0 };
    BACNET_TIMESTAMP timestamp_value = { 0 };
    BACNET_CHARACTER_STRING *cstring = NULL;
    BACNET_BIT_STRING *bstring = NULL;
    BACNET_PROPERTY_STATE *property_state = NULL;
    BACNET_NOTIFY_TYPE notify_type = NOTIFY_MAX;
    BACNET_EVENT_TYPE event_type = EVENT_NONE;
    BACNET_DEVICE_OBJECT_REFERENCE *dev_obj_ref = NULL;
    BACNET_DEVICE_OBJECT_PROPERTY_REFERENCE *dev_obj_prop_ref = NULL;
    BACNET_AUTHENTICATION_FACTOR *auth_factor = NULL;
    BACNET_PROPERTY_VALUE *property_value = NULL;
    BACNET_EVENT_EXTENDED_PARAMETER *parameter_value = NULL;
    BACNET_EVENT_DISCRETE_VALUE *discrete_value = NULL;
    BACNET_DATE_TIME *datetime_value = NULL;
    bool boolean_value = false;
    float real_value = 0.0f;
    double double_value = 0.0;
    int32_t signed_value = 0;
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
                if (data) {
                    data->notificationClass = (uint32_t)unsigned_value;
                }
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
                if (data) {
                    data->priority = (uint8_t)unsigned_value;
                }
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
                if (data) {
#if BACNET_DECODE_COMPLEX_EVENT_TYPE_PARAMETERS
                    property_value =
                        data->notificationParams.complexEventType.values;
#endif
                }
                bacapp_property_value_list_init(
                    property_value, BACNET_COMPLEX_EVENT_TYPE_MAX_PARAMETERS);
                while (apdu_len < apdu_size) {
                    len = bacapp_property_value_decode(
                        &apdu[apdu_len], apdu_size - apdu_len, property_value);
                    if (len > 0) {
                        apdu_len += len;
                    } else {
                        return BACNET_STATUS_ERROR;
                    }
                    /* end of list? */
                    if (bacnet_is_closing_tag_number(
                            &apdu[apdu_len], apdu_size - apdu_len,
                            EVENT_COMPLEX_EVENT_TYPE, &len)) {
                        apdu_len += len;
                        if (property_value) {
                            /* mark the end of the list */
                            property_value->next = NULL;
                        }
                        break;
                    }
                    /* is there another slot in the data store? */
                    if (property_value) {
                        property_value = property_value->next;
                    }
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
                    /* change-of-bitstring [0] SEQUENCE */
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
                    /* change-of-state [1] SEQUENCE */
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
                    /* change-of-value [2] SEQUENCE */
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
                    /* command-failure [3] SEQUENCE */
                    /* command-value [0] ABSTRACT-SYNTAX.&Type
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
                    /* floating-limit [4] SEQUENCE*/
                    /* reference-value[0] Real */
                    len = bacnet_real_context_decode(
                        &apdu[apdu_len], apdu_size - apdu_len, 0, &real_value);
                    if (len > 0) {
                        apdu_len += len;
                        if (data) {
                            data->notificationParams.floatingLimit
                                .referenceValue = real_value;
                        }
                    } else {
                        return BACNET_STATUS_ERROR;
                    }
                    /* status-flags[1] BACnetStatusFlags */
                    if (data) {
                        bstring =
                            &data->notificationParams.floatingLimit.statusFlags;
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
                    /* setpoint-value[2] Real */
                    len = bacnet_real_context_decode(
                        &apdu[apdu_len], apdu_size - apdu_len, 2, &real_value);
                    if (len > 0) {
                        apdu_len += len;
                        if (data) {
                            data->notificationParams.floatingLimit
                                .setPointValue = real_value;
                        }
                    } else {
                        return BACNET_STATUS_ERROR;
                    }
                    /* error-limit[3] Real */
                    len = bacnet_real_context_decode(
                        &apdu[apdu_len], apdu_size - apdu_len, 3, &real_value);
                    if (len > 0) {
                        apdu_len += len;
                        if (data) {
                            data->notificationParams.floatingLimit.errorLimit =
                                real_value;
                        }
                    } else {
                        return BACNET_STATUS_ERROR;
                    }
                    break;
                case EVENT_OUT_OF_RANGE:
                    /* out-of-range [5] SEQUENCE */
                    /* exceeding-value[0] Real */
                    len = bacnet_real_context_decode(
                        &apdu[apdu_len], apdu_size - apdu_len, 0, &real_value);
                    if (len > 0) {
                        apdu_len += len;
                        if (data) {
                            data->notificationParams.outOfRange.exceedingValue =
                                real_value;
                        }
                    } else {
                        return BACNET_STATUS_ERROR;
                    }
                    /* status-flags[1] BACnetStatusFlags */
                    if (data) {
                        bstring =
                            &data->notificationParams.outOfRange.statusFlags;
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
                    /* deadband[2] Real */
                    len = bacnet_real_context_decode(
                        &apdu[apdu_len], apdu_size - apdu_len, 2, &real_value);
                    if (len > 0) {
                        apdu_len += len;
                        if (data) {
                            data->notificationParams.outOfRange.deadband =
                                real_value;
                        }
                    } else {
                        return BACNET_STATUS_ERROR;
                    }
                    /* exceeded-limit[3] Real */
                    len = bacnet_real_context_decode(
                        &apdu[apdu_len], apdu_size - apdu_len, 3, &real_value);
                    if (len > 0) {
                        apdu_len += len;
                        if (data) {
                            data->notificationParams.outOfRange.exceededLimit =
                                real_value;
                        }
                    } else {
                        return BACNET_STATUS_ERROR;
                    }
                    break;

                case EVENT_CHANGE_OF_LIFE_SAFETY:
                    /* change-of-life-safety [8] SEQUENCE */
                    /* new-state[0] BACnetLifeSafetyState */
                    len = bacnet_enumerated_context_decode(
                        &apdu[apdu_len], apdu_size - apdu_len, 0, &enum_value);
                    if (len > 0) {
                        apdu_len += len;
                        if (enum_value > LIFE_SAFETY_STATE_PROPRIETARY_MAX) {
                            enum_value = LIFE_SAFETY_STATE_PROPRIETARY_MAX;
                        }
                        if (data) {
                            data->notificationParams.changeOfLifeSafety
                                .newState =
                                (BACNET_LIFE_SAFETY_STATE)enum_value;
                        }
                    } else {
                        return BACNET_STATUS_ERROR;
                    }
                    /* new-mode[1] BACnetLifeSafetyMode */
                    len = bacnet_enumerated_context_decode(
                        &apdu[apdu_len], apdu_size - apdu_len, 1, &enum_value);
                    if (len > 0) {
                        apdu_len += len;
                        if (enum_value > LIFE_SAFETY_MODE_PROPRIETARY_MAX) {
                            enum_value = LIFE_SAFETY_MODE_PROPRIETARY_MAX;
                        }
                        if (data) {
                            data->notificationParams.changeOfLifeSafety
                                .newMode = (BACNET_LIFE_SAFETY_MODE)enum_value;
                        }
                    } else {
                        return BACNET_STATUS_ERROR;
                    }
                    /* status-flags[2] BACnetStatusFlags */
                    if (data) {
                        bstring = &data->notificationParams.changeOfLifeSafety
                                       .statusFlags;
                    } else {
                        bstring = NULL;
                    }
                    len = bacnet_bitstring_context_decode(
                        &apdu[apdu_len], apdu_size - apdu_len, 2, bstring);
                    if (len > 0) {
                        apdu_len += len;
                    } else {
                        return BACNET_STATUS_ERROR;
                    }
                    /* operation-expected[3] BACnetLifeSafetyOperation */
                    len = bacnet_enumerated_context_decode(
                        &apdu[apdu_len], apdu_size - apdu_len, 3, &enum_value);
                    if (len > 0) {
                        apdu_len += len;
                        if (enum_value > LIFE_SAFETY_OP_PROPRIETARY_MAX) {
                            enum_value = LIFE_SAFETY_OP_PROPRIETARY_MAX;
                        }
                        if (data) {
                            data->notificationParams.changeOfLifeSafety
                                .operationExpected =
                                (BACNET_LIFE_SAFETY_OPERATION)enum_value;
                        }
                    } else {
                        return BACNET_STATUS_ERROR;
                    }
                    break;
                case EVENT_EXTENDED:
                    /* extended [9] SEQUENCE */
                    /* vendor-id[0] Unsigned16 */
                    len = bacnet_unsigned_context_decode(
                        &apdu[apdu_len], apdu_size - apdu_len, 0,
                        &unsigned_value);
                    if (len > 0) {
                        apdu_len += len;
                        if (unsigned_value <= UINT16_MAX) {
#if BACNET_EVENT_EXTENDED_ENABLED
                            if (data) {
                                data->notificationParams.extended.vendorID =
                                    (uint16_t)unsigned_value;
                            }
#endif
                        } else {
                            return BACNET_STATUS_ERROR;
                        }
                    } else {
                        return BACNET_STATUS_ERROR;
                    }
                    /* extended-event-type[1] Unsigned */
                    len = bacnet_unsigned_context_decode(
                        &apdu[apdu_len], apdu_size - apdu_len, 1,
                        &unsigned_value);
                    if (len > 0) {
                        apdu_len += len;
                        if (data) {
#if BACNET_EVENT_EXTENDED_ENABLED
                            data->notificationParams.extended
                                .extendedEventType = unsigned_value;
#endif
                        }
                    } else {
                        return BACNET_STATUS_ERROR;
                    }
                    /* parameters[2] SEQUENCE OF CHOICE */
                    if (bacnet_is_opening_tag_number(
                            &apdu[apdu_len], apdu_size - apdu_len, 2, &len)) {
                        apdu_len += len;
#if BACNET_EVENT_EXTENDED_ENABLED
                        if (data) {
                            parameter_value =
                                &data->notificationParams.extended.parameters;
                        } else
#endif
                        {
                            parameter_value = NULL;
                        }
                        len = event_extended_parameter_decode(
                            &apdu[apdu_len], apdu_size - apdu_len,
                            parameter_value);
                        if (len < 0) {
                            return BACNET_STATUS_ERROR;
                        }
                        apdu_len += len;
                        if (bacnet_is_closing_tag_number(
                                &apdu[apdu_len], apdu_size - apdu_len, 2,
                                &len)) {
                            apdu_len += len;
                        } else {
                            return BACNET_STATUS_ERROR;
                        }
                    } else {
                        return BACNET_STATUS_ERROR;
                    }
                    break;
                case EVENT_BUFFER_READY:
                    /* buffer-ready [10] SEQUENCE */
                    /* buffer-property[0] BACnetDeviceObjectPropertyReference */
                    if (data) {
                        dev_obj_prop_ref = &data->notificationParams.bufferReady
                                                .bufferProperty;
                    } else {
                        dev_obj_prop_ref = NULL;
                    }
                    len =
                        bacnet_device_object_property_reference_context_decode(
                            &apdu[apdu_len], apdu_size - apdu_len, 0,
                            dev_obj_prop_ref);
                    if (len <= 0) {
                        return BACNET_STATUS_ERROR;
                    }
                    apdu_len += len;
                    /* previous-notification[1] Unsigned32 */
                    len = bacnet_unsigned_context_decode(
                        &apdu[apdu_len], apdu_size - apdu_len, 1,
                        &unsigned_value);
                    if (len > 0) {
                        apdu_len += len;
                        if (unsigned_value <= UINT32_MAX) {
                            if (data) {
                                data->notificationParams.bufferReady
                                    .previousNotification =
                                    (uint32_t)unsigned_value;
                            }
                        } else {
                            return BACNET_STATUS_ERROR;
                        }
                    } else {
                        return BACNET_STATUS_ERROR;
                    }
                    /* current-notification[2] Unsigned32 */
                    len = bacnet_unsigned_context_decode(
                        &apdu[apdu_len], apdu_size - apdu_len, 2,
                        &unsigned_value);
                    if (len > 0) {
                        apdu_len += len;
                        if (unsigned_value <= UINT32_MAX) {
                            if (data) {
                                data->notificationParams.bufferReady
                                    .currentNotification =
                                    (uint32_t)unsigned_value;
                            }
                        } else {
                            return BACNET_STATUS_ERROR;
                        }
                    } else {
                        return BACNET_STATUS_ERROR;
                    }
                    break;
                case EVENT_UNSIGNED_RANGE:
                    /* unsigned-range[11] SEQUENCE*/
                    /* exceeding-value[0] Unsigned */
                    len = bacnet_unsigned_context_decode(
                        &apdu[apdu_len], apdu_size - apdu_len, 0,
                        &unsigned_value);
                    if (len > 0) {
                        apdu_len += len;
                        if (unsigned_value <= UINT32_MAX) {
                            if (data) {
                                data->notificationParams.unsignedRange
                                    .exceedingValue = (uint32_t)unsigned_value;
                            }
                        } else {
                            return BACNET_STATUS_ERROR;
                        }
                    } else {
                        return BACNET_STATUS_ERROR;
                    }
                    /* status-flags[1] BACnetStatusFlags */
                    if (data) {
                        bstring =
                            &data->notificationParams.unsignedRange.statusFlags;
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
                    /* exceeded-limit[2] Unsigned */
                    len = bacnet_unsigned_context_decode(
                        &apdu[apdu_len], apdu_size - apdu_len, 2,
                        &unsigned_value);
                    if (len > 0) {
                        apdu_len += len;
                        if (unsigned_value <= UINT32_MAX) {
                            if (data) {
                                data->notificationParams.unsignedRange
                                    .exceededLimit = (uint32_t)unsigned_value;
                            }
                        } else {
                            return BACNET_STATUS_ERROR;
                        }
                    } else {
                        return BACNET_STATUS_ERROR;
                    }
                    break;

                case EVENT_ACCESS_EVENT:
                    /* access-event[13] SEQUENCE */
                    /* access-event[0] BACnetAccessEvent */
                    len = bacnet_enumerated_context_decode(
                        &apdu[apdu_len], apdu_size - apdu_len, 0, &enum_value);
                    if (len > 0) {
                        apdu_len += len;
                        if (enum_value > ACCESS_EVENT_PROPRIETARY_MAX) {
                            enum_value = ACCESS_EVENT_PROPRIETARY_MAX;
                        }
                        if (data) {
                            data->notificationParams.accessEvent.accessEvent =
                                (BACNET_ACCESS_EVENT)enum_value;
                        }
                    } else {
                        return BACNET_STATUS_ERROR;
                    }
                    /* status-flags[1] BACnetStatusFlags */
                    if (data) {
                        bstring =
                            &data->notificationParams.accessEvent.statusFlags;
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
                    /* access-event-tag[2] Unsigned */
                    len = bacnet_unsigned_context_decode(
                        &apdu[apdu_len], apdu_size - apdu_len, 2,
                        &unsigned_value);
                    if (len > 0) {
                        apdu_len += len;
                        if (data) {
                            data->notificationParams.accessEvent
                                .accessEventTag = unsigned_value;
                        }
                    } else {
                        return BACNET_STATUS_ERROR;
                    }
                    /* access-event-time[3] BACnetTimeStamp */
                    len = bacnet_timestamp_context_decode(
                        &apdu[apdu_len], apdu_size - apdu_len, 3,
                        &timestamp_value);
                    if (len > 0) {
                        apdu_len += len;
                        if (data) {
                            bacapp_timestamp_copy(
                                &data->notificationParams.accessEvent
                                     .accessEventTime,
                                &timestamp_value);
                        }
                    } else {
                        return BACNET_STATUS_ERROR;
                    }
                    /* access-credential[4] BACnetDeviceObjectReference */
                    if (data) {
                        dev_obj_ref = &data->notificationParams.accessEvent
                                           .accessCredential;
                    } else {
                        dev_obj_ref = NULL;
                    }
                    len = bacnet_device_object_reference_context_decode(
                        &apdu[apdu_len], apdu_size - apdu_len, 4, dev_obj_ref);
                    if (len > 0) {
                        apdu_len += len;
                    } else {
                        return BACNET_STATUS_ERROR;
                    }
                    /* authentication-factor[5] BACnetAuthenticationFactor
                     * OPTIONAL */
                    if (data) {
                        auth_factor = &data->notificationParams.accessEvent
                                           .authenticationFactor;
                    } else {
                        auth_factor = NULL;
                    }
                    len = bacnet_authentication_factor_context_decode(
                        &apdu[apdu_len], apdu_size - apdu_len, 5, auth_factor);
                    if (len > 0) {
                        apdu_len += len;
                    } else if (len == 0) {
                        /* OPTIONAL - set default */
                        if (data) {
                            data->notificationParams.accessEvent
                                .authenticationFactor.format_type =
                                AUTHENTICATION_FACTOR_MAX;
                        }
                    } else {
                        return BACNET_STATUS_ERROR;
                    }
                    break;
                case EVENT_DOUBLE_OUT_OF_RANGE:
                    /* double-out-of-range[14] SEQUENCE */
                    /* exceeding-value[0] Double */
                    len = bacnet_double_context_decode(
                        &apdu[apdu_len], apdu_size - apdu_len, 0,
                        &double_value);
                    if (len > 0) {
                        apdu_len += len;
                        if (data) {
#if BACNET_EVENT_DOUBLE_OUT_OF_RANGE_ENABLED
                            data->notificationParams.doubleOutOfRange
                                .exceedingValue = double_value;
#endif
                        }
                    } else {
                        return BACNET_STATUS_ERROR;
                    }
                    /* status-flags[1] BACnetStatusFlags */
#if BACNET_EVENT_DOUBLE_OUT_OF_RANGE_ENABLED
                    if (data) {
                        bstring = &data->notificationParams.doubleOutOfRange
                                       .statusFlags;

                    } else
#endif
                    {
                        bstring = NULL;
                    }
                    len = bacnet_bitstring_context_decode(
                        &apdu[apdu_len], apdu_size - apdu_len, 1, bstring);
                    if (len > 0) {
                        apdu_len += len;
                    } else {
                        return BACNET_STATUS_ERROR;
                    }
                    /* deadband[2] Double */
                    len = bacnet_double_context_decode(
                        &apdu[apdu_len], apdu_size - apdu_len, 2,
                        &double_value);
                    if (len > 0) {
                        apdu_len += len;
                        if (data) {
#if BACNET_EVENT_DOUBLE_OUT_OF_RANGE_ENABLED
                            data->notificationParams.doubleOutOfRange.deadband =
                                double_value;
#endif
                        }
                    } else {
                        return BACNET_STATUS_ERROR;
                    }
                    /* exceeded-limit[3] Double */
                    len = bacnet_double_context_decode(
                        &apdu[apdu_len], apdu_size - apdu_len, 3,
                        &double_value);
                    if (len > 0) {
                        apdu_len += len;
                        if (data) {
#if BACNET_EVENT_DOUBLE_OUT_OF_RANGE_ENABLED
                            data->notificationParams.doubleOutOfRange
                                .exceededLimit = double_value;
#endif
                        }
                    } else {
                        return BACNET_STATUS_ERROR;
                    }
                    break;
                case EVENT_SIGNED_OUT_OF_RANGE:
                    /* signed-out-of-range[15] SEQUENCE */
                    /* exceeding-value[0] Integer */
                    len = bacnet_signed_context_decode(
                        &apdu[apdu_len], apdu_size - apdu_len, 0,
                        &signed_value);
                    if (len > 0) {
                        apdu_len += len;
                        if (data) {
#if BACNET_EVENT_SIGNED_OUT_OF_RANGE_ENABLED
                            data->notificationParams.signedOutOfRange
                                .exceedingValue = signed_value;
#endif
                        }
                    } else {
                        return BACNET_STATUS_ERROR;
                    }
                    /* status-flags[1] BACnetStatusFlags */
#if BACNET_EVENT_SIGNED_OUT_OF_RANGE_ENABLED
                    if (data) {
                        bstring = &data->notificationParams.signedOutOfRange
                                       .statusFlags;
                    } else
#endif
                    {
                        bstring = NULL;
                    }
                    len = bacnet_bitstring_context_decode(
                        &apdu[apdu_len], apdu_size - apdu_len, 1, bstring);
                    if (len > 0) {
                        apdu_len += len;
                    } else {
                        return BACNET_STATUS_ERROR;
                    }
                    /* deadband[2] Unsigned */
                    len = bacnet_unsigned_context_decode(
                        &apdu[apdu_len], apdu_size - apdu_len, 2,
                        &unsigned_value);
                    if (len > 0) {
                        apdu_len += len;
                        if (data) {
#if BACNET_EVENT_SIGNED_OUT_OF_RANGE_ENABLED
                            data->notificationParams.signedOutOfRange.deadband =
                                unsigned_value;
#endif
                        }
                    } else {
                        return BACNET_STATUS_ERROR;
                    }
                    /* exceeded-limit[3] Integer */
                    len = bacnet_signed_context_decode(
                        &apdu[apdu_len], apdu_size - apdu_len, 3,
                        &signed_value);
                    if (len > 0) {
                        apdu_len += len;
                        if (data) {
#if BACNET_EVENT_SIGNED_OUT_OF_RANGE_ENABLED
                            data->notificationParams.signedOutOfRange
                                .exceededLimit = signed_value;
#endif
                        }
                    } else {
                        return BACNET_STATUS_ERROR;
                    }
                    break;
                case EVENT_UNSIGNED_OUT_OF_RANGE:
                    /* unsigned-out-of-range[16] SEQUENCE */
                    /* exceeding-value[0] Unsigned */
                    len = bacnet_unsigned_context_decode(
                        &apdu[apdu_len], apdu_size - apdu_len, 0,
                        &unsigned_value);
                    if (len > 0) {
                        apdu_len += len;
                        if (data) {
#if BACNET_EVENT_UNSIGNED_OUT_OF_RANGE_ENABLED
                            data->notificationParams.unsignedOutOfRange
                                .exceedingValue = unsigned_value;
#endif
                        }
                    } else {
                        return BACNET_STATUS_ERROR;
                    }
                    /* status-flags[1] BACnetStatusFlags */
#if BACNET_EVENT_UNSIGNED_OUT_OF_RANGE_ENABLED
                    if (data) {
                        bstring = &data->notificationParams.unsignedOutOfRange
                                       .statusFlags;
                    } else
#endif
                    {
                        bstring = NULL;
                    }
                    len = bacnet_bitstring_context_decode(
                        &apdu[apdu_len], apdu_size - apdu_len, 1, bstring);
                    if (len > 0) {
                        apdu_len += len;
                    } else {
                        return BACNET_STATUS_ERROR;
                    }
                    /* deadband[2] Unsigned */
                    len = bacnet_unsigned_context_decode(
                        &apdu[apdu_len], apdu_size - apdu_len, 2,
                        &unsigned_value);
                    if (len > 0) {
                        apdu_len += len;
                        if (data) {
#if BACNET_EVENT_UNSIGNED_OUT_OF_RANGE_ENABLED
                            data->notificationParams.unsignedOutOfRange
                                .deadband = unsigned_value;
#endif
                        }
                    } else {
                        return BACNET_STATUS_ERROR;
                    }
                    /* exceeded-limit[3] Unsigned */
                    len = bacnet_unsigned_context_decode(
                        &apdu[apdu_len], apdu_size - apdu_len, 3,
                        &unsigned_value);
                    if (len > 0) {
                        apdu_len += len;
                        if (data) {
#if BACNET_EVENT_UNSIGNED_OUT_OF_RANGE_ENABLED
                            data->notificationParams.unsignedOutOfRange
                                .exceededLimit = unsigned_value;
#endif
                        }
                    } else {
                        return BACNET_STATUS_ERROR;
                    }
                    break;
                case EVENT_CHANGE_OF_CHARACTERSTRING:
                    /* change-of-characterstring [17] SEQUENCE */
                    /* changed-value[0] CharacterString */
#if BACNET_EVENT_CHANGE_OF_CHARACTERSTRING_ENABLED
                    if (data) {
                        cstring = data->notificationParams
                                      .changeOfCharacterstring.changedValue;
                    } else
#endif
                    {
                        cstring = NULL;
                    }
                    len = bacnet_character_string_context_decode(
                        &apdu[apdu_len], apdu_size - apdu_len, 0, cstring);
                    if (len > 0) {
                        apdu_len += len;
                    } else {
                        return BACNET_STATUS_ERROR;
                    }
                    /* status-flags[1] BACnetStatusFlags */
#if BACNET_EVENT_CHANGE_OF_CHARACTERSTRING_ENABLED
                    if (data) {
                        bstring = &data->notificationParams
                                       .changeOfCharacterstring.statusFlags;
                    } else
#endif
                    {
                        bstring = NULL;
                    }
                    len = bacnet_bitstring_context_decode(
                        &apdu[apdu_len], apdu_size - apdu_len, 1, bstring);
                    if (len > 0) {
                        apdu_len += len;
                    } else {
                        return BACNET_STATUS_ERROR;
                    }
                    /* alarm-value[2] CharacterString */
#if BACNET_EVENT_CHANGE_OF_CHARACTERSTRING_ENABLED
                    if (data) {
                        cstring = data->notificationParams
                                      .changeOfCharacterstring.alarmValue;
                    } else
#endif
                    {
                        cstring = NULL;
                    }
                    len = bacnet_character_string_context_decode(
                        &apdu[apdu_len], apdu_size - apdu_len, 2, cstring);
                    if (len > 0) {
                        apdu_len += len;
                    } else {
                        return BACNET_STATUS_ERROR;
                    }
                    break;
                case EVENT_CHANGE_OF_STATUS_FLAGS:
                    /* change-of-status-flags[18] SEQUENCE */
                    /* present-value[0] ABSTRACT-SYNTAX.&Type OPTIONAL,
                        -- depends on referenced property */
#if BACNET_EVENT_CHANGE_OF_STATUS_FLAGS_ENABLED
                    if (data) {
                        parameter_value =
                            &data->notificationParams.changeOfStatusFlags
                                 .presentValue;
                    } else
#endif
                    {
                        parameter_value = NULL;
                    }
                    if (bacnet_is_opening_tag_number(
                            &apdu[apdu_len], apdu_size - apdu_len, 0, &len)) {
                        apdu_len += len;
                        len = event_extended_parameter_decode(
                            &apdu[apdu_len], apdu_size - apdu_len,
                            parameter_value);
                        if (len < 0) {
                            return BACNET_STATUS_ERROR;
                        }
                        apdu_len += len;
                        if (bacnet_is_closing_tag_number(
                                &apdu[apdu_len], apdu_size - apdu_len, 0,
                                &len)) {
                            apdu_len += len;
                        } else {
                            return BACNET_STATUS_ERROR;
                        }
                    } else {
                        /* OPTIONAL */
                        if (parameter_value) {
#if BACNET_EVENT_CHANGE_OF_STATUS_FLAGS_ENABLED
                            parameter_value->tag =
                                BACNET_APPLICATION_TAG_EMPTYLIST;
#endif
                        }
                    }
                    /* referenced-flags[1] BACnetStatusFlags*/
#if BACNET_EVENT_CHANGE_OF_STATUS_FLAGS_ENABLED
                    if (data) {
                        bstring = &data->notificationParams.changeOfStatusFlags
                                       .referencedFlags;
                    } else
#endif
                    {
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
                case EVENT_CHANGE_OF_RELIABILITY:
                    /* change-of-reliability[19] SEQUENCE */
                    /* reliability[0] BACnetReliability */
                    len = bacnet_enumerated_context_decode(
                        &apdu[apdu_len], apdu_size - apdu_len, 0, &enum_value);
                    if (len > 0) {
                        apdu_len += len;
                        if (enum_value > RELIABILITY_PROPRIETARY_MAX) {
                            enum_value = RELIABILITY_PROPRIETARY_MAX;
                        }
#if BACNET_EVENT_CHANGE_OF_RELIABILITY_ENABLED
                        if (data) {
                            data->notificationParams.changeOfReliability
                                .reliability = enum_value;
                        }
#endif
                    } else {
                        return BACNET_STATUS_ERROR;
                    }
                    /* status-flags[1] BACnetStatusFlags */
#if BACNET_EVENT_CHANGE_OF_RELIABILITY_ENABLED
                    if (data) {
                        bstring = &data->notificationParams.changeOfReliability
                                       .statusFlags;
                    } else
#endif
                    {
                        bstring = NULL;
                    }
                    len = bacnet_bitstring_context_decode(
                        &apdu[apdu_len], apdu_size - apdu_len, 1, bstring);
                    if (len > 0) {
                        apdu_len += len;
                    } else {
                        return BACNET_STATUS_ERROR;
                    }
                    /* property-values[2] SEQUENCE OF BACnetPropertyValue */
#if BACNET_EVENT_CHANGE_OF_RELIABILITY_ENABLED
                    if (data) {
                        property_value =
                            data->notificationParams.changeOfReliability
                                .propertyValues;
                    } else
#endif
                    {
                        property_value = NULL;
                    }
                    if (bacnet_is_opening_tag_number(
                            &apdu[apdu_len], apdu_size - apdu_len, 2, &len)) {
                        apdu_len += len;
                        while (apdu_len < apdu_size) {
                            len = bacapp_property_value_decode(
                                &apdu[apdu_len], apdu_size - apdu_len,
                                property_value);
                            if (len >= 0) {
                                apdu_len += len;
                            } else {
                                return BACNET_STATUS_ERROR;
                            }
                            /* end of list? */
                            if (bacnet_is_closing_tag_number(
                                    &apdu[apdu_len], apdu_size - apdu_len, 2,
                                    &len)) {
                                apdu_len += len;
#if BACNET_EVENT_CHANGE_OF_RELIABILITY_ENABLED
                                if (property_value) {
                                    /* mark the end of the list */
                                    property_value->next = NULL;
                                }
#endif
                                break;
                            }
#if BACNET_EVENT_CHANGE_OF_RELIABILITY_ENABLED
                            if (property_value) {
                                /* load the next value store */
                                property_value = property_value->next;
                            }
#endif
                        }
                    }
                    break;
                case EVENT_NONE:
                    /* -- CHOICE [20] has been intentionally omitted.
                       It parallels the 'none' event type CHOICE[20] of
                       the BACnetEventParameter production which was
                       introduced for the case an object does not apply
                       an event algorithm */
                    break;
                case EVENT_CHANGE_OF_DISCRETE_VALUE:
                    /* change-of-discrete-value [21] SEQUENCE */
                    /* new-value [0] CHOICE */
                    if (bacnet_is_opening_tag_number(
                            &apdu[apdu_len], apdu_size - apdu_len, 0, &len)) {
                        apdu_len += len;
#if BACNET_EVENT_CHANGE_OF_DISCRETE_VALUE_ENABLED
                        if (data) {
                            discrete_value =
                                &data->notificationParams.changeOfDiscreteValue
                                     .newValue;
                        } else
#endif
                        {
                            discrete_value = NULL;
                        }
                        len = event_discrete_value_decode(
                            &apdu[apdu_len], apdu_size - apdu_len,
                            discrete_value);
                        if (len < 0) {
                            return BACNET_STATUS_ERROR;
                        }
                        apdu_len += len;
                        if (bacnet_is_closing_tag_number(
                                &apdu[apdu_len], apdu_size - apdu_len, 0,
                                &tag_len)) {
                            apdu_len += tag_len;
                        } else {
                            return BACNET_STATUS_ERROR;
                        }
                    } else {
                        return BACNET_STATUS_ERROR;
                    }
                    /* status-flags[1] BACnetStatusFlags*/
#if BACNET_EVENT_CHANGE_OF_DISCRETE_VALUE_ENABLED
                    if (data) {
                        bstring = &data->notificationParams
                                       .changeOfDiscreteValue.statusFlags;
                    } else
#endif
                    {
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
                case EVENT_CHANGE_OF_TIMER:
                    /* change-of-timer [22] SEQUENCE */
                    /* new-state[0] BACnetTimerState */
                    len = bacnet_enumerated_context_decode(
                        &apdu[apdu_len], apdu_size - apdu_len, 0, &enum_value);
                    if (len > 0) {
                        apdu_len += len;
                        if (data) {
                            if (enum_value > TIMER_STATE_MAX) {
                                enum_value = TIMER_STATE_MAX;
                            }
#if BACNET_EVENT_CHANGE_OF_TIMER_ENABLED
                            data->notificationParams.changeOfTimer.newState =
                                enum_value;
#endif
                        }
                    } else {
                        return BACNET_STATUS_ERROR;
                    }
                    /* status-flags[1] BACnetStatusFlags */
#if BACNET_EVENT_CHANGE_OF_TIMER_ENABLED
                    if (data) {
                        bstring =
                            &data->notificationParams.changeOfTimer.statusFlags;
                    } else
#endif
                    {
                        bstring = NULL;
                    }
                    len = bacnet_bitstring_context_decode(
                        &apdu[apdu_len], apdu_size - apdu_len, 1, bstring);
                    if (len > 0) {
                        apdu_len += len;
                    } else {
                        return BACNET_STATUS_ERROR;
                    }
                    /* update-time[2] BACnetDateTime */
#if BACNET_EVENT_CHANGE_OF_TIMER_ENABLED
                    if (data) {
                        datetime_value =
                            &data->notificationParams.changeOfTimer.updateTime;
                    } else
#endif
                    {
                        datetime_value = NULL;
                    }
                    len = bacnet_datetime_context_decode(
                        &apdu[apdu_len], apdu_size - apdu_len, 2,
                        datetime_value);
                    if (len > 0) {
                        apdu_len += len;
                    } else {
                        return BACNET_STATUS_ERROR;
                    }
                    /* last-state-change[3] BACnetTimerTransition OPTIONAL */
                    len = bacnet_enumerated_context_decode(
                        &apdu[apdu_len], apdu_size - apdu_len, 3, &enum_value);
                    if (len > 0) {
                        apdu_len += len;
                        if (enum_value > TIMER_TRANSITION_MAX) {
                            enum_value = TIMER_TRANSITION_MAX;
                        }
#if BACNET_EVENT_CHANGE_OF_TIMER_ENABLED
                        data->notificationParams.changeOfTimer.lastStateChange =
                            enum_value;
#endif
                    } else if (len == 0) {
                        /* OPTIONAL - set default */
#if BACNET_EVENT_CHANGE_OF_TIMER_ENABLED
                        data->notificationParams.changeOfTimer.lastStateChange =
                            TIMER_TRANSITION_MAX;
#endif
                    } else {
                        return BACNET_STATUS_ERROR;
                    }
                    /* initial-timeout[4] Unsigned OPTIONAL */
                    len = bacnet_unsigned_context_decode(
                        &apdu[apdu_len], apdu_size - apdu_len, 4,
                        &unsigned_value);
                    if (len > 0) {
                        apdu_len += len;
                        if (data) {
#if BACNET_EVENT_CHANGE_OF_TIMER_ENABLED
                            data->notificationParams.changeOfTimer
                                .initialTimeout = unsigned_value;
#endif
                        }
                    } else if (len == 0) {
                        /* OPTIONAL - set default */
                        if (data) {
#if BACNET_EVENT_CHANGE_OF_TIMER_ENABLED
                            data->notificationParams.changeOfTimer
                                .initialTimeout = 0;
#endif
                        }
                    } else {
                        return BACNET_STATUS_ERROR;
                    }
                    /* expiration-time[5] BACnetDateTime OPTIONAL */
#if BACNET_EVENT_CHANGE_OF_TIMER_ENABLED
                    if (data) {
                        datetime_value = &data->notificationParams.changeOfTimer
                                              .expirationTime;
                    } else
#endif
                    {
                        datetime_value = NULL;
                    }
                    len = bacnet_datetime_context_decode(
                        &apdu[apdu_len], apdu_size - apdu_len, 5,
                        datetime_value);
                    if (len > 0) {
                        apdu_len += len;
                    } else if (len == 0) {
                        /* OPTIONAL - set default */
                        datetime_wildcard_set(datetime_value);
                    } else {
                        return BACNET_STATUS_ERROR;
                    }
                    break;
                default:
                    return BACNET_STATUS_ERROR;
            }
            if (bacnet_is_closing_tag_number(
                    &apdu[apdu_len], apdu_size - apdu_len, (uint8_t)event_type,
                    &len)) {
                apdu_len += len;
            } else {
                return BACNET_STATUS_ERROR;
            }
        }
        if (bacnet_is_closing_tag_number(
                &apdu[apdu_len], apdu_size - apdu_len, 12, &len)) {
            apdu_len += len;
        } else {
            return BACNET_STATUS_ERROR;
        }
    }

    return apdu_len;
}

/**
 * @brief parse a list of strings for the event notification:
 * @details event-notification string:
 *  process-id initiating-device-id event-object-type event-object-instance
 *  sequence-number notification-class priority message-text notify-type
 *  ack-required from-state to-state event-type
 *  [change-of-bitstring reference-bit-string status-flags]
 *  [change-of-state new-state-tag new-state-value status-flags]
 * @param data [out] BACnetEventNotification data to store parsed data
 * @param argc [in] number of arguments in argv
 * @param argv [in] array of strings to parse
 * @return true if successful, false if error
 */
bool event_notify_parse(
    BACNET_EVENT_NOTIFICATION_DATA *data, int argc, char *argv[])
{
    int argi = 0;
    unsigned int target_args = 1;
    uint32_t found_index = 0;
    unsigned long long_value = 0;
    BACNET_PROPERTY_STATES tag = PROP_STATE_BOOLEAN_VALUE;
    BACNET_BIT_STRING *pBitString = NULL;

    for (argi = 0; argi < argc; argi++) {
        if (target_args == 1) {
            /* process-id */
            if (!bacnet_string_to_uint32(
                    argv[argi], &data->processIdentifier)) {
                return false;
            }
            target_args++;
        } else if (target_args == 2) {
            /* initiating-device-id */
            data->initiatingObjectIdentifier.type = OBJECT_DEVICE;
            if (!bacnet_strtoul(argv[argi], &long_value)) {
                return false;
            }
            if (long_value > BACNET_MAX_INSTANCE) {
                return false;
            }
            data->initiatingObjectIdentifier.instance = (uint32_t)long_value;
            target_args++;
        } else if (target_args == 3) {
            /* event-object-type */
            if (!bactext_object_type_strtol(argv[argi], &found_index)) {
                return false;
            }
            data->eventObjectIdentifier.type = (BACNET_OBJECT_TYPE)found_index;
            target_args++;
        } else if (target_args == 4) {
            /* event-object-instance */
            if (!bacnet_strtoul(argv[argi], &long_value)) {
                return false;
            }
            if (long_value > BACNET_MAX_INSTANCE) {
                return false;
            }
            data->eventObjectIdentifier.instance = (uint32_t)long_value;
            target_args++;
        } else if (target_args == 5) {
            /* sequence-number */
            if (!bacnet_string_to_uint16(
                    argv[argi], &data->timeStamp.value.sequenceNum)) {
                return false;
            }
            data->timeStamp.tag = TIME_STAMP_SEQUENCE;
            target_args++;
        } else if (target_args == 6) {
            /* notification-class */
            if (!bacnet_string_to_uint32(
                    argv[argi], &data->notificationClass)) {
                return false;
            }
            target_args++;
        } else if (target_args == 7) {
            /* priority */
            if (!bacnet_string_to_uint8(argv[argi], &data->priority)) {
                return false;
            }
            target_args++;
        } else if (target_args == 8) {
            /* message-text */
            if (!characterstring_init_ansi(data->messageText, argv[argi])) {
                return false;
            }
            target_args++;
        } else if (target_args == 9) {
            /* notify-type */
            if (!bactext_notify_type_strtol(argv[argi], &found_index)) {
                return false;
            }
            data->notifyType = (BACNET_NOTIFY_TYPE)found_index;
            target_args++;
        } else if (target_args == 10) {
            /* ack-required */
            if (!bacnet_string_to_bool(argv[argi], &data->ackRequired)) {
                return false;
            }
            target_args++;
        } else if (target_args == 11) {
            /* from-state */
            if (!bactext_event_state_strtol(argv[argi], &found_index)) {
                return false;
            }
            data->fromState = (BACNET_EVENT_STATE)found_index;
            target_args++;
        } else if (target_args == 12) {
            /* to-state */
            if (!bactext_event_state_strtol(argv[argi], &found_index)) {
                return false;
            }
            data->toState = (BACNET_EVENT_STATE)found_index;
            target_args++;
        } else if (target_args == 13) {
            /* event-type */
            if (!bactext_event_type_strtol(argv[argi], &found_index)) {
                return false;
            }
            data->eventType = (BACNET_EVENT_TYPE)found_index;
            target_args++;
        } else {
            if (data->eventType == EVENT_CHANGE_OF_BITSTRING) {
                if (target_args == 14) {
                    pBitString = &data->notificationParams.changeOfBitstring
                                      .referencedBitString;
                    bitstring_init_ascii(pBitString, argv[argi]);
                    target_args++;
                } else if (target_args == 15) {
                    pBitString =
                        &data->notificationParams.changeOfBitstring.statusFlags;
                    bitstring_init_ascii(pBitString, argv[argi]);
                    target_args++;
                } else {
                    return false;
                }
            } else if (data->eventType == EVENT_CHANGE_OF_STATE) {
                if (target_args == 14) {
                    if (!bacnet_strtoul(argv[argi], &long_value)) {
                        return false;
                    }
                    tag = (BACNET_PROPERTY_STATES)long_value;
                    data->notificationParams.changeOfState.newState.tag = tag;
                    target_args++;
                } else if (target_args == 15) {
                    if (!bactext_property_states_strtoul(
                            tag, argv[argi], &found_index)) {
                        return false;
                    }
                    if (tag == PROP_STATE_BOOLEAN_VALUE) {
                        if (found_index) {
                            data->notificationParams.changeOfState.newState
                                .state.booleanValue = true;
                        } else {
                            data->notificationParams.changeOfState.newState
                                .state.booleanValue = false;
                        }
                    } else if (tag == PROP_STATE_BINARY_VALUE) {
                        data->notificationParams.changeOfState.newState.state
                            .binaryValue = (BACNET_BINARY_PV)found_index;
                    } else if (tag == PROP_STATE_EVENT_TYPE) {
                        data->notificationParams.changeOfState.newState.state
                            .eventType = (BACNET_EVENT_TYPE)found_index;
                    } else if (tag == PROP_STATE_POLARITY) {
                        data->notificationParams.changeOfState.newState.state
                            .polarity = (BACNET_POLARITY)found_index;
                    } else if (tag == PROP_STATE_PROGRAM_CHANGE) {
                        data->notificationParams.changeOfState.newState.state
                            .programChange =
                            (BACNET_PROGRAM_REQUEST)found_index;
                    } else if (tag == PROP_STATE_PROGRAM_STATE) {
                        data->notificationParams.changeOfState.newState.state
                            .programState = (BACNET_PROGRAM_STATE)found_index;
                    } else if (tag == PROP_STATE_REASON_FOR_HALT) {
                        data->notificationParams.changeOfState.newState.state
                            .programError = (BACNET_PROGRAM_ERROR)found_index;
                    } else if (tag == PROP_STATE_RELIABILITY) {
                        data->notificationParams.changeOfState.newState.state
                            .reliability = (BACNET_RELIABILITY)found_index;
                    } else if (tag == PROP_STATE_EVENT_STATE) {
                        data->notificationParams.changeOfState.newState.state
                            .state = (BACNET_EVENT_STATE)found_index;
                    } else if (tag == PROP_STATE_SYSTEM_STATUS) {
                        data->notificationParams.changeOfState.newState.state
                            .systemStatus = (BACNET_DEVICE_STATUS)found_index;
                    } else if (tag == PROP_STATE_UNITS) {
                        data->notificationParams.changeOfState.newState.state
                            .units = (BACNET_ENGINEERING_UNITS)found_index;
                    } else if (tag == PROP_STATE_UNSIGNED_VALUE) {
                        data->notificationParams.changeOfState.newState.state
                            .unsignedValue =
                            (BACNET_UNSIGNED_INTEGER)found_index;
                    } else if (tag == PROP_STATE_LIFE_SAFETY_MODE) {
                        data->notificationParams.changeOfState.newState.state
                            .lifeSafetyMode =
                            (BACNET_LIFE_SAFETY_MODE)found_index;
                    } else if (tag == PROP_STATE_LIFE_SAFETY_STATE) {
                        data->notificationParams.changeOfState.newState.state
                            .lifeSafetyState =
                            (BACNET_LIFE_SAFETY_STATE)found_index;
                    } else {
                        return false;
                    }
                    target_args++;
                } else if (target_args == 16) {
                    pBitString =
                        &data->notificationParams.changeOfState.statusFlags;
                    bitstring_init_ascii(pBitString, argv[argi]);
                    target_args++;
                } else {
                    return false;
                }
            } else if (data->eventType == EVENT_CHANGE_OF_VALUE) {
                /* FIXME: add event type parameters */
            } else if (data->eventType == EVENT_COMMAND_FAILURE) {
                /* FIXME: add event type parameters */
            } else if (data->eventType == EVENT_FLOATING_LIMIT) {
                /* FIXME: add event type parameters */
            } else if (data->eventType == EVENT_OUT_OF_RANGE) {
                /* FIXME: add event type parameters */
            } else if (data->eventType == EVENT_CHANGE_OF_LIFE_SAFETY) {
                /* FIXME: add event type parameters */
            } else if (data->eventType == EVENT_EXTENDED) {
                /* FIXME: add event type parameters */
            } else if (data->eventType == EVENT_BUFFER_READY) {
                /* FIXME: add event type parameters */
            } else if (data->eventType == EVENT_UNSIGNED_RANGE) {
                /* FIXME: add event type parameters */
            } else if (data->eventType == EVENT_ACCESS_EVENT) {
                /* FIXME: add event type parameters */
            } else if (data->eventType == EVENT_DOUBLE_OUT_OF_RANGE) {
                /* FIXME: add event type parameters */
            } else if (data->eventType == EVENT_SIGNED_OUT_OF_RANGE) {
                /* FIXME: add event type parameters */
            } else if (data->eventType == EVENT_UNSIGNED_OUT_OF_RANGE) {
                /* FIXME: add event type parameters */
            } else if (data->eventType == EVENT_CHANGE_OF_CHARACTERSTRING) {
                /* FIXME: add event type parameters */
            } else if (data->eventType == EVENT_CHANGE_OF_STATUS_FLAGS) {
                /* FIXME: add event type parameters */
            } else if (data->eventType == EVENT_CHANGE_OF_RELIABILITY) {
                /* FIXME: add event type parameters */
            } else if (data->eventType == EVENT_NONE) {
                /* FIXME: add event type parameters */
            } else if (data->eventType == EVENT_CHANGE_OF_DISCRETE_VALUE) {
                /* FIXME: add event type parameters */
            } else if (data->eventType == EVENT_CHANGE_OF_TIMER) {
                /* FIXME: add event type parameters */
            } else if (
                (data->eventType >= EVENT_PROPRIETARY_MIN) &&
                (data->eventType <= EVENT_PROPRIETARY_MAX)) {
                /* Enumerated values 64-65535 may
                    be used by others subject to
                    the procedures and constraints
                    described in Clause 23.  */
            }
        }
    }

    return true;
}
