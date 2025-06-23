/**
 * @file
 * @brief command line tool that sends a BACnet Who-Am-I request to devices,
 * and prints any You-Are responses received.
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date June 2025
 * @copyright SPDX-License-Identifier: MIT
 */
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#if (__STDC_VERSION__ >= 199901L) && defined(__STDC_ISO_10646__)
#include <locale.h>
#endif
/* BACnet Stack defines - first */
#include "bacnet/bacdef.h"
/* BACnet Stack API */
#include "bacnet/bactext.h"
#include "bacnet/whoami.h"
#include "bacnet/youare.h"
#include "bacnet/npdu.h"
#include "bacnet/apdu.h"
#include "bacnet/bactext.h"
#include "bacnet/version.h"
/* some demo stuff needed */
#include "bacnet/basic/binding/address.h"
#include "bacnet/basic/sys/mstimer.h"
#include "bacnet/basic/sys/filename.h"
#include "bacnet/basic/services.h"
#include "bacnet/basic/tsm/tsm.h"
#include "bacnet/basic/object/device.h"
#include "bacnet/datalink/datalink.h"
#include "bacnet/datalink/dlenv.h"
#include "bacport.h"

/* buffer used for receive */
static uint8_t Rx_Buf[MAX_MPDU] = { 0 };

/* global variables used in this file */
static long Source_Vendor_ID = BACNET_VENDOR_ID;
static BACNET_CHARACTER_STRING Source_Model_Name = { 0 };
static BACNET_CHARACTER_STRING Source_Serial_Number = { 0 };
static bool Error_Detected = false;
/* debug info printing */
static bool BACnet_Debug_Enabled;

/**
 * @brief Print the BACnet Abort reason
 * @param src source address of the device reporting the abort
 * @param invoke_id invoke ID of the message triggering the abort
 * @param abort_reason abort reason given by the device reporting the abort
 * @param server
 */
static void MyAbortHandler(
    BACNET_ADDRESS *src, uint8_t invoke_id, uint8_t abort_reason, bool server)
{
    (void)src;
    (void)server;
    fprintf(
        stderr, "BACnet Abort[%u]: %s\n", (unsigned)invoke_id,
        bactext_abort_reason_name(abort_reason));
    Error_Detected = true;
}

static void
MyRejectHandler(BACNET_ADDRESS *src, uint8_t invoke_id, uint8_t reject_reason)
{
    (void)src;
    fprintf(
        stderr, "BACnet Reject[%u]: %s\n", invoke_id,
        bactext_reject_reason_name(reject_reason));
    Error_Detected = true;
}

static void init_service_handlers(void)
{
    Device_Init(NULL);
    /* Note: this applications doesn't need to handle who-is
       it is confusing for the user! */
    /* set the handler for all the services we don't implement
       It is required to send the proper reject message... */
    apdu_set_unrecognized_service_handler_handler(handler_unrecognized_service);
    /* we must implement read property - it's required! */
    apdu_set_confirmed_handler(
        SERVICE_CONFIRMED_READ_PROPERTY, handler_read_property);
    /* handle the unconfirmed request that may be sent to us */
    apdu_set_unconfirmed_handler(
        SERVICE_UNCONFIRMED_YOU_ARE, handler_you_are_json_print);
    /* handle any errors coming back */
    apdu_set_abort_handler(MyAbortHandler);
    apdu_set_reject_handler(MyRejectHandler);
}

static void print_usage(const char *filename)
{
    printf("Usage: %s", filename);
    printf(" [vendor-id model-name serial-number]\n");
    printf("       [--dnet][--dadr][--mac]\n");
    printf("       [--version][--help]\n");
}

static void print_help(const char *filename)
{
    printf("Send a BACnet Who-Am-I service request to the network\n"
           "and wait for any You-Are service responses.\n");
    printf("\n");
    printf("vendor-id:\n"
           "the identity of the vendor of the device initiating"
           "the Who-Am-I service request.\n");
    printf("\n");
    printf("model-name:\n"
           "the model name of the device initiating the Who-Am-I"
           "service request.\n");
    printf("\n");
    printf("serial-number:\n"
           "the serial identifier of the device initiating"
           "the Who-Am-I service request.\n");
    printf("\n");
    printf("--mac A\n"
           "BACnet mac address."
           "Valid ranges are from 00 to FF (hex) for MS/TP or ARCNET,\n"
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
           "Valid ranges are from 00 to FF (hex) for MS/TP or ARCNET,\n"
           "or an IP string with optional port number like 10.1.2.3:47808\n"
           "or an Ethernet MAC in hex like 00:21:70:7e:32:bb\n");
    printf("\n");
    printf("--repeat\n"
           "Send the message repeatedly until signalled to quit.\n"
           "Default is disabled, using the APDU timeout as time to quit.\n");
    printf("\n");
    printf("--retry C\n"
           "Send the message C number of times\n"
           "Default is retry 0, only sending one time.\n");
    printf("\n");
    printf("--timeout T\n"
           "Wait T milliseconds after sending before retry\n"
           "Default delay is 3000ms.\n");
    printf("\n");
    printf("--delay M\n"
           "Wait M milliseconds for responses after sending\n"
           "Default delay is 100ms.\n");
    printf("\n");
    printf("Example:\n");
    printf(
        "Send a Who-Am-I-Request to DNET 123:\n"
        "%s --dnet 123\n",
        filename);
    printf(
        "Send a Who-Am-I-Request to MAC 10.0.0.1 DNET 123 DADR 05h:\n"
        "%s --mac 10.0.0.1 --dnet 123 --dadr 05\n",
        filename);
    printf(
        "Send a Who-Am-I-Request to MAC 10.1.2.3:47808:\n"
        "%s --mac 10.1.2.3:47808\n",
        filename);
    printf(
        "Send a Who-Am-I-Request from "
        "vendor-id 123 model-name 456 serial-number 789\n"
        "%s 123 456 789\n",
        filename);
    printf(
        "Send a Who-Am-I-Request from the default "
        "vendor-id, model-name, and serial-number.\n"
        "%s\n",
        filename);
}

