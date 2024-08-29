/**
 * @file
 * @brief command line tool that sends a BACnet service to the network:
 * I-Am message
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date 2016
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
#include "bacnet/npdu.h"
#include "bacnet/apdu.h"
#include "bacnet/version.h"
/* some demo stuff needed */
#include "bacnet/basic/binding/address.h"
#include "bacnet/basic/object/device.h"
#include "bacnet/basic/sys/filename.h"
#include "bacnet/basic/services.h"
#include "bacnet/basic/tsm/tsm.h"
#include "bacnet/datalink/datalink.h"
#include "bacnet/datalink/dlenv.h"

/* buffer used for receive */
static uint8_t Rx_Buf[MAX_MPDU] = { 0 };

/* parsed command line parameters */
static uint32_t Target_Device_ID = BACNET_MAX_INSTANCE;
static uint16_t Target_Vendor_ID = BACNET_VENDOR_ID;
static unsigned int Target_Max_APDU = MAX_APDU;
static int Target_Segmentation = SEGMENTATION_NONE;
/* flag for signalling errors */
static bool Error_Detected = false;

static void MyAbortHandler(
    BACNET_ADDRESS *src, uint8_t invoke_id, uint8_t abort_reason, bool server)
{
    (void)src;
    (void)invoke_id;
    (void)server;
    printf("BACnet Abort: %s\n", bactext_abort_reason_name(abort_reason));
    Error_Detected = true;
}

static void MyRejectHandler(
    BACNET_ADDRESS *src, uint8_t invoke_id, uint8_t reject_reason)
{
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
    /* handle the reply (request) coming back */
    apdu_set_unconfirmed_handler(SERVICE_UNCONFIRMED_I_AM, handler_i_am_add);
    /* handle any errors coming back */
    apdu_set_abort_handler(MyAbortHandler);
    apdu_set_reject_handler(MyRejectHandler);
}

static void print_usage(const char *filename)
{
    printf("Usage: %s [device-instance vendor-id max-apdu segmentation]\n",
        filename);
    printf("       [--dnet][--dadr][--mac]\n");
    printf("       [--version][--help]\n");
}

static void print_help(const char *filename)
{
    printf("Send BACnet I-Am message for a device.\n");
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
        "Optional BACnet mac address on the destination BACnet network number.\n"
        "Valid ranges are from 00 to FF (hex) for MS/TP or ARCNET,\n"
        "or an IP string with optional port number like 10.1.2.3:47808\n"
        "or an Ethernet MAC in hex like 00:21:70:7e:32:bb\n");
    printf("\n");
    printf("--repeat\n"
        "Send the message repeatedly until signalled to quit.\n"
        "Default is to not repeat, sending only a single message.\n");
    printf("\n");
    printf("--retry C\n"
        "Send the message C number of times\n"
        "Default is retry 0, only sending one time.\n");
    printf("\n");
    printf("--delay\n"
        "Delay, in milliseconds, between repeated messages.\n"
        "Default delay is 100ms.\n");
    printf("\n");
    printf("device-instance:\n"
        "BACnet device-ID 0..4194303\n");
    printf("\n");
    printf("vendor-id:\n"
        "Vendor Identifier 0..65535\n");
    printf("\n");
    printf("max-apdu:\n"
        "Maximum APDU size 50..65535\n");
    printf("\n");
    printf("segmentation:\n"
        "BACnet Segmentation 0=both, 1=transmit, 2=receive, 3=none\n");
    printf("\n");
    printf("Example:\n"
        "To send an I-Am message of instance=1234 vendor-id=260 max-apdu=480\n"
        "%s 1234 260 480\n",
        filename);
}

int main(int argc, char *argv[])
{
    BACNET_ADDRESS src = { 0 }; /* address where message came from */
    uint16_t pdu_len = 0;
    long dnet = -1;
    BACNET_MAC_ADDRESS mac = { 0 };
    BACNET_MAC_ADDRESS adr = { 0 };
    BACNET_ADDRESS dest = { 0 };
    bool specific_address = false;
    bool repeat_forever = false;
    unsigned timeout = 100; /* milliseconds */
    int argi = 0;
    unsigned int target_args = 0;
    const char *filename = NULL;
    long retry_count = 0;

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
        } else if (strcmp(argv[argi], "--repeat") == 0) {
            repeat_forever = true;
        } else if (strcmp(argv[argi], "--retry") == 0) {
            if (++argi < argc) {
                retry_count = strtol(argv[argi], NULL, 0);
                if (retry_count < 0) {
                    retry_count = 0;
                }
            }
        } else if (strcmp(argv[argi], "--delay") == 0) {
            if (++argi < argc) {
                timeout = strtol(argv[argi], NULL, 0);
            }
        } else {
            if (target_args == 0) {
                Target_Device_ID = strtol(argv[argi], NULL, 0);
                target_args++;
            } else if (target_args == 1) {
                Target_Vendor_ID = strtol(argv[argi], NULL, 0);
                target_args++;
            } else if (target_args == 2) {
                Target_Max_APDU = strtol(argv[argi], NULL, 0);
                target_args++;
            } else if (target_args == 3) {
                Target_Segmentation = strtol(argv[argi], NULL, 0);
                target_args++;
            } else {
                print_usage(filename);
                return 1;
            }
        }
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
    }
    /* setup my info */
    Device_Set_Object_Instance_Number(BACNET_MAX_INSTANCE);
    Init_Service_Handlers();
    address_init();
    dlenv_init();
    atexit(datalink_cleanup);
    /* send the request */
    do {
        Send_I_Am_To_Network(&dest, Target_Device_ID, Target_Max_APDU,
            Target_Segmentation, Target_Vendor_ID);
        if (repeat_forever || retry_count) {
            /* returns 0 bytes on timeout */
            pdu_len = datalink_receive(&src, &Rx_Buf[0], MAX_MPDU, timeout);
            /* process */
            if (pdu_len) {
                npdu_handler(&src, &Rx_Buf[0], pdu_len);
            }
            if (Error_Detected) {
                break;
            }
            if (retry_count > 0) {
                retry_count--;
            }
        }
    } while (repeat_forever || retry_count);

    return 0;
}
