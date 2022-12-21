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
/* linux Ethernet/IP specific */
#include <asm/types.h>
#include <netinet/ether.h>
#include <netinet/in.h>
#include <net/if.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <linux/netlink.h>
#include <linux/rtnetlink.h>
#include <sys/types.h>
#include <unistd.h>
/* standard C */
#include <stdint.h> /* for standard integer types uint8_t etc. */
#include <stdbool.h> /* for the standard bool type. */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
/* BACnet specific */
#include "bacnet/bacdcode.h"
#include "bacnet/bacint.h"
#include "bacnet/datalink/bip.h"
#include "bacnet/basic/sys/debug.h"
#include "bacnet/basic/bbmd/h_bbmd.h"
#include "bacport.h"

/** @file linux/bip-init.c  Initializes BACnet/IP interface (Linux). */

/* unix sockets */
static int BIP_Socket = -1;
static int BIP_Broadcast_Socket = -1;

/* NOTE: we store address and port in network byte order
   since BACnet/IP uses network byte order for all address byte arrays
*/
/* port to use - stored here in network byte order */
/* Initialize to 0 - this will force initialization in demo apps */
static uint16_t BIP_Port;
/* IP address - stored here in network byte order */
static struct in_addr BIP_Address;
/* IP broadcast address - stored here in network byte order */
static struct in_addr BIP_Broadcast_Addr;
/* enable debugging */
static bool BIP_Debug = false;
/* interface name */
static char BIP_Interface_Name[IF_NAMESIZE] = { 0 };

/**
 * @brief Print the IPv4 address with debug info
 * @param str - debug info string
 * @param addr - IPv4 address
 */
static void debug_print_ipv4(const char *str,
    const struct in_addr *addr,
    const unsigned int port,
    const unsigned int count)
{
    if (BIP_Debug) {
        fprintf(stderr, "BIP: %s %s:%hu (%u bytes)\n", str, inet_ntoa(*addr),
            ntohs(port), count);
        fflush(stderr);
    }
}

/**
 * @brief Return the active BIP socket.
 * @return The active BIP socket, or -1 if uninitialized.
 * @see bip_get_broadcast_socket
 */
int bip_get_socket(void)
{
    return BIP_Socket;
}

/**
 * @brief Return the active BIP Broadcast socket.
 * @return The active BIP Broadcast socket, or -1 if uninitialized.
 * @see bip_get_socket
 */
int bip_get_broadcast_socket(void)
{
    return BIP_Broadcast_Socket;
}

/**
 * @brief Enabled debug printing of BACnet/IPv4
 */
void bip_debug_enable(void)
{
    BIP_Debug = true;
}

/**
 * @brief Disalbe debug printing of BACnet/IPv4
 */
