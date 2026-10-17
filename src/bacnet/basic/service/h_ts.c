/**************************************************************************
 *
 * Copyright (C) 2006 Steve Karg <skarg@users.sourceforge.net>
 *
 * SPDX-License-Identifier: MIT
 *
 *********************************************************************/
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
/* BACnet Stack defines - first */
#include "bacnet/bacdef.h"
/* BACnet Stack API */
#include "bacnet/datetime.h"
#include "bacnet/bacdcode.h"
#include "bacnet/bacdest.h"
#include "bacnet/timesync.h"
#include "bacnet/bacaddr.h"
#include "bacnet/npdu.h"
#include "bacnet/basic/object/device.h"
#include "bacnet/basic/services.h"
#include "bacnet/basic/tsm/tsm.h"
#if defined(BACNET_ADDRESS_CACHE)
#include "bacnet/basic/binding/address.h"
#endif

/** @file h_ts.c  Handles TimeSync requests. */

#if defined(BACNET_TIME_MASTER)
/* sending time sync to recipients */
#ifndef MAX_TIME_SYNC_RECIPIENTS
#define MAX_TIME_SYNC_RECIPIENTS 16
#endif
BACNET_RECIPIENT_LIST Time_Sync_Recipients[MAX_TIME_SYNC_RECIPIENTS];
static BACNET_RECIPIENT_LIST Time_Sync_UTC_Recipients[MAX_TIME_SYNC_RECIPIENTS];
/* variable used for controlling when to
   automatically send a TimeSynchronization request */
static BACNET_DATE_TIME Next_Sync_Time;
#endif

#if PRINT_ENABLED
/**
 * @brief Print a BACnet date and time to standard error.
 * @param bdate [in] Date to print
 * @param btime [in] Time to print
 */
static void
show_bacnet_date_time(const BACNET_DATE *bdate, const BACNET_TIME *btime)
{
    /* show the date received */
    fprintf(stderr, "%u", (unsigned)bdate->year);
    fprintf(stderr, "/%u", (unsigned)bdate->month);
    fprintf(stderr, "/%u", (unsigned)bdate->day);
    /* show the time received */
    fprintf(stderr, " %02u", (unsigned)btime->hour);
    fprintf(stderr, ":%02u", (unsigned)btime->min);
    fprintf(stderr, ":%02u", (unsigned)btime->sec);
    fprintf(stderr, ".%02u", (unsigned)btime->hundredths);
    fprintf(stderr, "\r\n");
}
#endif

/* Callback for timesync set */
static handler_timesync_set_callback_t handler_timesync_set_callback;

/**
 * @brief Handle a local TimeSynchronization service request.
 * @param service_request [in] Encoded service request
 * @param service_len [in] Length of the service request
 * @param src [in] Request source address
 *
 * Calls the registered set callback only when decoding and date/time
 * validation succeed.
 */
void handler_timesync(
    uint8_t *service_request, uint16_t service_len, BACNET_ADDRESS *src)
{
    int len = 0;
    BACNET_DATE bdate = { 0 };
    BACNET_TIME btime = { 0 };

    (void)src;
    (void)service_len;
    len = timesync_decode_service_request(
        service_request, service_len, &bdate, &btime);
    if (len > 0) {
        if (datetime_is_valid(&bdate, &btime)) {
            if (handler_timesync_set_callback) {
                handler_timesync_set_callback(&bdate, &btime, false);
            }
#if PRINT_ENABLED
            fprintf(stderr, "Received Local TimeSyncronization Request\r\n");
            show_bacnet_date_time(&bdate, &btime);
#endif
        }
    }

    return;
}

/**
 * @brief Handle a UTC TimeSynchronization service request.
 * @param service_request [in] Encoded service request
 * @param service_len [in] Length of the service request
 * @param src [in] Request source address
 *
 * Calls the registered set callback only when decoding and date/time
 * validation succeed.
 */
