/*####COPYRIGHTBEGIN####
 -------------------------------------------
 Copyright (C) 2016 Steve Karg

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

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h> /* for standard integer types uint8_t etc. */
#include <stdbool.h> /* for the standard bool type. */
#include "bacnet/bacdcode.h"
#include "bacnet/config.h"
#include "bacnet/basic/sys/debug.h"
#include "bacnet/basic/object/device.h"
#include "bacnet/basic/bbmd6/h_bbmd6.h"
#include "bacnet/datalink/bip6.h"
#include "bacport.h"

/* Win32 Socket */
static SOCKET BIP6_Socket = INVALID_SOCKET;
static int BIP6_Socket_Scope_Id = 0;
/* local address - filled by init functions */
static BACNET_IP6_ADDRESS BIP6_Addr;
static BACNET_IP6_ADDRESS BIP6_Broadcast_Addr;

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
 * @param addr - IPv6 address
 */
static void debug_print_ipv6(const char *str, const struct in6_addr *addr)
{
#if PRINT_ENABLED
    char addstr[40] = {};
    BACNET_IP6_ADDRESS bip6_addr = {};

    if (str && addr) {
        memcpy(&bip6_addr.address[0], &addr->s6_addr[0], IP6_ADDRESS_MAX);
        bvlc6_address_to_ascii(&bip6_addr, addstr,
            sizeof(addstr));
        PRINTF("BIP6: %s %s\n", str, addstr);
    }
#endif
}

/**
 * @brief Enabled debug printing of BACnet/IPv4
 */
void bip6_debug_enable(void)
{
    BIP6_Debug = true;
}

static LPSTR PrintError(int ErrorCode)
{
    static char Message[1024];

    // If this program was multithreaded, we'd want to use
    // FORMAT_MESSAGE_ALLOCATE_BUFFER instead of a static buffer here.
    // (And of course, free the buffer when we were done with it)

    FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS |
            FORMAT_MESSAGE_MAX_WIDTH_MASK,
        NULL, ErrorCode, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        (LPSTR)Message, 1024, NULL);
    return Message;
}

