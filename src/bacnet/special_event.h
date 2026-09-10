/**
 * @file
 * @brief API for BACnetSpecialEvent complex data type encode and decode
 * @author Ondřej Hruška <ondra@ondrovo.com>
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date August 2023
 * @copyright SPDX-License-Identifier: MIT
 */
#ifndef BACNET_SPECIAL_EVENT_H
#define BACNET_SPECIAL_EVENT_H

#include <stdint.h>
#include <stdbool.h>
/* BACnet Stack defines - first */
#include "bacnet/bacdef.h"
/* BACnet Stack API */
#include "bacnet/bactimevalue.h"
#include "bacnet/calendar_entry.h"
#include "bacnet/dailyschedule.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

typedef enum BACnet_SpecialEventPeriod_Tags {
    BACNET_SPECIAL_EVENT_PERIOD_CALENDAR_ENTRY = 0,
    BACNET_SPECIAL_EVENT_PERIOD_CALENDAR_REFERENCE = 1
} BACNET_SPECIAL_EVENT_PERIOD_TAG;

typedef struct BACnet_Special_Event {
    BACNET_SPECIAL_EVENT_PERIOD_TAG periodTag;
    union {
        BACNET_CALENDAR_ENTRY calendarEntry;
        BACNET_OBJECT_ID calendarReference;
    } period;
    /* We reuse the daily schedule struct and its encoding/decoding - it's
     * identical */
    BACNET_DAILY_SCHEDULE timeValues;
    uint8_t priority;
} BACNET_SPECIAL_EVENT;

/** Node of a dynamically-sized (linked list) BACnetSpecialEvent, for use
 *  when the number of list-of-time-values entries is not known ahead of
 *  time or changes at runtime. Memory management of the list-of-time-values
 *  is up to the caller; this module only encodes and decodes it. */
typedef struct BACnet_Special_Event_Entry {
    BACNET_SPECIAL_EVENT_PERIOD_TAG periodTag;
    union {
        BACNET_CALENDAR_ENTRY calendarEntry;
        BACNET_OBJECT_ID calendarReference;
    } period;
    /* head of a linked list of BACnetTimeValue entries; NULL is an empty
     * list */
    BACNET_DAILY_SCHEDULE_ENTRY *timeValues;
    uint8_t priority;
} BACNET_SPECIAL_EVENT_ENTRY;

/** Decode Special Event */
BACNET_STACK_EXPORT
int bacnet_special_event_decode(
    const uint8_t *apdu, int max_apdu_len, BACNET_SPECIAL_EVENT *value);

/** Encode Special Event */
BACNET_STACK_EXPORT
int bacnet_special_event_encode(
    uint8_t *apdu, const BACNET_SPECIAL_EVENT *value);

BACNET_STACK_EXPORT
int bacnet_special_event_context_encode(
    uint8_t *apdu, uint8_t tag_number, const BACNET_SPECIAL_EVENT *value);

BACNET_STACK_EXPORT
int bacnet_special_event_context_decode(
    const uint8_t *apdu,
    int max_apdu_len,
    uint8_t tag_number,
    BACNET_SPECIAL_EVENT *value);

BACNET_STACK_EXPORT
bool bacnet_special_event_same(
    const BACNET_SPECIAL_EVENT *value1, const BACNET_SPECIAL_EVENT *value2);

BACNET_STACK_EXPORT
bool bacnet_special_event_copy(
    BACNET_SPECIAL_EVENT *dest, const BACNET_SPECIAL_EVENT *src);

/** Encode a linked-list BACnetSpecialEvent */
BACNET_STACK_EXPORT
int bacnet_special_event_entry_encode(
    uint8_t *apdu, const BACNET_SPECIAL_EVENT_ENTRY *value);

/** Encode a context tagged linked-list BACnetSpecialEvent */
BACNET_STACK_EXPORT
int bacnet_special_event_entry_context_encode(
    uint8_t *apdu, uint8_t tag_number, const BACNET_SPECIAL_EVENT_ENTRY *value);

/** Decode a linked-list BACnetSpecialEvent. The period and priority are
 *  decoded into value, while the list-of-time-values is decoded by calling
 *  store_fn once per entry so the caller can build its own linked list.
 *  @param apdu [in] Buffer of data to be decoded.
 *  @param apdu_size [in] Number of bytes in the buffer.
 *  @param value [out] periodTag/period/priority destination, or NULL to
 *   only get the length.
 *  @param store_fn [in] Called per decoded time-value; return false to
 *   abort.
 *  @param ctx [in] Caller context passed to store_fn.
 *  @return Number of bytes decoded, or BACNET_STATUS_ERROR on failure. */
BACNET_STACK_EXPORT
int bacnet_special_event_entry_decode(
    const uint8_t *apdu,
    int apdu_size,
    BACNET_SPECIAL_EVENT_ENTRY *value,
    bacnet_dailyschedule_entry_store_fn store_fn,
    void *ctx);

/** Decode a context tagged linked-list BACnetSpecialEvent; see
 *  bacnet_special_event_entry_decode() for the store_fn/ctx behavior. */
BACNET_STACK_EXPORT
int bacnet_special_event_entry_context_decode(
    const uint8_t *apdu,
    int apdu_size,
    uint8_t tag_number,
    BACNET_SPECIAL_EVENT_ENTRY *value,
    bacnet_dailyschedule_entry_store_fn store_fn,
    void *ctx);

BACNET_STACK_EXPORT
bool bacnet_special_event_entry_same(
    const BACNET_SPECIAL_EVENT_ENTRY *value1,
    const BACNET_SPECIAL_EVENT_ENTRY *value2);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* BACNET_SPECIAL_EVENT_H */