int main(int argc, char *argv[])
{
    BACNET_ADDRESS src = { 0 }; /* address where message came from */
    uint16_t pdu_len = 0;
    unsigned timeout_milliseconds = 0;
    unsigned delay_milliseconds = 100;
    struct mstimer apdu_timer = { 0 };
    struct mstimer datalink_timer = { 0 };
    long dnet = -1;
    BACNET_MAC_ADDRESS mac = { 0 };
    BACNET_MAC_ADDRESS adr = { 0 };
    BACNET_ADDRESS dest = { 0 };
    bool global_broadcast = true;
    int argi = 0;
    unsigned int target_args = 0;
    const char *filename = NULL;
    bool repeat_forever = false;
    long retry_count = 0;

    /* check for local environment settings */
    if (getenv("BACNET_DEBUG")) {
        BACnet_Debug_Enabled = true;
    }
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
            printf("Copyright (C) 2025 by Steve Karg and others.\n"
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
        } else if (strcmp(argv[argi], "--dnet") == 0) {
            if (++argi < argc) {
                dnet = strtol(argv[argi], NULL, 0);
                if ((dnet >= 0) && (dnet <= BACNET_BROADCAST_NETWORK)) {
                    global_broadcast = false;
                }
            }
        } else if (strcmp(argv[argi], "--dadr") == 0) {
            if (++argi < argc) {
                if (bacnet_address_mac_from_ascii(&adr, argv[argi])) {
                    global_broadcast = false;
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
        } else if (strcmp(argv[argi], "--timeout") == 0) {
            if (++argi < argc) {
                timeout_milliseconds = strtol(argv[argi], NULL, 0);
            }
        } else if (strcmp(argv[argi], "--delay") == 0) {
            if (++argi < argc) {
                delay_milliseconds = strtol(argv[argi], NULL, 0);
            }
        } else {
            if (target_args == 0) {
                Source_Vendor_ID = strtol(argv[argi], NULL, 0);
                target_args++;
            } else if (target_args == 1) {
                characterstring_init_ansi(&Source_Model_Name, argv[argi]);
                target_args++;
            } else if (target_args == 2) {
                characterstring_init_ansi(&Source_Serial_Number, argv[argi]);
                target_args++;
            } else {
                print_usage(filename);
                return 1;
            }
        }
    }
    if (global_broadcast) {
        datalink_get_broadcast_address(&dest);
    } else {
        if ((dnet < 0) || (dnet > BACNET_BROADCAST_NETWORK)) {
            if (adr.len && mac.len) {
                dnet = BACNET_BROADCAST_NETWORK;
            } else if (mac.len) {
                dnet = 0;
            } else {
                dest.net = BACNET_BROADCAST_NETWORK;
            }
        }
        bacnet_address_init(&dest, &mac, dnet, &adr);
    }
    if (Source_Vendor_ID > UINT16_MAX) {
        fprintf(
            stderr, "vendor-id=%ld and must not be greater than %u\n",
            Source_Vendor_ID, UINT16_MAX);
        return 1;
    }
    /* setup my info */
    Device_Set_Object_Instance_Number(BACNET_MAX_INSTANCE);
    if (characterstring_length(&Source_Model_Name) == 0) {
        characterstring_init_ansi(&Source_Model_Name, Device_Model_Name());
    }
    if (characterstring_length(&Source_Serial_Number) == 0) {
        characterstring_init_ansi(
            &Source_Serial_Number, Device_Serial_Number());
    }
    init_service_handlers();
    dlenv_init();
#if (__STDC_VERSION__ >= 199901L) && defined(__STDC_ISO_10646__)
    /* Internationalized programs must call setlocale()
     * to initiate a specific language operation.
     * This can be done by calling setlocale() as follows.
     * If your native locale doesn't use UTF-8 encoding
     * you need to replace the empty string with a
     * locale like "en_US.utf8" which is the same as the string
     * used in the enviromental variable "LANG=en_US.UTF-8".
     */
    setlocale(LC_ALL, "");
#endif
    atexit(datalink_cleanup);
    if (timeout_milliseconds == 0) {
        timeout_milliseconds = apdu_timeout() * apdu_retries();
    }
    mstimer_set(&apdu_timer, timeout_milliseconds);
    mstimer_set(&datalink_timer, 1000);
    /* send the request */
    Send_Who_Am_I_To_Network(
        &dest, Source_Vendor_ID, &Source_Model_Name, &Source_Serial_Number);
    if (retry_count > 0) {
        retry_count--;
    }
    /* loop forever */
    for (;;) {
        /* returns 0 bytes on timeout */
        pdu_len =
            datalink_receive(&src, &Rx_Buf[0], MAX_MPDU, delay_milliseconds);
        /* process */
        if (pdu_len) {
            npdu_handler(&src, &Rx_Buf[0], pdu_len);
        }
        if (Error_Detected) {
            break;
        }
        if (mstimer_expired(&datalink_timer)) {
            datalink_maintenance_timer(
                mstimer_interval(&datalink_timer) / 1000);
            mstimer_reset(&datalink_timer);
        }
        if (mstimer_expired(&apdu_timer)) {
            if (repeat_forever || retry_count) {
                Send_Who_Am_I_To_Network(
                    &dest, Source_Vendor_ID, &Source_Model_Name,
                    &Source_Serial_Number);
                retry_count--;
            } else {
                break;
            }
            mstimer_reset(&apdu_timer);
        }
    }

    return 0;
}