/* on Windows, ifname is the IPv6 address of the interface */
void bip6_set_interface(char *ifname)
{
    int i, RetVal;
    struct addrinfo Hints, *AddrInfo, *AI;
    struct sockaddr_in6 *sin;
    struct in6_addr broadcast_address = {};
    struct ipv6_mreq join_request = {};
    SOCKET ServSock[FD_SETSIZE] = {};
    char port[6] = "";
    int sockopt = 0;

    // By setting the AI_PASSIVE flag in the hints to getaddrinfo, we're
    // indicating that we intend to use the resulting address(es) to bind
    // to a socket(s) for accepting incoming connections.  This means that
    // when the Address parameter is NULL, getaddrinfo will return one
    // entry per allowed protocol family containing the unspecified address
    // for that family.
    //
    memset(&Hints, 0, sizeof(Hints));
    Hints.ai_family = AF_INET6;
    Hints.ai_socktype = SOCK_DGRAM;
    Hints.ai_protocol = IPPROTO_UDP;
    Hints.ai_flags = AI_NUMERICHOST | AI_PASSIVE;
    snprintf(port, sizeof(port), "%u", BIP6_Addr.port);
    PRINTF("BIP6: seeking IPv6 address %s port %s...\n", ifname, port);
    RetVal = getaddrinfo(ifname, &port[0], &Hints, &AddrInfo);
    if (RetVal != 0) {
        fprintf(stderr, "BIP6: getaddrinfo failed with error %d: %s\n", RetVal,
            gai_strerror(RetVal));
        WSACleanup();
        return;
    }
    PRINTF("BIP6: getaddrinfo() succeeded!\n");
    //
    // Find the first matching address getaddrinfo returned so that
    // we can create a new socket and bind that address to it,
    // and create a queue to listen on.
    //
    for (i = 0, AI = AddrInfo; AI != NULL; AI = AI->ai_next) {
        // Highly unlikely, but check anyway.
        if (i == FD_SETSIZE) {
            fprintf(stderr,
                "BIP6: getaddrinfo returned more addresses than we could "
                "use.\n");
            break;
        }
        // only support AF_INET6.
        if (AI->ai_family != AF_INET6) {
            continue;
        }
        // only support SOCK_DGRAM.
        if (AI->ai_socktype != SOCK_DGRAM) {
            continue;
        }
        // only support IPPROTO_UDP.
        if (AI->ai_protocol != IPPROTO_UDP) {
            continue;
        }
        BIP6_Socket = socket(AF_INET6, SOCK_DGRAM, IPPROTO_UDP);
        if (BIP6_Socket == INVALID_SOCKET) {
            fprintf(stderr, "BIP6: socket() failed with error %d: %s\n",
                WSAGetLastError(), PrintError(WSAGetLastError()));
            continue;
        }
        if ((AI->ai_family == AF_INET6) && IN6_IS_ADDR_LINKLOCAL(AI->ai_addr) &&
            (((SOCKADDR_IN6 *)(AI->ai_addr))->sin6_scope_id == 0)) {
            fprintf(
                stderr, "BIP6: IPv6 link local addresses needs a scope ID!\n");
        }
        /* Allow us to use the same socket for sending and receiving */
        /* This makes sure that the src port is correct when sending */
        sockopt = 1;
        RetVal = setsockopt(BIP6_Socket, SOL_SOCKET, SO_REUSEADDR,
            (char *)&sockopt, sizeof(sockopt));
        if (RetVal < 0) {
            closesocket(BIP6_Socket);
            BIP6_Socket = INVALID_SOCKET;
            fprintf(stderr,
                "BIP6: setsockopt(SO_REUSEADDR) failed with error %d: %s\n",
                WSAGetLastError(), PrintError(WSAGetLastError()));
            continue;
        }
        /* allow us to send a broadcast */
        RetVal = setsockopt(BIP6_Socket, SOL_SOCKET, SO_BROADCAST,
            (char *)&sockopt, sizeof(sockopt));
        if (RetVal < 0) {
            closesocket(BIP6_Socket);
            BIP6_Socket = INVALID_SOCKET;
            fprintf(stderr,
                "BIP6: setsockopt(SO_BROADCAST) failed "
                "with error %d: %s\n",
                WSAGetLastError(), PrintError(WSAGetLastError()));
            continue;
        }
        /* subscribe to a multicast address */
        memcpy(&broadcast_address.s6_addr[0], &BIP6_Broadcast_Addr.address[0],
            IP6_ADDRESS_MAX);
        memcpy(&join_request.ipv6mr_multiaddr, &broadcast_address,
            sizeof(struct in6_addr));
        /* Let system not choose the interface */
        BIP6_Socket_Scope_Id = if_nametoindex(ifname);
        join_request.ipv6mr_interface = BIP6_Socket_Scope_Id;
        RetVal = setsockopt(BIP6_Socket, IPPROTO_IPV6, IPV6_JOIN_GROUP,
            (char *)&join_request, sizeof(join_request));
        if (RetVal < 0) {
            fprintf(stderr,
                "BIP6: setsockopt(IPV6_JOIN_GROUP) failed "
                "with error %d: %s\n",
                WSAGetLastError(), PrintError(WSAGetLastError()));
        }
        //
        // bind() associates a local address and port combination
        // with the socket just created. This is most useful when
        // the application is a server that has a well-known port
        // that clients know about in advance.
        //
        if (bind(BIP6_Socket, AI->ai_addr, AI->ai_addrlen) == SOCKET_ERROR) {
            fprintf(stderr, "BIP6: bind() failed with error %d: %s\n",
                WSAGetLastError(), PrintError(WSAGetLastError()));
            closesocket(ServSock[i]);
            continue;
        } else {
            sin = (struct sockaddr_in6 *)AI->ai_addr;
            bvlc6_address_set(&BIP6_Addr, ntohs(sin->sin6_addr.s6_addr16[0]),
                ntohs(sin->sin6_addr.s6_addr16[1]),
                ntohs(sin->sin6_addr.s6_addr16[2]),
                ntohs(sin->sin6_addr.s6_addr16[3]),
                ntohs(sin->sin6_addr.s6_addr16[4]),
                ntohs(sin->sin6_addr.s6_addr16[5]),
                ntohs(sin->sin6_addr.s6_addr16[6]),
                ntohs(sin->sin6_addr.s6_addr16[7]));
            debug_print_ipv6("bind() succeeded!", &sin->sin6_addr);
            /* https://msdn.microsoft.com/en-us/library/windows/desktop/ms740496(v=vs.85).aspx
             */
        }
        i++;
        if (BIP6_Socket != INVALID_SOCKET) {
            break;
        }
    }
    freeaddrinfo(AddrInfo);
    if (BIP6_Socket == INVALID_SOCKET) {
        fprintf(stderr, "BIP6: AF_INET6 address not found getaddrinfo()\n");
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
 * Determine if the BACnet/IPv6 address matches our own address
 *
 * @param addr - network IPv6 address
 *
 * @return true if the BACnet/IPv6 address matches our own address
 */
bool bip6_address_match_self(BACNET_IP6_ADDRESS *addr)
{
    return !bvlc6_address_different(addr, &BIP6_Addr);
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
    return sendto(BIP6_Socket, (char *)mtu, mtu_len, 0,
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
    memcpy(&addr.address[0], &(sin.sin6_addr), 16);
    memcpy(&addr.port, &sin.sin6_port, 2);
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
    if (BIP6_Socket != -1) {
        closesocket(BIP6_Socket);
    }
    BIP6_Socket = -1;
    WSACleanup();

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
 * @note For Windows, ifname is the IPv6 address of the interface.
 *
 * https://msdn.microsoft.com/en-us/library/windows/desktop/ms738639(v=vs.85).aspx
 *
 * @param ifname [in] The named interface to use for the network layer.
 *
 * @return True if the socket is successfully opened for BACnet/IPv6,
 *         else False if the socket functions fail.
 */
bool bip6_init(char *ifname)
{
    WSADATA wd;
    int RetVal;

    // Ask for Winsock version 2.2.
    if ((RetVal = WSAStartup(MAKEWORD(2, 2), &wd)) != 0) {
        fprintf(stderr, "BIP6: WSAStartup failed with error %d: %s\n", RetVal,
            PrintError(RetVal));
        WSACleanup();
        exit(1);
    }
    if (BIP6_Addr.port == 0) {
        bip6_set_port(0xBAC0U);
    }
    PRINTF("BIP6: IPv6 UDP port: 0x%04X\n", BIP6_Addr.port);
    bip6_set_interface(ifname);
    if (BIP6_Broadcast_Addr.address[0] == 0) {
        bvlc6_address_set(&BIP6_Broadcast_Addr, BIP6_MULTICAST_LINK_LOCAL, 0, 0,
            0, 0, 0, 0, BIP6_MULTICAST_GROUP_ID);
    }
    if (BIP6_Socket == INVALID_SOCKET) {
        fprintf(stderr, "BIP6: Fatal error: unable to serve on any address.\n");
        WSACleanup();
        exit(1);
    }

    return true;
}
