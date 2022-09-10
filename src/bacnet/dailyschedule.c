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
#include "bacnet/dailyschedule.h"
#include "bacnet/bactimevalue.h"

int bacnet_dailyschedule_decode(
    uint8_t *apdu, int max_apdu_len, BACNET_DAILY_SCHEDULE *day)
{
    unsigned int tv_count = 0;
    int retval = bacnet_time_values_context_decode(apdu, max_apdu_len, 0,
        &day->Time_Values[0], MAX_DAY_SCHEDULE_VALUES, &tv_count);
    day->TV_Count = (uint16_t)tv_count;
    return retval;
}

int bacnet_dailyschedule_encode(uint8_t *apdu, BACNET_DAILY_SCHEDULE *day)
{
    return bacnet_time_values_context_encode(
        apdu, 0, &day->Time_Values[0], day->TV_Count);
}
