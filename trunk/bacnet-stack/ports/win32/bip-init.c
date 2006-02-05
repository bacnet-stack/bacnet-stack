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

#define WIN32_LEAN_AND_MEAN
#define STRICT

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h> // for standard integer types uint8_t etc.
#include <stdbool.h> // for the standard bool type.
#include "bacdcode.h"
#include "bip.h"
#include "net.h"

void bip_set_interface(char *ifname)
{
  /*dummy function - to make the demos compile easier */
}

/* To fill a need, we invent the gethostaddr() function. */
static long gethostaddr(void)
{
  struct hostent *host_ent;
  char host_name[255];

  if (gethostname(host_name, sizeof(host_name)) != 0)
    return -1;
  #ifdef BIP_DEBUG
  printf("host name: %s\n",host_name);
  #endif
  if ((host_ent = gethostbyname(host_name)) == NULL)
    return -1;

  return *(long *)host_ent->h_addr;
}

static void set_broadcast_address(uint32_t net_address)
{
  long broadcast_address = 0;
  long mask = 0;

  /*   Note: sometimes INADDR_BROADCAST does not let me get
       any unicast messages.  Not sure why... */
  #if USE_INADDR
  (void)net_address;
  bip_set_broadcast_addr(INADDR_BROADCAST);
  #else
  if (IN_CLASSA(ntohl(net_address)))
    broadcast_address = (ntohl(net_address) & ~IN_CLASSA_HOST) | IN_CLASSA_HOST;
  else if (IN_CLASSB(ntohl(net_address)))
    broadcast_address = (ntohl(net_address) & ~IN_CLASSB_HOST) | IN_CLASSB_HOST;
  else if (IN_CLASSC(ntohl(net_address)))
    broadcast_address = (ntohl(net_address) & ~IN_CLASSC_HOST) | IN_CLASSC_HOST;
  else if (IN_CLASSD(ntohl(net_address)))
    broadcast_address = (ntohl(net_address) & ~IN_CLASSD_HOST) | IN_CLASSD_HOST;
  else
    broadcast_address = INADDR_BROADCAST;
  bip_set_broadcast_addr(htonl(broadcast_address));
  #endif
}

static void cleanup(void)
{
  WSACleanup();
}

void bip_set_interface(char *ifname)
{
  (void)ifname;
  /* dummy function */
}

bool bip_init(void)
{
    int rv = 0; // return from socket lib calls
    struct sockaddr_in sin = {-1};
    int value = 1;
    int sock_fd = -1;
    int Result;
    int Code;
    WSADATA wd;
    struct in_addr address;

    Result = WSAStartup((1 << 8) | 1, &wd);
    //Result = WSAStartup(MAKEWORD(2,2), &wd);
    if (Result != 0)
    {
      Code = WSAGetLastError();
      printf("TCP/IP stack initialization failed, error code: %i\n",
        Code);
      exit(1);
    }
    atexit(cleanup);

    address.s_addr = gethostaddr();
    if (address.s_addr == (unsigned)-1)
    {
      Code = WSAGetLastError();
      printf("Get host address failed, error code: %i\n",
        Code);
      exit(1);
    }
    #ifdef BIP_DEBUG
    printf("host address: %s\n",inet_ntoa(address));
    #endif
    bip_set_addr(address.s_addr);
    set_broadcast_address(address.s_addr);

    /* assumes that the driver has already been initialized */
    sock_fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    bip_set_socket(sock_fd);
    if (sock_fd < 0)
    {
      fprintf(stderr,"bip: failed to allocate a socket.\n");
      return false;
    }

    // Allow us to use the same socket for sending and receiving
    // This makes sure that the src port is correct when sending
    rv = setsockopt(sock_fd, SOL_SOCKET, SO_REUSEADDR,
        (char *)&value, sizeof(value));
    if (rv < 0)
    {
      fprintf(stderr,"bip: failed to set REUSEADDR socket option.\n");
      close(sock_fd);
      bip_set_socket(-1);
      return false;
    }
    // allow us to send a broadcast
    rv = setsockopt(sock_fd, SOL_SOCKET, SO_BROADCAST,
      (char *)&value, sizeof(value));
    if (rv < 0)
    {
      fprintf(stderr,"bip: failed to set BROADCAST socket option.\n");
      close(sock_fd);
      bip_set_socket(-1);
      return false;
    }

    // bind the socket to the local port number and IP address
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
       IP addresses.

       Note: sometimes INADDR_ANY does not let me get
       any unicast messages.  Not sure why... */
    sin.sin_addr.s_addr = htonl(INADDR_ANY);
    #else
    /* or we could use the specific adapter address
       note: already in network byte order */
    sin.sin_addr.s_addr = address.s_addr;
    #endif
    sin.sin_port = htons(bip_get_port());
    memset(&(sin.sin_zero), '\0', sizeof(sin.sin_zero));
    rv = bind(sock_fd,
        (const struct sockaddr*)&sin, sizeof(struct sockaddr));
    if (rv < 0)
    {
        fprintf(stderr,"bip: failed to bind to %s port %hd\n",
          inet_ntoa(sin.sin_addr), 
          bip_get_port());
        close(sock_fd);
        bip_set_socket(-1);
        return false;
    }

    return true;
}

