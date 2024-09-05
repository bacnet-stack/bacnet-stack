/**
 * @file
 * @brief BACnetCalendarEntry complex data type encode and decode
 * @author Ondřej Hruška <ondra@ondrovo.com>
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date May 2022
 * @copyright SPDX-License-Identifier: GPL-2.0-or-later WITH GCC-exception-2.0
 */
#include <stdint.h>
#include "bacnet/calendar_entry.h"
#include "bacnet/bacapp.h"
#include "bacnet/bacdcode.h"
#include "bacnet/bactimevalue.h"
#include "bacnet/datetime.h"
#include "bacnet/basic/sys/days.h"

/*
 * @brief Encode the BACnetCalendarEntry complex data
 *
 * BACnetCalendarEntry ::= CHOICE {
 *     date [0] Date,
 *     date-range [1] BACnetDateRange,
 *     weekNDay [2] BACnetWeekNDay
 * }
 *
 * @param apdu  Pointer to the buffer for encoding, or NULL for only length
 * @param value Pointer to the property data to be encoded.
 * @return bytes encoded or zero on error.
 */
int bacnet_calendar_entry_encode(
    uint8_t *apdu, const BACNET_CALENDAR_ENTRY *value)
{
    int len = 0;
    int apdu_len = 0;
    BACNET_OCTET_STRING octetstring = { 0 };

    switch (value->tag) {
        case BACNET_CALENDAR_DATE:
            len = encode_context_date(apdu, value->tag, &value->type.Date);
            apdu_len += len;
            break;
        case BACNET_CALENDAR_DATE_RANGE:
            len = bacnet_daterange_context_encode(
                apdu, value->tag, &value->type.DateRange);
            apdu_len += len;
            break;
        case BACNET_CALENDAR_WEEK_N_DAY:
            octetstring.value[0] = value->type.WeekNDay.month;
            octetstring.value[1] = value->type.WeekNDay.weekofmonth;
            octetstring.value[2] = value->type.WeekNDay.dayofweek;
            octetstring.length = 3;
            len = encode_context_octet_string(apdu, value->tag, &octetstring);
            apdu_len += len;
            break;
        default:
            /* do nothing */
            break;
    }

    return apdu_len;
}

/**
 * @brief Encodes into bytes from the calendar-entry structure
 *  a context tagged chunk (opening and closing tag)
 * @param apdu  Pointer to the buffer for encoding, or NULL for only length
 * @param tag_number - tag number to encode this chunk
 * @param value - calendar entry value to encode
 * @return  number of bytes encoded, or 0 if unable to encode.
 */
