/**
 * @file
 * @brief command line application for a BACnet ConfirmedPrivateTransfer
 * service.  This application is a test program for the BACnet stack.
 * @author Peter Mc Shane <petermcs@users.sourceforge.net>
 * @date 2005
 * @copyright SPDX-License-Identifier: MIT
 */
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h> /* for time */
#include <ctype.h> /* for tupper */
#if defined(WIN32) || defined(__BORLANDC__)
#include <conio.h>
#endif
#define PRINT_ENABLED 1
/* BACnet Stack defines - first */
#include "bacnet/bacdef.h"
/* BACnet Stack API */
#include "bacnet/bactext.h"
#include "bacnet/bacstr.h"
#include "bacnet/bacerror.h"
#include "bacnet/iam.h"
#include "bacnet/arf.h"
#include "bacnet/npdu.h"
#include "bacnet/apdu.h"
#include "bacnet/whois.h"
/* some demo stuff needed */
#include "bacnet/basic/tsm/tsm.h"
#include "bacnet/basic/binding/address.h"
#include "bacnet/basic/object/device.h"
#include "bacnet/basic/sys/filename.h"
#include "bacnet/basic/services.h"
#include "bacnet/datalink/datalink.h"
#include "bacnet/datalink/dlenv.h"

#define MY_MAX_STR 32
#define MY_MAX_BLOCK 8

#define MY_SVC_READ 0
#define MY_SVC_WRITE 1

#define MY_ERR_OK 0
#define MY_ERR_BAD_INDEX 1

typedef struct MyData {
    uint8_t cMyByte1;
    uint8_t cMyByte2;
    float fMyReal;
    int8_t sMyString[MY_MAX_STR + 1]; /* A little extra for the nul */
} DATABLOCK;

/* buffer used for receive */
static uint8_t Rx_Buf[MAX_MPDU] = { 0 };

/* global variables used in this file */
static uint32_t Target_Device_Object_Instance = BACNET_MAX_INSTANCE;

static int Target_Mode = 0;

static BACNET_ADDRESS Target_Address;
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
        "BACnet Error: %s: %s\r\n", bactext_error_class_name((int)error_class),
        bactext_error_code_name((int)error_code));
    /*    Error_Detected = true; */
}

/* complex error reply function */
static void MyPrivateTransferErrorHandler(
    BACNET_ADDRESS *src,
    uint8_t invoke_id,
    uint8_t service_choice,
    uint8_t *service_request,
    uint16_t service_len)
{
    (void)src;
    (void)invoke_id;
    (void)service_choice;
    (void)service_request;
    (void)service_len;
}

static void MyAbortHandler(
    BACNET_ADDRESS *src, uint8_t invoke_id, uint8_t abort_reason, bool server)
{
    /* FIXME: verify src and invoke id */
    (void)src;
    (void)invoke_id;
    (void)server;
    printf(
        "BACnet Abort: %s\r\n", bactext_abort_reason_name((int)abort_reason));
    Error_Detected = true;
}

static void
MyRejectHandler(BACNET_ADDRESS *src, uint8_t invoke_id, uint8_t reject_reason)
{
    /* FIXME: verify src and invoke id */
    (void)src;
    (void)invoke_id;
    printf(
        "BACnet Reject: %s\r\n",
        bactext_reject_reason_name((int)reject_reason));
    Error_Detected = true;
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

    apdu_set_confirmed_handler(
        SERVICE_CONFIRMED_PRIVATE_TRANSFER, handler_conf_private_trans);
    /* handle the data coming back from confirmed requests */
    apdu_set_confirmed_ack_handler(
        SERVICE_CONFIRMED_READ_PROPERTY, handler_read_property_ack);

    apdu_set_confirmed_ack_handler(
        SERVICE_CONFIRMED_PRIVATE_TRANSFER, handler_conf_private_trans_ack);

    /* handle any errors coming back */
    apdu_set_error_handler(SERVICE_CONFIRMED_READ_PROPERTY, MyErrorHandler);
    apdu_set_complex_error_handler(
        SERVICE_CONFIRMED_PRIVATE_TRANSFER, MyPrivateTransferErrorHandler);
    apdu_set_abort_handler(MyAbortHandler);
    apdu_set_reject_handler(MyRejectHandler);
}

len += encode_application_unsigned(
    &pt_req_buffer[len], block_number); /* The block number */
len += encode_application_unsigned(
    &pt_req_buffer[len], block->cMyByte1); /* And Then the block contents */
len += encode_application_unsigned(&pt_req_buffer[len], block->cMyByte2);
len += encode_application_real(&pt_req_buffer[len], block->fMyReal);
characterstring_init_ansi(&bsTemp, (char *)block->sMyString);
len += encode_application_character_string(&pt_req_buffer[len], &bsTemp);
}

