/**
 * @file
 * @brief BACnetSpecialEvent complex data type encode and decode
 * @author Ondřej Hruška <ondra@ondrovo.com>
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date May 2022
 * @copyright SPDX-License-Identifier: GPL-2.0-or-later WITH GCC-exception-2.0
 */
#include <stdint.h>
#include "bacnet/special_event.h"
#include "bacnet/bacdcode.h"
#include "bacapp.h"

/**
 * @brief Decode a BACnetSpecialEvent complex data type
 *
 * BACnetSpecialEvent ::= SEQUENCE {
 *   period CHOICE {
 *     calendar-entry [0] BACnetCalendarEntry,
 *     calendar-reference [1] BACnetObjectIdentifier
 *   },
 *   list-of-time-values [2] SEQUENCE OF BACnetTimeValue,
 *   event-priority [3] Unsigned (1..16)
 * }
 *
 * @param apdu - the APDU buffer
 * @param apdu_size - the size of the APDU buffer
 * @param value - BACnetSpecialEvent structure, or NULL to only get the length
 * @return length of the APDU buffer, or BACNET_STATUS_ERROR if unable to decode
 */
int bacnet_special_event_decode(
    const uint8_t *apdu, int apdu_size, BACNET_SPECIAL_EVENT *value)
{
    int len = 0;
    int apdu_len = 0;
    BACNET_UNSIGNED_INTEGER priority = 0;
    BACNET_TAG tag = { 0 };
    BACNET_CALENDAR_ENTRY *calendar_entry = NULL;
    BACNET_OBJECT_TYPE *object_type = NULL;
    uint32_t *object_instance = NULL;
    BACNET_DAILY_SCHEDULE *time_values = NULL;

    if (!apdu) {
        return BACNET_STATUS_ERROR;
    }
    len = bacnet_tag_decode(&apdu[apdu_len], apdu_size - apdu_len, &tag);
    if (len <= 0) {
        return BACNET_STATUS_ERROR;
    }
    if (tag.opening &&
        (tag.number == BACNET_SPECIAL_EVENT_PERIOD_CALENDAR_ENTRY)) {
        if (value) {
            value->periodTag = BACNET_SPECIAL_EVENT_PERIOD_CALENDAR_ENTRY;
            calendar_entry = &value->period.calendarEntry;
        }
        len = bacnet_calendar_entry_context_decode(
            &apdu[apdu_len], apdu_size - apdu_len,
            BACNET_SPECIAL_EVENT_PERIOD_CALENDAR_ENTRY, calendar_entry);
        if (len < 0) {
            return BACNET_STATUS_ERROR;
        }
        apdu_len += len;
    } else if (
        tag.context &&
        (tag.number == BACNET_SPECIAL_EVENT_PERIOD_CALENDAR_REFERENCE)) {
        if (value) {
            value->periodTag = BACNET_SPECIAL_EVENT_PERIOD_CALENDAR_REFERENCE;
            object_type = &value->period.calendarReference.type;
            object_instance = &value->period.calendarReference.instance;
        }
        len = bacnet_object_id_context_decode(
            &apdu[apdu_len], apdu_size - apdu_len,
            BACNET_SPECIAL_EVENT_PERIOD_CALENDAR_REFERENCE, object_type,
            object_instance);
        if (len < 0) {
            return BACNET_STATUS_ERROR;
        }
        apdu_len += len;
    } else {
        return BACNET_STATUS_ERROR;
    }
    /* Values [2] */
    if (value) {
        time_values = &value->timeValues;
    }
    len = bacnet_dailyschedule_context_decode(
        &apdu[apdu_len], apdu_size - apdu_len, 2, time_values);
    if (len < 0) {
        return BACNET_STATUS_ERROR;
    }
    apdu_len += len;

    /* Priority [3] */
    len = bacnet_unsigned_context_decode(
        &apdu[apdu_len], apdu_size - apdu_len, 3, &priority);
    if (len < 0) {
        return BACNET_STATUS_ERROR;
    }
    if ((priority == 0) || (priority > BACNET_MAX_PRIORITY)) {
        return BACNET_STATUS_ERROR;
    }
    if (value) {
        value->priority = (uint8_t)priority;
    }
    apdu_len += len;

    return apdu_len;
}

/**
 * @brief Encode a BACnetSpecialEvent complex data type
 * @param apdu - the APDU buffer (NULL to determine the length)
 * @param value - BACnetSpecialEvent structure
 * @return length of the APDU buffer, or 0 if not able to encode
 */
