/*####COPYRIGHTBEGIN####
 -------------------------------------------
 Copyright (C) 2006 Steve Karg

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
#include <time.h>
#include "bacenum.h"
#include "bacdcode.h"
#include "bacint.h"
#include "bvlc.h"
#include "bip.h"
#ifndef DEBUG_ENABLED
#define DEBUG_ENABLED 0
#endif
#include "debug.h"
#include "session.h"
#include "bvlc-core.h"
#include "net.h"

/** @file bvlc.c  Handle the BACnet Virtual Link Control (BVLC) */

#if 0
static BBMD_TABLE_ENTRY session_object->BVLC_BBMD_Table[MAX_BBMD_ENTRIES];
static FD_TABLE_ENTRY session_object->BVLC_FD_Table[MAX_FD_ENTRIES];
/* result from a client request */
BACNET_BVLC_RESULT BVLC_Result_Code = BVLC_RESULT_SUCCESSFUL_COMPLETION;
/* if we are a foreign device, store the
   remote BBMD address/port here in network byte order */
static struct sockaddr_in session_object->BVLC_Remote_BBMD;
#endif

#if defined(BBMD_ENABLED) && BBMD_ENABLED
void bvlc_maintenance_timer(
    struct bacnet_session_object *session_object,
    time_t seconds)
{
    unsigned i = 0;

    for (i = 0; i < MAX_FD_ENTRIES; i++) {
        if (session_object->BVLC_FD_Table[i].valid) {
            if (session_object->BVLC_FD_Table[i].seconds_remaining) {
                if (session_object->BVLC_FD_Table[i].seconds_remaining <
                    seconds) {
                    session_object->BVLC_FD_Table[i].seconds_remaining = 0;
                } else {
                    session_object->BVLC_FD_Table[i].seconds_remaining -=
                        seconds;
                }
                if (session_object->BVLC_FD_Table[i].seconds_remaining == 0) {
                    session_object->BVLC_FD_Table[i].valid = false;
                }
            }
        }
    }
}
#endif /*defined(BBMD_ENABLED) && BBMD_ENABLED */

/* Addressing within B/IP Networks
   In the case of B/IP networks, six octets consisting of the four-octet
   IP address followed by a two-octet UDP port number (both of
   which shall be transmitted most significant octet first).
   Note: for local storage, the storage order is host byte order.
   Note: BACnet unsigned is encoded as most significant octet. */
static int bvlc_encode_bip_address(
    uint8_t * pdu,      /* buffer to store encoding */
    struct in_addr *address,    /* in host format */
    uint16_t port)
{
    int len = 0;

    if (pdu) {
        encode_unsigned32(&pdu[0], address->s_addr);
        encode_unsigned16(&pdu[4], port);
        len = 6;
    }

    return len;
}

static int bvlc_decode_bip_address(
    uint8_t * pdu,      /* buffer to extract encoded address */
    struct in_addr *address,    /* in host format */
    uint16_t * port)
{
    int len = 0;
    uint32_t raw_address = 0;

    if (pdu) {
        (void) decode_unsigned32(&pdu[0], &raw_address);
        address->s_addr = raw_address;
        (void) decode_unsigned16(&pdu[4], port);
        len = 6;
    }

    return len;
}

/* used for both read and write entries */
static int bvlc_encode_address_entry(
    uint8_t * pdu,
    struct in_addr *address,
    uint16_t port,
    struct in_addr *mask)
{
    int len = 0;

    if (pdu) {
        len = bvlc_encode_bip_address(pdu, address, port);
        len += encode_unsigned32(&pdu[len], mask->s_addr);
    }

    return len;
}

static int bvlc_encode_bvlc_result(
    uint8_t * pdu,
    BACNET_BVLC_RESULT result_code)
{
    if (pdu) {
        pdu[0] = BVLL_TYPE_BACNET_IP;
        pdu[1] = BVLC_RESULT;
        /* The 2-octet BVLC Length field is the length, in octets,
           of the entire BVLL message, including the two octets of the
           length field itself, most significant octet first. */
        encode_unsigned16(&pdu[2], 6);
        encode_unsigned16(&pdu[4], (uint16_t) result_code);
    }

    return 6;
}

int bvlc_encode_write_bdt_init(
    uint8_t * pdu,
    unsigned entries)
{
    int len = 0;
    uint16_t BVLC_length = 0;

    if (pdu) {
        pdu[0] = BVLL_TYPE_BACNET_IP;
        pdu[1] = BVLC_WRITE_BROADCAST_DISTRIBUTION_TABLE;
        /* The 2-octet BVLC Length field is the length, in octets,
           of the entire BVLL message, including the two octets of the
           length field itself, most significant octet first. */
        BVLC_length = 4 + (uint16_t) (entries * 10);
        encode_unsigned16(&pdu[2], BVLC_length);
        len = 4;
    }

    return len;
}

int bvlc_encode_read_bdt(
    uint8_t * pdu)
{
    int len = 0;

    if (pdu) {
        pdu[0] = BVLL_TYPE_BACNET_IP;
        pdu[1] = BVLC_READ_BROADCAST_DIST_TABLE;
        /* The 2-octet BVLC Length field is the length, in octets,
           of the entire BVLL message, including the two octets of the
           length field itself, most significant octet first. */
        encode_unsigned16(&pdu[2], 4);
        len = 4;
    }

    return len;
}

static int bvlc_encode_read_bdt_ack_init(
    uint8_t * pdu,
    unsigned entries)
{
    int len = 0;
    uint16_t BVLC_length = 0;

    if (pdu) {
        pdu[0] = BVLL_TYPE_BACNET_IP;
        pdu[1] = BVLC_READ_BROADCAST_DIST_TABLE_ACK;
        /* The 2-octet BVLC Length field is the length, in octets,
           of the entire BVLL message, including the two octets of the
           length field itself, most significant octet first. */
        BVLC_length = 4 + (uint16_t) (entries * 10);
        encode_unsigned16(&pdu[2], BVLC_length);
        len = 4;
    }

    return len;
}

#if defined(BBMD_ENABLED) && BBMD_ENABLED

static int bvlc_encode_read_bdt_ack(
    struct bacnet_session_object *session_object,
    uint8_t * pdu,
    uint16_t max_pdu)
{
    int pdu_len = 0;    /* return value */
    int len = 0;
    unsigned count = 0;
    unsigned i;

    for (i = 0; i < MAX_BBMD_ENTRIES; i++) {
        if (session_object->BVLC_BBMD_Table[i].valid) {
            count++;
        }
    }
    len = bvlc_encode_read_bdt_ack_init(&pdu[0], count);
    pdu_len += len;
    for (i = 0; i < MAX_BBMD_ENTRIES; i++) {
        if (session_object->BVLC_BBMD_Table[i].valid) {
            /* too much to send */
            if ((pdu_len + 10) > max_pdu) {
                pdu_len = 0;
                break;
            }
            len =
                bvlc_encode_address_entry(&pdu[pdu_len],
                &session_object->BVLC_BBMD_Table[i].dest_address,
                session_object->BVLC_BBMD_Table[i].dest_port,
                &session_object->BVLC_BBMD_Table[i].broadcast_mask);
            pdu_len += len;
        }
    }

    return pdu_len;
}
#endif /*defined(BBMD_ENABLED) && BBMD_ENABLED */

static int bvlc_encode_forwarded_npdu(
    uint8_t * pdu,
    struct sockaddr_in *sin,    /* source address in network order */
    uint8_t * npdu,
    unsigned npdu_length)
{
    int len = 0;
    struct in_addr address;
    uint16_t port;

    unsigned i; /* for loop counter */

    if (pdu) {
        pdu[0] = BVLL_TYPE_BACNET_IP;
        pdu[1] = BVLC_FORWARDED_NPDU;
        /* The 2-octet BVLC Length field is the length, in octets,
           of the entire BVLL message, including the two octets of the
           length field itself, most significant octet first. */
        encode_unsigned16(&pdu[2], (uint16_t) (4 + 6 + npdu_length));
        len = 4;
        address.s_addr = ntohl(sin->sin_addr.s_addr);
        port = ntohs(sin->sin_port);
        len += bvlc_encode_bip_address(&pdu[len], &address, port);
        for (i = 0; i < npdu_length; i++) {
            pdu[len] = npdu[i];
            len++;
        }
    }

    return len;
}

static int bvlc_encode_register_foreign_device(
    uint8_t * pdu,
    uint16_t time_to_live_seconds)
{
    int len = 0;

    if (pdu) {
        pdu[0] = BVLL_TYPE_BACNET_IP;
        pdu[1] = BVLC_REGISTER_FOREIGN_DEVICE;
        /* The 2-octet BVLC Length field is the length, in octets,
           of the entire BVLL message, including the two octets of the
           length field itself, most significant octet first. */
        encode_unsigned16(&pdu[2], 6);
        encode_unsigned16(&pdu[4], time_to_live_seconds);
        len = 6;
    }

    return len;
}

int bvlc_encode_read_fdt(
    uint8_t * pdu)
{
    int len = 0;

    if (pdu) {
        pdu[0] = BVLL_TYPE_BACNET_IP;
        pdu[1] = BVLC_READ_FOREIGN_DEVICE_TABLE;
        /* The 2-octet BVLC Length field is the length, in octets,
           of the entire BVLL message, including the two octets of the
           length field itself, most significant octet first. */
        encode_unsigned16(&pdu[2], 4);
        len = 4;
    }

    return len;
}

static int bvlc_encode_read_fdt_ack_init(
    uint8_t * pdu,
    unsigned entries)
{
    int len = 0;
    uint16_t BVLC_length = 0;

    if (pdu) {
        pdu[0] = BVLL_TYPE_BACNET_IP;
        pdu[1] = BVLC_READ_FOREIGN_DEVICE_TABLE_ACK;
        /* The 2-octet BVLC Length field is the length, in octets,
           of the entire BVLL message, including the two octets of the
           length field itself, most significant octet first. */
        BVLC_length = 4 + (uint16_t) (entries * 10);
        encode_unsigned16(&pdu[2], BVLC_length);
        len = 4;
    }

    return len;
}

#if defined(BBMD_ENABLED) && BBMD_ENABLED

static int bvlc_encode_read_fdt_ack(
    struct bacnet_session_object *session_object,
    uint8_t * pdu,
    uint16_t max_pdu)
{
    int pdu_len = 0;    /* return value */
    int len = 0;
    unsigned count = 0;
    unsigned i;
    uint16_t seconds_remaining = 0;

    for (i = 0; i < MAX_FD_ENTRIES; i++) {
        if (session_object->BVLC_FD_Table[i].valid) {
            count++;
        }
    }
    len = bvlc_encode_read_fdt_ack_init(&pdu[0], count);
    pdu_len += len;
    for (i = 0; i < MAX_FD_ENTRIES; i++) {
        if (session_object->BVLC_FD_Table[i].valid) {
            /* too much to send */
            if ((pdu_len + 10) > max_pdu) {
                pdu_len = 0;
                break;
            }
            len =
                bvlc_encode_bip_address(&pdu[pdu_len],
                &session_object->BVLC_FD_Table[i].dest_address,
                session_object->BVLC_FD_Table[i].dest_port);
            pdu_len += len;
            len =
                encode_unsigned16(&pdu[pdu_len],
                session_object->BVLC_FD_Table[i].time_to_live);
            pdu_len += len;
            /* The ASHRAE Norm says: 65535 maximum */
            seconds_remaining =
                (uint16_t) session_object->BVLC_FD_Table[i].seconds_remaining;
            len = encode_unsigned16(&pdu[pdu_len], seconds_remaining);
            pdu_len += len;
        }
    }

    return pdu_len;
}
#endif /*defined(BBMD_ENABLED) && BBMD_ENABLED */

int bvlc_encode_delete_fdt_entry(
    uint8_t * pdu,
    struct in_addr *address,
    uint16_t port)
{
    int len = 0;

    if (pdu) {
        pdu[0] = BVLL_TYPE_BACNET_IP;
        pdu[1] = BVLC_READ_FOREIGN_DEVICE_TABLE;
        /* The 2-octet BVLC Length field is the length, in octets,
           of the entire BVLL message, including the two octets of the
           length field itself, most significant octet first. */
        encode_unsigned16(&pdu[2], 10);
        /* FDT Entry */
        encode_unsigned32(&pdu[4], address->s_addr);
        encode_unsigned16(&pdu[8], port);
        len = 10;
    }

    return len;
}

int bvlc_encode_original_unicast_npdu(
    uint8_t * pdu,
    uint8_t * npdu,
    unsigned npdu_length)
{
    int len = 0;        /* return value */
    unsigned i = 0;     /* loop counter */
    uint16_t BVLC_length = 0;

    if (pdu) {
        pdu[0] = BVLL_TYPE_BACNET_IP;
        pdu[1] = BVLC_ORIGINAL_UNICAST_NPDU;
        /* The 2-octet BVLC Length field is the length, in octets,
           of the entire BVLL message, including the two octets of the
           length field itself, most significant octet first. */
        BVLC_length = 4 + (uint16_t) npdu_length;
        len = encode_unsigned16(&pdu[2], BVLC_length) + 2;
        for (i = 0; i < npdu_length; i++) {
            pdu[len] = npdu[i];
            len++;
        }
    }

    return len;
}

int bvlc_encode_original_broadcast_npdu(
    uint8_t * pdu,
    uint8_t * npdu,
    unsigned npdu_length)
{
    int len = 0;        /* return value */
    unsigned i = 0;     /* loop counter */
    uint16_t BVLC_length = 0;

    if (pdu) {
        pdu[0] = BVLL_TYPE_BACNET_IP;
        pdu[1] = BVLC_ORIGINAL_BROADCAST_NPDU;
        /* The 2-octet BVLC Length field is the length, in octets,
           of the entire BVLL message, including the two octets of the
           length field itself, most significant octet first. */
        BVLC_length = 4 + (uint16_t) npdu_length;
        len = encode_unsigned16(&pdu[2], BVLC_length) + 2;
        for (i = 0; i < npdu_length; i++) {
            pdu[len] = npdu[i];
            len++;
        }
    }

    return len;
}

/* copy the source internet address to the BACnet address */
/* FIXME: IPv6? */
static void bvlc_internet_to_bacnet_address(
    BACNET_ADDRESS * src,       /* returns the BACnet source address */
    struct sockaddr_in *sin)
{       /* source address in network order */
    int len = 0;
    uint32_t address;
    uint16_t port;

    if (src && sin) {
        address = ntohl(sin->sin_addr.s_addr);
        len = encode_unsigned32(&src->mac[0], address);
        port = ntohs(sin->sin_port);
        len += encode_unsigned16(&src->mac[4], port);
        src->mac_len = (uint8_t) len;
        src->net = 0;
        src->len = 0;
    }

    return;
}

/* copy the source internet address to the BACnet address */
/* FIXME: IPv6? */
void bvlc_bacnet_to_internet_address(
    struct sockaddr_in *sin,    /* source address in network order */
    BACNET_ADDRESS * src)
{       /* returns the BACnet source address */
    int len = 0;
    uint32_t address;
    uint16_t port;

    if (src && sin) {
        if (src->mac_len == 6) {
            len = decode_unsigned32(&src->mac[0], &address);
            len += decode_unsigned16(&src->mac[4], &port);
            sin->sin_addr.s_addr = htonl(address);
            sin->sin_port = htons(port);
        }
    }

    return;
}

#if defined(BBMD_ENABLED) && BBMD_ENABLED

static bool bvlc_create_bdt(
    struct bacnet_session_object *session_object,
    uint8_t * npdu,
    uint16_t npdu_length)
{
    bool status = false;
    unsigned i = 0;
    uint16_t pdu_offset = 0;

    for (i = 0; i < MAX_BBMD_ENTRIES; i++) {
        if (npdu_length >= 10) {
            session_object->BVLC_BBMD_Table[i].valid = true;
            session_object->BVLC_BBMD_Table[i].dest_address.s_addr =
                ntohl(*(long *) &npdu[pdu_offset]);
            pdu_offset += 4;
            session_object->BVLC_BBMD_Table[i].dest_port =
                ntohs(*(short *) &npdu[pdu_offset]);
            pdu_offset += 2;
            session_object->BVLC_BBMD_Table[i].broadcast_mask.s_addr =
                ntohl(*(long *) &npdu[pdu_offset]);
            pdu_offset += 4;
            npdu_length -= 10;
        } else {
            session_object->BVLC_BBMD_Table[i].valid = false;
            session_object->BVLC_BBMD_Table[i].dest_address.s_addr = 0;
            session_object->BVLC_BBMD_Table[i].dest_port = 0;
            session_object->BVLC_BBMD_Table[i].broadcast_mask.s_addr = 0;
        }
    }
    /* did they all fit? */
    if (npdu_length < 10) {
        status = true;
    }

    return status;
}

static bool bvlc_register_foreign_device(
    struct bacnet_session_object *session_object,
    struct sockaddr_in *sin,    /* source address in network order */
    uint16_t time_to_live)
{       /* time in seconds */
    unsigned i = 0;
    bool status = false;

    /* am I here already?  If so, update my time to live... */
    for (i = 0; i < MAX_FD_ENTRIES; i++) {
        if (session_object->BVLC_FD_Table[i].valid) {
            if ((session_object->BVLC_FD_Table[i].dest_address.s_addr ==
                    ntohl(sin->sin_addr.s_addr)) &&
                (session_object->BVLC_FD_Table[i].dest_port ==
                    ntohs(sin->sin_port))) {
                status = true;
                session_object->BVLC_FD_Table[i].time_to_live = time_to_live;
                /*  Upon receipt of a BVLL Register-Foreign-Device message, 
                   a BBMD shall start a timer with a value equal to the 
                   Time-to-Live parameter supplied plus a fixed grace 
                   period of 30 seconds. */
                session_object->BVLC_FD_Table[i].seconds_remaining =
                    time_to_live + 30;
                break;
            }
        }
    }
    if (!status) {
        for (i = 0; i < MAX_FD_ENTRIES; i++) {
            if (!session_object->BVLC_FD_Table[i].valid) {
                session_object->BVLC_FD_Table[i].dest_address.s_addr =
                    ntohl(sin->sin_addr.s_addr);
                session_object->BVLC_FD_Table[i].dest_port =
                    ntohs(sin->sin_port);
                session_object->BVLC_FD_Table[i].time_to_live = time_to_live;
                session_object->BVLC_FD_Table[i].seconds_remaining =
                    time_to_live + 30;
                session_object->BVLC_FD_Table[i].valid = true;
                status = true;
                break;
            }
        }
    }


    return status;
}

static bool bvlc_delete_foreign_device(
    struct bacnet_session_object *session_object,
    uint8_t * pdu)
{
    struct sockaddr_in sin = { 0 };     /* the ip address */
    uint16_t port = 0;  /* the decoded port */
    bool status = false;        /* return value */
    unsigned i = 0;

    bvlc_decode_bip_address(pdu, &sin.sin_addr, &port);
    for (i = 0; i < MAX_FD_ENTRIES; i++) {
        if (session_object->BVLC_FD_Table[i].valid) {
            if ((session_object->BVLC_FD_Table[i].dest_address.s_addr ==
                    sin.sin_addr.s_addr) &&
                (session_object->BVLC_FD_Table[i].dest_port == sin.sin_port)) {
                session_object->BVLC_FD_Table[i].valid = false;
                session_object->BVLC_FD_Table[i].seconds_remaining = 0;
                status = true;
                break;
            }
        }
    }
    return status;
}
#endif /*defined(BBMD_ENABLED) && BBMD_ENABLED */


