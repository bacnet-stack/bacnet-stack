/**
 * @file
 * @brief mock system datetime functions
 * @author Steve Karg
 * @date August 2024
 * @copyright SPDX-License-Identifier: MIT
 */
#include <zephyr/ztest.h>
#include <bacnet/datetime.h>

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
    return true;
}
