/**
 * @file
 * @brief command line tool that sends a BACnet service to the network:
 *  Ack-Alarm message
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date 2021
 * @copyright SPDX-License-Identifier: MIT
 */
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h> /* for time */
#include <errno.h>
/* BACnet Stack defines - first */
#include "bacnet/bacdef.h"
/* BACnet Stack API */
#include "bacnet/bactext.h"
#include "bacnet/iam.h"
#include "bacnet/cov.h"
#include "bacnet/npdu.h"
#include "bacnet/apdu.h"
#include "bacnet/basic/tsm/tsm.h"
#include "bacnet/basic/binding/address.h"
#include "bacnet/basic/object/device.h"
#include "bacport.h"
#include "bacnet/datalink/datalink.h"
#include "bacnet/alarm_ack.h"
#include "bacnet/whois.h"
#include "bacnet/version.h"
/* some demo stuff needed */
#include "bacnet/basic/sys/filename.h"
#include "bacnet/basic/services.h"
#include "bacnet/basic/tsm/tsm.h"
#include "bacnet/datalink/dlenv.h"

/* buffer used for receive */
static uint8_t Rx_Buf[MAX_MPDU] = { 0 };
/* global variables used in this file */
static uint32_t Target_Device_Object_Instance = BACNET_MAX_INSTANCE;
/* needed to filter incoming messages */
static uint8_t Request_Invoke_ID = 0;
static BACNET_ADDRESS Target_Address;
/* needed for return value of main application */
static bool Error_Detected = false;

/** Handler for an Error PDU.
 *
 * @param src [in] BACNET_ADDRESS of the source of the message
 * @param invoke_id [in] the invokeID from the rejected message
 * @param error_class [in] the error class
 * @param error_code [in] the error code
 */
