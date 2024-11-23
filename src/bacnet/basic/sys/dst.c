/**
 * @file
 * @brief computes whether day is during daylight savings time
 * @note Public domain algorithms from ACM
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date 1997
 * @copyright SPDX-License-Identifier: CC-PDDC
 */
#include <stdint.h>
#include <stdbool.h>
#include "days.h"
#include "dst.h"

/**
 * This function returns the number of seconds after midnight
 *
 * @param hours - hours after midnight (0..23)
 * @param minutes - minutes after hour (0..59)
 * @param seconds - holds seconds after minute (0..59)
 *
 * @return true if date-time falls in DST.  false if not.
 */
static inline uint32_t
time_to_seconds(uint32_t hours, uint32_t minutes, uint32_t seconds)
{
    return (((hours) * 60 * 60) + ((minutes) * 60) + (seconds));
}

/**
 * This function returns the day of the month for starting the Nth week
 *
 * @param year - Year of our Lord A.D. (1900..9999)
 * @param month - months of the year (1=Jan,...,12=Dec)
 * @param ordinal - Ordinal Day of the Month
 *  1=1st, 2=2nd, 3=3rd, 4=4th, or 5=LAST
 * @return day of the month (1..31)
 */
static inline uint8_t
ordinal_week_month_day(uint16_t year, uint8_t month, uint8_t ordinal)
{
    uint8_t day = 0;

    if (ordinal == 5) {
        /* last week of the month */
        day = days_per_month(year, month) - 6;
    } else {
        if (ordinal) {
            ordinal--;
            day = 1 + (ordinal * 7);
        }
    }

    return day;
}

/**
 * This function returns true if the date-time is during DST
 *
 * @param year - Year of our Lord A.D. (2000,2001,..2099)
 * @param month - months of the year (1=Jan,...,12=Dec)
 * @param day - day of the month (1..31)
 * @param hour - hours after midnight (0..23)
 * @param minute - minutes after hour (0..59)
 * @param second - holds seconds after minute (0..59)
 *
 * @return true if date-time falls in DST.  false if not.
 */
bool dst_active(
    struct daylight_savings_data *data,
    uint16_t year,
    uint8_t month,
    uint8_t day,
    uint8_t hour,
    uint8_t minute,
    uint8_t second)
{
    bool active = false;
    uint8_t i = 0;
    uint32_t time_now = 0;
    uint32_t time_dst = 0;
    uint8_t day_of_week = 0;
    uint8_t days = 0;
    uint32_t days_begin = 0;
    uint32_t days_now = 0;
    uint32_t days_end = 0;

    if (data->Ordinal) {
        if ((month >= data->Begin_Month) && (month <= data->End_Month)) {
            if (month == data->Begin_Month) {
                days = days_per_month(year, month);
                i = ordinal_week_month_day(year, month, data->Begin_Which_Day);
                for (; i <= days; i++) {
                    day_of_week = days_of_week(
                        data->Epoch_Day,
                        days_since_epoch(data->Epoch_Year, year, month, i));
                    if (day_of_week == data->Begin_Day) {
                        if (day == i) {
                            time_now = time_to_seconds(hour, minute, second);
                            /* begins at 2 AM Standard Time */
                            time_dst = time_to_seconds(2, 0, 0);
                            if (time_now >= time_dst) {
                                active = true;
                            }
                        } else if (day > i) {
                            active = true;
                        }
                        break;
                    }
                }
            } else if (month == data->End_Month) {
                days = days_per_month(year, month);
                i = ordinal_week_month_day(year, month, data->End_Which_Day);
                for (; i <= days; i++) {
                    day_of_week = days_of_week(
                        data->Epoch_Day,
                        days_since_epoch(data->Epoch_Year, year, month, i));
                    if (day_of_week == data->End_Day) {
                        if (day == i) {
                            time_now = time_to_seconds(hour, minute, second);
                            /* ends at 2 AM Daylight time,
                               which is 1 AM Standard Time */
                            time_dst = time_to_seconds(1, 0, 0);
                            if (time_now < time_dst) {
                                active = true;
                            }
                        } else if (day < i) {
                            active = true;
                        }
                        break;
                    }
                }
            } else {
                /* months between the beginning and end months */
                active = true;
            }
        }
    } else {
        days_now = days_since_epoch(data->Epoch_Year, year, month, day);
        days_begin = days_since_epoch(
            data->Epoch_Year, year, data->Begin_Month, data->Begin_Day);
        days_end = days_since_epoch(
            data->Epoch_Year, year, data->End_Month, data->End_Day);
        if ((days_now >= days_begin) && (days_now <= days_end)) {
            if (days_now == days_begin) {
                time_now = time_to_seconds(hour, minute, second);
                /* begins at 2 AM Standard Time */
                time_dst = time_to_seconds(2, 0, 0);
                if (time_now >= time_dst) {
                    active = true;
                }
            } else if (days_now == days_end) {
                time_now = time_to_seconds(hour, minute, second);
                /* ends at 2 AM Daylight time,
                   which is 1 AM Standard Time */
                time_dst = time_to_seconds(1, 0, 0);
                if (time_now < time_dst) {
                    active = true;
                }
            } else {
                active = true;
            }
        }
    }

    return active;
}

/**
 * @brief This function sets the daylight savings time parameters
 * @param data - daylight savings time data
 * @param ordinal - true when ordinal day of month used, false if specific dates
 * @param begin_month - month DST begins 1=Jan,...,12=Dec
 * @param begin_day - day of the month DST begins
 *  1..31 for specific day, or day of week 1..7 for ordinal day
 * @param begin_which_day - which ordinal day of the month is used
 *  1=1st, 2=2nd, 3=3rd, 4=4th, or 5=LAST
 * @param end_month - month DST ends 1=Jan,...,12=Dec
 * @param end_day - day of the month DST ends
 *  1..31 for specific day, or day of week 1..7 for ordinal day
 * @param end_which_day - which ordinal day of the month is used
 *  1=1st, 2=2nd, 3=3rd, 4=4th, or 5=LAST
 * @param epoch_day - day of the week for the BACnet Epoch (1=Monday..7=Sunday)
 * @param epoch_year - year of the BACnet Epoch (1900..9999)
 */
void dst_init(
    struct daylight_savings_data *data,
    bool ordinal,
    uint8_t begin_month,
    uint8_t begin_day,
    uint8_t begin_which_day,
    uint8_t end_month,
    uint8_t end_day,
    uint8_t end_which_day,
    uint8_t epoch_day,
    uint16_t epoch_year)
{
    if (data) {
        data->Ordinal = ordinal;
        data->Begin_Month = begin_month;
        data->Begin_Day = begin_day;
        data->Begin_Which_Day = begin_which_day;
        data->End_Month = end_month;
        data->End_Day = end_day;
        data->End_Which_Day = end_which_day;
        data->Epoch_Day = epoch_day;
        data->Epoch_Year = epoch_year;
    }
}

/**
 * Initializes the daylight savings time parameters to their defaults.
 */
void dst_init_defaults(struct daylight_savings_data *data)
{
    if (data) {
        /* Starts: Second=2 Sunday=7 in March=3 */
        /* Ends: First=1 Sunday=7 in November=11 */
        data->Ordinal = true;
        data->Begin_Month = 3;
        data->Begin_Day = 7;
        data->Begin_Which_Day = 2;
        data->End_Month = 11;
        data->End_Day = 7;
        data->End_Which_Day = 1;
        /* BACnet Epoch */
        data->Epoch_Day = 1 /* Monday=1 */;
        data->Epoch_Year = 1900;
    }
}
