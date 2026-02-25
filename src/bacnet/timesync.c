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
int timesync_encode_apdu_service_paramters(
    uint8_t *apdu, const BACNET_DATE *my_date, const BACNET_TIME *my_time)
{
    int len = 0; /* length of each encoding */
    int apdu_len = 0; /* total length of the apdu, return value */

    len = encode_application_date(apdu, my_date);
    apdu_len += len;
    if (apdu) {
        apdu += len;
    }
    len = encode_application_time(apdu, my_time);
    apdu_len += len;

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
    len = timesync_encode_apdu_service_paramters(apdu, my_date, my_time);
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
 *  @param max_apdu [in] Max length of the APDU buffer.
 *  @param recipient [in] BACNET_RECIPIENT_LIST type linked list of recipients.
 *
 *  @return How many bytes were encoded in the buffer, or
 *   BACNET_STATUS_ABORT if the response would not fit within the buffer.
 */
int timesync_encode_timesync_recipients(
    uint8_t *apdu, unsigned max_apdu, BACNET_RECIPIENT_LIST *recipient)
{
    int len = 0;
    int apdu_len = 0;
    BACNET_OCTET_STRING octet_string;
    BACNET_RECIPIENT_LIST *pRecipient;

    if ((!apdu) || (max_apdu < 1) || (!recipient)) {
        return (0);
    }

    pRecipient = recipient;
    while (pRecipient != NULL) {
        if (pRecipient->tag == 0) {
            if (max_apdu >= (1 + 4)) {
                /* CHOICE - device [0] BACnetObjectIdentifier */
                if (pRecipient->type.device.type == OBJECT_DEVICE) {
                    len = encode_context_object_id(
                        &apdu[apdu_len], 0, pRecipient->type.device.type,
                        pRecipient->type.device.instance);
                    apdu_len += len;
                }
            } else {
                return BACNET_STATUS_ABORT;
            }
        } else if (pRecipient->tag == 1) {
            if (pRecipient->type.address.net) {
                len = (int)(1 + 3 + 2 + pRecipient->type.address.len + 1);
            } else {
                len = (int)(1 + 3 + 2 + pRecipient->type.address.mac_len + 1);
            }
            if (max_apdu >= (unsigned)len) {
                /* CHOICE - address [1] BACnetAddress - opening */
                len = encode_opening_tag(&apdu[apdu_len], 1);
                apdu_len += len;
                /* network-number Unsigned16, */
                /* -- A value of 0 indicates the local network */
                len = encode_application_unsigned(
                    &apdu[apdu_len], pRecipient->type.address.net);
                apdu_len += len;
                /* mac-address OCTET STRING */
                /* -- A string of length 0 indicates a broadcast */
                if (pRecipient->type.address.net == BACNET_BROADCAST_NETWORK) {
                    octetstring_init(&octet_string, NULL, 0);
                } else if (pRecipient->type.address.net) {
                    octetstring_init(
                        &octet_string, &pRecipient->type.address.adr[0],
                        pRecipient->type.address.len);
                } else {
                    octetstring_init(
                        &octet_string, &pRecipient->type.address.mac[0],
                        pRecipient->type.address.mac_len);
                }
                len = encode_application_octet_string(
                    &apdu[apdu_len], &octet_string);
                apdu_len += len;
                /* CHOICE - address [1] BACnetAddress - closing */
                len = encode_closing_tag(&apdu[apdu_len], 1);
                apdu_len += len;
            } else {
                /* not a valid tag - don't encode this one */
            }
        }
        pRecipient = pRecipient->next;
    }

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
    const uint8_t *apdu, unsigned apdu_size, BACNET_RECIPIENT_LIST *recipient)
{
    int len = 0;
    int apdu_len = 0;
    uint16_t network_number = 0;
    BACNET_UNSIGNED_INTEGER unsigned_value = 0;
    BACNET_OBJECT_TYPE object_type;
    uint32_t object_instance;
    BACNET_OCTET_STRING_BUFFER octet_string;
    BACNET_RECIPIENT_LIST *pRecipient;

    if ((!apdu) || (apdu_size < 1) || (!recipient)) {
        return BACNET_STATUS_ABORT;
    }
    pRecipient = recipient;
    while (apdu_len < apdu_size) {
        /* device [0] BACnetObjectIdentifier CHOICE */
        len = bacnet_object_id_context_decode(
            &apdu[apdu_len], apdu_size - apdu_len, 0, &object_type,
            &object_instance);
        if (len > 0) {
            if (pRecipient) {
                pRecipient->tag = 0;
                pRecipient->type.device.type = object_type;
                pRecipient->type.device.instance = object_instance;
            }
            apdu_len += len;
        } else if (len < 0) {
            /* malformed */
            return BACNET_STATUS_ABORT;
        } else if (bacnet_is_opening_tag_number(
                       &apdu[apdu_len], apdu_size - apdu_len, 1, &len)) {
            apdu_len += len;
            if (pRecipient) {
                pRecipient->tag = 1;
            }
            /* network-number Unsigned16 */
            len = bacnet_unsigned_application_decode(
                &apdu[apdu_len], apdu_size - apdu_len, &unsigned_value);
            if (len < 0) {
                return BACNET_STATUS_ABORT;
            }
            apdu_len += len;
            if (unsigned_value > UINT16_MAX) {
                return BACNET_STATUS_ABORT;
            }
            network_number = (uint16_t)unsigned_value;
            if (pRecipient) {
                pRecipient->type.address.net = network_number;
            }
            /* mac-address OCTET STRING */
            if (pRecipient) {
                octet_string.buffer = pRecipient->type.address.mac;
                octet_string.buffer_size = sizeof(pRecipient->type.address.mac);
            } else {
                octet_string.buffer = NULL;
                octet_string.buffer_size = 0;
            }
            len = bacnet_octet_string_buffer_application_decode(
                &apdu[apdu_len], apdu_size - apdu_len, octet_string.buffer,
                octet_string.buffer_size);
            if (len < 0) {
                return BACNET_STATUS_ERROR;
            }
            apdu_len += len;
            octet_string.buffer_length = (uint32_t)len;
            if (pRecipient) {
                pRecipient->type.address.mac_len =
                    (uint8_t)octet_string.buffer_length;
            }
            if (octet_string.buffer_length == 0) {
                if (pRecipient) {
                    pRecipient->type.address.net = BACNET_BROADCAST_NETWORK;
                }
            } else if (network_number) {
                /* A network number indicates a remote network */
                if (pRecipient) {
                    bacnet_address_mac_to_adr(
                        &pRecipient->type.address, &pRecipient->type.address);
                    /* another process will need to fill the router MAC for
                       this network using Who-Is-Router-To-Network */
                    pRecipient->type.address.mac_len = 0;
                }
            } else {
                /* MAC local addresss: already decoded in place */
            }
            if (!bacnet_is_closing_tag_number(
                    &apdu[apdu_len], apdu_size - apdu_len, 1, &len)) {
                return BACNET_STATUS_ABORT;
            }
            apdu_len += len;
        } else {
            return BACNET_STATUS_ABORT;
        }
        if (pRecipient) {
            pRecipient = pRecipient->next;
        }
    }

    return apdu_len;
}
