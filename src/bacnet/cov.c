/**
 * @file
 * @brief BACnet Change-of-Value (COV) services encode and decode functions
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date 2006
 * @copyright SPDX-License-Identifier: GPL-2.0-or-later WITH GCC-exception-2.0
 */
#include <stdint.h>
/* BACnet Stack defines - first */
#include "bacnet/bacdef.h"
/* BACnet Stack API */
#include "bacnet/bacdcode.h"
#include "bacnet/bacapp.h"
/* me! */
#include "bacnet/cov.h"

/* Change-Of-Value Services
COV Subscribe
COV Subscribe Property
COV Notification
Unconfirmed COV Notification
*/

/**
 * @brief Encode APDU for COV Notification.
 * @param apdu  Pointer to the buffer, or NULL for length
 * @param data  Pointer to the data to encode.
 * @return number of bytes encoded, or zero on error.
 */
int cov_notify_encode_apdu(uint8_t *apdu, BACNET_COV_DATA *data)
{
    int len = 0; /* length of each encoding */
    int apdu_len = 0; /* total length of the apdu, return value */
    BACNET_PROPERTY_VALUE *value = NULL; /* value in list */

    if (!data) {
        return 0;
    }
    /* tag 0 - subscriberProcessIdentifier */
    len =
        encode_context_unsigned(apdu, 0, data->subscriberProcessIdentifier);
    apdu_len += len;
    if (apdu) {
        apdu += len;
    }
    /* tag 1 - initiatingDeviceIdentifier */
    len = encode_context_object_id(
        apdu, 1, OBJECT_DEVICE, data->initiatingDeviceIdentifier);
    apdu_len += len;
    if (apdu) {
        apdu += len;
    }
    /* tag 2 - monitoredObjectIdentifier */
    len = encode_context_object_id(apdu, 2,
        data->monitoredObjectIdentifier.type,
        data->monitoredObjectIdentifier.instance);
    apdu_len += len;
    if (apdu) {
        apdu += len;
    }
    /* tag 3 - timeRemaining */
    len = encode_context_unsigned(apdu, 3, data->timeRemaining);
    apdu_len += len;
    if (apdu) {
        apdu += len;
    }
    /* tag 4 - listOfValues */
    len = encode_opening_tag(apdu, 4);
    apdu_len += len;
    if (apdu) {
        apdu += len;
    }
    /* the first value includes a pointer to the next value, etc */
    value = data->listOfValues;
    while (value != NULL) {
        len = bacapp_property_value_encode(apdu, value);
        apdu_len += len;
        if (apdu) {
            apdu += len;
        }
        /* is there another one to encode? */
        value = value->next;
    }
    len = encode_closing_tag(apdu, 4);
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
size_t cov_notify_service_request_encode(
    uint8_t *apdu, size_t apdu_size, BACNET_COV_DATA *data)
{
    size_t apdu_len = 0; /* total length of the apdu, return value */

    apdu_len = cov_notify_encode_apdu(NULL, data);
    if (apdu_len > apdu_size) {
        apdu_len = 0;
    } else {
        apdu_len = cov_notify_encode_apdu(apdu, data);
    }

    return apdu_len;
}

/**
 * Encode APDU for confirmed notification.
 *
 * @param apdu  Pointer to the buffer, or NULL for length
 * @param apdu_size number of bytes available in the buffer
 * @param invoke_id  ID to invoke for notification
 * @param data  Pointer to the data to encode.
 *
 * @return bytes encoded or zero on error.
 */
int ccov_notify_encode_apdu(uint8_t *apdu,
    unsigned apdu_size,
    uint8_t invoke_id,
    BACNET_COV_DATA *data)
{
    int len = 0; /* length of each encoding */
    int apdu_len = 0; /* return value */

    if (apdu && (apdu_size > 4)) {
        apdu[0] = PDU_TYPE_CONFIRMED_SERVICE_REQUEST;
        apdu[1] = encode_max_segs_max_apdu(0, MAX_APDU);
        apdu[2] = invoke_id;
        apdu[3] = SERVICE_CONFIRMED_COV_NOTIFICATION;
    }
    len = 4;
    apdu_len += len;
    if (apdu) {
        apdu += len;
    }
    len = cov_notify_service_request_encode(
        apdu, apdu_size - apdu_len, data);
    if (len > 0) {
        apdu_len += len;
    } else {
        apdu_len = 0;
    }

    return apdu_len;
}

/**
 * @brief Encode APDU for unconfirmed notification.
 * @param apdu  Pointer to the buffer for encoding into, or NULL for length
 * @param apdu_size number of bytes available in the buffer
 * @param data  Pointer to the service data used for encoding values
 * @return number of bytes encoded, or zero if unable to encode or too large
 */
int ucov_notify_encode_apdu(
    uint8_t *apdu, unsigned apdu_size, BACNET_COV_DATA *data)
{
    int len = 0; /* length of each encoding */
    int apdu_len = 0; /* return value */

    if (apdu && (apdu_size > 2)) {
        apdu[0] = PDU_TYPE_UNCONFIRMED_SERVICE_REQUEST;
        apdu[1] = SERVICE_UNCONFIRMED_COV_NOTIFICATION; 
    } 
    len = 2;
    apdu_len += len;
    if (apdu) {
        apdu += len;
    }
    len = cov_notify_service_request_encode(
        apdu, apdu_size - apdu_len, data);
    if (len > 0) {
        apdu_len += len;
    } else {
        apdu_len = 0;
    }

    return apdu_len;
}

/**
 * @brief Decode the COV-service request only.
 *
 * ConfirmedCOVNotification-Request ::= SEQUENCE {
 *      subscriber-process-identifier [0] Unsigned32,
 *      initiating-device-identifier [1] BACnetObjectIdentifier,
 *      monitored-object-identifier [2] BACnetObjectIdentifier,
 *      time-remaining [3] Unsigned,
 *      list-of-values [4] SEQUENCE OF BACnetPropertyValue
 *  }
 *
 * @note: COV and Unconfirmed COV are the same.
 * @param apdu  Pointer to the buffer.
 * @param apdu_size  Number of valid bytes in the buffer.
 * @param data  Pointer to the data to store the decoded values, or NULL
 *
 * @return Bytes decoded or BACNET_STATUS_ERROR on error.
 */
int cov_notify_decode_service_request(
    uint8_t *apdu, unsigned apdu_size, BACNET_COV_DATA *data)
{
    int len = 0; /* return value */
    int value_len = 0, tag_len = 0;
    BACNET_UNSIGNED_INTEGER decoded_value = 0;
    BACNET_OBJECT_TYPE decoded_type = OBJECT_NONE;
    uint32_t decoded_instance = 0;
    BACNET_PROPERTY_ID property_identifier = PROP_ALL;
    BACNET_PROPERTY_VALUE *value = NULL;

    /* subscriber-process-identifier [0] Unsigned32 */
    value_len = bacnet_unsigned_context_decode(
        &apdu[len], apdu_size - len, 0, &decoded_value);
    if (value_len > 0) {
        if (data) {
            data->subscriberProcessIdentifier = decoded_value;
        }
        len += value_len;
    } else {
        return BACNET_STATUS_ERROR;
    }
    /* initiating-device-identifier [1] BACnetObjectIdentifier */
    value_len = bacnet_object_id_context_decode(
        &apdu[len], apdu_size - len, 1, &decoded_type, &decoded_instance);
    if (value_len > 0) {
        if (decoded_type != OBJECT_DEVICE) {
            return BACNET_STATUS_ERROR;
        }
        if (data) {
            data->initiatingDeviceIdentifier = decoded_instance;
        }
        len += value_len;
    } else {
        return BACNET_STATUS_ERROR;
    }
    /* monitored-object-identifier [2] BACnetObjectIdentifier */
    value_len = bacnet_object_id_context_decode(
        &apdu[len], apdu_size - len, 2, &decoded_type, &decoded_instance);
    if (value_len > 0) {
        if (data) {
            data->monitoredObjectIdentifier.type = decoded_type;
            data->monitoredObjectIdentifier.instance = decoded_instance;
        }
        len += value_len;
    } else {
        return BACNET_STATUS_ERROR;
    }
    /* time-remaining [3] Unsigned */
    value_len = bacnet_unsigned_context_decode(
        &apdu[len], apdu_size - len, 3, &decoded_value);
    if (value_len > 0) {
        if (data) {
            data->timeRemaining = decoded_value;
        }
        len += value_len;
    } else {
        return BACNET_STATUS_ERROR;
    }
    /* list-of-values [4] SEQUENCE OF BACnetPropertyValue */
    if (bacnet_is_opening_tag_number(
            &apdu[len], apdu_size - len, 4, &tag_len)) {
        if (data) {
            len += tag_len;
            /* the first value includes a pointer to the next value, etc */
            value = data->listOfValues;
            while (value != NULL) {
                value_len = bacapp_property_value_decode(
                    &apdu[len], apdu_size - len, value);
                if (value_len == BACNET_STATUS_ERROR) {
                    return BACNET_STATUS_ERROR;
                } else {
                    len += value_len;
                }
                /* end of list? */
                if (bacnet_is_closing_tag_number(
                        &apdu[len], apdu_size - len, 4, &tag_len)) {
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
        } else {
            /* this len function needs to start at the opening tag
               to match opening/closing tags like a stack.
               However, it returns the len between the tags. */
            value_len = bacapp_data_len(&apdu[len], apdu_size - len,
                (BACNET_PROPERTY_ID)property_identifier);
            len += value_len;
            /* add the opening tag length to the totals */
            len += tag_len;
        }
    }

    return len;
}

/*
12.11.38Active_COV_Subscriptions
The Active_COV_Subscriptions property is a List of BACnetCOVSubscription,
each of which consists of a Recipient, a Monitored Property Reference,
an Issue Confirmed Notifications flag, a Time Remaining value and an
optional COV Increment. This property provides a network-visible indication
of those COV subscriptions that are active at any given time.
Whenever a COV Subscription is created with the SubscribeCOV or
SubscribeCOVProperty service, a new entry is added to
the Active_COV_Subscriptions list. Similarly, whenever a COV Subscription
is terminated, the corresponding entry shall be
removed from the Active_COV_Subscriptions list.
*/
/*
SubscribeCOV-Request ::= SEQUENCE {
        subscriberProcessIdentifier  [0] Unsigned32,
        monitoredObjectIdentifier    [1] BACnetObjectIdentifier,
        issueConfirmedNotifications  [2] BOOLEAN OPTIONAL,
        lifetime                     [3] Unsigned OPTIONAL
        }
*/

/**
 * @brief Encode APDU for SubscribeCOV-Request
 * @note COV and Unconfirmed COV are the same encodings
 * @param apdu  Pointer to the buffer, or NULL for length
 * @param data  Pointer to the data to encode.
 * @return number of bytes encoded, or zero on error.
 */
int cov_subscribe_apdu_encode(uint8_t *apdu, BACNET_SUBSCRIBE_COV_DATA *data)
{
    int len = 0; /* length of each encoding */
    int apdu_len = 0; /* total length of the apdu, return value */

    /* tag 0 - subscriberProcessIdentifier */
    len = encode_context_unsigned(apdu, 0, data->subscriberProcessIdentifier);
    apdu_len += len;
    if (apdu) {
        apdu += len;
    }

    /* tag 1 - monitoredObjectIdentifier */
    len =
        encode_context_object_id(apdu, 1, data->monitoredObjectIdentifier.type,
            data->monitoredObjectIdentifier.instance);
    apdu_len += len;
    if (apdu) {
        apdu += len;
    }
    /*
        If both the 'Issue Confirmed Notifications' and
        'Lifetime' parameters are absent, then this shall
        indicate a cancellation request.
        */
    if (!data->cancellationRequest) {
        /* tag 2 - issueConfirmedNotifications */
        len =
            encode_context_boolean(apdu, 2, data->issueConfirmedNotifications);
        apdu_len += len;
        if (apdu) {
            apdu += len;
        }
        /* tag 3 - lifetime */
        len = encode_context_unsigned(apdu, 3, data->lifetime);
        apdu_len += len;
    }

    return apdu_len;
}

/**
 * @brief Encode the SubscribeCOV service request
 * @note COV and Unconfirmed COV are the same encodings
 * @param apdu  Pointer to the buffer for encoding into
 * @param apdu_size number of bytes available in the buffer
 * @param data  Pointer to the service data used for encoding values
 * @return number of bytes encoded, or zero if unable to encode or too large
 */
size_t cov_subscribe_service_request_encode(
    uint8_t *apdu, size_t apdu_size, BACNET_SUBSCRIBE_COV_DATA *data)
{
    size_t apdu_len = 0; /* total length of the apdu, return value */

    apdu_len = cov_subscribe_apdu_encode(NULL, data);
    if (apdu_len > apdu_size) {
        apdu_len = 0;
    } else {
        apdu_len = cov_subscribe_apdu_encode(apdu, data);
    }

    return apdu_len;
}

/**
 * @brief Encode the COV-service request.
 * @note COV and Unconfirmed COV are the same encodings
 * @param apdu  Pointer to the buffer.
 * @param apdu_size number of bytes available in the buffer
 * @param invoke_id  Invoke ID
 * @param data  Pointer to the data to store the decoded values.
 * @return number of bytes encoded, or zero if unable to encode or too large
 */
int cov_subscribe_encode_apdu(uint8_t *apdu,
    unsigned apdu_size,
    uint8_t invoke_id,
    BACNET_SUBSCRIBE_COV_DATA *data)
{
    int len = 0; /* length of each encoding */
    int apdu_len = 0; /* total length of the apdu, return value */

    if (apdu && (apdu_size > 4)) {
        apdu[0] = PDU_TYPE_CONFIRMED_SERVICE_REQUEST;
        apdu[1] = encode_max_segs_max_apdu(0, MAX_APDU);
        apdu[2] = invoke_id;
        apdu[3] = SERVICE_CONFIRMED_SUBSCRIBE_COV;
    }
    len = 4;
    apdu_len += len;
    if (apdu) {
        apdu += len;
    }
    len =
        cov_subscribe_service_request_encode(apdu, apdu_size - apdu_len, data);
    if (len > 0) {
        apdu_len += len;
    } else {
        apdu_len = 0;
    }

    return apdu_len;
}

/**
 * @brief Decode the subscribe-service request only.
 *
 *  SubscribeCOV-Request ::= SEQUENCE {
 *      subscriberProcessIdentifier  [0] Unsigned32,
 *      monitoredObjectIdentifier    [1] BACnetObjectIdentifier,
 *      issueConfirmedNotifications  [2] BOOLEAN OPTIONAL,
 *      lifetime                     [3] Unsigned OPTIONAL
 *  }
 *
 * @param apdu  Pointer to the buffer.
 * @param apdu_size  number of valid bytes in the buffer.
 * @param data  Pointer to the data to store the decoded values.
 *
 * @return Bytes decoded or Zero/BACNET_STATUS_ERROR on error.
 */
int cov_subscribe_decode_service_request(
    uint8_t *apdu, unsigned apdu_size, BACNET_SUBSCRIBE_COV_DATA *data)
{
    int len = 0; /* return value */
    int value_len = 0;
    BACNET_UNSIGNED_INTEGER decoded_value = 0;
    BACNET_OBJECT_TYPE decoded_type = OBJECT_NONE;
    uint32_t decoded_instance = 0;
    bool decoded_boolean = false;

    /* subscriberProcessIdentifier [0] Unsigned32 */
    value_len = bacnet_unsigned_context_decode(
        &apdu[len], apdu_size - len, 0, &decoded_value);
    if (value_len > 0) {
        if (data) {
            data->subscriberProcessIdentifier = decoded_value;
        }
        len += value_len;
    } else {
        if (data) {
            data->error_code = ERROR_CODE_REJECT_INVALID_TAG;
        }
        return BACNET_STATUS_ERROR;
    }
    /* monitoredObjectIdentifier [1] BACnetObjectIdentifier */
    value_len = bacnet_object_id_context_decode(
        &apdu[len], apdu_size - len, 1, &decoded_type, &decoded_instance);
    if (value_len > 0) {
        if (data) {
            data->monitoredObjectIdentifier.type = decoded_type;
            data->monitoredObjectIdentifier.instance = decoded_instance;
        }
        len += value_len;
    } else {
        if (data) {
            data->error_code = ERROR_CODE_REJECT_INVALID_TAG;
        }
        return BACNET_STATUS_ERROR;
    }
    if ((unsigned)len < apdu_size) {
        if (data) {
            /* does not indicate a cancellation request */
            data->cancellationRequest = false;
        }
        /* issueConfirmedNotifications [2] BOOLEAN OPTIONAL */
        value_len = bacnet_boolean_context_decode(
            &apdu[len], apdu_size - len, 2, &decoded_boolean);
        if (value_len > 0) {
            if (data) {
                data->issueConfirmedNotifications = decoded_boolean;
            }
            len += value_len;
        } else if (value_len == 0) {
            /* invalid tag */
            if (data) {
                data->issueConfirmedNotifications = false;
            }
        } else {
            if (data) {
                data->error_code = ERROR_CODE_REJECT_INVALID_TAG;
            }
            return BACNET_STATUS_ERROR;
        }
    } else {
        /* If both the 'Issue Confirmed Notifications' and
           'Lifetime' parameters are absent, then this shall
           indicate a cancellation request. */
        if (data) {
            data->cancellationRequest = true;
        }
    }
    if ((unsigned)len < apdu_size) {
        /* lifetime [3] Unsigned OPTIONAL */
        value_len = bacnet_unsigned_context_decode(
            &apdu[len], apdu_size - len, 3, &decoded_value);
        if (value_len > 0) {
            if (data) {
                data->lifetime = decoded_value;
            }
            len += value_len;
        } else {
            if (data) {
                data->error_code = ERROR_CODE_REJECT_INVALID_TAG;
            }
            return BACNET_STATUS_ERROR;
        }
    } else {
        if (data) {
            data->lifetime = 0;
        }
    }

    return len;
}

/*
SubscribeCOVProperty-Request ::= SEQUENCE {
        subscriberProcessIdentifier  [0] Unsigned32,
        monitoredObjectIdentifier    [1] BACnetObjectIdentifier,
        issueConfirmedNotifications  [2] BOOLEAN OPTIONAL,
        lifetime                     [3] Unsigned OPTIONAL,
        monitoredPropertyIdentifier  [4] BACnetPropertyReference,
        covIncrement                 [5] REAL OPTIONAL
        }

BACnetPropertyReference ::= SEQUENCE {
      propertyIdentifier      [0] BACnetPropertyIdentifier,
      propertyArrayIndex      [1] Unsigned OPTIONAL
      -- used only with array datatype
      -- if omitted with an array the entire array is referenced
      }

*/

/**
 * @brief Encode APDU for SubscribeCOVProperty request
 * @param apdu  Pointer to the buffer, or NULL for length
 * @param data  Pointer to the data to encode.
 * @return bytes encoded or zero on error.
 */
int cov_subscribe_property_apdu_encode(
    uint8_t *apdu, BACNET_SUBSCRIBE_COV_DATA *data)
{
    int len = 0; /* length of each encoding */
    int apdu_len = 0; /* total length of the apdu, return value */

    if (!data) {
        return 0;
    }
    /* tag 0 - subscriberProcessIdentifier */
    len = encode_context_unsigned(apdu, 0, data->subscriberProcessIdentifier);
    apdu_len += len;
    if (apdu) {
        apdu += len;
    }
    /* tag 1 - monitoredObjectIdentifier */
    len =
        encode_context_object_id(apdu, 1, data->monitoredObjectIdentifier.type,
            data->monitoredObjectIdentifier.instance);
    apdu_len += len;
    if (apdu) {
        apdu += len;
    }
    if (!data->cancellationRequest) {
        /* tag 2 - issueConfirmedNotifications */
        len =
            encode_context_boolean(apdu, 2, data->issueConfirmedNotifications);
        apdu_len += len;
        if (apdu) {
            apdu += len;
        }
        /* tag 3 - lifetime */
        len = encode_context_unsigned(apdu, 3, data->lifetime);
        apdu_len += len;
        if (apdu) {
            apdu += len;
        }
    }
    /* tag 4 - monitoredPropertyIdentifier */
    len = encode_opening_tag(apdu, 4);
    apdu_len += len;
    if (apdu) {
        apdu += len;
    }
    len = encode_context_enumerated(
        apdu, 0, data->monitoredProperty.propertyIdentifier);
    apdu_len += len;
    if (apdu) {
        apdu += len;
    }
    if (data->monitoredProperty.propertyArrayIndex != BACNET_ARRAY_ALL) {
        len = encode_context_unsigned(
            apdu, 1, data->monitoredProperty.propertyArrayIndex);
        apdu_len += len;
        if (apdu) {
            apdu += len;
        }
    }
    len = encode_closing_tag(apdu, 4);
    apdu_len += len;
    if (apdu) {
        apdu += len;
    }
    /* tag 5 - covIncrement */
    if (data->covIncrementPresent) {
        len = encode_context_real(apdu, 5, data->covIncrement);
        apdu_len += len;
    }

    return apdu_len;
}

/**
 * @brief Encode the SubscribeCOVProperty service request
 * @param apdu  Pointer to the buffer for encoding into
 * @param apdu_size number of bytes available in the buffer
 * @param data  Pointer to the service data used for encoding values
 * @return number of bytes encoded, or zero if unable to encode or too large
 */
size_t cov_subscribe_property_service_request_encode(
    uint8_t *apdu, size_t apdu_size, BACNET_SUBSCRIBE_COV_DATA *data)
{
    size_t apdu_len = 0; /* total length of the apdu, return value */

    apdu_len = cov_subscribe_property_apdu_encode(NULL, data);
    if (apdu_len > apdu_size) {
        apdu_len = 0;
    } else {
        apdu_len = cov_subscribe_property_apdu_encode(apdu, data);
    }

    return apdu_len;
}

/**
 * @brief Encode SubscribeCOVProperty request
 * @param apdu  Pointer to the buffer.
 * @param apdu_size number of bytes available in the buffer
 * @param invoke_id  Invoke Id.
 * @param data  Pointer to the data to encode.
 * @return number of bytes encoded, or zero on error.
 */
int cov_subscribe_property_encode_apdu(uint8_t *apdu,
    unsigned apdu_size,
    uint8_t invoke_id,
    BACNET_SUBSCRIBE_COV_DATA *data)
{
    int len = 0; /* length of each encoding */
    int apdu_len = 0; /* total length of the apdu, return value */

    if (!data) {
        return 0;
    }
    if (apdu && (apdu_size > 4)) {
        apdu[0] = PDU_TYPE_CONFIRMED_SERVICE_REQUEST;
        apdu[1] = encode_max_segs_max_apdu(0, MAX_APDU);
        apdu[2] = invoke_id;
        apdu[3] = SERVICE_CONFIRMED_SUBSCRIBE_COV_PROPERTY;
    }
    len = 4;
    apdu_len += len;
    if (apdu) {
        apdu += len;
    }
    len = cov_subscribe_property_service_request_encode(
        apdu, apdu_size - apdu_len, data);
    if (len > 0) {
        apdu_len += len;
    } else {
        apdu_len = 0;
    }

    return apdu_len;
}

/**
 * Decode the COV-service property request only.
 *
 * @param apdu  Pointer to the buffer.
 * @param apdu_len  Count of valid bytes in the buffer.
 * @param data  Pointer to the data to store the decoded values.
 *
 * @return Bytes decoded or Zero/BACNET_STATUS_ERROR on error.
 */
int cov_subscribe_property_decode_service_request(
    uint8_t *apdu, unsigned apdu_len, BACNET_SUBSCRIBE_COV_DATA *data)
{
    int len = 0; /* return value */
    uint8_t tag_number = 0;
    uint32_t len_value = 0;
    BACNET_UNSIGNED_INTEGER decoded_value = 0; /* for decoding */
    BACNET_OBJECT_TYPE decoded_type = OBJECT_NONE; /* for decoding */
    uint32_t property = 0; /* for decoding */

    if ((apdu_len > 2) && data) {
        /* tag 0 - subscriberProcessIdentifier */
        if (decode_is_context_tag(&apdu[len], 0)) {
            len += decode_tag_number_and_value(
                &apdu[len], &tag_number, &len_value);
            len += decode_unsigned(&apdu[len], len_value, &decoded_value);
            data->subscriberProcessIdentifier = decoded_value;
        } else {
            data->error_code = ERROR_CODE_REJECT_INVALID_TAG;
            return BACNET_STATUS_REJECT;
        }
        /* tag 1 - monitoredObjectIdentifier */
        if (len >= (int)apdu_len) {
            return BACNET_STATUS_REJECT;
        }
        if (decode_is_context_tag(&apdu[len], 1)) {
            len += decode_tag_number_and_value(
                &apdu[len], &tag_number, &len_value);
            len += decode_object_id(&apdu[len], &decoded_type,
                &data->monitoredObjectIdentifier.instance);
            data->monitoredObjectIdentifier.type = decoded_type;
        } else {
            data->error_code = ERROR_CODE_REJECT_INVALID_TAG;
            return BACNET_STATUS_REJECT;
        }
        /* tag 2 - issueConfirmedNotifications - optional */
        if (len >= (int)apdu_len) {
            return BACNET_STATUS_REJECT;
        }
        if (decode_is_context_tag(&apdu[len], 2)) {
            data->cancellationRequest = false;
            len += decode_tag_number_and_value(
                &apdu[len], &tag_number, &len_value);
            data->issueConfirmedNotifications =
                decode_context_boolean(&apdu[len]);
            len++;
        } else {
            data->cancellationRequest = true;
        }
        /* tag 3 - lifetime - optional */
        if (len >= (int)apdu_len) {
            return BACNET_STATUS_REJECT;
        }
        if (decode_is_context_tag(&apdu[len], 3)) {
            len += decode_tag_number_and_value(
                &apdu[len], &tag_number, &len_value);
            len += decode_unsigned(&apdu[len], len_value, &decoded_value);
            data->lifetime = decoded_value;
        } else {
            data->lifetime = 0;
        }
        /* tag 4 - monitoredPropertyIdentifier */
        if (len >= (int)apdu_len) {
            return BACNET_STATUS_REJECT;
        }
        if (!decode_is_opening_tag_number(&apdu[len], 4)) {
            data->error_code = ERROR_CODE_REJECT_INVALID_TAG;
            return BACNET_STATUS_REJECT;
        }
        /* a tag number of 4 is not extended so only one octet */
        len++;
        /* the propertyIdentifier is tag 0 */
        if (len >= (int)apdu_len) {
            return BACNET_STATUS_REJECT;
        }
        if (decode_is_context_tag(&apdu[len], 0)) {
            len += decode_tag_number_and_value(
                &apdu[len], &tag_number, &len_value);
            len += decode_enumerated(&apdu[len], len_value, &property);
            data->monitoredProperty.propertyIdentifier =
                (BACNET_PROPERTY_ID)property;
        } else {
            data->error_code = ERROR_CODE_REJECT_INVALID_TAG;
            return BACNET_STATUS_REJECT;
        }
        /* the optional array index is tag 1 */
        if (len >= (int)apdu_len) {
            return BACNET_STATUS_REJECT;
        }
        if (decode_is_context_tag(&apdu[len], 1)) {
            len += decode_tag_number_and_value(
                &apdu[len], &tag_number, &len_value);
            len += decode_unsigned(&apdu[len], len_value, &decoded_value);
            data->monitoredProperty.propertyArrayIndex = decoded_value;
        } else {
            data->monitoredProperty.propertyArrayIndex = BACNET_ARRAY_ALL;
        }

        if (!decode_is_closing_tag_number(&apdu[len], 4)) {
            data->error_code = ERROR_CODE_REJECT_INVALID_TAG;
            return BACNET_STATUS_REJECT;
        }
        /* a tag number of 4 is not extended so only one octet */
        len++;
        /* tag 5 - covIncrement - optional */
        if (len < (int)apdu_len) {
            if (decode_is_context_tag(&apdu[len], 5)) {
                data->covIncrementPresent = true;
                len += decode_tag_number_and_value(
                    &apdu[len], &tag_number, &len_value);
                len += decode_real(&apdu[len], &data->covIncrement);
            } else {
                data->covIncrementPresent = false;
            }
        } else {
            data->covIncrementPresent = false;
        }
    }

    return len;
}

/**
 * @brief Link an array or buffer of BACNET_PROPERTY_VALUE elements
 * @param value_list - One or more BACNET_PROPERTY_VALUE elements in
 * a buffer or array.
 * @param count - number of BACNET_PROPERTY_VALUE elements
 */
void cov_property_value_list_link(
    BACNET_PROPERTY_VALUE *value_list, size_t count)
{
    BACNET_PROPERTY_VALUE *current_value_list = NULL;

    if (value_list) {
        while (count) {
            if (count > 1) {
                current_value_list = value_list;
                value_list++;
                current_value_list->next = value_list;
            } else {
                value_list->next = NULL;
            }
            count--;
        }
    }
}

/** Link an array or buffer of BACNET_PROPERTY_VALUE elements and add them
 * to the BACNET_COV_DATA structure.  It is used prior to encoding or
 * decoding the APDU data into the structure.
 *
 * @param data - The BACNET_COV_DATA structure that holds the data to
 * be encoded or decoded.
 * @param value_list - One or more BACNET_PROPERTY_VALUE elements in
 * a buffer or array.
 * @param count - number of BACNET_PROPERTY_VALUE elements
 */
void cov_data_value_list_link(
    BACNET_COV_DATA *data, BACNET_PROPERTY_VALUE *value_list, size_t count)
{
    if (data && value_list) {
        data->listOfValues = value_list;
        cov_property_value_list_link(value_list, count);
    }
}

/**
 * @brief Encode the Value List for REAL Present-Value and Status-Flags
 * @param value_list - #BACNET_PROPERTY_VALUE with at least 2 entries
 * @param value - REAL present-value
 * @param in_alarm - value of in-alarm status-flags
 * @param fault - value of fault status-flags
 * @param overridden - value of overridden status-flags
 * @param out_of_service - value of out-of-service status-flags
 *
 * @return true if values were encoded
 */
bool cov_value_list_encode_real(BACNET_PROPERTY_VALUE *value_list,
    float value,
    bool in_alarm,
    bool fault,
    bool overridden,
    bool out_of_service)
{
    bool status = false;

    if (value_list) {
        value_list->propertyIdentifier = PROP_PRESENT_VALUE;
        value_list->propertyArrayIndex = BACNET_ARRAY_ALL;
        value_list->value.context_specific = false;
        value_list->value.tag = BACNET_APPLICATION_TAG_REAL;
        value_list->value.type.Real = value;
        value_list->value.next = NULL;
        value_list->priority = BACNET_NO_PRIORITY;
        value_list = value_list->next;
    }
    if (value_list) {
        value_list->propertyIdentifier = PROP_STATUS_FLAGS;
        value_list->propertyArrayIndex = BACNET_ARRAY_ALL;
        value_list->value.context_specific = false;
        value_list->value.tag = BACNET_APPLICATION_TAG_BIT_STRING;
        bitstring_init(&value_list->value.type.Bit_String);
        bitstring_set_bit(
            &value_list->value.type.Bit_String, STATUS_FLAG_IN_ALARM, in_alarm);
        bitstring_set_bit(
            &value_list->value.type.Bit_String, STATUS_FLAG_FAULT, fault);
        bitstring_set_bit(&value_list->value.type.Bit_String,
            STATUS_FLAG_OVERRIDDEN, overridden);
        bitstring_set_bit(&value_list->value.type.Bit_String,
            STATUS_FLAG_OUT_OF_SERVICE, out_of_service);
        value_list->value.next = NULL;
        value_list->priority = BACNET_NO_PRIORITY;
        value_list->next = NULL;
        status = true;
    }

    return status;
}

/**
 * @brief Encode the Value List for ENUMERATED Present-Value and Status-Flags
 * @param value_list - #BACNET_PROPERTY_VALUE with at least 2 entries
 * @param value - ENUMERATED present-value
 * @param in_alarm - value of in-alarm status-flags
 * @param fault - value of in-alarm status-flags
 * @param overridden - value of overridden status-flags
 * @param out_of_service - value of out-of-service status-flags
 *
 * @return true if values were encoded
 */
bool cov_value_list_encode_enumerated(BACNET_PROPERTY_VALUE *value_list,
    uint32_t value,
    bool in_alarm,
    bool fault,
    bool overridden,
    bool out_of_service)
{
    bool status = false;

    if (value_list) {
        value_list->propertyIdentifier = PROP_PRESENT_VALUE;
        value_list->propertyArrayIndex = BACNET_ARRAY_ALL;
        value_list->value.context_specific = false;
        value_list->value.tag = BACNET_APPLICATION_TAG_ENUMERATED;
        value_list->value.type.Enumerated = value;
        value_list->value.next = NULL;
        value_list->priority = BACNET_NO_PRIORITY;
        value_list = value_list->next;
    }
    if (value_list) {
        value_list->propertyIdentifier = PROP_STATUS_FLAGS;
        value_list->propertyArrayIndex = BACNET_ARRAY_ALL;
        value_list->value.context_specific = false;
        value_list->value.tag = BACNET_APPLICATION_TAG_BIT_STRING;
        bitstring_init(&value_list->value.type.Bit_String);
        bitstring_set_bit(
            &value_list->value.type.Bit_String, STATUS_FLAG_IN_ALARM, in_alarm);
        bitstring_set_bit(
            &value_list->value.type.Bit_String, STATUS_FLAG_FAULT, fault);
        bitstring_set_bit(&value_list->value.type.Bit_String,
            STATUS_FLAG_OVERRIDDEN, overridden);
        bitstring_set_bit(&value_list->value.type.Bit_String,
            STATUS_FLAG_OUT_OF_SERVICE, out_of_service);
        value_list->value.next = NULL;
        value_list->priority = BACNET_NO_PRIORITY;
        value_list->next = NULL;
        status = true;
    }

    return status;
}

/**
 * @brief Encode the Value List for UNSIGNED INT Present-Value and Status-Flags
 * @param value_list - #BACNET_PROPERTY_VALUE with at least 2 entries
 * @param value - UNSIGNED INT present-value
 * @param in_alarm - value of in-alarm status-flags
 * @param fault - value of in-alarm status-flags
 * @param overridden - value of overridden status-flags
 * @param out_of_service - value of out-of-service status-flags
 *
 * @return true if values were encoded
 */
bool cov_value_list_encode_unsigned(BACNET_PROPERTY_VALUE *value_list,
    uint32_t value,
    bool in_alarm,
    bool fault,
    bool overridden,
    bool out_of_service)
{
    bool status = false;

    if (value_list) {
        value_list->propertyIdentifier = PROP_PRESENT_VALUE;
        value_list->propertyArrayIndex = BACNET_ARRAY_ALL;
        value_list->value.context_specific = false;
        value_list->value.tag = BACNET_APPLICATION_TAG_UNSIGNED_INT;
        value_list->value.type.Unsigned_Int = value;
        value_list->value.next = NULL;
        value_list->priority = BACNET_NO_PRIORITY;
        value_list = value_list->next;
    }
    if (value_list) {
        value_list->propertyIdentifier = PROP_STATUS_FLAGS;
        value_list->propertyArrayIndex = BACNET_ARRAY_ALL;
        value_list->value.context_specific = false;
        value_list->value.tag = BACNET_APPLICATION_TAG_BIT_STRING;
        bitstring_init(&value_list->value.type.Bit_String);
        bitstring_set_bit(
            &value_list->value.type.Bit_String, STATUS_FLAG_IN_ALARM, in_alarm);
        bitstring_set_bit(
            &value_list->value.type.Bit_String, STATUS_FLAG_FAULT, fault);
        bitstring_set_bit(&value_list->value.type.Bit_String,
            STATUS_FLAG_OVERRIDDEN, overridden);
        bitstring_set_bit(&value_list->value.type.Bit_String,
            STATUS_FLAG_OUT_OF_SERVICE, out_of_service);
        value_list->value.next = NULL;
        value_list->priority = BACNET_NO_PRIORITY;
        value_list->next = NULL;
        status = true;
    }

    return status;
}

#if defined(BACAPP_CHARACTER_STRING)
/**
 * @brief Encode the Value List for CHARACTER_STRING Present-Value and
 * Status-Flags
 * @param value_list - #BACNET_PROPERTY_VALUE with at least 2 entries
 * @param value - CHARACTER_STRING present-value
 * @param in_alarm - value of in-alarm status-flags
 * @param fault - value of in-alarm status-flags
 * @param overridden - value of overridden status-flags
 * @param out_of_service - value of out-of-service status-flags
 *
 * @return true if values were encoded
 */
bool cov_value_list_encode_character_string(BACNET_PROPERTY_VALUE *value_list,
    BACNET_CHARACTER_STRING *value,
    bool in_alarm,
    bool fault,
    bool overridden,
    bool out_of_service)
{
    bool status = false;

    if (value_list) {
        value_list->propertyIdentifier = PROP_PRESENT_VALUE;
        value_list->propertyArrayIndex = BACNET_ARRAY_ALL;
        value_list->value.context_specific = false;
        value_list->value.tag = BACNET_APPLICATION_TAG_CHARACTER_STRING;
        characterstring_copy(&value_list->value.type.Character_String, value);
        value_list->value.next = NULL;
        value_list->priority = BACNET_NO_PRIORITY;
        value_list = value_list->next;
    }
    if (value_list) {
        value_list->propertyIdentifier = PROP_STATUS_FLAGS;
        value_list->propertyArrayIndex = BACNET_ARRAY_ALL;
        value_list->value.context_specific = false;
        value_list->value.tag = BACNET_APPLICATION_TAG_BIT_STRING;
        bitstring_init(&value_list->value.type.Bit_String);
        bitstring_set_bit(
            &value_list->value.type.Bit_String, STATUS_FLAG_IN_ALARM, in_alarm);
        bitstring_set_bit(
            &value_list->value.type.Bit_String, STATUS_FLAG_FAULT, fault);
        bitstring_set_bit(&value_list->value.type.Bit_String,
            STATUS_FLAG_OVERRIDDEN, overridden);
        bitstring_set_bit(&value_list->value.type.Bit_String,
            STATUS_FLAG_OUT_OF_SERVICE, out_of_service);
        value_list->value.next = NULL;
        value_list->priority = BACNET_NO_PRIORITY;
        value_list->next = NULL;
        status = true;
    }

    return status;
}
#endif

#if defined(BACAPP_BIT_STRING)
/**
 * @brief Encode the Value List for CHARACTER_STRING Present-Value and
 * Status-Flags
 * @param value_list - #BACNET_PROPERTY_VALUE with at least 2 entries
 * @param value - CHARACTER_STRING present-value
 * @param in_alarm - value of in-alarm status-flags
 * @param fault - value of in-alarm status-flags
 * @param overridden - value of overridden status-flags
 * @param out_of_service - value of out-of-service status-flags
 *
 * @return true if values were encoded
 */
bool cov_value_list_encode_bit_string(BACNET_PROPERTY_VALUE *value_list,
    BACNET_BIT_STRING *value,
    bool in_alarm,
    bool fault,
    bool overridden,
    bool out_of_service)
{
    bool status = false;

    if (value_list) {
        value_list->propertyIdentifier = PROP_PRESENT_VALUE;
        value_list->propertyArrayIndex = BACNET_ARRAY_ALL;
        value_list->value.context_specific = false;
        value_list->value.tag = BACNET_APPLICATION_TAG_BIT_STRING;
        bitstring_copy(&value_list->value.type.Bit_String, value);
        value_list->value.next = NULL;
        value_list->priority = BACNET_NO_PRIORITY;
        value_list = value_list->next;
    }
    if (value_list) {
        value_list->propertyIdentifier = PROP_STATUS_FLAGS;
        value_list->propertyArrayIndex = BACNET_ARRAY_ALL;
        value_list->value.context_specific = false;
        value_list->value.tag = BACNET_APPLICATION_TAG_BIT_STRING;
        bitstring_init(&value_list->value.type.Bit_String);
        bitstring_set_bit(
            &value_list->value.type.Bit_String, STATUS_FLAG_IN_ALARM, in_alarm);
        bitstring_set_bit(
            &value_list->value.type.Bit_String, STATUS_FLAG_FAULT, fault);
        bitstring_set_bit(&value_list->value.type.Bit_String,
            STATUS_FLAG_OVERRIDDEN, overridden);
        bitstring_set_bit(&value_list->value.type.Bit_String,
            STATUS_FLAG_OUT_OF_SERVICE, out_of_service);
        value_list->value.next = NULL;
        value_list->priority = BACNET_NO_PRIORITY;
        value_list->next = NULL;
        status = true;
    }

    return status;
}
#endif
