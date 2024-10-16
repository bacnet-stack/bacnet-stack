/**
 * @file
 * @brief Sample BACnet/SC hub.
 * @author Mikhail Antropov
 * @date December 2022
 * @section LICENSE
 *
 * Copyright (C) 2022 Legrand North America, LLC
 * as an unpublished work.
 *
 * SPDX-License-Identifier: MIT
 */
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "bacnet/config.h"
#include "bacnet/apdu.h"
#include "bacnet/iam.h"
#include "bacnet/version.h"
#include "bacnet/basic/services.h"
#include "bacnet/datalink/dlenv.h"
#include "bacnet/basic/sys/filename.h"
#include "bacnet/basic/sys/mstimer.h"
#include "bacnet/datalink/datalink.h"
/* include the device object */
#include "bacnet/basic/object/device.h"
#if defined(BAC_UCI)
#include "bacnet/basic/ucix/ucix.h"
#endif /* defined(BAC_UCI) */
#include "bacnet/datalink/bsc/bsc-datalink.h"
#include "bacnet/datalink/bsc/bsc-event.h"

/* (Doxygen note: The next two lines pull all the following Javadoc
 *  into the ServerDemo module.) */
/** @addtogroup SCServerDemo */
/*@{*/

#ifndef BACDL_BSC
#error "BACDL_BSC must be defined"
#endif

/* current version of the BACnet stack */
static const char *BACnet_Version = BACNET_VERSION_TEXT;

/** Buffer used for receiving */
static uint8_t Rx_Buf[MAX_MPDU] = { 0 };

/** Initialize the handlers we will utilize.
 * @see Device_Init, apdu_set_unconfirmed_handler, apdu_set_confirmed_handler
 */
static void Init_Service_Handlers(void)
{
    Device_Init(NULL);

    /* we need to handle who-is to support dynamic device binding */
    apdu_set_unconfirmed_handler(SERVICE_UNCONFIRMED_WHO_IS, handler_who_is);
    apdu_set_unconfirmed_handler(SERVICE_UNCONFIRMED_WHO_HAS, handler_who_has);
    /* set the handler for all the services we don't implement */
    /* It is required to send the proper reject message... */
    apdu_set_unrecognized_service_handler_handler(handler_unrecognized_service);
    /* Set the handlers for any confirmed services that we support. */
    /* We must implement read property - it's required! */
    apdu_set_confirmed_handler(
        SERVICE_CONFIRMED_READ_PROPERTY, handler_read_property);
    apdu_set_confirmed_handler(
        SERVICE_CONFIRMED_READ_PROP_MULTIPLE, handler_read_property_multiple);
    apdu_set_confirmed_handler(
        SERVICE_CONFIRMED_WRITE_PROPERTY, handler_write_property);
    apdu_set_confirmed_handler(
        SERVICE_CONFIRMED_WRITE_PROP_MULTIPLE, handler_write_property_multiple);
    apdu_set_confirmed_handler(
        SERVICE_CONFIRMED_REINITIALIZE_DEVICE, handler_reinitialize_device);
    apdu_set_unconfirmed_handler(
        SERVICE_UNCONFIRMED_UTC_TIME_SYNCHRONIZATION, handler_timesync_utc);
    apdu_set_unconfirmed_handler(
        SERVICE_UNCONFIRMED_TIME_SYNCHRONIZATION, handler_timesync);
#if defined(BACNET_TIME_MASTER)
    handler_timesync_init();
#endif
}

static void print_usage(const char *filename)
{
    printf("Usage: %s [device-instance [device-name]]\n", filename);
    printf("       [--version][--help]\n");
}

static void print_help(const char *filename)
{
    printf(
        "Simulate a BACnet/SC HUB device\n"
        "device-instance: BACnet Device Object Instance number that you are\n"
        "trying simulate.\n"
        "device-name: The Device name in ASCII for this device.\n"
        "\n");
    printf("Other parameters are passing over environment variables:\n"
           "- BACNET_SC_ISSUER_1_CERTIFICATE_FILE: Filename of CA certificate\n"
           "- BACNET_SC_OPERATIONAL_CERTIFICATE_FILE: Filename of device "
           "certificate\n"
           "- BACNET_SC_OPERATIONAL_CERTIFICATE_PRIVATE_KEY_FILE: Filename of "
           "device certificate key\n"
           "- BACNET_SC_HUB_FUNCTION_BINDING: Local port or pair \"interface "
           "name:port number\"\n"
           "For additional information see file bin/bsc-server.sh\n");
    printf(
        "\nExample:\n"
        "To simulate Device 111, use following command:\n"
        "%s 111\n",
        filename);
    printf(
        "To simulate Device 111 named NoFred, use following command:\n"
        "%s 111 NoFred\n",
        filename);
}

