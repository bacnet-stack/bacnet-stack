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
#include <device.h>
#include <init.h>
#include <kernel.h>
#include <sys/printk.h>
#include <net/net_ip.h>
#include <net/socket.h>
#include <net/socket_select.h>
#include "bacnet/bacdcode.h"
#include "bacnet/bacint.h"
#include "bacnet/datalink/bip6.h"
#include "bacnet/basic/sys/debug.h"
#include "bacnet/basic/bbmd6/h_bbmd6.h"

/* Logging module registration is already done in ports/zephyr/main.c */
#include <logging/log.h>
#include <logging/log_ctrl.h>

LOG_MODULE_DECLARE(bacnet, CONFIG_BACNETSTACK_LOG_LEVEL);

#define THIS_FILE "bip6-init.c"

#if (MAX_MAC_LEN < BIP6_ADDRESS_MAX) /* Make sure an 18 byte address can be stored */
#error "BACNET_ADDRESS.mac (bacdef.h) is not large enough to store an IPv6 address."
#endif

/* zephyr socket */
static int BIP6_Socket = -1;

/* NOTE: we store address and port in network byte order
   since BACnet/IP uses network byte order for all address byte arrays
*/
/* port to use - stored here in network byte order */
static uint16_t BIP6_Port = htons(CONFIG_BACDL_BIP6_PORT);
/* IP address - stored here in network byte order */
static struct in6_addr BIP6_Address;
/* IP multicast address - stored here in network byte order */
static struct in6_addr BIP6_Multicast_Addr;

/* Used by inet6_ntoa */
#if CONFIG_BACNETSTACK_LOG_LEVEL
static char ipv6_addr_str[42]={0};
#else
static char ipv6_addr_str[]="";
#endif

/**
* @brief Return a string representation of an IPv6 address
* @param a - IPv6 address
* @return Pointer to global string
*/
static char* inet6_ntoa(struct in6_addr *a)
{
    #if CONFIG_BACNETSTACK_LOG_LEVEL
    uint8_t x=0;
    uint8_t d=0;
    uint8_t non_zero_count = 0;

    /* Avoid overwhelming the logging system */
    while(log_buffered_cnt())
    {  
        k_sleep(K_MSEC(1));
    }

    for(x=0; x<IP6_ADDRESS_MAX; x+=2)
    {
        if(a->s6_addr[x] | a->s6_addr[x+1])
        {
            non_zero_count++;
            d+=sprintf(&ipv6_addr_str[d], "%02X%02X", a->s6_addr[x], a->s6_addr[x+1]);
        }

        if(x<14)
        {
            d+=sprintf(&ipv6_addr_str[d], ":");
        }
    }

    if(!non_zero_count)
    {
        sprintf(&ipv6_addr_str[0], "undefined");
    }
    #endif
    return &ipv6_addr_str[0];
}

/**
 * @brief Set the BACnet IPv6 UDP port number
 * @param port - IPv6 UDP port number - in host byte order
 */
void bip6_set_port(uint16_t port)
{
    BIP6_Port = htons(port);
}

/**
 * @brief Get the BACnet IPv6 UDP port number
 * @return IPv4 UDP port number - in host byte order
 */
uint16_t bip6_get_port(void)
{
    return ntohs(BIP6_Port);
}

/**
 * @brief Get the IPv6 address for my interface. Used for sending src address.
 * @param addr - BACnet datalink address
 */
