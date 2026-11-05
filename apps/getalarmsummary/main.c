/**
 * @file
 * @brief command line tool that sends a BACnet service to the network:
 *  GetAlarmSummary message, and displays the reply
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date 2026
 * @copyright SPDX-License-Identifier: MIT
 */
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
/* BACnet Stack defines - first */
#include "bacnet/bacdef.h"
/* BACnet Stack API */
#include "bacnet/bactext.h"
#include "bacnet/apdu.h"
#include "bacnet/npdu.h"
#include "bacnet/whois.h"
#include "bacnet/get_alarm_sum.h"
#include "bacnet/version.h"
/* some demo stuff needed */
#include "bacnet/basic/binding/address.h"
#include "bacnet/basic/object/device.h"
#include "bacnet/basic/services.h"
#include "bacnet/basic/sys/debug.h"
#include "bacnet/basic/sys/filename.h"
#include "bacnet/basic/sys/mstimer.h"
#include "bacnet/basic/tsm/tsm.h"
#include "bacnet/datalink/datalink.h"
#include "bacnet/datalink/dlenv.h"
#include "bacport.h"

/* buffer used for receive */
static uint8_t Rx_Buf[MAX_MPDU] = { 0 };
/* global variables used in this file */
static uint32_t Target_Device_Object_Instance = BACNET_MAX_INSTANCE;
/* needed to filter incoming messages */
static uint8_t Request_Invoke_ID = 0;
static BACNET_ADDRESS Target_Address;
/* needed for return value of main application */
static bool Error_Detected = false;
static unsigned Alarm_Count = 0;

/** Handler for an Error PDU.
 *
 * @param src [in] BACNET_ADDRESS of the source of the message
 * @param invoke_id [in] the invokeID from the rejected message
 * @param error_class [in] the error class
 * @param error_code [in] the error code
 */
static void MyErrorHandler(
    BACNET_ADDRESS *src,
    uint8_t invoke_id,
    BACNET_ERROR_CLASS error_class,
    BACNET_ERROR_CODE error_code)
{
    if (address_match(&Target_Address, src) &&
        (invoke_id == Request_Invoke_ID)) {
        printf(
            "BACnet Error: %s: %s\n",
            bactext_error_class_name((int)error_class),
            bactext_error_code_name((int)error_code));
        Error_Detected = true;
    }
}

/** Handler for an Abort PDU.
 *
 * @param src [in] BACNET_ADDRESS of the source of the message
 * @param invoke_id [in] the invokeID from the rejected message
 * @param abort_reason [in] the reason for the message abort
 * @param server [in] true if the abort was sent by a server
 */
static void MyAbortHandler(
    BACNET_ADDRESS *src, uint8_t invoke_id, uint8_t abort_reason, bool server)
{
    (void)server;
    if (address_match(&Target_Address, src) &&
        (invoke_id == Request_Invoke_ID)) {
        printf(
            "BACnet Abort: %s\n", bactext_abort_reason_name((int)abort_reason));
        Error_Detected = true;
    }
}

/** Handler for a Reject PDU.
 *
 * @param src [in] BACNET_ADDRESS of the source of the message
 * @param invoke_id [in] the invokeID from the rejected message
 * @param reject_reason [in] the reason for the rejection
 */
static void
MyRejectHandler(BACNET_ADDRESS *src, uint8_t invoke_id, uint8_t reject_reason)
{
    if (address_match(&Target_Address, src) &&
        (invoke_id == Request_Invoke_ID)) {
        printf(
            "BACnet Reject: %s\n",
            bactext_reject_reason_name((int)reject_reason));
        Error_Detected = true;
    }
}

/**
 * @brief Callback for each decoded GetAlarmSummary ACK entry
 * @param src [in] BACNET_ADDRESS of the source of the ACK
 * @param invoke_id [in] invoke ID of the ACK
 * @param data [in] the decoded entry
 */
static void My_Get_Alarm_Summary_Ack_Callback(
    BACNET_ADDRESS *src,
    uint8_t invoke_id,
    const BACNET_GET_ALARM_SUMMARY_DATA *data)
{
    if (address_match(&Target_Address, src) &&
        (invoke_id == Request_Invoke_ID)) {
        get_alarm_summary_ack_print_data(data);
        Alarm_Count++;
    }
}

/**
 * Initializes the BACnet objects and services supported
 */
