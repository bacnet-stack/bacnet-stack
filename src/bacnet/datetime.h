/**************************************************************************
 *
 * Copyright (C) 2012 Steve Karg <skarg@users.sourceforge.net>
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *********************************************************************/
#ifndef DATE_TIME_H
#define DATE_TIME_H

#include <stdint.h>
#include <stdbool.h>
#include "bacnet/bacnet_stack_exports.h"
#include "bacnet/basic/sys/platform.h"

/* define our epic beginnings */
#define BACNET_DATE_YEAR_EPOCH 1900
/* 1/1/1900 is a Monday */
#define BACNET_DAY_OF_WEEK_EPOCH BACNET_WEEKDAY_MONDAY

typedef enum BACnet_Weekday {
    BACNET_WEEKDAY_MONDAY = 1,
    BACNET_WEEKDAY_TUESDAY = 2,
    BACNET_WEEKDAY_WEDNESDAY = 3,
    BACNET_WEEKDAY_THURSDAY = 4,
    BACNET_WEEKDAY_FRIDAY = 5,
    BACNET_WEEKDAY_SATURDAY = 6,
    BACNET_WEEKDAY_SUNDAY = 7
} BACNET_WEEKDAY;

/* date */
typedef struct BACnet_Date {
    uint16_t year; /* AD */
    uint8_t month; /* 1=Jan */
    uint8_t day; /* 1..31 */
    uint8_t wday; /* 1=Monday-7=Sunday */
} BACNET_DATE;

/* time */
typedef struct BACnet_Time {
    uint8_t hour;
    uint8_t min;
    uint8_t sec;
    uint8_t hundredths;
} BACNET_TIME;

typedef struct BACnet_DateTime {
    BACNET_DATE date;
    BACNET_TIME time;
} BACNET_DATE_TIME;

/* range of dates */
typedef struct BACnet_Date_Range {
    BACNET_DATE startdate;
    BACNET_DATE enddate;
} BACNET_DATE_RANGE;

/* week and days */
typedef struct BACnet_Weeknday {
    uint8_t month; /* 1=Jan 13=odd 14=even FF=any */
    uint8_t weekofmonth; /* 1=days 1-7, 2=days 8-14, 3=days 15-21, 4=days 22-28,
                            5=days 29-31, 6=last 7 days, FF=any week */
    uint8_t dayofweek; /* 1=Monday-7=Sunday, FF=any */
} BACNET_WEEKNDAY;

#ifdef UINT64_MAX
typedef uint64_t bacnet_time_t;
#else
typedef uint32_t bacnet_time_t;
#endif

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* utility initialization functions */
BACNET_STACK_EXPORT
void datetime_set_date(
    BACNET_DATE *bdate, uint16_t year, uint8_t month, uint8_t day);
BACNET_STACK_EXPORT
void datetime_set_time(BACNET_TIME *btime,
    uint8_t hour,
    uint8_t minute,
    uint8_t seconds,
    uint8_t hundredths);
BACNET_STACK_EXPORT
void datetime_set(
    BACNET_DATE_TIME *bdatetime, BACNET_DATE *bdate, BACNET_TIME *btime);
BACNET_STACK_EXPORT
void datetime_set_values(BACNET_DATE_TIME *bdatetime,
    uint16_t year,
    uint8_t month,
    uint8_t day,
    uint8_t hour,
    uint8_t minute,
    uint8_t seconds,
    uint8_t hundredths);
BACNET_STACK_EXPORT
uint32_t datetime_hms_to_seconds_since_midnight(
    uint8_t hours, uint8_t minutes, uint8_t seconds);
BACNET_STACK_EXPORT
uint16_t datetime_hm_to_minutes_since_midnight(uint8_t hours, uint8_t minutes);
BACNET_STACK_EXPORT
void datetime_hms_from_seconds_since_midnight(
    uint32_t seconds, uint8_t *pHours, uint8_t *pMinutes, uint8_t *pSeconds);
/* utility test for validity */
BACNET_STACK_EXPORT
bool datetime_is_valid(BACNET_DATE *bdate, BACNET_TIME *btime);
BACNET_STACK_EXPORT
bool datetime_time_is_valid(BACNET_TIME *btime);
BACNET_STACK_EXPORT
bool datetime_date_is_valid(BACNET_DATE *bdate);
/* date and time calculations and summaries */
BACNET_STACK_EXPORT
void datetime_ymd_from_days_since_epoch(
    uint32_t days, uint16_t *pYear, uint8_t *pMonth, uint8_t *pDay);
