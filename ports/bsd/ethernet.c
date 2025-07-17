/**************************************************************************
 *
 * Copyright (C) 2005 Steve Karg
 *
 * SPDX-License-Identifier: GPL-2.0-or-later WITH GCC-exception-2.0
 *
 *********************************************************************/
#include <stdint.h> /* for standard integer types uint8_t etc. */
#include <stdbool.h> /* for the standard bool type. */
#include <ifaddrs.h>

/* includes for accessing ethernet by using pcap */
#include "pcap.h"

#include "bacport.h"
#include "bacnet/bacdef.h"
#include "bacnet/datalink/ethernet.h"
#include "bacnet/bacint.h"

/** @file bsd/ethernet.c  Provides BSD-specific functions for
 * BACnet/Ethernet. */

/* commonly used comparison address for ethernet */
uint8_t Ethernet_Broadcast[MAX_MAC_LEN] = {
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF
};
/* commonly used empty address for ethernet quick compare */
uint8_t Ethernet_Empty_MAC[MAX_MAC_LEN] = { 0, 0, 0, 0, 0, 0 };

/* my local device data - MAC address */
uint8_t Ethernet_MAC_Address[MAX_MAC_LEN] = { 0 };

/* couple of var for using pcap */
static char pcap_errbuf[PCAP_ERRBUF_SIZE + 1];
static pcap_t *pcap_eth802_fp = NULL; /* 802.2 file handle, from pcap */
static unsigned eth_timeout = 100;

bool ethernet_valid(void)
{
    return (pcap_eth802_fp != NULL);
}

void ethernet_cleanup(void)
{
    if (pcap_eth802_fp) {
        pcap_close(pcap_eth802_fp);
        pcap_eth802_fp = NULL;
    }
    fprintf(stderr, "ethernet: ethernet_cleanup() ok.\n");
}

/*----------------------------------------------------------------------
 Portable function to set a socket into nonblocking mode.
 Calling this on a socket causes all future read() and write() calls on
 that socket to do only as much as they can immediately, and return
 without waiting.
 If no data can be read or written, they return -1 and set errno
 to EAGAIN (or EWOULDBLOCK).
 Thanks to Bjorn Reese for this code.
----------------------------------------------------------------------*/
/*
int setNonblocking(
    int fd)
{
    int flags;

    if (-1 == (flags = fcntl(fd, F_GETFL, 0)))
        flags = 0;
    return fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

void ethernet_set_timeout(unsigned timeout)
{
    eth_timeout = timeout;
}
*/

/* function to find the local ethernet MAC address */
static int get_local_hwaddr(const char *ifname, unsigned char *mac)
{
    struct ifaddrs *ifap, *ifaptr;
    unsigned char *ptr;
    int i;

    if (getifaddrs(&ifap) == 0) {
        for (ifaptr = ifap; ifaptr != NULL; ifaptr = (ifaptr)->ifa_next) {
            if (!strcmp((ifaptr)->ifa_name, ifname) &&
                (((ifaptr)->ifa_addr)->sa_family == AF_LINK)) {
                ptr = (unsigned char *)LLADDR(
                    (struct sockaddr_dl *)(ifaptr)->ifa_addr);
                for (i = 0; i < 6; ++i) {
                    mac[i] = *(ptr + i);
                }
                break;
            }
        }
        freeifaddrs(ifap);
        return ifaptr != NULL;
    } else {
        return 0;
    }
}

bool ethernet_init(char *if_name)
{
    pcap_if_t *pcap_all_if;
    pcap_if_t *dev;

    if (ethernet_valid()) {
        ethernet_cleanup();
    }

    /**
     * Find the interface user specified
     */
    /* Retrieve the device list */
    if (pcap_findalldevs(&pcap_all_if, pcap_errbuf) == -1) {
        fprintf(
            stderr, "ethernet.c: error in pcap_findalldevs: %s\n", pcap_errbuf);
        return false;
    }
    /* Scan the list printing every entry */
    for (dev = pcap_all_if; dev; dev = dev->next) {
        if (strcmp(if_name, dev->name) == 0) {
            break;
        }
    }
    pcap_freealldevs(pcap_all_if); /* we don't need it anymore */
    if (dev == NULL) {
        fprintf(
            stderr, "ethernet.c: specified interface not found: %s\n", if_name);
        return false;
    }

    /**
     * Get local MAC address
     */
    get_local_hwaddr(if_name, Ethernet_MAC_Address);
    fprintf(
        stderr, "ethernet: src mac %02x:%02x:%02x:%02x:%02x:%02x \n",
        Ethernet_MAC_Address[0], Ethernet_MAC_Address[1],
        Ethernet_MAC_Address[2], Ethernet_MAC_Address[3],
        Ethernet_MAC_Address[4], Ethernet_MAC_Address[5]);

    /**
     * Open interface for subsequent sending and receiving
     */
    /* Open the output device */
    pcap_eth802_fp = pcap_open_live(
        if_name, /* name of the device */
        ETHERNET_MPDU_MAX, /* portion of the packet to capture */
        PCAP_OPENFLAG_PROMISCUOUS, /* promiscuous mode */
        eth_timeout, /* read timeout */
        // NULL, /* authentication on the remote machine */
        pcap_errbuf /* error buffer */
    );
    if (pcap_eth802_fp == NULL) {
        ethernet_cleanup();
        fprintf(
            stderr,
            "ethernet.c: unable to open the adapter. %s is not supported by "
            "Pcap\n",
            if_name);
        return false;
    }

    fprintf(stderr, "ethernet: ethernet_init() ok.\n");

    atexit(ethernet_cleanup);

    return ethernet_valid();
}

