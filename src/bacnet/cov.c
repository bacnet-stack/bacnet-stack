/*####COPYRIGHTBEGIN####
 -------------------------------------------
 Copyright (C) 2006 Steve Karg

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
#include "bacnet/bacapp.h"
#include "bacnet/memcopy.h"
/* me! */
#include "bacnet/cov.h"

/** @file cov.c  Encode/Decode Change of Value (COV) services */

/* Change-Of-Value Services
COV Subscribe
COV Subscribe Property
COV Notification
Unconfirmed COV Notification
*/

/**
 * Encode APDU for notification.
 *
 * @param apdu  Pointer to the buffer.
 * @param data  Pointer to the data to encode.
 *
 * @return bytes encoded or zero on error.
 */
static int notify_encode_apdu(
    uint8_t *apdu, unsigned max_apdu_len, BACNET_COV_DATA *data)
{
    int len = 0; /* length of each encoding */
    int apdu_len = 0; /* total length of the apdu, return value */
    BACNET_PROPERTY_VALUE *value = NULL; /* value in list */
    BACNET_APPLICATION_DATA_VALUE *app_data = NULL;

    if (apdu) {
        /* tag 0 - subscriberProcessIdentifier */
        len = encode_context_unsigned(
            &apdu[apdu_len], 0, data->subscriberProcessIdentifier);
        apdu_len += len;
        /* tag 1 - initiatingDeviceIdentifier */
        len = encode_context_object_id(&apdu[apdu_len], 1, OBJECT_DEVICE,
            data->initiatingDeviceIdentifier);
        apdu_len += len;
        /* tag 2 - monitoredObjectIdentifier */
        len = encode_context_object_id(&apdu[apdu_len], 2,
            data->monitoredObjectIdentifier.type,
            data->monitoredObjectIdentifier.instance);
        apdu_len += len;
        /* tag 3 - timeRemaining */
        len = encode_context_unsigned(&apdu[apdu_len], 3, data->timeRemaining);
        apdu_len += len;
        /* tag 4 - listOfValues */
        len = encode_opening_tag(&apdu[apdu_len], 4);
        apdu_len += len;
        /* the first value includes a pointer to the next value, etc */
        /* FIXME: for small implementations, we might try a partial
           approach like the rpm.c where the values are encoded with
           a separate function */
        value = data->listOfValues;
        while (value != NULL) {
            /* tag 0 - propertyIdentifier */
            len = encode_context_enumerated(
                &apdu[apdu_len], 0, value->propertyIdentifier);
            apdu_len += len;
            /* tag 1 - propertyArrayIndex OPTIONAL */
            if (value->propertyArrayIndex != BACNET_ARRAY_ALL) {
                len = encode_context_unsigned(
                    &apdu[apdu_len], 1, value->propertyArrayIndex);
                apdu_len += len;
            }
            /* tag 2 - value */
            /* abstract syntax gets enclosed in a context tag */
            len = encode_opening_tag(&apdu[apdu_len], 2);
            apdu_len += len;
            app_data = &value->value;
            while (app_data != NULL) {
                len = bacapp_encode_application_data(&apdu[apdu_len], app_data);
                apdu_len += len;
                app_data = app_data->next;
            }

            len = encode_closing_tag(&apdu[apdu_len], 2);
            apdu_len += len;
            /* tag 3 - priority OPTIONAL */
            if (value->priority != BACNET_NO_PRIORITY) {
                len = encode_context_unsigned(
                    &apdu[apdu_len], 3, value->priority);
                apdu_len += len;
            }
            /* is there another one to encode? */
            /* FIXME: check to see if there is room in the APDU */
            value = value->next;
        }
        len = encode_closing_tag(&apdu[apdu_len], 4);
        apdu_len += len;
    }

    return apdu_len;
}

/**
 * Encode APDU for confirmed notification.
 *
 * @param apdu  Pointer to the buffer.
 * @param max_apdu_len  Buffer size.
 * @param invoke_id  ID to invoke for notification
 * @param data  Pointer to the data to encode.
 *
 * @return bytes encoded or zero on error.
 */