static void Init_Service_Handlers(void)
{
    Device_Init(NULL);
    /* we need to handle who-is
       to support dynamic device binding to us */
    apdu_set_unconfirmed_handler(SERVICE_UNCONFIRMED_WHO_IS, handler_who_is);
    /* handle i-am to support binding to other devices */
    apdu_set_unconfirmed_handler(SERVICE_UNCONFIRMED_I_AM, handler_i_am_bind);
    /* set the handler for all the services we don't implement
       It is required to send the proper reject message... */
    apdu_set_unrecognized_service_handler_handler(handler_unrecognized_service);
    /* we must implement read property - it's required! */
    apdu_set_confirmed_handler(
        SERVICE_CONFIRMED_READ_PROPERTY, handler_read_property);
    /* handle the data coming back from confirmed requests */
    apdu_set_confirmed_ack_handler(
        SERVICE_CONFIRMED_GET_ALARM_SUMMARY, get_alarm_summary_ack_handler);
    get_alarm_summary_ack_callback_set(My_Get_Alarm_Summary_Ack_Callback);
    /* handle any errors coming back */
    apdu_set_error_handler(SERVICE_CONFIRMED_GET_ALARM_SUMMARY, MyErrorHandler);
    apdu_set_abort_handler(MyAbortHandler);
    apdu_set_reject_handler(MyRejectHandler);
}

static void print_usage(const char *filename)
{
    printf("Usage: %s device-id\n", filename);
    printf("       [--dnet][--dadr][--mac][--debug]\n");
    printf("       [--version][--help]\n");
}

static void print_help(const char *filename)
{
    printf("Send BACnet GetAlarmSummary message to a device\n"
           "and print the active alarms in the reply.\n");
    printf("\n");
    printf(
        "device-id:\n"
        "BACnet Device Object Instance number that you are trying to\n"
        "communicate to.  This number will be used to try and bind with\n"
        "the device using Who-Is and I-Am services.  For example, if you were\n"
        "requesting from Device Object 123, the device-id would be 123.\n");
    printf("\n");
    printf("--mac A\n"
           "Optional BACnet mac address.\n"
           "Valid ranges are from 00 to FF (hex) for MS/TP or ARCNET,\n"
           "or an IP string with optional port number like 10.1.2.3:47808\n"
           "or an Ethernet MAC in hex like 00:21:70:7e:32:bb\n");
    printf("\n");
    printf("--dnet N\n"
           "Optional BACnet network number N for directed requests.\n"
           "Valid range is from 0 to 65535 where 0 is the local connection\n"
           "and 65535 is network broadcast.\n");
    printf("\n");
    printf("--dadr A\n"
           "Optional BACnet mac address on the destination BACnet network.\n"
           "Valid ranges are from 00 to FF (hex) for MS/TP or ARCNET,\n"
           "or an IP string with optional port number like 10.1.2.3:47808\n"
           "or an Ethernet MAC in hex like 00:21:70:7e:32:bb\n");
    printf("\n");
    printf("--debug S\n"
           "Optional debug severity level 0=emergency, 1=alert, 2=critical,\n"
           "3=error, 4=warning, 5=notice, 6=info, 7=debug, -1=disable.\n");
    printf("\n");
    printf(
        "Example:\n"
        "To request the alarm summary from Device 123:\n"
        "%s 123\n",
        filename);
}

/**
 * @brief Send a GetAlarmSummary request to a device and print the reply
 * @param argc [in] number of command line arguments
 * @param argv [in] command line arguments
 * @return 0 on success, 1 on error
 */
