/*####COPYRIGHTBEGIN####
 -------------------------------------------
 Copyright (C) 2005-2020 Steve Karg

 This program is free software; you can redistribute it and/or
 modify it under the terms of the GNU General Public License
 as published by the Free Software Foundation; either version 2
 of the License, or (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program; if not, write to:
 The Free Software Foundation, Inc.
 59 Temple Place - Suite 330
 Boston, MA  02111-1307, USA.

 As a special exception, if other files instantiate templates or
 use macros or inline functions from this file, or you compile
 this file and link it with other works to produce a work based
 on this file, this file does not by itself cause the resulting
 work to be covered by the GNU General Public License. However
 the source code for this file must still be made available in
 accordance with section (3) of the GNU General Public License.

 This exception does not invalidate any other reasons why a work
 based on this file might be covered by the GNU General Public
 License.
 -------------------------------------------
####COPYRIGHTEND####*/

#include <stdint.h>
#include <stdbool.h>
#include <zephyr/device.h>
#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/net/net_ip.h>
#include <zephyr/net/socket.h>
#include <zephyr/net/socket_select.h>
/* BACnet Stack defines - first */
#include "bacnet/bacdef.h"
/* BACnet Stack API */
#include "bacnet/bacdcode.h"
#include "bacnet/bacint.h"
#include "bacnet/datalink/bip6.h"
#include "bacnet/basic/object/device.h"
#include "bacnet/basic/sys/debug.h"
#include "bacnet/basic/bbmd6/h_bbmd6.h"

/* Logging module registration is already done in ports/zephyr/main.c */
#include <zephyr/logging/log.h>
#include <zephyr/logging/log_ctrl.h>

LOG_MODULE_DECLARE(bacnet, CONFIG_BACNETSTACK_LOG_LEVEL);

#define THIS_FILE "bip6-init.c"

/* zephyr socket */
static int BIP6_Socket = -1;
static int BIP6_Socket_Scope_Id;
/* local address - filled by init functions */
static BACNET_IP6_ADDRESS BIP6_Addr;
static BACNET_IP6_ADDRESS BIP6_Broadcast_Addr;

/* Used by inet6_ntoa */
#if CONFIG_BACNETSTACK_LOG_LEVEL
static char ipv6_addr_str[42] = { 0 };
#else
static char ipv6_addr_str[] = "";
#endif

/**
 * @brief Return a string representation of an IPv6 address
 * @param a - IPv6 address
 * @return Pointer to global string
 */
static char *inet6_ntoa(struct in6_addr *a)
{
#if CONFIG_BACNETSTACK_LOG_LEVEL
    uint8_t x = 0;
    uint8_t d = 0;
    uint8_t non_zero_count = 0;

    /* Avoid overwhelming the logging system */
    while (log_buffered_cnt()) {
        k_sleep(K_MSEC(1));
    }

    for (x = 0; x < IP6_ADDRESS_MAX; x += 2) {
        if (a->s6_addr[x] | a->s6_addr[x + 1]) {
            non_zero_count++;
            d += snprintf(
                &ipv6_addr_str[d], sizeof(ipv6_addr_str), "%02X%02X",
                a->s6_addr[x], a->s6_addr[x + 1]);
        }

        if (x < 14) {
            d += snprintf(&ipv6_addr_str[d], sizeof(ipv6_addr_str), ":");
        }
    }

    if (!non_zero_count) {
        snprintf(&ipv6_addr_str[0], sizeof(ipv6_addr_str), "undefined");
    }
#endif
    return &ipv6_addr_str[0];
}

