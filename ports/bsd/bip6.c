/**************************************************************************
 *
 * Copyright (C) 2016 Steve Karg
 *
 * SPDX-License-Identifier: GPL-2.0-or-later WITH GCC-exception-2.0
 *
 *********************************************************************/
#include <ifaddrs.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h> /* for standard integer types uint8_t etc. */
#include <stdbool.h> /* for the standard bool type. */
#include "bacnet/bacdcode.h"
#include "bacnet/config.h"
#include "bacnet/datalink/bip6.h"
#include "bacnet/basic/sys/debug.h"
#include "bacnet/basic/object/device.h"
#include "bacnet/basic/bbmd6/h_bbmd6.h"
#include "bacport.h"

#if defined(__APPLE__) || defined(__darwin__)
/* OSX seems not to define these. */
#ifndef s6_addr16
#define s6_addr16 __u6_addr.__u6_addr16
#endif
#ifndef s6_addr32
#define s6_addr32 __u6_addr.__u6_addr32
#endif
#endif

/* enable debugging */
static bool BIP6_Debug = false;
#if PRINT_ENABLED
#include <stdarg.h>
#include <stdio.h>
#define PRINTF(...) \
    if (BIP6_Debug) { \
        fprintf(stderr,__VA_ARGS__); \
        fflush(stderr); \
    }
#else
#define PRINTF(...)
#endif

/**
 * @brief Print the IPv6 address with debug info
 * @param str - debug info string
 * @param addr - IPv4 address
 */
static void debug_print_ipv6(const char *str, const struct in6_addr *addr)
{
    PRINTF("BIP6: %s "
        "%02x%02x:%02x%02x:%02x%02x:%02x%02x:"
        "%02x%02x:%02x%02x:%02x%02x:%02x%02x\n",
        str, (int)addr->s6_addr[0], (int)addr->s6_addr[1],
        (int)addr->s6_addr[2], (int)addr->s6_addr[3],
        (int)addr->s6_addr[4], (int)addr->s6_addr[5],
        (int)addr->s6_addr[6], (int)addr->s6_addr[7],
        (int)addr->s6_addr[8], (int)addr->s6_addr[9],
        (int)addr->s6_addr[10], (int)addr->s6_addr[11],
        (int)addr->s6_addr[12], (int)addr->s6_addr[13],
        (int)addr->s6_addr[14], (int)addr->s6_addr[15]);
}

/**
 * @brief Enabled debug printing of BACnet/IPv4
 */
void bip6_debug_enable(void)
{
    BIP6_Debug = true;
}

/** @file bsd/bip6.c  Initializes BACnet/IPv6 interface (BSD). */

/* unix socket */
static int BIP6_Socket = -1;
static int BIP6_Socket_Scope_Id = 0;
/* local address - filled by init functions */
static BACNET_IP6_ADDRESS BIP6_Addr;
static BACNET_IP6_ADDRESS BIP6_Broadcast_Addr;

/**
 * Set the interface name. On BSD, ifname is the /dev/ name of the interface.
 *
 * @param ifname - C string for name or text address
 */
void bip6_set_interface(char *ifname)
{
    struct ifaddrs *ifa, *ifa_tmp;
    struct sockaddr_in6 *sin;
    bool found = false;

    if (getifaddrs(&ifa) == -1) {
        perror("BIP6: getifaddrs failed");
        exit(1);
    }
    ifa_tmp = ifa;
    if (BIP6_Debug) {
        PRINTF("BIP6: seeking interface: %s\n", ifname);
    }
    while (ifa_tmp) {
        if ((ifa_tmp->ifa_addr) && (ifa_tmp->ifa_addr->sa_family == AF_INET6)) {
            PRINTF("BIP6: found interface: %s\n", ifa_tmp->ifa_name);
        }
        if ((ifa_tmp->ifa_addr) && (ifa_tmp->ifa_addr->sa_family == AF_INET6) &&
            (strcasecmp(ifa_tmp->ifa_name, ifname) == 0)) {
            sin = (struct sockaddr_in6 *)ifa_tmp->ifa_addr;
            bvlc6_address_set(&BIP6_Addr, ntohs(sin->sin6_addr.s6_addr16[0]),
                ntohs(sin->sin6_addr.s6_addr16[1]),
                ntohs(sin->sin6_addr.s6_addr16[2]),
                ntohs(sin->sin6_addr.s6_addr16[3]),
                ntohs(sin->sin6_addr.s6_addr16[4]),
                ntohs(sin->sin6_addr.s6_addr16[5]),
                ntohs(sin->sin6_addr.s6_addr16[6]),
                ntohs(sin->sin6_addr.s6_addr16[7]));
            debug_print_ipv6(ifname, (&sin->sin6_addr));
            found = true;
            BIP6_Socket_Scope_Id = if_nametoindex(ifname);
            break;
        }
        ifa_tmp = ifa_tmp->ifa_next;
    }
    if (!found) {
        PRINTF("BIP6: unable to set interface: %s\n", ifname);
        exit(1);
    }
}