void bip_debug_disable(void)
{
    BIP_Debug = false;
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
        addr->mac_len = 6;
        memcpy(&addr->mac[0], &BIP_Address.s_addr, 4);
        memcpy(&addr->mac[4], &BIP_Port, 2);
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
    int i = 0; /* counter */

    if (dest) {
        dest->mac_len = 6;
        memcpy(&dest->mac[0], &BIP_Broadcast_Addr.s_addr, 4);
        memcpy(&dest->mac[4], &BIP_Port, 2);
        dest->net = BACNET_BROADCAST_NETWORK;
        dest->len = 0; /* no SLEN */
        for (i = 0; i < MAX_MAC_LEN; i++) {
            /* no SADR */
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
    /* not something we do within this driver */
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
        memcpy(&addr->address[0], &BIP_Address.s_addr, 4);
        addr->port = ntohs(BIP_Port);
    }

    return true;
}

/**
 * @brief Set the BACnet/IP address
 * @param addr - network IPv4 address
 * @return true if the address was set
 */
bool bip_set_broadcast_addr(BACNET_IP_ADDRESS *addr)
{
    /* not something we do within this driver */
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
        memcpy(&addr->address[0], &BIP_Broadcast_Addr.s_addr, 4);
        addr->port = ntohs(BIP_Port);
    }

    return true;
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
        mask = mask << 1;
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
        if (BIP_Debug) {
            fprintf(stderr, "BIP: driver not initialized!\n");
            fflush(stderr);
        }
        return BIP_Socket;
    }
    /* load destination IP address */
    bip_dest.sin_family = AF_INET;
    memcpy(&bip_dest.sin_addr.s_addr, &dest->address[0], 4);
    bip_dest.sin_port = htons(dest->port);
    /* Send the packet */
    debug_print_ipv4(
        "Sending MPDU->", &bip_dest.sin_addr, bip_dest.sin_port, mtu_len);
    return sendto(BIP_Socket, (char *)mtu, mtu_len, 0,
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
    fd_set read_fds;
    int max = 0;
    struct timeval select_timeout;
    struct sockaddr_in sin = { 0 };
    BACNET_IP_ADDRESS addr = { { 0 } };
    socklen_t sin_len = sizeof(sin);
    int received_bytes = 0;
    int offset = 0;
    uint16_t i = 0;
    int socket;

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
    FD_ZERO(&read_fds);
    FD_SET(BIP_Socket, &read_fds);
    FD_SET(BIP_Broadcast_Socket, &read_fds);

    max = BIP_Socket > BIP_Broadcast_Socket ? BIP_Socket : BIP_Broadcast_Socket;

    /* see if there is a packet for us */
    if (select(max + 1, &read_fds, NULL, NULL, &select_timeout) > 0) {
        socket = FD_ISSET(BIP_Socket, &read_fds) ? BIP_Socket :
            BIP_Broadcast_Socket;
        received_bytes = recvfrom(socket, (char *)&npdu[0], max_npdu, 0,
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
    /* the signature of a BACnet/IPv packet */
    if (npdu[0] != BVLL_TYPE_BACNET_IP) {
        return 0;
    }
    /* Erase up to 16 bytes after the received bytes as safety margin to
     * ensure that the decoding functions will run into a 'safe field'
     * of zero, if for any reason they would overrun, when parsing the
     * message. */
    max = (int)max_npdu - received_bytes;
    if (max > 0) {
        if (max > 16) {
            max = 16;
        }
        memset(&npdu[received_bytes], 0, max);
    }
    /* Data link layer addressing between B/IPv4 nodes consists of a 32-bit
       IPv4 address followed by a two-octet UDP port number (both of which
       shall be transmitted with the most significant octet first). This
       address shall be referred to as a B/IPv4 address.
    */
    memcpy(&addr.address[0], &sin.sin_addr.s_addr, 4);
    addr.port = ntohs(sin.sin_port);
    debug_print_ipv4(
        "Received MPDU->", &sin.sin_addr, sin.sin_port, received_bytes);
    /* pass the packet into the BBMD handler */
    offset = socket == BIP_Socket ?
        bvlc_handler(&addr, src, npdu, received_bytes) :
        bvlc_broadcast_handler(&addr, src, npdu, received_bytes);
    if (offset > 0) {
        npdu_len = received_bytes - offset;
        debug_print_ipv4(
            "Received NPDU->", &sin.sin_addr, sin.sin_port, npdu_len);
        if (npdu_len <= max_npdu) {
            /* shift the buffer to return a valid NPDU */
            for (i = 0; i < npdu_len; i++) {
                npdu[i] = npdu[offset + i];
            }
        } else {
            if (BIP_Debug) {
                fprintf(stderr, "BIP: NPDU dropped!\n");
                fflush(stderr);
            }
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
    return bvlc_send_pdu(dest, npdu_data, pdu, pdu_len);
}

/**
 * @brief gets an IP address by hostname (or string of numbers)
 *
 * gets an IP address by name, where name can be a string that is an
 * IP address in dotted form, or a name that is a domain name
 *
 * @param host_name - the host name
 * @return true if the address was retrieved
 */
bool bip_get_addr_by_name(const char *host_name, BACNET_IP_ADDRESS *addr)
{
    struct hostent *host_ent;

    if ((host_ent = gethostbyname(host_name)) == NULL) {
        return false;
    }

    if (addr) {
        /* Host addresses in a struct hostent structure are always
           given in network byte order */
        /* h_addr: This is a synonym for h_addr_list[0];
           in other words, it is the first host address.*/
        memcpy(&addr->address[0], host_ent->h_addr, 4);
    }

    return true;
}

/**
 * @brief Issue a specific request for an interface via an ioctl() call.
 * @param ifname - the interface name
 * @param ifr - interface request
 * @param request - the ioctl() request
 * @return 0 on success, else the error from the ioctl() call.
 */
static int get_local_ifr_ioctl(char *ifname, struct ifreq *ifr, int request)
{
    int fd;
    int rv; /* return value */

    strncpy(ifr->ifr_name, ifname, sizeof(ifr->ifr_name) - 1);
    ifr->ifr_name[sizeof(ifr->ifr_name) - 1] = 0;

    fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);
    if (fd < 0) {
        rv = fd;
    } else {
        rv = ioctl(fd, request, ifr);
        close(fd);
    }

    return rv;
}

/**
 * @brief Issue a specific request foor an interface via an ioctl() call.
 * @param ifname - the interface name
 * @param addr [out] the address in host order.
 * @param request - the ioctl() request
 * @return 0 on success, else the error from the ioctl() call.
 */
int bip_get_local_address_ioctl(char *ifname, struct in_addr *addr, int request)
{
    struct ifreq ifr = { { { 0 } }, { { 0 } } };
    struct sockaddr_in *tcpip_address;
    int rv; /* return value */

    rv = get_local_ifr_ioctl(ifname, &ifr, request);
    if (rv >= 0) {
        tcpip_address = (struct sockaddr_in *)&ifr.ifr_addr;
        memcpy(addr, &tcpip_address->sin_addr, sizeof(struct in_addr));
    }

    return rv;
}

/* structure to hold IPv4 route info when dynamically finding interface */
struct route_info {
    uint32_t dstAddr;
    uint32_t srcAddr;
    uint32_t gateWay;
    char ifName[IF_NAMESIZE];
};

/**
 * @brief Read the Network Layer socket info
 * @param sockFd - socket file descriptor
 * @param bufPtr - buffer to hold response from kernel
 * @param buf_size - size of the buffer
 * @param seqNum - sequence number that tracks our specific request response
 * @param pId - process identifier of our specific request response
 * @return number of bytes placed into the buffer
 */
static int readNlSock(
    int sockFd, char *bufPtr, size_t buf_size, int seqNum, int pId)
{
    struct nlmsghdr *nlHdr;
    int readLen = 0, msgLen = 0;

    do {
        /* Receive response from the kernel */
        if ((readLen = recv(sockFd, bufPtr, buf_size - msgLen, 0)) < 0) {
            perror("SOCK READ: ");
            return -1;
        }
        if (readLen == 0) {
            /* no data in the response */
            break;
        }
        nlHdr = (struct nlmsghdr *)bufPtr;
        /* Check if the header is valid */
        if ((0 == NLMSG_OK(nlHdr, readLen)) ||
            (NLMSG_ERROR == nlHdr->nlmsg_type)) {
            perror("Error in received packet");
            return -1;
        }
        /* Check if it is the last message */
        if (NLMSG_DONE == nlHdr->nlmsg_type) {
            break;
        }
        /* Else move the pointer to buffer appropriately */
        bufPtr += readLen;
        msgLen += readLen;
        /* Check if its a multi part message; return if it is not. */
        if (0 == (nlHdr->nlmsg_flags & NLM_F_MULTI)) {
            break;
        }
    } while ((nlHdr->nlmsg_seq != seqNum) || (nlHdr->nlmsg_pid != pId));

    return msgLen;
}

/**
 * @brief Convert an IPv4 address into ASCII dotted decimal
 * @param addr - Route info
 * @return character string of ASCII dotted decimal
 */
static char *ntoa(uint32_t addr)
{
    static char buffer[18];

    sprintf(buffer, "%d.%d.%d.%d", (addr & 0x000000FF),
        (addr & 0x0000FF00) >> 8, (addr & 0x00FF0000) >> 16,
        (addr & 0xFF000000) >> 24);

    return buffer;
}

/**
 * @brief Printing one route
 * @param rtInfo - Route info
 */
static void printRoute(struct route_info *rtInfo)
{
    if (BIP_Debug) {
        /* Print Destination address */
        fprintf(stderr, "%s\t",
            rtInfo->dstAddr ? ntoa(rtInfo->dstAddr) : "0.0.0.0  ");

        /* Print Gateway address */
        fprintf(stderr, "%s\t",
            rtInfo->gateWay ? ntoa(rtInfo->gateWay) : "*.*.*.*");

        /* Print Interface Name */
        fprintf(stderr, "%s\t", rtInfo->ifName);

        /* Print Source address */
        fprintf(stderr, "%s\n",
            rtInfo->srcAddr ? ntoa(rtInfo->srcAddr) : "*.*.*.*");
    }
}

/**
 * @brief Parse the route info returned from kernel
 * @param nlHdr - Network Layer message header from the kernel
 * @param rtInfo - Route info
 */
static void parseRoutes(struct nlmsghdr *nlHdr, struct route_info *rtInfo)
{
    struct rtmsg *rtMsg;
    struct rtattr *rtAttr;
    int rtLen;

    rtMsg = (struct rtmsg *)NLMSG_DATA(nlHdr);

    /* If the route is not for AF_INET or does not belong to main routing table
    then return. */
    if ((rtMsg->rtm_family != AF_INET) || (rtMsg->rtm_table != RT_TABLE_MAIN))
        return;

    /* get the rtattr field */
    rtAttr = (struct rtattr *)RTM_RTA(rtMsg);
    rtLen = RTM_PAYLOAD(nlHdr);
    for (; RTA_OK(rtAttr, rtLen); rtAttr = RTA_NEXT(rtAttr, rtLen)) {
        switch (rtAttr->rta_type) {
            case RTA_OIF:
                if_indextoname(*(int *)RTA_DATA(rtAttr), rtInfo->ifName);
                break;
            case RTA_GATEWAY:
                rtInfo->gateWay = *(u_int *)RTA_DATA(rtAttr);
                break;
            case RTA_PREFSRC:
                rtInfo->srcAddr = *(u_int *)RTA_DATA(rtAttr);
                break;
            case RTA_DST:
                rtInfo->dstAddr = *(u_int *)RTA_DATA(rtAttr);
                break;
        }
    }
}

/**
 * @brief Get the default interface name using routing info
 * @return interface name, or NULL if not found or none
 */
static char *ifname_default(void)
{
    struct nlmsghdr *nlMsg = NULL;
    struct route_info *rtInfo = NULL;
    char msgBuf[8192] = { 0 };
    int sock, len, msgSeq = 0;

    if (BIP_Interface_Name[0] != 0) {
        return BIP_Interface_Name;
    }
    /* Create Socket */
    if ((sock = socket(PF_NETLINK, SOCK_DGRAM, NETLINK_ROUTE)) < 0) {
        perror("Socket Creation: ");
    }
    /* point the header and the msg structure pointers into the buffer */
    nlMsg = (struct nlmsghdr *)msgBuf;
    /* Fill in the nlmsg header*/
    nlMsg->nlmsg_len = NLMSG_LENGTH(sizeof(struct rtmsg));
    /* Get the routes from kernel routing table */
    nlMsg->nlmsg_type = RTM_GETROUTE;
    /* The message is a request for dump */
    nlMsg->nlmsg_flags = NLM_F_DUMP | NLM_F_REQUEST;
    /* Sequence of the message packet */
    nlMsg->nlmsg_seq = msgSeq++;
    /* PID of process sending the request */
    nlMsg->nlmsg_pid = getpid();

    /* Send the request */
    if (send(sock, nlMsg, nlMsg->nlmsg_len, 0) < 0) {
        fprintf(stderr, "BIP: Write To Socket Failed...\n");
        return BIP_Interface_Name;
    }
    /* Read the response */
    if ((len = readNlSock(sock, msgBuf, sizeof(msgBuf), msgSeq, getpid())) <
        0) {
        fprintf(stderr, "BIP: Read From Socket Failed...\n");
        return BIP_Interface_Name;
    }
    /* Parse and print the response */
    rtInfo = (struct route_info *)malloc(sizeof(struct route_info));
    if (BIP_Debug) {
        fprintf(stderr, "Destination\tGateway\tInterface\tSource\n");
    }
    for (; NLMSG_OK(nlMsg, len); nlMsg = NLMSG_NEXT(nlMsg, len)) {
        memset(rtInfo, 0, sizeof(struct route_info));
        parseRoutes(nlMsg, rtInfo);
        printRoute(rtInfo);
        if (BIP_Interface_Name[0] == 0) {
            if ((rtInfo->dstAddr == 0) && (rtInfo->ifName[0] != 0)) {
                /* default route */
                memcpy(BIP_Interface_Name, rtInfo->ifName,
                    sizeof(BIP_Interface_Name));
            }
        }
    }
    free(rtInfo);
    close(sock);

    return BIP_Interface_Name;
}

/**
 * @brief Get the netmask of the BACnet/IP's interface via an ioctl() call.
 * @param netmask [out] The netmask, in host order.
 * @return 0 on success, else the error from the ioctl() call.
 */
int bip_get_local_netmask(struct in_addr *netmask)
{
    int rv;
    char *ifname = getenv("BACNET_IFACE");

    if (ifname == NULL) {
        ifname = ifname_default();
    }
    rv = bip_get_local_address_ioctl(ifname, netmask, SIOCGIFNETMASK);

    return rv;
}

/** Gets the local IP address and local broadcast address from the system,
 *  and saves it into the BACnet/IP data structures.
 *
 * @param ifname [in] The named interface to use for the network layer.
 *        Eg, for Linux, ifname is eth0, ath0, arc0, and others.
 */
void bip_set_interface(char *ifname)
{
    struct in_addr local_address;
    struct in_addr netmask;
    int rv = 0;

    /* setup local address */
    rv = bip_get_local_address_ioctl(ifname, &local_address, SIOCGIFADDR);
    if (rv < 0) {
        local_address.s_addr = 0;
    }
    BIP_Address.s_addr = local_address.s_addr;
    if (BIP_Debug) {
        fprintf(stderr, "BIP: Interface: %s\n", ifname);
        fprintf(stderr, "BIP: Address: %s\n", inet_ntoa(local_address));
        fflush(stderr);
    }
    /* setup local broadcast address */
    rv = bip_get_local_address_ioctl(ifname, &netmask, SIOCGIFNETMASK);
    if (rv < 0) {
        BIP_Broadcast_Addr.s_addr = ~0;
    } else {
        BIP_Broadcast_Addr = local_address;
        BIP_Broadcast_Addr.s_addr |= (~netmask.s_addr);
    }
    if (BIP_Debug) {
        fprintf(stderr, "BIP: Broadcast Address: %s\n",
            inet_ntoa(BIP_Broadcast_Addr));
        fprintf(stderr, "BIP: UDP Port: 0x%04X [%hu]\n", ntohs(BIP_Port),
            ntohs(BIP_Port));
        fflush(stderr);
    }
}

static int createSocket(struct sockaddr_in *sin)
{
    int status = 0; /* return from socket lib calls */
    int sockopt = 0;
    int sock_fd = -1;

    /* assumes that the driver has already been initialized */
    sock_fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sock_fd < 0) {
        return sock_fd;
    }
    /* Allow us to use the same socket for sending and receiving */
    /* This makes sure that the src port is correct when sending */
    sockopt = 1;
    status = setsockopt(
        sock_fd, SOL_SOCKET, SO_REUSEADDR, &sockopt, sizeof(sockopt));
    if (status < 0) {
        close(sock_fd);
        return status;
    }
    /* allow us to send a broadcast */
    status = setsockopt(
        sock_fd, SOL_SOCKET, SO_BROADCAST, &sockopt, sizeof(sockopt));
    if (status < 0) {
        close(sock_fd);
        return status;
    }
    /* Bind to the proper interface to send without default gateway */
    setsockopt(sock_fd, SOL_SOCKET, SO_BINDTODEVICE, BIP_Interface_Name,
        strlen(BIP_Interface_Name));

    /* bind the socket to the local port number and IP address */
    status =
        bind(sock_fd, (const struct sockaddr *)sin, sizeof(struct sockaddr));
    if (status < 0) {
        close(sock_fd);
        return status;
    }

    return sock_fd;
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
 * @param ifname [in] The named interface to use for the network layer.
 *        If NULL, the default interface is assigned.
 * @return True if the socket is successfully opened for BACnet/IP,
 *         else False if the socket functions fail.
 */
bool bip_init(char *ifname)
{
    struct sockaddr_in sin;
    int sock_fd = -1;

    if (ifname) {
        strncpy(BIP_Interface_Name, ifname, sizeof(BIP_Interface_Name));
        bip_set_interface(ifname);
    } else {
        bip_set_interface(ifname_default());
    }
    if (BIP_Address.s_addr == 0) {
        fprintf(stderr, "BIP: Failed to get an IP address from %s!\n",
            BIP_Interface_Name);
        fflush(stderr);
        return false;
    }

    sin.sin_family = AF_INET;
    sin.sin_port = BIP_Port;
    memset(&(sin.sin_zero), '\0', sizeof(sin.sin_zero));

    sin.sin_addr.s_addr = BIP_Address.s_addr;
    sock_fd = createSocket(&sin);
    BIP_Socket = sock_fd;
    if (sock_fd < 0) {
        return false;
    }

    sin.sin_addr.s_addr = htonl(INADDR_ANY);
    sock_fd = createSocket(&sin);
    BIP_Broadcast_Socket = sock_fd;
    if (sock_fd < 0) {
        return false;
    }

    bvlc_init();

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
    if (BIP_Socket != -1) {
        close(BIP_Socket);
    }
    BIP_Socket = -1;

    if (BIP_Broadcast_Socket != -1) {
        close(BIP_Broadcast_Socket);
    }
    BIP_Broadcast_Socket = -1;

    return;
}
