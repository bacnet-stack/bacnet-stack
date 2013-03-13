/*************************************************************************
* Copyright (C) 2006 Steve Karg <skarg@users.sourceforge.net>
*
* Permission is hereby granted, free of charge, to any person obtaining
* a copy of this software and associated documentation files (the
* "Software"), to deal in the Software without restriction, including
* without limitation the rights to use, copy, modify, merge, publish,
* distribute, sublicense, and/or sell copies of the Software, and to
* permit persons to whom the Software is furnished to do so, subject to
* the following conditions:
*
* The above copyright notice and this permission notice shall be included
* in all copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*
*********************************************************************/

#define PRINT_ENABLED 1

/* command line tool that sends a BACnet service, and displays the reply */
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <time.h>       /* for time */
#if defined(WIN32) || defined(__BORLANDC__)
#include <conio.h>
#endif
#include "bacdef.h"
#include "config.h"
#include "bactext.h"
#include "bacerror.h"
#include "iam.h"
#include "arf.h"
#include "tsm.h"
#include "address.h"
#include "npdu.h"
#include "apdu.h"
#include "device.h"
#include "net.h"
#include "datalink.h"
#include "whois.h"
/* some demo stuff needed */
#include "filename.h"
#include "handlers.h"
#include "client.h"
#include "txbuf.h"
#include "dlenv.h"
#include "mydata.h"
#include "readrange.h"
#include "bactext.h"
/* include the objects */
#include "device.h"
#include "ai.h"
#include "ao.h"
#include "av.h"
#include "bi.h"
#include "bo.h"
#include "bv.h"
#include "lc.h"
#include "lsp.h"
#include "mso.h"
#include "trendlog.h"
#include "bacfile.h"

#if !defined(WIN32) && !defined(__BORLANDC__)
#define stricmp strcasecmp
#endif

#if defined(__BORLANDC__)
#define _kbhit kbhit
#define _stricmp stricmp
#endif

/* buffer used for receive */
static uint8_t Rx_Buf[MAX_MPDU] = { 0 };

/* global variables used in this file */
static uint32_t Target_Device_Object_Instance = BACNET_MAX_INSTANCE;

static int Target_Mode = 0;

static BACNET_ADDRESS Target_Address;
static bool Error_Detected = false;

static void MyErrorHandler(
    BACNET_ADDRESS * src,
    uint8_t invoke_id,
    BACNET_ERROR_CLASS error_class,
    BACNET_ERROR_CODE error_code)
{
    /* FIXME: verify src and invoke id */
    (void) src;
    (void) invoke_id;
    printf("BACnet Error: %s: %s\r\n",
        bactext_error_class_name((int) error_class),
        bactext_error_code_name((int) error_code));
/*    Error_Detected = true; */
}

void MyAbortHandler(
    BACNET_ADDRESS * src,
    uint8_t invoke_id,
    uint8_t abort_reason,
    bool server)
{
    /* FIXME: verify src and invoke id */
    (void) src;
    (void) invoke_id;
    (void) server;
    printf("BACnet Abort: %s\r\n",
        bactext_abort_reason_name((int) abort_reason));
/*    Error_Detected = true; */
}

void MyRejectHandler(
    BACNET_ADDRESS * src,
    uint8_t invoke_id,
    uint8_t reject_reason)
{
    /* FIXME: verify src and invoke id */
    (void) src;
    (void) invoke_id;
    printf("BACnet Reject: %s\r\n",
        bactext_reject_reason_name((int) reject_reason));
/*    Error_Detected = true; */
}

static void Init_Objects(
    void)
{
    Device_Init(NULL);
}

static void Init_Service_Handlers(
    void)
{
    /* we need to handle who-isx
       to support dynamic device binding to us */
    apdu_set_unconfirmed_handler(SERVICE_UNCONFIRMED_WHO_IS, handler_who_is);
    /* handle i-am to support binding to other devices */
#if 0
    apdu_set_unconfirmed_handler(SERVICE_UNCONFIRMED_I_AM, handler_i_am_bind);
#else
    apdu_set_unconfirmed_handler(SERVICE_UNCONFIRMED_I_AM, handler_i_am_add);
#endif
    /* set the handler for all the services we don't implement
       It is required to send the proper reject message... */
    apdu_set_unrecognized_service_handler_handler
        (handler_unrecognized_service);
    /* we must implement read property - it's required! */
    apdu_set_confirmed_handler(SERVICE_CONFIRMED_READ_PROPERTY,
        handler_read_property);
    apdu_set_confirmed_handler(SERVICE_CONFIRMED_READ_PROP_MULTIPLE,
        handler_read_property_multiple);
    apdu_set_confirmed_handler(SERVICE_CONFIRMED_WRITE_PROPERTY,
        handler_write_property);
    apdu_set_confirmed_handler(SERVICE_CONFIRMED_READ_RANGE,
        handler_read_range);
    apdu_set_confirmed_handler(SERVICE_CONFIRMED_PRIVATE_TRANSFER,
        handler_conf_private_trans);
    /* handle the data coming back from confirmed requests */
    apdu_set_confirmed_ack_handler(SERVICE_CONFIRMED_READ_PROPERTY,
        handler_read_property_ack);
    apdu_set_confirmed_ack_handler(SERVICE_CONFIRMED_READ_RANGE,
        handler_read_range_ack);
    apdu_set_confirmed_ack_handler(SERVICE_CONFIRMED_PRIVATE_TRANSFER,
        handler_conf_private_trans_ack);

    /* handle any errors coming back */
    apdu_set_error_handler(SERVICE_CONFIRMED_READ_PROPERTY, MyErrorHandler);
    apdu_set_error_handler(SERVICE_CONFIRMED_PRIVATE_TRANSFER, MyErrorHandler);
    apdu_set_abort_handler(MyAbortHandler);
    apdu_set_reject_handler(MyRejectHandler);
}

