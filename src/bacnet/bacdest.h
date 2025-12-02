/**
 * @file
 * @brief API for BACnetDestination complex data type encode and decode
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date May 2022
 * @copyright SPDX-License-Identifier: MIT
 */
#ifndef BACNET_DESTINATION_H
#define BACNET_DESTINATION_H

#include <stdint.h>
#include <stdbool.h>
/* BACnet Stack defines - first */
#include "bacnet/bacdef.h"
/* BACnet Stack API */
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
int bacnet_destination_encode(
    uint8_t *apdu, const BACNET_DESTINATION *destination);
BACNET_STACK_EXPORT
int bacnet_destination_context_encode(
    uint8_t *apdu, uint8_t tag_number, const BACNET_DESTINATION *destination);
BACNET_STACK_EXPORT
int bacnet_destination_decode(
    const uint8_t *apdu, int apdu_len, BACNET_DESTINATION *destination);
BACNET_STACK_EXPORT
int bacnet_destination_context_decode(
    const uint8_t *apdu,
    uint32_t apdu_size,
    uint8_t tag_number,
    BACNET_DESTINATION *value);
BACNET_STACK_EXPORT
void bacnet_destination_default_init(BACNET_DESTINATION *destination);
BACNET_STACK_EXPORT
bool bacnet_destination_default(const BACNET_DESTINATION *destination);
BACNET_STACK_EXPORT
bool bacnet_destination_same(
    const BACNET_DESTINATION *dest1, const BACNET_DESTINATION *dest2);
BACNET_STACK_EXPORT
void bacnet_destination_copy(
    BACNET_DESTINATION *dest, const BACNET_DESTINATION *src);

BACNET_STACK_EXPORT
void bacnet_recipient_device_set(
    BACNET_RECIPIENT *dest, BACNET_OBJECT_TYPE object_type, uint32_t instance);
BACNET_STACK_EXPORT
void bacnet_recipient_address_set(
    BACNET_RECIPIENT *dest, const BACNET_ADDRESS *address);
BACNET_STACK_EXPORT
void bacnet_recipient_copy(BACNET_RECIPIENT *dest, const BACNET_RECIPIENT *src);
BACNET_STACK_EXPORT
bool bacnet_recipient_same(
    const BACNET_RECIPIENT *r1, const BACNET_RECIPIENT *r2);

BACNET_STACK_EXPORT
void bacnet_recipient_device_wildcard_set(BACNET_RECIPIENT *recipient);
BACNET_STACK_EXPORT
bool bacnet_recipient_device_wildcard(const BACNET_RECIPIENT *recipient);
BACNET_STACK_EXPORT
bool bacnet_recipient_device_valid(const BACNET_RECIPIENT *recipient);

BACNET_STACK_EXPORT
int bacnet_recipient_encode(uint8_t *apdu, const BACNET_RECIPIENT *recipient);
BACNET_STACK_EXPORT
int bacnet_recipient_context_encode(
    uint8_t *apdu, uint8_t tag_number, const BACNET_RECIPIENT *recipient);
BACNET_STACK_EXPORT
int bacnet_recipient_decode(
    const uint8_t *apdu, int apdu_size, BACNET_RECIPIENT *recipient);
BACNET_STACK_EXPORT
int bacnet_recipient_context_decode(
    const uint8_t *apdu,
    uint32_t apdu_size,
    uint8_t tag_number,
    BACNET_RECIPIENT *value);

BACNET_STACK_EXPORT
bool bacnet_recipient_address_from_ascii(BACNET_ADDRESS *src, const char *arg);
BACNET_STACK_EXPORT
bool bacnet_recipient_from_ascii(BACNET_RECIPIENT *value_out, const char *str);
BACNET_STACK_EXPORT
int bacnet_recipient_to_ascii(
    const BACNET_RECIPIENT *value, char *buf, size_t buf_size);

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
