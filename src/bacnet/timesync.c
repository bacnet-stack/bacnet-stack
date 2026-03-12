/**
 * @file
 * @brief BACnet TimeSyncronization service and BACnetRecipientList data
 *  encoder and decoder
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
#include "bacnet/timesync.h"

#if BACNET_SVC_TS_A
/**
 * @brief Encode the time synchronisation service.
 *
 * TimeSynchronization-Request ::= SEQUENCE {
 *   time BACnetDateTime
 * }
 *
 * @param apdu [in] Buffer in which the APDU contents are written
 * @param service [in] Time service that shall be encoded, like
 *                     SERVICE_UNCONFIRMED_UTC_TIME_SYNCHRONIZATION.
 * @param my_date [in] Pointer to the date structure used to encode.
 * @param my_time [in] Pointer to the time structure used to encode.
 *
 * @return Count of encoded bytes.
 */
int timesync_encode_apdu_service_parameters(
    uint8_t *apdu, const BACNET_DATE *my_date, const BACNET_TIME *my_time)
{
    int len = 0; /* length of each encoding */
    int apdu_len = 0; /* total length of the apdu, return value */

    if (my_date && my_time) {
        /* encode the date and time */
        len = encode_application_date(apdu, my_date);
        apdu_len += len;
        if (apdu) {
            apdu += len;
        }
        len = encode_application_time(apdu, my_time);
        apdu_len += len;
    }

    return apdu_len;
}

/** Encode the time synchronisation service.
 *
 * @param apdu [in] Buffer in which the APDU contents are written
 * @param service [in] Time service that shall be encoded, like
 *                     SERVICE_UNCONFIRMED_UTC_TIME_SYNCHRONIZATION.
 * @param my_date [in] Pointer to the date structure used to encode.
 * @param my_time [in] Pointer to the time structure used to encode.
 *
 * @return Count of encoded bytes.
 */
int timesync_encode_apdu_service(
    uint8_t *apdu,
    BACNET_UNCONFIRMED_SERVICE service,
    const BACNET_DATE *my_date,
    const BACNET_TIME *my_time)
{
    int len = 0; /* length of each encoding */
    int apdu_len = 0; /* total length of the apdu, return value */

    if (apdu) {
        apdu[0] = PDU_TYPE_UNCONFIRMED_SERVICE_REQUEST;
        apdu[1] = service;
    }
    len = 2;
    apdu_len += len;
    if (apdu) {
        apdu += len;
    }
    len = timesync_encode_apdu_service_parameters(apdu, my_date, my_time);
    apdu_len += len;

    return apdu_len;
}

/** Encode the service 'UTC_TIME_SYNCHRONIZATION'.
 *
 * @param apdu [in] Buffer in which the APDU contents are written
 * @param my_date [in] Pointer to the date structure used to encode.
 * @param my_time [in] Pointer to the time structure used to encode.
 *
 * @return Count of encoded bytes.
 */
int timesync_utc_encode_apdu(
    uint8_t *apdu, const BACNET_DATE *my_date, const BACNET_TIME *my_time)
{
    return timesync_encode_apdu_service(
        apdu, SERVICE_UNCONFIRMED_UTC_TIME_SYNCHRONIZATION, my_date, my_time);
}

/** Encode the service 'TIME_SYNCHRONIZATION'.
 *
 * @param apdu [in] Buffer in which the APDU contents are written
 * @param my_date [in] Pointer to the date structure used to encode.
 * @param my_time [in] Pointer to the time structure used to encode.
 *
 * @return Count of encoded bytes.
 */
int timesync_encode_apdu(
    uint8_t *apdu, const BACNET_DATE *my_date, const BACNET_TIME *my_time)
{
    return timesync_encode_apdu_service(
        apdu, SERVICE_UNCONFIRMED_TIME_SYNCHRONIZATION, my_date, my_time);
}
#endif

/** Decode the service request only.
 *
 * @param apdu [in] Buffer in which the APDU contents are read
 * @param apdu_size [in] length of the APDU buffer.
 * @param my_date [in] Pointer to the date structure filled in.
 * @param my_time [in] Pointer to the time structure filled in.
 *
 * @return Count of decoded bytes.
 */
