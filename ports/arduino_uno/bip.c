/**************************************************************************
 *
 * Copyright (C) 2005 Steve Karg
 *
 * SPDX-License-Identifier: GPL-2.0-or-later WITH GCC-exception-2.0
 *
 *********************************************************************/
#include <stdint.h> /* for standard integer types uint8_t etc. */
#include <stdbool.h> /* for the standard bool type. */
#include <string.h>
#include "bacnet/bacdcode.h"
#include "bacnet/bacint.h"
#include "bacnet/datalink/bip.h"
#include "bvlc-arduino.h"
#include "socketWrapper.h"
#include "w5100Wrapper.h"

#if PRINT_ENABLED | DEBUG
#include <stdio.h> /* for standard i/o, like printing */
#endif

/** @file bip.c  Configuration and Operations for BACnet/IP */

static uint8_t BIP_Socket = MAX_SOCK_NUM;
/* port to use - stored in network byte order */
static uint16_t BIP_Port = 0; /* this will force initialization in demos */
/* IP Address - stored in network byte order */
// static struct in_addr BIP_Address;
static uint8_t BIP_Address[4] = { 0, 0, 0, 0 };
/* Broadcast Address - stored in network byte order */
// static struct in_addr BIP_Broadcast_Address;
static uint8_t BIP_Broadcast_Address[4] = { 0, 0, 0, 0 };

/** Converter from uint8_t[4] type address to uint32_t
 *
 */
uint32_t convertBIP_Address2uint32(uint8_t *bip_address)
{
    return (uint32_t)((bip_address[0] * 2 ^ 24) + (bip_address[1] * 2 ^ 16) +
        (bip_address[2] * 2 ^ 8) + bip_address[3]);
}

/** Convert from uint32_t IPv4 address to uint8_t[4] address
 *
 */
void convertUint32Address_2_uint8Address(uint32_t ip, uint8_t *address)
{
    address[0] = (uint8_t)(ip >> 24);
    address[1] = (uint8_t)(ip >> 16);
    address[2] = (uint8_t)(ip >> 8);
    address[3] = (uint8_t)(ip >> 0);
}

/** Setter for the BACnet/IP socket handle.
 *
 * @param sock_fd [in] Handle for the BACnet/IP socket.
 */
void bip_set_socket(uint8_t sock_fd)
{
    BIP_Socket = sock_fd;
}

/** Getter for the BACnet/IP socket handle.
 *
 * @return The handle to the BACnet/IP socket.
 */
uint8_t bip_socket(void)
{
    return BIP_Socket;
}

bool bip_valid(void)
{
    return (BIP_Socket < MAX_SOCK_NUM);
}

void bip_set_addr(uint8_t *net_address)
{ /* in network byte order */
    for (uint8_t i = 0; i < 4; i++)
        BIP_Address[i] = net_address[i];
}

/* returns network byte order */
uint8_t *bip_get_addr(void)
{
    return BIP_Address;
}

void bip_set_broadcast_addr(uint8_t *net_address)
{ /* in network byte order */
    for (uint8_t i = 0; i < 4; i++)
        BIP_Broadcast_Address[i] = net_address[i];
}

/* returns network byte order */
uint8_t *bip_get_broadcast_addr(void)
{
    return BIP_Broadcast_Address;
}

void bip_set_port(uint16_t port)
{ /* in network byte order */
    BIP_Port = htons(port);
}

/* returns network byte order */
uint16_t bip_get_port(void)
{
    return ntohs(BIP_Port);
}

static int bip_decode_bip_address(BACNET_ADDRESS *bac_addr,
    uint8_t *address, /* in network format */

    uint16_t *port)
{ /* in network format */
    int len = 0;

    if (bac_addr) {
        memcpy(address, &bac_addr->mac[0], 4);
        memcpy(port, &bac_addr->mac[4], 2);
        len = 6;
    }

    return len;
}

/** Function to send a packet out the BACnet/IP socket (Annex J).
 * @ingroup DLBIP
 *
 * @param dest [in] Destination address (may encode an IP address and port #).
 * @param npdu_data [in] The NPDU header (Network) information (not used).
 * @param pdu [in] Buffer of data to be sent - may be null (why?).
 * @param pdu_len [in] Number of bytes in the pdu buffer.
 * @return Number of bytes sent on success, negative number on failure.
 */