/* function to send a packet out the 802.2 socket */
/* returns bytes sent success, negative on failure */
int ethernet_send_addr(
    BACNET_ADDRESS *dest, /* destination address */
    BACNET_ADDRESS *src, /* source address */
    uint8_t *pdu, /* any data to be sent - may be null */
    unsigned pdu_len /* number of bytes of data */
)
{
    uint8_t mtu[ETHERNET_MPDU_MAX] = { 0 };
    int mtu_len = 0;
    int i = 0;

    /* don't waste time if the socket is not valid */
    if (!ethernet_valid()) {
        fprintf(
            stderr, "ethernet: invalid 802.2 ethernet interface descriptor!\n");
        return -1;
    }
    /* load destination ethernet MAC address */
    if (dest->mac_len == 6) {
        for (i = 0; i < 6; i++) {
            mtu[mtu_len] = dest->mac[i];
            mtu_len++;
        }
    } else {
        fprintf(stderr, "ethernet: invalid destination MAC address!\n");
        return -2;
    }

    /* load source ethernet MAC address */
    if (src->mac_len == 6) {
        for (i = 0; i < 6; i++) {
            mtu[mtu_len] = src->mac[i];
            mtu_len++;
        }
    } else {
        fprintf(stderr, "ethernet: invalid source MAC address!\n");
        return -3;
    }
    if ((14 + 3 + pdu_len) > ETHERNET_MPDU_MAX) {
        fprintf(stderr, "ethernet: PDU is too big to send!\n");
        return -4;
    }
    /* packet length */
    mtu_len += encode_unsigned16(&mtu[12], 3 /*DSAP,SSAP,LLC */ + pdu_len);
    /* Logical PDU portion */
    mtu[mtu_len++] = 0x82; /* DSAP for BACnet */
    mtu[mtu_len++] = 0x82; /* SSAP for BACnet */
    mtu[mtu_len++] = 0x03; /* Control byte in header */
    memcpy(&mtu[mtu_len], pdu, pdu_len);
    mtu_len += pdu_len;

    /* Send the packet */
    if (pcap_sendpacket(pcap_eth802_fp, mtu, mtu_len) != 0) {
        /* did it get sent? */
        fprintf(
            stderr, "ethernet: error in sending packet: %s\n",
            pcap_geterr(pcap_eth802_fp));
        return -5;
    }

    return mtu_len;
}

/* function to send a packet out the 802.2 socket */
/* returns number of bytes sent on success, negative on failure */
int ethernet_send_pdu(
    BACNET_ADDRESS *dest, /* destination address */
    BACNET_NPDU_DATA *npdu_data, /* network information */
    uint8_t *pdu, /* any data to be sent - may be null */
    unsigned pdu_len /* number of bytes of data */
)
{
    int i = 0; /* counter */
    BACNET_ADDRESS src = { 0 }; /* source address */
    (void)npdu_data;

    for (i = 0; i < 6; i++) {
        src.mac[i] = Ethernet_MAC_Address[i];
        src.mac_len++;
    }
    /* function to send a packet out the 802.2 socket */
    /* returns 1 on success, 0 on failure */
    return ethernet_send_addr(
        dest, /* destination address */
        &src, /* source address */
        pdu, /* any data to be sent - may be null */
        pdu_len /* number of bytes of data */
    );
}

