/**
 * @file
 * @brief BACnetDailySchedule encode and decode functions
 * @author Nikola Jelic <nikola.jelic@euroicc.com>
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date 2015
 * @copyright SPDX-License-Identifier: MIT
 */
#ifndef BACNET_DAILY_SCHEDULE_H
#define BACNET_DAILY_SCHEDULE_H

#include <stdint.h>
#include <stdbool.h>
/* BACnet Stack defines - first */
#include "bacnet/bacdef.h"
/* BACnet Stack API */
#include "bacnet/bactimevalue.h"

/* arbitrary value, shall be unlimited for B-OWS but we don't care, 640k shall
 * be enough */
/* however we try not to boost the bacnet application value structure size,  */
/* so 7 x (this value) x sizeof(BACNET_TIME_VALUE) fits. */
#ifndef BACNET_DAILY_SCHEDULE_TIME_VALUES_SIZE
#define BACNET_DAILY_SCHEDULE_TIME_VALUES_SIZE 40
#endif

/*
    BACnetDailySchedule ::= SEQUENCE {
        day-schedule [0] SEQUENCE OF BACnetTimeValue
    }
*/

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

typedef struct BACnet_Daily_Schedule {
    BACNET_TIME_VALUE Time_Values[BACNET_DAILY_SCHEDULE_TIME_VALUES_SIZE];
    uint16_t TV_Count; /* the number of time values actually used */
} BACNET_DAILY_SCHEDULE;

/** Node of a dynamically-sized (linked list) BACnetDailySchedule, for use
 *  when the number of day-schedule time-values is not known ahead of time
 *  or changes at runtime. Memory management of the list is up to the
 *  caller; this module only encodes and decodes the list. */
typedef struct BACnet_Daily_Schedule_Entry {
    BACNET_TIME_VALUE Time_Value;
    struct BACnet_Daily_Schedule_Entry *next;
} BACNET_DAILY_SCHEDULE_ENTRY;

/** Decode DailySchedule (sequence of times and values) */
BACNET_STACK_EXPORT
int bacnet_dailyschedule_context_decode(
    const uint8_t *apdu,
    int max_apdu_len,
    uint8_t tag_number,
    BACNET_DAILY_SCHEDULE *day);

/** Encode DailySchedule (sequence of times and values) */
BACNET_STACK_EXPORT
int bacnet_dailyschedule_context_encode(
    uint8_t *apdu, uint8_t tag_number, const BACNET_DAILY_SCHEDULE *day);

BACNET_STACK_EXPORT
bool bacnet_dailyschedule_same(
    const BACNET_DAILY_SCHEDULE *a, const BACNET_DAILY_SCHEDULE *b);

BACNET_STACK_EXPORT
void bacnet_dailyschedule_copy(
    BACNET_DAILY_SCHEDULE *dest, const BACNET_DAILY_SCHEDULE *src);

/** Encode a linked-list BACnetDailySchedule (day-schedule SEQUENCE OF
 *  BACnetTimeValue) wrapped in an opening/closing context tag pair.
 *  @param apdu [out] Buffer to encode to, or NULL for length-only.
 *  @param tag_number [in] Context tag number to use.
 *  @param head [in] Head of linked list; NULL encodes an empty list.
 *  @return Number of bytes encoded, or BACNET_STATUS_ERROR on failure. */
BACNET_STACK_EXPORT
int bacnet_dailyschedule_list_context_encode(
    uint8_t *apdu, uint8_t tag_number, const BACNET_DAILY_SCHEDULE_ENTRY *head);

/** Callback invoked by bacnet_dailyschedule_list_context_decode() for each
 *  decoded BACnetTimeValue entry. Return false to abort decoding. */
typedef bool (*bacnet_dailyschedule_entry_store_fn)(
    const BACNET_TIME_VALUE *time_value, void *ctx);

/** Decode a linked-list BACnetDailySchedule (day-schedule SEQUENCE OF
 *  BACnetTimeValue) wrapped in an opening/closing context tag pair,
 *  calling store_fn once per decoded time-value so the caller can build
 *  its own linked list.
 *  @param apdu [in] Buffer of data to be decoded.
 *  @param apdu_size [in] Number of bytes in the buffer.
 *  @param tag_number [in] Context tag number to match.
 *  @param store_fn [in] Called per decoded entry; return false to abort.
 *  @param ctx [in] Caller context passed to store_fn.
 *  @return Number of bytes decoded, or BACNET_STATUS_ERROR on failure. */
BACNET_STACK_EXPORT
int bacnet_dailyschedule_list_context_decode(
    const uint8_t *apdu,
    int apdu_size,
    uint8_t tag_number,
    bacnet_dailyschedule_entry_store_fn store_fn,
    void *ctx);

BACNET_STACK_EXPORT
bool bacnet_dailyschedule_list_same(
    const BACNET_DAILY_SCHEDULE_ENTRY *a, const BACNET_DAILY_SCHEDULE_ENTRY *b);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* DAILYSCHEDULE_H */
