/**
 * @file
 * @brief API for BACnetDestination complex data type encode and decode
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date May 2022
 * @section LICENSE
 *
 * Copyright (C) 2022 Steve Karg <skarg@users.sourceforge.net>
 *
 * SPDX-License-Identifier: MIT
 */
#ifndef BACNET_DESTINATION_H
#define BACNET_DESTINATION_H

#include <stdint.h>
#include <stdbool.h>
#include "bacnet/bacnet_stack_exports.h"
#include "bacnet/bacenum.h"
#include "bacnet/bacstr.h"
#include "bacnet/datetime.h"

typedef enum BACnet_Recipient_Tag {
    BACNET_RECIPIENT_TAG_DEVICE = 0,
    BACNET_RECIPIENT_TAG_ADDRESS = 1,
    BACNET_RECIPIENT_TAG_MAX = 2
} BACNET_RECIPIENT_TAG;

typedef struct BACnet_Recipient {
    uint8_t tag;
    /**
     *  BACnetRecipient ::= CHOICE {
     *      device [0] BACnetObjectIdentifier,
     *      address [1] BACnetAddress
     *  }
     */
    union {
        BACNET_OBJECT_ID device;
        BACNET_ADDRESS address;
    } type;
} BACNET_RECIPIENT;

typedef struct BACnet_Destination {
    /**
     *  BACnetDestination ::= SEQUENCE {
     *      valid-days                      BACnetDaysOfWeek,
     *      from-time                       Time,
     *      to-time                         Time,
     *      recipient                       BACnetRecipient,
     *      process-identifier              Unsigned32,
     *      issue-confirmed-notifications   BOOLEAN,
     *      transitions                     BACnetEventTransitionBits
     *  }
     */
    BACNET_BIT_STRING ValidDays;
    BACNET_TIME FromTime;
    BACNET_TIME ToTime;
    BACNET_RECIPIENT Recipient;
    uint32_t ProcessIdentifier;
    bool ConfirmedNotify;
    BACNET_BIT_STRING Transitions;
} BACNET_DESTINATION;

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

BACNET_STACK_EXPORT
int bacnet_destination_encode(uint8_t *apdu, BACNET_DESTINATION *destination);
BACNET_STACK_EXPORT
int bacnet_destination_context_encode(
    uint8_t *apdu, uint8_t tag_number, BACNET_DESTINATION *destination);
BACNET_STACK_EXPORT
int bacnet_destination_decode(
    uint8_t *apdu, int apdu_len, BACNET_DESTINATION *destination);
BACNET_STACK_EXPORT
void bacnet_destination_default_init(BACNET_DESTINATION *destination);
BACNET_STACK_EXPORT
bool bacnet_destination_default(BACNET_DESTINATION *destination);
BACNET_STACK_EXPORT
bool bacnet_destination_same(
    BACNET_DESTINATION *dest1, BACNET_DESTINATION *dest2);
BACNET_STACK_EXPORT
void bacnet_destination_copy(BACNET_DESTINATION *dest, BACNET_DESTINATION *src);

BACNET_STACK_EXPORT
void bacnet_recipient_copy(BACNET_RECIPIENT *dest, BACNET_RECIPIENT *src);
BACNET_STACK_EXPORT
bool bacnet_recipient_same(BACNET_RECIPIENT *r1, BACNET_RECIPIENT *r2);
BACNET_STACK_EXPORT
bool bacnet_recipient_device_wildcard(BACNET_RECIPIENT *recipient);
BACNET_STACK_EXPORT
bool bacnet_recipient_device_valid(BACNET_RECIPIENT *recipient);

BACNET_STACK_EXPORT
int bacnet_destination_to_ascii(
    const BACNET_DESTINATION *bacdest, char *buf, size_t buf_size);
BACNET_STACK_EXPORT
bool bacnet_destination_from_ascii(
    BACNET_DESTINATION *bacdest, const char *buf);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif
