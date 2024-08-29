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
#include <errno.h>
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
    printf("Usage: %s pid object-type object-instance \n"
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
    BACNET_BIT_STRING *pBitString;
    BACNET_CHARACTER_STRING bcstring;
    BACNET_PROPERTY_STATES tag = PROP_STATE_BOOLEAN_VALUE;
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
                event_data.processIdentifier = strtol(argv[argi], NULL, 0);
                target_args++;
            } else if (target_args == 1) {
                event_data.initiatingObjectIdentifier.type =
                    strtol(argv[argi], NULL, 0);
                target_args++;
            } else if (target_args == 2) {
                event_data.initiatingObjectIdentifier.instance =
                    strtol(argv[argi], NULL, 0);
                target_args++;
            } else if (target_args == 3) {
                event_data.eventObjectIdentifier.type =
                    strtol(argv[argi], NULL, 0);
                target_args++;
            } else if (target_args == 4) {
                event_data.eventObjectIdentifier.instance =
                    strtol(argv[argi], NULL, 0);
                target_args++;
            } else if (target_args == 5) {
                event_data.timeStamp.tag = TIME_STAMP_SEQUENCE;
                event_data.timeStamp.value.sequenceNum =
                    strtol(argv[argi], NULL, 0);
                target_args++;
            } else if (target_args == 6) {
                event_data.notificationClass = strtol(argv[argi], NULL, 0);
                target_args++;
            } else if (target_args == 7) {
                event_data.priority = strtol(argv[argi], NULL, 0);
                target_args++;
            } else if (target_args == 8) {
                event_data.eventType = strtol(argv[argi], NULL, 0);
                target_args++;
            } else {
                if (event_data.eventType == EVENT_CHANGE_OF_BITSTRING) {
                    if (target_args == 9) {
                        pBitString =
                            &event_data.notificationParams.changeOfBitstring
                                 .referencedBitString;
                        bitstring_init_ascii(pBitString, argv[argi]);
                        target_args++;
                    } else if (target_args == 10) {
                        pBitString = &event_data.notificationParams
                                          .changeOfBitstring.statusFlags;
                        bitstring_init_ascii(pBitString, argv[argi]);
                        target_args++;
                    } else if (target_args == 11) {
                        characterstring_init_ansi(&bcstring, argv[argi]);
                        event_data.messageText = &bcstring;
                        target_args++;
                    } else if (target_args == 12) {
                        event_data.notifyType = strtol(argv[argi], NULL, 0);
                        target_args++;
                    } else if (target_args == 13) {
                        event_data.ackRequired = strtol(argv[argi], NULL, 0);
                        target_args++;
                    } else if (target_args == 14) {
                        event_data.fromState = strtol(argv[argi], NULL, 0);
                        target_args++;
                    } else if (target_args == 15) {
                        event_data.toState = strtol(argv[argi], NULL, 0);
                        target_args++;
                    } else {
                        print_usage(filename);
                        return 1;
                    }
                } else if (event_data.eventType == EVENT_CHANGE_OF_STATE) {
                    if (target_args == 9) {
                        tag = strtol(argv[argi], NULL, 0);
                        event_data.notificationParams.changeOfState.newState
                            .tag = tag;
                        target_args++;
                    } else if (target_args == 10) {
                        if (tag == PROP_STATE_BOOLEAN_VALUE) {
                            event_data.notificationParams.changeOfState.newState
                                .state.booleanValue =
                                strtol(argv[argi], NULL, 0);
                        } else if (tag == PROP_STATE_BINARY_VALUE) {
                            event_data.notificationParams.changeOfState.newState
                                .state.binaryValue =
                                strtol(argv[argi], NULL, 0);
                        } else if (tag == PROP_STATE_EVENT_TYPE) {
                            event_data.notificationParams.changeOfState.newState
                                .state.eventType = strtol(argv[argi], NULL, 0);
                        } else if (tag == PROP_STATE_POLARITY) {
                            event_data.notificationParams.changeOfState.newState
                                .state.polarity = strtol(argv[argi], NULL, 0);
                        } else if (tag == PROP_STATE_PROGRAM_CHANGE) {
                            event_data.notificationParams.changeOfState.newState
                                .state.programChange =
                                strtol(argv[argi], NULL, 0);
                        } else if (tag == PROP_STATE_PROGRAM_STATE) {
                            event_data.notificationParams.changeOfState.newState
                                .state.programState =
                                strtol(argv[argi], NULL, 0);
                        } else if (tag == PROP_STATE_REASON_FOR_HALT) {
                            event_data.notificationParams.changeOfState.newState
                                .state.programError =
                                strtol(argv[argi], NULL, 0);
                        } else if (tag == PROP_STATE_RELIABILITY) {
                            event_data.notificationParams.changeOfState.newState
                                .state.reliability =
                                strtol(argv[argi], NULL, 0);
                        } else if (tag == PROP_STATE_EVENT_STATE) {
                            event_data.notificationParams.changeOfState.newState
                                .state.state = strtol(argv[argi], NULL, 0);
                        } else if (tag == PROP_STATE_SYSTEM_STATUS) {
                            event_data.notificationParams.changeOfState.newState
                                .state.systemStatus =
                                strtol(argv[argi], NULL, 0);
                        } else if (tag == PROP_STATE_UNITS) {
                            event_data.notificationParams.changeOfState.newState
                                .state.units = strtol(argv[argi], NULL, 0);
                        } else if (tag == PROP_STATE_UNSIGNED_VALUE) {
                            event_data.notificationParams.changeOfState.newState
                                .state.unsignedValue =
                                strtol(argv[argi], NULL, 0);
                        } else if (tag == PROP_STATE_LIFE_SAFETY_MODE) {
                            event_data.notificationParams.changeOfState.newState
                                .state.lifeSafetyMode =
                                strtol(argv[argi], NULL, 0);
                        } else if (tag == PROP_STATE_LIFE_SAFETY_STATE) {
                            event_data.notificationParams.changeOfState.newState
                                .state.lifeSafetyState =
                                strtol(argv[argi], NULL, 0);
                        } else {
                            printf("Invalid Change-Of-State Tag\n");
                            return 1;
                        }
                        target_args++;
                    } else if (target_args == 11) {
                        pBitString = &event_data.notificationParams
                                          .changeOfBitstring.statusFlags;
                        bitstring_init_ascii(pBitString, argv[argi]);
                        target_args++;
                    } else if (target_args == 12) {
                        characterstring_init_ansi(&bcstring, argv[argi]);
                        event_data.messageText = &bcstring;
                        target_args++;
                    } else if (target_args == 13) {
                        event_data.notifyType = strtol(argv[argi], NULL, 0);
                        target_args++;
                    } else if (target_args == 14) {
                        event_data.ackRequired = strtol(argv[argi], NULL, 0);
                        target_args++;
                    } else if (target_args == 15) {
                        event_data.fromState = strtol(argv[argi], NULL, 0);
                        target_args++;
                    } else if (target_args == 16) {
                        event_data.toState = strtol(argv[argi], NULL, 0);
                        target_args++;
                    } else {
                        print_usage(filename);
                        return 1;
                    }
                } else if (event_data.eventType == EVENT_CHANGE_OF_VALUE) {
                } else if (event_data.eventType == EVENT_COMMAND_FAILURE) {
                } else if (event_data.eventType == EVENT_FLOATING_LIMIT) {
                } else if (event_data.eventType == EVENT_OUT_OF_RANGE) {
                } else if (event_data.eventType ==
                    EVENT_CHANGE_OF_LIFE_SAFETY) {
                } else if (event_data.eventType == EVENT_EXTENDED) {
                } else if (event_data.eventType == EVENT_BUFFER_READY) {
                } else if (event_data.eventType == EVENT_UNSIGNED_RANGE) {
                } else {
                    print_usage(filename);
                    return 1;
                }
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
    dlenv_init();
    atexit(datalink_cleanup);
    Send_UEvent_Notify(&Handler_Transmit_Buffer[0], &event_data, &dest);

    return 0;
}
