/**
 * @file
 * @brief command line tool that uses BACnet ReinitializeDevice service
 * message to reinitialize another device on the network and prints an
 * acknowledgment or error response of this confirmed service request.
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date 2006
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
#include "bacnet/arf.h"
#include "bacnet/npdu.h"
#include "bacnet/apdu.h"
#include "bacnet/whois.h"
#include "bacnet/rd.h"
/* some demo stuff needed */
#include "bacnet/basic/binding/address.h"
#include "bacnet/basic/object/device.h"
#include "bacnet/basic/sys/filename.h"
#include "bacnet/basic/services.h"
#include "bacnet/basic/tsm/tsm.h"
#include "bacnet/datalink/datalink.h"
#include "bacnet/datalink/dlenv.h"
#include "bacport.h"

/* buffer used for receive */
static uint8_t Rx_Buf[MAX_MPDU] = { 0 };

/* global variables used in this file */
static uint32_t Target_Device_Object_Instance = BACNET_MAX_INSTANCE;
static BACNET_ADDRESS Target_Address;
static BACNET_REINITIALIZED_STATE Reinitialize_State = BACNET_REINIT_COLDSTART;
static char *Reinitialize_Password = NULL;

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
        "BACnet Error: %s: %s\r\n", bactext_error_class_name(error_class),
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
    printf("BACnet Abort: %s\r\n", bactext_abort_reason_name(abort_reason));
    Error_Detected = true;
}

static void
MyRejectHandler(BACNET_ADDRESS *src, uint8_t invoke_id, uint8_t reject_reason)
{
    /* FIXME: verify src and invoke id */
    (void)src;
    (void)invoke_id;
    printf("BACnet Reject: %s\r\n", bactext_reject_reason_name(reject_reason));
    Error_Detected = true;
}

static void
MyReinitializeDeviceSimpleAckHandler(BACNET_ADDRESS *src, uint8_t invoke_id)
{
    (void)src;
    (void)invoke_id;
    printf("ReinitializeDevice Acknowledged!\r\n");
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
    /* handle the ack coming back */
    apdu_set_confirmed_simple_ack_handler(
        SERVICE_CONFIRMED_REINITIALIZE_DEVICE,
        MyReinitializeDeviceSimpleAckHandler);
    /* handle any errors coming back */
    apdu_set_error_handler(
        SERVICE_CONFIRMED_REINITIALIZE_DEVICE, MyErrorHandler);
    apdu_set_abort_handler(MyAbortHandler);
    apdu_set_reject_handler(MyRejectHandler);
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

    if (argc < 3) {
        /* note: priority 16 and 0 should produce the same end results... */
        printf(
            "Usage: %s device-instance state [password]\r\n"
            "Send BACnet ReinitializeDevice service to device.\r\n"
            "\r\n"
            "The device-instance can be 0 to %d.\r\n"
            "Possible state values:\r\n"
            "  0=coldstart\r\n"
            "  1=warmstart\r\n"
            "  2=startbackup\r\n"
            "  3=endbackup\r\n"
            "  4=startrestore\r\n"
            "  5=endrestore\r\n"
            "  6=abortrestore\r\n"
            "The optional password is a character string of 1 to 20 "
            "characters.\r\n",
            filename_remove_path(argv[0]), BACNET_MAX_INSTANCE - 1);
        return 0;
    }
    /* decode the command line parameters */
    Target_Device_Object_Instance = strtol(argv[1], NULL, 0);
    Reinitialize_State = strtol(argv[2], NULL, 0);
    /* optional password */
    if (argc > 3) {
        Reinitialize_Password = argv[3];
    }

    if (Target_Device_Object_Instance > BACNET_MAX_INSTANCE) {
        fprintf(
            stderr, "device-instance=%u - not greater than %u\r\n",
            Target_Device_Object_Instance, BACNET_MAX_INSTANCE);
        return 1;
    }

    /* setup my info */
    Device_Set_Object_Instance_Number(BACNET_MAX_INSTANCE);
    address_init();
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
            tsm_timer_milliseconds(((current_seconds - last_seconds) * 1000));
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
                invoke_id = Send_Reinitialize_Device_Request(
                    Target_Device_Object_Instance, Reinitialize_State,
                    Reinitialize_Password);
            } else if (tsm_invoke_id_free(invoke_id)) {
                break;
            } else if (tsm_invoke_id_failed(invoke_id)) {
                fprintf(stderr, "\rError: TSM Timeout!\r\n");
                tsm_free_invoke_id(invoke_id);
                /* try again or abort? */
                Error_Detected = true;
                break;
            }
        } else {
            /* increment timer - exit if timed out */
            elapsed_seconds += (current_seconds - last_seconds);
            if (elapsed_seconds > timeout_seconds) {
                fprintf(stderr, "\rError: APDU Timeout!\r\n");
                Error_Detected = true;
                break;
            }
        }
        /* keep track of time for next check */
        last_seconds = current_seconds;
    }

    if (Error_Detected) {
        return 1;
    }
    return 0;
}
