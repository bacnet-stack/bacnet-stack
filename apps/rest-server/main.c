/**
 * @file
 * @brief Sample BACnet/REST server.
 * @author Mikhail Antropov
 * @date August 2023
 * @section LICENSE
 *
 * Copyright (C) 2023 Legrand North America, LLC
 * as an unpublished work.
 *
 * SPDX-License-Identifier: MIT
 */

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <time.h>
#include "bacnet/config.h"
#include "bacnet/bacdef.h"
#include "bacnet/bacdcode.h"
#include "bacnet/apdu.h"
#include "bacnet/dcc.h"
#include "bacnet/iam.h"
#include "bacnet/npdu.h"
#include "bacnet/getevent.h"
#include "bacnet/version.h"
#include "bacnet/basic/services.h"
#include "bacnet/datalink/dlenv.h"
#include "bacnet/basic/sys/filename.h"
#include "bacnet/basic/sys/mstimer.h"
#include "bacnet/basic/tsm/tsm.h"
#include "bacnet/datalink/datalink.h"
#include "bacnet/basic/binding/address.h"
/* include the device object */
#include "bacnet/basic/object/device.h"
#include "bacnet/basic/object/lc.h"
#include "bacnet/basic/object/trendlog.h"
#if defined(BACFILE)
#include "bacnet/basic/object/bacfile.h"
#endif /* defined(BACFILE) */
#if defined(BAC_UCI)
#include "bacnet/basic/ucix/ucix.h"
#endif /* defined(BAC_UCI) */
#include <bacnet/basic/service/ws_restful/ws-service.h>

/** @file server/main.c  Example server application using the BACnet Stack. */

/* (Doxygen note: The next two lines pull all the following Javadoc
 *  into the ServerDemo module.) */
/** @addtogroup ServerDemo */
/*@{*/


#ifndef WS_NETWORK_IFACE
#define WS_NETWORK_IFACE "127.0.0.1"
#endif

#define INFINITE_TIMEOUT 10000000
#define DEFAULT_TIMEOUT 10
#define DEFAULT_WAIT_TIMEOUT_MS 100

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

#if 0
    /*  BACnet Testing Observed Incident oi00107
        Server only devices should not indicate that they EXECUTE I-Am
        Revealed by BACnet Test Client v1.8.16 ( www.bac-test.com/bacnet-test-client-download )
            BITS: BIT00040
        Any discussions can be directed to edward@bac-test.com
        Please feel free to remove this comment when my changes accepted after suitable time for
        review by all interested parties. Say 6 months -> September 2016 */
    /* In this demo, we are the server only ( BACnet "B" device ) so we do not indicate
       that we can execute the I-Am message */
    /* handle i-am to support binding to other devices */
    apdu_set_unconfirmed_handler(SERVICE_UNCONFIRMED_I_AM, handler_i_am_bind);
#endif

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
        SERVICE_CONFIRMED_READ_RANGE, handler_read_range);
#if defined(BACFILE)
    apdu_set_confirmed_handler(
        SERVICE_CONFIRMED_ATOMIC_READ_FILE, handler_atomic_read_file);
    apdu_set_confirmed_handler(
        SERVICE_CONFIRMED_ATOMIC_WRITE_FILE, handler_atomic_write_file);
#endif
    apdu_set_confirmed_handler(
        SERVICE_CONFIRMED_REINITIALIZE_DEVICE, handler_reinitialize_device);
    apdu_set_unconfirmed_handler(
        SERVICE_UNCONFIRMED_UTC_TIME_SYNCHRONIZATION, handler_timesync_utc);
    apdu_set_unconfirmed_handler(
        SERVICE_UNCONFIRMED_TIME_SYNCHRONIZATION, handler_timesync);
    apdu_set_confirmed_handler(
        SERVICE_CONFIRMED_SUBSCRIBE_COV, handler_cov_subscribe);
    apdu_set_unconfirmed_handler(
        SERVICE_UNCONFIRMED_COV_NOTIFICATION, handler_ucov_notification);
    /* handle communication so we can shutup when asked */
    apdu_set_confirmed_handler(SERVICE_CONFIRMED_DEVICE_COMMUNICATION_CONTROL,
        handler_device_communication_control);
    /* handle the data coming back from private requests */
    apdu_set_unconfirmed_handler(SERVICE_UNCONFIRMED_PRIVATE_TRANSFER,
        handler_unconfirmed_private_transfer);
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
    printf("Simulate a BACnet/REST server\n"
        "device-instance: BACnet Device Object Instance number that you are\n"
        "trying simulate.\n"
        "device-name: The Device name in ASCII for this device.\n"
        "\n");
    printf("Other parameters are passing over environment variables:\n"
        "- BACNET_HTTP_PORT: HTTP port number\n"
        "- BACNET_HTTPS_PORT: HTTPS port number\n"
        "- BACNET_CA_CERTIFICATE_FILE: Filename of CA certificate\n"
        "- BACNET_SERVER_CERTIFICATE_FILE: Filename of device certificate\n"
        "- BACNET_SERVER_CERTIFICATE_PRIVATE_KEY_FILE: Filename of device certificate key\n"
        "For additional information see file bin/rest-server.sh\n");
    printf("\nExample:\n"
        "To simulate Device 111, use following command:\n"
        "%s 111\n", filename);
    printf("To simulate Device 111 named NoFred, use following command:\n"
        "%s 111 NoFred\n", filename);
}