int main(int argc, char *argv[])
{
    BACNET_ADDRESS src = { 0 }; /* address where message came from */
    unsigned timeout = 100; /* milliseconds */
    unsigned max_apdu = 0;
    struct mstimer apdu_timer = { 0 };
    struct mstimer maintenance_timer = { 0 };
    bool found = false;
    long dnet = -1;
    uint32_t severity = 0;
    unsigned long long_value = 0;
    BACNET_MAC_ADDRESS mac = { 0 };
    BACNET_MAC_ADDRESS adr = { 0 };
    BACNET_ADDRESS dest = { 0 };
    bool specific_address = false;
    int argi = 0;
    unsigned int target_args = 0;
    const char *filename = NULL;

    filename = filename_remove_path(argv[0]);
    for (argi = 1; argi < argc; argi++) {
        if (strcmp(argv[argi], "--help") == 0) {
            print_usage(filename);
            print_help(filename);
            return 0;
        }
        if (strcmp(argv[argi], "--version") == 0) {
            printf("%s %s\n", filename, BACNET_VERSION_TEXT);
            printf("Copyright (C) 2026 by Steve Karg and others.\n"
                   "This is free software; see the source for copying "
                   "conditions.\n"
                   "There is NO warranty; not even for MERCHANTABILITY or\n"
                   "FITNESS FOR A PARTICULAR PURPOSE.\n");
            return 0;
        }
        if (strcmp(argv[argi], "--mac") == 0) {
            if (++argi < argc) {
                if (bacnet_address_mac_from_ascii(&mac, argv[argi])) {
                    specific_address = true;
                }
            }
        } else if (strcmp(argv[argi], "--dnet") == 0) {
            if (++argi < argc) {
                if (!bacnet_strtol(argv[argi], &dnet)) {
                    fprintf(stderr, "dnet=%s invalid\n", argv[argi]);
                    return 1;
                }
                if ((dnet >= 0) && (dnet <= BACNET_BROADCAST_NETWORK)) {
                    specific_address = true;
                }
            }
        } else if (strcmp(argv[argi], "--dadr") == 0) {
            if (++argi < argc) {
                if (bacnet_address_mac_from_ascii(&adr, argv[argi])) {
                    specific_address = true;
                }
            }
        } else if (strcmp(argv[argi], "--debug") == 0) {
            if (++argi < argc) {
                if (bactext_debug_severity_strtol(argv[argi], &severity)) {
                    debug_log_severity_set(severity);
                } else {
                    debug_log_severity_set(DEBUG_LOG_DISABLED);
                }
            }
        } else if (target_args == 0) {
            /* device-id */
            if (!bacnet_strtoul(argv[argi], &long_value)) {
                fprintf(stderr, "device-instance=%s invalid\n", argv[argi]);
                return 1;
            }
            if (long_value > BACNET_MAX_INSTANCE) {
                fprintf(
                    stderr, "device-instance=%lu - exceeds %d\n", long_value,
                    BACNET_MAX_INSTANCE);
                return 1;
            }
            Target_Device_Object_Instance = (uint32_t)long_value;
            target_args++;
        } else {
            print_usage(filename);
            return 1;
        }
    }
    if (target_args < 1) {
        print_usage(filename);
        return 0;
    }
    /* setup my info */
    address_init();
    if (specific_address) {
        if (dnet < 0) {
            dnet = 0;
        }
        bacnet_address_init(&dest, &mac, (uint16_t)dnet, &adr);
        address_add(Target_Device_Object_Instance, MAX_APDU, &dest);
        printf(
            "Added Device %lu to address cache\n",
            (unsigned long)Target_Device_Object_Instance);
    }
    Device_Set_Object_Instance_Number(BACNET_MAX_INSTANCE);
    Init_Service_Handlers();
    dlenv_init();
    atexit(datalink_cleanup);
    mstimer_init();
    mstimer_set(&apdu_timer, apdu_timeout() * apdu_retries());
    mstimer_set(&maintenance_timer, 1000);
    /* try to bind with the device */
    found = address_bind_request(
        Target_Device_Object_Instance, &max_apdu, &Target_Address);
    if (!found) {
        Send_WhoIs(
            Target_Device_Object_Instance, Target_Device_Object_Instance);
    }
    /* loop forever */
    for (;;) {
        uint16_t pdu_len = 0;

        if (mstimer_expired(&maintenance_timer)) {
            mstimer_reset(&maintenance_timer);
            tsm_timer_milliseconds(
                (uint16_t)mstimer_interval(&maintenance_timer));
            datalink_maintenance_timer(
                (uint16_t)(mstimer_interval(&maintenance_timer) / 1000L));
        }
        if (Error_Detected) {
            break;
        }
        /* wait until the device is bound, or timeout and quit */
        if (!found) {
            found = address_bind_request(
                Target_Device_Object_Instance, &max_apdu, &Target_Address);
        }
        if (found) {
            if (Request_Invoke_ID == 0) {
                Request_Invoke_ID = Send_Get_Alarm_Summary_Address(
                    &Target_Address, (uint16_t)max_apdu);
            } else if (tsm_invoke_id_free(Request_Invoke_ID)) {
                printf("%u active alarm(s)\n", Alarm_Count);
                break;
            } else if (tsm_invoke_id_failed(Request_Invoke_ID)) {
                fprintf(stderr, "\rError: TSM Timeout!\n");
                tsm_free_invoke_id(Request_Invoke_ID);
                Error_Detected = true;
                break;
            }
        } else if (mstimer_expired(&apdu_timer)) {
            printf("\rError: APDU Timeout!\n");
            Error_Detected = true;
            break;
        }
        /* returns 0 bytes on timeout */
        pdu_len = datalink_receive(&src, &Rx_Buf[0], MAX_MPDU, timeout);
        /* process */
        if (pdu_len) {
            npdu_handler(&src, &Rx_Buf[0], pdu_len);
        }
    }
    if (Error_Detected) {
        return 1;
    }

    return 0;
}
