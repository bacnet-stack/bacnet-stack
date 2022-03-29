/*####COPYRIGHTBEGIN####
 -------------------------------------------
 Copyright (C) 2012 Steve Karg

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
#include <stdint.h> /* for standard integer types uint8_t etc. */
#include <stdbool.h> /* for the standard bool type. */
#include "bacnet/bacdef.h"
#include "bacnet/bacdcode.h"
#include "bacnet/bacint.h"
#include "bacnet/datalink/bip.h"
#include "bacnet/datalink/bvlc.h"
#include "bacnet/basic/services.h"
#include "bacnet/basic/bbmd/h_bbmd.h"
#include "bacport.h" /* custom per port */

/** @file bip.c  Configuration and Operations for BACnet/IP */

/* port to use - stored in network byte order */
static bool BIP_Port_Changed;
/* IP Address */
static BACNET_IP_ADDRESS BIP_Address;
/* Broadcast Address */
static BACNET_IP_ADDRESS BIP_Broadcast_Address;
/* lwIP socket, of sorts */
static struct udp_pcb *Server_upcb;
/* track packets for diagnostics */
struct bacnet_stats {
    uint32_t xmit; /* Transmitted packets. */
    uint32_t recv; /* Received packets. */
    uint32_t drop; /* Dropped packets. */
};
struct bacnet_stats BIP_Stats;
#define BIP_STATS_INC(x) ++BIP_Stats.x

/**
 * @brief Get the BACnet/IP transmit stats
 * @return number of BACnet/IP transmitted packets
 */
uint32_t bip_stats_xmit(void)
{
    return BIP_Stats.xmit;
}

/**
 * @brief Get the BACnet/IP received stats
 * @return number of BACnet/IP received packets
 */
uint32_t bip_stats_recv(void)
{
    return BIP_Stats.recv;
}

/**
 * @brief Get the BACnet/IP drop stats
 * @return number of BACnet/IP drops
 */
uint32_t bip_stats_drop(void)
{
    return BIP_Stats.drop;
}

/**
 * @brief Set the BACnet/IP address
 * @param addr - network IPv4 address
 * @return true if the address is set
 */
bool bip_set_addr(BACNET_IP_ADDRESS *addr)
{
    return bvlc_address_copy(&BIP_Address, addr);
}

/**
 * @brief Get the BACnet/IP address
 * @param network IPv4 address
 * @return true if the address is set
 */
bool bip_get_addr(BACNET_IP_ADDRESS *addr)
{
    return bvlc_address_copy(addr, &BIP_Address);
}

/**
 * @brief Set the BACnet/IP broadcast address
 * @param network IPv4 broadcast address
 * @return true if the broadcast address is retrieved
 */
bool bip_set_broadcast_addr(BACNET_IP_ADDRESS *addr)
{
    return bvlc_address_copy(&BIP_Broadcast_Address, addr);
}

/**
 * @brief Get the BACnet/IP broadcast address
 * @param network IPv4 broadcast address
 * @return true if the broadcast address is set
 */
bool bip_get_broadcast_addr(BACNET_IP_ADDRESS *addr)
{
    return bvlc_address_copy(addr, &BIP_Broadcast_Address);
}

/**
 * @brief Set the BACnet IPv4 UDP port number
 * @param port - IPv4 UDP port number - in host byte order
 */
void bip_set_port(uint16_t port)
{
    if (BIP_Address.port != htons(port)) {
        BIP_Port_Changed = true;
        BIP_Address.port = htons(port);
    }
}

/**
 * @brief Determine if the BACnet IPv4 UDP port number changed
 * @return true of the BACnet IPv4 UDP port number changed
 */
bool bip_port_changed(void)
{
    return BIP_Port_Changed;
}

/**
 * @brief Get the BACnet IPv4 UDP port number
 * @return IPv4 UDP port number - in host byte order
 */
uint16_t bip_get_port(void)
{
    return ntohs(BIP_Address.port);
}

/**
 * @brief Convert the BACnet IPv4 address
 * @param address - IPv4 address from LwIP
 * @param mac - IP address from BACnet/IP
 */
static void bip_mac_to_addr(ip4_addr_t *address, uint8_t *mac)
{
    if (mac && address) {
        address->addr = ((u32_t)((((uint32_t)mac[0]) << 24) & 0xff000000));
        address->addr |= ((u32_t)((((uint32_t)mac[1]) << 16) & 0x00ff0000));
        address->addr |= ((u32_t)((((uint32_t)mac[2]) << 8) & 0x0000ff00));
        address->addr |= ((u32_t)(((uint32_t)mac[3]) & 0x000000ff));
    }
}

/**
 * @brief Convert from a BACnet/IP address
 * @param baddr - BACnet/IP address
 * @param address - IPv4 address from LwIP
 * @param port - IPv4 UDP port number
 */
static int bip_decode_bip_address(BACNET_IP_ADDRESS *baddr,
    ip_addr_t *address,
    uint16_t *port)
{
    int len = 0;

    if (baddr && address && port) {
        address->type = IPADDR_TYPE_V4;
        bip_mac_to_addr(&address->u_addr.ip4, &baddr->address[0]);
        *port = baddr->port;
        len = 6;
    }

    return len;
}

/**
 * @brief Convert the BACnet IPv4 address
 * @param mac - IP address from BACnet/IP
 * @param address - IPv4 address from LwIP
 */
static void bip_addr_to_mac(uint8_t *mac, const ip4_addr_t *address)
{
    if (mac && address) {
        mac[0] = (uint8_t)(address->addr >> 24);
        mac[1] = (uint8_t)(address->addr >> 16);
        mac[2] = (uint8_t)(address->addr >> 8);
        mac[3] = (uint8_t)(address->addr);
    }
}

