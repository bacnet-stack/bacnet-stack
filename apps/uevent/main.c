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
#include "bacnet/basic/services.h"
#include "bacnet/basic/tsm/tsm.h"
#include "bacnet/datalink/datalink.h"
#include "bacnet/datalink/dlenv.h"
#include "bacport.h"

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
    printf("       [--dnet][--dadr][--mac]\n");
    printf("       [--version][--help]\n");
}

static void print_help(const char *filename)
{
    printf("Send BACnet UnconfirmedEventNotification message for a device.\n");
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
    (void)filename;
}

int main(int argc, char *argv[])
{
    BACNET_EVENT_NOTIFICATION_DATA event_data = { 0 };
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
                if (!event_notify_parse(
                        &event_data, argc - argi, &argv[argi])) {
                    fprintf(stderr, "event=%s invalid\n", argv[argi]);
                }
            } else {
                print_usage(filename);
                return 1;
            }
        }
    }
    if (specific_address) {
        bacnet_address_init(&dest, &mac, dnet, &adr);
    }
    /* setup my info */
    Device_Set_Object_Instance_Number(BACNET_MAX_INSTANCE);
    Init_Service_Handlers();
    dlenv_init();
    atexit(datalink_cleanup);
    Send_UEvent_Notify(&Handler_Transmit_Buffer[0], &event_data, &dest);

    return 0;
}
