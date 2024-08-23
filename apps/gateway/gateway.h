/**
 * @file
 * @brief Header for example gateway (ie, BACnet Router and Devices)
 * using the BACnet Stack.
 * @author Tom Brennan <tbrennan3@users.sourceforge.net>
 * @date 2010
 * @copyright SPDX-License-Identifier: MIT
 */
#ifndef BACNET_GATEWAY_H_
#define BACNET_GATEWAY_H_

/** @defgroup GatewayDemo Demo of a BACnet virtual gateway (multiple Device).
 * @ingroup Demos
 * This is a basic demonstration of a BACnet Router with child devices (ie,
 * gateway) appearing on a virtual BACnet network behind the Router.
 * This is an extension of the ServerDemo project.
 */

/* Device configuration definitions. */
#define FIRST_DEVICE_NUMBER 260001
#define VIRTUAL_DNET  2709      /* your choice of number here */
#define DEV_NAME_BASE "Gateway Demo Device"
#define DEV_DESCR_GATEWAY "Gateway Device and Router"
#define DEV_DESCR_REMOTE  "Routed Remote Device"

#endif
