/*####COPYRIGHTBEGIN####
 -------------------------------------------
 Copyright (C) 2005 Steve Karg
 Contributions by Thomas Neumann in 2008.

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
#include "bacnet/bacint.h"
#include "bacnet/config.h"
#include "bacnet/datalink/bip.h"
#include "bacnet/basic/sys/debug.h"
#include "bacnet/basic/bbmd/h_bbmd.h"
#include "bacport.h"

/* alternate methods of choosing broadcast address */
#ifndef USE_INADDR
#define USE_INADDR 0
#endif
#ifndef USE_CLASSADDR
#define USE_CLASSADDR 0
#endif

/* Windows socket */
static SOCKET BIP_Socket = INVALID_SOCKET;

/* NOTE: we store address and port in network byte order
   since BACnet/IP uses network byte order for all address byte arrays
*/
/* port to use - stored here in network byte order */
/* Initialize to 0 - this will force initialization in demo apps */
static uint16_t BIP_Port;
/* IP address - stored here in network byte order */
static struct in_addr BIP_Address;
/* IP broadcast address - stored here in network byte order */
static struct in_addr BIP_Broadcast_Address;

/* enable debugging */
static bool BIP_Debug = false;

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
 * @return The active BIP socket, or INVALID_SOCKET if uninitialized.
 * @note Strictly, the return type should be SOCKET, however in practice 
 *  Windows never returns values large enough that truncation is an issue.
 */
int bip_get_socket(void)
{
    return (int)BIP_Socket;
}

/**
 * @brief Enabled debug printing of BACnet/IPv4
 */
void bip_debug_enable(void)
{
    BIP_Debug = true;
}

/**
 * @brief Get the text string for Windows Error Codes
 */
static char *winsock_error_code_text(int code)
{
    switch (code) {
        case WSAEACCES:
            return "Permission denied.";
        case WSAEINTR:
            return "Interrupted system call.";
        case WSAEBADF:
            return "Bad file number.";
        case WSAEFAULT:
            return "Bad address.";
        case WSAEINVAL:
            return "Invalid argument.";
        case WSAEMFILE:
            return "Too many open files.";
        case WSAEWOULDBLOCK:
            return "Operation would block.";
        case WSAEINPROGRESS:
            return "Operation now in progress. "
                   "This error is returned if any Windows Sockets API "
                   "function is called while a blocking function "
                   "is in progress.";
        case WSAENOTSOCK:
            return "Socket operation on nonsocket.";
        case WSAEDESTADDRREQ:
            return "Destination address required.";
        case WSAEMSGSIZE:
            return "Message too long.";
        case WSAEPROTOTYPE:
            return "Protocol wrong type for socket.";
        case WSAENOPROTOOPT:
            return "Protocol not available.";
        case WSAEPROTONOSUPPORT:
            return "Protocol not supported.";
        case WSAESOCKTNOSUPPORT:
            return "Socket type not supported.";
        case WSAEOPNOTSUPP:
            return "Operation not supported on socket.";
        case WSAEPFNOSUPPORT:
            return "Protocol family not supported.";
        case WSAEAFNOSUPPORT:
            return "Address family not supported by protocol family.";
        case WSAEADDRINUSE:
            return "Address already in use.";
        case WSAEADDRNOTAVAIL:
            return "Cannot assign requested address.";
        case WSAENETDOWN:
            return "Network is down. "
                   "This error may be reported at any time "
                   "if the Windows Sockets implementation "
                   "detects an underlying failure.";
        case WSAENETUNREACH:
            return "Network is unreachable.";
        case WSAENETRESET:
            return "Network dropped connection on reset.";
        case WSAECONNABORTED:
            return "Software caused connection abort.";
        case WSAECONNRESET:
            return "Connection reset by peer.";
        case WSAENOBUFS:
            return "No buffer space available.";
        case WSAEISCONN:
            return "Socket is already connected.";
        case WSAENOTCONN:
            return "Socket is not connected.";
        case WSAESHUTDOWN:
            return "Cannot send after socket shutdown.";
        case WSAETOOMANYREFS:
            return "Too many references: cannot splice.";
        case WSAETIMEDOUT:
            return "Connection timed out.";
        case WSAECONNREFUSED:
            return "Connection refused.";
        case WSAELOOP:
            return "Too many levels of symbolic links.";
        case WSAENAMETOOLONG:
            return "File name too long.";
        case WSAEHOSTDOWN:
            return "Host is down.";
        case WSAEHOSTUNREACH:
            return "No route to host.";
        case WSASYSNOTREADY:
            return "Returned by WSAStartup(), "
                   "indicating that the network subsystem is unusable.";
        case WSAVERNOTSUPPORTED:
            return "Returned by WSAStartup(), "
                   "indicating that the Windows Sockets DLL cannot support "
                   "this application.";
        case WSANOTINITIALISED:
            return "Winsock not initialized. "
                   "This message is returned by any function "
                   "except WSAStartup(), "
                   "indicating that a successful WSAStartup() has not yet "
                   "been performed.";
        case WSAEDISCON:
            return "Disconnect.";
        case WSAHOST_NOT_FOUND:
            return "Host not found. "
                   "This message indicates that the key "
                   "(name, address, and so on) was not found.";
        case WSATRY_AGAIN:
            return "Nonauthoritative host not found. "
                   "This error may suggest that the name service itself "
                   "is not functioning.";
        case WSANO_RECOVERY:
            return "Nonrecoverable error. "
                   "This error may suggest that the name service itself "
                   "is not functioning.";
        case WSANO_DATA:
            return "Valid name, no data record of requested type. "
                   "This error indicates that the key "
                   "(name, address, and so on) was not found.";
        default:
            return "unknown";
    }
}

