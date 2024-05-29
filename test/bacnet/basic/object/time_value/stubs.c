/**
 * @file
 * @brief Stub functions for unit test of a BACnet object
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date December 2022
 *
 * SPDX-License-Identifier: MIT
 */
#include <stdbool.h>
#include <stdint.h>
#include "bacnet/datetime.h"

bool datetime_local(
    BACNET_DATE *bdate,
    BACNET_TIME *btime,
    int16_t *utc_offset_minutes,
    bool *dst_active)
{
    bdate->year = 2023;
    bdate->month = 6;
    bdate->day = 26;
    bdate->wday = 1;

    (void)btime;
    (void)utc_offset_minutes;
    (void)dst_active;
    return true;
}