int bip_send_pdu(BACNET_ADDRESS *dest, /* destination address */

    BACNET_NPDU_DATA *npdu_data, /* network information */

    uint8_t *pdu, /* any data to be sent - may be null */

    unsigned pdu_len)
{ /* number of bytes of data */

    uint8_t mtu[BIP_MPDU_MAX] = { 0 };
    int mtu_len = 0;
    int bytes_sent = 0;
    /* addr and port in host format */
    uint8_t address[] = { 0, 0, 0, 0 };
    uint16_t port = 0;

    (void)npdu_data;
    /* assumes that the driver has already been initialized */
    if (BIP_Socket < 0) {
        return BIP_Socket;
    }

    mtu[0] = BVLL_TYPE_BACNET_IP;
    if ((dest->net == BACNET_BROADCAST_NETWORK) ||
        ((dest->net > 0) && (dest->len == 0)) || (dest->mac_len == 0)) {
        /* broadcast */
        for (uint8_t i = 0; i < 4; i++)
            address[i] = BIP_Broadcast_Address[i];
        port = BIP_Port;
        mtu[1] = BVLC_ORIGINAL_BROADCAST_NPDU;
#ifdef DEBUG
        fprintf(stderr, "Send Broadcast NPDU to %d.%d.%d.%d:%d\n", address[0],
            address[1], address[2], address[3], port);
#endif
    } else if (dest->mac_len == 6) {
        bip_decode_bip_address(dest, address, &port);
        mtu[1] = BVLC_ORIGINAL_UNICAST_NPDU;
#ifdef DEBUG
        fprintf(stderr, "Send Unicast NPDU to %d.%d.%d.%d:%d\n", address[0],
            address[1], address[2], address[3], port);
#endif
    } else {
        /* invalid address */
        return -1;
    }

    mtu_len = 2;
    mtu_len += encode_unsigned16(
        &mtu[mtu_len], (uint16_t)(pdu_len + 4 /*inclusive */));
    memcpy(&mtu[mtu_len], pdu, pdu_len);
    mtu_len += pdu_len;

#ifdef DEBUG
    fprintf(stderr, "MTU size %d\n", mtu_len);
#endif

    /* Send the packet */
    bytes_sent = sendto_func(BIP_Socket, mtu, (uint16_t)mtu_len, address, port);

    return bytes_sent;
}

/** Implementation of the receive() function for BACnet/IP; receives one
 * packet, verifies its BVLC header, and removes the BVLC header from
 * the PDU data before returning.
 *
 * @param src [out] Source of the packet - who should receive any response.
 * @param pdu [out] A buffer to hold the PDU portion of the received packet,
 * 					after the BVLC portion has been stripped
 * off.
 * @param max_pdu [in] Size of the pdu[] buffer.
 * @param timeout [in] The number of milliseconds to wait for a packet.
 * @return The number of octets (remaining) in the PDU, or zero on failure.
 */
