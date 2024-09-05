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
 * @param value - BACnetSpecialEvent structure
 * @return length of the APDU buffer, or BACNET_STATUS_ERROR if unable to decode
 */
int bacnet_special_event_decode(
    const uint8_t *apdu, int apdu_size, BACNET_SPECIAL_EVENT *value)
{
    int len = 0;
    int apdu_len = 0;
    BACNET_UNSIGNED_INTEGER priority = 0;
    BACNET_TAG tag = { 0 };

    if (!apdu || !value) {
        return BACNET_STATUS_ERROR;
    }
    len = bacnet_tag_decode(&apdu[apdu_len], apdu_size - apdu_len, &tag);
    if (len <= 0) {
        return BACNET_STATUS_ERROR;
    }
    if (tag.opening &&
        (tag.number == BACNET_SPECIAL_EVENT_PERIOD_CALENDAR_ENTRY)) {
        value->periodTag = BACNET_SPECIAL_EVENT_PERIOD_CALENDAR_ENTRY;
        len = bacnet_calendar_entry_context_decode(
            &apdu[apdu_len], apdu_size - apdu_len,
            BACNET_SPECIAL_EVENT_PERIOD_CALENDAR_ENTRY,
            &value->period.calendarEntry);
        if (len < 0) {
            return BACNET_STATUS_ERROR;
        }
        apdu_len += len;
    } else if (
        tag.context &&
        (tag.number == BACNET_SPECIAL_EVENT_PERIOD_CALENDAR_REFERENCE)) {
        value->periodTag = BACNET_SPECIAL_EVENT_PERIOD_CALENDAR_REFERENCE;
        len = bacnet_object_id_context_decode(
            &apdu[apdu_len], apdu_size - apdu_len,
            BACNET_SPECIAL_EVENT_PERIOD_CALENDAR_REFERENCE,
            &value->period.calendarReference.type,
            &value->period.calendarReference.instance);
        if (len < 0) {
            return BACNET_STATUS_ERROR;
        }
        apdu_len += len;
    } else {
        return BACNET_STATUS_ERROR;
    }
    /* Values [2] */
    len = bacnet_dailyschedule_context_decode(
        &apdu[apdu_len], apdu_size - apdu_len, 2, &value->timeValues);
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
    if (priority > BACNET_MAX_PRIORITY) {
        return BACNET_STATUS_ERROR;
    }
    value->priority = (uint8_t)priority;
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
    BACNET_APPLICATION_DATA_VALUE adv1, adv2;
    const BACNET_TIME_VALUE *tv1, *tv2;
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

    /* TODO extract shared code with `bacnet_weeklyschedule_same` */
    if (value1->timeValues.TV_Count != value2->timeValues.TV_Count) {
        return false;
    }
    for (ti = 0; ti < value1->timeValues.TV_Count; ti++) {
        tv1 = &value1->timeValues.Time_Values[ti];
        tv2 = &value2->timeValues.Time_Values[ti];
        if (0 != datetime_compare_time(&tv1->Time, &tv2->Time)) {
            return false;
        }

        bacnet_primitive_to_application_data_value(&adv1, &tv1->Value);
        bacnet_primitive_to_application_data_value(&adv2, &tv2->Value);

        if (!bacapp_same_value(&adv1, &adv2)) {
            return false;
        }
    }

    return true;
}
