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

#include <stdio.h>              /* Standard I/O */
#include <stdlib.h>             /* Standard Library */
#include <errno.h>              /* Error number and related */
#include <stdint.h>             // for standard integer types uint8_t etc.
#include <stdbool.h>            // for the standard bool type.
#include <sys/types.h>          /* System data types */
#include <unistd.h>             /* Command-line options */
#include <string.h>             /* string hanfling functions */

#define ENUMS
#include <sys/socket.h>
#include <net/route.h>
#include <net/if.h>

#include <features.h>           /* for the glibc version number */
#if __GLIBC__ >= 2 && __GLIBC_MINOR >= 1
#include <netpacket/packet.h>
#include <net/ethernet.h>       /* the L2 protocols */
#else
#include <asm/types.h>
#include <linux/if_packet.h>
#include <linux/if_ether.h>     /* The L2 protocols */
#endif
#include <netinet/in.h>
#include <sys/ioctl.h>

#include "bacdef.h"
#include "ethernet.h"
#include "bacdcode.h"

 // commonly used comparison address for ethernet
uint8_t Ethernet_Broadcast[MAX_MAC_LEN] =
    { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };
// commonly used empty address for ethernet quick compare
uint8_t Ethernet_Empty_MAC[MAX_MAC_LEN] = { 0, 0, 0, 0, 0, 0 };
// my local device data - MAC address
uint8_t Ethernet_MAC_Address[MAX_MAC_LEN] = { 0 };

static int eth802_sockfd = -1;         /* 802.2 file handle */
static struct sockaddr eth_addr = { 0 };       // used for binding 802.2

bool ethernet_valid(void)
{
  return (eth802_sockfd >= 0);
}

bool ethernet_cleanup(void)
{
  if (ethernet_valid())
      close(eth802_sockfd);
  eth802_sockfd = -1;

  return true;    
}

/* opens an 802.2 socket to receive and send packets */
static int ethernet_bind(struct sockaddr *eth_addr, char *interface_name)
{
    int sock_fd = -1; // return value
    int uid = 0;

    /* check to see if we are being run as root */
    uid = getuid();
    if (uid != 0) {             
        fprintf(stderr,
            "ethernet: Unable to open an 802.2 socket.  "
            "Try running with root priveleges.\n");
        return sock_fd;
    }
    /* Attempt to open the socket for 802.2 ethernet frames */
    if ((sock_fd = socket(AF_INET, SOCK_PACKET, htons(ETH_P_802_2))) < 0)
    {
        /* Error occured */
        fprintf(stderr,
            "ethernet: Error opening socket : %s\n",
            strerror(errno));
        exit(-1);
    }
    /* Bind the socket to an address */
    eth_addr->sa_family = AF_UNIX;
    /* Clear the memory before copying */
    memset(eth_addr->sa_data, '\0', sizeof(struct sockaddr_in));
    /* Strcpy the interface name into the address */
    strncpy(eth_addr->sa_data, interface_name, IFNAMSIZ);
    /* Attempt to bind the socket to the interface */
    if (bind(sock_fd, eth_addr, sizeof(struct sockaddr)) != 0)
    {
        /* Bind problem, close socket and return */
        fprintf(stderr,
            "ethernet: Unable to bind 802.2 socket : %s\n",
            strerror(errno));
        /* Close the socket */
        close(sock_fd);
        exit(-1);
    }

    return sock_fd;
}

/* function to find the local ethernet MAC address */
static int get_local_hwaddr(const char *ifname, unsigned char *mac)
{
    struct ifreq ifr;
    int fd;
    int rv; // return value - error value from df or ioctl call

    /* determine the local MAC address */
    strcpy(ifr.ifr_name, ifname);
    fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);
    if (fd < 0)
        rv = fd;
    else {
        rv = ioctl(fd, SIOCGIFHWADDR, &ifr);
        if (rv >= 0)            /* worked okay */
            memcpy(mac, ifr.ifr_hwaddr.sa_data, IFHWADDRLEN);
    }

    return rv;
}

bool ethernet_init(char *interface_name)
{
    get_local_hwaddr(interface_name, Ethernet_MAC_Address);
    eth802_sockfd =
        ethernet_bind(&eth_addr, interface_name);

    return ethernet_valid(); 
}

