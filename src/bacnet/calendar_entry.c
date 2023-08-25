/*####COPYRIGHTBEGIN####
-------------------------------------------
Copyright (C) 2023 Steve Karg <skarg@users.sourceforge.net>

This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License
       as published by the Free Software Foundation; either version 2
   of the License, or (at your option) any later version.

      This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to:
The Free Software Foundation, Inc.
59 Temple Place - Suite 330
Boston, MA  02111-1307, USA.

As a special exception, if other files instantiate templates or
use macros or inline functions from this file, or you compile
this file and link it with other works to produce a work based
on this file, this file does not by itself cause the resulting
work to be covered by the GNU General Public License. However
the source code for this file must still be made available in
accordance with section (3) of the GNU General Public License.

This exception does not invalidate any other reasons why a work
based on this file might be covered by the GNU General Public
License.
-------------------------------------------
####COPYRIGHTEND####*/

#include <stdint.h>
#include "bacnet/calendar_entry.h"
#include "bacnet/bacapp.h"
#include "bacnet/bacdcode.h"
#include "bacnet/bactimevalue.h"
#include "bacnet/datetime.h"
#include "bacnet/basic/sys/days.h"

/** @file calendar_entry.c  Manipulate BACnet calendar entry values */

/*
 * @brief Encode the BACnetCalendarEntry complex data
 *
 * BACnetCalendarEntry ::= CHOICE {
 *     date [0] Date,
 *     date-range [1] BACnetDateRange,
 *     weekNDay [2] BACnetWeekNDay
 * }
 *
 * @param apdu  Pointer to the buffer for encoding.
 * @param value Pointer to the property data to be encoded.
 * @return bytes encoded or zero on error.
 */