/**
 * Set the BACnet IPv6 UDP port number
 *
 * @param port - IPv6 UDP port number
 */
void bip6_set_port(uint16_t port)
{
    BIP6_Addr.port = port;
    BIP6_Broadcast_Addr.port = port;
}

/**
 * Get the BACnet IPv6 UDP port number
 *
 * @return IPv6 UDP port number
 */
uint16_t bip6_get_port(void)
{
    return BIP6_Addr.port;
}

/**
 * Get the BACnet broadcast address for my interface.
 * Used as dest address in messages sent as BROADCAST
 *
 * @param addr - IPv6 source address
 */
void bip6_get_broadcast_address(BACNET_ADDRESS *addr)
{
    if (addr) {
        addr->net = BACNET_BROADCAST_NETWORK;
        addr->mac_len = 0;
        addr->len = 0;
    }
}

/**
 * Get the IPv6 address for my interface. Used as src address in messages sent.
 *
 * @param addr - IPv6 source address
 */
void bip6_get_my_address(BACNET_ADDRESS *addr)
{
    uint32_t device_id = 0;

    if (addr) {
        device_id = Device_Object_Instance_Number();
        bvlc6_vmac_address_set(addr, device_id);
    }
}

/**
 * Set the BACnet/IP address
 *
 * @param addr - network IPv6 address
 */
bool bip6_set_addr(const BACNET_IP6_ADDRESS *addr)
{
    return bvlc6_address_copy(&BIP6_Addr, addr);
}

/**
 * Get the BACnet/IP address
 *
 * @return BACnet/IP address
 */
bool bip6_get_addr(BACNET_IP6_ADDRESS *addr)
{
    return bvlc6_address_copy(addr, &BIP6_Addr);
}

/**
 * Set the BACnet/IP address
 *
 * @param addr - network IPv6 address
 */
bool bip6_set_broadcast_addr(const BACNET_IP6_ADDRESS *addr)
{
    return bvlc6_address_copy(&BIP6_Broadcast_Addr, addr);
}

/**
 * Get the BACnet/IP address
 *
 * @return BACnet/IP address
 */
bool bip6_get_broadcast_addr(BACNET_IP6_ADDRESS *addr)
{
    return bvlc6_address_copy(addr, &BIP6_Broadcast_Addr);
}

/**
 * The send function for BACnet/IPv6 driver layer
 *
 * @param dest - Points to a BACNET_IP6_ADDRESS structure containing the
 *  destination address.
 * @param mtu - the bytes of data to send
 * @param mtu_len - the number of bytes of data to send
 *
 * @return Upon successful completion, returns the number of bytes sent.
 *  Otherwise, -1 shall be returned and errno set to indicate the error.
 */
int bip6_send_mpdu(
    const BACNET_IP6_ADDRESS *dest, const uint8_t *mtu, uint16_t mtu_len)
{
    struct sockaddr_in6 bvlc_dest = { 0 };
    uint16_t addr16[8];

    /* assumes that the driver has already been initialized */
    if (BIP6_Socket < 0) {
        return 0;
    }
    /* load destination IP address */
    bvlc_dest.sin6_family = AF_INET6;
    bvlc6_address_get(dest, &addr16[0], &addr16[1], &addr16[2], &addr16[3],
        &addr16[4], &addr16[5], &addr16[6], &addr16[7]);
    bvlc_dest.sin6_addr.s6_addr16[0] = htons(addr16[0]);
    bvlc_dest.sin6_addr.s6_addr16[1] = htons(addr16[1]);
    bvlc_dest.sin6_addr.s6_addr16[2] = htons(addr16[2]);
    bvlc_dest.sin6_addr.s6_addr16[3] = htons(addr16[3]);
    bvlc_dest.sin6_addr.s6_addr16[4] = htons(addr16[4]);
    bvlc_dest.sin6_addr.s6_addr16[5] = htons(addr16[5]);
    bvlc_dest.sin6_addr.s6_addr16[6] = htons(addr16[6]);
    bvlc_dest.sin6_addr.s6_addr16[7] = htons(addr16[7]);
    bvlc_dest.sin6_port = htons(dest->port);
    bvlc_dest.sin6_scope_id = BIP6_Socket_Scope_Id;
    debug_print_ipv6("Sending MPDU->", &bvlc_dest.sin6_addr);
    /* Send the packet */
    return sendto(BIP6_Socket, (const char *)mtu, mtu_len, 0,
        (struct sockaddr *)&bvlc_dest, sizeof(bvlc_dest));
}

/**
 * The common send function for BACnet/IPv6 application layer
 *
 * @param dest - Points to a #BACNET_ADDRESS structure containing the
 *  destination address.
 * @param npdu_data - Points to a BACNET_NPDU_DATA structure containing the
 *  destination network layer control flags and data.
 * @param pdu - the bytes of data to send
 * @param pdu_len - the number of bytes of data to send
 * @return Upon successful completion, returns the number of bytes sent.
 *  Otherwise, -1 shall be returned and errno set to indicate the error.
 */
int bip6_send_pdu(BACNET_ADDRESS *dest,
    BACNET_NPDU_DATA *npdu_data,
    uint8_t *pdu,
    unsigned pdu_len)
{
    return bvlc6_send_pdu(dest, npdu_data, pdu, pdu_len);
}

/**
 * BACnet/IP Datalink Receive handler.
 *
 * @param src - returns the source address
 * @param npdu - returns the NPDU buffer
 * @param max_npdu -maximum size of the NPDU buffer
 * @param timeout - number of milliseconds to wait for a packet
 *
 * @return Number of bytes received, or 0 if none or timeout.
 */
uint16_t bip6_receive(
    BACNET_ADDRESS *src, uint8_t *npdu, uint16_t max_npdu, unsigned timeout)
{
    uint16_t npdu_len = 0; /* return value */
    fd_set read_fds;
    int max = 0;
    struct timeval select_timeout;
    struct sockaddr_in6 sin = { 0 };
    BACNET_IP6_ADDRESS addr = { 0 };
    socklen_t sin_len = sizeof(sin);
    int received_bytes = 0;
    int offset = 0;
    uint16_t i = 0;

    /* Make sure the socket is open */
    if (BIP6_Socket < 0) {
        return 0;
    }
    /* we could just use a non-blocking socket, but that consumes all
       the CPU time.  We can use a timeout; it is only supported as
       a select. */
    if (timeout >= 1000) {
        select_timeout.tv_sec = timeout / 1000;
        select_timeout.tv_usec =
            1000 * (timeout - select_timeout.tv_sec * 1000);
    } else {
        select_timeout.tv_sec = 0;
        select_timeout.tv_usec = 1000 * timeout;
    }
    FD_ZERO(&read_fds);
    FD_SET(BIP6_Socket, &read_fds);
    max = BIP6_Socket;
    /* see if there is a packet for us */
    if (select(max + 1, &read_fds, NULL, NULL, &select_timeout) > 0) {
        received_bytes = recvfrom(BIP6_Socket, (char *)&npdu[0], max_npdu, 0,
            (struct sockaddr *)&sin, &sin_len);
    } else {
        return 0;
    }
    /* See if there is a problem */
    if (received_bytes < 0) {
        return 0;
    }
    /* no problem, just no bytes */
    if (received_bytes == 0) {
        return 0;
    }
    /* the signature of a BACnet/IPv6 packet */
    if (npdu[0] != BVLL_TYPE_BACNET_IP6) {
        return 0;
    }
    /* pass the packet into the BBMD handler */
    debug_print_ipv6("Received MPDU->", &sin.sin6_addr);
    bvlc6_address_set(&addr, ntohs(sin.sin6_addr.s6_addr16[0]),
        ntohs(sin.sin6_addr.s6_addr16[1]), ntohs(sin.sin6_addr.s6_addr16[2]),
        ntohs(sin.sin6_addr.s6_addr16[3]), ntohs(sin.sin6_addr.s6_addr16[4]),
        ntohs(sin.sin6_addr.s6_addr16[5]), ntohs(sin.sin6_addr.s6_addr16[6]),
        ntohs(sin.sin6_addr.s6_addr16[7]));
    addr.port = ntohs(sin.sin6_port);
    offset = bvlc6_handler(&addr, src, npdu, received_bytes);
    if (offset > 0) {
        npdu_len = received_bytes - offset;
        if (npdu_len <= max_npdu) {
            /* shift the buffer to return a valid NPDU */
            for (i = 0; i < npdu_len; i++) {
                npdu[i] = npdu[offset + i];
            }
        } else {
            npdu_len = 0;
        }
    }

    return npdu_len;
}

/** Cleanup and close out the BACnet/IP services by closing the socket.
 * @ingroup DLBIP6
 */
void bip6_cleanup(void)
{
    bvlc6_cleanup();
    if (BIP6_Socket != -1) {
        close(BIP6_Socket);
    }
    BIP6_Socket = -1;

    return;
}

/** Initialize the BACnet/IP services at the given interface.
 * @ingroup DLBIP6
 * -# Gets the local IP address and local broadcast address from the system,
 *  and saves it into the BACnet/IPv6 data structures.
 * -# Opens a UDP socket
 * -# Configures the socket for sending and receiving
 * -# Configures the socket so it can send multicasts
 * -# Binds the socket to the local IP address at the specified port for
 *    BACnet/IPv6 (by default, 0xBAC0 = 47808).
 *
 * @note For BSD, ifname is en0, e0, and others.
 *
 * @param ifname [in] The named interface to use for the network layer.
 *        If NULL, the "en0" interface is assigned.
 * @return True if the socket is successfully opened for BACnet/IP,
 *         else False if the socket functions fail.
 */
bool bip6_init(char *ifname)
{
    int status = 0; /* return from socket lib calls */
    struct sockaddr_in6 server = { 0 };
    struct in6_addr broadcast_address;
    struct ipv6_mreq join_request;
    int sockopt = 0;

    if (ifname) {
        bip6_set_interface(ifname);
    } else {
        bip6_set_interface("en0");
    }
    if (BIP6_Addr.port == 0) {
        bip6_set_port(0xBAC0U);
    }
    PRINTF("BIP6: IPv6 UDP port: 0x%04X\n", BIP6_Addr.port);
    if (BIP6_Broadcast_Addr.address[0] == 0) {
        bvlc6_address_set(&BIP6_Broadcast_Addr, BIP6_MULTICAST_SITE_LOCAL, 0, 0,
            0, 0, 0, 0, BIP6_MULTICAST_GROUP_ID);
    }
    /* assumes that the driver has already been initialized */
    BIP6_Socket = socket(AF_INET6, SOCK_DGRAM, IPPROTO_UDP);
    if (BIP6_Socket < 0)
        return false;
    /* Allow us to use the same socket for sending and receiving */
    /* This makes sure that the src port is correct when sending */
    sockopt = 1;
    status = setsockopt(
        BIP6_Socket, SOL_SOCKET, SO_REUSEADDR, &sockopt, sizeof(sockopt));
    if (status < 0) {
        close(BIP6_Socket);
        BIP6_Socket = -1;
        return status;
    }
    /* allow us to send a broadcast */
    status = setsockopt(
        BIP6_Socket, SOL_SOCKET, SO_BROADCAST, &sockopt, sizeof(sockopt));
    if (status < 0) {
        close(BIP6_Socket);
        BIP6_Socket = -1;
        return false;
    }
    /* subscribe to a multicast address */
    memcpy(&broadcast_address.s6_addr[0], &BIP6_Broadcast_Addr.address[0],
        IP6_ADDRESS_MAX);
    memcpy(&join_request.ipv6mr_multiaddr, &broadcast_address,
        sizeof(struct in6_addr));
    /* Let system not choose the interface */
    join_request.ipv6mr_interface = BIP6_Socket_Scope_Id;
    status = setsockopt(BIP6_Socket, IPPROTO_IPV6, IPV6_JOIN_GROUP,
        &join_request, sizeof(join_request));
    if (status < 0) {
        perror("BIP: setsockopt(IPV6_JOIN_GROUP)");
    }

    /* bind the socket to the local port number and IP address */
    server.sin6_family = AF_INET6;
    server.sin6_addr = in6addr_any;
    server.sin6_port = htons(BIP6_Addr.port);
    debug_print_ipv6("Binding->", &server.sin6_addr);
    status = bind(BIP6_Socket, (const void *)&server, sizeof(server));
    if (status < 0) {
        perror("BIP: bind");
        close(BIP6_Socket);
        BIP6_Socket = -1;
        return false;
    }
    bvlc6_init();

    return true;
}
