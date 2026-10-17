/**
 * @file
 * @brief Test support for the TimeSynchronization handler
 * @copyright SPDX-License-Identifier: MIT
 */
#include <stdbool.h>
#include <stdint.h>
#include "bacnet/bacaddr.h"
#include "bacnet/basic/object/device.h"
#include "bacnet/basic/service/s_ts.h"
#include "bacnet/datetime.h"

static BACNET_DATE_TIME Test_Date_Time;
static int16_t Test_UTC_Offset = 300;
static bool Test_DST_Active;
static int8_t Test_DST_Adjust = -60;
static uint32_t Test_Time_Sync_Interval;
static uint32_t Test_Interval_Offset;
static bool Test_Align_Intervals;
unsigned Test_Local_Sync_Count;
unsigned Test_UTC_Sync_Count;
BACNET_DATE_TIME Test_Local_Sync_Time;
BACNET_DATE_TIME Test_UTC_Sync_Time;
BACNET_ADDRESS Test_Local_Sync_Address;
BACNET_ADDRESS Test_UTC_Sync_Address;

bool datetime_local(
    BACNET_DATE *bdate,
    BACNET_TIME *btime,
    int16_t *utc_offset_minutes,
    bool *dst_active)
{
    if (bdate) {
        datetime_copy_date(bdate, &Test_Date_Time.date);
    }
    if (btime) {
        datetime_copy_time(btime, &Test_Date_Time.time);
    }
    if (utc_offset_minutes) {
        *utc_offset_minutes = Test_UTC_Offset;
    }
    if (dst_active) {
        *dst_active = Test_DST_Active;
    }

    return true;
}

void test_datetime_local_set(
    const BACNET_DATE_TIME *date_time, int16_t utc_offset, bool dst_active)
{
    datetime_copy(&Test_Date_Time, date_time);
    Test_UTC_Offset = utc_offset;
    Test_DST_Active = dst_active;
    Test_DST_Adjust = -60;
}

bool datetime_zone(
    BACNET_DATE_TIME *local_time,
    int16_t *utc_offset_minutes,
    int8_t *dst_adjust_minutes)
{
    if (local_time) {
        datetime_copy(local_time, &Test_Date_Time);
    }
    if (utc_offset_minutes) {
        *utc_offset_minutes = Test_UTC_Offset;
    }
    if (dst_adjust_minutes) {
        if (Test_DST_Active) {
            *dst_adjust_minutes = Test_DST_Adjust;
        } else {
            *dst_adjust_minutes = 0;
        }
    }

    return true;
}

void test_datetime_dst_adjust_set(int8_t dst_adjust_minutes)
{
    Test_DST_Adjust = dst_adjust_minutes;
}

bool Device_Align_Intervals(void)
{
    return Test_Align_Intervals;
}

uint32_t Device_Time_Sync_Interval(void)
{
    return Test_Time_Sync_Interval;
}

uint32_t Device_Interval_Offset(void)
{
    return Test_Interval_Offset;
}

void test_device_time_sync_set(uint32_t interval, bool align, uint32_t offset)
{
    Test_Time_Sync_Interval = interval;
    Test_Align_Intervals = align;
    Test_Interval_Offset = offset;
}

void Send_TimeSync_Remote(
    BACNET_ADDRESS *dest, const BACNET_DATE *bdate, const BACNET_TIME *btime)
{
    if (dest) {
        bacnet_address_copy(&Test_Local_Sync_Address, dest);
    }
    datetime_set(&Test_Local_Sync_Time, bdate, btime);
    Test_Local_Sync_Count++;
}

void Send_TimeSyncUTC_Remote(
    BACNET_ADDRESS *dest, const BACNET_DATE *bdate, const BACNET_TIME *btime)
{
    if (dest) {
        bacnet_address_copy(&Test_UTC_Sync_Address, dest);
    }
    datetime_set(&Test_UTC_Sync_Time, bdate, btime);
    Test_UTC_Sync_Count++;
}
