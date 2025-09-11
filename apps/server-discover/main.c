/**
 * @file
 * @author Steve Karg
 * @date 2022
 * @brief Application to acquire devices and their object list from a BACnet
 * network.
 *
 * SPDX-License-Identifier: MIT
 */
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
/* BACnet Stack defines - first */
#include "bacnet/bacdef.h"
/* BACnet Stack API */
#include "bacnet/apdu.h"
#include "bacnet/bacdcode.h"
#include "bacnet/bacerror.h"
#include "bacnet/bacstr.h"
#include "bacnet/bactext.h"
#include "bacnet/dcc.h"
#include "bacnet/iam.h"
#include "bacnet/npdu.h"
#include "bacnet/version.h"
#include "bacnet/whois.h"
/* basic services */
#include "bacnet/basic/binding/address.h"
#include "bacnet/basic/client/bac-discover.h"
#include "bacnet/basic/object/device.h"
#include "bacnet/basic/sys/debug.h"
#include "bacnet/basic/sys/filename.h"
#include "bacnet/basic/sys/mstimer.h"
#include "bacnet/basic/services.h"
#include "bacnet/basic/tsm/tsm.h"
#include "bacnet/datalink/datalink.h"
#include "bacnet/datalink/dlenv.h"

#if BACNET_SVC_SERVER
#error "App requires server-only features disabled! Set BACNET_SVC_SERVER=0"
#endif

/* current version of the BACnet stack */
static const char *BACnet_Version = BACNET_VERSION_TEXT;
/** Buffer used for receiving */
static uint8_t Rx_Buf[MAX_MPDU];
/* task timer for various BACnet timeouts */
static struct mstimer BACnet_Task_Timer;
/* task timer for TSM timeouts */
static struct mstimer BACnet_TSM_Timer;
/* task timer for printing data */
static struct mstimer BACnet_Print_Timer;
/* flag to determine if devices or both devices and objects are printed */
static bool Print_Summary = false;

/**
 * @brief Print the list of discovered devices and their objects
 */
static void print_discovered_devices(void)
{
    unsigned int device_index = 0;
    unsigned int device_count = 0;
    unsigned int object_count = 0;
    unsigned int object_index = 0;
    unsigned int property_count = 0;
    uint32_t device_id = 0;
    BACNET_OBJECT_ID object_id = { 0 };
    unsigned long milliseconds = 0;
    size_t heap_ram = 0;
    char model_name[MAX_CHARACTER_STRING_BYTES] = { 0 };
    char object_name[MAX_CHARACTER_STRING_BYTES] = { 0 };

    device_count = bacnet_discover_device_count();
    printf("----list of %u devices ----\n", device_count);
    for (device_index = 0; device_index < device_count; device_index++) {
        device_id = bacnet_discover_device_instance(device_index);
        object_count = bacnet_discover_device_object_count(device_id);
        milliseconds = bacnet_discover_device_elapsed_milliseconds(device_id);
        heap_ram = bacnet_discover_device_memory(device_id);
        /* convert to KB next highest value */
        bacnet_discover_property_name(
            device_id, OBJECT_DEVICE, device_id, PROP_MODEL_NAME, model_name,
            sizeof(model_name), "");
        printf(
            "device[%u] %7u \"%s\" object_list[%d] in %lums using %lu bytes\n",
            device_index, device_id, model_name, object_count, milliseconds,
            (unsigned long)heap_ram);
        if (Print_Summary) {
            continue;
        }
        for (object_index = 0; object_index < object_count; object_index++) {
            if (bacnet_discover_device_object_identifier(
                    device_id, object_index, &object_id)) {
                property_count = bacnet_discover_object_property_count(
                    device_id, object_id.type, object_id.instance);
                bacnet_discover_property_name(
                    device_id, object_id.type, object_id.instance,
                    PROP_OBJECT_NAME, object_name, sizeof(object_name), "");
                printf(
                    "    object_list[%d] %s %u \"%s\" has %u properties\n",
                    object_index, bactext_object_type_name(object_id.type),
                    object_id.instance, object_name, property_count);
            }
        }
    }
}

/**
 * @brief Non-blocking task for running BACnet server tasks
 */
static void bacnet_server_task(void)
{
    static bool initialized = false;
    BACNET_ADDRESS src = { 0 }; /* address where message came from */
    uint16_t pdu_len = 0;
    const unsigned timeout_ms = 5;

    if (!initialized) {
        initialized = true;
        /* broadcast an I-Am on startup */
        Send_I_Am(&Handler_Transmit_Buffer[0]);
    }
    /* input */
    /* returns 0 bytes on timeout */
    pdu_len = datalink_receive(&src, &Rx_Buf[0], MAX_MPDU, timeout_ms);
    /* process */
    if (pdu_len) {
        npdu_handler(&src, &Rx_Buf[0], pdu_len);
    }
    /* 1 second tasks */
    if (mstimer_expired(&BACnet_Task_Timer)) {
        mstimer_reset(&BACnet_Task_Timer);
        dcc_timer_seconds(1);
        datalink_maintenance_timer(1);
        dlenv_maintenance_timer(1);
    }
    if (mstimer_expired(&BACnet_TSM_Timer)) {
        mstimer_reset(&BACnet_TSM_Timer);
        tsm_timer_milliseconds(mstimer_interval(&BACnet_TSM_Timer));
    }
}

/**
 * @brief Initialize the handlers for this server device
 */
