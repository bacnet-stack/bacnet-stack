/**
 * @file
 * @brief command line tool that sends a BACnet ConfirmedPrivateTransfer
 * message with one or more application values.
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date 2026
 * @copyright SPDX-License-Identifier: MIT
 */
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#define PRINT_ENABLED 1
/* BACnet Stack defines - first */
#include "bacnet/bacdef.h"
/* BACnet Stack API */
#include "bacnet/bacapp.h"
#include "bacnet/bacerror.h"
#include "bacnet/bactext.h"
#include "bacnet/iam.h"
#include "bacnet/npdu.h"
#include "bacnet/apdu.h"
#include "bacnet/ptransfer.h"
#include "bacnet/whois.h"
/* some demo stuff needed */
#include "bacnet/basic/binding/address.h"
#include "bacnet/basic/object/device.h"
#include "bacnet/basic/service/h_pt.h"
#include "bacnet/basic/service/h_pt_a.h"
#include "bacnet/basic/service/s_pt.h"
#include "bacnet/basic/sys/filename.h"
#include "bacnet/basic/sys/mstimer.h"
#include "bacnet/basic/services.h"
#include "bacnet/basic/tsm/tsm.h"
#include "bacnet/datalink/datalink.h"
#include "bacnet/datalink/dlenv.h"
#include "bacport.h"

#ifndef MAX_PROPERTY_VALUES
#define MAX_PROPERTY_VALUES 64
#endif

static uint8_t Rx_Buf[MAX_MPDU] = { 0 };
static uint32_t Target_Device_Object_Instance = BACNET_MAX_INSTANCE;
static uint16_t Target_Vendor_Identifier = 260;
static uint32_t Target_Service_Number = 0;
static BACNET_APPLICATION_DATA_VALUE
    Target_Object_Property_Value[MAX_PROPERTY_VALUES];
static BACNET_ADDRESS Target_Address;
static uint8_t Request_Invoke_ID = 0;
static bool Error_Detected = false;

static void MyPrivateTransferErrorHandler(
    BACNET_ADDRESS *src,
    uint8_t invoke_id,
    uint8_t service_choice,
    uint8_t *service_request,
    uint16_t service_len)
{
    BACNET_ERROR_CLASS error_class = ERROR_CLASS_DEVICE;
    BACNET_ERROR_CODE error_code = ERROR_CODE_OTHER;
    BACNET_PRIVATE_TRANSFER_DATA private_data = { 0 };
    int len = 0;

    (void)service_choice;
    if (address_match(&Target_Address, src) &&
        (invoke_id == Request_Invoke_ID)) {
        len = ptransfer_error_decode_service_request(
            service_request, service_len, &error_class, &error_code,
            &private_data);
        if (len > 0) {
            printf(
                "BACnet Error: %s: %s\n",
                bactext_error_class_name((int)error_class),
                bactext_error_code_name((int)error_code));
            printf(
                "PrivateTransfer: vendorID=%u serviceNumber=%lu\n",
                (unsigned)private_data.vendorID,
                (unsigned long)private_data.serviceNumber);
        }
        Error_Detected = true;
    }
}

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

static void Init_Service_Handlers(void)
{
    Device_Init(NULL);
    apdu_set_unconfirmed_handler(SERVICE_UNCONFIRMED_WHO_IS, handler_who_is);
    apdu_set_unconfirmed_handler(SERVICE_UNCONFIRMED_I_AM, handler_i_am_bind);
    apdu_set_unrecognized_service_handler_handler(handler_unrecognized_service);
    apdu_set_confirmed_handler(
        SERVICE_CONFIRMED_READ_PROPERTY, handler_read_property);
    apdu_set_confirmed_handler(
        SERVICE_CONFIRMED_PRIVATE_TRANSFER, handler_conf_private_trans);
    apdu_set_confirmed_ack_handler(
        SERVICE_CONFIRMED_PRIVATE_TRANSFER, handler_conf_private_trans_ack);
    apdu_set_complex_error_handler(
        SERVICE_CONFIRMED_PRIVATE_TRANSFER, MyPrivateTransferErrorHandler);
    apdu_set_abort_handler(MyAbortHandler);
    apdu_set_reject_handler(MyRejectHandler);
}

static void print_usage(const char *filename)
{
    printf(
        "Usage: %s device-instance vendor-id service-number tag value "
        "[tag value...]\n",
        filename);
    printf("       [--help]\n");
}

static void print_help(const char *filename)
{
    printf("Send a BACnet ConfirmedPrivateTransfer with one or more values.\n");
    printf("device-instance:\n"
           "BACnet Device Object Instance number that you are\n"
           "trying to communicate with.\n");
    printf("vendor-id:\n"
           "The specific vendor ID for the proprietary service.\n");
    printf("service-number:\n"
           "The vendor-defined private service number.\n");
    printf("tag:\n"
           "BACNET_APPLICATION_TAG value for the data item.\n");
    printf("value:\n"
           "ASCII representation of the payload value to encode.\n");
    printf(
        "Example:\n"
        "%s 99 260 23 4 1.1 2 42\n",
        filename);
}

int main(int argc, char *argv[])
{
    BACNET_ADDRESS src = { 0 };
    uint16_t pdu_len = 0;
    unsigned timeout = 100;
    unsigned max_apdu = 0;
    unsigned timeout_milliseconds = 0;
    bool found = false;
    const char *filename = NULL;
    char *value_string = NULL;
    struct mstimer apdu_timer = { 0 };
    struct mstimer datalink_timer = { 0 };
    bool status = false;
    int args_remaining = 0;
    int tag_value_arg = 0;
    int i = 0;
    BACNET_APPLICATION_TAG property_tag;
    uint8_t context_tag = 0;
    uint8_t tx_buffer[MAX_APDU] = { 0 };
    bool sent_message = false;

    if ((argc < 6) || ((argc > 1) && (strcmp(argv[1], "--help") == 0))) {
        filename = filename_remove_path(argv[0]);
        print_usage(filename);
        if ((argc > 1) && (strcmp(argv[1], "--help") == 0)) {
            print_help(filename);
        }
        return 0;
    }

    Target_Device_Object_Instance = strtol(argv[1], NULL, 0);
    Target_Vendor_Identifier = strtol(argv[2], NULL, 0);
    Target_Service_Number = strtol(argv[3], NULL, 0);

    if (Target_Device_Object_Instance > BACNET_MAX_INSTANCE) {
        fprintf(
            stderr, "device-instance=%u - not greater than %u\n",
            Target_Device_Object_Instance, BACNET_MAX_INSTANCE);
        return 1;
    }

    args_remaining = argc - 4;
    for (i = 0; i < MAX_PROPERTY_VALUES; i++) {
        tag_value_arg = 4 + (i * 2);
        if (tag_value_arg >= argc) {
            break;
        }
        if (toupper(argv[tag_value_arg][0]) == 'C') {
            context_tag = strtol(&argv[tag_value_arg][1], NULL, 0);
            tag_value_arg++;
            args_remaining--;
            Target_Object_Property_Value[i].context_tag = context_tag;
            Target_Object_Property_Value[i].context_specific = true;
        } else {
            Target_Object_Property_Value[i].context_specific = false;
        }
        property_tag = strtol(argv[tag_value_arg], NULL, 0);
        args_remaining--;
        if (tag_value_arg + 1 >= argc) {
            fprintf(stderr, "Error: not enough tag-value pairs\n");
            return 1;
        }
        value_string = argv[tag_value_arg + 1];
        args_remaining--;
        if (property_tag >= MAX_BACNET_APPLICATION_TAG) {
            fprintf(
                stderr, "Error: tag=%u - it must be less than %u\n",
                property_tag, MAX_BACNET_APPLICATION_TAG);
            return 1;
        }
        status = bacapp_parse_application_data(
            property_tag, value_string, &Target_Object_Property_Value[i]);
        if (!status) {
            fprintf(stderr, "Error: unable to parse the tag value\n");
            return 1;
        }
        Target_Object_Property_Value[i].next = NULL;
        if (i > 0) {
            Target_Object_Property_Value[i - 1].next =
                &Target_Object_Property_Value[i];
        }
        if (args_remaining <= 0) {
            break;
        }
    }

    if (args_remaining > 0) {
        fprintf(
            stderr, "Error: Exceeded %d tag-value pairs.\n",
            MAX_PROPERTY_VALUES);
        return 1;
    }

    Device_Set_Object_Instance_Number(BACNET_MAX_INSTANCE);
    address_init();
    Init_Service_Handlers();
    dlenv_init();
    atexit(datalink_cleanup);
    mstimer_init();
    timeout_milliseconds = apdu_timeout() * apdu_retries();
    mstimer_set(&apdu_timer, timeout_milliseconds);
    mstimer_set(&datalink_timer, 1000);

    found = address_bind_request(
        Target_Device_Object_Instance, &max_apdu, &Target_Address);
    if (!found) {
        Send_WhoIs(
            Target_Device_Object_Instance, Target_Device_Object_Instance);
    }

    for (;;) {
        pdu_len = datalink_receive(&src, &Rx_Buf[0], MAX_MPDU, timeout);
        if (pdu_len) {
            npdu_handler(&src, &Rx_Buf[0], pdu_len);
        }

        if (Error_Detected) {
            break;
        }

        if (!found) {
            found = address_bind_request(
                Target_Device_Object_Instance, &max_apdu, &Target_Address);
        }

        if (mstimer_expired(&datalink_timer)) {
            datalink_maintenance_timer(
                mstimer_interval(&datalink_timer) / 1000);
            mstimer_reset(&datalink_timer);
        }

        if (!sent_message) {
            if (found) {
                Request_Invoke_ID = Send_Private_Transfer_Request(
                    &tx_buffer[0], sizeof(tx_buffer),
                    Target_Device_Object_Instance, Target_Vendor_Identifier,
                    Target_Service_Number, &Target_Object_Property_Value[0]);
                if (!Request_Invoke_ID) {
                    printf("Error: failed to send PrivateTransfer\n");
                    Error_Detected = true;
                    break;
                }
                printf(
                    "Sent ConfirmedPrivateTransfer to Device %u.\n",
                    Target_Device_Object_Instance);
                sent_message = true;
            } else if (mstimer_expired(&apdu_timer)) {
                printf("\rError: APDU Timeout!\n");
                Error_Detected = true;
                break;
            }
        }

        if (sent_message && mstimer_expired(&apdu_timer)) {
            printf("\rError: APDU Timeout!\n");
            Error_Detected = true;
            break;
        }
    }

    if (Error_Detected) {
        return 1;
    }
    return 0;
}