void bip6_get_my_address(BACNET_ADDRESS *addr)
{
    unsigned int i = 0;

    if (addr) {
        addr->mac_len = BIP6_ADDRESS_MAX; /* 18 */
        memcpy(&addr->mac[0], &BIP6_Address.s6_addr, IP6_ADDRESS_MAX); /* 16 */
        memcpy(&addr->mac[IP6_ADDRESS_MAX], &BIP6_Port, sizeof(BIP6_Port));
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
 * Get the IPv6 broadcast address for my interface.
 *
 * @param addr - BACnet datalink address
 */
void bip6_get_broadcast_address(BACNET_ADDRESS *dest)
{
    /* BIP6_ADDRESS_MAX = 18 */
    /*  IP6_ADDRESS_MAX = 16 */

    /* Store IPv6 address and port in dest->mac and
       clear dest->adr which is used for hardware MAC */

    if (dest) {
        dest->mac_len = BIP6_ADDRESS_MAX;
        memcpy(&dest->mac[0], &BIP6_Multicast_Addr.s6_addr, IP6_ADDRESS_MAX);
        memcpy(&dest->mac[IP6_ADDRESS_MAX], &BIP6_Port, sizeof(BIP6_Port) );
        memset(&dest->adr, 0, sizeof(dest->adr));
        dest->net = BACNET_BROADCAST_NETWORK;
        dest->len = 0;
    }
    return;
}

/**
 * Set the BACnet/IP address
 *
 * @param addr - network IPv6 address
 */
bool bip6_set_addr(BACNET_IP6_ADDRESS *addr)
{
    if (addr) {
        memcpy(&BIP6_Address.s6_addr, &addr->address[0], IP6_ADDRESS_MAX);
        memcpy(&BIP6_Port, &addr->port, sizeof(addr->port));
        return true;
    }
    return false;
}

/**
 * @brief Get the BACnet/IP address
 * @param addr - network IPv6 address
 * @return true if the address was retrieved
 */
bool bip6_get_addr(BACNET_IP6_ADDRESS *addr)
{
    if (addr) {
        memcpy(&addr->address[0], &BIP6_Address, IP6_ADDRESS_MAX);
        memcpy(&addr->port, &BIP6_Port, sizeof(addr->port));
        return true;
    }
    return false;
}

/**
 * @brief Set the BACnet/IP address
 * @param addr - network IPv6 address
 * @return true if the address was set
 */
bool bip6_set_broadcast_addr(BACNET_IP6_ADDRESS *addr)
{
    if (addr) {
        memcpy(&BIP6_Multicast_Addr.s6_addr, &addr->address[0], IP6_ADDRESS_MAX);
        return true;
    }
    return false;
}

/**
 * Get the BACnet/IP address
 *
 * @return BACnet/IP address
 */
bool bip6_get_broadcast_addr(BACNET_IP6_ADDRESS *addr)
{
    if (addr) {
        memcpy(&addr->address[0], &BIP6_Multicast_Addr.s6_addr, IP6_ADDRESS_MAX);
        addr->port = ntohs(BIP6_Port);
        return true;
    }
    return false;
}

/**
 * @brief Set the BACnet/IP subnet mask CIDR prefix
 * @return true if the subnet mask CIDR prefix is set
 */
bool bip6_set_subnet_prefix(uint8_t prefix)
{
    /* not something we do within this driver */
    return false;
}

/**
 * @brief Set the BACnet/IP subnet mask CIDR prefix
 * @return true if the subnet mask CIDR prefix is set
 */
uint8_t bip6_get_subnet_prefix(void)
{
    /* not something we do within this driver */
    return 0;
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
int bip6_send_mpdu(BACNET_IP6_ADDRESS *dest, uint8_t *mtu, uint16_t mtu_len)
{
    struct sockaddr_in6 bip6_dest = { 0 };

    /* assumes that the driver has already been initialized */
    if (BIP6_Socket < 0) {
        LOG_ERR("%s:%d - Socket not initialized!", THIS_FILE, __LINE__);
        return BIP6_Socket;
    }

    /* load destination IP address */
    bip6_dest.sin6_family = AF_INET6;
    memcpy(&bip6_dest.sin6_addr.s6_addr, &dest->address[0], IP6_ADDRESS_MAX);
    bip6_dest.sin6_port = htons(dest->port);

    /* Send the packet */
    return zsock_sendto(BIP6_Socket, (char *)mtu, mtu_len, 0,
        (struct sockaddr *)&bip6_dest, sizeof(struct sockaddr));
}

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
        received_bytes = zsock_recvfrom(BIP6_Socket, (char *)&npdu[0], max_npdu,
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
    /* the signature of a BACnet/IPv6 packet */
    if (npdu[0] != BVLL_TYPE_BACNET_IP6) {
        LOG_WRN("%s:%d - RX bad packet", THIS_FILE, __LINE__);
        return 0;
    }

    /* Data link layer addressing between B/IPv6 nodes consists of a 128-bit
       IPv6 address followed by a two-octet UDP port number (both of which
       shall be transmitted with the most significant octet first). This
       address shall be referred to as a B/IPv6 address.
    */

    memcpy(&addr.address[0], &sin.sin6_addr.s6_addr, IP6_ADDRESS_MAX);
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
            LOG_WRN("%s:%d - NPDU dropped!", THIS_FILE, __LINE__);
            npdu_len = 0;
        }
    }
    return npdu_len;
}

int bip6_send_pdu(BACNET_ADDRESS *dest,
    BACNET_NPDU_DATA *npdu_data,
    uint8_t *pdu,
    unsigned pdu_len)
{
    dest->net = BACNET_BROADCAST_NETWORK;
    return bvlc6_send_pdu(dest, npdu_data, pdu, pdu_len);
}

void bip6_set_interface(char *ifname)
{
    struct net_if *interface = 0;
    int index = -1;
    int x=0;

    BACNET_IP6_ADDRESS unicast = {0};
    BACNET_IP6_ADDRESS multicast = {0};

    unicast.port = BIP6_Port;
    multicast.port = BIP6_Port;

    LOG_INF("bip6_set_interface()");
    LOG_INF("UDP port: %d", ntohs(BIP6_Port));

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
        LOG_INF("Interface set - Configured addresses:");

        for(x=0; x<NET_IF_MAX_IPV6_ADDR; x++)
        {
            inet6_ntoa(&interface->config.ip.ipv6->unicast[x].address.in6_addr );
            LOG_INF("  unicast[%d]: %s", x, log_strdup(ipv6_addr_str));
        }

        for(x=0; x<NET_IF_MAX_IPV6_MADDR; x++)
        {
            inet6_ntoa(&interface->config.ip.ipv6->mcast[x].address.in6_addr );
            LOG_INF(" multicast[%d]: %s", x, log_strdup(ipv6_addr_str));
        }

        if(CONFIG_BACDL_BIP6_ADDRESS_INDEX >= NET_IF_MAX_IPV6_ADDR)
        {
            LOG_ERR("%s:%d - IPv6 address index of %d is out of range (0-%d)", THIS_FILE,
                __LINE__, CONFIG_BACDL_BIP6_ADDRESS_INDEX, NET_IF_MAX_IPV6_ADDR-1);
            return;
        }

        LOG_INF("Using IPv6 address at index %d", CONFIG_BACDL_BIP6_ADDRESS_INDEX);

        memcpy(&unicast.address, &interface->config.ip.ipv6->unicast
            [CONFIG_BACDL_BIP6_ADDRESS_INDEX].address.in6_addr, IP6_ADDRESS_MAX);
    
        if(net_addr_pton(AF_INET6, CONFIG_BACDL_BIP6_MCAST_ADDRESS, &multicast.address))
        {
            LOG_ERR("%s:%d - Failed to parse IPv6 multicast address: %s", THIS_FILE, __LINE__, CONFIG_BACDL_BIP6_MCAST_ADDRESS);
        }

        bip6_set_addr(&unicast);
        bip6_set_broadcast_addr(&multicast);

        LOG_INF("   Unicast: %s", log_strdup(inet6_ntoa((struct in6_addr*)&unicast.address)));
        LOG_INF(" Multicast: %s", log_strdup(inet6_ntoa((struct in6_addr*)&multicast.address)));
    }
    else
    {
        LOG_ERR("%s:%d - Failed to set interface", THIS_FILE, __LINE__);
    }
}


