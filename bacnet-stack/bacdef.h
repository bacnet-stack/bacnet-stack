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

#include <stdint.h>

// largest BACnet Instance Number
// Also used as a device instance number wildcard address
#define BACNET_MAX_INSTANCE (0x3FFFFF)
// large BACnet Object Type
#define BACNET_MAX_OBJECT (0x3FF)
// Array index 0=size of array, n=array element n,  MAX=all array elements
#define BACNET_ARRAY_ALL (~0)

#define MAX_MAC_LEN 8
struct BACnet_Device_Address {
    // mac_len = 0 if global address
    int mac_len;
    uint8_t mac[MAX_MAC_LEN];
    // DNET,DLEN,DADR or SNET,SLEN,SADR
    // the following are used if the device is behind a router
    // net = 0 indicates local
    uint16_t net; /* BACnet network number */
    // LEN = 0 denotes broadcast MAC ADR and ADR field is absent
    // LEN > 0 specifies length of ADR field
    int len; /* length of MAC address */
    uint8_t adr[MAX_MAC_LEN]; /* hwaddr (MAC) address */
};
typedef struct BACnet_Device_Address BACNET_ADDRESS;

// Max number of bytes in an APDU.
// Typical sizes are 50, 128, 206, 480, 1024, and 1476 octets
// This is used in constructing messages and to tell others our limits
// 50 is the minimum; adjust to your memory and physical layer constraints
// Lon=206, MS/TP=480, ARCNET=480, Ethernet=1476
#define MAX_APDU 50
#define MAX_NPDU (1+1+2+1+MAX_MAC_LEN+2+1+MAX_MAC_LEN+1+1+2)
#define MAX_PDU (MAX_APDU + MAX_NPDU)

// FIXME: maybe we can encapsulate the Physical layer details in the
// physical layer modules and not have to allocate the entire packet

// packet includes physical layer octets such as destination, len, etc.
// this is highly dependent on the physical layer used
// ARCNET=1+1+2+2+1+1+1+1=10
// MS/TP=2+1+1+1+2+1+2+1=11
// Ethernet=6+6+2+1+1+1=17
#ifdef BACNET_ARCNET
  #define MAX_HEADER 10
#endif
#ifdef BACNET_MSTP
  #define MAX_HEADER 11
#endif
#ifdef BACNET_ETHERNET
  #define MAX_HEADER 17
#endif
#define MAX_MPDU (MAX_HEADER+MAX_PDU)
#endif
