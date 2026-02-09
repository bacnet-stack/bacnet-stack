/**
 * @file
 * @brief command line tool that sends a BACnet UnconfirmedEventNotification
 * to the network
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date 2016
 * @copyright SPDX-License-Identifier: MIT
 */
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h> /* for time */
/* BACnet Stack defines - first */
#include "bacnet/bacdef.h"
/* BACnet Stack API */
#include "bacnet/bactext.h"
#include "bacnet/iam.h"
#include "bacnet/cov.h"
#include "bacnet/npdu.h"
#include "bacnet/apdu.h"
#include "bacnet/event.h"
#include "bacnet/whois.h"
#include "bacnet/version.h"
/* some demo stuff needed */
#include "bacnet/basic/binding/address.h"
#include "bacnet/basic/object/device.h"
#include "bacnet/basic/sys/filename.h"
#include "bacnet/basic/sys/mstimer.h"
#include "bacnet/basic/services.h"
#include "bacnet/basic/tsm/tsm.h"
#include "bacnet/datalink/datalink.h"
#include "bacnet/datalink/dlenv.h"
#include "bacport.h"

static uint32_t Target_Device_Object_Instance = BACNET_MAX_INSTANCE;
static BACNET_ADDRESS Target_Address;
static bool Error_Detected = false;
static uint8_t Handler_Receive_Buffer[MAX_MPDU] = { 0 };

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
}

static void print_usage(const char *filename)
{
    printf(
        "Usage: %s pid object-type object-instance \n"
        "    event-object-type event-object-instance \n"
        "    sequence-number notification-class priority event-type\n"
        "    [reference-bit-string status-flags message notify-type\n"
        "     ack-required from-state to-state]\n"
        "    [new-state status-flags message notify-type\n"
        "     ack-required from-state to-state]\n",
        filename);
    printf("       [--dnet][--dadr][--mac][--device]\n");
    printf("       [--version][--help]\n");
}

static void print_help(const char *filename)
{
    printf("Send BACnet UnconfirmedEventNotification message for a device.\n");
    printf("process-id:\n"
           "Process Identifier in the receiving device for which the\n"
           "notification is intended.\n");
    printf("\n");
    printf("initiating-device-id: the BACnet Device Object Instance number\n"
           "that initiated the UnconfirmedEventNotification request.\n");
    printf("\n");
    printf(
        "event-object-type:\n"
        "The object type is defined either as the object-type name string\n"
        "as defined in the BACnet specification, or as the integer value.\n");
    printf("\n");
    printf("event-object-instance:\n"
           "The object instance number of the event object.\n");
    printf("\n");
    printf("sequence-number:\n"
           "The sequence number of the event.\n");
    printf("\n");
    printf("notification-class:\n"
           "The notification-class of the event.\n");
    printf("\n");
    printf("priority:\n"
           "The priority of the event.\n");
    printf("\n");
    printf("message-text:\n"
           "The message text of the event.\n");
    printf("\n");
    printf("notify-type:\n"
           "The notify type of the event.\n");
    printf("\n");
    printf("ack-required:\n"
           "The ack-required of the event (0=FALSE,1=TRUE).\n");
    printf("\n");
    printf("from-state:\n"
           "The from-state of the event.\n");
    printf("\n");
    printf("to-state:\n"
           "The to-state of the event.\n");
    printf("\n");
    printf("event-type:\n"
           "The event-type of the event.\n");
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
           "Optional BACnet mac address on the destination BACnet network\n"
           "Valid ranges are from 00 to FF (hex) for MS/TP or ARCNET,\n"
           "or an IP string with optional port number like 10.1.2.3:47808\n"
           "or an Ethernet MAC in hex like 00:21:70:7e:32:bb\n");
    printf("\n");
    printf("--device D:\n"
           "BACnet Device Object Instance number of the target device.\n"
           "This application will try and bind with this device using\n"
           "Who-Is and I-Am services.\n");
    printf("Example:\n");
    printf("%s 1 2 binary-value 4 5 6 7 message event\n", filename);
}