int timesync_decode_service_request(
    const uint8_t *apdu,
    unsigned apdu_size,
    BACNET_DATE *my_date,
    BACNET_TIME *my_time)
{
    int len = 0, apdu_len = 0;

    if (!(apdu && apdu_size)) {
        return BACNET_STATUS_ERROR;
    }
    len = bacnet_date_application_decode(
        &apdu[apdu_len], apdu_size - apdu_len, my_date);
    if (len <= 0) {
        return BACNET_STATUS_ERROR;
    }
    apdu_len += len;
    len = bacnet_time_application_decode(
        &apdu[apdu_len], apdu_size - apdu_len, my_time);
    if (len <= 0) {
        return BACNET_STATUS_ERROR;
    }
    apdu_len += len;

    return apdu_len;
}

/** Handle a request to encode the list of timesync recipients.
 *
 *  Invoked by a request to read the Device object's
 *  PROP_TIME_SYNCHRONIZATION_RECIPIENTS.
 *  Loops through the list of timesync recipients, and, for each one,
 *  adds its data to the APDU.
 *
 *   BACnetRecipient ::= CHOICE {
 *       device [0] BACnetObjectIdentifier,
 *       address [1] BACnetAddress
 *   }
 *
 *   BACnetAddress ::= SEQUENCE {
 *       network-number Unsigned16, -- A value of 0 indicates the local network
 *       mac-address OCTET STRING -- A string of length 0 indicates a broadcast
 *   }
 *
 *  @param apdu [out] Buffer in which the APDU contents are built.
 *  @param apdu_size [in] Max length of the APDU buffer.
 *  @param recipient [in] BACNET_RECIPIENT_LIST type linked list of recipients.
 *
 *  @return How many bytes were encoded in the buffer, or
 *   BACNET_STATUS_ABORT if the response would not fit within the buffer.
 */
int timesync_encode_timesync_recipients(
    uint8_t *apdu, unsigned apdu_size, BACNET_RECIPIENT_LIST *list_head)
{
    int apdu_len = 0;

    apdu_len = bacnet_recipient_list_encode(NULL, list_head);
    if (apdu_len > apdu_size) {
        /* response would not fit within the buffer */
        return BACNET_STATUS_ABORT;
    }
    apdu_len = bacnet_recipient_list_encode(apdu, list_head);

    return apdu_len;
}

/** Handle a request to decode a list of timesync recipients.
 *
 *  Invoked by a request to write the Device object's
 *  PROP_TIME_SYNCHRONIZATION_RECIPIENTS.
 *  Loops through the list of timesync recipients, and, for each one,
 *  adds its data from the APDU.
 *
 *   BACnetRecipient ::= CHOICE {
 *       device [0] BACnetObjectIdentifier,
 *       address [1] BACnetAddress
 *   }
 *
 *   BACnetAddress ::= SEQUENCE {
 *       network-number Unsigned16, -- A value of 0 indicates the local network
 *       mac-address OCTET STRING -- A string of length 0 indicates a broadcast
 *   }
 *
 *  @param apdu [in] Buffer in which the APDU contents are read
 *  @param max_apdu [in] length of the APDU buffer.
 *  @param recipient [out] BACNET_RECIPIENT_LIST type linked list of recipients.
 *
 *  @return How many bytes were decoded from the buffer, or
 *   BACNET_STATUS_ABORT if there was a problem decoding the buffer
 */
int timesync_decode_timesync_recipients(
    const uint8_t *apdu, unsigned apdu_size, BACNET_RECIPIENT_LIST *list_head)
{
    int len = 0;
    int apdu_len = 0;
    BACNET_RECIPIENT_LIST *list;
    BACNET_RECIPIENT *recipient;

    if ((!apdu) || (apdu_size == 0)) {
        return BACNET_STATUS_ABORT;
    }
    list = list_head;
    while (apdu_len < apdu_size) {
        if (list) {
            recipient = &list->recipient;
        } else {
            recipient = NULL;
        }
        len = bacnet_recipient_decode(
            &apdu[apdu_len], apdu_size - apdu_len, recipient);
        if (len <= 0) {
            return BACNET_STATUS_ABORT;
        }
        apdu_len += len;
        if (list) {
            list = list->next;
        }
    }

    return apdu_len;
}
