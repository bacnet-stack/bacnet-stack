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
#include "bacnet/calendar.h"
#include "bacnet/bacdcode.h"
#include "bacnet/bactimevalue.h"
#include "bacnet/datetime.h"

typedef enum BACnet_CalendarEntry_Tags {
    BACNET_CALENDAR_DATE = 0,
    BACNET_CALENDAR_DATE_RANGE = 1,
    BACNET_CALENDAR_WEEK_N_DAY = 2
} BACNET_CALENDAR_ENTRY_TAGS;

int bacapp_encode_CalendarEntry(uint8_t *apdu, BACNET_CALENDAR_ENTRY *value)
{
    int len = 0;
    int apdu_len = 0;
    uint8_t *apdu_offset = NULL;

    len = encode_opening_tag(apdu, value->tag);
    apdu_len += len;
    if (apdu) {
        apdu_offset = &apdu[apdu_len];
    }

    switch (value->tag) {
    case BACNET_CALENDAR_DATE:
        len = encode_bacnet_date(apdu_offset, &value->type.Date);
        apdu_len += len;
        break;
    case BACNET_CALENDAR_DATE_RANGE:
        len = encode_bacnet_date(apdu_offset, &value->type.DateRange.startdate);
        apdu_len += len;
        if (apdu) {
            apdu_offset = &apdu[apdu_len];
        }
        len = encode_bacnet_date(apdu_offset, &value->type.DateRange.enddate);
        apdu_len += len;
        break;
    case BACNET_CALENDAR_WEEK_N_DAY:
        len = encode_bacnet_unsigned(apdu_offset, value->type.WeekNDay.month);
        apdu_len += len;
        if (apdu) {
            apdu_offset = &apdu[apdu_len];
        }
        len = encode_bacnet_unsigned(
            apdu_offset, value->type.WeekNDay.weekofmonth);
        apdu_len += len;
        if (apdu) {
            apdu_offset = &apdu[apdu_len];
        }
        len = encode_bacnet_unsigned(
            apdu_offset, value->type.WeekNDay.dayofweek);
        apdu_len += len;
        break;
    default:
        /* do nothing */
        break;
    }

    if (apdu) {
        apdu_offset = &apdu[apdu_len];
    }
    len = encode_closing_tag(apdu_offset, value->tag);
    apdu_len += len;

    return apdu_len;
}

int bacapp_encode_context_CalendarEntry(
    uint8_t *apdu, uint8_t tag_number, BACNET_CALENDAR_ENTRY *value)
{
    int len = 0;
    int apdu_len = 0;

    if (apdu && value) {
        len = encode_opening_tag(&apdu[apdu_len], tag_number);
        apdu_len += len;

        len = bacapp_encode_CalendarEntry(&apdu[apdu_len], value);
        apdu_len += len;

        len = encode_closing_tag(&apdu[apdu_len], tag_number);
        apdu_len += len;
    }
    return apdu_len;
}

int bacapp_decode_CalendarEntry(uint8_t *apdu, uint32_t len_value,
    BACNET_CALENDAR_ENTRY *value)
{
    int apdu_len;
    int len = 0;
    BACNET_UNSIGNED_INTEGER v;

    apdu_len = decode_tag_number(apdu, &value->tag);

    switch (value->tag) {
    case BACNET_CALENDAR_DATE:
        if ( -1 == (len = decode_date(&apdu[apdu_len], &value->type.Date))) {
            return -1;
        }
        apdu_len += len;
        break;
    case BACNET_CALENDAR_DATE_RANGE:
        if ( -1 == (len =
            decode_date(&apdu[apdu_len], &value->type.DateRange.startdate))) {
            return -1;
        }
        apdu_len += len;
        if ( -1 == (len =
            decode_date(&apdu[apdu_len], &value->type.DateRange.enddate))) {
            return -1;
        }
        apdu_len += len;
        break;
    case BACNET_CALENDAR_WEEK_N_DAY:
        if ( -1 == (len =
            decode_unsigned(&apdu[apdu_len], len_value - apdu_len, &v))) {
            return -1;
        }
        value->type.WeekNDay.month = v;
        apdu_len += len;
        if ( -1 == (len =
            decode_unsigned(&apdu[apdu_len], len_value - apdu_len, &v))) {
            return -1;
        }
        value->type.WeekNDay.weekofmonth = v;
        apdu_len += len;
        if ( -1 == (len =
            decode_unsigned(&apdu[apdu_len], len_value - apdu_len, &v))) {
            return -1;
        }
        value->type.WeekNDay.dayofweek = v;
        apdu_len += len;
        break;
    default:
       /* none */
        return BACNET_STATUS_REJECT;
    }

    if (!decode_is_closing_tag_number(&apdu[apdu_len++], value->tag)) {
        return BACNET_STATUS_REJECT;
    }

    if (apdu_len > len_value) {
        return BACNET_STATUS_REJECT;
    }

    return apdu_len;
}

int bacapp_decode_context_CalendarEntry(uint8_t *apdu, uint32_t len_value,
    uint8_t tag_number, BACNET_CALENDAR_ENTRY *value)
{
    int apdu_len = 0;
    int len;

    if (decode_is_opening_tag_number(&apdu[apdu_len], tag_number)) {
        apdu_len++;
    } else {
        return -1;
    }

    if (-1 == (len = bacapp_decode_CalendarEntry(&apdu[apdu_len],
                                    len_value - apdu_len, value))) {
        return -1;
    } else {
        apdu_len += len;
    }

    if (decode_is_closing_tag_number(&apdu[apdu_len], tag_number)) {
        apdu_len++;
    } else {
        return -1;
    }
    if (apdu_len > len_value) {
        return -1;
    }
    return apdu_len;
}