int main(
    int argc,
    char *argv[])
{
    BACNET_ADDRESS src = {
        0
    };  /* address where message came from */
    uint16_t pdu_len = 0;
    unsigned timeout = 100;     /* milliseconds */
    unsigned max_apdu = 0;
    time_t elapsed_seconds = 0;
    time_t last_seconds = 0;
    time_t current_seconds = 0;
    time_t timeout_seconds = 0;
    uint8_t invoke_id = 0;
    bool found = false;
    BACNET_READ_RANGE_DATA Request;
    int iCount = 0;
    int iType = 0;
    int iKey;
    int iSecondsRun = 0;


    if (((argc != 2) && (argc != 3)) || ((argc >= 2) &&
            (strcmp(argv[1], "--help") == 0))) {
        printf("%s\n", argv[0]);
        printf("Usage: %s server local-device-instance\r\n       or\r\n"
            "       %s remote-device-instance\r\n"
            "--help gives further information\r\n",
            filename_remove_path(argv[0]), filename_remove_path(argv[0]));
        if ((argc > 1) && (strcmp(argv[1], "--help") == 0)) {
            printf("\r\nServer mode:\r\n\r\n"
                "<local-device-instance> determins the device id of the application\r\n"
                "when running as the server end of a test set up. The Server simply\r\n"
                "returns dummy data for each ReadRange request\r\n\r\n"
                "Non server:\r\n\r\n"
                "<remote-device-instance> indicates the device id of the server\r\n"
                "instance of the application.\r\n"
                "The non server application will send a series of ReadRange requests to the\r\n"
                "server with examples of different range types.\r\n");
        }
        return 0;
    }
    /* decode the command line parameters */
    if (_stricmp(argv[1], "server") == 0)
        Target_Mode = 1;
    else
        Target_Mode = 0;

    Target_Device_Object_Instance = strtol(argv[1 + Target_Mode], NULL, 0);

    if (Target_Device_Object_Instance > BACNET_MAX_INSTANCE) {
        fprintf(stderr, "device-instance=%u - it must be less than %u\r\n",
            Target_Device_Object_Instance, BACNET_MAX_INSTANCE);
        return 1;
    }

    /* setup my info */
    if (Target_Mode)
        Device_Set_Object_Instance_Number(Target_Device_Object_Instance);
    else
        Device_Set_Object_Instance_Number(BACNET_MAX_INSTANCE);

    Init_Objects();
    address_init();
    Init_Service_Handlers();
    dlenv_init();
    atexit(datalink_cleanup);
    /* configure the timeout values */
    last_seconds = time(NULL);
    timeout_seconds = (apdu_timeout() / 1000) * apdu_retries();

    if (Target_Mode) {
#if defined(WIN32) || defined(__BORLANDC__)
        printf("Entering server mode. press q to quit program\r\n\r\n");
#else
        printf("Entering server mode.\r\n\r\n");
#endif
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
                putchar('.');   /* Just to show that time is passing... */
                tsm_timer_milliseconds(((current_seconds -
                            last_seconds) * 1000));
                address_cache_timer(current_seconds - last_seconds);
                trend_log_timer(current_seconds - last_seconds);
                last_seconds = current_seconds;
                /* Change the analog input PVs for testing purposes */
                for (iCount = 0; iCount < Analog_Input_Count(); iCount++) {
                    Analog_Input_Present_Value_Set(iCount,
                        iSecondsRun * (iCount + 1));
                }

                iSecondsRun++;
            }
#if defined(WIN32) || defined(__BORLANDC__)
            if (_kbhit()) {
                iKey = toupper(_getch());
                if (iKey == 'Q') {
                    printf("\r\nExiting program now\r\n");
                    exit(0);
                }
            }
#endif
        }
    } else {

        /* try to bind with the device */
        found =
            address_bind_request(Target_Device_Object_Instance, &max_apdu,
            &Target_Address);
        if (!found) {
            Send_WhoIs(Target_Device_Object_Instance,
                Target_Device_Object_Instance);
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
                tsm_timer_milliseconds(((current_seconds -
                            last_seconds) * 1000));
                address_cache_timer(current_seconds - last_seconds);
                trend_log_timer(current_seconds - last_seconds);
                last_seconds = current_seconds;
            }
            if (Error_Detected)
                break;
            /* wait until the device is bound, or timeout and quit */
            if (!found)
                found =
                    address_bind_request(Target_Device_Object_Instance,
                    &max_apdu, &Target_Address);
            if (found) {
                if (invoke_id == 0) {   /* Safe to send a new request */
                    switch (iCount) {
                        case 0:        /* Pass - should read up to 1st 10 */
                            Request.RequestType = RR_BY_POSITION;
                            Request.Range.RefIndex = 1;
                            Request.Count = 10;
                            Request.object_type = OBJECT_DEVICE;
                            Request.object_instance =
                                Target_Device_Object_Instance;
                            Request.object_property =
                                PROP_DEVICE_ADDRESS_BINDING;
                            Request.array_index = 0;
                            break;

                        case 1:        /* Pass - should read entries 2 and 3 */
                            Request.RequestType = RR_BY_POSITION;
                            Request.Range.RefSeqNum = 3;
                            Request.Count = -2;
                            Request.object_type = OBJECT_DEVICE;
                            Request.object_instance =
                                Target_Device_Object_Instance;
                            Request.object_property =
                                PROP_DEVICE_ADDRESS_BINDING;
                            Request.array_index = 0;
                            break;

                        case 2:        /* Fail - By Time not supported */
                            Request.RequestType = RR_BY_TIME;
                            Request.Range.RefTime.date.year = 2009;
                            Request.Range.RefTime.date.month = 9;
                            Request.Range.RefTime.date.day = 23;
                            Request.Range.RefTime.date.wday = 0xFF;
                            Request.Range.RefTime.time.hour = 22;
                            Request.Range.RefTime.time.min = 23;
                            Request.Range.RefTime.time.sec = 24;
                            Request.Range.RefTime.time.hundredths = 0;

                            Request.Count = 10;
                            Request.object_type = OBJECT_DEVICE;
                            Request.object_instance =
                                Target_Device_Object_Instance;
                            Request.object_property =
                                PROP_DEVICE_ADDRESS_BINDING;
                            Request.array_index = 0;
                            break;

                        case 3:        /* Fail - array not supported */
                            Request.RequestType = RR_BY_POSITION;
                            Request.Range.RefIndex = 1;
                            Request.Count = 10;
                            Request.object_type = OBJECT_DEVICE;
                            Request.object_instance =
                                Target_Device_Object_Instance;
                            Request.object_property =
                                PROP_DEVICE_ADDRESS_BINDING;
                            Request.array_index = 1;
                            break;

                        case 4:        /* Fail - By Sequence not supported */
                            Request.RequestType = RR_BY_SEQUENCE;
                            Request.Range.RefSeqNum = 1;
                            Request.Count = 10;
                            Request.object_type = OBJECT_DEVICE;
                            Request.object_instance =
                                Target_Device_Object_Instance;
                            Request.object_property =
                                PROP_DEVICE_ADDRESS_BINDING;
                            Request.array_index = 0;
                            break;

                        case 5:        /* Fail Bytime not supported and array not supported */
                            Request.RequestType = RR_BY_TIME;
                            Request.Range.RefTime.date.year = 2009;
                            Request.Range.RefTime.date.month = 9;
                            Request.Range.RefTime.date.day = 23;
                            Request.Range.RefTime.date.wday = 0xFF;     /* Day of week unspecified */
                            Request.Range.RefTime.time.hour = 22;
                            Request.Range.RefTime.time.min = 23;
                            Request.Range.RefTime.time.sec = 24;
                            Request.Range.RefTime.time.hundredths = 0;

                            Request.Count = 10;
                            Request.object_type = OBJECT_DEVICE;
                            Request.object_instance =
                                Target_Device_Object_Instance;
                            Request.object_property =
                                PROP_DEVICE_ADDRESS_BINDING;
                            Request.array_index = 1;
                            break;

                        case 6:        /* Pass - should try to return all entries */
                            Request.RequestType = RR_READ_ALL;
                            Request.object_type = OBJECT_DEVICE;
                            Request.object_instance =
                                Target_Device_Object_Instance;
                            Request.object_property =
                                PROP_DEVICE_ADDRESS_BINDING;
                            Request.array_index = 0;
                            break;

                        case 7:        /* Fail - array not supported */
                            Request.RequestType = RR_READ_ALL;
                            Request.object_type = OBJECT_DEVICE;
                            Request.object_instance =
                                Target_Device_Object_Instance;
                            Request.object_property =
                                PROP_DEVICE_ADDRESS_BINDING;
                            Request.array_index = 1;
                            break;

                        case 8:        /* Pass - should read 1st 1 */
                            Request.RequestType = RR_BY_POSITION;
                            Request.Range.RefIndex = 1;
                            Request.Count = 1;
                            Request.object_type = OBJECT_DEVICE;
                            Request.object_instance =
                                Target_Device_Object_Instance;
                            Request.object_property =
                                PROP_DEVICE_ADDRESS_BINDING;
                            Request.array_index = 0;
                            break;

                        case 9:        /* Pass - should read 1st 2 */
                            Request.RequestType = RR_BY_POSITION;
                            Request.Range.RefIndex = 1;
                            Request.Count = 2;
                            Request.object_type = OBJECT_DEVICE;
                            Request.object_instance =
                                Target_Device_Object_Instance;
                            Request.object_property =
                                PROP_DEVICE_ADDRESS_BINDING;
                            Request.array_index = 0;
                            break;

                        case 10:       /* Pass - should read 2nd and 3rd */
                            Request.RequestType = RR_BY_POSITION;
                            Request.Range.RefIndex = 2;
                            Request.Count = 2;
                            Request.object_type = OBJECT_DEVICE;
                            Request.object_instance =
                                Target_Device_Object_Instance;
                            Request.object_property =
                                PROP_DEVICE_ADDRESS_BINDING;
                            Request.array_index = 0;
                            break;

                        case 11:       /* Pass - should read 2nd up to 11th */
                            Request.RequestType = RR_BY_POSITION;
                            Request.Range.RefIndex = 2;
                            Request.Count = 10;
                            Request.object_type = OBJECT_DEVICE;
                            Request.object_instance =
                                Target_Device_Object_Instance;
                            Request.object_property =
                                PROP_DEVICE_ADDRESS_BINDING;
                            Request.array_index = 0;
                            break;
                    }

                    invoke_id =
                        Send_ReadRange_Request(Target_Device_Object_Instance,
                        &Request);
                } else if (tsm_invoke_id_free(invoke_id)) {
                    if (iCount != 11) {
                        iCount++;
                        invoke_id = 0;
                    } else {
                        break;
                    }
                } else if (tsm_invoke_id_failed(invoke_id)) {
                    fprintf(stderr, "\rError: TSM Timeout!\r\n");
                    tsm_free_invoke_id(invoke_id);
                    /* Error_Detected = true; */
                    /* try again or abort? */
                    invoke_id = 0;      /* Try next operation */
                    /* break; */
                }
            } else {
                /* increment timer - exit if timed out */
                elapsed_seconds += (current_seconds - last_seconds);
                if (elapsed_seconds > timeout_seconds) {
                    printf("\rError: APDU Timeout!\r\n");
                    /* Error_Detected = true;
                       break; */
                    invoke_id = 0;
                }
            }
            /* keep track of time for next check */
            last_seconds = current_seconds;
        }
    }

    if (Error_Detected)
        return 1;
    return 0;
}
