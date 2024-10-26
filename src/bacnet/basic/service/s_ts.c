/**************************************************************************
 *
 * Copyright (C) 2005 Steve Karg <skarg@users.sourceforge.net>
 *
 * SPDX-License-Identifier: MIT
 *
 *********************************************************************/
#include <stddef.h>
#include <stdint.h>
#include <errno.h>
#include <string.h>
/* BACnet Stack defines - first */
#include "bacnet/bacdef.h"
/* BACnet Stack API */
#include "bacnet/bacdcode.h"
#include "bacnet/npdu.h"
#include "bacnet/apdu.h"
#include "bacnet/datetime.h"
#include "bacnet/dcc.h"
#include "bacnet/timesync.h"
/* basic services, TSM, binding, and datalink */
#include "bacnet/basic/binding/address.h"
#include "bacnet/basic/services.h"
#include "bacnet/basic/tsm/tsm.h"
#include "bacnet/datalink/datalink.h"

/** @file s_ts.c  Send TimeSync requests. */

/**
 * Sends a TimeSync message to a specific destination
 *
 * @param dest - #BACNET_ADDRESS - the specific destination
 * @param bdate - #BACNET_DATE
 * @param btime - #BACNET_TIME
 */
void Send_TimeSync_Remote(
    BACNET_ADDRESS *dest, const BACNET_DATE *bdate, const BACNET_TIME *btime)
{
    int len = 0;
    int pdu_len = 0;
#if PRINT_ENABLED
    int bytes_sent = 0;
#endif
    BACNET_NPDU_DATA npdu_data;
    BACNET_ADDRESS my_address;

    if (!dcc_communication_enabled()) {
        return;
    }

    datalink_get_my_address(&my_address);
    /* encode the NPDU portion of the packet */
    npdu_encode_npdu_data(&npdu_data, false, MESSAGE_PRIORITY_NORMAL);
    pdu_len = npdu_encode_pdu(
        &Handler_Transmit_Buffer[0], dest, &my_address, &npdu_data);
    /* encode the APDU portion of the packet */
    len = timesync_encode_apdu(&Handler_Transmit_Buffer[pdu_len], bdate, btime);
    pdu_len += len;
    /* send it out the datalink */
#if PRINT_ENABLED
    bytes_sent =
#endif
        datalink_send_pdu(
            dest, &npdu_data, &Handler_Transmit_Buffer[0], pdu_len);
#if PRINT_ENABLED
    if (bytes_sent <= 0) {
        fprintf(
            stderr, "Failed to Send Time-Synchronization Request (%s)!\n",
            strerror(errno));
    }
#endif
}

/**
 * Sends a TimeSync message as a broadcast
 *
 * @param bdate - #BACNET_DATE
 * @param btime - #BACNET_TIME
 */
void Send_TimeSync(const BACNET_DATE *bdate, const BACNET_TIME *btime)
{
    BACNET_ADDRESS dest;

    datalink_get_broadcast_address(&dest);
    Send_TimeSync_Remote(&dest, bdate, btime);
}

/**
 * Sends a UTC TimeSync message to a specific destination
 *
 * @param dest - #BACNET_ADDRESS - the specific destination
 * @param bdate - #BACNET_DATE
 * @param btime - #BACNET_TIME
 */
void Send_TimeSyncUTC_Remote(
    BACNET_ADDRESS *dest, const BACNET_DATE *bdate, const BACNET_TIME *btime)
{
    int len = 0;
    int pdu_len = 0;
#if PRINT_ENABLED
    int bytes_sent = 0;
#endif
    BACNET_NPDU_DATA npdu_data;
    BACNET_ADDRESS my_address;

    if (!dcc_communication_enabled()) {
        return;
    }

    datalink_get_my_address(&my_address);
    /* encode the NPDU portion of the packet */
    npdu_encode_npdu_data(&npdu_data, false, MESSAGE_PRIORITY_NORMAL);
    pdu_len = npdu_encode_pdu(
        &Handler_Transmit_Buffer[0], dest, &my_address, &npdu_data);
    /* encode the APDU portion of the packet */
    len = timesync_utc_encode_apdu(
        &Handler_Transmit_Buffer[pdu_len], bdate, btime);
    pdu_len += len;
#if PRINT_ENABLED
    bytes_sent =
#endif
        datalink_send_pdu(
            dest, &npdu_data, &Handler_Transmit_Buffer[0], pdu_len);
#if PRINT_ENABLED
    if (bytes_sent <= 0) {
        fprintf(
            stderr, "Failed to Send UTC-Time-Synchronization Request (%s)!\n",
            strerror(errno));
    }
#endif
}

/**
 * Sends a UTC TimeSync message as a broadcast
 *
 * @param bdate - #BACNET_DATE
 * @param btime - #BACNET_TIME
 */
void Send_TimeSyncUTC(const BACNET_DATE *bdate, const BACNET_TIME *btime)
{
    BACNET_ADDRESS dest;

    datalink_get_broadcast_address(&dest);
    Send_TimeSyncUTC_Remote(&dest, bdate, btime);
}

/**
 * Sends a UTC TimeSync message using the local time from the device.
 */
void Send_TimeSyncUTC_Device(void)
{
    int16_t utc_offset_minutes = 0;
    bool dst_active = false;
    BACNET_DATE_TIME local_time;
    BACNET_DATE_TIME utc_time;
    bool status = false;

    status = datetime_local(
        &local_time.date, &local_time.time, &utc_offset_minutes, &dst_active);
    if (status) {
        datetime_copy(&utc_time, &local_time);
        datetime_add_minutes(&utc_time, utc_offset_minutes);
        if (dst_active) {
            datetime_add_minutes(&utc_time, -60);
        }
        Send_TimeSyncUTC(&utc_time.date, &utc_time.time);
    }
}

/**
 * Sends a TimeSync message using the local time from the device.
 */
void Send_TimeSync_Device(void)
{
    BACNET_DATE_TIME local_time = { 0 };
    bool status = false;

    status = datetime_local(&local_time.date, &local_time.time, NULL, NULL);
    if (status) {
        Send_TimeSync(&local_time.date, &local_time.time);
    }
}
