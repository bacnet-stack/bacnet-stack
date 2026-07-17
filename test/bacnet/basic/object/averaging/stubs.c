/**
 * @file
 * @brief Stub functions for unit test of averaging object
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date July 2026
 *
 * @copyright SPDX-License-Identifier: MIT
 */
#include <stdbool.h>
#include <stdint.h>
#include "bacnet/bacdef.h"
#include "bacnet/datetime.h"

bool datetime_local(
    BACNET_DATE *bdate,
    BACNET_TIME *btime,
    int16_t *utc_offset_minutes,
    bool *dst_active)
{
    (void)bdate;
    (void)btime;
    (void)utc_offset_minutes;
    (void)dst_active;

    return false;
}
