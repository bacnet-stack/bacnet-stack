/**
 * @file
 * @brief Application to send an arbitrary BACnet APDU message
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date February 2024
 *
 * SPDX-License-Identifier: MIT
 */
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <errno.h>
#include "bacnet/bactext.h"
#include "bacnet/iam.h"
#include "bacnet/basic/binding/address.h"
#include "bacnet/config.h"
#include "bacnet/bacdef.h"
#include "bacnet/npdu.h"
#include "bacnet/abort.h"
#include "bacnet/apdu.h"
#include "bacnet/basic/object/device.h"
#include "bacnet/datalink/datalink.h"
#include "bacnet/bactext.h"
#include "bacnet/reject.h"
#include "bacnet/version.h"
/* some demo stuff needed */
#include "bacnet/basic/sys/debug.h"
#include "bacnet/basic/sys/mstimer.h"
#include "bacnet/basic/sys/filename.h"
#include "bacnet/basic/services.h"
#include "bacnet/basic/tsm/tsm.h"
#include "bacnet/datalink/dlenv.h"
#include "bacport.h"

/* buffer used for receive */
static uint8_t Rx_Buf[MAX_MPDU] = { 0 };
/* buffer used for transmit */
static const char *APDU_Hex_ASCII;
static uint8_t APDU_Buf[MAX_APDU] = { 0 };
static size_t APDU_Buf_Len = 0;

/* global variables used in this file */

/* target device data for the request */
static BACNET_ADDRESS Target_Address;
static uint32_t Target_Device_Object_Instance = BACNET_MAX_INSTANCE + 1;
/* needed to filter incoming messages */
static uint8_t Request_Invoke_ID = 0;
/* track errors for early exit */
static bool Error_Detected = false;
/* debug info printing */
static bool BACnet_Debug_Enabled;

/**
 * @brief Print error messages
 */
static void MyPrintHandler(const char *hex_ascii,
    BACNET_ERROR_CLASS error_class,
    BACNET_ERROR_CODE error_code)
{
    debug_printf("[{\n");
    debug_printf("  \"%s\": {\n"
                 "    \"error-class\": \"%s\",\n    \"error-code\": \"%s\"",
        hex_ascii, bactext_error_class_name((int)error_class),
        bactext_error_code_name((int)error_code));
    debug_printf("\n  }\n}]\n");
}

/**
 * @brief Handler for a abort messages
 */
static void MyAbortHandler(
    BACNET_ADDRESS *src, uint8_t invoke_id, uint8_t abort_reason, bool server)
{
    (void)server;
    if (address_match(&Target_Address, src) &&
        (invoke_id == Request_Invoke_ID)) {
        MyPrintHandler(APDU_Hex_ASCII, ERROR_CLASS_SERVICES,
            abort_convert_to_error_code(abort_reason));
        Error_Detected = true;
    }
}

/**
 * @brief Handler for reject messages
 */
static void MyRejectHandler(
    BACNET_ADDRESS *src, uint8_t invoke_id, uint8_t reject_reason)
{
    if (address_match(&Target_Address, src) &&
        (invoke_id == Request_Invoke_ID)) {
        MyPrintHandler(APDU_Hex_ASCII, ERROR_CLASS_SERVICES,
            reject_convert_to_error_code(reject_reason));
        Error_Detected = true;
    }
}

/**
 * @brief Initilize the BACnet application service handlers
 */
static void init_service_handlers(void)
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
    /* handle any errors coming back */
    apdu_set_abort_handler(MyAbortHandler);
    apdu_set_reject_handler(MyRejectHandler);
}

/**
 * @brief Print the usage message
 */
static void print_usage(char *filename)
{
    printf("Usage: %s", filename);
    printf(" <device-instance> <hex-ASCII>\n");
    printf("       [--repeat][--retry][--timeout][--delay]\n");
    printf("       [--dnet][--dadr][--mac]\n");
    printf("       [--version][--help]\n");
}

/**
 * @brief Print the help message
 */
static void print_help(char *filename)
{
    printf("Send an arbitrary BACnet APDU to a device.\n");
    printf("\n");
    printf("device-instance:\n"
           "BACnet Device Object Instance number that you are trying\n"
           "to send the NDPU. The value should be in\n"
           "the range of 0 to 4194303.\n");
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
           "Default is retry 1, only sending one time.\n");
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
    printf("Send an APDU to DNET 123:\n"
           "%s 1 --dnet 123 0123456789ABCDEF\n",
        filename);
    printf("Send an APDU to MAC 10.0.0.1 DNET 123 DADR 05h:\n"
           "%s 1 --mac 10.0.0.1 --dnet 123 --dadr 05 0123456789ABCDEF\n",
        filename);
    printf("Send APDU to MAC 10.1.2.3:47808:\n"
           "%s 1 --mac 10.1.2.3:47808  0123456789ABCDEF\n",
        filename);
    printf("Send an APDU to Device 1:\n"
           "%s 1 0123456789ABCDEF\n",
        filename);
}

/**
 * @brief Send an APDU to a remote network for a specific device
 * @param target_address [in] BACnet address of target router
 * @param buffer [in] The buffer to store the binary data.
 * @param buffer_len [in] The size of the buffer.
 */
