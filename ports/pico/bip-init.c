/**************************************************************************
 *
 * Copyright (C) 2005 Steve Karg
 * Copyright (C) 2025 Testimony Adams <adamstestimony@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later WITH GCC-exception-2.0
 *
 *********************************************************************/
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include "bacnet/bacdcode.h"
#include "bip.h"

/** @file pico/bip-init.c  Initializes BACnet/IP interface (Pico). */

static bool BIP_Debug = false;

/**
 * @brief Enabled debug printing of BACnet/IPv4
 */
void bip_debug_enable(void)
{
    BIP_Debug = true;
}

/**
 * @brief Disable debug printing of BACnet/IPv4
 */
void bip_debug_disable(void)
{
    BIP_Debug = false;
}

/* gets an IP address by name, where name can be a
   string that is an IP address in dotted form, or
   a name that is a domain name
   returns 0 if not found, or
   an IP address in network byte order */
long bip_getaddrbyname(const char *host_name)
{
    (void)host_name;
    return 0;
}

/** Gets the local IP address and local broadcast address from the system,
 *  and saves it into the BACnet/IP data structures.
 */
void bip_set_interface(void)
{
    uint8_t local_address[] = { 0, 0, 0, 0 };
    uint8_t broadcast_address[] = { 0, 0, 0, 0 };
    uint8_t netmask[] = { 0, 0, 0, 0 };
    uint8_t invertedNetmask[] = { 0, 0, 0, 0 };
    int i;

    /* Get network info from platform-specific function */
    if (!bip_get_local_network_info(local_address, netmask)) {
        return;
    }

    bip_set_addr(local_address);

    /* setup local broadcast address */
    for (i = 0; i < 4; i++) {
        invertedNetmask[i] = ~netmask[i];
        broadcast_address[i] = (local_address[i] | invertedNetmask[i]);
    }

    bip_set_broadcast_addr(broadcast_address);
}

/** Initialize the BACnet/IP services at the given interface.
 * @ingroup DLBIP
 * -# Gets the local IP address and local broadcast address from the system,
 *  and saves it into the BACnet/IP data structures.
 * -# Opens a UDP socket
 * -# Configures the socket for sending and receiving
 * -# Configures the socket so it can send broadcasts
 * -# Binds the socket to the local IP address at the specified port for
 *    BACnet/IP (by default, 0xBAC0 = 47808).
 *
 * @note For Linux, ifname is eth0, ath0, arc0, and others.
 *
 * @param port [in] UDP port to use for BACnet/IP
 * @return True if the socket is successfully opened for BACnet/IP,
 *         else False if the socket functions fail.
 */
bool bip_init(uint16_t port)
{
    /* Initialize the socket using platform-specific function */
    if (!bip_socket_init(port)) {
        return false;
    }

    bip_set_interface();
    bip_set_port(port);

    /* Mark socket as valid (0 is a valid socket ID for Pico) */
    bip_set_socket(0);

    return true;
}

/** Cleanup and close out the BACnet/IP services by closing the socket.
 * @ingroup DLBIP
 */
void bip_cleanup(void)
{
    if (bip_valid()) {
        bip_socket_cleanup();
    }

    return;
}