uint16_t bip_receive(BACNET_ADDRESS *src, /* source address */

    uint8_t *pdu, /* PDU data */

    uint16_t max_pdu, /* amount of space available in the PDU  */

    unsigned timeout)
{
    int received_bytes = 0;
    int max = 0;
    uint16_t pdu_len = 0; /* return value */
    uint8_t src_addr[] = { 0, 0, 0, 0 };
    uint16_t src_port = 0;
    uint16_t i = 0;
    int function = 0;

    /* Make sure the socket is open */
    if (BIP_Socket < 0)
        return 0;

    if (getRXReceivedSize_func(CW5100Class_new(), BIP_Socket)) {
        memcpy(&src_addr, &src->mac[0], 4);
        memcpy(&src_port, &src->mac[4], 2);
        received_bytes = (int)recvfrom_func(
            BIP_Socket, &pdu[0], max_pdu, src_addr, &src_port);
    }

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

    /* Erase up to 16 bytes after the received bytes as safety margin to
     * ensure that the decoding functions will run into a 'safe field'
     * of zero, if for any reason they would overrun, when parsing the
     * message. */
    max = (int)max_pdu - received_bytes;
    if (max > 0) {
        if (max > 16) {
            max = 16;
        }
        memset(&pdu[received_bytes], 0, max);
    }

    if (bvlc_for_non_bbmd(src_addr, &src_port, pdu, received_bytes) > 0) {
        /* Handled, usually with a NACK. */
#if PRINT_ENABLED
        fprintf(stderr, "BIP: BVLC discarded!\n");
#endif
        return 0;
    }

    function = bvlc_get_function_code(); /* aka, pdu[1] */
    if ((function == BVLC_ORIGINAL_UNICAST_NPDU) ||
        (function == BVLC_ORIGINAL_BROADCAST_NPDU)) {
        /* ignore messages from me */
        if ((convertBIP_Address2uint32(src_addr) ==
                convertBIP_Address2uint32(BIP_Address)) &&
            (src_port == BIP_Port)) {
            pdu_len = 0;
#if 0
            fprintf(stderr, "BIP: src is me. Discarded!\n");
#endif
        } else {
            /* data in src->mac[] is in network format */
            src->mac_len = 6;
            memcpy(&src->mac[0], &src_addr, 4);
            memcpy(&src->mac[4], &src_port, 2);
#ifdef DEBUG
            fprintf(stderr, "BIP receive from %d.%d.%d.%d\n", src->mac[0],
                src->mac[1], src->mac[2], src->mac[3]);
#endif
            /* FIXME: check destination address */
            /* see if it is broadcast or for us */
            /* decode the length of the PDU - length is inclusive of BVLC */
            (void)decode_unsigned16(&pdu[2], &pdu_len);
            /* subtract off the BVLC header */
            pdu_len -= 4;
            if (pdu_len < max_pdu) {
#if 0
                fprintf(stderr, "BIP: NPDU[%hu]:", pdu_len);
#endif
                /* shift the buffer to return a valid PDU */
                for (i = 0; i < pdu_len; i++) {
                    pdu[i] = pdu[4 + i];
#if 0
                    fprintf(stderr, "%02X ", pdu[i]);
#endif
                }
#if 0
                fprintf(stderr, "\n");
#endif
            }
            /* ignore packets that are too large */
            /* clients should check my max-apdu first */
            else {
                pdu_len = 0;
#if PRINT_ENABLED
                fprintf(stderr, "BIP: PDU too large. Discarded!.\n");
#endif
            }
        }
    } else if (function == BVLC_FORWARDED_NPDU) {
        memcpy(&src_addr, &pdu[4], 4);
        memcpy(&src_port, &pdu[8], 2);
        if ((convertBIP_Address2uint32(src_addr) ==
                convertBIP_Address2uint32(BIP_Address)) &&
            (src_port == BIP_Port)) {
            /* ignore messages from me */
            pdu_len = 0;
        } else {
            /* data in src->mac[] is in network format */
            src->mac_len = 6;
            memcpy(&src->mac[0], &src_addr, 4);
            memcpy(&src->mac[4], &src_port, 2);
            /* FIXME: check destination address */
            /* see if it is broadcast or for us */
            /* decode the length of the PDU - length is inclusive of BVLC */
            (void)decode_unsigned16(&pdu[2], &pdu_len);
            /* subtract off the BVLC header */
            pdu_len -= 10;
            if (pdu_len < max_pdu) {
                /* shift the buffer to return a valid PDU */
                for (i = 0; i < pdu_len; i++) {
                    pdu[i] = pdu[4 + 6 + i];
                }
            } else {
                /* ignore packets that are too large */
                /* clients should check my max-apdu first */
                pdu_len = 0;
            }
        }
    }

    return pdu_len;
}

void bip_get_my_address(BACNET_ADDRESS *my_address)
{
    int i = 0;

    if (my_address) {
        my_address->mac_len = 6;
        memcpy(&my_address->mac[0], &BIP_Address, 4);
        memcpy(&my_address->mac[4], &BIP_Port, 2);
        my_address->net = 0; /* local only, no routing */
        my_address->len = 0; /* no SLEN */
        for (i = 0; i < MAX_MAC_LEN; i++) {
            /* no SADR */
            my_address->adr[i] = 0;
        }
    }

    return;
}

void bip_get_broadcast_address(BACNET_ADDRESS *dest)
{ /* destination address */
    int i = 0; /* counter */

    if (dest) {
        dest->mac_len = 6;
        memcpy(&dest->mac[0], &BIP_Broadcast_Address, 4);
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