int bacnet_special_event_encode(
    uint8_t *apdu, const BACNET_SPECIAL_EVENT *value)
{
    int apdu_len = 0;
    int len;

    if (value->periodTag == BACNET_SPECIAL_EVENT_PERIOD_CALENDAR_ENTRY) {
        len = bacnet_calendar_entry_context_encode(
            apdu, BACNET_SPECIAL_EVENT_PERIOD_CALENDAR_ENTRY,
            &value->period.calendarEntry);
        if (len < 0) {
            return -1;
        }
        apdu_len += len;
        if (apdu) {
            apdu += len;
        }
    } else {
        len = encode_context_object_id(
            apdu, BACNET_SPECIAL_EVENT_PERIOD_CALENDAR_REFERENCE,
            value->period.calendarReference.type,
            value->period.calendarReference.instance);
        if (len < 0) {
            return -1;
        }
        apdu_len += len;
        if (apdu) {
            apdu += len;
        }
    }

    len = bacnet_dailyschedule_context_encode(apdu, 2, &value->timeValues);
    if (len < 0) {
        return -1;
    }
    apdu_len += len;
    if (apdu) {
        apdu += len;
    }

    len = encode_context_unsigned(apdu, 3, value->priority);
    if (len < 0) {
        return -1;
    }
    apdu_len += len;

    return apdu_len;
}

/**
 * @brief Encode a context tagged BACnetSpecialEvent complex data type
 * @param apdu - the APDU buffer (NULL to determine the length)
 * @param tag_number - tag number to context encode
 * @param value - BACnetSpecialEvent structure
 * @return length of the APDU buffer, or 0 if not able to encode
 */
int bacnet_special_event_context_encode(
    uint8_t *apdu, uint8_t tag_number, const BACNET_SPECIAL_EVENT *value)
{
    int len = 0;
    int apdu_len = 0;

    if (value) {
        len = encode_opening_tag(apdu, tag_number);
        apdu_len += len;
        if (apdu) {
            apdu += len;
        }
        len = bacnet_special_event_encode(apdu, value);
        apdu_len += len;
        if (apdu) {
            apdu += len;
        }
        len = encode_closing_tag(apdu, tag_number);
        apdu_len += len;
    }

    return apdu_len;
}

/**
 * @brief Decode a context tagged BACnetSpecialEvent complex data type
 * @param apdu - the APDU buffer
 * @param apdu_size - the size of the APDU buffer
 * @param tag_number - tag number to context decode
 * @param value - BACnetSpecialEvent structure
 * @return length of the APDU buffer, or BACNET_STATUS_ERROR if unable to decode
 */
int bacnet_special_event_context_decode(
    const uint8_t *apdu,
    int apdu_size,
    uint8_t tag_number,
    BACNET_SPECIAL_EVENT *value)
{
    int apdu_len = 0;
    int len = 0;

    if (bacnet_is_opening_tag_number(
            &apdu[apdu_len], apdu_size - apdu_len, tag_number, &len)) {
        apdu_len += len;
    } else {
        return BACNET_STATUS_ERROR;
    }
    len = bacnet_special_event_decode(
        &apdu[apdu_len], apdu_size - apdu_len, value);
    if (len < 0) {
        return BACNET_STATUS_ERROR;
    } else {
        apdu_len += len;
    }
    if (bacnet_is_closing_tag_number(
            &apdu[apdu_len], apdu_size - apdu_len, tag_number, &len)) {
        apdu_len += len;
    } else {
        return BACNET_STATUS_ERROR;
    }

    return apdu_len;
}

/**
 * @brief Compare the BACnetSpecialEvent complex data
 * @param value1 - BACNET_SPECIAL_EVENT structure
 * @param value2 - BACNET_SPECIAL_EVENT structure
 * @return true if the same
 */
bool bacnet_special_event_same(
    const BACNET_SPECIAL_EVENT *value1, const BACNET_SPECIAL_EVENT *value2)
{
    int ti;

    if (value1->periodTag != value2->periodTag ||
        value1->priority != value2->priority) {
        return false;
    }

    if (value1->periodTag == BACNET_SPECIAL_EVENT_PERIOD_CALENDAR_ENTRY) {
        if (!bacnet_calendar_entry_same(
                &value1->period.calendarEntry, &value2->period.calendarEntry)) {
            return false;
        }
    }

    if (value1->timeValues.TV_Count != value2->timeValues.TV_Count) {
        return false;
    }
    for (ti = 0; ti < value1->timeValues.TV_Count; ti++) {
        if (!bacnet_time_value_same(
                &value1->timeValues.Time_Values[ti],
                &value2->timeValues.Time_Values[ti])) {
            return false;
        }
    }

    return true;
}

/**
 * @brief Compare the BACnetSpecialEvent complex data
 * @param value1 - BACNET_SPECIAL_EVENT structure
 * @param value2 - BACNET_SPECIAL_EVENT structure
 * @return true if the same
 */
bool bacnet_special_event_copy(
    BACNET_SPECIAL_EVENT *dest, const BACNET_SPECIAL_EVENT *src)
{
    if (!dest || !src) {
        return false;
    }

    memcpy(dest, src, sizeof(BACNET_SPECIAL_EVENT));

    return true;
}

