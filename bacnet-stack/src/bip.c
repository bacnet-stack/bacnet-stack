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
#include "bacint.h"
#include "bip.h"
#include "net.h"        /* custom per port */

static int BIP_Socket = -1;
/* port to use - stored in host byte order */
static uint16_t BIP_Port = 0xBAC0;
/* IP Address - stored in host byte order */
static struct in_addr BIP_Address;
/* Broadcast Address - stored in host byte order */
static struct in_addr BIP_Broadcast_Address;

void bip_set_socket(
    int sock_fd)
{
    BIP_Socket = sock_fd;
}

int bip_socket(
    void)
{
    return BIP_Socket;
}

bool bip_valid(
    void)
{
    return (BIP_Socket != -1);
}

void bip_cleanup(
    void)
{
    if (bip_valid())
        close(BIP_Socket);
    BIP_Socket = -1;

    return;
}

/* set using network byte order */
void bip_set_addr(
    uint32_t net_address)
{
    BIP_Address.s_addr = ntohl(net_address);
}

/* returns host byte order */
uint32_t bip_get_addr(
    void)
{
    return BIP_Address.s_addr;
}

/* set using network byte order */
void bip_set_broadcast_addr(
    uint32_t net_address)
{
    BIP_Broadcast_Address.s_addr = ntohl(net_address);
}

/* returns host byte order */
uint32_t bip_get_broadcast_addr(
    void)
{
    return BIP_Broadcast_Address.s_addr;
}

/* set using host byte order */
void bip_set_port(
    uint16_t port)
{
    BIP_Port = port;
}

/* returns host byte order */
uint16_t bip_get_port(
    void)
{
    return BIP_Port;
}

/* function to send a packet out the BACnet/IP socket (Annex J) */
/* returns number of bytes sent on success, negative number on failure */
int bip_send_pdu(
    BACNET_ADDRESS * dest,      /* destination address */
    BACNET_NPDU_DATA * npdu_data,       /* network information */
    uint8_t * pdu,      /* any data to be sent - may be null */
    unsigned pdu_len)
{       /* number of bytes of data */
    struct sockaddr_in bip_dest;
    uint8_t mtu[MAX_MPDU] = { 0 };
    int mtu_len = 0;
    int bytes_sent = 0;

    (void) npdu_data;
    /* assumes that the driver has already been initialized */
    if (BIP_Socket < 0)
        return BIP_Socket;

    mtu[0] = BVLL_TYPE_BACNET_IP;
    bip_dest.sin_family = AF_INET;
    if (dest->net == BACNET_BROADCAST_NETWORK) {
        /* broadcast */
        bip_dest.sin_addr.s_addr = htonl(BIP_Broadcast_Address.s_addr);
        bip_dest.sin_port = htons(BIP_Port);
        memset(&(bip_dest.sin_zero), '\0', 8);
        mtu[1] = BVLC_ORIGINAL_BROADCAST_NPDU;
    } else if (dest->mac_len == 6) {
        /* valid unicast */
        (void) decode_unsigned32(&dest->mac[0],
            (uint32_t *) & (bip_dest.sin_addr.s_addr));
        (void) decode_unsigned16(&dest->mac[4], &(bip_dest.sin_port));
        memset(&(bip_dest.sin_zero), '\0', 8);
        mtu[1] = BVLC_ORIGINAL_UNICAST_NPDU;
    } else {
        /* invalid address */
        return -1;
    }

    mtu_len = 2;
    mtu_len +=
        encode_unsigned16(&mtu[mtu_len],
        (uint16_t) (pdu_len + 4 /*inclusive */ ));
    memcpy(&mtu[mtu_len], pdu, pdu_len);
    mtu_len += pdu_len;

    /* Send the packet */
    bytes_sent = sendto(BIP_Socket, (char *) mtu, mtu_len, 0,
        (struct sockaddr *) &bip_dest, sizeof(struct sockaddr));

    return bytes_sent;
}

/* receives a BACnet/IP packet */
/* returns the number of octets in the PDU, or zero on failure */
uint16_t bip_receive(
    BACNET_ADDRESS * src,       /* source address */
    uint8_t * pdu,      /* PDU data */
    uint16_t max_pdu,   /* amount of space available in the PDU  */
    unsigned timeout)
{       /* number of milliseconds to wait for a packet */
    int received_bytes = 0;
    uint16_t pdu_len = 0;       /* return value */
    fd_set read_fds;
    int max = 0;
    struct timeval select_timeout;
    struct sockaddr_in sin = { -1 };
    socklen_t sin_len = sizeof(sin);
    unsigned i = 0;

    /* Make sure the socket is open */
    if (BIP_Socket < 0)
        return 0;

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
    FD_SET((unsigned int) BIP_Socket, &read_fds);
    max = BIP_Socket;
    /* see if there is a packet for us */
    if (select(max + 1, &read_fds, NULL, NULL, &select_timeout) > 0)
        received_bytes = recvfrom(BIP_Socket,
            (char *) &pdu[0], max_pdu, 0, (struct sockaddr *) &sin, &sin_len);
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
    if (pdu[0] != BVLL_TYPE_BACNET_IP)
        return 0;
    if ((pdu[1] == BVLC_ORIGINAL_UNICAST_NPDU) ||
        (pdu[1] == BVLC_ORIGINAL_BROADCAST_NPDU)) {
        /* ignore messages from me */
        if ((sin.sin_addr.s_addr == htonl(BIP_Address.s_addr)) &&
            (sin.sin_port == htonl(BIP_Port))) {
            pdu_len = 0;
        } else {
            /* copy the source address
               FIXME: IPv6? */
            src->mac_len = 6;
            (void) encode_unsigned32(&src->mac[0], sin.sin_addr.s_addr);
            (void) encode_unsigned16(&src->mac[4], sin.sin_port);
            /* FIXME: check destination address */
            /* see if it is broadcast or for us */
            /* decode the length of the PDU - length is inclusive of BVLC */
            (void) decode_unsigned16(&pdu[2], &pdu_len);
            /* subtract off the BVLC header */
            pdu_len -= 4;
            if (pdu_len < max_pdu) {
                /* shift the buffer to return a valid PDU */
                for (i = 0; i < pdu_len; i++) {
                    pdu[i] = pdu[i + 4];
                }
            }
            /* ignore packets that are too large */
            /* clients should check my max-apdu first */
            else
                pdu_len = 0;
        }
    }

    return pdu_len;
}

void bip_get_my_address(
    BACNET_ADDRESS * my_address)
{
    int i = 0;

    my_address->mac_len = 6;
    (void) encode_unsigned32(&my_address->mac[0], htonl(BIP_Address.s_addr));
    (void) encode_unsigned16(&my_address->mac[4], htons(BIP_Port));
    my_address->net = 0;        /* local only, no routing */
    my_address->len = 0;        /* no SLEN */
    for (i = 0; i < MAX_MAC_LEN; i++) {
        /* no SADR */
        my_address->adr[i] = 0;
    }

    return;
}

void bip_get_broadcast_address(
    BACNET_ADDRESS * dest)
{       /* destination address */
    int i = 0;  /* counter */

    if (dest) {
        dest->mac_len = 6;
        (void) encode_unsigned32(&dest->mac[0],
            htonl(BIP_Broadcast_Address.s_addr));
        (void) encode_unsigned16(&dest->mac[4], htons(BIP_Port));
        dest->net = BACNET_BROADCAST_NETWORK;
        dest->len = 0;  /* no SLEN */
        for (i = 0; i < MAX_MAC_LEN; i++) {
            /* no SADR */
            dest->adr[i] = 0;
        }
    }

    return;
}