/* function to send a packet out the 802.2 socket */
/* returns 0 on success, non-zero on failure */
int ethernet_send(
  BACNET_ADDRESS *dest,  // destination address
  BACNET_ADDRESS *src,  // source address
  uint8_t *pdu, // any data to be sent - may be null
  unsigned pdu_len) // number of bytes of data
{
    int status = -1;
    int bytes = 0;
    uint8_t mtu[MAX_MPDU] = { 0 };
    int mtu_len = 0;
    int i = 0;

    // don't waste time if the socket is not valid
    if (eth802_sockfd < 0)
    {
        fprintf(stderr, "ethernet: 802.2 socket is invalid!\n");
        return status;
    }
    /* load destination ethernet MAC address */
    if (dest->mac_len == 6)
    {
      for (i = 0; i < 6; i++)
      {
        mtu[i] = dest->mac[i];
        mtu_len++;
      }
    }
    // Global Broadcast
    else if (dest->mac_len == 0)
    {
      for (i = 0; i < 6; i++)
      {
        mtu[i] = Ethernet_Broadcast[i];
        mtu_len++;
      }
    }
    else
    {
        fprintf(stderr, "ethernet: invalid destination MAC address!\n");
        return status;
    }

    /* load source ethernet MAC address */
    if (src->mac_len == 6)
    {
      for (i = 0; i < 6; i++)
      {
        mtu[i] = src->mac[i];
        mtu_len++;
      }
    }
    else
    {
        fprintf(stderr, "ethernet: invalid source MAC address!\n");
        return status;
    }
    if ((14 + 3 + pdu_len) > MAX_MPDU)
    {
        fprintf(stderr, "ethernet: PDU is too big to send!\n");
        return status;
    }
    /* packet length */
    mtu_len += encode_unsigned16(&mtu[12], 
      3 /*DSAP,SSAP,LLC*/ + pdu_len);
    // Logical PDU portion
    mtu[mtu_len++] = 0x82; /* DSAP for BACnet */
    mtu[mtu_len++] = 0x82; /* SSAP for BACnet */
    mtu[mtu_len++] = 0x03; /* Control byte in header */
    memcpy(&mtu[mtu_len], pdu, pdu_len);
    mtu_len += pdu_len;
    
    /* Send the packet */
    bytes =
        sendto(eth802_sockfd, &mtu, mtu_len, 0,
        (struct sockaddr *) &eth_addr, sizeof(struct sockaddr));
    /* did it get sent? */
    if (bytes < 0) {            
      fprintf(stderr,"ethernet: Error sending packet: %s\n", 
        strerror(errno));
      return status;
    }
    
    // got this far - must be good!
    status = 0;

    return status;
}

/* function to send a packet out the 802.2 socket */
/* returns zero on success, non-zero on failure */
int ethernet_send_pdu(
  BACNET_ADDRESS *dest,  // destination address
  uint8_t *pdu, // any data to be sent - may be null
  unsigned pdu_len) // number of bytes of data
{
  int i = 0; // counter
  BACNET_ADDRESS src = {0};  // source address
    
  for (i = 0; i < 6; i++)
  {
    src.mac[i] = Ethernet_MAC_Address[i];
    src.mac_len++;
  }
  /* function to send a packet out the 802.2 socket */
  /* returns 1 on success, 0 on failure */
  return ethernet_send(dest,  // destination address
    &src,  // source address
    pdu, // any data to be sent - may be null
    pdu_len); // number of bytes of data
}

// receives an 802.2 framed packet
// returns the number of octets in the PDU, or zero on failure
uint16_t ethernet_receive(
  BACNET_ADDRESS *src,  // source address
  uint8_t *pdu, // PDU data
  uint16_t max_pdu) // amount of space available in the PDU 
{
    int received_bytes;
    uint8_t buf[MAX_MPDU] = {0}; // data
    uint16_t pdu_len = 0; // return value

    /* Make sure the socket is open */
    if (eth802_sockfd <= 0)
        return 0;

    // FIXME: what about accept()?

    /* Attempt a read */
    received_bytes = read(eth802_sockfd, &buf[0], MAX_MPDU);

    /* See if there is a problem */
    if (received_bytes < 0) {
        fprintf(stderr,"ethernet: Read error in receiving packet: %s\n",
            strerror(errno));
        return 0;
    }

    /* the signature of an 802.2 BACnet packet */
    if ((buf[14] != 0x82) && (buf[15] != 0x82)) {
        //fprintf(stderr,"ethernet: Non-BACnet packet\n");
        return 0;
    }
    // copy the source address
    src->mac_len = 6;
    memmove(src->mac, &buf[6], 6);

    // check destination address for when
    // the Ethernet card is in promiscious mode
    if ((memcmp(&buf[0], Ethernet_MAC_Address,6) != 0)
        && (memcmp(&buf[0], Ethernet_Broadcast, 6) != 0)) 
    {
        //fprintf(stderr, "ethernet: This packet isn't for us\n");
        return 0;
    }

    (void)decode_unsigned16(&buf[12],&pdu_len);
    pdu_len -= 3 /* DSAP, SSAP, LLC Control */ ;
    // copy the buffer into the PDU
    if (pdu_len < max_pdu)
      memmove(&pdu[0],&buf[17],pdu_len);

    return pdu_len;
}

void ethernet_get_my_address(BACNET_ADDRESS *my_address)
{
  int i = 0;
  
  my_address->mac_len = 0;
  my_address->net = 0; // local only, no routing
  for (i = 0; i < 6; i++)
  {
    my_address->mac[i] = Ethernet_MAC_Address[i];
    my_address->mac_len++;
  }

  return;
}