int ccov_notify_encode_apdu(uint8_t *apdu,
    unsigned max_apdu_len,
    uint8_t invoke_id,
    BACNET_COV_DATA *data)
{
    int len = 0; /* length of each encoding */
    int apdu_len = BACNET_STATUS_ERROR; /* return value */

    if (apdu && data && memcopylen(0, max_apdu_len, 4)) {
        apdu[0] = PDU_TYPE_CONFIRMED_SERVICE_REQUEST;
        apdu[1] = encode_max_segs_max_apdu(0, MAX_APDU);
        apdu[2] = invoke_id;
        apdu[3] = SERVICE_CONFIRMED_COV_NOTIFICATION;
        apdu_len = 4;
        len =
            notify_encode_apdu(&apdu[apdu_len], max_apdu_len - apdu_len, data);
        if (len < 0) {
            /* return the error */
            apdu_len = len;
        } else {
            apdu_len += len;
        }
    }

    return apdu_len;
}

/**
 * Encode APDU for unconfirmed notification.
 *
 * @param apdu  Pointer to the buffer.
 * @param max_apdu_len  Buffer size.
 * @param data  Pointer to the data to encode.
 *
 * @return bytes encoded or zero on error.
 */
int ucov_notify_encode_apdu(
    uint8_t *apdu, unsigned max_apdu_len, BACNET_COV_DATA *data)
{
    int len = 0; /* length of each encoding */
    int apdu_len = BACNET_STATUS_ERROR; /* return value */

    if (apdu && data && memcopylen(0, max_apdu_len, 2)) {
        apdu[0] = PDU_TYPE_UNCONFIRMED_SERVICE_REQUEST;
        apdu[1] = SERVICE_UNCONFIRMED_COV_NOTIFICATION; /* service choice */
        apdu_len = 2;
        len =
            notify_encode_apdu(&apdu[apdu_len], max_apdu_len - apdu_len, data);
        if (len < 0) {
            /* return the error */
            apdu_len = len;
        } else {
            apdu_len += len;
        }
    }

    return apdu_len;
}

/**
 * Decode the COV-service request only.
 * Note: COV and Unconfirmed COV are the same.
 *
 * @param apdu  Pointer to the buffer.
 * @param apdu_len  Count of valid bytes in the buffer.
 * @param data  Pointer to the data to store the decoded values.
 *
 * @return Bytes decoded or Zero/BACNET_STATUS_ERROR on error.
 */
