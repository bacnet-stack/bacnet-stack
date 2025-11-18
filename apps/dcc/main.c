/**
 * @file
 * @brief command line tool that sends a BACnet service to the network:
 *  DeviceCommunicationControl message
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date 2006
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
#include "bacnet/arf.h"
#include "bacnet/npdu.h"
#include "bacnet/apdu.h"
#include "bacport.h"
#include "bacnet/whois.h"
#include "bacnet/dcc.h"
#include "bacnet/version.h"
/* some demo stuff needed */
#include "bacnet/basic/object/device.h"
#include "bacnet/basic/binding/address.h"
#include "bacnet/basic/sys/filename.h"
#include "bacnet/basic/services.h"
#include "bacnet/basic/tsm/tsm.h"
#include "bacnet/datalink/dlenv.h"
#include "bacnet/datalink/datalink.h"

/* buffer used for receive */
static uint8_t Rx_Buf[MAX_MPDU] = { 0 };

/* global variables used in this file */
static uint32_t Target_Device_Object_Instance = BACNET_MAX_INSTANCE;
static BACNET_ADDRESS Target_Address;
static uint16_t Communication_Timeout_Minutes = 0;
static BACNET_COMMUNICATION_ENABLE_DISABLE Communication_State =
    COMMUNICATION_ENABLE;
static char *Communication_Password = NULL;

static bool Error_Detected = false;

static void MyErrorHandler(
    BACNET_ADDRESS *src,
    uint8_t invoke_id,
    BACNET_ERROR_CLASS error_class,
    BACNET_ERROR_CODE error_code)
{
    /* FIXME: verify src and invoke id */
    (void)src;
    (void)invoke_id;
    printf(
        "BACnet Error: %s: %s\n", bactext_error_class_name(error_class),
        bactext_error_code_name(error_code));
    Error_Detected = true;
}

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

static void MyDeviceCommunicationControlSimpleAckHandler(
    BACNET_ADDRESS *src, uint8_t invoke_id)
{
    (void)src;
    (void)invoke_id;
    printf("DeviceCommunicationControl Acknowledged!\n");
}

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
    /* handle communication so we can shutup when asked */
    apdu_set_confirmed_handler(
        SERVICE_CONFIRMED_DEVICE_COMMUNICATION_CONTROL,
        handler_device_communication_control);
    /* handle the ack coming back */
    apdu_set_confirmed_simple_ack_handler(
        SERVICE_CONFIRMED_DEVICE_COMMUNICATION_CONTROL,
        MyDeviceCommunicationControlSimpleAckHandler);
    /* handle any errors coming back */
    apdu_set_error_handler(
        SERVICE_CONFIRMED_DEVICE_COMMUNICATION_CONTROL, MyErrorHandler);
    apdu_set_abort_handler(MyAbortHandler);
    apdu_set_reject_handler(MyRejectHandler);
}

static void print_usage(const char *filename)
{
    printf("Usage: %s device-instance state [timeout [password]]\n", filename);
    printf("       [--dnet][--dadr][--mac]\n");
    printf("       [--version][--help]\n");
}

static void print_help(const char *filename)
{
    printf("Send BACnet DeviceCommunicationControl service to device.\n");
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
           "Optional BACnet mac address on the destination BACnet network "
           "number.\n"
           "Valid ranges are from 00 to FF (hex) for MS/TP or ARCNET,\n"
           "or an IP string with optional port number like 10.1.2.3:47808\n"
           "or an Ethernet MAC in hex like 00:21:70:7e:32:bb\n");
    printf("\n");
    printf("device-instance:\n"
           "BACnet Device Object Instance number that you are\n"
           "trying to communicate to.  This number will be used\n"
           "to try and bind with the device using Who-Is and\n"
           "I-Am services.  For example, if you were writing\n"
           "Device Object 123, the device-instance would be 123.\n");
    printf("\n");
    printf("state:\n"
           "Possible state values:\n"
           "  0=enable\n"
           "  1=disable\n"
           "  2=disable-initiation\n");
    printf("\n");
    printf("timeout:\n"
           "The timeout can be 0 for infinite, or a value in minutes for "
           "disable.\n");
    printf("\n");
    printf(
        "password:\n"
        "The optional password is a character string of 1 to 20 characters.\n");
    printf("\n");
    printf(
        "Example:\n"
        "If you want disable Device Communications in Device 123\n"
        "for 60 minutes with password 'filister', use the following command:\n"
        "%s 123 1 60 filister\n",
        filename);
}