int main(int argc, char *argv[])
{
    BACNET_EVENT_NOTIFICATION_DATA event_data = { 0 };
    BACNET_CHARACTER_STRING event_data_message_text = { 0 };
    BACNET_ADDRESS src = { 0 }; /* address where message came from */
    uint16_t pdu_len = 0;
    unsigned timeout = 100; /* milliseconds */
    unsigned max_apdu = 0;
    bool found = false;
    int apdu_len = 0;
    struct mstimer apdu_timer = { 0 };
    long dnet = -1;
    BACNET_MAC_ADDRESS mac = { 0 };
    BACNET_MAC_ADDRESS adr = { 0 };
    BACNET_ADDRESS dest = { 0 };
    bool specific_address = false;
    int argi = 0;
    unsigned int target_args = 0;
    const char *filename = NULL;

    event_data.messageText = &event_data_message_text;
    filename = filename_remove_path(argv[0]);
    for (argi = 1; argi < argc; argi++) {
        if (strcmp(argv[argi], "--help") == 0) {
            print_usage(filename);
            print_help(filename);
            return 0;
        }
        if (strcmp(argv[argi], "--version") == 0) {
            printf("%s %s\n", filename, BACNET_VERSION_TEXT);
            printf("Copyright (C) 2016 by Steve Karg and others.\n"
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
        } else if (strcmp(argv[argi], "--device") == 0) {
            if (++argi < argc) {
                Target_Device_Object_Instance = strtol(argv[argi], NULL, 0);
                if (Target_Device_Object_Instance > BACNET_MAX_INSTANCE) {
                    fprintf(
                        stderr, "device=%u exceeds maximum %u\n",
                        Target_Device_Object_Instance, BACNET_MAX_INSTANCE);
                    return 1;
                }
            }
        } else {
            if (target_args == 0) {
                if (event_notify_parse(&event_data, argc - argi, &argv[argi])) {
                    target_args++;
                    break;
                } else {
                    fprintf(stderr, "event parsing invalid\n");
                    return 1;
                }
            }
        }
    }
    if (target_args < 1) {
        print_usage(filename);
        return 0;
    }
    address_init();
    if (specific_address) {
        bacnet_address_init(&dest, &mac, dnet, &adr);
        address_add(Target_Device_Object_Instance, MAX_APDU, &dest);
        printf(
            "Added Device %u to address cache\n",
            Target_Device_Object_Instance);
    } else if (Target_Device_Object_Instance == BACNET_MAX_INSTANCE) {
        printf("Using broadcast to notify device\n");
        bacnet_address_init(&dest, NULL, BACNET_BROADCAST_NETWORK, NULL);
        address_add(Target_Device_Object_Instance, MAX_APDU, &dest);
    }
    /* setup my info */
    Device_Set_Object_Instance_Number(BACNET_MAX_INSTANCE);
    Init_Service_Handlers();
    dlenv_init();
    atexit(datalink_cleanup);
    mstimer_init();
    mstimer_set(&apdu_timer, apdu_timeout());
    /* try to bind with the device */
    found = address_bind_request(
        Target_Device_Object_Instance, &max_apdu, &Target_Address);
    if (!found) {
        Send_WhoIs(
            Target_Device_Object_Instance, Target_Device_Object_Instance);
    }
    /* main loop: run until the event notification is sent or
       an error/timeout occurs */
    for (;;) {
        if (found) {
            if (Target_Device_Object_Instance == BACNET_MAX_INSTANCE) {
                /* use BACNET_ADDRESS API */
                apdu_len = Send_UEvent_Notify(
                    Handler_Transmit_Buffer, &event_data, &Target_Address);
            } else {
                /* use device API */
                apdu_len = Send_UEvent_Notify_Device(
                    Handler_Transmit_Buffer, &event_data,
                    Target_Device_Object_Instance);
            }
            if (apdu_len <= 0) {
                printf("Error: Failed to send UEvent Notification!\n");
                Error_Detected = true;
            } else {
                printf(
                    "Sent UEvent Notification (%u bytes) to device %u\n",
                    apdu_len, Target_Device_Object_Instance);
                break;
            }
        } else {
            found = address_bind_request(
                Target_Device_Object_Instance, &max_apdu, &Target_Address);
        }
        /* returns 0 bytes on timeout */
        pdu_len = datalink_receive(
            &src, &Handler_Receive_Buffer[0], MAX_MPDU, timeout);
        /* process */
        if (pdu_len) {
            npdu_handler(&src, &Handler_Receive_Buffer[0], pdu_len);
        }
        if (Error_Detected) {
            break;
        }
        if (mstimer_expired(&apdu_timer)) {
            printf("\rError: APDU Timeout!\n");
            Error_Detected = true;
        }
    }

    return 0;
}