BACNET_STACK_EXPORT
uint32_t datetime_days_since_epoch(BACNET_DATE *bdate);
BACNET_STACK_EXPORT
void datetime_days_since_epoch_into_date(uint32_t days, BACNET_DATE *bdate);
BACNET_STACK_EXPORT
uint32_t datetime_day_of_year(BACNET_DATE *bdate);
BACNET_STACK_EXPORT
uint32_t datetime_ymd_to_days_since_epoch(
    uint16_t year, uint8_t month, uint8_t day);
BACNET_STACK_EXPORT
void datetime_day_of_year_into_date(
    uint32_t days, uint16_t year, BACNET_DATE *bdate);
BACNET_STACK_EXPORT
bool datetime_is_leap_year(uint16_t year);
BACNET_STACK_EXPORT
uint8_t datetime_month_days(uint16_t year, uint8_t month);
BACNET_STACK_EXPORT
uint8_t datetime_day_of_week(uint16_t year, uint8_t month, uint8_t day);
BACNET_STACK_EXPORT
bool datetime_ymd_is_valid(uint16_t year, uint8_t month, uint8_t day);
BACNET_STACK_EXPORT
uint32_t datetime_ymd_day_of_year(uint16_t year, uint8_t month, uint8_t day);

BACNET_STACK_EXPORT
void datetime_seconds_since_midnight_into_time(
    uint32_t seconds, BACNET_TIME *btime);
BACNET_STACK_EXPORT
uint32_t datetime_seconds_since_midnight(BACNET_TIME *btime);
BACNET_STACK_EXPORT
uint16_t datetime_minutes_since_midnight(BACNET_TIME *btime);

/* utility comparison functions:
   if the date/times are the same, return is 0
   if date1 is before date2, returns negative
   if date1 is after date2, returns positive */
BACNET_STACK_EXPORT
int datetime_compare_date(BACNET_DATE *date1, BACNET_DATE *date2);
BACNET_STACK_EXPORT
int datetime_compare_time(BACNET_TIME *time1, BACNET_TIME *time2);
BACNET_STACK_EXPORT
int datetime_compare(BACNET_DATE_TIME *datetime1, BACNET_DATE_TIME *datetime2);

/* full comparison functions:
 * taking into account FF fields in date and time structures,
 * do a full comparison of two values */
BACNET_STACK_EXPORT
int datetime_wildcard_compare_date(BACNET_DATE *date1, BACNET_DATE *date2);
BACNET_STACK_EXPORT
int datetime_wildcard_compare_time(BACNET_TIME *time1, BACNET_TIME *time2);
BACNET_STACK_EXPORT
int datetime_wildcard_compare(
    BACNET_DATE_TIME *datetime1, BACNET_DATE_TIME *datetime2);

/* utility copy functions */
BACNET_STACK_EXPORT
void datetime_copy_date(BACNET_DATE *dest, BACNET_DATE *src);
BACNET_STACK_EXPORT
void datetime_copy_time(BACNET_TIME *dest, BACNET_TIME *src);
BACNET_STACK_EXPORT
void datetime_copy(BACNET_DATE_TIME *dest, BACNET_DATE_TIME *src);

/* utility add or subtract minutes function */
BACNET_STACK_EXPORT
void datetime_add_minutes(BACNET_DATE_TIME *bdatetime, int32_t minutes);

BACNET_STACK_EXPORT
bacnet_time_t datetime_seconds_since_epoch(BACNET_DATE_TIME *bdatetime);
BACNET_STACK_EXPORT
void datetime_since_epoch_seconds(
    BACNET_DATE_TIME *bdatetime, bacnet_time_t seconds);
BACNET_STACK_EXPORT
bacnet_time_t datetime_seconds_since_epoch_max(void);

