/**************************************************************************
*
* Copyright (C) 2012 Steve Karg <skarg@users.sourceforge.net>
*
* Permission is hereby granted, free of charge, to any person obtaining
* a copy of this software and associated documentation files (the
* "Software"), to deal in the Software without restriction, including
* without limitation the rights to use, copy, modify, merge, publish,
* distribute, sublicense, and/or sell copies of the Software, and to
* permit persons to whom the Software is furnished to do so, subject to
* the following conditions:
*
* The above copyright notice and this permission notice shall be included
* in all copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*********************************************************************/
#ifndef BACDEF_H
#define BACDEF_H

#include <stddef.h>
#include <stdint.h>
#include "bacenum.h"
#include "config.h"

#if defined(_MSC_VER)
/* Silence the warnings about unsafe versions of library functions */
/* as we need to keep the code portable */
#pragma warning( disable : 4996)
#endif

/* This stack implements this version of BACnet */
#define BACNET_PROTOCOL_VERSION 1
#define BACNET_PROTOCOL_REVISION 10

/* largest BACnet Instance Number */
/* Also used as a device instance number wildcard address */
#define BACNET_MAX_INSTANCE (0x3FFFFF)
#define BACNET_INSTANCE_BITS 22
/* large BACnet Object Type */
#define BACNET_MAX_OBJECT (0x3FF)
/* Array index 0=size of array, n=array element n,  MAX=all array elements */
/* 32-bit MAX, to use with uint32_t */
#define BACNET_ARRAY_ALL (0xFFFFFFFFU)
/* Priority Array for commandable objects */
#define BACNET_NO_PRIORITY 0
#define BACNET_MIN_PRIORITY 1
#define BACNET_MAX_PRIORITY 16

#define BACNET_BROADCAST_NETWORK (0xFFFF)
/* Any size MAC address should be allowed which is less than or
   equal to 7 bytes.  The IPv6 addresses are planned to be handled
   outside this area. */
/* FIXME: mac[] only needs to be as big as our local datalink MAC */
#define MAX_MAC_LEN 7
struct BACnet_Device_Address {
    /* mac_len = 0 is a broadcast address */
    uint8_t mac_len;
    /* note: MAC for IP addresses uses 4 bytes for addr, 2 bytes for port */
    /* use de/encode_unsigned32/16 for re/storing the IP address */
    uint8_t mac[MAX_MAC_LEN];
    /* DNET,DLEN,DADR or SNET,SLEN,SADR */
    /* the following are used if the device is behind a router */
    /* net = 0 indicates local */
    uint16_t net;       /* BACnet network number */
    /* LEN = 0 denotes broadcast MAC ADR and ADR field is absent */
    /* LEN > 0 specifies length of ADR field */
    uint8_t len;        /* length of MAC address */
    uint8_t adr[MAX_MAC_LEN];   /* hwaddr (MAC) address */
};
typedef struct BACnet_Device_Address BACNET_ADDRESS;

/* note: with microprocessors having lots more code space than memory,
   it might be better to have a packed encoding with a library to
   easily access the data. */
typedef struct BACnet_Object_Id {
    uint16_t type;
    uint32_t instance;
} BACNET_OBJECT_ID;

#define MAX_NPDU (1+1+2+1+MAX_MAC_LEN+2+1+MAX_MAC_LEN+1+1+2)
#define MAX_PDU (MAX_APDU + MAX_NPDU)

#define BACNET_ID_VALUE(bacnet_object_instance, bacnet_object_type) ((((bacnet_object_type) & BACNET_MAX_OBJECT) << BACNET_INSTANCE_BITS) | ((bacnet_object_instance) & BACNET_MAX_INSTANCE))
#define BACNET_INSTANCE(bacnet_object_id_num) ((bacnet_object_id_num)&BACNET_MAX_INSTANCE)
#define BACNET_TYPE(bacnet_object_id_num) (((bacnet_object_id_num) >> BACNET_INSTANCE_BITS ) & BACNET_MAX_OBJECT)

#define BACNET_STATUS_OK (0)
#define BACNET_STATUS_ERROR (-1)
#define BACNET_STATUS_ABORT (-2)
#define BACNET_STATUS_REJECT (-3)

#endif
