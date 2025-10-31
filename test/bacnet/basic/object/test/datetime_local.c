/**
 * @file
 * @brief Stub functions for unit test of a BACnet object
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date December 2022
 *
 * @copyright SPDX-License-Identifier: MIT
 */
#include <stdbool.h>
#include <stdint.h>
#include "bacnet/datetime.h"

static BACNET_DATE BACnet_Date;
static BACNET_TIME BACnet_Time;

bool datetime_local(
    BACNET_DATE *bdate,
    BACNET_TIME *btime,
    int16_t *utc_offset_minutes,
    bool *dst_active)
{
    if (bdate) {
        datetime_copy_date(bdate, &BACnet_Date);
    }
    if (btime) {
        datetime_copy_time(btime, &BACnet_Time);
    }
    (void)utc_offset_minutes;
    (void)dst_active;

    return true;
}

void datetime_timesync(BACNET_DATE *bdate, BACNET_TIME *btime, bool utc)
{
    if (bdate) {
        datetime_copy_date(&BACnet_Date, bdate);
    }
    if (btime) {
        datetime_copy_time(&BACnet_Time, btime);
    }
    (void)utc;
}

void datetime_init(void)
{
    /* nothing to do */
}
