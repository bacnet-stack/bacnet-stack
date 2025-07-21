/**
 * @file
 * @brief command line tool that sends a BACnet service to the network:
 * BACnet EventNotification message
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date 2021
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
#include "bacport.h"
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

/** Handler for a Simple ACK PDU.
 *
 * @param src [in] BACNET_ADDRESS of the source of the message
 * @param invoke_id [in] the invokeID from the rejected message
 */
static void
MyWritePropertySimpleAckHandler(BACNET_ADDRESS *src, uint8_t invoke_id)
{
    if (address_match(&Target_Address, src) &&
        (invoke_id == Request_Invoke_ID)) {
        printf("\nEventNotification Acknowledged!\n");
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
        SERVICE_CONFIRMED_EVENT_NOTIFICATION, MyWritePropertySimpleAckHandler);
    /* handle any errors coming back */
    apdu_set_error_handler(
        SERVICE_CONFIRMED_EVENT_NOTIFICATION, MyErrorHandler);
    apdu_set_abort_handler(MyAbortHandler);
    apdu_set_reject_handler(MyRejectHandler);
}

static void print_usage(const char *filename)
{
    printf(
        "Usage: %s device-id process-id initiating-device-id\n"
        "    event-object-type event-object-instance\n"
        "    sequence-number notification-class priority message-text\n"
        "    notify-type ack-required from-state to-state event-type\n"
        "    [change-of-bitstring reference-bit-string status-flags]\n"
        "    [change-of-state new-state-tag new-state-value status-flags]\n",
        filename);
    printf("       [--dnet][--dadr][--mac]\n");
    printf("       [--version][--help]\n");
}