void handler_timesync_utc(
    uint8_t *service_request, uint16_t service_len, BACNET_ADDRESS *src)
{
    int len = 0;
    BACNET_DATE bdate;
    BACNET_TIME btime;

    (void)src;
    (void)service_len;
    len = timesync_decode_service_request(
        service_request, service_len, &bdate, &btime);
    if (len > 0) {
        if (datetime_is_valid(&bdate, &btime)) {
            if (handler_timesync_set_callback) {
                handler_timesync_set_callback(&bdate, &btime, true);
            }
#if PRINT_ENABLED
            fprintf(stderr, "Received UTC TimeSyncronization Request\r\n");
            show_bacnet_date_time(&bdate, &btime);
#endif
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
int handler_timesync_encode_recipients(uint8_t *apdu, int max_apdu)
{
    return timesync_encode_timesync_recipients(
        apdu, max_apdu, &Time_Sync_Recipients[0]);
}

/**
 * @brief Encode the UTC TimeSynchronization recipient list.
 * @param apdu [out] Buffer receiving the encoded list
 * @param max_apdu [in] Maximum length of the output buffer
 * @return Number of bytes encoded, or BACNET_STATUS_ABORT if it does not fit
 */
int handler_timesync_encode_utc_recipients(uint8_t *apdu, int max_apdu)
{
    return timesync_encode_timesync_recipients(
        apdu, max_apdu, &Time_Sync_UTC_Recipients[0]);
}
#endif

#if defined(BACNET_TIME_MASTER)
/**
 * @brief Return true when a recipient can be stored.
 * @param recipient [in] Recipient to check
 * @return true when the recipient tag is supported
 *
 * Address recipients are supported. Device recipients are supported
 * when BACNET_ADDRESS_CACHE is enabled.
 */
static bool
handler_timesync_recipient_supported(const BACNET_RECIPIENT *recipient)
{
    bool status = false;

    if (recipient) {
        if (recipient->tag == BACNET_RECIPIENT_TAG_ADDRESS) {
            status = true;
#if defined(BACNET_ADDRESS_CACHE)
        } else if (recipient->tag == BACNET_RECIPIENT_TAG_DEVICE) {
            status = true;
#endif
        }
    }

    return status;
}

/**
 * @brief Resolve a recipient to a destination address.
 * @param recipient [in] Recipient to resolve
 * @param dest [out] Destination address
 * @return true when dest contains an address to use
 */
static bool handler_timesync_recipient_destination(
    const BACNET_RECIPIENT *recipient, BACNET_ADDRESS *dest)
{
    bool status = false;

    if (recipient && dest) {
        if (recipient->tag == BACNET_RECIPIENT_TAG_ADDRESS) {
            bacnet_address_copy(dest, &recipient->type.address);
            status = true;
#if defined(BACNET_ADDRESS_CACHE)
        } else if (bacnet_recipient_device_valid(recipient)) {
            status = address_get_by_device(
                recipient->type.device.instance, NULL, dest);
#endif
        }
    }

    return status;
}

/**
 * @brief Decode and replace a TimeSynchronization recipient list.
 * @param wp_data [in,out] WriteProperty data and error result
 * @param list [out] Recipient list to replace after successful decoding
 * @return true if the complete list was decoded and stored
 */
static bool handler_timesync_recipient_list_write(
    BACNET_WRITE_PROPERTY_DATA *wp_data, BACNET_RECIPIENT_LIST *list)
{
    BACNET_RECIPIENT_LIST decoded[MAX_TIME_SYNC_RECIPIENTS];
    BACNET_RECIPIENT_LIST *entry = NULL;
    int apdu_length = 0;
    int length = 0;
    unsigned count = 0;
    unsigned index = 0;
    bool status = false;

    if (!wp_data || !list) {
        return false;
    }
    if ((wp_data->application_data_len < 0) ||
        (wp_data->application_data_len > MAX_APDU)) {
        wp_data->error_class = ERROR_CLASS_PROPERTY;
        wp_data->error_code = ERROR_CODE_INVALID_DATA_TYPE;
        return false;
    }
    if (wp_data->array_index != BACNET_ARRAY_ALL) {
        wp_data->error_class = ERROR_CLASS_PROPERTY;
        wp_data->error_code = ERROR_CODE_PROPERTY_IS_NOT_AN_ARRAY;
        return false;
    }
    if (wp_data->application_data_len > 0) {
        bacnet_recipient_list_link_array(decoded, MAX_TIME_SYNC_RECIPIENTS);
        for (index = 0; index < MAX_TIME_SYNC_RECIPIENTS; index++) {
            decoded[index].recipient.tag = BACNET_RECIPIENT_TAG_MAX;
        }
        while (apdu_length < wp_data->application_data_len) {
            if (count >= MAX_TIME_SYNC_RECIPIENTS) {
                break;
            }
            entry = &decoded[count];
            length = bacnet_recipient_decode(
                &wp_data->application_data[apdu_length],
                wp_data->application_data_len - apdu_length, &entry->recipient);
            if (length <= 0) {
                break;
            }
            apdu_length += length;
            count++;
        }
        if (apdu_length != wp_data->application_data_len) {
            wp_data->error_class = ERROR_CLASS_PROPERTY;
            wp_data->error_code = ERROR_CODE_INVALID_DATA_TYPE;
            return false;
        }
        for (index = 0; index < count; index++) {
            if (!handler_timesync_recipient_supported(
                    &decoded[index].recipient)) {
                wp_data->error_class = ERROR_CLASS_PROPERTY;
                wp_data->error_code =
                    ERROR_CODE_OPTIONAL_FUNCTIONALITY_NOT_SUPPORTED;
                return false;
            }
        }
    }
    for (index = 0; index < MAX_TIME_SYNC_RECIPIENTS; index++) {
        list[index].recipient.tag = BACNET_RECIPIENT_TAG_MAX;
        if (index < count) {
            list[index].recipient = decoded[index].recipient;
        }
    }
    status = true;

    return status;
}

/**
 * @brief Write the local TimeSynchronization recipient list.
 * @param wp_data [in,out] WriteProperty data and error result
 * @return true if the recipient list was accepted
 */
bool handler_timesync_recipient_write(BACNET_WRITE_PROPERTY_DATA *wp_data)
{
    return handler_timesync_recipient_list_write(
        wp_data, &Time_Sync_Recipients[0]);
}

/**
 * @brief Write the UTC TimeSynchronization recipient list.
 * @param wp_data [in,out] WriteProperty data and error result
 * @return true if the recipient list was accepted
 */
bool handler_timesync_utc_recipient_write(BACNET_WRITE_PROPERTY_DATA *wp_data)
{
    return handler_timesync_recipient_list_write(
        wp_data, &Time_Sync_UTC_Recipients[0]);
}
#endif

#if defined(BACNET_TIME_MASTER)
/**
 * @brief Send local and UTC TimeSynchronization requests.
 * @param current_date_time [in] Current local date and time
 *
 * Address recipients are sent directly. Device recipients are sent
 * when address binding resolves them. Unresolved devices are skipped.
 */
static void handler_timesync_send(BACNET_DATE_TIME *current_date_time)
{
    unsigned index = 0;
    BACNET_ADDRESS dest = { 0 };
    BACNET_DATE_TIME local_time = { 0 };
    BACNET_DATE_TIME utc_time = { 0 };
    int16_t utc_offset = 0;
    int8_t dst_adjust = 0;
    bool zoned = false;

    /* Local time, standard offset, and daylight adjustment come from one
       sample. The adjustment is the host's daylight value. */
    zoned = datetime_zone(&local_time, &utc_offset, &dst_adjust);
    if (!zoned) {
        datetime_copy(&local_time, current_date_time);
    }
    for (index = 0; index < MAX_TIME_SYNC_RECIPIENTS; index++) {
        if (handler_timesync_recipient_destination(
                &Time_Sync_Recipients[index].recipient, &dest)) {
            Send_TimeSync_Remote(&dest, &local_time.date, &local_time.time);
        }
    }
    if (!zoned ||
        !datetime_local_to_utc(
            &utc_time, &local_time, utc_offset, dst_adjust)) {
        return;
    }
    for (index = 0; index < MAX_TIME_SYNC_RECIPIENTS; index++) {
        if (handler_timesync_recipient_destination(
                &Time_Sync_UTC_Recipients[index].recipient, &dest)) {
            Send_TimeSyncUTC_Remote(&dest, &utc_time.date, &utc_time.time);
        }
    }
}
#endif

#if defined(BACNET_TIME_MASTER)
/**
 * @brief Calculate the next TimeSynchronization send time.
 * @param device_interval [in] Synchronization interval in minutes
 * @param current_date_time [in] Current local date and time
 */
static void handler_timesync_update(
    uint32_t device_interval, BACNET_DATE_TIME *current_date_time)
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
            interval = current_minutes / device_interval;
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
            interval = current_minutes / device_interval;
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
/**
 * @brief Set a local TimeSynchronization recipient address.
 * @param index [in] Recipient list index
 * @param address [in] Address to store
 * @return true if the address was stored
 */
bool handler_timesync_recipient_address_set(
    unsigned index, BACNET_ADDRESS *address)
{
    bool status = false;

    if (address && (index < MAX_TIME_SYNC_RECIPIENTS)) {
        Time_Sync_Recipients[index].recipient.tag =
            BACNET_RECIPIENT_TAG_ADDRESS;
        bacnet_address_copy(
            &Time_Sync_Recipients[index].recipient.type.address, address);
        status = true;
    }

    return status;
}

/**
 * @brief Set a UTC TimeSynchronization recipient address.
 * @param index [in] Recipient list index
 * @param address [in] Address to store
 * @return true if the address was stored
 */
bool handler_timesync_utc_recipient_address_set(
    unsigned index, BACNET_ADDRESS *address)
{
    bool status = false;

    if (address && (index < MAX_TIME_SYNC_RECIPIENTS)) {
        Time_Sync_UTC_Recipients[index].recipient.tag =
            BACNET_RECIPIENT_TAG_ADDRESS;
        bacnet_address_copy(
            &Time_Sync_UTC_Recipients[index].recipient.type.address, address);
        status = true;
    }

    return status;
}
#endif

#if defined(BACNET_TIME_MASTER)
/**
 * @brief Run the periodic TimeSynchronization master task.
 * @param current_date_time [in] Current local date and time
 */
void handler_timesync_task(BACNET_DATE_TIME *current_date_time)
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
/**
 * @brief Initialize local and UTC TimeSynchronization recipient lists.
 */
void handler_timesync_init(void)
{
    unsigned i = 0;

    /* connect linked list */
    bacnet_recipient_list_link_array(
        Time_Sync_Recipients, MAX_TIME_SYNC_RECIPIENTS);
    bacnet_recipient_list_link_array(
        Time_Sync_UTC_Recipients, MAX_TIME_SYNC_RECIPIENTS);
    for (i = 0; i < MAX_TIME_SYNC_RECIPIENTS; i++) {
        Time_Sync_Recipients[i].recipient.tag = BACNET_RECIPIENT_TAG_MAX;
        Time_Sync_UTC_Recipients[i].recipient.tag = BACNET_RECIPIENT_TAG_MAX;
    }
    datetime_set_values(&Next_Sync_Time, 0, 0, 0, 0, 0, 0, 0);
}
#endif

/**
 * @brief Configure the callback used to apply received TimeSynchronization.
 * @param cb [in] Callback to register, or NULL to clear it
 */
void handler_timesync_set_callback_set(handler_timesync_set_callback_t cb)
{
    handler_timesync_set_callback = cb;
}
