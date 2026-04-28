/**
 * @file main.c
 * @brief Generic Modbus RTU <-> BACnet Gateway
 *
 * Overview:
 *  1. Load gateway_config.json  (BACnet settings, Modbus settings, point table)
 *  2. Initialize BACnet datalink (MS/TP, BACnet/IP, Ethernet, etc.)
 *  3. Create BACnet objects defined in the point table (AI / AO / BI / BO)
 *  4. Initialize Modbus RTU master
 *  5. Main loop
 *     a) Receive and dispatch BACnet frames
 *     b) Run 1-second BACnet timers
 *     c) Poll Modbus every poll_interval_sec seconds
 *
 * Usage:
 *   sudo ./modbusgw [config.json] [device_instance_override]
 *
 * Default config file: gateway_config.json (in the working directory)
 *
 * The BACnet datalink type is selected by the "datalink_type" field in the
 * "bacnet" section of the JSON config file (e.g. "mstp", "bip", "bip6",
 * "ethernet").  All datalink-specific parameters are also read from JSON.
 * Alternatively, the standard BACNET_* environment variables can be used to
 * override any setting before the process starts.
 *
 * @author Jihye Ahn <jen.ahn@lge.com>
 *
 * @copyright Copyright (c) 2026 LG Electronics Inc.
 * SPDX-License-Identifier: GPL-2.0-or-later WITH GCC-exception-2.0
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 */

/* ── Standard libraries ── */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <time.h>
#include <stdint.h>
#include <stdbool.h>
#include <unistd.h>

/* ── BACnet stack ── */
#include "bacnet/bacdef.h"
#include "bacnet/bacdcode.h"
#include "bacnet/bacstr.h"
#include "bacnet/npdu.h"
#include "bacnet/apdu.h"
#include "bacnet/iam.h"
#include "bacnet/dcc.h"
#include "bacnet/version.h"
#include "bacnet/bacenum.h"
#include "bacnet/basic/binding/address.h"
#include "bacnet/basic/tsm/tsm.h"
#include "bacnet/basic/services.h"
#include "bacnet/basic/sys/debug.h"
#include "bacnet/datalink/datalink.h"
#include "bacnet/datalink/dlenv.h"
#include "bacnet/basic/object/device.h"

/* ── This project ── */
#include "gateway_config.h"
#include "modbus_rtu.h"
#include "point_table.h"

/* ────────────────────────────────────────────────────────────────────────
 * Helper macros
 * ──────────────────────────────────────────────────────────────────────*/
#define STRINGIFY_IMPL(x) #x
#define STRINGIFY(x) STRINGIFY_IMPL(x)

/* ────────────────────────────────────────────────────────────────────────
 * Globals
 * ──────────────────────────────────────────────────────────────────────*/

/** BACnet receive buffer */
static uint8_t Rx_Buf[MAX_MPDU] = { 0 };

/** Program running flag – cleared by SIGINT / SIGTERM */
static volatile bool Running = true;

/** Gateway configuration (loaded from JSON) */
static GW_CONFIG Gw_Config;

/** Point table (loaded from JSON) */
static GW_POINT_TABLE Point_Table;

/* ────────────────────────────────────────────────────────────────────────
 * Signal handler
 * ──────────────────────────────────────────────────────────────────────*/
static void signal_handler(int sig)
{
    (void)sig;
    Running = false;
}

/* ────────────────────────────────────────────────────────────────────────
 * Register BACnet service handlers
 * ──────────────────────────────────────────────────────────────────────*/