void bip6_set_interface(char *ifname)
{
    struct net_if *interface = 0;
    int index = -1;
    int x = 0;

    BACNET_IP6_ADDRESS unicast = { 0 };
    BACNET_IP6_ADDRESS multicast = { 0 };

    unicast.port = bip6_get_port();
    multicast.port = unicast.port;

    LOG_DBG("bip6_set_interface()");
    LOG_INF("BIP6: UDP port: 0x%04X", (unsigned)unicast.port);
    LOG_INF("BIP6: seeking interface: %s", ifname?ifname:"NULL");
    if (ifname) {
        index = atoi(ifname);

        /* if index is zero, discern between "0" and a parse error */
        if (!index && strcmp(ifname, "0")) {
            LOG_ERR(
                "%s:%d - Argument must parse to an integer", THIS_FILE,
                __LINE__);
        } else {
            interface = net_if_get_by_index(index);
            if (interface) {
                LOG_INF("BIP6: Using interface %d", index);
            } else {
                LOG_ERR(
                    "%s:%d - No interface at index %d", THIS_FILE, __LINE__,
                    index);
            }
        }
    }
    if (index == -1) {
        LOG_INF("BIP6: No valid interface specified. Using default ");
        interface = net_if_get_default();
    }

    if (interface) {
        BIP6_Socket_Scope_Id = net_if_get_by_iface(interface);
        LOG_INF("BIP6: Socket Scope ID = %d", BIP6_Socket_Scope_Id);
        LOG_INF("BIP6: Interface set - Configured addresses:");
        for (x = 0; x < NET_IF_MAX_IPV6_ADDR; x++) {
            inet6_ntoa(&interface->config.ip.ipv6->unicast[x].address.in6_addr);
            LOG_INF("  unicast[%d]: %s", x, ipv6_addr_str);
        }
        for (x = 0; x < NET_IF_MAX_IPV6_MADDR; x++) {
            inet6_ntoa(&interface->config.ip.ipv6->mcast[x].address.in6_addr);
            LOG_INF(" multicast[%d]: %s", x, ipv6_addr_str);
        }
        if (CONFIG_BACDL_BIP6_ADDRESS_INDEX >= NET_IF_MAX_IPV6_ADDR) {
            LOG_ERR(
                "%s:%d - IPv6 address index of %d is out of range (0-%d)",
                THIS_FILE, __LINE__, CONFIG_BACDL_BIP6_ADDRESS_INDEX,
                NET_IF_MAX_IPV6_ADDR - 1);
            return;
        }
        LOG_INF("BIP6: Using configured index %d", 
            CONFIG_BACDL_BIP6_ADDRESS_INDEX);
        memcpy(
            &unicast.address,
            &interface->config.ip.ipv6->unicast[CONFIG_BACDL_BIP6_ADDRESS_INDEX]
                 .address.in6_addr,
            IP6_ADDRESS_MAX);

        if (net_addr_pton(
                AF_INET6, CONFIG_BACDL_BIP6_MCAST_ADDRESS,
                &multicast.address)) {
            LOG_ERR(
                "%s:%d - Failed to parse IPv6 multicast address: %s", THIS_FILE,
                __LINE__, CONFIG_BACDL_BIP6_MCAST_ADDRESS);
        }

        bip6_set_addr(&unicast);
        bip6_set_broadcast_addr(&multicast);

        LOG_INF(
            "   Unicast: %s", inet6_ntoa((struct in6_addr *)&unicast.address));
        LOG_INF(
            " Multicast: %s",
            inet6_ntoa((struct in6_addr *)&multicast.address));
    } else {
        LOG_ERR("%s:%d - Failed to set interface", THIS_FILE, __LINE__);
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
bool bip6_set_addr(BACNET_IP6_ADDRESS *addr)
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
bool bip6_set_broadcast_addr(BACNET_IP6_ADDRESS *addr)
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
int bip6_send_mpdu(BACNET_IP6_ADDRESS *dest, uint8_t *mtu, uint16_t mtu_len)
{
    struct sockaddr_in6 bvlc_dest = { 0 };
    uint16_t addr16[8];

    /* assumes that the driver has already been initialized */
    if (BIP6_Socket < 0) {
        LOG_ERR("%s:%d - Socket not initialized!", THIS_FILE, __LINE__);
        return BIP6_Socket;
    }

    /* load destination IP address */
    bvlc_dest.sin6_family = AF_INET6;
    bvlc6_address_get(
        dest, &addr16[0], &addr16[1], &addr16[2], &addr16[3], &addr16[4],
        &addr16[5], &addr16[6], &addr16[7]);
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
    inet6_ntoa(&bvlc_dest.sin6_addr);
    LOG_DBG("BIP6: Sending MPDU to %s", ipv6_addr_str);
    /* Send the packet */
    return zsock_sendto(
        BIP6_Socket, (char *)mtu, mtu_len, 0, (struct sockaddr *)&bvlc_dest,
        sizeof(bvlc_dest));
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
int bip6_send_pdu(
    BACNET_ADDRESS *dest,
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
    zsock_fd_set read_fds;
    int max = 0;
    struct zsock_timeval select_timeout;
    struct sockaddr_in6 sin = { 0 };
    BACNET_IP6_ADDRESS addr = { { 0 } };
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
    ZSOCK_FD_ZERO(&read_fds);
    ZSOCK_FD_SET(BIP6_Socket, &read_fds);
    max = BIP6_Socket;
    /* see if there is a packet for us */
    if (zsock_select(max + 1, &read_fds, NULL, NULL, &select_timeout) > 0) {
        received_bytes = zsock_recvfrom(
            BIP6_Socket, (char *)&npdu[0], max_npdu, 0, (struct sockaddr *)&sin,
            &sin_len);
    } else {
        return 0;
    }

    /* See if there is a problem */
    if (received_bytes < 0) {
        LOG_WRN(
            "%s:%d - RX zsock_recvfrom() error: %d", THIS_FILE, __LINE__,
            received_bytes);
        return 0;
    }
    /* no problem, just no bytes */
    if (received_bytes == 0) {
        return 0;
    }
    /* the signature of a BACnet/IPv6 packet */
    if (npdu[0] != BVLL_TYPE_BACNET_IP6) {
        LOG_DBG("BIP6: not a BACnet packet. Dropped.");
        return 0;
    }
    /* pass the packet into the BBMD handler */
    inet6_ntoa(&sin.sin6_addr);
    LOG_DBG("BIP6: Received MPDU from %s", ipv6_addr_str);
    bvlc6_address_set(
        &addr, ntohs(sin.sin6_addr.s6_addr16[0]),
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
            LOG_WRN("%s:%d - NPDU dropped (too long)!", THIS_FILE, __LINE__);
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
    LOG_DBG("bip6_cleanup();");
    bvlc6_cleanup();
    if (BIP6_Socket != -1) {
        zsock_close(BIP6_Socket);
    }
    BIP6_Socket = -1;
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
 * @param ifname [in] The named interface to use for the network layer.
 *        If NULL, the "eth0" interface is assigned.
 * @return True if the socket is successfully opened for BACnet/IP,
 *         else False if the socket functions fail.
 */
bool bip6_init(char *ifname)
{
    int sock_fd = -1;
    int status = -1;
    struct sockaddr_in6 server = { 0 };
    struct in6_addr broadcast_address;
    struct ipv6_mreq join_request;
    const int sockopt = 1;

    LOG_DBG("bip6_init()");
    bip6_set_interface(ifname);
    if (BIP6_Addr.port == 0) {
        bip6_set_port(0xBAC0U);
    }
    LOG_INF("BIP6: IPv6 UDP port: 0x%04X", (unsigned)BIP6_Addr.port);
    if (BIP6_Broadcast_Addr.address[0] == 0) {
        bvlc6_address_set(
            &BIP6_Broadcast_Addr, BIP6_MULTICAST_SITE_LOCAL, 0, 0, 0, 0, 0, 0,
            BIP6_MULTICAST_GROUP_ID);
        LOG_INF("BIP6: IPv6 MULTICAST_SITE_LOCAL");
    }
    /* assumes that the driver has already been initialized */
    sock_fd = zsock_socket(AF_INET6, SOCK_DGRAM, IPPROTO_UDP);
    BIP6_Socket = sock_fd;
    if (sock_fd < 0) {
        LOG_ERR("%s:%d - Failed to create socket", THIS_FILE, __LINE__);
        return false;
    } else {
        LOG_INF("Socket created");
    }

    /* Allow us to use the same socket for sending and receiving */
    /* This makes sure that the src port is correct when sending */
    status = zsock_setsockopt(
        sock_fd, SOL_SOCKET, SO_REUSEADDR, &sockopt, sizeof(sockopt));
    if (status < 0) {
        zsock_close(sock_fd);
        BIP6_Socket = -1;
        return false;
    }
    /* subscribe to a multicast address */
    memcpy(
        &broadcast_address.s6_addr[0], &BIP6_Broadcast_Addr.address[0],
        IP6_ADDRESS_MAX);
    memcpy(
        &join_request.ipv6mr_multiaddr, &broadcast_address,
        sizeof(struct in6_addr));
    /* Let system not choose the interface */
    join_request.ipv6mr_ifindex = BIP6_Socket_Scope_Id;
    status = setsockopt(
        BIP6_Socket, IPPROTO_IPV6, IPV6_ADD_MEMBERSHIP, &join_request,
        sizeof(join_request));
    if (status < 0) {
        perror("BIP6: setsockopt(IPV6_ADD_MEMBERSHIP)");
    }

    /* bind the socket to the local port number and IP address */
    server.sin6_family = AF_INET6;

    server.sin6_addr = in6addr_any;
    server.sin6_port = htons(BIP6_Addr.port);

    LOG_INF("BIP6: Binding to port %d", BIP6_Addr.port);

    status =
        zsock_bind(sock_fd, (const struct sockaddr *)&server, sizeof(server));
    if (status < 0) {
        zsock_close(sock_fd);
        BIP6_Socket = -1;
        LOG_ERR("%s:%d - zsock_bind() failure", THIS_FILE, __LINE__);
        return false;
    } else {
        LOG_INF("BIP6: Socket bound");
    }

    bvlc6_init();

    LOG_DBG("bip6_init() success");
    return true;
}

/**
 * @brief Check if the BACnet/IPv6 socket is valid
 * @return True if the socket is valid, else False
 */
bool bip6_valid(void)
{
    return (BIP6_Socket != -1);
}
