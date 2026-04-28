/**
 * @file gateway_config.h
 * @brief Compile-time default constants for the Modbus RTU <-> BACnet MSTP
 * gateway.
 *
 * These values are used when gateway_config.json is absent or incomplete.
 * Override at runtime by editing gateway_config.json.
 *
 * @author Jihye Ahn <jen.ahn@lge.com>
 *
 * @copyright Copyright (c) 2026 LG Electronics Inc.
 * SPDX-License-Identifier: MIT
 */
#ifndef GATEWAY_CONFIG_H
#define GATEWAY_CONFIG_H

/* =======================================================================
 * BACnet datalink defaults
 * =======================================================================*/
/** Default datalink type: "mstp", "bip", "bip6", "ethernet" */
#define GW_DATALINK_TYPE "mstp"
/** Default network interface / serial port used by the datalink */
#define GW_BACNET_IFACE "/dev/ttyUSB0"

/* =======================================================================
 * BACnet MS/TP defaults (used when datalink_type == "mstp")
 * =======================================================================*/
#define GW_DEVICE_INSTANCE 50002U
#define GW_DEVICE_NAME "Modbus Gateway"
#define GW_MSTP_MAC_ADDRESS 2
#define GW_MSTP_BAUD 38400
#define GW_VENDOR_ID 260

/* =======================================================================
 * BACnet/IP defaults (used when datalink_type == "bip")
 * =======================================================================*/
#define GW_BIP_PORT 0xBAC0U /**< 47808 */

/* =======================================================================
 * Modbus RTU defaults
 * =======================================================================*/
#define GW_MODBUS_PORT "/dev/ttyUSB1"
#define GW_MODBUS_BAUD 9600
#define GW_POLL_INTERVAL_SEC 5

/* =======================================================================
 * Point table limits
 * =======================================================================*/
#define GW_MAX_POINTS 256

#endif /* GATEWAY_CONFIG_H */