/**
 * @brief Encode a linked-list BACnetSpecialEvent complex data type
 * @param apdu - the APDU buffer (NULL to determine the length)
 * @param value - BACnetSpecialEvent structure
 * @return length of the APDU buffer, or BACNET_STATUS_ERROR if not able to
 *  encode
 */
int bacnet_special_event_entry_encode(
    uint8_t *apdu, const BACNET_SPECIAL_EVENT_ENTRY *value)
{
    int apdu_len = 0;
    int len;

    if (!value) {
        return BACNET_STATUS_ERROR;
    }
    if (value->periodTag == BACNET_SPECIAL_EVENT_PERIOD_CALENDAR_ENTRY) {
        len = bacnet_calendar_entry_context_encode(
            apdu, BACNET_SPECIAL_EVENT_PERIOD_CALENDAR_ENTRY,
            &value->period.calendarEntry);
        if (len < 0) {
            return BACNET_STATUS_ERROR;
        }
        apdu_len += len;
        if (apdu) {
            apdu += len;
        }
    } else {
        len = encode_context_object_id(
            apdu, BACNET_SPECIAL_EVENT_PERIOD_CALENDAR_REFERENCE,
            value->period.calendarReference.type,
            value->period.calendarReference.instance);
        if (len < 0) {
            return BACNET_STATUS_ERROR;
        }
        apdu_len += len;
        if (apdu) {
            apdu += len;
        }
    }

    len = bacnet_dailyschedule_list_context_encode(apdu, 2, value->timeValues);
    if (len < 0) {
        return BACNET_STATUS_ERROR;
    }
    apdu_len += len;
    if (apdu) {
        apdu += len;
    }

    len = encode_context_unsigned(apdu, 3, value->priority);
    if (len < 0) {
        return BACNET_STATUS_ERROR;
    }
    apdu_len += len;

    return apdu_len;
}

/**
 * @brief Encode a context tagged linked-list BACnetSpecialEvent
 * @param apdu - the APDU buffer (NULL to determine the length)
 * @param tag_number - tag number to context encode
 * @param value - BACnetSpecialEvent structure
 * @return length of the APDU buffer, or BACNET_STATUS_ERROR if not able to
 *  encode
 */
int bacnet_special_event_entry_context_encode(
    uint8_t *apdu, uint8_t tag_number, const BACNET_SPECIAL_EVENT_ENTRY *value)
{
    int len = 0;
    int apdu_len = 0;

    if (!value) {
        return BACNET_STATUS_ERROR;
    }
    len = encode_opening_tag(apdu, tag_number);
    apdu_len += len;
    if (apdu) {
        apdu += len;
    }
    len = bacnet_special_event_entry_encode(apdu, value);
    if (len < 0) {
        return BACNET_STATUS_ERROR;
    }
    apdu_len += len;
    if (apdu) {
        apdu += len;
    }
    len = encode_closing_tag(apdu, tag_number);
    apdu_len += len;

    return apdu_len;
}

/**
 * @brief Decode a linked-list BACnetSpecialEvent complex data type
 * @param apdu - the APDU buffer
 * @param apdu_size - the size of the APDU buffer
 * @param value - periodTag/period/priority destination, or NULL to only
 *  get the length
 * @param store_fn - called per decoded time-value; return false to abort
 * @param ctx - caller context passed to store_fn
 * @return length of the APDU buffer, or BACNET_STATUS_ERROR if unable to
 *  decode
 */