/*
 * bsc_node_load_cert_bacfile loads one credentional file from bacfile object
 * Note: the function adds null-terminated byte to loaded file
 * (certificate, certificate key).
 * The MbedTLS PEM parser requires data to be null-terminated.
 */
#ifdef CONFIG_MBEDTLS
#define ZERO_BYTE 1
#else
#define ZERO_BYTE 0
#endif

static bool load_cert(char *env_name, uint8_t **pbuf, size_t *psize)
{
    char *filename;
    uint8_t *buf = NULL;
    FILE *pFile = NULL;
    bool ret = false;

    filename = getenv(env_name);
    if (!filename || !filename[0]) {
        goto end;
    }
 
    pFile = fopen(filename, "rb");
    if (!pFile) {
        goto end;
    }

    fseek(pFile, 0L, SEEK_END);
    *psize = ftell(pFile) + ZERO_BYTE;
    fseek(pFile, 0L, SEEK_SET);

    buf = calloc(1, *psize);
    if (buf == NULL) {
        goto end;
    }

    if (fread(buf, (*psize - ZERO_BYTE), 1, pFile) == 0) {
        goto end;
    }

#ifdef CONFIG_MBEDTLS
    buf[*psize - 1] = 0;
#endif

    ret = true;
    *pbuf = buf;

end:
    if (pFile) {
        fclose(pFile);
    }

    if (!ret) {
        free(buf);
    }

    return ret;
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
    uint16_t http_port = 0;
    uint16_t https_port = 0;
    uint8_t *ca_cert = NULL;
    size_t ca_cert_size = 0;
    uint8_t *cert = NULL;
    size_t cert_size = 0;
    uint8_t *key = NULL;
    size_t key_size = 0;

    BACNET_ADDRESS src = { 0 }; /* address where message came from */
    unsigned timeout = 1; /* milliseconds */
    uint16_t pdu_len = 0;
    BACNET_CHARACTER_STRING DeviceName;
    unsigned elapsed_seconds = 0;
    struct mstimer datalink_timer = { 0 };
#if defined(BACNET_TIME_MASTER)
    BACNET_DATE_TIME bdatetime;
#endif
#if defined(BAC_UCI)
    int uciId = 0;
    struct uci_context *ctx;
#endif
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
        if (argc > 1) {
            Device_Set_Object_Instance_Number(strtol(argv[1], NULL, 0));
        }

#if defined(BAC_UCI)
    }
    ucix_cleanup(ctx);
#endif /* defined(BAC_UCI) */

    printf("BACnet Server Demo\n"
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
        if (argc > 2) {
            Device_Object_Name_ANSI_Init(argv[2]);
        }
#if defined(BAC_UCI)
    }
    ucix_cleanup(ctx);
#endif /* defined(BAC_UCI) */
    if (Device_Object_Name(Device_Object_Instance_Number(), &DeviceName)) {
        printf("BACnet Device Name: %s\n", DeviceName.value);
    }

    /* run ws server */
    if (!load_cert("BACNET_CA_CERTIFICATE_FILE", &ca_cert, &ca_cert_size)) {
        printf("Can not load file by ENV BACNET_CA_CERTIFICATE_FILE\n");
    }
    if (!load_cert("BACNET_SERVER_CERTIFICATE_FILE", &cert, &cert_size)) {
        printf("Can not load file by ENV BACNET_SERVER_CERTIFICATE_FILE\n");
    }
    if (!load_cert("BACNET_SERVER_CERTIFICATE_PRIVATE_KEY_FILE",
                    &key, &key_size)) {
        printf("Can not load file by ENV "
                "BACNET_SERVER_CERTIFICATE_PRIVATE_KEY_FILE\n");
    }
    http_port = atoi(getenv("BACNET_HTTP_PORT"));
    https_port = atoi(getenv("BACNET_HTTPS_PORT"));

    if (ws_server_start(http_port, https_port, WS_NETWORK_IFACE,
                        WS_NETWORK_IFACE, ca_cert, ca_cert_size, cert,
                        cert_size, key, key_size, DEFAULT_TIMEOUT)
        != BACNET_WS_SERVICE_SUCCESS) {
        printf("Can not start REST server\n");
    }
    
    ws_service_info_registry();
    ws_service_auth_registry();
    /* end run server */

    dlenv_init();
    atexit(datalink_cleanup);
    mstimer_set(&datalink_timer, 1000);
    /* broadcast an I-Am on startup */
    Send_I_Am(&Handler_Transmit_Buffer[0]);
    /* loop forever */
    for (;;) {
        /* input */
        pdu_len = datalink_receive(&src, &Rx_Buf[0], MAX_MPDU, timeout);
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

    ws_server_stop();
    free(ca_cert);
    free(cert);
    free(key);

    return 0;
}

/* @} */

/* End group ServerDemo */