int main(int argc, char *argv[])
{
    BACNET_ADDRESS src = { 0 }; /* address where message came from */
    uint16_t pdu_len = 0;
    unsigned timeout = 100; /* milliseconds */
    unsigned max_apdu = 0;
    time_t elapsed_seconds = 0;
    time_t last_seconds = 0;
    time_t current_seconds = 0;
    time_t timeout_seconds = 0;
    uint8_t invoke_id = 0;
    bool found = false;
    long dnet = -1;
    BACNET_MAC_ADDRESS mac = { 0 };
    BACNET_MAC_ADDRESS adr = { 0 };
    BACNET_ADDRESS dest = { 0 };
    bool specific_address = false;
    unsigned int target_args = 0;
    int argi = 0;
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
                    specific_address = true;
                }
            }
        } else if (strcmp(argv[argi], "--dnet") == 0) {
            if (++argi < argc) {
                if (!bacnet_strtol(argv[argi], &dnet)) {
                    fprintf(stderr, "dnet=%s invalid\n", argv[argi]);
                    return 1;
                }
                if ((dnet >= 0) && (dnet <= UINT16_MAX)) {
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
            /* unnamed arguments */
            if (target_args == 0) {
                if (!bacnet_strtouint32(
                        argv[argi], &Target_Device_Object_Instance)) {
                    fprintf(stderr, "device-instance=%s invalid\n", argv[argi]);
                    return 1;
                }
                if (Target_Device_Object_Instance > BACNET_MAX_INSTANCE) {
                    fprintf(
                        stderr, "device-instance=%u - not greater than %u\n",
                        Target_Device_Object_Instance, BACNET_MAX_INSTANCE);
                    return 1;
                }
                target_args++;
            } else if (target_args == 1) {
                if (!bacnet_strtouint16(argv[argi], &Communication_State)) {
                    fprintf(stderr, "state=%s invalid\n", argv[argi]);
                    return 1;
                }
                target_args++;
            } else if (target_args == 2) {
                if (!bacnet_strtouint16(
                        argv[argi], &Communication_Timeout_Minutes)) {
                    fprintf(stderr, "timeout=%s invalid\n", argv[argi]);
                    return 1;
                }
                target_args++;
            } else if (target_args == 3) {
                Communication_Password = argv[argi];
                target_args++;
            }
        }
    }
    if (argc < 3) {
        print_usage(filename);
        return 0;
    }
    /* setup my info */
    address_init();
    if (specific_address) {
        bacnet_address_init(&dest, &mac, dnet, &adr);
        address_add(Target_Device_Object_Instance, MAX_APDU, &dest);
    }
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

        /* returns 0 bytes on timeout */
        pdu_len = datalink_receive(&src, &Rx_Buf[0], MAX_MPDU, timeout);

        /* process */
        if (pdu_len) {
            npdu_handler(&src, &Rx_Buf[0], pdu_len);
        }
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
            if (invoke_id == 0) {
                invoke_id = Send_Device_Communication_Control_Request(
                    Target_Device_Object_Instance,
                    Communication_Timeout_Minutes, Communication_State,
                    Communication_Password);
            } else if (tsm_invoke_id_free(invoke_id)) {
                break;
            } else if (tsm_invoke_id_failed(invoke_id)) {
                fprintf(stderr, "\rError: TSM Timeout!\n");
                tsm_free_invoke_id(invoke_id);
                /* try again or abort? */
                break;
            }
        } else {
            /* increment timer - exit if timed out */
            elapsed_seconds += (current_seconds - last_seconds);
            if (elapsed_seconds > timeout_seconds) {
                printf("\rError: APDU Timeout!\n");
                break;
            }
        }
        /* keep track of time for next check */
        last_seconds = current_seconds;
    }

    return 0;
}