int bacnet_special_event_entry_decode(
    const uint8_t *apdu,
    int apdu_size,
    BACNET_SPECIAL_EVENT_ENTRY *value,
    bacnet_dailyschedule_entry_store_fn store_fn,
    void *ctx)
{
    int len = 0;
    int apdu_len = 0;
    BACNET_UNSIGNED_INTEGER priority = 0;
    BACNET_TAG tag = { 0 };
    BACNET_CALENDAR_ENTRY *calendar_entry = NULL;
    BACNET_OBJECT_TYPE *object_type = NULL;
    uint32_t *object_instance = NULL;

    if (!apdu) {
        return BACNET_STATUS_ERROR;
    }
    len = bacnet_tag_decode(&apdu[apdu_len], apdu_size - apdu_len, &tag);
    if (len <= 0) {
        return BACNET_STATUS_ERROR;
    }
    if (tag.opening &&
        (tag.number == BACNET_SPECIAL_EVENT_PERIOD_CALENDAR_ENTRY)) {
        if (value) {
            value->periodTag = BACNET_SPECIAL_EVENT_PERIOD_CALENDAR_ENTRY;
            calendar_entry = &value->period.calendarEntry;
        }
        len = bacnet_calendar_entry_context_decode(
            &apdu[apdu_len], apdu_size - apdu_len,
            BACNET_SPECIAL_EVENT_PERIOD_CALENDAR_ENTRY, calendar_entry);
        if (len < 0) {
            return BACNET_STATUS_ERROR;
        }
        apdu_len += len;
    } else if (
        tag.context &&
        (tag.number == BACNET_SPECIAL_EVENT_PERIOD_CALENDAR_REFERENCE)) {
        if (value) {
            value->periodTag = BACNET_SPECIAL_EVENT_PERIOD_CALENDAR_REFERENCE;
            object_type = &value->period.calendarReference.type;
            object_instance = &value->period.calendarReference.instance;
        }
        len = bacnet_object_id_context_decode(
            &apdu[apdu_len], apdu_size - apdu_len,
            BACNET_SPECIAL_EVENT_PERIOD_CALENDAR_REFERENCE, object_type,
            object_instance);
        if (len < 0) {
            return BACNET_STATUS_ERROR;
        }
        apdu_len += len;
    } else {
        return BACNET_STATUS_ERROR;
    }
    /* Values [2] - built by the caller via store_fn, since this module
       does not allocate list nodes */
    len = bacnet_dailyschedule_list_context_decode(
        &apdu[apdu_len], apdu_size - apdu_len, 2, store_fn, ctx);
    if (len < 0) {
        return BACNET_STATUS_ERROR;
    }
    apdu_len += len;
    if (value) {
        value->timeValues = NULL;
    }

    /* Priority [3] */
    len = bacnet_unsigned_context_decode(
        &apdu[apdu_len], apdu_size - apdu_len, 3, &priority);
    if (len < 0) {
        return BACNET_STATUS_ERROR;
    }
    if ((priority == 0) || (priority > BACNET_MAX_PRIORITY)) {
        return BACNET_STATUS_ERROR;
    }
    if (value) {
        value->priority = (uint8_t)priority;
    }
    apdu_len += len;

    return apdu_len;
}

/**
 * @brief Decode a context tagged linked-list BACnetSpecialEvent
 * @param apdu - the APDU buffer
 * @param apdu_size - the size of the APDU buffer
 * @param tag_number - tag number to context decode
 * @param value - periodTag/period/priority destination, or NULL to only
 *  get the length
 * @param store_fn - called per decoded time-value; return false to abort
 * @param ctx - caller context passed to store_fn
 * @return length of the APDU buffer, or BACNET_STATUS_ERROR if unable to
 *  decode
 */
int bacnet_special_event_entry_context_decode(
    const uint8_t *apdu,
    int apdu_size,
    uint8_t tag_number,
    BACNET_SPECIAL_EVENT_ENTRY *value,
    bacnet_dailyschedule_entry_store_fn store_fn,
    void *ctx)
{
    int apdu_len = 0;
    int len = 0;

    if (bacnet_is_opening_tag_number(
            &apdu[apdu_len], apdu_size - apdu_len, tag_number, &len)) {
        apdu_len += len;
    } else {
        return BACNET_STATUS_ERROR;
    }
    len = bacnet_special_event_entry_decode(
        &apdu[apdu_len], apdu_size - apdu_len, value, store_fn, ctx);
    if (len < 0) {
        return BACNET_STATUS_ERROR;
    }
    apdu_len += len;
    if (bacnet_is_closing_tag_number(
            &apdu[apdu_len], apdu_size - apdu_len, tag_number, &len)) {
        apdu_len += len;
    } else {
        return BACNET_STATUS_ERROR;
    }

    return apdu_len;
}

/**
 * @brief Compare two linked-list BACnetSpecialEvent values
 * @param value1 - BACNET_SPECIAL_EVENT_ENTRY structure
 * @param value2 - BACNET_SPECIAL_EVENT_ENTRY structure
 * @return true if the same
 */
bool bacnet_special_event_entry_same(
    const BACNET_SPECIAL_EVENT_ENTRY *value1,
    const BACNET_SPECIAL_EVENT_ENTRY *value2)
{
    if (!value1 || !value2) {
        return false;
    }
    if ((value1->periodTag != value2->periodTag) ||
        (value1->priority != value2->priority)) {
        return false;
    }
    if (value1->periodTag == BACNET_SPECIAL_EVENT_PERIOD_CALENDAR_ENTRY) {
        if (!bacnet_calendar_entry_same(
                &value1->period.calendarEntry, &value2->period.calendarEntry)) {
            return false;
        }
    } else if (
        (value1->period.calendarReference.type !=
         value2->period.calendarReference.type) ||
        (value1->period.calendarReference.instance !=
         value2->period.calendarReference.instance)) {
        return false;
    }

    return bacnet_dailyschedule_list_same(
        value1->timeValues, value2->timeValues);
}