/**
 * @brief Convert to a BACnet/IP address
 * @param baddr - BACnet/IP address
 * @param address - IPv4 address from LwIP
 * @param port - IPv4 UDP port number
 */
static int bip_encode_bip_address(BACNET_IP_ADDRESS *baddr,
    const ip_addr_t *address,
    uint16_t port)
{
    int len = 0;

    if (baddr && address) {
        if (address->type == IPADDR_TYPE_V4) {
            bip_addr_to_mac(&baddr->address[0], &address->u_addr.ip4);
            baddr->port = port;
            len = 6;
        }
    }

    return len;
}

/** Function to send a packet out the BACnet/IP socket (Annex J).
 * @ingroup DLBIP
 *
 * @param dest [in] Destination address
 * @param port [in] UDP port number
 * @param mtu_len [in] PBUF packet
 * @return number of bytes sent, or 0 on failure.
 */
int bip_send_mpdu(
    BACNET_IP_ADDRESS *dest, uint8_t *mtu, uint16_t mtu_len)
{
    struct pbuf *pkt = NULL;
    /* addr and port in host format */
    ip_addr_t dst_ip;
    uint16_t port = 0;
    err_t status = ERR_OK;

    pkt = pbuf_alloc(PBUF_TRANSPORT, mtu_len, PBUF_POOL);
    if (pkt == NULL) {
        return 0;
    }
    bip_decode_bip_address(dest, &dst_ip, &port);
    pbuf_take(pkt, mtu, mtu_len);
    status = udp_sendto(Server_upcb, pkt, &dst_ip, port);
    if (status == ERR_OK) {
        BIP_STATS_INC(xmit);
    } else {
        mtu_len = 0;
    }
    pbuf_free(pkt);

    return mtu_len;
}

/** Send the Original Broadcast or Unicast messages
 *
 * @param dest [in] Destination address (may encode an IP address and port #).
 * @param npdu_data [in] The NPDU header (Network) information (not used).
 * @param pdu [in] Buffer of data to be sent - may be null (why?).
 * @param pdu_len [in] Number of bytes in the pdu buffer.
 *
 * @return number of bytes sent
 */
int bip_send_pdu(BACNET_ADDRESS *dest, /* destination address */
    BACNET_NPDU_DATA *npdu_data, /* network information */
    uint8_t *pdu, /* any data to be sent - may be null */
    unsigned pdu_len)
{
    return bvlc_send_pdu(dest, npdu_data, pdu, pdu_len);
}

/** LwIP BACnet service callback
 *
 * @param arg [in] optional argument from service
 * @param upcb [in] UDP control block
 * @param pkt [in] UDP packet - PBUF
 * @param addr [in] UDP source address
 * @param port [in] UDP port number
 */
void bip_server_callback(void *arg,
    struct udp_pcb *upcb,
    struct pbuf *pkt,
    const ip_addr_t *addr,
    u16_t port)
{
    uint8_t function = 0;
    uint16_t npdu_offset = 0;
    BACNET_ADDRESS src = { 0 }; /* address where message came from */
    BACNET_IP_ADDRESS saddr;
    uint8_t *npdu = (uint8_t *) pkt->payload;
    uint16_t npdu_len = pkt->tot_len;

    bip_encode_bip_address(&saddr, addr, port);
    npdu_offset = bvlc_handler(&saddr, &src, npdu, npdu_len);
    if (npdu_offset > 0) {
        BIP_STATS_INC(recv);
        npdu_len -= npdu_offset;
        npdu_handler(&src, &npdu[npdu_offset], npdu_len);
    } else {
        BIP_STATS_INC(drop);
    }
    /* free our packet */
    pbuf_free(pkt);
}

void bip_get_my_address(BACNET_ADDRESS *my_address)
{
    int i = 0;

    if (my_address) {
        my_address->mac_len = 6;
        memcpy(&my_address->mac[0], &BIP_Address.address, 4);
        memcpy(&my_address->mac[4], &BIP_Address.port, 2);
        my_address->net = 0; /* local only, no routing */
        my_address->len = 0; /* no SLEN */
        for (i = 0; i < MAX_MAC_LEN; i++) {
            /* no SADR */
            my_address->adr[i] = 0;
        }
    }

    return;
}

void bip_get_broadcast_address(BACNET_ADDRESS *dest)
{ /* destination address */
    int i = 0; /* counter */

    if (dest) {
        dest->mac_len = 6;
        memcpy(&dest->mac[0], &BIP_Broadcast_Address.address, 4);
        memcpy(&dest->mac[4], &BIP_Address.port, 2);
        dest->net = BACNET_BROADCAST_NETWORK;
        dest->len = 0; /* no SLEN */
        for (i = 0; i < MAX_MAC_LEN; i++) {
            /* no SADR */
            dest->adr[i] = 0;
        }
    }

    return;
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
 * @note For Windows, ifname is the dotted ip address of the interface.
 *
 * @param ifname [in] The named interface to use for the network layer.
 *        If NULL, the "eth0" interface is assigned.
 * @return True if the socket is successfully opened for BACnet/IP,
 *         else False if the socket functions fail.
 */
bool bip_init(char *ifname)
{
    (void)ifname;
    /* Create a new UDP control block  */
    Server_upcb = udp_new();
    if (Server_upcb == NULL) {
        while (1) {
            /* fixme: increase MEMP_NUM_UDP_PCB in lwipopts.h */
        };
    }
    /* Bind the upcb to the UDP_PORT port */
    /* Using IP_ADDR_ANY allow the upcb to be used by any local interface */
    udp_bind(Server_upcb, IP_ADDR_ANY, BIP_Address.port);
    /* Set a receive callback for the upcb */
    udp_recv(Server_upcb, bip_server_callback, NULL);

    return true;
}