int bacnet_calendar_entry_context_encode(
    uint8_t *apdu, uint8_t tag_number, const BACNET_CALENDAR_ENTRY *value)
{
    int len = 0;
    int apdu_len = 0;

    if (value) {
        len = encode_opening_tag(apdu, tag_number);
        apdu_len += len;
        if (apdu) {
            apdu += len;
        }
        len = bacnet_calendar_entry_encode(apdu, value);
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
 * @brief Decodes from bytes into the calendar-entry structure
 * @param apdu - buffer to hold the bytes
 * @param apdu_size - number of bytes in the buffer to decode
 * @param entry - calendar entry value to place the decoded values
 * @return number of bytes decoded, or BACNET_STATUS_REJECT
 */
int bacnet_calendar_entry_decode(
    const uint8_t *apdu, uint32_t apdu_size, BACNET_CALENDAR_ENTRY *entry)
{
    int apdu_len = 0;
    int len = 0;
    BACNET_TAG tag = { 0 };
    BACNET_OCTET_STRING octet_string = { 0 };

    if (!apdu || !entry) {
        return BACNET_STATUS_REJECT;
    }
    if (apdu_size == 0) {
        /* empty list */
        return 0;
    }
    len = bacnet_tag_decode(&apdu[apdu_len], apdu_size - apdu_len, &tag);
    if (len <= 0) {
        return BACNET_STATUS_REJECT;
    }
    if (tag.context || tag.opening) {
        entry->tag = tag.number;
    } else {
        return BACNET_STATUS_REJECT;
    }
    switch (entry->tag) {
        case BACNET_CALENDAR_DATE:
            len = bacnet_date_context_decode(
                &apdu[apdu_len], apdu_size - apdu_len, entry->tag,
                &entry->type.Date);
            if (len <= 0) {
                return BACNET_STATUS_REJECT;
            }
            apdu_len += len;
            break;

        case BACNET_CALENDAR_DATE_RANGE:
            len = bacnet_daterange_context_decode(
                &apdu[apdu_len], apdu_size - apdu_len, entry->tag,
                &entry->type.DateRange);
            if (len <= 0) {
                return BACNET_STATUS_REJECT;
            }
            apdu_len += len;
            break;

        case BACNET_CALENDAR_WEEK_N_DAY:
            len = bacnet_octet_string_context_decode(
                &apdu[apdu_len], apdu_size - apdu_len, entry->tag,
                &octet_string);
            if (len <= 0) {
                return BACNET_STATUS_REJECT;
            }
            apdu_len += len;
            /* additional checks for valid Week-n-Day */
            if (octet_string.length != 3) {
                return BACNET_STATUS_ERROR;
            }
            entry->type.WeekNDay.month = octet_string.value[0];
            entry->type.WeekNDay.weekofmonth = octet_string.value[1];
            entry->type.WeekNDay.dayofweek = octet_string.value[2];
            break;
        default:
            /* none */
            return BACNET_STATUS_REJECT;
    }

    return apdu_len;
}

/**
 * @brief Decodes from bytes into the calendar-entry structure
 * a context tagged chunk (opening and closing tag)
 * @param apdu - buffer to hold the bytes
 * @param apdu_size - number of bytes in the buffer to decode
 * @param tag_number - tag number to encode this chunk
 * @param value - calendar entry value to place the decoded values
 *
 * @return number of bytes decoded, or BACNET_STATUS_REJECT
 */
int bacnet_calendar_entry_context_decode(
    const uint8_t *apdu,
    uint32_t apdu_size,
    uint8_t tag_number,
    BACNET_CALENDAR_ENTRY *value)
{
    int apdu_len = 0;
    int len = 0;

    if (bacnet_is_opening_tag_number(
            &apdu[apdu_len], apdu_size - apdu_len, tag_number, &len)) {
        apdu_len += len;
    } else {
        return BACNET_STATUS_REJECT;
    }
    len = bacnet_calendar_entry_decode(
        &apdu[apdu_len], apdu_size - apdu_len, value);
    if (len <= 0) {
        return BACNET_STATUS_REJECT;
    } else {
        apdu_len += len;
    }
    if (bacnet_is_closing_tag_number(
            &apdu[apdu_len], apdu_size - apdu_len, tag_number, &len)) {
        apdu_len += len;
    } else {
        return BACNET_STATUS_REJECT;
    }

    return apdu_len;
}

/**
 * @brief Compare a month to a BACnetDate value month
 * @param date - BACnetDate with a month value to compare
 * @param month - month to compare
 * @return true if the same month including special values, else false
 */
static bool month_match(const BACNET_DATE *date, uint8_t month)
{
    if (month == 0xff) {
        return true;
    }
    if (!date) {
        return false;
    }

    return (
        (month == date->month) || ((month == 13) && (date->month % 2 == 1)) ||
        ((month == 14) && (date->month % 2 == 0)));
}

/**
 * @brief Compare a week of the month to a BACnetDate value
 * @param date - BACnetDate value to compare
 * @param weekofmonth - week of the month to compare
 * @return true if the same week of the month including special values
 */
static bool weekofmonth_match(const BACNET_DATE *date, uint8_t weekofmonth)
{
    bool st = false;
    uint8_t day_to_end_month;

    switch (weekofmonth) {
        case 1:
        case 2:
        case 3:
        case 4:
        case 5:
            if (date) {
                st = (weekofmonth == (date->day - 1) % 7 + 1);
            }
            break;
        case 6:
        case 7:
        case 8:
        case 9:
            if (date) {
                day_to_end_month =
                    days_per_month(date->year, date->month) - date->day;
                st = ((weekofmonth - 6) == day_to_end_month % 7);
            }
            break;
        case 0xff:
            st = true;
            break;
        default:
            break;
    }

    return st;
}

/**
 * @brief Compare a day of the week to a BACnetDate value
 * @param date - BACnetDate value to compare
 * @param dayofweek - day of the week to compare
 * @return true if the same day of the week including special values
 */
static bool dayofweek_match(const BACNET_DATE *date, uint8_t dayofweek)
{
    if (dayofweek == 0xff) {
        return true;
    }
    if (!date) {
        return false;
    }

    return (dayofweek == date->wday);
}

/**
 * @brief Determine if a BACnetCalendarEntry includes a BACnetDate value
 * @param date - BACnetDate value to compare
 * @param entry - BACnetCalendarEntry value to compare
 * @return true if a BACnetCalendarEntry includes the BACnetDate value
 */
bool bacapp_date_in_calendar_entry(
    const BACNET_DATE *date, const BACNET_CALENDAR_ENTRY *entry)
{
    if (!entry) {
        return false;
    }
    switch (entry->tag) {
        case BACNET_CALENDAR_DATE:
            if (datetime_compare_date(date, &entry->type.Date) == 0) {
                return true;
            }
            break;
        case BACNET_CALENDAR_DATE_RANGE:
            if ((datetime_compare_date(
                     &entry->type.DateRange.startdate, date) <= 0) &&
                (datetime_compare_date(date, &entry->type.DateRange.enddate) <=
                 0)) {
                return true;
            }
            break;
        case BACNET_CALENDAR_WEEK_N_DAY:
            if (month_match(date, entry->type.WeekNDay.month) &&
                weekofmonth_match(date, entry->type.WeekNDay.weekofmonth) &&
                dayofweek_match(date, entry->type.WeekNDay.dayofweek)) {
                return true;
            }
            break;
        default:
            /* do nothing */
            break;
    }

    return false;
}

/**
 * @brief Determine if two BACnetCalendarEntry are the same
 * @param value1 - BACnetCalendarEntry value to compare
 * @param value2 - BACnetCalendarEntry value to compare
 * @return true if both BACnetCalendarEntry are the same
 */
bool bacnet_calendar_entry_same(
    const BACNET_CALENDAR_ENTRY *value1, const BACNET_CALENDAR_ENTRY *value2)
{
    if (!value1 || !value2) {
        return false;
    }
    if (value1->tag != value2->tag) {
        return false;
    }
    switch (value1->tag) {
        case BACNET_CALENDAR_DATE:
            return datetime_compare_date(
                       &value1->type.Date, &value2->type.Date) == 0;
        case BACNET_CALENDAR_DATE_RANGE:
            return (datetime_compare_date(
                        &value1->type.DateRange.startdate,
                        &value2->type.DateRange.startdate) == 0) &&
                (datetime_compare_date(
                     &value2->type.DateRange.enddate,
                     &value2->type.DateRange.enddate) == 0);
        case BACNET_CALENDAR_WEEK_N_DAY:
            return (value1->type.WeekNDay.month ==
                    value2->type.WeekNDay.month) &&
                (value1->type.WeekNDay.weekofmonth ==
                 value2->type.WeekNDay.weekofmonth) &&
                (value1->type.WeekNDay.dayofweek ==
                 value2->type.WeekNDay.dayofweek);
        default:
            /* should be unreachable */
            return false;
    }
}
