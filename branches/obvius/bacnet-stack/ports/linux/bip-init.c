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

#include <stdint.h>     /* for standard integer types uint8_t etc. */
#include <stdbool.h>    /* for the standard bool type. */
#include "bacdcode.h"
#include "bip.h"
#include "net.h"
#include "debug.h"

/** @file linux/bip-init.c  Initializes BACnet/IP interface (Linux). */

bool BIP_Debug = false;

/* If non-zero, it means we've bound our B/IP socket to an alternate
 * UDP port, so that we can register as foreign devices with a BACnet
 * server running on the same host, and thereby "share" the standard
 * BACnet socket between several Linux processes.
 */
uint16_t BIP_My_Port = 0;

/* gets an IP address by name, where name can be a
   string that is an IP address in dotted form, or
   a name that is a domain name
   returns 0 if not found, or
   an IP address in network byte order */
long bip_getaddrbyname(
    const char *host_name)
{
    struct hostent *host_ent;

    if ((host_ent = gethostbyname(host_name)) == NULL)
        return 0;

    return *(long *) host_ent->h_addr;
}

static int get_local_ifr_ioctl(
    char *ifname,
    struct ifreq *ifr,
    int request)
{
    int fd;
    int rv;     /* return value */

    strncpy(ifr->ifr_name, ifname, sizeof(ifr->ifr_name));
    fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);
    if (fd < 0) {
        rv = fd;
    } else {
        rv = ioctl(fd, request, ifr);
        close(fd);
    }

    return rv;
}

int get_local_address_ioctl(
    char *ifname,
    struct in_addr *addr,
    int request)
{
    struct ifreq ifr = { {{0}} };
    struct sockaddr_in *tcpip_address;
    int rv;     /* return value */

    rv = get_local_ifr_ioctl(ifname, &ifr, request);
    if (rv >= 0) {
        tcpip_address = (struct sockaddr_in *) &ifr.ifr_addr;
        memcpy(addr, &tcpip_address->sin_addr, sizeof(struct in_addr));
    }

    return rv;
}

/** Gets the local IP address and local broadcast address from the system,
 *  and saves it into the BACnet/IP data structures.
 *
 * @param ifname [in] The named interface to use for the network layer.
 *        Eg, for Linux, ifname is eth0, ath0, arc0, and others.
 */