static void print_help(const char *filename)
{
    printf("Send BACnet ConfirmedEventNotification message to a device.\n");
    printf(
        "device-id:\n"
        "BACnet Device Object Instance number that you are trying to\n"
        "communicate to.  This number will be used to try and bind with\n"
        "the device using Who-Is and I-Am services.  For example, if you were\n"
        "notifying Device Object 123, the device-instance would be 123.\n");
    printf("\n");
    printf("process-id:\n"
           "Process Identifier in the receiving device for which the\n"
           "notification is intended.\n");
    printf("\n");
    printf("initiating-device-id: the BACnet Device Object Instance number\n"
           "that initiated the ConfirmedEventNotification service request.\n");
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
    printf("event-type\n"
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
           "Optional BACnet mac address on the destination BACnet network.\n"
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
    BACNET_MAC_ADDRESS mac = { 0 };
    BACNET_MAC_ADDRESS adr = { 0 };
    BACNET_ADDRESS dest = { 0 };
    bool specific_address = false;
    int argi = 0;
    unsigned int target_args = 0;
    const char *filename = NULL;
    unsigned found_index = 0;

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
                /* device-id */
                Target_Device_Object_Instance = strtol(argv[argi], NULL, 0);
                target_args++;
            } else if (target_args == 1) {
                /* process-id */
                event_data.processIdentifier = strtol(argv[argi], NULL, 0);
                target_args++;
            } else if (target_args == 2) {
                /* initiating-device-id */
                event_data.initiatingObjectIdentifier.type = OBJECT_DEVICE;
                event_data.initiatingObjectIdentifier.instance =
                    strtol(argv[argi], NULL, 0);
                target_args++;
            } else if (target_args == 3) {
                /* event-object-type */
                event_data.eventObjectIdentifier.type =
                    strtol(argv[argi], NULL, 0);
                target_args++;
            } else if (target_args == 4) {
                /* event-object-instance */
                event_data.eventObjectIdentifier.instance =
                    strtol(argv[argi], NULL, 0);
                target_args++;
            } else if (target_args == 5) {
                /* sequence-number */
                event_data.timeStamp.tag = TIME_STAMP_SEQUENCE;
                event_data.timeStamp.value.sequenceNum =
                    strtol(argv[argi], NULL, 0);
                target_args++;
            } else if (target_args == 6) {
                /* notification-class */
                event_data.notificationClass = strtol(argv[argi], NULL, 0);
                target_args++;
            } else if (target_args == 7) {
                /* priority */
                event_data.priority = strtol(argv[argi], NULL, 0);
                target_args++;
            } else if (target_args == 8) {
                /* message-text */
                characterstring_init_ansi(&bcstring, argv[argi]);
                event_data.messageText = &bcstring;
                target_args++;
            } else if (target_args == 9) {
                /* notify-type */
                if (bactext_notify_type_index(argv[argi], &found_index)) {
                    event_data.notifyType = found_index;
                } else {
                    event_data.notifyType = strtol(argv[argi], NULL, 0);
                }
                target_args++;
            } else if (target_args == 10) {
                /* ack-required */
                event_data.ackRequired = strtol(argv[argi], NULL, 0);
                target_args++;
            } else if (target_args == 11) {
                /* from-state */
                if (bactext_event_state_index(argv[argi], &found_index)) {
                    event_data.fromState = found_index;
                } else {
                    event_data.fromState = strtol(argv[argi], NULL, 0);
                }
                target_args++;
            } else if (target_args == 12) {
                /* to-state */
                if (bactext_event_state_index(argv[argi], &found_index)) {
                    event_data.toState = found_index;
                } else {
                    event_data.toState = strtol(argv[argi], NULL, 0);
                }
                target_args++;
            } else if (target_args == 13) {
                /* event-type - see BACNET_EVENT_TYPE */
                if (bactext_event_type_index(argv[argi], &found_index)) {
                    event_data.eventType = found_index;
                } else {
                    event_data.eventType = strtol(argv[argi], NULL, 0);
                }
                target_args++;
            } else {
                if (event_data.eventType == EVENT_CHANGE_OF_BITSTRING) {
                    if (target_args == 14) {
                        pBitString =
                            &event_data.notificationParams.changeOfBitstring
                                 .referencedBitString;
                        bitstring_init_ascii(pBitString, argv[argi]);
                        target_args++;
                    } else if (target_args == 15) {
                        pBitString = &event_data.notificationParams
                                          .changeOfBitstring.statusFlags;
                        bitstring_init_ascii(pBitString, argv[argi]);
                        target_args++;
                    } else {
                        print_usage(filename);
                        return 1;
                    }
                } else if (event_data.eventType == EVENT_CHANGE_OF_STATE) {
                    if (target_args == 14) {
                        tag = strtol(argv[argi], NULL, 0);
                        event_data.notificationParams.changeOfState.newState
                            .tag = tag;
                        target_args++;
                    } else if (target_args == 15) {
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
                    } else if (target_args == 16) {
                        pBitString = &event_data.notificationParams
                                          .changeOfBitstring.statusFlags;
                        bitstring_init_ascii(pBitString, argv[argi]);
                        target_args++;
                    } else {
                        print_usage(filename);
                        return 1;
                    }
                } else if (event_data.eventType == EVENT_CHANGE_OF_VALUE) {
                    /* FIXME: add event type parameters */
                } else if (event_data.eventType == EVENT_COMMAND_FAILURE) {
                    /* FIXME: add event type parameters */
                } else if (event_data.eventType == EVENT_FLOATING_LIMIT) {
                    /* FIXME: add event type parameters */
                } else if (event_data.eventType == EVENT_OUT_OF_RANGE) {
                    /* FIXME: add event type parameters */
                } else if (
                    event_data.eventType == EVENT_CHANGE_OF_LIFE_SAFETY) {
                    /* FIXME: add event type parameters */
                } else if (event_data.eventType == EVENT_EXTENDED) {
                    /* FIXME: add event type parameters */
                } else if (event_data.eventType == EVENT_BUFFER_READY) {
                    /* FIXME: add event type parameters */
                } else if (event_data.eventType == EVENT_UNSIGNED_RANGE) {
                    /* FIXME: add event type parameters */
                } else if (event_data.eventType == EVENT_ACCESS_EVENT) {
                    /* FIXME: add event type parameters */
                } else if (event_data.eventType == EVENT_DOUBLE_OUT_OF_RANGE) {
                    /* FIXME: add event type parameters */
                } else if (event_data.eventType == EVENT_SIGNED_OUT_OF_RANGE) {
                    /* FIXME: add event type parameters */
                } else if (
                    event_data.eventType == EVENT_UNSIGNED_OUT_OF_RANGE) {
                    /* FIXME: add event type parameters */
                } else if (
                    event_data.eventType == EVENT_CHANGE_OF_CHARACTERSTRING) {
                    /* FIXME: add event type parameters */
                } else if (
                    event_data.eventType == EVENT_CHANGE_OF_STATUS_FLAGS) {
                    /* FIXME: add event type parameters */
                } else if (
                    event_data.eventType == EVENT_CHANGE_OF_RELIABILITY) {
                    /* FIXME: add event type parameters */
                } else if (event_data.eventType == EVENT_NONE) {
                    /* FIXME: add event type parameters */
                } else if (
                    event_data.eventType == EVENT_CHANGE_OF_DISCRETE_VALUE) {
                    /* FIXME: add event type parameters */
                } else if (event_data.eventType == EVENT_CHANGE_OF_TIMER) {
                    /* FIXME: add event type parameters */
                } else if (
                    (event_data.eventType >= EVENT_PROPRIETARY_MIN) &&
                    (event_data.eventType <= EVENT_PROPRIETARY_MAX)) {
                    /* Enumerated values 64-65535 may
                       be used by others subject to
                       the procedures and constraints
                       described in Clause 23.  */
                } else {
                    print_usage(filename);
                    return 1;
                }
            }
        }
    }
    if (target_args < 14) {
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
        printf(
            "Added Device %u to address cache\n",
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
                Request_Invoke_ID = Send_CEvent_Notify_Address(
                    Handler_Transmit_Buffer, sizeof(Handler_Transmit_Buffer),
                    &event_data, &Target_Address);
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