/* receives an 802.2 framed packet */
/* returns the number of octets in the PDU, or zero on failure */
uint16_t ethernet_receive(
    BACNET_ADDRESS *src, /* source address */
    uint8_t *pdu, /* PDU data */
    uint16_t max_pdu, /* amount of space available in the PDU  */
    unsigned timeout /* number of milliseconds to wait for a packet. we ommit it
                        due to pcap API. */
)
{
    struct pcap_pkthdr *header;
    int res;
    const u_char *pkt_data;
    uint16_t pdu_len = 0; /* return value */

    /* unused ? */
    (void)timeout;

    /* Make sure the socket is open */
    if (!ethernet_valid()) {
        fprintf(
            stderr, "ethernet: invalid 802.2 ethernet interface descriptor!\n");
        return 0;
    }

    /* Capture a packet */
    res = pcap_next_ex(pcap_eth802_fp, &header, &pkt_data);
    if (res < 0) {
        fprintf(
            stderr, "ethernet: error in receiving packet: %s\n",
            pcap_geterr(pcap_eth802_fp));
        return 0;
    } else if (res == 0) {
        return 0;
    }

    if (header->len == 0 || header->caplen == 0) {
        return 0;
    }

    /* the signature of an 802.2 BACnet packet */
    if ((pkt_data[14] != 0x82) && (pkt_data[15] != 0x82)) {
        /* fprintf(stderr, "ethernet: Non-BACnet packet\n"); */
        return 0;
    }
    /* copy the source address */
    src->mac_len = 6;
    memmove(src->mac, &pkt_data[6], 6);

    /* check destination address for when */
    /* the Ethernet card is in promiscious mode */
    if ((memcmp(&pkt_data[0], Ethernet_MAC_Address, 6) != 0) &&
        (memcmp(&pkt_data[0], Ethernet_Broadcast, 6) != 0)) {
        /* fprintf(stderr, "ethernet: This packet isn't for us\n"); */
        return 0;
    }

    (void)decode_unsigned16(&pkt_data[12], &pdu_len);
    pdu_len -= 3 /* DSAP, SSAP, LLC Control */;
    /* copy the buffer into the PDU */
    if (pdu_len < max_pdu) {
        memmove(&pdu[0], &pkt_data[17], pdu_len);
    }
    /* ignore packets that are too large */
    else {
        pdu_len = 0;
    }

    return pdu_len;
}

void ethernet_set_my_address(const BACNET_ADDRESS *my_address)
{
    int i = 0;

    for (i = 0; i < 6; i++) {
        Ethernet_MAC_Address[i] = my_address->mac[i];
    }

    return;
}

void ethernet_get_my_address(BACNET_ADDRESS *my_address)
{
    int i = 0;

    my_address->mac_len = 0;
    for (i = 0; i < 6; i++) {
        my_address->mac[i] = Ethernet_MAC_Address[i];
        my_address->mac_len++;
    }
    my_address->net = 0; /* DNET=0 is local only, no routing */
    my_address->len = 0;
    for (i = 0; i < MAX_MAC_LEN; i++) {
        my_address->adr[i] = 0;
    }

    return;
}

void ethernet_get_broadcast_address(BACNET_ADDRESS *dest)
{ /* destination address */
    int i = 0; /* counter */

    if (dest) {
        for (i = 0; i < 6; i++) {
            dest->mac[i] = Ethernet_Broadcast[i];
        }
        dest->mac_len = 6;
        dest->net = BACNET_BROADCAST_NETWORK;
        dest->len = 0; /* always zero when DNET is broadcast */
        for (i = 0; i < MAX_MAC_LEN; i++) {
            dest->adr[i] = 0;
        }
    }

    return;
}

void ethernet_debug_address(const char *info, const BACNET_ADDRESS *dest)
{
    int i = 0; /* counter */

    if (info) {
        fprintf(stderr, "%s", info);
    }
    if (dest) {
        fprintf(stderr, "Address:\n");
        fprintf(stderr, "  MAC Length=%d\n", dest->mac_len);
        fprintf(stderr, "  MAC Address=");
        for (i = 0; i < MAX_MAC_LEN; i++) {
            fprintf(stderr, "%02X ", (unsigned)dest->mac[i]);
        }
        fprintf(stderr, "\n");
        fprintf(stderr, "  Net=%hu\n", dest->net);
        fprintf(stderr, "  Len=%d\n", dest->len);
        fprintf(stderr, "  Adr=");
        for (i = 0; i < MAX_MAC_LEN; i++) {
            fprintf(stderr, "%02X ", (unsigned)dest->adr[i]);
        }
        fprintf(stderr, "\n");
    }

    return;
}