static int bvlc_send_mpdu(
    struct bacnet_session_object *session_object,
    struct sockaddr_in *dest,   /* the destination address */
    uint8_t * mtu,      /* the data */
    uint16_t mtu_len)
{       /* amount of data to send  */
    struct sockaddr_in bvlc_dest = { 0 };

    /* assumes that the driver has already been initialized */
    if (bip_socket(session_object) < 0) {
        return 0;
    }
    /* load destination IP address */
    bvlc_dest.sin_family = AF_INET;
    bvlc_dest.sin_addr.s_addr = dest->sin_addr.s_addr;
    bvlc_dest.sin_port = dest->sin_port;
    memset(&(bvlc_dest.sin_zero), '\0', 8);
    /* Send the packet */
    return sendto(bip_socket(session_object), (char *) mtu, mtu_len, 0,
        (struct sockaddr *) &bvlc_dest, sizeof(struct sockaddr));
}

#if defined(BBMD_ENABLED) && BBMD_ENABLED

static void bvlc_bdt_forward_npdu(
    struct bacnet_session_object *session_object,
    struct sockaddr_in *sin,    /* source address in network order */
    uint8_t * npdu,     /* the NPDU */
    uint16_t npdu_length)
{       /* length of the NPDU  */
    uint8_t mtu[MAX_MPDU] = { 0 };
    uint16_t mtu_len = 0;
    unsigned i = 0;     /* loop counter */
    struct sockaddr_in bip_dest = { 0 };

    mtu_len = bvlc_encode_forwarded_npdu(&mtu[0], sin, npdu, npdu_length);
    /* loop through the BDT and send one to each entry, except us */
    for (i = 0; i < MAX_BBMD_ENTRIES; i++) {
        if (session_object->BVLC_BBMD_Table[i].valid) {
            /* The B/IP address to which the Forwarded-NPDU message is
               sent is formed by inverting the broadcast distribution
               mask in the BDT entry and logically ORing it with the
               BBMD address of the same entry. */
            bip_dest.sin_addr.s_addr =
                htonl(((~session_object->BVLC_BBMD_Table[i].broadcast_mask.
                        s_addr) | session_object->BVLC_BBMD_Table[i].
                    dest_address.s_addr));
            bip_dest.sin_port =
                htons(session_object->BVLC_BBMD_Table[i].dest_port);
            /* don't send to my broadcast address and same port */
            if ((bip_dest.sin_addr.s_addr ==
                    htonl(bip_get_broadcast_addr(session_object)))
                && (bip_dest.sin_port == htons(bip_get_port(session_object)))) {
                continue;
            }
            /* don't send to my ip address and same port */
            if ((bip_dest.sin_addr.s_addr ==
                    htonl(bip_get_addr(session_object))) &&
                (bip_dest.sin_port == htons(bip_get_port(session_object)))) {
                continue;
            }
            bvlc_send_mpdu(session_object, &bip_dest, mtu, mtu_len);
            debug_printf("BVLC: BDT Sent Forwarded-NPDU to %s:%04X\n",
                inet_ntoa(bip_dest.sin_addr), ntohs(bip_dest.sin_port));
        }
    }

    return;
}
#endif /*defined(BBMD_ENABLED) && BBMD_ENABLED */

/* Generate BVLL Forwarded-NPDU message on its local IP subnet using
   the local B/IP broadcast address as the destination address.  */
static void bvlc_forward_npdu(
    struct bacnet_session_object *session_object,
    struct sockaddr_in *sin,    /* source address in network order */
    uint8_t * npdu,     /* the NPDU */
    uint16_t npdu_length)
{       /* length of the NPDU  */
    uint8_t mtu[MAX_MPDU] = { 0 };
    uint16_t mtu_len = 0;
    struct sockaddr_in bip_dest = { 0 };

    mtu_len = bvlc_encode_forwarded_npdu(&mtu[0], sin, npdu, npdu_length);
    bip_dest.sin_addr.s_addr = htonl(bip_get_broadcast_addr(session_object));
    bip_dest.sin_port = htons(bip_get_port(session_object));
    bvlc_send_mpdu(session_object, &bip_dest, mtu, mtu_len);
    debug_printf("BVLC: Sent Forwarded-NPDU as local broadcast.\n");
}

#if defined(BBMD_ENABLED) && BBMD_ENABLED

static void bvlc_fdt_forward_npdu(
    struct bacnet_session_object *session_object,
    struct sockaddr_in *sin,    /* source address in network order */
    uint8_t * npdu,     /* returns the NPDU */
    uint16_t max_npdu)
{       /* amount of space available in the NPDU  */
    uint8_t mtu[MAX_MPDU] = { 0 };
    uint16_t mtu_len = 0;
    unsigned i = 0;     /* loop counter */
    struct sockaddr_in bip_dest = { 0 };

    mtu_len = bvlc_encode_forwarded_npdu(&mtu[0], sin, npdu, max_npdu);
    /* loop through the FDT and send one to each entry */
    for (i = 0; i < MAX_FD_ENTRIES; i++) {
        if (session_object->BVLC_FD_Table[i].valid &&
            session_object->BVLC_FD_Table[i].seconds_remaining) {
            bip_dest.sin_addr.s_addr =
                htonl(session_object->BVLC_FD_Table[i].dest_address.s_addr);
            bip_dest.sin_port =
                htons(session_object->BVLC_FD_Table[i].dest_port);
            /* don't send to my ip address and same port */
            if ((bip_dest.sin_addr.s_addr ==
                    htonl(bip_get_addr(session_object))) &&
                (bip_dest.sin_port == htons(bip_get_port(session_object)))) {
                continue;
            }
            /* don't send to src ip address and same port */
            if ((bip_dest.sin_addr.s_addr == sin->sin_addr.s_addr) &&
                (bip_dest.sin_port == sin->sin_port)) {
                continue;
            }
            bvlc_send_mpdu(session_object, &bip_dest, mtu, mtu_len);
            debug_printf("BVLC: FDT Sent Forwarded-NPDU to %s:%04X\n",
                inet_ntoa(bip_dest.sin_addr), ntohs(bip_dest.sin_port));
        }
    }

    return;
}
#endif /*defined(BBMD_ENABLED) && BBMD_ENABLED */

