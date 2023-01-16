/**
 * @file
 * @brief Samble BACNet/SC hub.
 * @author Mikhail Antropov
 * @date December 2022
 * @section LICENSE
 *
 * Copyright (C) 2022 Legrand North America, LLC
 * as an unpublished work.
 *
 * SPDX-License-Identifier: GPL-2.0-or-later WITH GCC-exception-2.0
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

/** Initialize the handlers we will utilize.
 * @see Device_Init, apdu_set_unconfirmed_handler, apdu_set_confirmed_handler
 */
static void Init_Service_Handlers(void)
{
    Device_Init(NULL);
   /* set the handler for all the services we don't implement
       It is required to send the proper reject message... */
    apdu_set_unrecognized_service_handler_handler(handler_unrecognized_service);
    /* we must implement read property - it's required! */
    apdu_set_confirmed_handler(
        SERVICE_CONFIRMED_READ_PROPERTY, handler_read_property);
    /* we need to handle who-is to support dynamic device binding */
    apdu_set_unconfirmed_handler(SERVICE_UNCONFIRMED_WHO_IS, handler_who_is);
    apdu_set_unconfirmed_handler(SERVICE_UNCONFIRMED_WHO_HAS, handler_who_has);
}

static void print_usage(const char *filename)
{
    printf("Usage: %s [device-instance [device-name]]\n", filename);
    printf("       [--version][--help]\n");
}

static void print_help(const char *filename)
{
    printf("Simulate a BACnet/SC HUB device\n"
        "device-instance: BACnet Device Object Instance number that you are\n"
        "trying simulate.\n"
        "device-name: The Device name in ASCII for this device.\n"
        "\n"
        "Other parameters are passing over environment variables:\n"
        "BACNET_SC_DIRECT_CONNECT_PORT: Local port\n"
        "BACNET_SC_ISSUER_1_CERTIFICATE_FILE: Filename of CA certificate\n"
        "BACNET_SC_OPERATIONAL_CERTIFICATE_FILE: Filename of device certificate\n"
        "BACNET_SC_CERTIFICATE_SIGNING_REQUEST_FILE: Filename of device certificate key\n");
    printf("\n");
    printf("Example:\n"
        "To simulate Device 123:\n"
        "%s  123\n",
        filename);
    printf("To simulate Device 123 named Fred,\n"
        "use following command:\n"
        "%s 123 Fred\n",
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
    int argi = 0;
    const char *filename = NULL;

    BACNET_CHARACTER_STRING DeviceName;

    filename = filename_remove_path(argv[0]);
    argi = 0;
    if ((argc >= 2) && (strcmp(argv[argi], "--help") == 0)) {
        print_usage(filename);
        print_help(filename);
        return 0;
    }
    if ((argc >= 2) && (strcmp(argv[argi], "--version") == 0)) {
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
    if (!ctx)
        fprintf(stderr, "Failed to load config file bacnet_dev\n");
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

    printf("BACnet SC Hub Demo\n"
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
    if (!ctx)
        fprintf(stderr, "Failed to load config file bacnet_dev\n");
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
    if (Device_Object_Name(Device_Object_Instance_Number(),&DeviceName)) {
        printf("BACnet Device Name: %s\n", DeviceName.value);
    }
    dlenv_init();
    atexit(datalink_cleanup);
    /* loop forever */
    for (;;) {
        /* input */
        bsc_wait(1);
        datalink_maintenance_timer(1);
    }

    return 0;
}

/* @} */

/* End group SCServerDemo */
