/*####COPYRIGHTBEGIN####
 -------------------------------------------
 Copyright (C) 2009 John Minack

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
#include <stddef.h>
#include <stdint.h>
#include "bacnet/alarm_ack.h"

/** @file alarm_ack.c  Handles Event Notifications (ACKs) */

/**
 * @brief Creates an Unconfirmed Event Notification APDU
 *
 * @param apdu  application data buffer, or NULL for length
 * @param invoke_id  Invoked ID
 * @param data  Alarm acknowledge data that will be copied into the payload.
 *
 * @return number of bytes encoded
 */
int alarm_ack_encode_apdu(
    uint8_t *apdu, uint8_t invoke_id, BACNET_ALARM_ACK_DATA *data)
{
    int len = 0; /* length of each encoding */
    int apdu_len = 0; /* total length of the apdu, return value */

    if (apdu) {
        apdu[0] = PDU_TYPE_CONFIRMED_SERVICE_REQUEST;
        apdu[1] = encode_max_segs_max_apdu(0, MAX_APDU);
        apdu[2] = invoke_id;
        apdu[3] = SERVICE_CONFIRMED_ACKNOWLEDGE_ALARM; /* service choice */
    }
    len = 4;
    apdu_len += len;
    if (apdu) {
        apdu += len;
    }
    len = alarm_ack_encode_service_request(apdu, data);
    apdu_len += len;

    return apdu_len;
}

/**
 * @brief Encodes the service data part of Event Notification.
 *
 * @param apdu  application data buffer, or NULL for length
 * @param data  Alarm acknowledge data that will be filled in
 *              with the data from the payload.
 *
 * @return number of bytes encoded
 */
int alarm_ack_encode_service_request(uint8_t *apdu, BACNET_ALARM_ACK_DATA *data)
{
    int len = 0; /* length of each encoding */
    int apdu_len = 0; /* total length of the apdu, return value */

    if (!data) {
        return 0;
    }
    len = encode_context_unsigned(apdu, 0, data->ackProcessIdentifier);
    apdu_len += len;
    if (apdu) {
        apdu += len;
    }
    len = encode_context_object_id(apdu, 1, data->eventObjectIdentifier.type,
        data->eventObjectIdentifier.instance);
    apdu_len += len;
    if (apdu) {
        apdu += len;
    }
    len = encode_context_enumerated(apdu, 2, data->eventStateAcked);
    apdu_len += len;
    if (apdu) {
        apdu += len;
    }
    len = bacapp_encode_context_timestamp(apdu, 3, &data->eventTimeStamp);
    apdu_len += len;
    if (apdu) {
        apdu += len;
    }
    len = encode_context_character_string(apdu, 4, &data->ackSource);
    apdu_len += len;
    if (apdu) {
        apdu += len;
    }
    len = bacapp_encode_context_timestamp(apdu, 5, &data->ackTimeStamp);
    apdu_len += len;

    return apdu_len;
}

/**
 * @brief Decodes the service data part of Event Notification.
 *
 * @param apdu  application date buffer
 * @param apdu_size number of bytes application date buffer
 * @param data  Alarm acknowledge data that will be filled.
 *
 * @return number of bytes decoded, or BACNET_STATUS_ERROR on error.
 */
int alarm_ack_decode_service_request(
    uint8_t *apdu, unsigned apdu_size, BACNET_ALARM_ACK_DATA *data)
{
    int len = 0;
    int apdu_len = 0;
    uint32_t enum_value = 0;
    BACNET_UNSIGNED_INTEGER unsigned_value = 0;
    BACNET_OBJECT_TYPE object_type = 0;
    uint32_t instance = 0;
    BACNET_TIMESTAMP *timestamp_value = NULL;
    BACNET_CHARACTER_STRING *cs_value = NULL;

    if (!apdu) {
        return BACNET_STATUS_ERROR;
    }
    if (apdu_size == 0) {
        return BACNET_STATUS_ERROR;
    }
    len = bacnet_unsigned_context_decode(
        &apdu[apdu_len], apdu_size - apdu_len, 0, &unsigned_value);
    if (len > 0) {
        apdu_len += len;
    } else {
        return BACNET_STATUS_ERROR;
    }
    if (data) {
        data->ackProcessIdentifier = (uint32_t)unsigned_value;
    }
    len = bacnet_object_id_context_decode(
        &apdu[apdu_len], apdu_size - apdu_len, 1, &object_type, &instance);
    if (len > 0) {
        apdu_len += len;
    } else {
        return BACNET_STATUS_ERROR;
    }
    if (data) {
        data->eventObjectIdentifier.type = object_type;
        data->eventObjectIdentifier.instance = instance;
    }
    len = bacnet_enumerated_context_decode(
        &apdu[apdu_len], apdu_size - apdu_len, 2, &enum_value);
    if (len > 0) {
        apdu_len += len;
    } else {
        return BACNET_STATUS_ERROR;
    }
    if (data) {
        data->eventStateAcked = (BACNET_EVENT_STATE)enum_value;
    }
    if (data) {
        timestamp_value = &data->eventTimeStamp;
    }
    len = bacnet_timestamp_context_decode(
        &apdu[apdu_len], apdu_size - apdu_len, 3, timestamp_value);
    if (len > 0) {
        apdu_len += len;
    } else {
        return BACNET_STATUS_ERROR;
    }
    if (data) {
        cs_value = &data->ackSource;
    }
    len = bacnet_character_string_context_decode(
        &apdu[apdu_len], apdu_size - apdu_len, 4, cs_value);
    if (len > 0) {
        apdu_len += len;
    } else {
        return BACNET_STATUS_ERROR;
    }
    if (data) {
        timestamp_value = &data->ackTimeStamp;
    }
    len = bacnet_timestamp_context_decode(
        &apdu[apdu_len], apdu_size - apdu_len, 5, timestamp_value);
    if (len > 0) {
        apdu_len += len;
    } else {
        return BACNET_STATUS_ERROR;
    }

    return apdu_len;
}