/* Send a registration message with bbmd, no context memorized */
void bvlc_send_register_with_bbmd(
    struct bacnet_session_object *session_object,
    BACNET_ADDRESS * dest,
    uint16_t time_to_live_seconds)
{
    uint8_t mtu[MAX_MPDU] = { 0 };
    uint16_t mtu_len = 0;
    struct in_addr address;
    uint16_t port = 0;
    struct sockaddr_in bvlc_bbmd_dest = { 0 };

    if (dest->mac_len == 6) {
        /* valid unicast */
        bvlc_decode_bip_address(&dest->mac[0], &address, &port);
    } else {
        /* invalid address */
        return;
    }
    bvlc_bbmd_dest.sin_addr.s_addr = htonl(address.s_addr);
    bvlc_bbmd_dest.sin_port = htons(port);

    /* In order for their broadcasts to get here,
       we need to register our address with the remote BBMD using
       Write Broadcast Distribution Table, or
       register with the BBMD as a Foreign Device */
    mtu_len =
        bvlc_encode_register_foreign_device(&mtu[0], time_to_live_seconds);
    bvlc_send_mpdu(session_object, &bvlc_bbmd_dest, &mtu[0], mtu_len);

}

void bvlc_register_with_bbmd(
    struct bacnet_session_object *session_object,
    long bbmd_address,  /* in network byte order */
    uint16_t bbmd_port, /* in host byte order */
    uint16_t time_to_live_seconds)
{
    BACNET_ADDRESS dest = { 0 };
    struct in_addr address;
    uint16_t port = 0;

    /* Store the BBMD address and port so that we
       won't broadcast locally. */
    session_object->BVLC_Remote_BBMD.sin_addr.s_addr = bbmd_address;
    session_object->BVLC_Remote_BBMD.sin_port = htons(bbmd_port);

    /* encoded in host byte order ! */
    address.s_addr = ntohl(bbmd_address);
    port = bbmd_port;
    dest.mac_len = 6;
    bvlc_encode_bip_address(&dest.mac[0], &address, port);

    bvlc_send_register_with_bbmd(session_object, &dest, time_to_live_seconds);
}

static void bvlc_send_result(
    struct bacnet_session_object *session_object,
    struct sockaddr_in *dest,   /* the destination address */
    BACNET_BVLC_RESULT result_code)
{
    uint8_t mtu[MAX_MPDU] = { 0 };
    uint16_t mtu_len = 0;

    mtu_len = bvlc_encode_bvlc_result(&mtu[0], result_code);
    bvlc_send_mpdu(session_object, dest, mtu, mtu_len);

    return;
}

#if defined(BBMD_ENABLED) && BBMD_ENABLED

static int bvlc_send_bdt(
    struct bacnet_session_object *session_object,
    struct sockaddr_in *dest)
{
    uint8_t mtu[MAX_MPDU] = { 0 };
    uint16_t mtu_len = 0;

    mtu_len = bvlc_encode_read_bdt_ack(session_object, &mtu[0], sizeof(mtu));
    if (mtu_len) {
        bvlc_send_mpdu(session_object, dest, &mtu[0], mtu_len);
    }

    return mtu_len;
}

static int bvlc_send_fdt(
    struct bacnet_session_object *session_object,
    struct sockaddr_in *dest)
{
    uint8_t mtu[MAX_MPDU] = { 0 };
    uint16_t mtu_len = 0;

    mtu_len = bvlc_encode_read_fdt_ack(session_object, &mtu[0], sizeof(mtu));
    if (mtu_len) {
        bvlc_send_mpdu(session_object, dest, &mtu[0], mtu_len);
    }

    return mtu_len;
}


static bool bvlc_bdt_member_mask_is_unicast(
    struct bacnet_session_object *session_object,
    struct sockaddr_in *sin)
{       /* network order address */
    bool unicast = false;
    unsigned i = 0;     /* loop counter */

    for (i = 0; i < MAX_BBMD_ENTRIES; i++) {
        if (session_object->BVLC_BBMD_Table[i].valid) {
            /* find the source address in the table */
            if ((session_object->BVLC_BBMD_Table[i].dest_address.s_addr ==
                    htonl(sin->sin_addr.s_addr)) &&
                (session_object->BVLC_BBMD_Table[i].dest_port ==
                    htons(sin->sin_port))) {
                /* unicast mask? */
                if (session_object->BVLC_BBMD_Table[i].broadcast_mask.s_addr ==
                    0xFFFFFFFFL) {
                    unicast = true;
                    break;
                }
            }
        }
    }

    return unicast;
}
#endif /*defined(BBMD_ENABLED) && BBMD_ENABLED */

/* returns:
    Number of bytes received, or 0 if none or timeout. */
