/*####COPYRIGHTBEGIN####
 -------------------------------------------
 Copyright (C) 2005 Steve Karg

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
#include <device.h>
#include <init.h>
#include <kernel.h>
#include <sys/printk.h>
#include <net/net_ip.h>
#include <net/socket.h>
#include <net/socket_select.h>
#include "bacnet/bacdcode.h"
#include "bacnet/bacint.h"
#include "bacnet/datalink/bip.h"
#include "bacnet/basic/sys/debug.h"
#include "bacnet/basic/bbmd/h_bbmd.h"

/* Logging module registration is already done in ports/zephyr/main.c */
#include <logging/log.h>
#include <logging/log_ctrl.h>

LOG_MODULE_DECLARE(bacnet, CONFIG_BACNETSTACK_LOG_LEVEL);

#define THIS_FILE "bip-init.c"

/* zephyr socket */
static int BIP_Socket = -1;

/* NOTE: we store address and port in network byte order
   since BACnet/IP uses network byte order for all address byte arrays
*/
/* port to use - stored here in network byte order */
static uint16_t BIP_Port = htons(CONFIG_BACDL_BIP_PORT);
/* IP address - stored here in network byte order */
static struct in_addr BIP_Address;
/* IP broadcast address - stored here in network byte order */
static struct in_addr BIP_Broadcast_Addr;

/* Used by inet_ntoa */
#if CONFIG_BACNETSTACK_LOG_LEVEL
static char ipv4_addr_str[16]={0};
#else
static char ipv4_addr_str[]="";
#endif

/**
* @brief Return a string representation of an IPv4 address
* @param a - IPv4 address
* @return Pointer to global string
*/
char* inet_ntoa(struct in_addr *a)
{
    if (IS_ENABLED(CONFIG_BACNETSTACK_LOG_LEVEL)) {

        snprintf(ipv4_addr_str, sizeof(ipv4_addr_str), "%d.%d.%d.%d",
            a->s4_addr[0],a->s4_addr[1],a->s4_addr[2],a->s4_addr[3]);
    }

    return &ipv4_addr_str[0];
}

/**
 * @brief Print the IPv4 address with debug info
 * @param str - debug info string
 * @param addr - IPv4 address
 */
static void debug_print_ipv4(const char *str, const struct in_addr *addr,
    const unsigned int port, const unsigned int count)
{
    LOG_DBG("%s %s:%hu (%u bytes)", log_strdup(str), log_strdup(inet_ntoa((struct in_addr*) &addr)),
        ntohs(port), count);
}


/**
 * @brief Set the BACnet IPv4 UDP port number
 * @param port - IPv4 UDP port number - in host byte order
 */
void bip_set_port(uint16_t port)
{
    BIP_Port = htons(port);
}

/**
 * @brief Get the BACnet IPv4 UDP port number
 * @return IPv4 UDP port number - in host byte order
 */
uint16_t bip_get_port(void)
{
    return ntohs(BIP_Port);
}

/**
 * @brief Get the IPv4 address for my interface. Used for sending src address.
 * @param addr - BACnet datalink address
 */
void bip_get_my_address(BACNET_ADDRESS *addr)
{
    unsigned int i = 0;

    if (addr) {
        addr->mac_len = BIP_ADDRESS_MAX; /* 6 */
        memcpy(&addr->mac[0], &BIP_Address.s_addr, IP_ADDRESS_MAX); /* 4 */
        memcpy(&addr->mac[IP_ADDRESS_MAX], &BIP_Port, sizeof(BIP_Port));
        /* local only, no routing */
        addr->net = 0;
        /* no SLEN */
        addr->len = 0;
        for (i = 0; i < MAX_MAC_LEN; i++) {
            /* no SADR */
            addr->adr[i] = 0;
        }
    }
}

/**
 * Get the IPv4 broadcast address for my interface.
 *
 * @param addr - BACnet datalink address
 */

void bip_get_broadcast_address(BACNET_ADDRESS *dest)
{
    int i = 0; 

    if (dest) {
        dest->mac_len = BIP_ADDRESS_MAX;
        memcpy(&dest->mac[0], &BIP_Broadcast_Addr.s_addr, IP_ADDRESS_MAX);
        memcpy(&dest->mac[IP_ADDRESS_MAX], &BIP_Port, sizeof(BIP_Port));
        dest->net = BACNET_BROADCAST_NETWORK;
        dest->len = 0;
        for (i = 0; i < MAX_MAC_LEN; i++) {
            dest->adr[i] = 0;
        }
    }
    return;
}


/**
 * Set the BACnet/IP address
 *
 * @param addr - network IPv4 address
 */
