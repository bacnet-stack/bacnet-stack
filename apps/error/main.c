/**
 * @file
 * @brief command line tool that sends a BACnet service to the network:
 * BACnet Error message
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
#include "bacnet/npdu.h"
#include "bacnet/apdu.h"
#include "bacnet/version.h"
/* some demo stuff needed */
#include "bacnet/basic/binding/address.h"
#include "bacnet/basic/object/device.h"
#include "bacnet/basic/services.h"
#include "bacnet/basic/sys/filename.h"
#include "bacnet/basic/tsm/tsm.h"
#include "bacnet/datalink/datalink.h"
#include "bacnet/datalink/dlenv.h"

/* parsed command line parameters */
static uint16_t Target_Error_Class;
static uint16_t Target_Error_Code;
static uint8_t Target_Invoke_ID = 1;
static uint16_t Target_Service = SERVICE_CONFIRMED_READ_PROPERTY;
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

static void
MyRejectHandler(BACNET_ADDRESS *src, uint8_t invoke_id, uint8_t reject_reason)
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
    printf(
        "Usage: %s [error-class error-code service-number invoke-id]\n",
        filename);
    printf("       [--dnet][--dadr][--mac]\n");
    printf("       [--version][--help]\n");
}

static void print_help(const char *filename)
{
    printf("Send BACnet Error message to the network.\n");
    printf("--mac A\n"
           "Optional destination BACnet mac address."
           "Valid ranges are from 00 to FF (hex) for MS/TP or ARCNET,\n"
           "or an IP string with optional port number like 10.1.2.3:47808\n"
           "or an Ethernet MAC in hex like 00:21:70:7e:32:bb\n");
    printf("\n");
    printf(
        "--dnet N\n"
        "Optional destination BACnet network number N for directed requests.\n"
        "Valid range is from 0 to 65535 where 0 is the local connection\n"
        "and 65535 is network broadcast.\n");
    printf("\n");
    printf("--dadr A\n"
           "Optional BACnet mac address on the destination BACnet network.\n"
           "Valid ranges are from 00 to FF (hex) for MS/TP or ARCNET,\n"
           "or an IP string with optional port number like 10.1.2.3:47808\n"
           "or an Ethernet MAC in hex like 00:21:70:7e:32:bb\n");
    printf("\n");
    printf(
        "error-class:\n"
        "    number from 0 to 65535\n"
        "error-code:\n"
        "    number from 0 to 65535\n"
        "service-number:\n"
        "    number from 0 to 65535 for BACnet Services\n"
        "invoke-id:\n"
        "    number from 1 to 255\n"
        "Example:\n"
        "%s 3 2 1\n",
        filename);
}

int main(int argc, char *argv[])
{
    long dnet = -1;
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
        } else {
            if (target_args == 0) {
                if (!bacnet_string_to_uint16(argv[argi], &Target_Error_Class)) {
                    fprintf(stderr, "error-class=%s invalid\n", argv[argi]);
                    return 1;
                }
                target_args++;
            } else if (target_args == 1) {
                if (!bacnet_string_to_uint16(argv[argi], &Target_Error_Code)) {
                    fprintf(stderr, "error-code=%s invalid\n", argv[argi]);
                    return 1;
                }
                target_args++;
            } else if (target_args == 2) {
                if (!bacnet_string_to_uint16(argv[argi], &Target_Service)) {
                    fprintf(stderr, "service=%s invalid\n", argv[argi]);
                    return 1;
                }
                target_args++;
            } else if (target_args == 3) {
                if (!bacnet_string_to_uint8(argv[argi], &Target_Invoke_ID)) {
                    fprintf(stderr, "invoke-id=%s invalid\n", argv[argi]);
                    return 1;
                }
                target_args++;
            } else {
                print_usage(filename);
                return 1;
            }
        }
    }
    address_init();
    if (specific_address) {
        bacnet_address_init(&dest, &mac, dnet, &adr);
    }
    /* setup my info */
    Device_Set_Object_Instance_Number(BACNET_MAX_INSTANCE);
    Init_Service_Handlers();
    address_init();
    dlenv_init();
    atexit(datalink_cleanup);
    /* send the request */
    Send_Error_To_Network(
        &Handler_Transmit_Buffer[0], &dest, Target_Invoke_ID, Target_Service,
        Target_Error_Class, Target_Error_Code);

    return 0;
}