static void bacnet_server_init(void)
{
    Device_Init(NULL);
    /* we need to handle who-is to support dynamic device binding */
    apdu_set_unconfirmed_handler(SERVICE_UNCONFIRMED_WHO_IS, handler_who_is);
    /* we need to handle who-has to support dynamic object binding */
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
    /* handle communication so we can shutup when asked */
    apdu_set_confirmed_handler(
        SERVICE_CONFIRMED_DEVICE_COMMUNICATION_CONTROL,
        handler_device_communication_control);
    mstimer_set(&BACnet_Task_Timer, 1000);
    mstimer_set(&BACnet_TSM_Timer, 50);
}

/**
 * @brief Print the usage information for this application
 */
static void print_usage(const char *filename)
{
    printf("Usage: %s [--dnet][--dadr][--mac]\n", filename);
    printf("       [--discover-seconds][--print-seconds][--print-summary]\n");
    printf("       [--version][--help]\n");
}

/**
 * @brief Print the help information for this application
 */
static void print_help(const char *filename)
{
    printf("Simulate a BACnet server-discovery device.\n");
    printf("--discover-seconds:\n"
           "Number of seconds to wait before initiating the next discovery.\n");
    printf("--print-seconds:\n"
           "Number of seconds to wait before printing list of devices.\n");
    printf("--print-summary:\n"
           "Print only the list of devices.\n");
    printf("\n");
    printf("--dnet N\n"
           "Optional BACnet network number N for directed requests.\n"
           "Valid range is from 0 to 65535 where 0 is the local connection\n"
           "and 65535 is network broadcast.\n");
    printf("\n");
    printf("--mac A\n"
           "Optional BACnet mac address."
           "Valid ranges are from 00 to FF (hex) for MS/TP or ARCNET,\n"
           "or an IP string with optional port number like 10.1.2.3:47808\n"
           "or an Ethernet MAC in hex like 00:21:70:7e:32:bb\n");
    printf("\n");
    printf("--dadr A\n"
           "Optional BACnet mac address on the destination BACnet network "
           "number.\n"
           "Valid ranges are from 00 to FF (hex) for MS/TP or ARCNET,\n"
           "or an IP string with optional port number like 10.1.2.3:47808\n"
           "or an Ethernet MAC in hex like 00:21:70:7e:32:bb\n");
    (void)filename;
}

/**
 * @brief Main function of server-client demo.
 * @param argc [in] Arg count.
 * @param argv [in] Takes one argument: the Device Instance #.
 * @return 0 on success.
 */
int main(int argc, char *argv[])
{
    int argi = 0;
    unsigned int target_args = 0;
    const char *filename = NULL;
    uint32_t device_id = BACNET_MAX_INSTANCE;
    long long_value = -1;
    /* data from the command line */
    unsigned long print_seconds = 60;
    unsigned long discover_seconds = 60;
    uint16_t dnet = BACNET_BROADCAST_NETWORK;
    BACNET_MAC_ADDRESS mac = { 0 };
    BACNET_MAC_ADDRESS adr = { 0 };
    BACNET_ADDRESS dest = { 0 };
    bool specific_address = false;

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
        if (strcmp(argv[argi], "--discover-seconds") == 0) {
            if (++argi < argc) {
                discover_seconds = strtol(argv[argi], NULL, 0);
            }
        } else if (strcmp(argv[argi], "--print-seconds") == 0) {
            if (++argi < argc) {
                print_seconds = strtol(argv[argi], NULL, 0);
            }
        } else if (strcmp(argv[argi], "--print-summary") == 0) {
            Print_Summary = true;
        } else if (strcmp(argv[argi], "--dnet") == 0) {
            if (++argi < argc) {
                long_value = strtol(argv[argi], NULL, 0);
                if ((long_value >= 0) &&
                    (long_value <= BACNET_BROADCAST_NETWORK)) {
                    dnet = (uint16_t)long_value;
                    specific_address = true;
                }
            }
        } else if (strcmp(argv[argi], "--mac") == 0) {
            if (++argi < argc) {
                if (bacnet_address_mac_from_ascii(&mac, argv[argi])) {
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
                device_id = strtol(argv[argi], NULL, 0);
                target_args++;
            }
        }
    }
    if (device_id > BACNET_MAX_INSTANCE) {
        debug_fprintf(
            stderr, "device-instance=%u - not greater than %u\n", device_id,
            BACNET_MAX_INSTANCE);
        return 1;
    }
    if (specific_address) {
        bacnet_address_init(&dest, &mac, dnet, &adr);
    }
    Device_Set_Object_Instance_Number(device_id);
    debug_printf_stdout(
        "BACnet Server-Discovery Demo\n"
        "BACnet Stack Version %s\n"
        "BACnet Device ID: %u\n"
        "DNET: %u every %lu seconds\n"
        "Print Devices: every %lu seconds (0=none)\n"
        "Max APDU: %d\n",
        BACnet_Version, Device_Object_Instance_Number(), dest.net,
        discover_seconds, print_seconds, MAX_APDU);
    dlenv_init();
    atexit(datalink_cleanup);
    bacnet_server_init();
    /* configure the discovery module */
    bacnet_discover_dest_set(&dest);
    bacnet_discover_seconds_set(discover_seconds);
    bacnet_discover_init();
    atexit(bacnet_discover_cleanup);
    mstimer_set(&BACnet_Print_Timer, print_seconds * 1000UL);
    /* loop forever */
    for (;;) {
        bacnet_server_task();
        bacnet_discover_task();
        if (mstimer_expired(&BACnet_Print_Timer)) {
            mstimer_reset(&BACnet_Print_Timer);
            print_discovered_devices();
        }
    }

    return 0;
}