static void Init_Service_Handlers(void)
{
    Device_Init(NULL);

    apdu_set_unconfirmed_handler(
        SERVICE_UNCONFIRMED_WHO_IS, handler_who_is_unicast);
    apdu_set_unconfirmed_handler(SERVICE_UNCONFIRMED_WHO_HAS, handler_who_has);
    apdu_set_unrecognized_service_handler_handler(handler_unrecognized_service);
    apdu_set_confirmed_handler(
        SERVICE_CONFIRMED_READ_PROPERTY, handler_read_property);
    apdu_set_confirmed_handler(
        SERVICE_CONFIRMED_READ_PROP_MULTIPLE, handler_read_property_multiple);
    apdu_set_confirmed_handler(
        SERVICE_CONFIRMED_WRITE_PROPERTY, handler_write_property);
    apdu_set_unconfirmed_handler(
        SERVICE_UNCONFIRMED_UTC_TIME_SYNCHRONIZATION, handler_timesync_utc);
    apdu_set_unconfirmed_handler(
        SERVICE_UNCONFIRMED_TIME_SYNCHRONIZATION, handler_timesync);
    apdu_set_confirmed_handler(
        SERVICE_CONFIRMED_SUBSCRIBE_COV, handler_cov_subscribe);
    apdu_set_confirmed_handler(
        SERVICE_CONFIRMED_DEVICE_COMMUNICATION_CONTROL,
        handler_device_communication_control);
    apdu_set_confirmed_handler(
        SERVICE_CONFIRMED_REINITIALIZE_DEVICE, handler_reinitialize_device);
}

/* ────────────────────────────────────────────────────────────────────────
 * main
 * ──────────────────────────────────────────────────────────────────────*/