/* date and time wildcards */
BACNET_STACK_EXPORT
bool datetime_wildcard_year(BACNET_DATE *bdate);
BACNET_STACK_EXPORT
void datetime_wildcard_year_set(BACNET_DATE *bdate);
BACNET_STACK_EXPORT
bool datetime_wildcard_month(BACNET_DATE *bdate);
BACNET_STACK_EXPORT
void datetime_wildcard_month_set(BACNET_DATE *bdate);
BACNET_STACK_EXPORT
bool datetime_wildcard_day(BACNET_DATE *bdate);
BACNET_STACK_EXPORT
void datetime_wildcard_day_set(BACNET_DATE *bdate);
BACNET_STACK_EXPORT
bool datetime_wildcard_weekday(BACNET_DATE *bdate);
BACNET_STACK_EXPORT
void datetime_wildcard_weekday_set(BACNET_DATE *bdate);
BACNET_STACK_EXPORT
bool datetime_wildcard_hour(BACNET_TIME *btime);
BACNET_STACK_EXPORT
void datetime_wildcard_hour_set(BACNET_TIME *btime);
BACNET_STACK_EXPORT
bool datetime_wildcard_minute(BACNET_TIME *btime);
BACNET_STACK_EXPORT
void datetime_wildcard_minute_set(BACNET_TIME *btime);
BACNET_STACK_EXPORT
bool datetime_wildcard_second(BACNET_TIME *btime);
BACNET_STACK_EXPORT
void datetime_wildcard_second_set(BACNET_TIME *btime);
BACNET_STACK_EXPORT
bool datetime_wildcard_hundredths(BACNET_TIME *btime);
BACNET_STACK_EXPORT
void datetime_wildcard_hundredths_set(BACNET_TIME *btime);
BACNET_STACK_EXPORT
bool datetime_wildcard(BACNET_DATE_TIME *bdatetime);
BACNET_STACK_EXPORT
bool datetime_wildcard_present(BACNET_DATE_TIME *bdatetime);
BACNET_STACK_EXPORT
void datetime_wildcard_set(BACNET_DATE_TIME *bdatetime);
BACNET_STACK_EXPORT
void datetime_date_wildcard_set(BACNET_DATE *bdate);
BACNET_STACK_EXPORT
void datetime_time_wildcard_set(BACNET_TIME *btime);

BACNET_STACK_EXPORT
bool datetime_local_to_utc(BACNET_DATE_TIME *utc_time,
    BACNET_DATE_TIME *local_time,
    int16_t utc_offset_minutes,
    int8_t dst_adjust_minutes);
BACNET_STACK_EXPORT
bool datetime_utc_to_local(BACNET_DATE_TIME *local_time,
    BACNET_DATE_TIME *utc_time,
    int16_t utc_offset_minutes,
    int8_t dst_adjust_minutes);

BACNET_STACK_EXPORT
bool datetime_date_init_ascii(BACNET_DATE *bdate, const char *ascii);
BACNET_STACK_EXPORT
bool datetime_time_init_ascii(BACNET_TIME *btime, const char *ascii);
BACNET_STACK_EXPORT
bool datetime_init_ascii(BACNET_DATE_TIME *bdatetime, const char *ascii);

BACNET_STACK_EXPORT
int bacapp_encode_datetime(uint8_t *apdu, BACNET_DATE_TIME *value);
BACNET_STACK_EXPORT
int bacapp_encode_context_datetime(
    uint8_t *apdu, uint8_t tag_number, BACNET_DATE_TIME *value);
BACNET_STACK_EXPORT
int bacnet_datetime_decode(
    uint8_t *apdu, uint32_t apdu_size, BACNET_DATE_TIME *value);
BACNET_STACK_EXPORT
int bacnet_datetime_context_decode(
    uint8_t *apdu, uint32_t apdu_size,  uint8_t tag_number,
    BACNET_DATE_TIME *value);

BACNET_STACK_DEPRECATED("Use bacnet_datetime_decode() instead")
BACNET_STACK_EXPORT
int bacapp_decode_datetime(
    uint8_t *apdu, BACNET_DATE_TIME *value);
BACNET_STACK_DEPRECATED("Use bacnet_datetime_context_decode() instead")
BACNET_STACK_EXPORT
int bacapp_decode_context_datetime(uint8_t *apdu,
    uint8_t tag_number,
    BACNET_DATE_TIME *value);

/* implementation agnostic functions - create your own! */
BACNET_STACK_EXPORT
bool datetime_local(BACNET_DATE *bdate,
    BACNET_TIME *btime,
    int16_t *utc_offset_minutes,
    bool *dst_active);
BACNET_STACK_EXPORT
void datetime_init(void);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* DATE_TIME_H */