/**
 * @brief Print the text string for the last Windows Error Code
 */
static void print_last_error(const char *info)
{
    int Code = WSAGetLastError();
    fprintf(stderr, "BIP: %s [error code %i] %s\n", info, Code,
        winsock_error_code_text(Code));
    fflush(stderr);
}

/**
 * @brief Initialize the Windows Socket Layer
 */
static void bip_init_windows(void)
{
    static bool initialized = false;
    int Result;
    WSADATA wd;

    if (!initialized) {
        Result = WSAStartup((1 << 8) | 1, &wd);
        /*Result = WSAStartup(MAKEWORD(2,2), &wd); */
        if (Result != 0) {
            print_last_error("TCP/IP stack initialization failed");
            exit(1);
        }
        initialized = true;
        atexit(bip_cleanup);
    }
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
        memcpy(&dest->mac[0], &BIP_Broadcast_Address.s_addr, 4);
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
    /* not something we do here within this application */
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
    /* not something we do within this application */
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
        memcpy(&addr->address[0], &BIP_Broadcast_Address.s_addr, 4);
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
    /* not something we do within this application */
    return false;
}

/**
 * @brief Get the BACnet/IP subnet mask CIDR prefix
 * @return subnet mask CIDR prefix
 */
uint8_t bip_get_subnet_prefix(void)
{
    uint32_t address = 0;
    uint32_t broadcast = 0;
    uint32_t test_broadcast = 0;
    uint32_t mask = 0xFFFFFFFE;
    uint8_t prefix = 0;

    address = BIP_Broadcast_Address.s_addr;
    broadcast = BIP_Broadcast_Address.s_addr;
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
    int rv = 0;

    /* assumes that the driver has already been initialized */
    if (BIP_Socket == INVALID_SOCKET) {
        if (BIP_Debug) {
            fprintf(stderr, "BIP: driver not initialized!\n");
            fflush(stderr);
        }
        return -1;
    }
    /* load destination IP address */
    bip_dest.sin_family = AF_INET;
    memcpy(&bip_dest.sin_addr.s_addr, &dest->address[0], 4);
    bip_dest.sin_port = htons(dest->port);
    /* Send the packet */
    debug_print_ipv4(
        "Sending MPDU->", &bip_dest.sin_addr, bip_dest.sin_port, mtu_len);
    rv = sendto(BIP_Socket, (char *)mtu, mtu_len, 0,
        (struct sockaddr *)&bip_dest, sizeof(struct sockaddr));
    if (rv == SOCKET_ERROR) {
        print_last_error("sendto");
    }

    return rv;
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

    /* Make sure the socket is open */
    if (BIP_Socket == INVALID_SOCKET) {
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
    max = BIP_Socket;
    /* see if there is a packet for us */
    if (select(max + 1, &read_fds, NULL, NULL, &select_timeout) > 0) {
        received_bytes = recvfrom(max, (char *)&npdu[0], max_npdu, 0,
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
    offset = bvlc_handler(&addr, src, npdu, received_bytes);
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

    bip_init_windows();
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

/* To fill a need, we invent the gethostaddr() function. */
static long gethostaddr(void)
{
    struct hostent *host_ent;
    char host_name[255];

    if (gethostname(host_name, sizeof(host_name)) != 0) {
        print_last_error("gethostname");
        exit(1);
    }
    if ((host_ent = gethostbyname(host_name)) == NULL) {
        print_last_error("gethostbyname");
        exit(1);
    }
    if (BIP_Debug) {
        fprintf(stderr, "BIP: host %s at %u.%u.%u.%u\n", host_name,
            (unsigned)((uint8_t *)host_ent->h_addr)[0],
            (unsigned)((uint8_t *)host_ent->h_addr)[1],
            (unsigned)((uint8_t *)host_ent->h_addr)[2],
            (unsigned)((uint8_t *)host_ent->h_addr)[3]);
        fflush(stderr);
    }
    /* note: network byte order */
    return *(long *)host_ent->h_addr;
}

#if ((USE_INADDR == 0) || (USE_CLASSADDR == 0))
/* returns the subnet mask in network byte order */
static uint32_t getIpMaskForIpAddress(uint32_t ipAddress)
{
    /* Allocate information for up to 16 NICs */
    IP_ADAPTER_INFO AdapterInfo[16];
    /* Save memory size of buffer */
    DWORD dwBufLen = sizeof(AdapterInfo);
    uint32_t ipMask = INADDR_BROADCAST;
    bool found = false;

    PIP_ADAPTER_INFO pAdapterInfo;

    /* GetAdapterInfo:
       [out] buffer to receive data
       [in] size of receive data buffer */
    DWORD dwStatus = GetAdaptersInfo(AdapterInfo, &dwBufLen);
    if (dwStatus == ERROR_SUCCESS) {
        /* Verify return value is valid, no buffer overflow
           Contains pointer to current adapter info */
        pAdapterInfo = AdapterInfo;

        do {
            IP_ADDR_STRING *pIpAddressInfo = &pAdapterInfo->IpAddressList;
            do {
                unsigned long adapterAddress =
                    inet_addr(pIpAddressInfo->IpAddress.String);
                unsigned long adapterMask =
                    inet_addr(pIpAddressInfo->IpMask.String);
                if (adapterAddress == ipAddress) {
                    ipMask = adapterMask;
                    found = true;
                }
                pIpAddressInfo = pIpAddressInfo->Next;
            } while (pIpAddressInfo && !found);
            /* Progress through linked list */
            pAdapterInfo = pAdapterInfo->Next;
            /* Terminate on last adapter */
        } while (pAdapterInfo && !found);
    }

    return ipMask;
}
#endif

static void set_broadcast_address(uint32_t net_address)
{
#if USE_INADDR
    /*   Note: sometimes INADDR_BROADCAST does not let me get
       any unicast messages.  Not sure why... */
    (void)net_address;
    BIP_Broadcast_Address.s_addr = INADDR_BROADCAST;
#elif USE_CLASSADDR
    long broadcast_address = 0;

    if (IN_CLASSA(ntohl(net_address)))
        broadcast_address =
            (ntohl(net_address) & ~IN_CLASSA_HOST) | IN_CLASSA_HOST;
    else if (IN_CLASSB(ntohl(net_address)))
        broadcast_address =
            (ntohl(net_address) & ~IN_CLASSB_HOST) | IN_CLASSB_HOST;
    else if (IN_CLASSC(ntohl(net_address)))
        broadcast_address =
            (ntohl(net_address) & ~IN_CLASSC_HOST) | IN_CLASSC_HOST;
    else if (IN_CLASSD(ntohl(net_address)))
        broadcast_address =
            (ntohl(net_address) & ~IN_CLASSD_HOST) | IN_CLASSD_HOST;
    else
        broadcast_address = INADDR_BROADCAST;
    BIP_Broadcast_Address.s_addr = htonl(broadcast_address));
#else
    /* these are network byte order variables */
    long broadcast_address = 0;
    long net_mask = 0;

    net_mask = getIpMaskForIpAddress(net_address);
    if (BIP_Debug) {
        struct in_addr address;
        address.s_addr = net_mask;
        fprintf(stderr, "BIP: net mask: %s\n", inet_ntoa(address));
        fflush(stderr);
    }
    broadcast_address = (net_address & net_mask) | (~net_mask);
    BIP_Broadcast_Address.s_addr = broadcast_address;
#endif
}

/**
 * @brief Gets the local IP address and local broadcast address from the
 *  system, and saves it into the BACnet/IP data structures.
 *
 * @param ifname [in] The named interface to use for the network layer.
 *        Eg, for Windows, ifname is the dotted ip address of the interface
 */
void bip_set_interface(char *ifname)
{
    bip_init_windows();
    /* setup local address */
    if (BIP_Address.s_addr == 0) {
        BIP_Address.s_addr = inet_addr(ifname);
    }
    if (BIP_Debug) {
        fprintf(stderr, "BIP: Interface: %s\n", ifname);
        fprintf(stderr, "BIP: Address: %s\n", inet_ntoa(BIP_Address));
        fflush(stderr);
    }
    /* setup local broadcast address */
    if (BIP_Broadcast_Address.s_addr == 0) {
        set_broadcast_address(BIP_Address.s_addr);
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
 * @note For Windows, ifname is the dotted ip address of the interface.
 *
 * @param ifname [in] The named interface to use for the network layer.
 *        If NULL, the "eth0" interface is assigned.
 * @return True if the socket is successfully opened for BACnet/IP,
 *         else False if the socket functions fail.
 */
bool bip_init(char *ifname)
{
    int rv = 0; /* return from socket lib calls */
    struct sockaddr_in sin = { -1 };
    int value = 1;
    SOCKET sock_fd = INVALID_SOCKET;

    bip_init_windows();
    if (ifname) {
        bip_set_interface(ifname);
    }
    /* has address been set? */
    if (BIP_Address.s_addr == 0) {
        BIP_Address.s_addr = gethostaddr();
    }
    /* has broadcast address been set? */
    if (BIP_Broadcast_Address.s_addr == 0) {
        set_broadcast_address(BIP_Address.s_addr);
    }
    if (BIP_Debug) {
        fprintf(stderr, "BIP: Address: %s\n", inet_ntoa(BIP_Address));
        fprintf(stderr, "BIP: Broadcast Address: %s\n",
            inet_ntoa(BIP_Broadcast_Address));
        fprintf(stderr, "BIP: UDP Port: 0x%04X [%hu]\n", ntohs(BIP_Port),
            ntohs(BIP_Port));
        fflush(stderr);
    }
    /* assumes that the driver has already been initialized */
    sock_fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    BIP_Socket = sock_fd;
    if (sock_fd < 0) {
        print_last_error("failed to allocate a socket");
        return false;
    }
    /* Allow us to use the same socket for sending and receiving */
    /* This makes sure that the src port is correct when sending */
    rv = setsockopt(
        sock_fd, SOL_SOCKET, SO_REUSEADDR, (char *)&value, sizeof(value));
    if (rv < 0) {
        print_last_error("failed to set REUSEADDR socket option");
        closesocket(sock_fd);
        BIP_Socket = INVALID_SOCKET;
        return false;
    }
    /* Enables transmission and receipt of broadcast messages on the socket. */
    rv = setsockopt(
        sock_fd, SOL_SOCKET, SO_BROADCAST, (char *)&value, sizeof(value));
    if (rv < 0) {
        print_last_error("failed to set BROADCAST socket option");
        closesocket(sock_fd);
        BIP_Socket = INVALID_SOCKET;
        return false;
    }
    /* bind the socket to the local port number and IP address */
    sin.sin_family = AF_INET;
#if USE_INADDR
    /* by setting sin.sin_addr.s_addr to INADDR_ANY,
       I am telling the IP stack to automatically fill
       in the IP address of the machine the process
       is running on.

       Some server computers have multiple IP addresses.
       A socket bound to one of these will not accept
       connections to another address. Frequently you prefer
       to allow any one of the computer's IP addresses
       to be used for connections.  Use INADDR_ANY (0L) to
       allow clients to connect using any one of the host's
       IP addresses. */
    sin.sin_addr.s_addr = htonl(INADDR_ANY);
#else
    /* or we could use the specific adapter address
       note: already in network byte order */
    sin.sin_addr.s_addr = BIP_Address.s_addr;
#endif
    sin.sin_port = BIP_Port;
    memset(&(sin.sin_zero), '\0', sizeof(sin.sin_zero));
    if (BIP_Debug) {
        fprintf(stderr, "BIP: bind %s:%hu\n", inet_ntoa(sin.sin_addr),
            ntohs(sin.sin_port));
        fflush(stderr);
    }
    rv = bind(sock_fd, (const struct sockaddr *)&sin, sizeof(struct sockaddr));
    if (rv < 0) {
        print_last_error("failed to bind");
        closesocket(sock_fd);
        BIP_Socket = INVALID_SOCKET;
        return false;
    }

    return true;
}

/**
 * @brief Determine if this BACnet/IP datalink is valid
 * @return true if the BACnet/IP datalink is valid
 */
bool bip_valid(void)
{
    return (BIP_Socket != INVALID_SOCKET);
}

/** Cleanup and close out the BACnet/IP services by closing the socket.
 * @ingroup DLBIP
 */
void bip_cleanup(void)
{
    SOCKET sock_fd = 0;

    if (BIP_Socket != INVALID_SOCKET) {
        sock_fd = BIP_Socket;
        closesocket(sock_fd);
    }
    BIP_Socket = INVALID_SOCKET;
    WSACleanup();

    return;
}
