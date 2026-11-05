/**
 * @file
 * @date 2011
 * @brief GetAlarmSummary service encoding and decoding
 * @author Krzysztof Malorny <malornykrzysztof@gmail.com>
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @details The GetAlarmSummary service is used by a client BACnet-user
 * to obtain a summary of "active alarms." The term "active alarm" refers
 * to BACnet standard objects that have an Event_State property whose value
 * is not equal to NORMAL and a Notify_Type property whose value is ALARM.
 * The GetEnrollmentSummary service provides a more sophisticated approach
 * with various kinds of filters
 * @copyright SPDX-License-Identifier: GPL-2.0-or-later WITH GCC-exception-2.0
 */
#include "bacnet/bacdcode.h"
#include "bacnet/get_alarm_sum.h"
#include "bacnet/npdu.h"

/**
 * @brief Encode the GetAlarmSummary confirmed service request
 * @param apdu - buffer where to put encoding, or NULL for length
 * @param invoke_id - unique sequence number sent with the message
 * @return number of bytes encoded
 */
int get_alarm_summary_encode_apdu(uint8_t *apdu, uint8_t invoke_id)
{
    if (apdu) {
        apdu[0] = PDU_TYPE_CONFIRMED_SERVICE_REQUEST;
        apdu[1] = encode_max_segs_max_apdu(0, MAX_APDU);
        apdu[2] = invoke_id;
        apdu[3] = SERVICE_CONFIRMED_GET_ALARM_SUMMARY;
    }

    return 4;
}

/** Helper function encode the beginning of a GetAlarmSummary ACK.
 *
 * @param apdu - buffer where to put encoding, or NULL for length
 * @param invoke_id - unique sequence number sent with the message
 *
 * @return number of bytes encoded
 */
int get_alarm_summary_ack_encode_apdu_init(uint8_t *apdu, uint8_t invoke_id)
{
    if (apdu) {
        apdu[0] = PDU_TYPE_COMPLEX_ACK; /* complex ACK service */
        apdu[1] = invoke_id; /* original invoke id from request */
        apdu[2] = SERVICE_CONFIRMED_GET_ALARM_SUMMARY;
    }

    return 3;
}

/**
 * @brief Encode one entry of the GetAlarmSummary ACK
 *
 *  GetAlarmSummary-ACK ::= SEQUENCE OF SEQUENCE {
 *    object-identifier         BACnetObjectIdentifier,
 *    alarm-state               BACnetEventState,
 *    acknowledged-transitions  BACnetEventTransitionBits
 *    }
 *
 * @param apdu - buffer where to put encoding, or NULL for length
 * @param data - BACNET_GET_ALARM_SUMMARY_DATA type with data
 * @return number of bytes encoded, or zero if data is NULL
 */
int get_alarm_summary_ack_data_encode(
    uint8_t *apdu, const BACNET_GET_ALARM_SUMMARY_DATA *data)
{
    int len = 0;
    int apdu_len = 0;

    if (!data) {
        return 0;
    }
    /* object-identifier - BACnetObjectIdentifier, */
    len = encode_application_object_id(
        apdu, data->objectIdentifier.type, data->objectIdentifier.instance);
    apdu_len += len;
    if (apdu) {
        apdu += len;
    }
    /* alarm-state - BACnetEventState, */
    len = encode_application_enumerated(apdu, data->alarmState);
    apdu_len += len;
    if (apdu) {
        apdu += len;
    }
    /* acknowledged-transitions - BACnetEventTransitionBits */
    len = encode_application_bitstring(apdu, &data->acknowledgedTransitions);
    apdu_len += len;

    return apdu_len;
}

/** Helper function encode the data portion of a GetAlarmSummary ACK.
 *
 * @param apdu - buffer where to put encoding, or NULL for length
 * @param max_apdu - number of bytes available in the buffer for encoding
 * @param get_alarm_data - BACNET_GET_ALARM_SUMMARY_DATA type with data
 *
 * @return number of bytes encoded, BACNET_STATUS_ERROR if get_alarm_data is
 *  NULL, or BACNET_STATUS_ABORT if the data does not fit in max_apdu.
 */
int get_alarm_summary_ack_encode_apdu_data(
    uint8_t *apdu,
    size_t max_apdu,
    const BACNET_GET_ALARM_SUMMARY_DATA *get_alarm_data)
{
    int apdu_len = 0; /* total length of the apdu, return value */

    if (!get_alarm_data) {
        return BACNET_STATUS_ERROR;
    }
    apdu_len = get_alarm_summary_ack_data_encode(NULL, get_alarm_data);
    if (!apdu) {
        return apdu_len;
    }
    if ((size_t)apdu_len > max_apdu) {
        return BACNET_STATUS_ABORT;
    }

    return get_alarm_summary_ack_data_encode(apdu, get_alarm_data);
}

/** Helper function to decode the data portion of a GetAlarmSummary ACK.
 *
 *  GetAlarmSummary-ACK ::= SEQUENCE OF SEQUENCE {
 *    object-identifier         BACnetObjectIdentifier,
 *    alarm-state               BACnetEventState,
 *    acknowledged-transitions  BACnetEventTransitionBits
 *    }
 *
 * @param apdu - buffer of data to be decoded
 * @param apdu_size - number of bytes in the buffer to decode
 * @param get_alarm_data - BACNET_GET_ALARM_SUMMARY_DATA type for data,
 *  or NULL to only return the decoded length
 *
 * @return number of bytes decoded, 0 if apdu_size is too small for one
 *  entry, or BACNET_STATUS_ERROR if an error.
 */
int get_alarm_summary_ack_decode_apdu_data(
    const uint8_t *apdu,
    size_t apdu_size,
    BACNET_GET_ALARM_SUMMARY_DATA *get_alarm_data)
{
    int apdu_len = 0; /* total length of the apdu, return value */
    int len = 0;
    BACNET_OBJECT_TYPE object_type = OBJECT_NONE;
    uint32_t object_instance = 0;
    uint32_t enum_value = 0;
    BACNET_BIT_STRING bit_string = { 0 };

    if (!apdu) {
        return BACNET_STATUS_ERROR;
    }
    if (apdu_size < 10) {
        return 0;
    }
    /* object-identifier - BACnetObjectIdentifier, */
    len = bacnet_object_id_application_decode(
        &apdu[apdu_len], (uint32_t)(apdu_size - (size_t)apdu_len), &object_type,
        &object_instance);
    if (len <= 0) {
        return BACNET_STATUS_ERROR;
    }
    apdu_len += len;
    /* alarm-state - BACnetEventState, */
    len = bacnet_enumerated_application_decode(
        &apdu[apdu_len], (uint32_t)(apdu_size - (size_t)apdu_len), &enum_value);
    if (len <= 0) {
        return BACNET_STATUS_ERROR;
    }
    apdu_len += len;
    /* acknowledged-transitions - BACnetEventTransitionBits */
    len = bacnet_bitstring_application_decode(
        &apdu[apdu_len], (uint32_t)(apdu_size - (size_t)apdu_len), &bit_string);
    if (len <= 0) {
        return BACNET_STATUS_ERROR;
    }
    apdu_len += len;
    if (get_alarm_data) {
        get_alarm_data->objectIdentifier.type = object_type;
        get_alarm_data->objectIdentifier.instance = object_instance;
        get_alarm_data->alarmState = (BACNET_EVENT_STATE)enum_value;
        bitstring_copy(&get_alarm_data->acknowledgedTransitions, &bit_string);
    }

    return apdu_len;
}