/** Main function of server demo.
 *
 * @see Device_Set_Object_Instance_Number, dlenv_init, Send_I_Am,
 *      datalink_receive, npdu_handler,
 *      dcc_timer_seconds, datalink_maintenance_timer,
 *      Load_Control_State_Machine_Handler, handler_cov_task,
 *      tsm_timer_milliseconds
 *
 * @param argc [in] Arg count.
 * @param argv [in] Takes one argument: the Device Instance #.
 * @return 0 on success.
 */
int main(int argc, char *argv[])
{
#if defined(BAC_UCI)
    int uciId = 0;
    struct uci_context *ctx;
#endif
#if defined(BACNET_TIME_MASTER)
    BACNET_DATE_TIME bdatetime = { 0 };
#endif
    BACNET_ADDRESS src = { 0 }; /* address where message came from */
    int argi = 0;
    const char *filename = NULL;

    BACNET_CHARACTER_STRING DeviceName;
    uint16_t pdu_len = 0;
    unsigned delay_milliseconds = 1;
    unsigned elapsed_seconds = 0;
    struct mstimer datalink_timer = { 0 };

    filename = filename_remove_path(argv[0]);
    argi = 0;
    if ((argc >= 2) && (strcmp(argv[1], "--help") == 0)) {
        print_usage(filename);
        print_help(filename);
        return 0;
    }
    if ((argc >= 2) && (strcmp(argv[1], "--version") == 0)) {
        printf("%s %s\n", filename, BACNET_VERSION_TEXT);
        printf("Copyright (C) 2022 by Steve Karg and others.\n"
               "This is free software; see the source for copying "
               "conditions.\n"
               "There is NO warranty; not even for MERCHANTABILITY or\n"
               "FITNESS FOR A PARTICULAR PURPOSE.\n");
        return 0;
    }

#if defined(BAC_UCI)
    ctx = ucix_init("bacnet_dev");
    if (!ctx) {
        fprintf(stderr, "Failed to load config file bacnet_dev\n");
    }
    uciId = ucix_get_option_int(ctx, "bacnet_dev", "0", "Id", 0);
    if (uciId != 0) {
        Device_Set_Object_Instance_Number(uciId);
    } else {
#endif /* defined(BAC_UCI) */
        /* allow the device ID to be set */
        if (++argi < argc) {
            Device_Set_Object_Instance_Number(strtol(argv[argi], NULL, 0));
        }

#if defined(BAC_UCI)
    }
    ucix_cleanup(ctx);
#endif /* defined(BAC_UCI) */

    printf(
        "BACnet SC Hub Demo\n"
        "BACnet Stack Version %s\n"
        "BACnet Device ID: %u\n"
        "Max APDU: %d\n",
        BACnet_Version, Device_Object_Instance_Number(), MAX_APDU);
    /* load any static address bindings to show up
       in our device bindings list */
    address_init();
    Init_Service_Handlers();
#if defined(BAC_UCI)
    const char *uciname;
    ctx = ucix_init("bacnet_dev");
    if (!ctx) {
        fprintf(stderr, "Failed to load config file bacnet_dev\n");
    }
    uciname = ucix_get_option(ctx, "bacnet_dev", "0", "Name");
    if (uciname != 0) {
        Device_Object_Name_ANSI_Init(uciname);
    } else {
#endif /* defined(BAC_UCI) */
        if (++argi < argc) {
            Device_Object_Name_ANSI_Init(argv[argi]);
        }
#if defined(BAC_UCI)
    }
    ucix_cleanup(ctx);
#endif /* defined(BAC_UCI) */
    if (Device_Object_Name(Device_Object_Instance_Number(), &DeviceName)) {
        printf("BACnet Device Name: %s\n", DeviceName.value);
    }
    dlenv_init();
    atexit(datalink_cleanup);
    mstimer_set(&datalink_timer, 1000);
    /* loop forever */
    for (;;) {
        /* input */
        pdu_len =
            datalink_receive(&src, &Rx_Buf[0], MAX_MPDU, delay_milliseconds);
        /* process */
        if (pdu_len) {
            npdu_handler(&src, &Rx_Buf[0], pdu_len);
        }
        if (mstimer_expired(&datalink_timer)) {
            elapsed_seconds = mstimer_interval(&datalink_timer) / 1000;
            mstimer_reset(&datalink_timer);
            datalink_maintenance_timer(elapsed_seconds);
#if defined(BACNET_TIME_MASTER)
            Device_getCurrentDateTime(&bdatetime);
            handler_timesync_task(&bdatetime);
#endif
        }
    }

    return 0;
}

/* @} */

/* End group SCServerDemo */