int bacnet_calendar_entry_encode(uint8_t *apdu, BACNET_CALENDAR_ENTRY *value)
{
    int len = 0;
    int apdu_len = 0;
    BACNET_OCTET_STRING octetstring;

    switch (value->tag) {
    case BACNET_CALENDAR_DATE:
        len = encode_context_date(apdu, value->tag, &value->type.Date);
        apdu_len += len;
        break;
    case BACNET_CALENDAR_DATE_RANGE:
        len = bacapp_daterange_context_encode(apdu, value->tag, &value->type.DateRange);
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
 * Encodes into bytes from the calendar-entry structure
 * a context tagged chunk (opening and closing tag)
 *
 * @param apdu - buffer to hold the bytes
 * @param tag_number - tag number to encode this chunk
 * @param value - calendar entry value to encode
 *
 * @return  number of bytes encoded, or 0 if unable to encode.
 */
int bacnet_calendar_entry_context_encode(
    uint8_t *apdu, uint8_t tag_number, BACNET_CALENDAR_ENTRY *value)
{
    int len = 0;
    int apdu_len = 0;

    if (apdu && value) {
        len = encode_opening_tag(&apdu[apdu_len], tag_number);
        apdu_len += len;

        len = bacnet_calendar_entry_encode(&apdu[apdu_len], value);
        apdu_len += len;

        len = encode_closing_tag(&apdu[apdu_len], tag_number);
        apdu_len += len;
    }
    return apdu_len;
}

/**
 * Decodes from bytes into the calendar-entry structure
 *
 * @param apdu - buffer to hold the bytes
 * @param apdu_max_len - number of bytes in the buffer to decode
 * @param entry - calendar entry value to place the decoded values
 *
 * @return number of bytes decoded, or BACNET_STATUS_REJECT
 */
int bacnet_calendar_entry_decode(uint8_t *apdu, uint32_t apdu_max_len,
    BACNET_CALENDAR_ENTRY *entry)
{
    int apdu_len = 0;
    int len;
    BACNET_OCTET_STRING octet_string = { 0 };

    if (!apdu || !entry) {
        return BACNET_STATUS_REJECT;
    }

    len = decode_tag_number(apdu, &entry->tag);
    if (len <= 0) {
        return BACNET_STATUS_REJECT;
    }

    switch (entry->tag) {
    case BACNET_CALENDAR_DATE:
        len = decode_context_date(apdu, entry->tag, &entry->type.Date);
        if (len <= 0) {
            return BACNET_STATUS_REJECT;
        }
        apdu_len += len;
        break;

    case BACNET_CALENDAR_DATE_RANGE:
        len = bacapp_daterange_context_decode(apdu, entry->tag, &entry->type.DateRange);
        if (len <= 0) {
            return BACNET_STATUS_REJECT;
        }
        apdu_len += len;
        break;

    case BACNET_CALENDAR_WEEK_N_DAY:
        len = decode_context_octet_string(apdu, entry->tag, &octet_string);
        if (len <= 0) {
            return BACNET_STATUS_REJECT;
        }
        apdu_len += len;

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
 * Decodes from bytes into the calendar-entry structure
 * a context tagged chunk (opening and closing tag)
 *
 * @param apdu - buffer to hold the bytes
 * @param apdu_max_len - number of bytes in the buffer to decode
 * @param tag_number - tag number to encode this chunk
 * @param value - calendar entry value to place the decoded values
 *
 * @return number of bytes decoded, or BACNET_STATUS_REJECT
 */
int bacnet_calendar_entry_context_decode(uint8_t *apdu, uint32_t apdu_max_len,
    uint8_t tag_number, BACNET_CALENDAR_ENTRY *value)
{
    int apdu_len = 0;
    int len;

    if (decode_is_opening_tag_number(&apdu[apdu_len], tag_number)) {
        apdu_len++;
    } else {
        return BACNET_STATUS_REJECT;
    }

    if (BACNET_STATUS_REJECT ==
        (len = bacnet_calendar_entry_decode(&apdu[apdu_len],
                                    apdu_max_len - apdu_len, value))) {
        return BACNET_STATUS_REJECT;
    } else {
        apdu_len += len;
    }

    if (decode_is_closing_tag_number(&apdu[apdu_len], tag_number)) {
        apdu_len++;
    } else {
        return BACNET_STATUS_REJECT;
    }
    if (apdu_len > apdu_max_len) {
        return BACNET_STATUS_REJECT;
    }
    return apdu_len;
}

static bool month_match(BACNET_DATE *date, uint8_t month)
{
    return (month == 0xff) || (month == date->month) ||
        ((month == 13) && (date->month % 2 == 1))  ||
        ((month == 14) && (date->month % 2 == 0));
}

static bool weekofmonth_match(BACNET_DATE *date, uint8_t weekofmonth)
{
    bool st = false;
    uint8_t day_to_end_month =
        days_per_month(date->year, date->month) - date->day;

    switch (weekofmonth) {
    case 1:
    case 2:
    case 3:
    case 4:
    case 5:
        st = (weekofmonth == (date->day - 1) % 7 + 1);
        break;
    case 6:
    case 7:
    case 8:
    case 9:
        st = ((weekofmonth - 6) == day_to_end_month % 7);
        break;
    case 0xff:
        st = true;
        break;
    default:
        break;
    }
    return st;
}

static bool dayofweek_match(BACNET_DATE *date, uint8_t dayofweek)
{
    return (dayofweek == 0xff) || (dayofweek == date->wday);
}

bool bacapp_date_in_calendar_entry(BACNET_DATE *date,
    BACNET_CALENDAR_ENTRY *entry)
 {
    switch (entry->tag) {
    case BACNET_CALENDAR_DATE:
        if (datetime_compare_date(date, &entry->type.Date) == 0) {
            return true;
        }
        break;
    case BACNET_CALENDAR_DATE_RANGE:
        if ((datetime_compare_date(
                &entry->type.DateRange.startdate, date) <= 0) &&
            (datetime_compare_date(
                date, &entry->type.DateRange.enddate) <= 0)) {
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

bool bacnet_calendar_entry_same(
    BACNET_CALENDAR_ENTRY *value1, BACNET_CALENDAR_ENTRY *value2)
{
    if (value1->tag != value2->tag) {
    return false;
    }

    switch (value1->tag) {
    case BACNET_CALENDAR_DATE:
        return datetime_compare_date(&value1->type.Date, &value2->type.Date) ==
            0;
    case BACNET_CALENDAR_DATE_RANGE:
        return (datetime_compare_date(&value1->type.DateRange.startdate,
                    &value2->type.DateRange.startdate) == 0) &&
            (datetime_compare_date(&value2->type.DateRange.enddate,
                 &value2->type.DateRange.enddate) == 0);
    case BACNET_CALENDAR_WEEK_N_DAY:
        return (value1->type.WeekNDay.month == value2->type.WeekNDay.month) &&
            (value1->type.WeekNDay.weekofmonth ==
                value2->type.WeekNDay.weekofmonth) &&
            (value1->type.WeekNDay.dayofweek ==
                value2->type.WeekNDay.dayofweek);
    default:
        /* should be unreachable */
        return false;
    }
}