bool bip_set_addr(BACNET_IP_ADDRESS *addr)
{
    if (addr) {
        memcpy(&BIP_Address.s_addr, &addr->address[0], IP_ADDRESS_MAX);
        memcpy(&BIP_Port, &addr->port, sizeof(addr->port));
        return true;
    }
    return false;
}

/**
 * @brief Get the BACnet/IP address
 * @param addr - network IPv4 address
 * @return true if the address was retrieved
 */
bool bip_get_addr(BACNET_IP_ADDRESS *addr)
{
    if (addr) {
        memcpy(&addr->address[0], &BIP_Address.s_addr, IP_ADDRESS_MAX);
        memcpy(&addr->port, &BIP_Port, sizeof(addr->port));
        return true;
    }
    return false;
}

/**
 * @brief Set the BACnet/IP address
 * @param addr - network IPv4 address
 * @return true if the address was set
 */
bool bip_set_broadcast_addr(BACNET_IP_ADDRESS *addr)
{
    if (addr) {
        memcpy(&BIP_Broadcast_Addr.s_addr, &addr->address[0], IP_ADDRESS_MAX);
        return true;
    }
    return false;
}

/**
 * Get the BACnet/IP address
 *
 * @return BACnet/IP address
 */
bool bip_get_broadcast_addr(BACNET_IP_ADDRESS *addr)
{
    if (addr) {
        memcpy(&addr->address[0], &BIP_Broadcast_Addr.s_addr, IP_ADDRESS_MAX);
        addr->port = ntohs(BIP_Port);
        return true;
    }
    return false;
}

/**
 * @brief Set the BACnet/IP subnet mask CIDR prefix
 * @return true if the subnet mask CIDR prefix is set
 */
bool bip_set_subnet_prefix(uint8_t prefix)
{
    /* not something we do within this driver */
    return false;
}

/**
 * @brief Get the BACnet/IP subnet mask CIDR prefix
 * @return subnet mask CIDR prefix 1..32
 */
uint8_t bip_get_subnet_prefix(void)
{
    uint32_t address = 0;
    uint32_t broadcast = 0;
    uint32_t test_broadcast = 0;
    uint32_t mask = 0xFFFFFFFE;
    uint8_t prefix = 0;

    address = BIP_Address.s_addr;
    broadcast = BIP_Broadcast_Addr.s_addr;
    /* calculate the subnet prefix from the broadcast address */
    for (prefix = 1; prefix <= 32; prefix++) {
        test_broadcast = (address & mask) | (~mask);
        if (test_broadcast == broadcast) {
            break;
        }
        mask = mask<<1;
    }

    return prefix;
}

/**
 * The send function for BACnet/IP driver layer
 *
 * @param dest - Points to a BACNET_IP_ADDRESS structure containing the
 *  destination address.
 * @param mtu - the bytes of data to send
 * @param mtu_len - the number of bytes of data to send
 *
 * @return Upon successful completion, returns the number of bytes sent.
 *  Otherwise, -1 shall be returned and errno set to indicate the error.
 */