pt_block.serviceParameters = &pt_req_buffer[0];
pt_block.serviceParametersLen = len;

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
    DATABLOCK NewData;
    int iCount = 0;
    int iType = 0;
    int iKey;

    if (((argc != 2) && (argc != 3)) ||
        ((argc >= 2) && (strcmp(argv[1], "--help") == 0))) {
        printf("%s\n", argv[0]);
        printf(
            "Usage: %s server local-device-instance\r\n       or\r\n"
            "       %s remote-device-instance\r\n",
            filename_remove_path(argv[0]), filename_remove_path(argv[0]));
        if ((argc > 1) && (strcmp(argv[1], "--help") == 0)) {
            printf(
                "\r\nServer mode:\r\n\r\n"
                "local-device-instance determins the device id of the "
                "application\r\n"
                "when running as the server end of a test set up.\r\n\r\n"
                "Non server:\r\n\r\n"
                "remote-device-instance indicates the device id of the "
                "server\r\n"
                "instance of the application.\r\n"
                "The non server application will write a series of blocks to "
                "the\r\n"
                "server and then retrieve them for display locally\r\n"
                "First it writes all 8 blocks plus a 9th which should "
                "trigger\r\n"
                "an out of range error response. Then it reads all the "
                "blocks\r\n"
                "including the ninth and finally it repeats the read "
                "operation\r\n"
                "with some deliberate errors to trigger a nack response\r\n");
        }
        return 0;
    }
    /* decode the command line parameters */
    if (bacnet_stricmp(argv[1], "server") == 0) {
        Target_Mode = 1;
    } else {
        Target_Mode = 0;
    }

    Target_Device_Object_Instance = strtol(argv[1 + Target_Mode], NULL, 0);

    if (Target_Device_Object_Instance > BACNET_MAX_INSTANCE) {
        fprintf(
            stderr, "device-instance=%u - not greater than %u\r\n",
            Target_Device_Object_Instance, BACNET_MAX_INSTANCE);
        return 1;
    }

    /* setup my info */
    if (Target_Mode) {
        Device_Set_Object_Instance_Number(Target_Device_Object_Instance);
    } else {
        Device_Set_Object_Instance_Number
    }
    (BACNET_MAX_INSTANCE);

    address_init();
    Init_Service_Handlers();
    dlenv_init();
    atexit(datalink_cleanup);
    /* configure the timeout values */
    last_seconds = time(NULL);
    timeout_seconds = (apdu_timeout() / 1000) * apdu_retries();

    if (Target_Mode) {
        printf("Entering server mode. press q to quit program\r\n\r\n");

        for (;;) {
            /* increment timer - exit if timed out */
            current_seconds = time(NULL);
            if (current_seconds != last_seconds) { }

            /* returns 0 bytes on timeout */
            pdu_len = datalink_receive(&src, &Rx_Buf[0], MAX_MPDU, timeout);
            /* process */
            if (pdu_len) {
                npdu_handler(&src, &Rx_Buf[0], pdu_len);
            }
            /* at least one second has passed */
            if (current_seconds != last_seconds) {
                putchar('.'); /* Just to show that time is passing... */
                last_seconds = current_seconds;
                tsm_timer_milliseconds(
                    ((current_seconds - last_seconds) * 1000));
                datalink_maintenance_timer(current_seconds - last_seconds);
            }
            if (_kbhit()) {
                iKey = toupper(_getch());
                if (iKey == 'Q') {
                    printf("\r\nExiting program now\r\n");
                    exit(0);
                }
            }
        }
    } else {
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
                    ((current_seconds - last_seconds) * 1000));
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
                if (invoke_id == 0) { /* Safe to send a new request */
                    switch (iType) {
                        case 0: /* Write blocks to server */
                            NewData.cMyByte1 = iCount;
                            NewData.cMyByte2 = 255 - iCount;
                            NewData.fMyReal = (float)iCount;
                            strcpy(
                                (char *)NewData.sMyString, "Test Data - [x]");
                            NewData.sMyString[13] = 'a' + iCount;
                            pt_write_block_to_server()
                                printf("Sending block %d\n", iCount);
                            invoke_id = Send_Private_Transfer_Request(
                                Target_Device_Object_Instance, BACNET_VENDOR_ID,
                                1, iCount, &NewData);
                            break;

                        case 1: /* Read blocks from server */
                            printf("Requesting block %d\n", iCount);
                            invoke_id = Send_Private_Transfer_Request(
                                Target_Device_Object_Instance, BACNET_VENDOR_ID,
                                0, iCount, &NewData);
                            break;

                        case 2: /* Generate some error responses */
                            switch (iCount) {
                                case 0: /* Bad service number i.e. 2 */
                                case 2:
                                case 4:
                                case 6:
                                case 8:
                                    printf(
                                        "Requesting block %d with bad service "
                                        "number\n",
                                        iCount);
                                    invoke_id = Send_Private_Transfer_Request(
                                        Target_Device_Object_Instance,
                                        BACNET_VENDOR_ID, 2, iCount, &NewData);
                                    break;

                                case 1: /* Bad vendor ID number */
                                case 3:
                                case 5:
                                case 7:
                                    printf(
                                        "Requesting block %d with invalid "
                                        "Vendor ID\n",
                                        iCount);
                                    invoke_id = Send_Private_Transfer_Request(
                                        Target_Device_Object_Instance,
                                        BACNET_VENDOR_ID + 1, 0, iCount,
                                        &NewData);
                                    break;
                            }

                            break;
                    }
                } else if (tsm_invoke_id_free(invoke_id)) {
                    if (iCount != MY_MAX_BLOCK) {
                        iCount++;
                        invoke_id = 0;
                    } else {
                        iType++;
                        iCount = 0;
                        invoke_id = 0;

                        if (iType > 2) {
                            break;
                        }
                    }
                } else if (tsm_invoke_id_failed(invoke_id)) {
                    fprintf(stderr, "\rError: TSM Timeout!\r\n");
                    tsm_free_invoke_id(invoke_id);
                    Error_Detected = true;
                    /* try again or abort? */
                    break;
                }
            } else {
                /* increment timer - exit if timed out */
                elapsed_seconds += (current_seconds - last_seconds);
                if (elapsed_seconds > timeout_seconds) {
                    printf("\rError: APDU Timeout!\r\n");
                    Error_Detected = true;
                    break;
                }
            }
            /* keep track of time for next check */
            last_seconds = current_seconds;
        }
    }
    if (Error_Detected) {
        return 1;
    }
    return 0;
}