void bip_set_interface(
    char *ifname)
{
    struct in_addr local_address;
    struct in_addr broadcast_address;
    int rv = 0;

    /* setup local address */
    rv = get_local_address_ioctl(ifname, &local_address, SIOCGIFADDR);
    if (rv < 0) {
        local_address.s_addr = 0;
    }
    bip_set_addr(local_address.s_addr);
    if (BIP_Debug) {
        fprintf(stderr, "Interface: %s\n", ifname);
        fprintf(stderr, "IP Address: %s\n", inet_ntoa(local_address));
    }
    /* setup local broadcast address */
    rv = get_local_address_ioctl(ifname, &broadcast_address, SIOCGIFBRDADDR);
    if (rv < 0) {
        broadcast_address.s_addr = ~0;
    }
    bip_set_broadcast_addr(broadcast_address.s_addr);
    if (BIP_Debug) {
        fprintf(stderr, "IP Broadcast Address: %s\n",
            inet_ntoa(broadcast_address));
        fprintf(stderr, "UDP Port: 0x%04X [%hu]\n", ntohs(bip_get_port()),
            ntohs(bip_get_port()));
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
 * @note For Linux, ifname is eth0, ath0, arc0, and others.
 *
 * @param ifname [in] The named interface to use for the network layer.
 *        If NULL, the "eth0" interface is assigned.
 * @return True if the socket is successfully opened for BACnet/IP,
 *         else False if the socket functions fail.
 */
bool bip_init(
    char *ifname)
{
    int status = 0;     /* return from socket lib calls */
    struct sockaddr_in sin;
    int sockopt = 0;
    int sock_fd = -1;
	char *sz;
	uint16_t our_port = 0;
	int client_range = 1;

    if (ifname)
        bip_set_interface(ifname);
    else
        bip_set_interface("eth0");
    /* assumes that the driver has already been initialized */
    sock_fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    bip_set_socket(sock_fd);
    if (sock_fd < 0)
        return false;
	/* The following variables:
	 *
	 *   BACNET_IP_CLIENT_PORT  (example: 48808)
	 *   BACNET_IP_CLIENT_RANGE (example: 100)
	 *
	 * let you specify an alternate UDP port for the B/IP app to bind to,
	 * separate from the standard BACnet port specified in $BACNET_IP_PORT.
	 *
	 * This is useful when trying to run a BACnet server and simultaneously
	 * use the BACnet command-line utilties on a single Linux host.
	 * Ordinarily this would be impossible as each of these "apps" would
	 * need to bind to and receive on the standard BACnet UDP port (47808),
	 * and Linux doesn't allow multiple processes to share a UDP socket
	 * in a useful way (i.e., such that all processes receive all packets).
	 *
	 * One solution is to move all code into a single Linux process.
	 *
	 * A simpler, more modular solution is to run the BACnet server on
	 * the standard BACnet port, with BBMD enabled, and then have each
	 * 'client-side' BACnet app register as a foreign device w/ this 
	 * server.  The above two variables simplify this by letting
	 * these 'client-side' apps automatically choose a free port in the
	 * range specified.
	 */
    sockopt = 1;
	if ((sz = getenv("BACNET_IP_CLIENT_PORT")) != NULL && (our_port = atoi(sz)) > 0) {
		sockopt = 0;
	}
    status =
        setsockopt(sock_fd, SOL_SOCKET, SO_REUSEADDR, &sockopt,
        sizeof(sockopt));
	debug_printf("bip_init: setsockopt SO_REUSEADDR=%d --> %d\n", sockopt, status);
    if (status < 0) {
        close(sock_fd);
        bip_set_socket(-1);
        return status;
    }
    /* allow us to send a broadcast */
	sockopt = 1;
    status =
        setsockopt(sock_fd, SOL_SOCKET, SO_BROADCAST, &sockopt,
        sizeof(sockopt));
	debug_printf("bip_init: SO_BROADCAST=%d --> %d\n", sockopt, status);
    if (status < 0) {
        close(sock_fd);
        bip_set_socket(-1);
        return false;
    }
    /* bind the socket to the local port number and IP address */
	if (!our_port) our_port = ntohs(bip_get_port());
	if ((sz = getenv("BACNET_IP_CLIENT_RANGE")) != NULL && (client_range = atoi(sz)) <= 0)
		client_range = 1;
	{	int skew = 0, tries = client_range;
		srand((unsigned)time(0));
		do {
    sin.sin_family = AF_INET;
    sin.sin_addr.s_addr = htonl(INADDR_ANY);
			sin.sin_port = htons((our_port + skew) & 0xFFFF);
    memset(&(sin.sin_zero), '\0', sizeof(sin.sin_zero));
    status =
        bind(sock_fd, (const struct sockaddr *) &sin, sizeof(struct sockaddr));
			debug_printf("bip_init: bind to %u --> %d\n", ntohs(sin.sin_port), status);
			skew = rand() % client_range;
		} while (status < 0 && --tries > 0);
	}

    if (status < 0) {
        close(sock_fd);
        bip_set_socket(-1);
		debug_printf("bip_init: bind failed\n");
        return false;
    }
	BIP_My_Port = sin.sin_port;
	debug_printf("bip_init: bound to port %u\n", (unsigned) ntohs(sin.sin_port));

    return true;
}

/** Return the UDP port from which *we* send packets, in network-byte order.
 *
 *  This will usually be the standard BACnet port, from bip_get_port(),
 *  only differing if -- for instance -- we've registered as a foreign
 *  device with a BBMD running on the same host.
 */

uint16_t bip_get_my_port(
	void)
{
	return BIP_My_Port == 0 ? bip_get_port() : BIP_My_Port;
}

/** Cleanup and close out the BACnet/IP services by closing the socket.
 * @ingroup DLBIP
  */
void bip_cleanup(
    void)
{
    int sock_fd = 0;

    if (bip_valid()) {
        sock_fd = bip_socket();
        close(sock_fd);
    }
    bip_set_socket(-1);

    return;
}

/** Get the netmask of the BACnet/IP's interface via an ioctl() call.
 * @param netmask [out] The netmask, in host order.
 * @return 0 on success, else the error from the ioctl() call.
 */
int bip_get_local_netmask(
    struct in_addr *netmask)
{
    int rv;
    char *ifname = getenv("BACNET_IFACE");      /* will probably be null */
    if (ifname == NULL)
        ifname = "eth0";
    rv = get_local_address_ioctl(ifname, netmask, SIOCGIFNETMASK);
    return rv;
}
