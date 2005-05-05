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

#include <stdint.h>             // for standard integer types uint8_t etc.
#include <stdbool.h>            // for the standard bool type.
#include "net.h" // local port of the stack network includes
#include "bacdcode.h"
#include "bip.h"

static int BIP_Receive_Socket = -1;
/* port to use - stored in network byte order */
static uint16_t BIP_Port = 0;
/* IP Address - stored in network byte order */
static struct in_addr BIP_Address = {{{-1}}};
/* Broadcast Address */
static struct in_addr BIP_Broadcast_Address = {{{-1}}};

bool bip_valid(void)
{
  return (BIP_Receive_Socket != -1);
}

void bip_cleanup(void)
{
  if (bip_valid())
      close(BIP_Receive_Socket);
  BIP_Receive_Socket = -1;

  return;    
}

static void set_network_address(struct in_addr *net_address,
    uint8_t octet1, uint8_t octet2, uint8_t octet3, uint8_t octet4)
{
    union {
        uint8_t byte[4];
        uint32_t value;
    } long_data = {{0}};

    long_data.byte[0] = octet1;
    long_data.byte[1] = octet2;
    long_data.byte[2] = octet3;
    long_data.byte[3] = octet4;
    
    net_address->s_addr = htonl(long_data.value);
}

void bip_set_address(uint8_t octet1, uint8_t octet2, 
    uint8_t octet3, uint8_t octet4)
{
    set_network_address(&BIP_Address, octet1, octet2, octet3, octet4);
}

void bip_set_broadcast_address(uint8_t octet1, uint8_t octet2, 
    uint8_t octet3, uint8_t octet4)
{
    set_network_address(&BIP_Broadcast_Address, 
        octet1, octet2, octet3, octet4);
}

void bip_set_port(uint16_t port)
{
    BIP_Port = htons(port);
}

bool bip_init(void)
{
    int rv = 0; // return from socket lib calls
    struct sockaddr_in sin = {-1};
    
    /* network global broadcast address */
    set_network_address(&BIP_Broadcast_Address,255,255,255,255);
    /* configure standard BACnet/IP port */
    bip_set_port(0xBAC0);

    // assumes that the driver has already been initialized
    BIP_Receive_Socket = socket(AF_INET, SOCK_DGRAM, 0);
    if (BIP_Receive_Socket < 0)
        return false;

    // bind the socket to the local port number and IP address
    sin.sin_family = AF_INET;
    sin.sin_addr.s_addr = htonl(INADDR_ANY);
    sin.sin_port = BIP_Port;
    memset(&(sin.sin_zero), '\0', 8);
    rv = bind(BIP_Receive_Socket, 
        (const struct sockaddr*)&sin, sizeof(struct sockaddr));
    if (rv < 0)
    {
        close(BIP_Receive_Socket);
        BIP_Receive_Socket = -1;
        return false;
    }

    return true; 
}

/* function to send a packet out the 802.2 socket */
/* returns 0 on success, non-zero on failure */
static int bip_send(
  struct sockaddr_in *bip_dest,
  uint8_t *pdu, // any data to be sent - may be null
  unsigned pdu_len) // number of bytes of data
{
    int status = -1; /* initially fail status */
    int bytes = 0;
    int bip_send_socket = -1;
    uint8_t mtu[MAX_MPDU] = { 0 };
    int mtu_len = 0;
    int i = 0;
    int rv = 0; /* return value from socket calls */
    
    // assumes that the driver has already been initialized
    // FIXME: can we use the same socket over and over?
    // FIXME: can we use the same socket as receive bip?
    bip_send_socket = socket(AF_INET, SOCK_DGRAM, 0);
    if (bip_send_socket < 0)
        return status;

    /* UDP is connection based */
    rv = connect(bip_send_socket, 
        (const struct sockaddr*)bip_dest, 
        sizeof(struct sockaddr));
    if (bip_send_socket < 0)
    {
        close(bip_send_socket);
        return status;
    }

    mtu[0] = 0x81; /* BVLL for BACnet/IP */
    if (bip_dest->sin_addr.s_addr == BIP_Broadcast_Address.s_addr)
        mtu[1] = 0x0B; /* Original-Broadcast-NPDU */
    else
        mtu[1] = 0x0A; /* Original-Unicast-NPDU */
    mtu_len = 2;
    mtu_len += encode_unsigned16(&mtu[mtu_len], pdu_len);
    memcpy(&mtu[mtu_len], pdu, pdu_len);
    mtu_len += pdu_len;
    
    /* Send the packet */
    rv = sendto(bip_send_socket, (char *)mtu, mtu_len, 0, 
        (struct sockaddr *)bip_dest, 
        sizeof(struct sockaddr));
    if (rv >= 0)
      status = 0;

    close(bip_send_socket);
    
    return status;
}