uint16_t bvlc_receive(
    struct bacnet_session_object * session_object,
    BACNET_ADDRESS * src,       /* returns the source address */
    uint8_t * npdu,     /* returns the NPDU */
    uint16_t max_npdu,  /* amount of space available in the NPDU  */
    unsigned timeout)
{       /* number of milliseconds to wait for a packet */
    uint16_t npdu_len = 0;      /* return value */
    fd_set read_fds;
    int max = 0;
    struct timeval select_timeout;
    struct sockaddr_in sin = { 0 };
    struct sockaddr_in original_sin = { 0 };
    struct sockaddr_in dest = { 0 };
    socklen_t sin_len = sizeof(sin);
    int function_type = 0;
    int received_bytes = 0;
    uint16_t result_code = 0;
    uint16_t i = 0;

    /* Make sure the socket is open */
    if (bip_socket(session_object) < 0) {
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
    FD_SET(bip_socket(session_object), &read_fds);
    max = bip_socket(session_object);
    /* see if there is a packet for us */
    if (select(max + 1, &read_fds, NULL, NULL, &select_timeout) > 0) {
        received_bytes =
            recvfrom(bip_socket(session_object), (char *) &npdu[0], max_npdu,
            0, (struct sockaddr *) &sin, &sin_len);
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
    /* the signature of a BACnet/IP packet */
    if (npdu[0] != BVLL_TYPE_BACNET_IP) {
        return 0;
    }
    function_type = npdu[1];
    /* decode the length of the PDU - length is inclusive of BVLC */
    (void) decode_unsigned16(&npdu[2], &npdu_len);
    /* subtract off the BVLC header */
    npdu_len -= 4;
    switch (function_type) {
        case BVLC_RESULT:
            /* Upon receipt of a BVLC-Result message containing a result code
               of X'0000' indicating the successful completion of the
               registration, a foreign device shall start a timer with a value
               equal to the Time-to-Live parameter of the preceding Register-
               Foreign-Device message. At the expiration of the timer, the
               foreign device shall re-register with the BBMD by sending a BVLL
               Register-Foreign-Device message */
            /* FIXME: clients may need this result */
            (void) decode_unsigned16(&npdu[4], &result_code);
            session_object->BVLC_Result_Code =
                (BACNET_BVLC_RESULT) result_code;
            debug_printf("BVLC: Result Code=%d\n",
                session_object->BVLC_Result_Code);

            /* Recovers source address in a 'standardised form' ... */
            bvlc_internet_to_bacnet_address(src, &sin);
            /* BVLC Callback to upper level client code */
            bvlc_call_result_handler(session_object, src, result_code);

            /* not an NPDU */
            npdu_len = 0;
            break;
        case BVLC_WRITE_BROADCAST_DISTRIBUTION_TABLE:
            debug_printf("BVLC: Received Write-BDT.\n");
#if defined(BBMD_ENABLED) && BBMD_ENABLED
            /* Upon receipt of a BVLL Write-Broadcast-Distribution-Table
               message, a BBMD shall attempt to create or replace its BDT,
               depending on whether or not a BDT has previously existed.
               If the creation or replacement of the BDT is successful, the BBMD
               shall return a BVLC-Result message to the originating device with
               a result code of X'0000'. Otherwise, the BBMD shall return a
               BVLC-Result message to the originating device with a result code
               of X'0010' indicating that the write attempt has failed. */
            status = bvlc_create_bdt(session_object, &npdu[4], npdu_len);
            if (status) {
                bvlc_send_result(session_object, &sin,
                    BVLC_RESULT_SUCCESSFUL_COMPLETION);
            } else {
                bvlc_send_result(session_object, &sin,
                    BVLC_RESULT_WRITE_BROADCAST_DISTRIBUTION_TABLE_NAK);
            }
#endif
            /* not an NPDU */
            npdu_len = 0;
            break;
        case BVLC_READ_BROADCAST_DIST_TABLE:
            debug_printf("BVLC: Received Read-BDT.\n");
#if defined(BBMD_ENABLED) && BBMD_ENABLED
            /* Upon receipt of a BVLL Read-Broadcast-Distribution-Table
               message, a BBMD shall load the contents of its BDT into a BVLL
               Read-Broadcast-Distribution-Table-Ack message and send it to the
               originating device. If the BBMD is unable to perform the
               read of its BDT, it shall return a BVLC-Result message to the
               originating device with a result code of X'0020' indicating that
               the read attempt has failed. */
            if (bvlc_send_bdt(session_object, &sin) <= 0) {
                bvlc_send_result(session_object, &sin,
                    BVLC_RESULT_READ_BROADCAST_DISTRIBUTION_TABLE_NAK);
            }
#endif
            /* not an NPDU */
            npdu_len = 0;
            break;
        case BVLC_READ_BROADCAST_DIST_TABLE_ACK:
            debug_printf("BVLC: Received Read-BDT-Ack.\n");
            /* FIXME: complete the code for client side read */
            /* not an NPDU */
            npdu_len = 0;
            break;
        case BVLC_FORWARDED_NPDU:
            /* Upon receipt of a BVLL Forwarded-NPDU message, a BBMD shall
               process it according to whether it was received from a peer
               BBMD as the result of a directed broadcast or a unicast
               transmission. A BBMD may ascertain the method by which Forwarded-
               NPDU messages will arrive by inspecting the broadcast distribution
               mask field in its own BDT entry since all BDTs are required
               to be identical. If the message arrived via directed broadcast,
               it was also received by the other devices on the BBMD's subnet. In
               this case the BBMD merely retransmits the message directly to each
               foreign device currently in the BBMD's FDT. If the
               message arrived via a unicast transmission it has not yet been
               received by the other devices on the BBMD's subnet. In this case,
               the message is sent to the devices on the BBMD's subnet using the
               B/IP broadcast address as well as to each foreign device
               currently in the BBMD's FDT. A BBMD on a subnet with no other
               BACnet devices may omit the broadcast using the B/IP
               broadcast address. The method by which a BBMD determines whether
               or not other BACnet devices are present is a local matter. */
            /* decode the 4 byte original address and 2 byte port */
            bvlc_decode_bip_address(&npdu[4], &original_sin.sin_addr,
                &original_sin.sin_port);
            npdu_len -= 6;
#if defined(BBMD_ENABLED) && BBMD_ENABLED
            /*  Broadcast locally if received via unicast from a BDT member */
            if (bvlc_bdt_member_mask_is_unicast(session_object, &sin)) {
                dest.sin_addr.s_addr =
                    htonl(bip_get_broadcast_addr(session_object));
                dest.sin_port = htons(bip_get_port(session_object));
                bvlc_send_mpdu(session_object, &dest, &npdu[4 + 6], npdu_len);
            }
#endif
            /* use the original addr from the BVLC for src */
            dest.sin_addr.s_addr = htonl(original_sin.sin_addr.s_addr);
            dest.sin_port = htons(original_sin.sin_port);
#if defined(BBMD_ENABLED) && BBMD_ENABLED
            bvlc_fdt_forward_npdu(session_object, &dest, &npdu[4 + 6],
                npdu_len);
#endif
            debug_printf("BVLC: Received Forwarded-NPDU from %s:%04X.\n",
                inet_ntoa(dest.sin_addr), ntohs(dest.sin_port));
            bvlc_internet_to_bacnet_address(src, &dest);
            if (npdu_len < max_npdu) {
                /* shift the buffer to return a valid PDU */
                for (i = 0; i < npdu_len; i++) {
                    npdu[i] = npdu[4 + 6 + i];
                }
            } else {
                /* ignore packets that are too large */
                /* clients should check my max-apdu first */
                npdu_len = 0;
            }
            break;
        case BVLC_REGISTER_FOREIGN_DEVICE:
#if defined(BBMD_ENABLED) && BBMD_ENABLED
            /* Upon receipt of a BVLL Register-Foreign-Device message, a BBMD
               shall start a timer with a value equal to the Time-to-Live
               parameter supplied plus a fixed grace period of 30 seconds. If,
               within the period during which the timer is active, another BVLL
               Register-Foreign-Device message from the same device is received,
               the timer shall be reset and restarted. If the time expires
               without the receipt of another BVLL Register-Foreign-Device
               message from the same foreign device, the FDT entry for this
               device shall be cleared. */
            (void) decode_unsigned16(&npdu[4], &time_to_live);
            if (bvlc_register_foreign_device(session_object, &sin,
                    time_to_live)) {
                bvlc_send_result(session_object, &sin,
                    BVLC_RESULT_SUCCESSFUL_COMPLETION);
                debug_printf("BVLC: Registered a Foreign Device.\n");
            } else {
                bvlc_send_result(session_object, &sin,
                    BVLC_RESULT_REGISTER_FOREIGN_DEVICE_NAK);
                debug_printf("BVLC: Failed to Register a Foreign Device.\n");
            }
#endif
            /* not an NPDU */
            npdu_len = 0;
            break;
        case BVLC_READ_FOREIGN_DEVICE_TABLE:
            debug_printf("BVLC: Received Read-FDT.\n");
#if defined(BBMD_ENABLED) && BBMD_ENABLED
            /* Upon receipt of a BVLL Read-Foreign-Device-Table message, a
               BBMD shall load the contents of its FDT into a BVLL Read-
               Foreign-Device-Table-Ack message and send it to the originating
               device. If the BBMD is unable to perform the read of its FDT,
               it shall return a BVLC-Result message to the originating device
               with a result code of X'0040' indicating that the read attempt has
               failed. */
            if (bvlc_send_fdt(session_object, &sin) <= 0) {
                bvlc_send_result(session_object, &sin,
                    BVLC_RESULT_READ_FOREIGN_DEVICE_TABLE_NAK);
            }
#endif
            /* not an NPDU */
            npdu_len = 0;
            break;
        case BVLC_READ_FOREIGN_DEVICE_TABLE_ACK:
            debug_printf("BVLC: Received Read-FDT-Ack.\n");
            /* FIXME: complete the code for client side read */
            /* not an NPDU */
            npdu_len = 0;
            break;
        case BVLC_DELETE_FOREIGN_DEVICE_TABLE_ENTRY:
            debug_printf("BVLC: Received Delete-FDT-Entry.\n");
#if defined(BBMD_ENABLED) && BBMD_ENABLED
            /* Upon receipt of a BVLL Delete-Foreign-Device-Table-Entry
               message, a BBMD shall search its foreign device table for an entry
               corresponding to the B/IP address supplied in the message. If an
               entry is found, it shall be deleted and the BBMD shall return a
               BVLC-Result message to the originating device with a result code
               of X'0000'. Otherwise, the BBMD shall return a BVLCResult
               message to the originating device with a result code of X'0050'
               indicating that the deletion attempt has failed. */
            if (bvlc_delete_foreign_device(session_object, &npdu[4])) {
                bvlc_send_result(session_object, &sin,
                    BVLC_RESULT_SUCCESSFUL_COMPLETION);
            } else {
                bvlc_send_result(session_object, &sin,
                    BVLC_RESULT_DELETE_FOREIGN_DEVICE_TABLE_ENTRY_NAK);
            }
#endif
            /* not an NPDU */
            npdu_len = 0;
            break;
        case BVLC_DISTRIBUTE_BROADCAST_TO_NETWORK:
            debug_printf
                ("BVLC: Received Distribute-Broadcast-to-Network from %s:%04X.\n",
                inet_ntoa(sin.sin_addr), ntohs(sin.sin_port));
#if defined(BBMD_ENABLED) && BBMD_ENABLED
            /* Upon receipt of a BVLL Distribute-Broadcast-To-Network message
               from a foreign device, the receiving BBMD shall transmit a
               BVLL Forwarded-NPDU message on its local IP subnet using the
               local B/IP broadcast address as the destination address. In
               addition, a Forwarded-NPDU message shall be sent to each entry
               in its BDT as described in the case of the receipt of a
               BVLL Original-Broadcast-NPDU as well as directly to each foreign
               device currently in the BBMD's FDT except the originating
               node. If the BBMD is unable to perform the forwarding function,
               it shall return a BVLC-Result message to the foreign device
               with a result code of X'0060' indicating that the forwarding
               attempt was unsuccessful */
            bvlc_forward_npdu(session_object, &sin, &npdu[4], npdu_len);
            bvlc_bdt_forward_npdu(session_object, &sin, &npdu[4], npdu_len);
            bvlc_fdt_forward_npdu(session_object, &sin, &npdu[4], npdu_len);
#endif
            /* not an NPDU */
            npdu_len = 0;
            break;
        case BVLC_ORIGINAL_UNICAST_NPDU:
            debug_printf("BVLC: Received Original-Unicast-NPDU.\n");
            /* ignore messages from me */
            if ((sin.sin_addr.s_addr == htonl(bip_get_addr(session_object))) &&
                (sin.sin_port == htons(bip_get_port(session_object)))) {
                npdu_len = 0;
            } else {
                bvlc_internet_to_bacnet_address(src, &sin);
                if (npdu_len < max_npdu) {
                    /* shift the buffer to return a valid PDU */
                    for (i = 0; i < npdu_len; i++) {
                        npdu[i] = npdu[4 + i];
                    }
                } else {
                    /* ignore packets that are too large */
                    /* clients should check my max-apdu first */
                    npdu_len = 0;
                }
            }
            break;
        case BVLC_ORIGINAL_BROADCAST_NPDU:
            debug_printf("BVLC: Received Original-Broadcast-NPDU.\n");
            /* Upon receipt of a BVLL Original-Broadcast-NPDU message,
               a BBMD shall construct a BVLL Forwarded-NPDU message and
               send it to each IP subnet in its BDT with the exception
               of its own. The B/IP address to which the Forwarded-NPDU
               message is sent is formed by inverting the broadcast
               distribution mask in the BDT entry and logically ORing it
               with the BBMD address of the same entry. This process
               produces either the directed broadcast address of the remote
               subnet or the unicast address of the BBMD on that subnet
               depending on the contents of the broadcast distribution
               mask. See J.4.3.2.. In addition, the received BACnet NPDU
               shall be sent directly to each foreign device currently in
               the BBMD's FDT also using the BVLL Forwarded-NPDU message. */
            bvlc_internet_to_bacnet_address(src, &sin);
            if (npdu_len < max_npdu) {
                /* shift the buffer to return a valid PDU */
                for (i = 0; i < npdu_len; i++) {
                    npdu[i] = npdu[4 + i];
                }
#if defined(BBMD_ENABLED) && BBMD_ENABLED
                /* if BDT or FDT entries exist, Forward the NPDU */
                bvlc_bdt_forward_npdu(session_object, &sin, &npdu[0],
                    npdu_len);
                bvlc_fdt_forward_npdu(session_object, &sin, &npdu[0],
                    npdu_len);
#endif
            } else {
                /* ignore packets that are too large */
                npdu_len = 0;
            }
            break;
        default:
            break;
    }

    return npdu_len;
}

/*/ Set result handler function callback */
void bvlc_set_result_handler(
    struct bacnet_session_object *session_object,
    bvlc_result_handler_function result_function)
{
    session_object->BVLC_result_handler = result_function;
}

/*/ Calls result handler function callback, if present */
void bvlc_call_result_handler(
    struct bacnet_session_object *session_object,
    BACNET_ADDRESS * src,
    uint16_t result_code)
{
    if (session_object->BVLC_result_handler)
        (session_object->BVLC_result_handler) (session_object, src,
            result_code);
}

/* Function to send a packet out the BACnet/IP socket (Annex J).
   The interface allows to specify a specific BVLC function number.
   @return number of bytes sent on success, negative number on failure */
int bvlc_send_pdu_function_to_address(
    struct bacnet_session_object *session_object,
    BACNET_ADDRESS * dest,      /* destination address */
    uint8_t bvlc_function_code, /*Function code, like BVLC_ORIGINAL_UNICAST_NPDU */
    BACNET_NPDU_DATA * npdu_data,       /* network information */
    uint8_t * pdu,      /* any data to be sent - may be null */
    unsigned pdu_len)
{       /* number of bytes of data */
    struct sockaddr_in bvlc_dest = { 0 };
    uint8_t mtu[MAX_MPDU] = { 0 };
    uint16_t mtu_len = 0;
    /* addr and port in host format */
    struct in_addr address;
    uint16_t port = 0;
    uint16_t BVLC_length = 0;

    /* bip datalink doesn't need to know the npdu data */
    (void) npdu_data;
    mtu[0] = BVLL_TYPE_BACNET_IP;
    mtu[1] = bvlc_function_code;

    if (dest->mac_len == 6) {
        /* valid unicast, possibly broadcast, or distribute broadcast to network */
        bvlc_decode_bip_address(&dest->mac[0], &address, &port);
    } else {
        /* invalid address */
        return -1;
    }
    bvlc_dest.sin_addr.s_addr = htonl(address.s_addr);
    bvlc_dest.sin_port = htons(port);
    BVLC_length = pdu_len + 4 /*inclusive */ ;
    mtu_len = 2;
    mtu_len += encode_unsigned16(&mtu[mtu_len], BVLC_length);
    memcpy(&mtu[mtu_len], pdu, pdu_len);
    mtu_len += pdu_len;
    return bvlc_send_mpdu(session_object, &bvlc_dest, mtu, mtu_len);
}


/* function to send a packet out the BACnet/IP socket (Annex J) */
/* returns number of bytes sent on success, negative number on failure */
int bvlc_send_pdu(
    struct bacnet_session_object *session_object,
    BACNET_ADDRESS * dest,      /* destination address */
    BACNET_NPDU_DATA * npdu_data,       /* network information */
    uint8_t * pdu,      /* any data to be sent - may be null */
    unsigned pdu_len)
{
    BACNET_ADDRESS alternative_destination = { 0 };
    uint8_t function_code;
    struct in_addr address;
    uint16_t port = 0;

    if (dest->net == BACNET_BROADCAST_NETWORK) {
        /* if we are a foreign device */
        if (session_object->BVLC_Remote_BBMD.sin_port) {
            function_code = BVLC_DISTRIBUTE_BROADCAST_TO_NETWORK;
            address.s_addr =
                ntohl(session_object->BVLC_Remote_BBMD.sin_addr.s_addr);
            port = ntohs(session_object->BVLC_Remote_BBMD.sin_port);
            bvlc_encode_bip_address(&alternative_destination.mac[0], &address,
                port);
            alternative_destination.mac_len = 6;
            debug_printf("BVLC: Sent Distribute-Broadcast-to-Network.\n");
        } else {
            address.s_addr = bip_get_broadcast_addr(session_object);
            port = bip_get_port(session_object);
            bvlc_encode_bip_address(&alternative_destination.mac[0], &address,
                port);
            alternative_destination.mac_len = 6;
            function_code = BVLC_ORIGINAL_BROADCAST_NPDU;
            debug_printf("BVLC: Sent Original-Broadcast-NPDU.\n");
        }
    } else if (dest->mac_len == 6) {
        /* valid unicast */
        alternative_destination = *dest;
        function_code = BVLC_ORIGINAL_UNICAST_NPDU;
        debug_printf("BVLC: Sent Original-Unicast-NPDU.\n");
    } else {
        /* invalid address */
        return -1;
    }

    /* Effective data sending */
    return bvlc_send_pdu_function_to_address(session_object, dest,
        function_code, npdu_data, pdu, pdu_len);
}

#ifdef TEST
#include <assert.h>
#include <string.h>
#include "ctest.h"

void testBIPAddress(
    Test * pTest)
{
    uint8_t apdu[50] = { 0 };
    uint32_t value = 0, test_value = 0;
    int len = 0, test_len = 0;
    struct in_addr address;
    struct in_addr test_address;
    uint16_t port = 0, test_port = 0;

    address.s_addr = 42;
    len = bvlc_encode_bip_address(&apdu[0], &address, port);
    test_len = bvlc_decode_bip_address(&apdu[0], &test_address, &test_port);
    ct_test(pTest, len == test_len);
    ct_test(pTest, address.s_addr == test_address.s_addr);
    ct_test(pTest, port == test_port);
}

void testInternetAddress(
    Test * pTest)
{
    BACNET_ADDRESS src;
    BACNET_ADDRESS test_src;
    struct sockaddr_in sin = { 0 };
    struct sockaddr_in test_sin = { 0 };

    sin.sin_port = htons(0xBAC0);
    sin.sin_addr.s_addr = inet_addr("192.168.0.1");
    bvlc_internet_to_bacnet_address(&src, &sin);
    bvlc_bacnet_to_internet_address(&test_sin, &src);
    ct_test(pTest, sin.sin_port == test_sin.sin_port);
    ct_test(pTest, sin.sin_addr.s_addr == test_sin.sin_addr.s_addr);
}

#ifdef TEST_BVLC
int main(
    void)
{
    Test *pTest;
    bool rc;

    pTest = ct_create("BACnet Virtual Link Control", NULL);
    /* individual tests */
    rc = ct_addTestFunction(pTest, testBIPAddress);
    assert(rc);
    rc = ct_addTestFunction(pTest, testInternetAddress);
    assert(rc);
    /* configure output */
    ct_setStream(pTest, stdout);
    ct_run(pTest);
    (void) ct_report(pTest);
    ct_destroy(pTest);

    return 0;
}

#endif /* TEST_BBMD */
#endif /* TEST */
