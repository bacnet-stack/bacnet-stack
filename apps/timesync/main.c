/**
 * @file
 * @brief command line tool that sends a BACnet TimeSynchronization service
 *  message with the local or arbitrary time and date to sync another device
 *  on the network.
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date 2006
 * @copyright SPDX-License-Identifier: MIT
 */
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <errno.h>
/* BACnet Stack defines - first */
#include "bacnet/bacdef.h"
/* BACnet Stack API */
#include "bacnet/bacaddr.h"
#include "bacnet/bactext.h"
#include "bacnet/npdu.h"
#include "bacnet/apdu.h"
#include "bacnet/datetime.h"
#include "bacnet/timesync.h"
#include "bacnet/version.h"
/* some demo stuff needed */
#include "bacnet/basic/binding/address.h"
#include "bacnet/basic/object/device.h"
#include "bacnet/basic/sys/mstimer.h"
#include "bacnet/basic/sys/filename.h"
#include "bacnet/basic/services.h"
#include "bacnet/basic/tsm/tsm.h"
#include "bacnet/datalink/datalink.h"
#include "bacnet/datalink/dlenv.h"
#include "bacport.h"

/* buffer used for receive */
static uint8_t Rx_Buf[MAX_MPDU] = { 0 };
/* error flag */
static bool Error_Detected = false;

static void MyAbortHandler(
    BACNET_ADDRESS *src, uint8_t invoke_id, uint8_t abort_reason, bool server)
{
    /* FIXME: verify src and invoke id */
    (void)src;
    (void)invoke_id;
    (void)server;
    printf("BACnet Abort: %s\n", bactext_abort_reason_name(abort_reason));
    Error_Detected = true;
}

static void
MyRejectHandler(BACNET_ADDRESS *src, uint8_t invoke_id, uint8_t reject_reason)
{
    /* FIXME: verify src and invoke id */
    (void)src;
    (void)invoke_id;
    printf("BACnet Reject: %s\n", bactext_reject_reason_name(reject_reason));
    Error_Detected = true;
}

static void Init_Service_Handlers(void)
{
    Device_Init(NULL);
    /* we need to handle who-is
       to support dynamic device binding to us */
    apdu_set_unconfirmed_handler(SERVICE_UNCONFIRMED_WHO_IS, handler_who_is);
    /* set the handler for all the services we don't implement
       It is required to send the proper reject message... */
    apdu_set_unrecognized_service_handler_handler(handler_unrecognized_service);
    /* we must implement read property - it's required! */
    apdu_set_confirmed_handler(
        SERVICE_CONFIRMED_READ_PROPERTY, handler_read_property);
    /* handle the reply (request) coming in */
    apdu_set_unconfirmed_handler(
        SERVICE_UNCONFIRMED_UTC_TIME_SYNCHRONIZATION, handler_timesync_utc);
    apdu_set_unconfirmed_handler(
        SERVICE_UNCONFIRMED_TIME_SYNCHRONIZATION, handler_timesync);
    /* handle any errors coming back */
    apdu_set_abort_handler(MyAbortHandler);
    apdu_set_reject_handler(MyRejectHandler);
}

static void print_usage(const char *filename)
{
    printf("Usage: %s [--dnet][--dadr][--mac]\n", filename);
    printf("       [--date][--time]\n");
    printf("       [--version][--help]\n");
}

static void print_help(const char *filename)
{
    printf("Send BACnet TimeSynchronization request.\n");
    printf("\n");
    printf("--date year/month/day[:weekday]\n"
           "Date formatted 2021/12/31 or 2021/12/31:1\n"
           "where day is 1..31,\n"
           "where month is 1=January..12=December,\n"
           "where weekday is 1=Monday..7=Sunday\n");
    printf("\n");
    printf("--time hours:minutes:seconds.hundredths\n"
           "Time formatted 23:59:59.99 or 23:59:59 or 23:59\n");
    printf("\n");
    printf("--utc\n"
           "Send BACnet UTCTimeSynchronization request.\n");
    printf("\n");
    printf("--mac A\n"
           "BACnet mac address."
           "Valid ranges are from 0 to 255\n"
           "or an IP string with optional port number like 10.1.2.3:47808\n"
           "or an Ethernet MAC in hex like 00:21:70:7e:32:bb\n");
    printf("\n");
    printf("--dnet N\n"
           "BACnet network number N for directed requests.\n"
           "Valid range is from 0 to 65535 where 0 is the local connection\n"
           "and 65535 is network broadcast.\n");
    printf("\n");
    printf("--dadr A\n"
           "BACnet mac address on the destination BACnet network number.\n"
           "Valid ranges are from 0 to 255\n"
           "or an IP string with optional port number like 10.1.2.3:47808\n"
           "or an Ethernet MAC in hex like 00:21:70:7e:32:bb\n");
    printf("\n");
    printf(
        "Examples:\n"
        "Send a TimeSynchronization request to DNET 123:\n"
        "%s --dnet 123\n",
        filename);
    printf(
        "Send a TimeSynchronization request to MAC 10.0.0.1 DNET 123 DADR 5:\n"
        "%s --mac 10.0.0.1 --dnet 123 --dadr 5\n",
        filename);
    printf(
        "Send a TimeSynchronization request to MAC 10.1.2.3:47808:\n"
        "%s --mac 10.1.2.3:47808\n",
        filename);
}

