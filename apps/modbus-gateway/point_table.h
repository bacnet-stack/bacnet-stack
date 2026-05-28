/**
 * @file point_table.h
 * @brief Generic Modbus-to-BACnet point mapping table.
 *
 * Each entry maps one Modbus register/coil to one BACnet object (AI/AO/BI/BO).
 * The table is loaded at startup from gateway_config.json.
 *
 * @author Jihye Ahn <jen.ahn@lge.com>
 *
 * @copyright Copyright (c) 2026 LG Electronics Inc.
 * SPDX-License-Identifier: GPL-2.0-or-later WITH GCC-exception-2.0
 */

#ifndef POINT_TABLE_H
#define POINT_TABLE_H

#include <stdint.h>
#include <stdbool.h>
#include "bacnet/bacenum.h"
#include "gateway_config.h"

#ifdef __cplusplus
extern "C" {
#endif

/* -----------------------------------------------------------------------
 * Enumerations
 * -----------------------------------------------------------------------*/

/** BACnet object types supported by this gateway */
typedef enum {
    GW_BACNET_TYPE_AI = 0, /**< Analog Input  */
    GW_BACNET_TYPE_AO, /**< Analog Output */
    GW_BACNET_TYPE_BI, /**< Binary Input  */
    GW_BACNET_TYPE_BO /**< Binary Output */
} GW_BACNET_TYPE;

/** Modbus function codes */
typedef enum {
    GW_MODBUS_FUNC_COIL = 0x01, /**< FC01 Read Coils */
    GW_MODBUS_FUNC_DISCRETE_INPUT = 0x02, /**< FC02 Read Discrete Inputs */
    GW_MODBUS_FUNC_HOLDING_REG = 0x03, /**< FC03 Read Holding Registers */
    GW_MODBUS_FUNC_INPUT_REG = 0x04 /**< FC04 Read Input Registers */
} GW_MODBUS_FUNC;

/* -----------------------------------------------------------------------
 * Point entry
 * -----------------------------------------------------------------------*/
typedef struct {
    /* Identification */
    char name[64];
    char description[128];

    /* BACnet side */
    GW_BACNET_TYPE bacnet_type;
    uint32_t bacnet_instance;
    BACNET_ENGINEERING_UNITS units;

    /* Modbus side */
    uint8_t modbus_slave;
    GW_MODBUS_FUNC modbus_func;
    uint16_t modbus_address;

    /* Value conversion: eng_value = raw * scale + offset */
    float scale;
    float offset;

    /* Valid engineering-value range; Out-of-Service when out of range */
    float range_min;
    float range_max;

    /* Runtime state (managed by point_table_poll_modbus) */
    bool enabled;
    bool initialized;
    bool comm_error;
} GW_POINT;

/* -----------------------------------------------------------------------
 * Point table
 * -----------------------------------------------------------------------*/
typedef struct {
    GW_POINT points[GW_MAX_POINTS];
    int count;
} GW_POINT_TABLE;

/* -----------------------------------------------------------------------
 * Gateway-level configuration read from JSON
 * -----------------------------------------------------------------------*/
typedef struct {
    uint32_t device_instance;
    char device_name[64];
    uint16_t vendor_id;

    /* ── BACnet datalink ── */
    char datalink_type[32]; /**< "mstp", "bip", "bip6", "ethernet", etc. */
    char bacnet_iface[64]; /**< network interface or serial port */

    /* MS/TP specific (used when datalink_type == "mstp") */
    uint32_t mstp_baud;
    uint8_t mstp_mac;
    uint16_t mstp_max_master;
    uint16_t mstp_net;

    /* BACnet/IP specific (used when datalink_type == "bip") */
    uint16_t bip_port; /**< UDP port, default 0xBAC0 (47808) */

    /* ── Modbus ── */
    char modbus_port[64];
    uint32_t modbus_baud;
    int poll_interval_sec;
} GW_CONFIG;

/* -----------------------------------------------------------------------
 * Public API
 * -----------------------------------------------------------------------*/

/**
 * @brief Load gateway config + point table from a JSON file.
 * @param cfg      Output: gateway-level configuration.
 * @param table    Output: point table.
 * @param filename Path to gateway_config.json.
 * @return true if at least one point was loaded successfully.
 */
bool point_table_load_json(
    GW_CONFIG *cfg, GW_POINT_TABLE *table, const char *filename);

/**
 * @brief Create BACnet objects for all points in the table.
 * Must be called after Device_Init() and before the main loop.
 * @param table  Populated point table.
 */
void point_table_init_bacnet_objects(const GW_POINT_TABLE *table);

/**
 * @brief Poll Modbus for all points and update BACnet Present_Value.
 * Called periodically from the main loop.
 * @param table  Point table (updated in-place).
 */
void point_table_poll_modbus(GW_POINT_TABLE *table);

#ifdef __cplusplus
}
#endif
#endif /* POINT_TABLE_H */