/* function to send a packet out the BACnet/IP socket (Annex J) */
/* returns zero on success, non-zero on failure */
int bip_send_pdu(
  BACNET_ADDRESS *dest,  // destination address
  uint8_t *pdu, // any data to be sent - may be null
  unsigned pdu_len) // number of bytes of data
{
    int i = 0; // counter
    struct sockaddr_in bip_dest;
    uint32_t network_address = 0;
    uint16_t network_port = 0;

    /* load destination IP address */
    bip_dest.sin_family = AF_INET;
    if (dest->mac_len == 6)
    {
        (void)decode_unsigned32(&dest->mac[0], &(bip_dest.sin_addr.s_addr));
        (void)decode_unsigned16(&dest->mac[4], &(bip_dest.sin_port));
        memset(&(bip_dest.sin_zero), '\0', 8);
    }
    /* broadcast */
    else if (dest->mac_len == 0)
    {
        bip_dest.sin_addr.s_addr = BIP_Broadcast_Address.s_addr;
        bip_dest.sin_port = BIP_Port;
        memset(&(bip_dest.sin_zero), '\0', 8);
    }
    else
        return -1;

    /* function to send a packet out the BACnet/IP socket */
    /* returns 1 on success, 0 on failure */
    return bip_send(&bip_dest,  // destination address
        pdu, // any data to be sent - may be null
        pdu_len); // number of bytes of data
}

// receives a BACnet/IP packet
// returns the number of octets in the PDU, or zero on failure
uint16_t bip_receive(
  BACNET_ADDRESS *src,  // source address
  uint8_t *pdu, // PDU data
  uint16_t max_pdu, // amount of space available in the PDU 
  unsigned timeout) // number of milliseconds to wait for a packet
{
    int received_bytes;
    uint8_t buf[MAX_MPDU] = {0}; // data
    uint16_t pdu_len = 0; // return value
    fd_set read_fds;
    int max;
    struct timeval select_timeout;
    struct sockaddr_in sin = {-1};
    int sin_len = sizeof(sin);

    /* Make sure the socket is open */
    if (BIP_Receive_Socket < 0)
        return 0;

    /* we could just use a non-blocking socket, but that consumes all
       the CPU time.  We can use a timeout; it is only supported as
       a select. */
    if (timeout >= 1000)
    {
        select_timeout.tv_sec = timeout / 1000;
        select_timeout.tv_usec = 
          1000 * (timeout - select_timeout.tv_sec * 1000);
    }
    else
    {
        select_timeout.tv_sec = 0;
        select_timeout.tv_usec = 1000 * timeout;
    }
    FD_ZERO(&read_fds);
    FD_SET(BIP_Receive_Socket, &read_fds);
    max = BIP_Receive_Socket;
    /* see if there is a packet for us */
    if (select(max + 1, &read_fds, NULL, NULL, &select_timeout) > 0)
        received_bytes = recvfrom(BIP_Receive_Socket, 
            (char *)&buf[0], MAX_MPDU, 0,
            (struct sockaddr *)&sin, &sin_len);
    else
        return 0;

    /* See if there is a problem */
    if (received_bytes < 0) {
        return 0;
    }

    /* no problem, just no bytes */
    if (received_bytes == 0)
        return 0;
    
    /* the signature of a BACnet/IP packet */
    if (buf[0] != 0x81)
        return 0;
    /* Original-Broadcast-NPDU or Original-Unicast-NPDU */
    if ((buf[1] == 0x0B) || (buf[1] == 0x0A))
    {
        // copy the source address
        src->mac_len = 6;
        (void)encode_unsigned32(&src->mac[0], 
            sin.sin_addr.s_addr);
        (void)encode_unsigned16(&src->mac[4], 
            sin.sin_port);
        // FIXME: check destination address
        // see if it is broadcast or for us
        /* decode the length of the PDU - length is inclusive of BVLC */
        (void)decode_unsigned16(&buf[2],&pdu_len);
        /* copy the buffer into the PDU */
        pdu_len -= 4; /* BVLC header */
        if (pdu_len < max_pdu)
            memmove(&pdu[0],&buf[4],pdu_len);
        // ignore packets that are too large
        // clients should check my max-apdu first
        else
            pdu_len = 0;
    }

    return pdu_len;
}

void bip_get_my_address(BACNET_ADDRESS *my_address)
{
  int i = 0;
  
  my_address->mac_len = 6;
  (void)encode_unsigned32(&my_address->mac[0], 
    BIP_Address.s_addr);
  (void)encode_unsigned16(&my_address->mac[4], 
    BIP_Port);
  my_address->net = 0; /* local only, no routing */
  my_address->len = 0; /* no SLEN */
  for (i = 0; i < MAX_MAC_LEN; i++)
  {
    /* no SADR */
    my_address->adr[i] = 0;
  }

  return;
}

void bip_get_broadcast_address(
  BACNET_ADDRESS *dest)  // destination address
{
  int i = 0; // counter
  
  if (dest)
  {
    dest->mac_len = 6;
    (void)encode_unsigned32(&dest->mac[0],
      BIP_Broadcast_Address.s_addr);
    (void)encode_unsigned16(&dest->mac[4], 
      BIP_Port);
    dest->net = BACNET_BROADCAST_NETWORK;
    dest->len = 0; /* no SLEN */
    for (i = 0; i < MAX_MAC_LEN; i++)
    {
      /* no SADR */
      dest->adr[i] = 0;
    }
  }

  return;
}