static void MyErrorHandler(BACNET_ADDRESS *src,
    uint8_t invoke_id,
    BACNET_ERROR_CLASS error_class,
    BACNET_ERROR_CODE error_code)
{
    if (address_match(&Target_Address, src) &&
        (invoke_id == Request_Invoke_ID)) {
        printf("BACnet Error: %s: %s\n",
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
 * @param server
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
static void MyRejectHandler(
    BACNET_ADDRESS *src, uint8_t invoke_id, uint8_t reject_reason)
{
    if (address_match(&Target_Address, src) &&
        (invoke_id == Request_Invoke_ID)) {
        printf("BACnet Reject: %s\n",
            bactext_reject_reason_name((int)reject_reason));
        Error_Detected = true;
    }
}

/** Handler for a Simple ACK PDU.
 *
 * @param src [in] BACNET_ADDRESS of the source of the message
 * @param invoke_id [in] the invokeID from the rejected message
 */
static void MyWritePropertySimpleAckHandler(
    BACNET_ADDRESS *src, uint8_t invoke_id)
{
    if (address_match(&Target_Address, src) &&
        (invoke_id == Request_Invoke_ID)) {
        printf("\nAcknowledgeAlarm Acknowledged!\n");
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
    /* handle the ack coming back */
    apdu_set_confirmed_simple_ack_handler(
        SERVICE_CONFIRMED_ACKNOWLEDGE_ALARM, MyWritePropertySimpleAckHandler);
    /* handle any errors coming back */
    apdu_set_error_handler(SERVICE_CONFIRMED_ACKNOWLEDGE_ALARM, MyErrorHandler);
    apdu_set_abort_handler(MyAbortHandler);
    apdu_set_reject_handler(MyRejectHandler);
}

static void print_usage(char *filename)
{
    printf("Usage: %s device-id process-id\n"
           "    event-object-type event-object-instance event-state-acked\n"
           "    event-time-stamp ack-source-name ack-time-stamp\n",
        filename);
    printf("       [--dnet][--dadr][--mac]\n");
    printf("       [--version][--help]\n");
}

static void print_help(char *filename)
{
    printf("Send BACnet AcknowledgedAlarm, message to a device.\n");
    printf("device-id:\n"
        "BACnet Device Object Instance number that you are trying to\n"
        "communicate to.  This number will be used to try and bind with\n"
        "the device using Who-Is and I-Am services.  For example, if you were\n"
        "notifying Device Object 123, the device-instance would be 123.\n");
    printf("\n");
    printf("process-id:\n"
        "Process Identifier in the receiving device for which the\n"
        "notification is intended.\n");
    printf("\n");
    printf("event-object-type:\n"
        "The object type is defined either as the object-type name string\n"
        "as defined in the BACnet specification, or as the integer value.\n");
    printf("\n");
    printf("event-object-instance:\n"
        "The object instance number of the event object.\n");
    printf("\n");
    printf("event-state-acked:\n"
        "The event-state that for this alarm acknowledge.\n");
    printf("\n");
    printf("event-time-stamp:\n"
        "The time-stamp of the event.\n");
    printf("\n");
    printf("ack-source-name\n"
        "The source name of the alarm acknowledge.\n");
    printf("\n");
    printf("ack-time-stamp\n"
        "The time-stamp of the alarm acknowledge.\n");
    printf("\n");
    printf("--mac A\n"
        "Optional BACnet mac address."
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
    (void)filename;
}

int main(int argc, char *argv[])
{
    BACNET_ALARM_ACK_DATA data = { 0 };
    BACNET_ADDRESS src = { 0 }; /* address where message came from */
    unsigned timeout = 100; /* milliseconds */
    uint16_t pdu_len = 0;
    unsigned max_apdu = 0;
    time_t elapsed_seconds = 0;
    time_t last_seconds = 0;
    time_t current_seconds = 0;
    time_t timeout_seconds = 0;
    bool found = false;
    long dnet = -1;
    unsigned object_type = 0;
    BACNET_MAC_ADDRESS mac = { 0 };
    BACNET_MAC_ADDRESS adr = { 0 };
    BACNET_ADDRESS dest = { 0 };
    bool specific_address = false;
    int argi = 0;
    unsigned int target_args = 0;
    char *filename = NULL;

    filename = filename_remove_path(argv[0]);
    for (argi = 1; argi < argc; argi++) {
        if (strcmp(argv[argi], "--help") == 0) {
            print_usage(filename);
            print_help(filename);
            return 0;
        }
        if (strcmp(argv[argi], "--version") == 0) {
            printf("%s %s\n", filename, BACNET_VERSION_TEXT);
            printf("Copyright (C) 2021 by Steve Karg and others.\n"
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
                dnet = strtol(argv[argi], NULL, 0);
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
        } else {
            if (target_args == 0) {
                /* device-id */
                Target_Device_Object_Instance = strtol(argv[argi], NULL, 0);
                target_args++;
            } else if (target_args == 1) {
                /* process-id */
                data.ackProcessIdentifier = strtol(argv[argi], NULL, 0);
                target_args++;
            } else if (target_args == 2) {
                /* event-object-type */
                if (bactext_object_type_strtol(argv[argi], &object_type)) {
                    data.eventObjectIdentifier.type = object_type;
                    target_args++;
                } else {
                    fprintf(
                        stderr, "event-object-type=%s invalid\n", argv[argi]);
                    return 1;
                }
            } else if (target_args == 3) {
                /* event-object-instance */
                data.eventObjectIdentifier.instance =
                    strtol(argv[argi], NULL, 0);
                target_args++;
            } else if (target_args == 4) {
                /* event-state-acked */
                if (bactext_event_state_strtol(argv[argi], &object_type)) {
                    data.eventStateAcked = object_type;
                    target_args++;
                } else {
                    fprintf(stderr, "event-state=%s invalid\n", argv[argi]);
                    return 1;
                }
            } else if (target_args == 5) {
                /* event-time-stamp */
                bacapp_timestamp_init_ascii(&data.eventTimeStamp, argv[argi]);
                target_args++;
            } else if (target_args == 6) {
                /* ack-source */
                characterstring_init_ansi(&data.ackSource, argv[argi]);
                target_args++;
            } else if (target_args == 7) {
                /* ack-time-stamp */
                bacapp_timestamp_init_ascii(&data.ackTimeStamp, argv[argi]);
                target_args++;
            } else {
                print_usage(filename);
                return 1;
            }
        }
    }
    if (target_args < 7) {
        print_usage(filename);
        return 0;
    }
    address_init();
    if (specific_address) {
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
        address_add(Target_Device_Object_Instance, MAX_APDU, &dest);
        printf("Added Device %u to address cache\n",
            Target_Device_Object_Instance);
    }
    /* setup my info */
    Device_Set_Object_Instance_Number(BACNET_MAX_INSTANCE);
    Init_Service_Handlers();
    dlenv_init();
    atexit(datalink_cleanup);
    /* configure the timeout values */
    last_seconds = time(NULL);
    timeout_seconds = (apdu_timeout() / 1000) * apdu_retries();
    /* try to bind with the device */
    found = address_bind_request(
        Target_Device_Object_Instance, &max_apdu, &Target_Address);
    if (!found) {
        Send_WhoIs(
            Target_Device_Object_Instance, Target_Device_Object_Instance);
    }
    /* loop forever */
    for (;;) {
        /* increment timer - exit if timed out */
        current_seconds = time(NULL);

        /* at least one second has passed */
        if (current_seconds != last_seconds) {
            tsm_timer_milliseconds(
                (uint16_t)((current_seconds - last_seconds) * 1000));
            datalink_maintenance_timer(current_seconds - last_seconds);
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
                Request_Invoke_ID = Send_Alarm_Acknowledgement_Address(
                    Handler_Transmit_Buffer, sizeof(Handler_Transmit_Buffer),
                    &data, &Target_Address);
            } else if (tsm_invoke_id_free(Request_Invoke_ID)) {
                break;
            } else if (tsm_invoke_id_failed(Request_Invoke_ID)) {
                fprintf(stderr, "\rError: TSM Timeout!\n");
                tsm_free_invoke_id(Request_Invoke_ID);
                Error_Detected = true;
                /* try again or abort? */
                break;
            }
        } else {
            /* increment timer - exit if timed out */
            elapsed_seconds += (current_seconds - last_seconds);
            if (elapsed_seconds > timeout_seconds) {
                printf("\rError: APDU Timeout!\n");
                Error_Detected = true;
                break;
            }
        }
        /* returns 0 bytes on timeout */
        pdu_len = datalink_receive(&src, &Rx_Buf[0], MAX_MPDU, timeout);
        /* process */
        if (pdu_len) {
            npdu_handler(&src, &Rx_Buf[0], pdu_len);
        }
        /* keep track of time for next check */
        last_seconds = current_seconds;
    }
    if (Error_Detected) {
        return 1;
    }

    return 0;
}
