/*####COPYRIGHTBEGIN####
-------------------------------------------
Copyright (C) 2022 Steve Karg <skarg@users.sourceforge.net>

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
#include "bacnet/weeklyschedule.h"
#include "bacnet/bacdcode.h"

int weeklyschedule_decode(
    uint8_t * apdu,
    int max_apdu_len,
    BACNET_WEEKLY_SCHEDULE * week)
{
    int j;
    int len = 0;
    int apdu_len = 0;

    for (j = 0; j < 7; j++) {
        len =
            dailyschedule_decode(&apdu[apdu_len], max_apdu_len - apdu_len,
                &week->weeklySchedule[j]);
        if (len < 0)
            return -1;
        apdu_len += len;
    }
    return apdu_len;
}

int weeklyschedule_encode(
    uint8_t * apdu,
    BACNET_WEEKLY_SCHEDULE * week)
{
    int j;
    int apdu_len = 0;
    int len = 0;
    uint8_t *apdu_offset = NULL;

    for (j = 0; j < 7; j++) {
        /* not enough room to write data */
        if (apdu) {
            apdu_offset = &apdu[apdu_len];
        }
        len =
            dailyschedule_encode(apdu_offset,
                &week->weeklySchedule[j]);
        if (len < 0)
            return -1;
        apdu_len += len;
    }
    return apdu_len;
}



/**
 * @brief Encode a context tagged WeeklySchedule complex data type
 * @param apdu - the APDU buffer
 * @param tag_number - the APDU buffer size
 * @param value - WeeklySchedule structure
 * @return length of the APDU buffer, or 0 if not able to encode
 */
int weeklyschedule_context_encode(
    uint8_t *apdu, uint8_t tag_number, BACNET_WEEKLY_SCHEDULE *value)
{
    int len = 0;
    int apdu_len = 0;
    uint8_t *apdu_offset = NULL;

    if (value) {
        apdu_offset = apdu;
        len = encode_opening_tag(apdu_offset, tag_number);
        apdu_len += len;
        if (apdu) {
            apdu_offset = &apdu[apdu_len];
        }
        len = weeklyschedule_encode(apdu_offset, value);
        apdu_len += len;
        if (apdu) {
            apdu_offset = &apdu[apdu_len];
        }
        len = encode_closing_tag(apdu_offset, tag_number);
        apdu_len += len;
    }

    return apdu_len;
}