int main(int argc, char *argv[])
{
    const char *config_file = "gateway_config.json";
    uint32_t override_inst = 0;
    BACNET_ADDRESS src = { 0 };
    uint16_t pdu_len = 0;
    time_t current_sec = 0;
    time_t last_sec = 0;
    time_t last_poll_sec = 0;
    uint32_t elapsed_sec = 0;
    char baud_str[16] = { 0 };
    char mac_str[8] = { 0 };
    char max_master_str[16] = { 0 };
    char net_str[16] = { 0 };
    char port_str[16] = { 0 };
    int i;

    /* ── Parse CLI arguments ──
     *   argv[1] : optional path to JSON config file
     *   argv[2] : optional Device Instance override (integer)
     */
    for (i = 1; i < argc; i++) {
        char *end = NULL;
        uint32_t v = (uint32_t)strtoul(argv[i], &end, 0);
        if (end != argv[i] && *end == '\0' && v > 0 &&
            v <= BACNET_MAX_INSTANCE) {
            /* pure number → device instance override */
            override_inst = v;
        } else {
            /* string → config file path */
            config_file = argv[i];
        }
    }

    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    /* Set stdout/stderr to unbuffered so log output appears immediately */
#if defined(__linux__) || defined(__unix__) || defined(__APPLE__)
    setbuf(stdout, NULL);
    setbuf(stderr, NULL);
#endif

    /* ── Load configuration and point table from JSON ── */
    if (!point_table_load_json(&Gw_Config, &Point_Table, config_file)) {
        fprintf(
            stderr, "[GW] Failed to load '%s'. Check the file.\n", config_file);
        return 1;
    }

    /* Apply device instance override from CLI */
    if (override_inst) {
        Gw_Config.device_instance = override_inst;
    }

    printf("===========================================\n");
    printf(" Generic Modbus RTU <-> BACnet Gateway\n");
    printf(" BACnet Stack v%s\n", BACNET_VERSION_TEXT);
    printf(" Config          : %s\n", config_file);
    printf(
        " Device          : %s [%u]\n", Gw_Config.device_name,
        Gw_Config.device_instance);
    printf(" Datalink        : %s\n", Gw_Config.datalink_type);
    printf(" BACnet Iface    : %s\n", Gw_Config.bacnet_iface);
    printf(
        " Modbus Port     : %s @ %u bps\n", Gw_Config.modbus_port,
        Gw_Config.modbus_baud);
    printf(" Poll Interval   : %d sec\n", Gw_Config.poll_interval_sec);
    printf(" Points          : %d\n", Point_Table.count);
    printf("===========================================\n");

    /* ── BACnet service handlers (Device_Init is called inside) ── */
    Init_Service_Handlers();

    /* ── BACnet Device configuration (must be after Device_Init) ── */
    Device_Set_Object_Instance_Number(Gw_Config.device_instance);
    {
        BACNET_CHARACTER_STRING name_str;
        characterstring_init_ansi(&name_str, Gw_Config.device_name);
        Device_Set_Object_Name(&name_str);
    }
    Device_Set_System_Status(STATUS_OPERATIONAL, true);
    Device_Set_Vendor_Identifier(Gw_Config.vendor_id);

    /* ── Configure BACnet datalink via environment variables ──
     *
     * The datalink type is read from JSON ("datalink_type" field).
     * Supported values: "mstp", "bip", "bip6", "ethernet"
     * All standard BACNET_* env vars can also be pre-set externally to
     * override any of these settings before the process starts.
     */
#if defined(__linux__) || defined(__unix__) || defined(__APPLE__)
    setenv("BACNET_DATALINK", Gw_Config.datalink_type, 0);
    setenv("BACNET_IFACE", Gw_Config.bacnet_iface, 0);

    if (strcmp(Gw_Config.datalink_type, "mstp") == 0) {
        snprintf(baud_str, sizeof(baud_str), "%u", Gw_Config.mstp_baud);
        snprintf(mac_str, sizeof(mac_str), "%u", Gw_Config.mstp_mac);
        snprintf(
            max_master_str, sizeof(max_master_str), "%u",
            Gw_Config.mstp_max_master);
        snprintf(net_str, sizeof(net_str), "%u", Gw_Config.mstp_net);
        setenv("BACNET_MSTP_BAUD", baud_str, 1);
        setenv("BACNET_MSTP_MAC", mac_str, 1);
        setenv("BACNET_MAX_MASTER", max_master_str, 1);
        setenv("BACNET_MSTP_NET", net_str, 1);
    } else if (
        strcmp(Gw_Config.datalink_type, "bip") == 0 ||
        strcmp(Gw_Config.datalink_type, "bip6") == 0) {
        if (Gw_Config.bip_port != 0) {
            snprintf(port_str, sizeof(port_str), "%u", Gw_Config.bip_port);
            setenv("BACNET_IP_PORT", port_str, 1);
        }
    }
#endif /* __linux__ || __unix__ || __APPLE__ */
    dlenv_init();

    if (!datalink_init(NULL)) {
        fprintf(
            stderr, "[GW] datalink_init() failed (datalink=%s, iface=%s)\n",
            Gw_Config.datalink_type, Gw_Config.bacnet_iface);
        return 1;
    }
    atexit(datalink_cleanup);

    /* ── Create BACnet objects from point table ── */
    point_table_init_bacnet_objects(&Point_Table);

    /* ── Initialize Modbus RTU ── */
    if (!Modbus_RTU_Init(Gw_Config.modbus_port, Gw_Config.modbus_baud, 1)) {
        fprintf(
            stderr, "[GW] Modbus init failed on %s – will retry on next poll\n",
            Gw_Config.modbus_port);
    }
    atexit(Modbus_RTU_Cleanup);

    /* ── Broadcast I-Am ── */
    Send_I_Am(&Handler_Transmit_Buffer[0]);

    last_sec = last_poll_sec = time(NULL);

    /* ── Initial poll immediately on startup ── */
    point_table_poll_modbus(&Point_Table);
    fflush(stdout);

    /* ════════════════════════════════════════
     * Main loop
     * ════════════════════════════════════════*/
    while (Running) {
        current_sec = time(NULL);

        /* ── Receive BACnet frames ── */
        pdu_len = datalink_receive(&src, &Rx_Buf[0], MAX_MPDU, 10 /*ms*/);
        if (pdu_len > 0) {
            npdu_handler(&src, &Rx_Buf[0], pdu_len);
        }

        /* ── 1-second BACnet timer tasks ── */
        elapsed_sec = (uint32_t)(current_sec - last_sec);
        if (elapsed_sec > 0) {
            last_sec = current_sec;
            dcc_timer_seconds(elapsed_sec);
            datalink_maintenance_timer(elapsed_sec);
            dlenv_maintenance_timer(elapsed_sec);
            tsm_timer_milliseconds(elapsed_sec * 1000);
            Device_Timer(elapsed_sec * 1000);
        }

        /* ── COV task ── */
        handler_cov_task();

        /* ── Modbus polling ── */
        if ((current_sec - last_poll_sec) >= Gw_Config.poll_interval_sec) {
            last_poll_sec = current_sec;
            point_table_poll_modbus(&Point_Table);
        }
    }

    printf("\n[GW] Shutting down...\n");
    return 0;
}
