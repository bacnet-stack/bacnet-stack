/**
 * @file
 * @brief API for BACnetDate, BACnetTime, BACnetDateTime, BACnetDateRange
 * complex data type encode and decode
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @author Greg Shue <greg.shue@outlook.com>
 * @author Ondřej Hruška <ondra@ondrovo.com>
 * @date 2012
 * @copyright SPDX-License-Identifier: MIT
 */
#ifndef BACNET_DATE_TIME_H
#define BACNET_DATE_TIME_H

#include <stdint.h>
#include <stdbool.h>
/* BACnet Stack defines - first */
#include "bacnet/bacdef.h"

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
void datetime_set_time(
    BACNET_TIME *btime,
    uint8_t hour,
    uint8_t minute,
    uint8_t seconds,
    uint8_t hundredths);
BACNET_STACK_EXPORT
void datetime_set(
    BACNET_DATE_TIME *bdatetime,
    const BACNET_DATE *bdate,
    const BACNET_TIME *btime);
BACNET_STACK_EXPORT
void datetime_set_values(
    BACNET_DATE_TIME *bdatetime,
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
bool datetime_is_valid(const BACNET_DATE *bdate, const BACNET_TIME *btime);
BACNET_STACK_EXPORT
bool datetime_time_is_valid(const BACNET_TIME *btime);
BACNET_STACK_EXPORT
bool datetime_date_is_valid(const BACNET_DATE *bdate);
/* date and time calculations and summaries */
BACNET_STACK_EXPORT
void datetime_ymd_from_days_since_epoch(
    uint32_t days, uint16_t *pYear, uint8_t *pMonth, uint8_t *pDay);
BACNET_STACK_EXPORT
uint32_t datetime_days_since_epoch(const BACNET_DATE *bdate);
BACNET_STACK_EXPORT
void datetime_days_since_epoch_into_date(uint32_t days, BACNET_DATE *bdate);
BACNET_STACK_EXPORT
uint32_t datetime_day_of_year(const BACNET_DATE *bdate);
BACNET_STACK_EXPORT
uint32_t
datetime_ymd_to_days_since_epoch(uint16_t year, uint8_t month, uint8_t day);
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
uint32_t datetime_seconds_since_midnight(const BACNET_TIME *btime);
BACNET_STACK_EXPORT
uint16_t datetime_minutes_since_midnight(const BACNET_TIME *btime);

/* utility comparison functions:
   if the date/times are the same, return is 0
   if date1 is before date2, returns negative
   if date1 is after date2, returns positive */
BACNET_STACK_EXPORT
int datetime_compare_date(const BACNET_DATE *date1, const BACNET_DATE *date2);
BACNET_STACK_EXPORT
int datetime_compare_time(const BACNET_TIME *time1, const BACNET_TIME *time2);
BACNET_STACK_EXPORT
int datetime_compare(
    const BACNET_DATE_TIME *datetime1, const BACNET_DATE_TIME *datetime2);

/* full comparison functions:
 * taking into account FF fields in date and time structures,
 * do a full comparison of two values */
BACNET_STACK_EXPORT
int datetime_wildcard_compare_date(
    const BACNET_DATE *date1, const BACNET_DATE *date2);
BACNET_STACK_EXPORT
int datetime_wildcard_compare_time(
    const BACNET_TIME *time1, const BACNET_TIME *time2);
BACNET_STACK_EXPORT
int datetime_wildcard_compare(
    const BACNET_DATE_TIME *datetime1, const BACNET_DATE_TIME *datetime2);

/* utility copy functions */
BACNET_STACK_EXPORT
void datetime_copy_date(BACNET_DATE *dest, const BACNET_DATE *src);
BACNET_STACK_EXPORT
void datetime_copy_time(BACNET_TIME *dest, const BACNET_TIME *src);
BACNET_STACK_EXPORT
void datetime_copy(BACNET_DATE_TIME *dest, const BACNET_DATE_TIME *src);

/* utility add or subtract time function */
BACNET_STACK_EXPORT
void datetime_add_minutes(BACNET_DATE_TIME *bdatetime, int32_t minutes);
BACNET_STACK_EXPORT
void datetime_add_seconds(BACNET_DATE_TIME *bdatetime, int32_t seconds);
BACNET_STACK_EXPORT
void datetime_add_milliseconds(
    BACNET_DATE_TIME *bdatetime, int32_t milliseconds);

BACNET_STACK_EXPORT
bacnet_time_t datetime_seconds_since_epoch(const BACNET_DATE_TIME *bdatetime);
BACNET_STACK_EXPORT
void datetime_since_epoch_seconds(
    BACNET_DATE_TIME *bdatetime, bacnet_time_t seconds);
BACNET_STACK_EXPORT
bacnet_time_t datetime_seconds_since_epoch_max(void);

/* date and time wildcards */
BACNET_STACK_EXPORT
bool datetime_wildcard_year(const BACNET_DATE *bdate);
BACNET_STACK_EXPORT
void datetime_wildcard_year_set(BACNET_DATE *bdate);
BACNET_STACK_EXPORT
bool datetime_wildcard_month(const BACNET_DATE *bdate);
BACNET_STACK_EXPORT
void datetime_wildcard_month_set(BACNET_DATE *bdate);
BACNET_STACK_EXPORT
bool datetime_wildcard_day(const BACNET_DATE *bdate);
BACNET_STACK_EXPORT
void datetime_wildcard_day_set(BACNET_DATE *bdate);
BACNET_STACK_EXPORT
bool datetime_wildcard_weekday(const BACNET_DATE *bdate);
BACNET_STACK_EXPORT
void datetime_wildcard_weekday_set(BACNET_DATE *bdate);
BACNET_STACK_EXPORT
bool datetime_wildcard_hour(const BACNET_TIME *btime);
BACNET_STACK_EXPORT
void datetime_wildcard_hour_set(BACNET_TIME *btime);
BACNET_STACK_EXPORT
bool datetime_wildcard_minute(const BACNET_TIME *btime);
BACNET_STACK_EXPORT
void datetime_wildcard_minute_set(BACNET_TIME *btime);
BACNET_STACK_EXPORT
bool datetime_wildcard_second(const BACNET_TIME *btime);
BACNET_STACK_EXPORT
void datetime_wildcard_second_set(BACNET_TIME *btime);
BACNET_STACK_EXPORT
bool datetime_wildcard_hundredths(const BACNET_TIME *btime);
BACNET_STACK_EXPORT
void datetime_wildcard_hundredths_set(BACNET_TIME *btime);
BACNET_STACK_EXPORT
bool datetime_wildcard(const BACNET_DATE_TIME *bdatetime);
BACNET_STACK_EXPORT
bool datetime_wildcard_present(const BACNET_DATE_TIME *bdatetime);
BACNET_STACK_EXPORT
void datetime_wildcard_set(BACNET_DATE_TIME *bdatetime);
BACNET_STACK_EXPORT
void datetime_date_wildcard_set(BACNET_DATE *bdate);
BACNET_STACK_EXPORT
void datetime_time_wildcard_set(BACNET_TIME *btime);

BACNET_STACK_EXPORT
bool datetime_local_to_utc(
    BACNET_DATE_TIME *utc_time,
    const BACNET_DATE_TIME *local_time,
    int16_t utc_offset_minutes,
    int8_t dst_adjust_minutes);
BACNET_STACK_EXPORT
bool datetime_utc_to_local(
    BACNET_DATE_TIME *local_time,
    const BACNET_DATE_TIME *utc_time,
    int16_t utc_offset_minutes,
    int8_t dst_adjust_minutes);

BACNET_STACK_EXPORT
bool datetime_date_init_ascii(BACNET_DATE *bdate, const char *ascii);
BACNET_STACK_EXPORT
int datetime_date_to_ascii(
    const BACNET_DATE *bdate, char *str, size_t str_size);
BACNET_STACK_EXPORT
bool datetime_time_init_ascii(BACNET_TIME *btime, const char *ascii);
BACNET_STACK_EXPORT
int datetime_time_to_ascii(
    const BACNET_TIME *btime, char *str, size_t str_size);
BACNET_STACK_EXPORT
bool datetime_init_ascii(BACNET_DATE_TIME *bdatetime, const char *ascii);
BACNET_STACK_EXPORT
int datetime_to_ascii(
    const BACNET_DATE_TIME *bdatetime, char *str, size_t str_size);

BACNET_STACK_EXPORT
int bacapp_encode_datetime(uint8_t *apdu, const BACNET_DATE_TIME *value);
BACNET_STACK_EXPORT
int bacapp_encode_context_datetime(
    uint8_t *apdu, uint8_t tag_number, const BACNET_DATE_TIME *value);
BACNET_STACK_EXPORT
int bacnet_datetime_decode(
    const uint8_t *apdu, uint32_t apdu_size, BACNET_DATE_TIME *value);
BACNET_STACK_EXPORT
int bacnet_datetime_context_decode(
    const uint8_t *apdu,
    uint32_t apdu_size,
    uint8_t tag_number,
    BACNET_DATE_TIME *value);

BACNET_STACK_DEPRECATED("Use bacnet_datetime_decode() instead")
BACNET_STACK_EXPORT
int bacapp_decode_datetime(const uint8_t *apdu, BACNET_DATE_TIME *value);
BACNET_STACK_DEPRECATED("Use bacnet_datetime_context_decode() instead")
BACNET_STACK_EXPORT
int bacapp_decode_context_datetime(
    const uint8_t *apdu, uint8_t tag_number, BACNET_DATE_TIME *value);

BACNET_STACK_EXPORT
bool bacnet_daterange_same(
    const BACNET_DATE_RANGE *value1, const BACNET_DATE_RANGE *value2);
BACNET_STACK_EXPORT
int bacnet_daterange_encode(uint8_t *apdu, const BACNET_DATE_RANGE *value);
BACNET_STACK_EXPORT
int bacnet_daterange_decode(
    const uint8_t *apdu, uint32_t apdu_size, BACNET_DATE_RANGE *value);
BACNET_STACK_EXPORT
int bacnet_daterange_context_encode(
    uint8_t *apdu, uint8_t tag_number, const BACNET_DATE_RANGE *value);
BACNET_STACK_EXPORT
int bacnet_daterange_context_decode(
    const uint8_t *apdu,
    uint32_t apdu_size,
    uint8_t tag_number,
    BACNET_DATE_RANGE *value);

/* implementation agnostic functions for clocks - create your own! */
BACNET_STACK_EXPORT
bool datetime_local(
    BACNET_DATE *bdate,
    BACNET_TIME *btime,
    int16_t *utc_offset_minutes,
    bool *dst_active);
/* UTC Offset API */
BACNET_STACK_EXPORT
int16_t datetime_utc_offset_minutes(void);
BACNET_STACK_EXPORT
bool datetime_utc_offset_minutes_set(int16_t minutes);
/* Daylight Savings Time API */
BACNET_STACK_EXPORT
bool datetime_dst_enabled(void);
BACNET_STACK_EXPORT
void datetime_dst_enabled_set(bool flag);
BACNET_STACK_EXPORT
bool datetime_dst_ordinal_range(
    uint8_t *start_month,
    uint8_t *start_week,
    uint8_t *start_day,
    uint8_t *end_month,
    uint8_t *end_week,
    uint8_t *end_day);
BACNET_STACK_EXPORT
bool datetime_dst_ordinal_range_set(
    uint8_t start_month,
    uint8_t start_week,
    BACNET_WEEKDAY start_day,
    uint8_t end_month,
    uint8_t end_week,
    BACNET_WEEKDAY end_day);
BACNET_STACK_EXPORT
bool datetime_dst_date_range(
    uint8_t *start_month,
    uint8_t *start_day,
    uint8_t *end_month,
    uint8_t *end_day);
BACNET_STACK_EXPORT
bool datetime_dst_date_range_set(
    uint8_t start_month, uint8_t start_day, uint8_t end_month, uint8_t end_day);
/* BACnet TimeSynchronization service handler API */
BACNET_STACK_EXPORT
void datetime_timesync(BACNET_DATE *bdate, BACNET_TIME *btime, bool utc);
/* Initialization for integration with a clock */
BACNET_STACK_EXPORT
void datetime_init(void);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif
