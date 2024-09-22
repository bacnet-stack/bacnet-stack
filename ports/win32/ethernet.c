/**************************************************************************
 *
 * Copyright (C) 2005 Steve Karg, modified by Kevin Liao
 *
 * SPDX-License-Identifier: GPL-2.0-or-later WITH GCC-exception-2.0
 *
 *********************************************************************/
#include <stdint.h> /* for standard integer types uint8_t etc. */
#include <stdbool.h> /* for the standard bool type. */
#include <assert.h>
#include <direct.h>
#include <io.h>

#include "bacnet/bacdef.h"
#include "bacnet/datalink/ethernet.h"
#include "bacnet/bacdcode.h"

/* Uses Npcap to access raw ethernet */
/* Notes:                                               */
/* To make ethernet.c work under win32, you have to:    */
/* 1. install Npcap 1.80 installer for Windows;         */
/* 2. install msys2 x86_64                              */
/* 3. remove or modify functions used for log such as   */
/* "LogError()", "LogInfo()", which were implemented    */
/*  as a wrapper of Log4cpp.                            */
/* -- Kevin Liao                                        */
/* -- Patrick Grimm                                     */

/* includes for accessing ethernet by using winpcap */
#include "pcap.h"
#include "ntddndis.h"

/* commonly used comparison address for ethernet */
uint8_t Ethernet_Broadcast[MAX_MAC_LEN] = {
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF
};
/* commonly used empty address for ethernet quick compare */
uint8_t Ethernet_Empty_MAC[MAX_MAC_LEN] = { 0, 0, 0, 0, 0, 0 };

/* my local device data - MAC address */
uint8_t Ethernet_MAC_Address[MAX_MAC_LEN] = { 0 };

/* couple of var for using winpcap */
static char pcap_errbuf[PCAP_ERRBUF_SIZE + 1];
static pcap_t *pcap_eth802_fp = NULL; /* 802.2 file handle, from winpcap */
static unsigned eth_timeout = 100;

/* #######Begin Packet32.h copy########*/
/*!
  \brief Structure containing an OID request.

  It is used by the PacketRequest() function to send an OID to the interface card driver. 
  It can be used, for example, to retrieve the status of the error counters on the adapter, its MAC address, 
  the list of the multicast groups defined on it, and so on.
*/
struct _PACKET_OID_DATA
{
	ULONG Oid;	/*				///< OID code. See the Microsoft DDK documentation or the file ntddndis.h
	///< for a complete list of valid codes. */
	ULONG Length;	/*			///< Length of the data field
	_Field_size_full_(Length)*/
	UCHAR Data[1];	/*			///< variable-lenght field that contains the information passed to or received 
	///< from the adapter.*/
}; 
typedef struct _PACKET_OID_DATA PACKET_OID_DATA, * PPACKET_OID_DATA;

#define 	   MAX_LINK_NAME_LENGTH	64	/*		//< Maximum length of the devices symbolic links*/
#define ADAPTER_NAME_LENGTH 256 + 12	/*///<  Maximum length for the name of an adapter. The value is the same used by the IP Helper API.*/
typedef struct WAN_ADAPTER_INT WAN_ADAPTER; /*///< Describes an opened wan (dialup, VPN...) network adapter using the NetMon API*/
typedef WAN_ADAPTER* PWAN_ADAPTER; /*///< Describes an opened wan (dialup, VPN...) network adapter using the NetMon API*/

/*!
  \brief Describes an opened network adapter.

  This structure is the most important for the functioning of packet.dll, but the great part of its fields
  should be ignored by the user, since the library offers functions that avoid to cope with low-level parameters
*/
typedef struct _ADAPTER
{
	HANDLE hFile;	/*			///< \internal Handle to an open instance of the NPF driver.*/
	CHAR SymbolicLink[MAX_LINK_NAME_LENGTH]; /*///< \internal A string containing the name of the network adapter currently opened.*/
	int NumWrites;	/*			///< \internal Number of times a packets written on this adapter will be repeated
	///< on the wire.*/
	HANDLE ReadEvent;	/*		///< A notification event associated with the read calls on the adapter.
	///< It can be passed to standard Win32 functions (like WaitForSingleObject
	///< or WaitForMultipleObjects) to wait until the driver's buffer contains some 
	///< data. It is particularly useful in GUI applications that need to wait 
	///< concurrently on several events. The PacketSetMinToCopy()
	///< function can be used to define the minimum amount of data in the kernel buffer
	///< that will cause the event to be signalled. */

	UINT ReadTimeOut;  /*///< \internal The amount of time PacketReceivePacket will wait for the ReadEvent to be signalled before issuing a ReadFile.*/
	CHAR Name[ADAPTER_NAME_LENGTH];
	PWAN_ADAPTER pWanAdapter;
	UINT Flags;		/*			///< Adapter's flags. Tell if this adapter must be treated in a different way.*/

#ifdef HAVE_AIRPCAP_API
	PAirpcapHandle AirpcapAd;
#endif /*// HAVE_AIRPCAP_API*/
}  ADAPTER, * LPADAPTER;

_Ret_maybenull_ LPADAPTER PacketOpenAdapter(_In_ PCCH AdapterName);
_Success_(return) BOOLEAN PacketRequest(_In_ LPADAPTER  AdapterObject, _In_ BOOLEAN Set, _Inout_ PPACKET_OID_DATA  OidData);
VOID PacketCloseAdapter(_In_ _Post_invalid_ LPADAPTER lpAdapter);

/* #######End Packet32.h copy##########*/

/* couple of external func for runtime error logging, you can simply    */
/* Logging functions: Info level */
void LogInfo(const char *msg)
{
    fprintf(stdout,"info  ethernet: %s", msg);
}
/* Logging functions: Error level*/
void LogError(const char *msg)
{
    fprintf(stderr,"error ethernet: %s", msg);
}
/* Logging functions: Debug level*/
void LogDebug(const char *msg)
{
    fprintf(stdout,"debug ethernet: %s", msg);
}


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
    LogInfo("ethernet_cleanup() ok.\n");
}

void ethernet_set_timeout(unsigned timeout)
{
    eth_timeout = timeout;
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
/**
 * We don't need to use this function since WinPCap has provided one
 *   named "pcap_setnonblock()".
 * Kevin, 2006.08.15
 */
/*
int setNonblocking(int fd)
{
    int flags;

    if (-1 == (flags = fcntl(fd, F_GETFL, 0)))
        flags = 0;
    return fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}
*/

bool ethernet_init(char *if_name)
{
    PPACKET_OID_DATA pOidData;
    LPADAPTER lpAdapter;
    pcap_if_t *pcap_all_if;
    pcap_if_t *dev;
    BOOLEAN result;
    CHAR str[sizeof(PACKET_OID_DATA) + 128];
    int i;
    char msg[400];
    int devnum;
    char *device;

    if (ethernet_valid()) {
        ethernet_cleanup();
    }

    /**
     * Find the interface user specified
     */
    /* Retrieve the device list */
    if (pcap_findalldevs(&pcap_all_if, pcap_errbuf) == PCAP_ERROR) {
        snprintf(
            msg, sizeof(msg), "pcap_findalldevs: %s\n",
            pcap_errbuf);
        LogError(msg);
        return false;
    }
    /* Scan the list printing every entry */
    devnum = atoi(if_name);
    i = 0;
    for (dev = pcap_all_if; dev; dev = dev->next) {
	    /* struct pcap_addr *dev_addr;*/
        i++;
        if (devnum == i) {
            device = dev->name;
            snprintf(
                msg, sizeof(msg), "interface select index: %i\n",
                i);
            LogInfo(msg);
        }
        if ((dev->flags & PCAP_IF_UP) && 
            ! (dev->flags & PCAP_IF_LOOPBACK) &&
            (dev->flags & PCAP_IF_RUNNING) &&
            (dev->flags & PCAP_IF_CONNECTION_STATUS_CONNECTED)
            )
        {
            snprintf(
                msg, sizeof(msg), "interface index: %i\n",
                i);
            LogInfo(msg);
            snprintf(
                msg, sizeof(msg), " name:        %s\n",
                dev->name);
            LogInfo(msg);
            snprintf(
                msg, sizeof(msg), " description: %s\n",
                dev->description);
            LogInfo(msg);
        }
    }
    pcap_freealldevs(pcap_all_if); /* we don't need it anymore */
    if (if_name == NULL) {
        snprintf(
            msg, sizeof(msg), "interface index not set\n");
        LogError(msg);
        return false;
    }
    if (device == NULL) {
        snprintf(
            msg, sizeof(msg), "specified interface not found: %s\n",
            if_name);
        LogError(msg);
        return false;
    }

    /**
     * Get local MAC address
     */
    ZeroMemory(str, sizeof(PACKET_OID_DATA) + 128);
    lpAdapter = PacketOpenAdapter(device);
    if (lpAdapter == NULL) {
        ethernet_cleanup();
        snprintf(
            msg, sizeof(msg),
            "local mac PacketOpenAdapter(\"%s\")\n", device);
        LogError(msg);
        return false;
    }
    pOidData = (PPACKET_OID_DATA)str;
    pOidData->Oid = OID_802_3_CURRENT_ADDRESS;
    pOidData->Length = 6;
    result = PacketRequest(lpAdapter, FALSE, pOidData);
    if (!result) {
        PacketCloseAdapter(lpAdapter);
        ethernet_cleanup();
        LogError("local mac PacketRequest()\n");
        return false;
    }
    for (i = 0; i < 6; ++i) {
        Ethernet_MAC_Address[i] = pOidData->Data[i];
    }
    PacketCloseAdapter(lpAdapter);

    snprintf(
        msg, sizeof(msg),
        "local mac %02x:%02x:%02x:%02x:%02x:%02x \n",
        Ethernet_MAC_Address[0],
        Ethernet_MAC_Address[1],
        Ethernet_MAC_Address[2],
        Ethernet_MAC_Address[3],
        Ethernet_MAC_Address[4],
        Ethernet_MAC_Address[5]
     );
    LogInfo(msg);

    /**
     * Open interface for subsequent sending and receiving
     */
    /* Open the output device */
    pcap_eth802_fp = pcap_open_live(
        device, /* name of the device */
        ETHERNET_MPDU_MAX, /* portion of the packet to capture */
        PCAP_OPENFLAG_PROMISCUOUS, /* promiscuous mode */
        eth_timeout, /* read timeout */
        /* NULL, *//* authentication on the remote machine */
        pcap_errbuf /* error buffer */
    );
    if (pcap_eth802_fp == NULL) {
        PacketCloseAdapter(lpAdapter);
        ethernet_cleanup();
        snprintf(
            msg, sizeof(msg),
            "unable to open the adapter. %s is not supported by "
            "Npcap\n",
            device);
        LogError(msg);
        return false;
    }

    LogInfo("ethernet_init() ok.\n");

    atexit(ethernet_cleanup);

    return ethernet_valid();
}

/* function to send a packet out the 802.2 socket */
/* returns bytes sent success, negative on failure */
int ethernet_send_dst(
    BACNET_ADDRESS *dest, /* destination address */
    BACNET_ADDRESS *src, /* source address */
    uint8_t *pdu, /* any data to be sent - may be null */
    unsigned pdu_len /* number of bytes of data */
)
{
    /* int bytes = 0; */
    uint8_t mtu[ETHERNET_MPDU_MAX] = { 0 };
    int mtu_len = 0;
    int i = 0;

    /* don't waste time if the socket is not valid */
    if (!ethernet_valid()) {
        LogError("invalid 802.2 ethernet interface descriptor!\n");
        return -1;
    }
    /* load destination ethernet MAC address */
    if (dest->mac_len == 6) {
        for (i = 0; i < 6; i++) {
            mtu[mtu_len] = dest->mac[i];
            mtu_len++;
        }
    } else {
        LogError("invalid destination MAC address!\n");
        return -2;
    }

    /* load source ethernet MAC address */
    if (src->mac_len == 6) {
        for (i = 0; i < 6; i++) {
            mtu[mtu_len] = src->mac[i];
            mtu_len++;
        }
    } else {
        LogError("invalid source MAC address!\n");
        return -3;
    }
    if ((14 + 3 + pdu_len) > ETHERNET_MPDU_MAX) {
        LogError("PDU is too big to send!\n");
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
        char msg[200];
        snprintf(
            msg, sizeof(msg), "error sending packet: %s\n",
            pcap_geterr(pcap_eth802_fp));
        LogError(msg);
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
    return ethernet_send_dst(
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
                        due to winpcap API. */
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
        LogError("invalid 802.2 ethernet interface descriptor!\n");
        return 0;
    }

    /* Capture a packet */
    res = pcap_next_ex(pcap_eth802_fp, &header, &pkt_data);
    if (res < 0) {
        char msg[200];
        snprintf(
            msg, sizeof(msg), "error in receiving packet: %s\n",
            pcap_geterr(pcap_eth802_fp));
        LogError(msg);
        return 0;
    } else if (res == 0) {
        return 0;
    }

    if (header->len == 0 || header->caplen == 0) {
        return 0;
    }

    /* the signature of an 802.2 BACnet packet */
    if ((pkt_data[14] != 0x82) && (pkt_data[15] != 0x82)) {
        /*LogError("ethernet.c: Non-BACnet packet\n"); */
        return 0;
    }
    /* copy the source address */
    src->mac_len = 6;
    memmove(src->mac, &pkt_data[6], 6);

    /* check destination address for when */
    /* the Ethernet card is in promiscious mode */
    if ((memcmp(&pkt_data[0], Ethernet_MAC_Address, 6) != 0) &&
        (memcmp(&pkt_data[0], Ethernet_Broadcast, 6) != 0)) {
        /*LogError( "ethernet.c: This packet isn't for us\n"); */
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
    my_address->net = 0; /* local only, no routing */
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
        dest->len = 0; /* denotes broadcast address  */
        for (i = 0; i < MAX_MAC_LEN; i++) {
            dest->adr[i] = 0;
        }
    }

    return;
}

void ethernet_debug_address(const char *info, const BACNET_ADDRESS *dest)
{
    int i = 0; /* counter */
    char msg[200];

    if (info) {
        snprintf(msg, sizeof(msg), "%s", info);
        LogError(msg);
    }
    /* if */
    if (dest) {
        snprintf(
            msg, sizeof(msg),
            "Address:\n  MAC Length=%d\n  MAC Address=", dest->mac_len);
        LogInfo(msg);
        for (i = 0; i < MAX_MAC_LEN; i++) {
            snprintf(msg, sizeof(msg), "%02X ", (unsigned)dest->mac[i]);
            LogInfo(msg);
        } /* for */
        LogInfo("\n");
        snprintf(
            msg, sizeof(msg), "  Net=%hu\n  Len=%d\n  Adr=", dest->net,
            dest->len);
        LogInfo(msg);
        for (i = 0; i < MAX_MAC_LEN; i++) {
            snprintf(msg, sizeof(msg), "%02X ", (unsigned)dest->adr[i]);
            LogInfo(msg);
        } /* for */
        LogInfo("\n");
    }
    /* if ( dest ) */
    return;
}