void Send_APDU_To_Network(
    BACNET_ADDRESS *target_address, uint8_t *buffer, size_t buffer_len)
{
    int pdu_len = 0;
    int bytes_sent = 0;
    BACNET_NPDU_DATA npdu_data;
    BACNET_ADDRESS my_address;

    datalink_get_my_address(&my_address);
    /* encode the NPDU portion of the packet */
    npdu_encode_npdu_data(&npdu_data, false, MESSAGE_PRIORITY_NORMAL);
    pdu_len = npdu_encode_pdu(
        &Handler_Transmit_Buffer[0], target_address, &my_address, &npdu_data);
    /* encode the APDU portion of the packet */
    memcpy(&Handler_Transmit_Buffer[pdu_len], buffer, buffer_len);
    pdu_len += buffer_len;
    bytes_sent = datalink_send_pdu(
        target_address, &npdu_data, &Handler_Transmit_Buffer[0], pdu_len);
    if (bytes_sent <= 0) {
        fprintf(stderr, "Failed to APDU (%s)!\n", strerror(errno));
    }
}

/**
 * @brief Convert a string of ASCII hex to binary.
 * @param buffer [out] The buffer to store the binary data.
 * @param buffer_size [in] The size of the buffer.
 * @param ascii_hex [in] The string of ASCII hex.
 * @return number of bytes converted, or 0 if error.
 */
static size_t ascii_hex_to_binary(
    uint8_t *buffer, size_t buffer_size, const char *ascii_hex)
{
    unsigned index = 0; /* offset into buffer */
    uint8_t value = 0;
    char hex_pair_string[3] = "";
    size_t length = 0;

    while (ascii_hex[index] != 0) {
        if (!isalnum((int)ascii_hex[index])) {
            /* skip non-numeric or alpha */
            index++;
            continue;
        }
        if (ascii_hex[index + 1] == 0) {
            /* not a hex pair */
            length = 0;
            break;
        }
        hex_pair_string[0] = ascii_hex[index];
        hex_pair_string[1] = ascii_hex[index + 1];
        value = (uint8_t)strtol(hex_pair_string, NULL, 16);
        if (length <= buffer_size) {
            buffer[length] = value;
            length++;
        } else {
            /* too long */
            length = 0;
            break;
        }
        /* set up for next pair */
        index += 2;
    }

    return length;
}

/**
 * @brief Main entry point of the application.
 * @param argc [in] The number of command line arguments.
 * @param argv [in] The array of command line arguments.
 * @return 0 if successful, otherwise a non-zero value.
 */
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
    bool specific_address = false;
    unsigned max_apdu = 0;
    bool found = false;
    int argi = 0;
    unsigned int target_args = 0;
    char *filename = NULL;
    bool repeat_forever = false;
    long retry_count = 1;

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
            printf("Copyright (C) 2024 by Steve Karg and others.\n"
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
        } else if (strcmp(argv[argi], "--repeat") == 0) {
            repeat_forever = true;
        } else if (strcmp(argv[argi], "--retry") == 0) {
            if (++argi < argc) {
                retry_count = strtol(argv[argi], NULL, 0);
                if (retry_count < 1) {
                    retry_count = 1;
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
                Target_Device_Object_Instance = strtol(argv[argi], NULL, 0);
                target_args++;
            } else if (target_args == 1) {
                APDU_Hex_ASCII = argv[argi];
                APDU_Buf_Len = ascii_hex_to_binary(
                    APDU_Buf, sizeof(APDU_Buf), APDU_Hex_ASCII);
                if (APDU_Buf_Len > 0) {
                    target_args++;
                } else {
                    debug_printf("Invalid hex ascii conversion!\n");
                    return 1;
                }
            } else if (target_args == 2) {
                print_usage(filename);
                return 1;
            }
        }
    }
    if (Target_Device_Object_Instance > BACNET_MAX_INSTANCE) {
        print_usage(filename);
        return 1;
    }
    /* setup my info */
    address_init();
    if (specific_address) {
        bacnet_address_init(&dest, &mac, dnet, &adr);
        address_add(Target_Device_Object_Instance, MAX_APDU, &dest);
    }
    /* setup my info */
    Device_Set_Object_Instance_Number(BACNET_MAX_INSTANCE);
    init_service_handlers();
    /* setup the datalink */
    dlenv_init();
    atexit(datalink_cleanup);
    /* setup the timers */
    if (timeout_milliseconds == 0) {
        timeout_milliseconds = apdu_timeout() * apdu_retries();
    }
    mstimer_set(&apdu_timer, timeout_milliseconds);
    mstimer_expire(&apdu_timer);
    mstimer_set(&datalink_timer, 1000);
    /* try to bind with the device */
    if (BACnet_Debug_Enabled) {
        debug_printf(
            "Binding with Device %u...\n", Target_Device_Object_Instance);
    }
    found = address_bind_request(
        Target_Device_Object_Instance, &max_apdu, &Target_Address);
    if (found) {
        if (BACnet_Debug_Enabled) {
            debug_printf("Found Device %u in address_cache.\n",
                Target_Device_Object_Instance);
        }
    } else {
        if (BACnet_Debug_Enabled) {
            debug_printf(
                "Sending Device %u Who-Is.\n", Target_Device_Object_Instance);
        }
        Send_WhoIs(
            Target_Device_Object_Instance, Target_Device_Object_Instance);
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
        if (found) {
            /* device is bound! */
            if (BACnet_Debug_Enabled) {
                printf("Sending APDU to Device %u.\n",
                    Target_Device_Object_Instance);
            }
            if (mstimer_expired(&apdu_timer)) {
                if (repeat_forever || retry_count) {
                    Send_APDU_To_Network(&dest, APDU_Buf, APDU_Buf_Len);
                    retry_count--;
                } else {
                    break;
                }
                mstimer_reset(&apdu_timer);
            }
        } else {
            found = address_bind_request(
                Target_Device_Object_Instance, &max_apdu, &Target_Address);
        }
    }

    return 0;
}