int bip_send_mpdu(BACNET_IP_ADDRESS *dest, uint8_t *mtu, uint16_t mtu_len)
{
    struct sockaddr_in bip_dest = { 0 };

    /* assumes that the driver has already been initialized */
    if (BIP_Socket < 0) {
        LOG_ERR("%s:%d - Socket not initialized!", THIS_FILE, __LINE__);
        return BIP_Socket;
    }

    /* load destination IP address */
    bip_dest.sin_family = AF_INET;
    memcpy(&bip_dest.sin_addr.s_addr, &dest->address[0], IP_ADDRESS_MAX);
    bip_dest.sin_port = htons(dest->port);

    /* Send the packet */
    debug_print_ipv4("Sending MPDU->", &bip_dest.sin_addr, bip_dest.sin_port,
        mtu_len);
    return zsock_sendto(BIP_Socket, (char *)mtu, mtu_len, 0,
        (struct sockaddr *)&bip_dest, sizeof(struct sockaddr));
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
uint16_t bip_receive(
    BACNET_ADDRESS *src, uint8_t *npdu, uint16_t max_npdu, unsigned timeout)
{
    uint16_t npdu_len = 0; /* return value */
    zsock_fd_set read_fds;
    int max = 0;
    struct zsock_timeval select_timeout;
    struct sockaddr_in sin = { 0 };
    BACNET_IP_ADDRESS addr = { { 0 } };
    socklen_t sin_len = sizeof(sin);
    int received_bytes = 0;
    int offset = 0;
    uint16_t i = 0;

    /* Make sure the socket is open */
    if (BIP_Socket < 0) {
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
    ZSOCK_FD_SET(BIP_Socket, &read_fds);
    max = BIP_Socket;

    /* see if there is a packet for us */
    if (zsock_select(max + 1, &read_fds, NULL, NULL, &select_timeout) > 0) {
        received_bytes = zsock_recvfrom(BIP_Socket, (char *)&npdu[0], max_npdu,
            0, (struct sockaddr *)&sin, &sin_len);
    }
    else 
    {
        return 0;
    }

    /* See if there is a problem */
    if (received_bytes < 0) {
        LOG_WRN("%s:%d - RX zsock_recvfrom() error: %d", THIS_FILE, __LINE__, received_bytes);
        return 0;
    }
    /* no problem, just no bytes */
    if (received_bytes == 0) {
        return 0;
    }
    /* the signature of a BACnet/IP packet */
    if (npdu[0] != BVLL_TYPE_BACNET_IP) {
        LOG_WRN("%s:%d - RX bad packet", THIS_FILE, __LINE__);
        return 0;
    }
    
    /* Data link layer addressing between B/IPv4 nodes consists of a 32-bit
       IPv4 address followed by a two-octet UDP port number (both of which
       shall be transmitted with the most significant octet first). This
       address shall be referred to as a B/IPv4 address.
    */
   
    memcpy(&addr.address[0], &sin.sin_addr.s_addr, IP_ADDRESS_MAX);
    addr.port = ntohs(sin.sin_port);
  
    debug_print_ipv4("Received MPDU->", &sin.sin_addr, sin.sin_port,
        received_bytes);
    /* pass the packet into the BBMD handler */
    offset = bvlc_handler(&addr, src, npdu, received_bytes);
    if (offset > 0) {
        npdu_len = received_bytes - offset;
        debug_print_ipv4("Received NPDU->", &sin.sin_addr, sin.sin_port,
            npdu_len);
        if (npdu_len <= max_npdu) {
            /* shift the buffer to return a valid NPDU */
            for (i = 0; i < npdu_len; i++) {
                npdu[i] = npdu[offset + i];
            }
        } else {
            LOG_WRN("%s:%d - NPDU dropped!", THIS_FILE, __LINE__);
            npdu_len = 0;
        }
    }

    return npdu_len;
}

/**
 * The common send function for BACnet/IP application layer
 *
 * @param dest - Points to a #BACNET_ADDRESS structure containing the
 *  destination address.
 * @param npdu_data - Points to a BACNET_NPDU_DATA structure containing the
 *  destination network layer control flags and data.
 * @param mtu - the bytes of data to send
 * @param mtu_len - the number of bytes of data to send
 * @return Upon successful completion, returns the number of bytes sent.
 *  Otherwise, -1 shall be returned and errno set to indicate the error.
 */
int bip_send_pdu(BACNET_ADDRESS *dest,
    BACNET_NPDU_DATA *npdu_data,
    uint8_t *pdu,
    unsigned pdu_len)
{
    dest->net = BACNET_BROADCAST_NETWORK;
    return bvlc_send_pdu(dest, npdu_data, pdu, pdu_len);
}

/** Gets the local IP address and local broadcast address from the system,
 *  and saves it into the BACnet/IP data structures.
 *
 * @param ifname [in] The named interface to use for the network layer.
 *        Eg, for Linux, ifname is eth0, ath0, arc0, and others.
 */
void bip_set_interface(char *ifname)
{
    struct net_if *interface = 0;
    int index = -1;
    uint8_t x=0;

    BACNET_IP_ADDRESS unicast = {0};
    BACNET_IP_ADDRESS broadcast = {0};

    /* Network byte order */
    unicast.port = BIP_Port;
    broadcast.port = BIP_Port;

    LOG_INF("bip_set_interface()");
    LOG_INF("UDP port: %d", ntohs(BIP_Port));

    if(ifname)
    {
        index = atoi(ifname);

        /* if index is zero, discern between "0" and a parse error */
        if(!index && strcmp(ifname,"0"))
        {
            LOG_ERR("%s:%d - Argument must parse to an integer", THIS_FILE, __LINE__);
        }
        else
        {
            interface = net_if_get_by_index(index);
            if(interface)
            {
                LOG_INF("Using interface %d", index);
            }
            else
            {
                LOG_ERR("%s:%d - No interface at index %d", THIS_FILE, __LINE__, index);
            }
        }
    }

    if(index == -1)
    {
        LOG_WRN("%s:%d - No valid interface specified - using default ",THIS_FILE, __LINE__);
        interface = net_if_get_default();
    }

    if(interface)
    {
        LOG_INF("Interface set");

        if(CONFIG_BACDL_BIP_ADDRESS_INDEX >= NET_IF_MAX_IPV4_ADDR)
        {
            LOG_ERR("%s:%d - IPv4 address index of %d is out of range (0-%d)",THIS_FILE, __LINE__, CONFIG_BACDL_BIP_ADDRESS_INDEX, NET_IF_MAX_IPV4_ADDR-1);
            return;
        }

        LOG_INF("Using IPv4 address at index %d", CONFIG_BACDL_BIP_ADDRESS_INDEX);

        /* Build the broadcast address from the unicast and netmask */
        for(x=0; x<IP_ADDRESS_MAX; x++)
        {
            broadcast.address[x] = interface->config.ip.ipv4->unicast[CONFIG_BACDL_BIP_ADDRESS_INDEX].address.in_addr.s4_addr[x] |
                ~interface->config.ip.ipv4->netmask.s4_addr[x];

            unicast.address[x] = interface->config.ip.ipv4->unicast[CONFIG_BACDL_BIP_ADDRESS_INDEX].address.in_addr.s4_addr[x];
        }

        bip_set_addr(&unicast);
        bip_set_broadcast_addr(&broadcast);

        /* net_if -> net_if_config . net_if_ip . net_if_ipv4 -> net_if_addr . net_addr . in_addr . s4_addr[4] */
        LOG_INF("   Unicast: %s", log_strdup(inet_ntoa(&interface->config.ip.ipv4->unicast->address.in_addr))); 
        LOG_INF(" Broadcast: %s", log_strdup(inet_ntoa(&BIP_Broadcast_Addr)));
        LOG_INF("   Netmask: %s", log_strdup(inet_ntoa(&interface->config.ip.ipv4->netmask)) );
    }
    else
    {
        LOG_ERR("%s:%d - Failed to set interface", THIS_FILE, __LINE__);
    }
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
 * @note For Zephyr, ifname is the index number of the interface as a string.
 *
 * @param ifname [in] The named interface to use for the network layer.
 *        If NULL, the default interface is assigned.
 * @return True if the socket is successfully opened for BACnet/IP,
 *        else False if the socket functions fail.
 */
bool bip_init(char *ifname)
{
    int sock_fd = -1;
    const int sockopt = 1;
    int status = -1;
    struct sockaddr_in sin = { 0 };

    bip_set_interface(ifname);

    if (BIP_Address.s_addr == 0) {
        LOG_ERR("%s:%d - Failed to get an IP address on interface: %s\n", THIS_FILE, __LINE__, log_strdup(ifname ? ifname : "[default]"));
        return false;
    }

    /* assumes that the driver has already been initialized */
    sock_fd = zsock_socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    BIP_Socket = sock_fd;
    if (sock_fd < 0) {
        LOG_ERR("%s:%d - Failed to create socket", THIS_FILE, __LINE__);
        return false;
    }
    else
    {
        LOG_DBG("Socket created");
    }

    /* Allow us to use the same socket for sending and receiving */
    /* This makes sure that the src port is correct when sending */
    status = zsock_setsockopt(
        sock_fd, SOL_SOCKET, SO_REUSEADDR, &sockopt, sizeof(sockopt));
    if (status < 0) {
        zsock_close(sock_fd);
        BIP_Socket = -1;
        return false;
    }

    /* bind the socket to the local port number and IP address */
    sin.sin_family = AF_INET;
    sin.sin_addr.s_addr = htonl(INADDR_ANY);
    sin.sin_port = BIP_Port;

    status =
        zsock_bind(sock_fd, (const struct sockaddr *)&sin, sizeof(struct sockaddr));
    if (status < 0) {
        zsock_close(sock_fd);
        BIP_Socket = -1;
        LOG_ERR("%s:%d - zsock_bind() failure", THIS_FILE, __LINE__);
        return false;
    }
    else
    {
        LOG_DBG("Socket bound");
    }

    bvlc_init();

    LOG_DBG("bip_init() success");
    return true;
}

/**
 * @brief Determine if this BACnet/IP datalink is valid
 * @return true if the BACnet/IP datalink is valid
 */
bool bip_valid(void)
{
    return (BIP_Socket != -1);
}

/** Cleanup and close out the BACnet/IP services by closing the socket.
 * @ingroup DLBIP
 */
void bip_cleanup(void)
{
    LOG_DBG("bip_cleanup()");

    BIP_Port = 0;
    memset(&BIP_Address, 0, sizeof(BIP_Address));
    memset(&BIP_Broadcast_Addr, 0, sizeof(BIP_Broadcast_Addr));

    if (BIP_Socket != -1) {
        zsock_close(BIP_Socket);
    }
    BIP_Socket = -1;

    return;
}