int main(int argc, char *argv[])
{
    BACNET_ADDRESS src = { 0 }; /* address where message came from */
    uint16_t pdu_len = 0;
    const unsigned timeout = 100; /* milliseconds */
    BACNET_DATE bdate;
    BACNET_TIME btime;
    BACNET_DATE_TIME utc_time;
    BACNET_DATE_TIME local_time;
    bool override_date = false;
    bool override_time = false;
    bool use_utc = false;
    int16_t utc_offset_minutes = 0;
    int8_t dst_adjust_minutes = 0;
    bool dst_active = 0;
    struct mstimer apdu_timer;
    long dnet = -1;
    BACNET_MAC_ADDRESS mac = { 0 };
    BACNET_MAC_ADDRESS adr = { 0 };
    BACNET_ADDRESS dest = { 0 };
    bool global_broadcast = true;
    int argi = 0;
    const char *filename = NULL;

    /* decode any command line parameters */
    filename = filename_remove_path(argv[0]);
    for (argi = 1; argi < argc; argi++) {
        if (strcmp(argv[argi], "--help") == 0) {
            print_usage(filename);
            print_help(filename);
            return 0;
        }
        if (strcmp(argv[argi], "--version") == 0) {
            printf("%s %s\n", filename, BACNET_VERSION_TEXT);
            printf("Copyright (C) 2014 by Steve Karg and others.\n"
                   "This is free software; see the source for copying "
                   "conditions.\n"
                   "There is NO warranty; not even for MERCHANTABILITY or\n"
                   "FITNESS FOR A PARTICULAR PURPOSE.\n");
            return 0;
        }
        if (strcmp(argv[argi], "--mac") == 0) {
            if (++argi < argc) {
                if (bacnet_address_mac_from_ascii(&mac, argv[argi])) {
                    global_broadcast = false;
                }
            }
        }
        if (strcmp(argv[argi], "--dnet") == 0) {
            if (++argi < argc) {
                dnet = strtol(argv[argi], NULL, 0);
                if ((dnet >= 0) && (dnet <= BACNET_BROADCAST_NETWORK)) {
                    global_broadcast = false;
                }
            }
        }
        if (strcmp(argv[argi], "--dadr") == 0) {
            if (++argi < argc) {
                if (bacnet_address_mac_from_ascii(&adr, argv[argi])) {
                    global_broadcast = false;
                }
            }
        }
        if (strcmp(argv[argi], "--date") == 0) {
            if (++argi < argc) {
                if (datetime_date_init_ascii(&bdate, argv[argi])) {
                    override_date = true;
                }
            }
        }
        if (strcmp(argv[argi], "--time") == 0) {
            if (++argi < argc) {
                if (datetime_time_init_ascii(&btime, argv[argi])) {
                    override_time = true;
                }
            }
        }
        if (strcmp(argv[argi], "--utc") == 0) {
            use_utc = true;
        }
    }
    if (global_broadcast) {
        datalink_get_broadcast_address(&dest);
    } else {
        if (adr.len && mac.len) {
            memcpy(&dest.mac[0], &mac.adr[0], mac.len);
            dest.mac_len = mac.len;
            memcpy(&dest.adr[0], &adr.adr[0], adr.len);
            dest.len = adr.len;
            if ((dnet >= 0) && (dnet <= BACNET_BROADCAST_NETWORK)) {
                dest.net = dnet;
            } else {
                dest.net = BACNET_BROADCAST_NETWORK;
            }
        } else if (mac.len) {
            memcpy(&dest.mac[0], &mac.adr[0], mac.len);
            dest.mac_len = mac.len;
            dest.len = 0;
            if ((dnet >= 0) && (dnet <= BACNET_BROADCAST_NETWORK)) {
                dest.net = dnet;
            } else {
                dest.net = 0;
            }
        } else {
            if ((dnet >= 0) && (dnet <= BACNET_BROADCAST_NETWORK)) {
                dest.net = dnet;
            } else {
                dest.net = BACNET_BROADCAST_NETWORK;
            }
            dest.mac_len = 0;
            dest.len = 0;
        }
    }
    /* setup my info */
    Device_Set_Object_Instance_Number(BACNET_MAX_INSTANCE);
    Init_Service_Handlers();
    dlenv_init();
    atexit(datalink_cleanup);
    mstimer_init();
    /* send the request */
    datetime_local(
        override_date ? NULL : &bdate, override_time ? NULL : &btime,
        &utc_offset_minutes, &dst_active);
    if (use_utc) {
        /* convert to UTC */
        if (dst_active) {
            dst_adjust_minutes = -60;
        }
        datetime_set(&local_time, &bdate, &btime);
        datetime_local_to_utc(
            &utc_time, &local_time, utc_offset_minutes, dst_adjust_minutes);
        Send_TimeSyncUTC_Remote(&dest, &utc_time.date, &utc_time.time);
    } else {
        Send_TimeSync_Remote(&dest, &bdate, &btime);
    }
    mstimer_set(&apdu_timer, apdu_timeout());
    /* loop forever - not necessary for time sync, but we can watch */
    for (;;) {
        /* returns 0 bytes on timeout */
        pdu_len = datalink_receive(&src, &Rx_Buf[0], MAX_MPDU, timeout);
        /* process */
        if (pdu_len) {
            npdu_handler(&src, &Rx_Buf[0], pdu_len);
        }
        if (Error_Detected) {
            break;
        }
        if (mstimer_expired(&apdu_timer)) {
            break;
        }
    }

    return 0;
}