int cov_notify_decode_service_request(
    uint8_t *apdu, unsigned apdu_len, BACNET_COV_DATA *data)
{
    int len = 0; /* return value */
    int app_len = 0;
    uint8_t tag_number = 0;
    uint32_t len_value = 0;
    BACNET_UNSIGNED_INTEGER decoded_value = 0; /* for decoding */
    BACNET_OBJECT_TYPE decoded_type = OBJECT_NONE; /* for decoding */
    uint32_t property = 0; /* for decoding */
    BACNET_PROPERTY_VALUE *value = NULL; /* value in list */
    BACNET_APPLICATION_DATA_VALUE *app_data = NULL;

    if ((apdu_len > 2) && data) {
        /* tag 0 - subscriberProcessIdentifier */
        if (decode_is_context_tag(&apdu[len], 0)) {
            len += decode_tag_number_and_value(
                &apdu[len], &tag_number, &len_value);
            len += decode_unsigned(&apdu[len], len_value, &decoded_value);
            data->subscriberProcessIdentifier = decoded_value;
        } else {
            return BACNET_STATUS_ERROR;
        }
        /* tag 1 - initiatingDeviceIdentifier */
        if (len >= (int)apdu_len) {
            return BACNET_STATUS_ERROR;
        }
        if (decode_is_context_tag(&apdu[len], 1)) {
            len += decode_tag_number_and_value(
                &apdu[len], &tag_number, &len_value);
            len += decode_object_id(
                &apdu[len], &decoded_type, &data->initiatingDeviceIdentifier);
            if (decoded_type != OBJECT_DEVICE) {
                return BACNET_STATUS_ERROR;
            }
        } else {
            return BACNET_STATUS_ERROR;
        }
        /* tag 2 - monitoredObjectIdentifier */
        if (len >= (int)apdu_len) {
            return BACNET_STATUS_ERROR;
        }
        if (decode_is_context_tag(&apdu[len], 2)) {
            len += decode_tag_number_and_value(
                &apdu[len], &tag_number, &len_value);
            len += decode_object_id(&apdu[len], &decoded_type,
                &data->monitoredObjectIdentifier.instance);
            data->monitoredObjectIdentifier.type = decoded_type;
        } else {
            return BACNET_STATUS_ERROR;
        }
        /* tag 3 - timeRemaining */
        if (len >= (int)apdu_len) {
            return BACNET_STATUS_ERROR;
        }
        if (decode_is_context_tag(&apdu[len], 3)) {
            len += decode_tag_number_and_value(
                &apdu[len], &tag_number, &len_value);
            len += decode_unsigned(&apdu[len], len_value, &decoded_value);
            data->timeRemaining = decoded_value;
        } else {
            return BACNET_STATUS_ERROR;
        }
        /* tag 4: opening context tag - listOfValues */
        if (!decode_is_opening_tag_number(&apdu[len], 4)) {
            return BACNET_STATUS_ERROR;
        }
        /* a tag number of 4 is not extended so only one octet */
        len++;
        /* the first value includes a pointer to the next value, etc */
        value = data->listOfValues;
        if (value == NULL) {
            /* no space to store any values */
            return BACNET_STATUS_ERROR;
        }
        while (value != NULL) {
            /* tag 0 - propertyIdentifier */
            if (len >= (int)apdu_len) {
                return BACNET_STATUS_ERROR;
            }
            if (decode_is_context_tag(&apdu[len], 0)) {
                len += decode_tag_number_and_value(
                    &apdu[len], &tag_number, &len_value);
                len += decode_enumerated(&apdu[len], len_value, &property);
                value->propertyIdentifier = (BACNET_PROPERTY_ID)property;
            } else {
                return BACNET_STATUS_ERROR;
            }
            /* tag 1 - propertyArrayIndex OPTIONAL */
            if (len >= (int)apdu_len) {
                return BACNET_STATUS_ERROR;
            }
            if (decode_is_context_tag(&apdu[len], 1)) {
                len += decode_tag_number_and_value(
                    &apdu[len], &tag_number, &len_value);
                len += decode_unsigned(&apdu[len], len_value, &decoded_value);
                value->propertyArrayIndex = decoded_value;
            } else {
                value->propertyArrayIndex = BACNET_ARRAY_ALL;
            }
            /* tag 2: opening context tag - value */
            if (len >= (int)apdu_len) {
                return BACNET_STATUS_ERROR;
            }
            if (!decode_is_opening_tag_number(&apdu[len], 2)) {
                return BACNET_STATUS_ERROR;
            }
            /* a tag number of 2 is not extended so only one octet */
            len++;
            app_data = &value->value;
            while (!decode_is_closing_tag_number(&apdu[len], 2)) {
                if (app_data == NULL) {
                    /* out of room to store more values */
                    return BACNET_STATUS_ERROR;
                }
                app_len = bacapp_decode_application_data(
                    &apdu[len], apdu_len - len, app_data);
                if (app_len < 0) {
                    return BACNET_STATUS_ERROR;
                }
                len += app_len;

                app_data = app_data->next;
            }
            /* a tag number of 2 is not extended so only one octet */
            len++;
            /* tag 3 - priority OPTIONAL */
            if (len >= (int)apdu_len) {
                return BACNET_STATUS_ERROR;
            }
            if (decode_is_context_tag(&apdu[len], 3)) {
                len += decode_tag_number_and_value(
                    &apdu[len], &tag_number, &len_value);
                len += decode_unsigned(&apdu[len], len_value, &decoded_value);
                value->priority = (uint8_t)decoded_value;
            } else {
                value->priority = BACNET_NO_PRIORITY;
            }
            /* end of list? */
            if (decode_is_closing_tag_number(&apdu[len], 4)) {
                value->next = NULL;
                break;
            }
            /* is there another one to decode? */
            value = value->next;
            if (value == NULL) {
                /* out of room to store more values */
                return BACNET_STATUS_ERROR;
            }
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
 * Encode the COV-service request.
 * Note: COV and Unconfirmed COV are the same.
 *
 * @param apdu  Pointer to the buffer.
 * @param invoke_id  Invoke ID
 * @param data  Pointer to the data to store the decoded values.
 *
 * @return Bytes encoded or zero on error.
 */
int cov_subscribe_encode_apdu(uint8_t *apdu,
    unsigned max_apdu_len,
    uint8_t invoke_id,
    BACNET_SUBSCRIBE_COV_DATA *data)
{
    int len = 0; /* length of each encoding */
    int apdu_len = 0; /* total length of the apdu, return value */

    if (apdu && data) {
        apdu[0] = PDU_TYPE_CONFIRMED_SERVICE_REQUEST;
        apdu[1] = encode_max_segs_max_apdu(0, MAX_APDU);
        apdu[2] = invoke_id;
        apdu[3] = SERVICE_CONFIRMED_SUBSCRIBE_COV;
        apdu_len = 4;
        /* tag 0 - subscriberProcessIdentifier */
        len = encode_context_unsigned(
            &apdu[apdu_len], 0, data->subscriberProcessIdentifier);
        apdu_len += len;
        /* tag 1 - monitoredObjectIdentifier */
        len = encode_context_object_id(&apdu[apdu_len], 1,
            data->monitoredObjectIdentifier.type,
            data->monitoredObjectIdentifier.instance);
        apdu_len += len;
        /*
           If both the 'Issue Confirmed Notifications' and
           'Lifetime' parameters are absent, then this shall
           indicate a cancellation request.
         */
        if (!data->cancellationRequest) {
            /* tag 2 - issueConfirmedNotifications */
            len = encode_context_boolean(
                &apdu[apdu_len], 2, data->issueConfirmedNotifications);
            apdu_len += len;
            /* tag 3 - lifetime */
            len = encode_context_unsigned(&apdu[apdu_len], 3, data->lifetime);
            apdu_len += len;
        }
    }

    return apdu_len;
}

/**
 * Decode the subscribe-service request only.
 *
 * @param apdu  Pointer to the buffer.
 * @param apdu_len  Count of valid bytes in the buffer.
 * @param data  Pointer to the data to store the decoded values.
 *
 * @return Bytes decoded or Zero/BACNET_STATUS_ERROR on error.
 */
int cov_subscribe_decode_service_request(
    uint8_t *apdu, unsigned apdu_len, BACNET_SUBSCRIBE_COV_DATA *data)
{
    int len = 0; /* return value */
    uint8_t tag_number = 0;
    uint32_t len_value = 0;
    BACNET_UNSIGNED_INTEGER unsigned_value = 0;
    BACNET_OBJECT_TYPE decoded_type = OBJECT_NONE;

    if ((apdu_len > 2) && data) {
        /* tag 0 - subscriberProcessIdentifier */
        if (decode_is_context_tag(&apdu[len], 0)) {
            len += decode_tag_number_and_value(
                &apdu[len], &tag_number, &len_value);
            len += decode_unsigned(&apdu[len], len_value, &unsigned_value);
            data->subscriberProcessIdentifier = unsigned_value;
        } else {
            data->error_code = ERROR_CODE_REJECT_INVALID_TAG;
            return BACNET_STATUS_REJECT;
        }
        /* tag 1 - monitoredObjectIdentifier */
        if ((unsigned)len >= apdu_len) {
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
        /* optional parameters - if missing, means cancellation */
        if ((unsigned)len < apdu_len) {
            /* tag 2 - issueConfirmedNotifications - optional */
            if (decode_is_context_tag(&apdu[len], 2)) {
                data->cancellationRequest = false;
                len += decode_tag_number_and_value(
                    &apdu[len], &tag_number, &len_value);
                data->issueConfirmedNotifications =
                    decode_context_boolean(&apdu[len]);
                len += len_value;
            } else {
                data->cancellationRequest = true;
            }
            /* tag 3 - lifetime - optional */
            if ((unsigned)len < apdu_len) {
                if (decode_is_context_tag(&apdu[len], 3)) {
                    len += decode_tag_number_and_value(
                        &apdu[len], &tag_number, &len_value);
                    len +=
                        decode_unsigned(&apdu[len], len_value, &unsigned_value);
                    data->lifetime = unsigned_value;
                } else {
                    data->lifetime = 0;
                }
            } else {
                data->lifetime = 0;
            }
        } else {
            data->cancellationRequest = true;
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
 * Encode the properties for subscription into the APDU.
 *
 * @param apdu  Pointer to the buffer.
 * @param max_apdu_len  Buffer size.
 * @param invoke_id  Invoke Id.
 * @param data  Pointer to the data to encode.
 *
 * @return Bytes decoded or Zero/BACNET_STATUS_ERROR on error.
 */
int cov_subscribe_property_encode_apdu(uint8_t *apdu,
    unsigned max_apdu_len,
    uint8_t invoke_id,
    BACNET_SUBSCRIBE_COV_DATA *data)
{
    int len = 0; /* length of each encoding */
    int apdu_len = 0; /* total length of the apdu, return value */

    if (apdu && data) {
        apdu[0] = PDU_TYPE_CONFIRMED_SERVICE_REQUEST;
        apdu[1] = encode_max_segs_max_apdu(0, MAX_APDU);
        apdu[2] = invoke_id;
        apdu[3] = SERVICE_CONFIRMED_SUBSCRIBE_COV_PROPERTY;
        apdu_len = 4;
        /* tag 0 - subscriberProcessIdentifier */
        len = encode_context_unsigned(
            &apdu[apdu_len], 0, data->subscriberProcessIdentifier);
        apdu_len += len;
        /* tag 1 - monitoredObjectIdentifier */
        len = encode_context_object_id(&apdu[apdu_len], 1,
            data->monitoredObjectIdentifier.type,
            data->monitoredObjectIdentifier.instance);
        apdu_len += len;
        if (!data->cancellationRequest) {
            /* tag 2 - issueConfirmedNotifications */
            len = encode_context_boolean(
                &apdu[apdu_len], 2, data->issueConfirmedNotifications);
            apdu_len += len;
            /* tag 3 - lifetime */
            len = encode_context_unsigned(&apdu[apdu_len], 3, data->lifetime);
            apdu_len += len;
        }
        /* tag 4 - monitoredPropertyIdentifier */
        len = encode_opening_tag(&apdu[apdu_len], 4);
        apdu_len += len;
        len = encode_context_enumerated(
            &apdu[apdu_len], 0, data->monitoredProperty.propertyIdentifier);
        apdu_len += len;
        if (data->monitoredProperty.propertyArrayIndex != BACNET_ARRAY_ALL) {
            len = encode_context_unsigned(
                &apdu[apdu_len], 1, data->monitoredProperty.propertyArrayIndex);
            apdu_len += len;
        }
        len = encode_closing_tag(&apdu[apdu_len], 4);
        apdu_len += len;

        /* tag 5 - covIncrement */
        if (data->covIncrementPresent) {
            len = encode_context_real(&apdu[apdu_len], 5, data->covIncrement);
            apdu_len += len;
        }
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
    BACNET_PROPERTY_VALUE *current_value_list = NULL;

    if (data && value_list) {
        data->listOfValues = value_list;
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
