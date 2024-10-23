/**
 * @file
 * @brief API for BACnet TimeSyncronization service and BACnetRecipientList
 *  encoder and decoder
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date 2012
 * @copyright SPDX-License-Identifier: MIT
 */
#ifndef BACNET_TIMESYNC_H
#define BACNET_TIMESYNC_H

#include <stdint.h>
#include <stdbool.h>
/* BACnet Stack defines - first */
#include "bacnet/bacdef.h"

struct BACnet_Recipient_List;
typedef struct BACnet_Recipient_List {
    /*
       BACnetRecipient ::= CHOICE {
       device [0] BACnetObjectIdentifier,
       address [1] BACnetAddress
       }
     */
    uint8_t tag;
    union {
        BACNET_OBJECT_ID device;
        BACNET_ADDRESS address;
    } type;
    /* simple linked list */
    struct BACnet_Recipient_List *next;
} BACNET_RECIPIENT_LIST;

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* encode service */
BACNET_STACK_EXPORT
int timesync_utc_encode_apdu(
    uint8_t *apdu, const BACNET_DATE *my_date, const BACNET_TIME *my_time);
BACNET_STACK_EXPORT
int timesync_encode_apdu(
    uint8_t *apdu, const BACNET_DATE *my_date, const BACNET_TIME *my_time);
BACNET_STACK_EXPORT
int timesync_encode_apdu_service(
    uint8_t *apdu,
    BACNET_UNCONFIRMED_SERVICE service,
    const BACNET_DATE *my_date,
    const BACNET_TIME *my_time);
/* decode the service request only */
BACNET_STACK_EXPORT
int timesync_decode_service_request(
    const uint8_t *apdu,
    unsigned apdu_len,
    BACNET_DATE *my_date,
    BACNET_TIME *my_time);
BACNET_STACK_EXPORT
int timesync_utc_decode_apdu(
    const uint8_t *apdu,
    unsigned apdu_len,
    BACNET_DATE *my_date,
    BACNET_TIME *my_time);
BACNET_STACK_EXPORT
int timesync_decode_apdu(
    const uint8_t *apdu,
    unsigned apdu_len,
    BACNET_DATE *my_date,
    BACNET_TIME *my_time);

BACNET_STACK_EXPORT
int timesync_encode_timesync_recipients(
    uint8_t *apdu, unsigned max_apdu, BACNET_RECIPIENT_LIST *recipient);
BACNET_STACK_EXPORT
int timesync_decode_timesync_recipients(
    const uint8_t *apdu, unsigned apdu_len, BACNET_RECIPIENT_LIST *recipient);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif
