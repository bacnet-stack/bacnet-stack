/**************************************************************************
*
* Copyright (C) 2006 Steve Karg <skarg@users.sourceforge.net>
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
*
*********************************************************************/
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include "config.h"
#include "device.h"
#include "datetime.h"
#include "bacdef.h"
#include "bacdcode.h"
#include "timesync.h"
#include "handlers.h"
#include "client.h"
#include "bacaddr.h"

/** @file h_ts.c  Handles TimeSync requests. */

#if defined(BACNET_TIME_MASTER)
/* sending time sync to recipients */
#ifndef MAX_TIME_SYNC_RECIPIENTS
#define MAX_TIME_SYNC_RECIPIENTS 16
#endif
BACNET_RECIPIENT_LIST Time_Sync_Recipients[MAX_TIME_SYNC_RECIPIENTS];
/* variable used for controlling when to
   automatically send a TimeSynchronization request */
static BACNET_DATE_TIME Next_Sync_Time;
#endif

#if PRINT_ENABLED
static void show_bacnet_date_time(
    BACNET_DATE * bdate,
    BACNET_TIME * btime)
{
    /* show the date received */
    fprintf(stderr, "%u", (unsigned) bdate->year);
    fprintf(stderr, "/%u", (unsigned) bdate->month);
    fprintf(stderr, "/%u", (unsigned) bdate->day);
    /* show the time received */
    fprintf(stderr, " %02u", (unsigned) btime->hour);
    fprintf(stderr, ":%02u", (unsigned) btime->min);
    fprintf(stderr, ":%02u", (unsigned) btime->sec);
    fprintf(stderr, ".%02u", (unsigned) btime->hundredths);
    fprintf(stderr, "\r\n");
}
#endif

void handler_timesync(
    uint8_t * service_request,
    uint16_t service_len,
    BACNET_ADDRESS * src)
{
    int len = 0;
    BACNET_DATE bdate = {0};
    BACNET_TIME btime = {0};

    (void) src;
    (void) service_len;
    len =
        timesync_decode_service_request(service_request, service_len, &bdate,
        &btime);
    if (len > 0) {
        if (datetime_is_valid(&bdate, &btime)) {
            /* fixme: only set the time if off by some amount */
#if PRINT_ENABLED
            fprintf(stderr, "Received TimeSyncronization Request\r\n");
            show_bacnet_date_time(&bdate, &btime);
#else
            /* FIXME: set the time?
               Maybe only set the time if off by some amount */
#endif
        }
    }

    return;
}

void handler_timesync_utc(
    uint8_t * service_request,
    uint16_t service_len,
    BACNET_ADDRESS * src)
{
    int len = 0;
    BACNET_DATE bdate;
    BACNET_TIME btime;

    (void) src;
    (void) service_len;
    len =
        timesync_decode_service_request(service_request, service_len, &bdate,
        &btime);
    if (len > 0) {
        if (datetime_is_valid(&bdate, &btime)) {
#if PRINT_ENABLED
            fprintf(stderr, "Received TimeSyncronization Request\r\n");
            show_bacnet_date_time(&bdate, &btime);
#endif
            /* FIXME: set the time?
               only set the time if off by some amount */
        }
    }

    return;
}

#if defined(BACNET_TIME_MASTER)
/** Handle a request to list all the timesync recipients.
 *
 *  Invoked by a request to read the Device object's
 *  PROP_TIME_SYNCHRONIZATION_RECIPIENTS.
 *  Loops through the list of timesync recipients, and, for each valid one,
 *  adds its data to the APDU.
 *
 *  @param apdu [out] Buffer in which the APDU contents are built.
 *  @param max_apdu [in] Max length of the APDU buffer.
 *
 *  @return How many bytes were encoded in the buffer, or
 *   BACNET_STATUS_ABORT if the response would not fit within the buffer.
 */
int handler_timesync_encode_recipients(
    uint8_t * apdu,
    int max_apdu)
{
    return timesync_encode_timesync_recipients(apdu, max_apdu,
        &Time_Sync_Recipients[0]);
}
#endif

#if defined(BACNET_TIME_MASTER)
bool handler_timesync_recipient_write(
    BACNET_WRITE_PROPERTY_DATA * wp_data)
{
    bool status = false;

    /* fixme: handle writing of the recipient list */
    wp_data->error_class = ERROR_CLASS_PROPERTY;
    wp_data->error_code = ERROR_CODE_WRITE_ACCESS_DENIED;

    return status;
}
#endif

#if defined(BACNET_TIME_MASTER)
static void handler_timesync_send(
    BACNET_DATE_TIME * current_date_time)
{
    unsigned index = 0;
    bool status = false;

    for (index = 0; index < MAX_TIME_SYNC_RECIPIENTS; index++) {
        if (Time_Sync_Recipients[index].tag == 1) {
            if (status) {
                Send_TimeSync_Remote(
                    &Time_Sync_Recipients[index].type.address,
                    &current_date_time->date,
                    &current_date_time->time);
            }
        }
    }
}
#endif

#if defined(BACNET_TIME_MASTER)
static void handler_timesync_update(
    uint32_t device_interval,
    BACNET_DATE_TIME * current_date_time)
{
    uint32_t current_minutes = 0;
    uint32_t next_minutes = 0;
    uint32_t delta_minutes = 0;
    uint32_t offset_minutes = 0;
    uint32_t interval = 0;
    uint32_t interval_offset = 0;

    datetime_copy(&Next_Sync_Time, current_date_time);
    if (Device_Align_Intervals()) {
        interval_offset = Device_Interval_Offset();
        /* If periodic time synchronization is enabled and
           the time synchronization interval is a factor of
           (divides without remainder) an hour or day, then
           the beginning of the period specified for time
           synchronization shall be aligned to the hour or
           day, respectively. */
        if ((60 % device_interval) == 0) {
            /* factor of an hour alignment */
            /* Interval_Minutes = 1  2  3  4  5  6  10  12  15  20  30  60 */
            /* determine next interval */
            current_minutes = Next_Sync_Time.time.min;
            interval = current_minutes/device_interval;
            interval++;
            next_minutes = interval * device_interval;
            offset_minutes = interval_offset % device_interval;
            next_minutes += offset_minutes;
            delta_minutes = next_minutes - current_minutes;
            datetime_add_minutes(&Next_Sync_Time, delta_minutes);
            Next_Sync_Time.time.sec = 0;
            Next_Sync_Time.time.hundredths = 0;
        } else if ((1440 % device_interval) == 0) {
            /* factor of a day alignment */
            /* Interval_Minutes = 1  2  3  4  5  6  8  9  10  12  15  16
               18  20  24  30  32  36  40  45  48  60  72  80  90  96  120
               144  160  180  240  288  360  480  720  1440   */
            current_minutes =
                datetime_minutes_since_midnight(&Next_Sync_Time.time);
            interval = current_minutes/device_interval;
            interval++;
            next_minutes = interval * device_interval;
            offset_minutes = interval_offset % device_interval;
            next_minutes += offset_minutes;
            delta_minutes = next_minutes - current_minutes;
            datetime_add_minutes(&Next_Sync_Time, delta_minutes);
            Next_Sync_Time.time.sec = 0;
            Next_Sync_Time.time.hundredths = 0;
        }
    } else {
        datetime_add_minutes(&Next_Sync_Time, device_interval);
        Next_Sync_Time.time.sec = 0;
        Next_Sync_Time.time.hundredths = 0;
    }
}
#endif

#if defined(BACNET_TIME_MASTER)
bool handler_timesync_recipient_address_set(
    unsigned index,
    BACNET_ADDRESS *address)
{
    bool status = false;

    if (address && (index < MAX_TIME_SYNC_RECIPIENTS)) {
        Time_Sync_Recipients[index].tag = 1;
        bacnet_address_copy(
            &Time_Sync_Recipients[index].type.address,
            address);
        status = true;
    }

    return status;
}
#endif

#if defined(BACNET_TIME_MASTER)
void handler_timesync_task(
    BACNET_DATE_TIME * current_date_time)
{
    int compare = 0;
    uint32_t device_interval = 0;

    device_interval = Device_Time_Sync_Interval();
    if (device_interval) {
        compare = datetime_compare(current_date_time, &Next_Sync_Time);
       /* if the date/times are the same, return is 0
          if date1 is before date2, returns negative
          if date1 is after date2, returns positive */
        if (compare >= 0) {
            handler_timesync_update(device_interval, current_date_time);
            handler_timesync_send(current_date_time);
        }
    }
}
#endif

#if defined(BACNET_TIME_MASTER)
void handler_timesync_init(void)
{
    unsigned i = 0;

    /* connect linked list */
    for (; i < (MAX_TIME_SYNC_RECIPIENTS-1); i++) {
        Time_Sync_Recipients[i].next = &Time_Sync_Recipients[i+1];
        Time_Sync_Recipients[i+1].next = NULL;
    }
    for (i = 0; i < MAX_TIME_SYNC_RECIPIENTS; i++) {
        Time_Sync_Recipients[i].tag = 0xFF;
    }
}
#endif
