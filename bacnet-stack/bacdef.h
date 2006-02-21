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
#ifndef BACDEF_H
#define BACDEF_H

#include <stddef.h>
#include <stdint.h>
#include "bacenum.h"
#include "config.h"

/* largest BACnet Instance Number */
/* Also used as a device instance number wildcard address */
#define BACNET_MAX_INSTANCE (0x3FFFFF)
#define BACNET_INSTANCE_BITS 22
/* large BACnet Object Type */
#define BACNET_MAX_OBJECT (0x3FF)
/* Array index 0=size of array, n=array element n,  MAX=all array elements */
#define BACNET_ARRAY_LENGTH_INDEX 0
#define BACNET_ARRAY_ALL (~0)
/* Priority Array for commandable objects */
#define BACNET_NO_PRIORITY 0
#define BACNET_MIN_PRIORITY 1
#define BACNET_MAX_PRIORITY 16

/* embedded systems need fixed name sizes */
#define MAX_OBJECT_NAME 10
/* common object properties */
typedef struct BACnet_Object_Data {
    uint32_t Object_Identifier;
    char Object_Name[MAX_OBJECT_NAME];
    BACNET_OBJECT_TYPE Object_Type;
} BACNET_OBJECT_DATA;

#define BACNET_BROADCAST_NETWORK 0xFFFF
/* IPv6 (16 octets) coupled with port number (2 octets) */
#define MAX_MAC_LEN 18
struct BACnet_Device_Address {
    /* mac_len = 0 if global address */
    int mac_len;
    /* note: MAC for IP addresses uses 4 bytes for addr, 2 bytes for port */
    /* use de/encode_unsigned32/16 for re/storing the IP address */
    uint8_t mac[MAX_MAC_LEN];
    /* DNET,DLEN,DADR or SNET,SLEN,SADR */
    /* the following are used if the device is behind a router */
    /* net = 0 indicates local */
    uint16_t net;               /* BACnet network number */
    /* LEN = 0 denotes broadcast MAC ADR and ADR field is absent */
    /* LEN > 0 specifies length of ADR field */
    int len;                    /* length of MAC address */
    uint8_t adr[MAX_MAC_LEN];   /* hwaddr (MAC) address */
};
typedef struct BACnet_Device_Address BACNET_ADDRESS;

/* date */
typedef struct BACnet_Date {
    uint16_t year;              /* AD */
    uint8_t month;              /* 1=Jan */
    uint8_t day;                /* 1..31 */
    uint8_t wday;               /* 1=Monday-7=Sunday */
} BACNET_DATE;

/* time */
typedef struct BACnet_Time {
    uint8_t hour;
    uint8_t min;
    uint8_t sec;
    uint8_t hundredths;
} BACNET_TIME;

/* note: with microprocessors having lots more code space than memory,
   it might be better to have a packed encoding with a library to
   easily access the data. */
typedef struct BACnet_Object_Id {
    uint16_t type;
    uint32_t instance;
} BACNET_OBJECT_ID;

#define MAX_NPDU (1+1+2+1+MAX_MAC_LEN+2+1+MAX_MAC_LEN+1+1+2)
#define MAX_PDU (MAX_APDU + MAX_NPDU)

#endif