bool bip6_init(char *ifname)
{
    LOG_INF("bip6_init()");

    int sock_fd = -1;
    const int sockopt = 1;
    int status = -1;
    struct sockaddr_in6 sin6 = { 0 };

    bip6_set_interface(ifname);

    if (BIP6_Address.s6_addr == 0) {
        LOG_ERR("%s:%d - Failed to get an IPv6 address on interface: %s\n", THIS_FILE, __LINE__, log_strdup(ifname ? ifname : "[default]"));
        return false;
    }

    /* assumes that the driver has already been initialized */
    sock_fd = zsock_socket(AF_INET6, SOCK_DGRAM, IPPROTO_UDP);
    BIP6_Socket = sock_fd;
     if (sock_fd < 0) {
        LOG_ERR("%s:%d - Failed to create socket", THIS_FILE, __LINE__);
        return false;
    }
    else
    {
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

    /* bind the socket to the local port number and IP address */
    sin6.sin6_family = AF_INET6;
    sin6.sin6_addr = in6addr_any;
    sin6.sin6_port = BIP6_Port;

    LOG_INF("Binding to port %d", ntohs(BIP6_Port));

    status =
        zsock_bind(sock_fd, (const struct sockaddr*)&sin6, sizeof(struct sockaddr));
    if (status < 0) {
        zsock_close(sock_fd);
        BIP6_Socket = -1;
        LOG_ERR("%s:%d - zsock_bind() failure", THIS_FILE, __LINE__);
        return false;
    }
    else
    {
        LOG_INF("Socket bound");
    }

    bvlc6_init();

    LOG_INF("bip6_init() success");
    return true;
}

bool bip6_valid(void)
{
    return (BIP6_Socket != -1);
}

void bip6_cleanup(void)
{
    LOG_INF("bip6_cleanup()");

    BIP6_Port = 0;
    memset(&BIP6_Address, 0, sizeof(BIP6_Address));
    memset(&BIP6_Multicast_Addr, 0, sizeof(BIP6_Multicast_Addr));

    if (BIP6_Socket != -1) {
        zsock_close(BIP6_Socket);
    }
    BIP6_Socket = -1;

    return;
}
