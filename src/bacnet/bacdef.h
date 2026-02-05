/**
 * @file
 * @brief Core BACnet defines and enumerations and structures
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date 2004
 * @copyright SPDX-License-Identifier: MIT
 */
#ifndef BACNET_DEFINES_H
#define BACNET_DEFINES_H

#include <stddef.h>
#include <stdint.h>
#include <limits.h>
#include <float.h>
/* config is always first to allow developers to override */
#include "bacnet/config.h"
/* BACnet Stack core enumerations */
#include "bacnet/bacenum.h"
/* BACnet Stack libc and compiler abstraction */
#include "bacnet/basic/sys/platform.h"
#include "bacnet/basic/sys/bacnet_stack_exports.h"
/* BACnet Stack common helper macros */
#include "bacnet/basic/sys/bits.h"
#include "bacnet/basic/sys/bytes.h"

/* This stack implements this version of BACnet */
#define BACNET_PROTOCOL_VERSION 1
/* Although this stack can implement any revision,
 * sometimes a specific revision is desired */
#ifndef BACNET_PROTOCOL_REVISION
#define BACNET_PROTOCOL_REVISION 28
#endif

/* there are a few dependencies on the BACnet Protocol-Revision */
#if (BACNET_PROTOCOL_REVISION == 0)
#define MAX_ASHRAE_OBJECT_TYPE 18
#define MAX_BACNET_SERVICES_SUPPORTED 35
#elif (BACNET_PROTOCOL_REVISION == 1)
#define MAX_ASHRAE_OBJECT_TYPE 21
#define MAX_BACNET_SERVICES_SUPPORTED 37
#elif (BACNET_PROTOCOL_REVISION == 2)
/* from 135-2001 version of the BACnet Standard */
#define MAX_ASHRAE_OBJECT_TYPE 23
#define MAX_BACNET_SERVICES_SUPPORTED 40
#elif (BACNET_PROTOCOL_REVISION == 3)
#define MAX_ASHRAE_OBJECT_TYPE 23
#define MAX_BACNET_SERVICES_SUPPORTED 40
#elif (BACNET_PROTOCOL_REVISION == 4)
/* from 135-2004 version of the BACnet Standard */
#define MAX_ASHRAE_OBJECT_TYPE 25
#define MAX_BACNET_SERVICES_SUPPORTED 40
#elif (BACNET_PROTOCOL_REVISION == 5)
#define MAX_ASHRAE_OBJECT_TYPE 30
#define MAX_BACNET_SERVICES_SUPPORTED 40
#elif (BACNET_PROTOCOL_REVISION == 6)
#define MAX_ASHRAE_OBJECT_TYPE 31
#define MAX_BACNET_SERVICES_SUPPORTED 40
#elif (BACNET_PROTOCOL_REVISION == 7)
#define MAX_ASHRAE_OBJECT_TYPE 31
#define MAX_BACNET_SERVICES_SUPPORTED 40
#elif (BACNET_PROTOCOL_REVISION == 8)
#define MAX_ASHRAE_OBJECT_TYPE 31
#define MAX_BACNET_SERVICES_SUPPORTED 40
#elif (BACNET_PROTOCOL_REVISION == 9)
/* from 135-2008 version of the BACnet Standard */
#define MAX_ASHRAE_OBJECT_TYPE 38
#define MAX_BACNET_SERVICES_SUPPORTED 40
#elif (BACNET_PROTOCOL_REVISION == 10)
#define MAX_ASHRAE_OBJECT_TYPE 51
#define MAX_BACNET_SERVICES_SUPPORTED 40
#elif (BACNET_PROTOCOL_REVISION == 11)
#define MAX_ASHRAE_OBJECT_TYPE 51
#define MAX_BACNET_SERVICES_SUPPORTED 40
#elif (BACNET_PROTOCOL_REVISION == 12)
/* from 135-2010 version of the BACnet Standard */
#define MAX_ASHRAE_OBJECT_TYPE 51
#define MAX_BACNET_SERVICES_SUPPORTED 40
#elif (BACNET_PROTOCOL_REVISION == 13)
#define MAX_ASHRAE_OBJECT_TYPE 53
#define MAX_BACNET_SERVICES_SUPPORTED 40
#elif (BACNET_PROTOCOL_REVISION == 14) || (BACNET_PROTOCOL_REVISION == 15)
/* from 135-2012 version of the BACnet Standard */
#define MAX_ASHRAE_OBJECT_TYPE 55
#define MAX_BACNET_SERVICES_SUPPORTED 41
#elif (BACNET_PROTOCOL_REVISION == 16)
/* Addendum 135-2012an, 135-2012at, 135-2012au,
   135-2012av, 135-2012aw, 135-2012ax, 135-2012az */
#define MAX_ASHRAE_OBJECT_TYPE 56
#define MAX_BACNET_SERVICES_SUPPORTED 41
#elif (BACNET_PROTOCOL_REVISION == 17)
/* Addendum 135-2012ai */
#define MAX_ASHRAE_OBJECT_TYPE 57
#define MAX_BACNET_SERVICES_SUPPORTED 41
#elif (BACNET_PROTOCOL_REVISION == 18) || (BACNET_PROTOCOL_REVISION == 19)
/* ANSI/ASHRAE 135-2016 */
#define MAX_ASHRAE_OBJECT_TYPE 60
#define MAX_BACNET_SERVICES_SUPPORTED 44
#elif (BACNET_PROTOCOL_REVISION == 20) || (BACNET_PROTOCOL_REVISION == 21)
/* Addendum 135-2016bd, 135-2016be, 135-2016bi */
#define MAX_ASHRAE_OBJECT_TYPE 63
#define MAX_BACNET_SERVICES_SUPPORTED 47
#elif (BACNET_PROTOCOL_REVISION == 22)
/* Addendum 135-2016bj, 135-2016by, 135-2016bz  */
/* ANSI/ASHRAE 135-2020 */
#define MAX_ASHRAE_OBJECT_TYPE 63
#define MAX_BACNET_SERVICES_SUPPORTED 47
#elif (BACNET_PROTOCOL_REVISION == 23)
/* Addendum 135-2020cd */
#define MAX_ASHRAE_OBJECT_TYPE 63
#define MAX_BACNET_SERVICES_SUPPORTED 47
#elif (BACNET_PROTOCOL_REVISION == 24)
/* Addendum 135-2020bv, 135-2020ca, 135-2020cc, 135-2020ce */
#define MAX_ASHRAE_OBJECT_TYPE 65
#define MAX_BACNET_SERVICES_SUPPORTED 49
#elif (BACNET_PROTOCOL_REVISION == 25)
/* Addendum 135-2020cf */
#define MAX_ASHRAE_OBJECT_TYPE 65
#define MAX_BACNET_SERVICES_SUPPORTED 49
#elif (BACNET_PROTOCOL_REVISION == 26)
/* Addendum 135-2020ch, 135-2020ck, 135-2020cn, 135-2020cq, 135-2020cs */
#define MAX_ASHRAE_OBJECT_TYPE 65
#define MAX_BACNET_SERVICES_SUPPORTED 49
#elif (BACNET_PROTOCOL_REVISION == 27)
/* Addendum 135-2020bx, 135-2020ci */
#define MAX_ASHRAE_OBJECT_TYPE 65
#define MAX_BACNET_SERVICES_SUPPORTED 49
#elif (BACNET_PROTOCOL_REVISION == 28)
/* Addendum 135-2020cj, 135-2020co */
#define MAX_ASHRAE_OBJECT_TYPE 65
#define MAX_BACNET_SERVICES_SUPPORTED 49
#elif (BACNET_PROTOCOL_REVISION == 29)
/* Addendum 135-2020cp Addition of Authentication and Authorization */
/* ANSI/ASHRAE 135-2024 */
#define MAX_ASHRAE_OBJECT_TYPE 65
#define MAX_BACNET_SERVICES_SUPPORTED 50
#elif (BACNET_PROTOCOL_REVISION == 30)
/* Addendum 135-2020cm BACnet Energy Services Interface */
#define MAX_ASHRAE_OBJECT_TYPE 65
#define MAX_BACNET_SERVICES_SUPPORTED 50
#else
#error MAX_ASHRAE_OBJECT_TYPE and MAX_BACNET_SERVICES_SUPPORTED not defined!
#endif

/* Support 64b integers when available */
#ifdef UINT64_MAX
typedef uint64_t BACNET_UNSIGNED_INTEGER;
#define BACNET_UNSIGNED_INTEGER_MAX UINT64_MAX
#else
typedef uint32_t BACNET_UNSIGNED_INTEGER;
#define BACNET_UNSIGNED_INTEGER_MAX UINT32_MAX
#endif

/* largest BACnet Instance Number */
/* Also used as a device instance number wildcard address */
#define BACNET_MAX_INSTANCE (0x3FFFFF)
#define BACNET_INSTANCE_BITS 22
/* large BACnet Object Type */
#define BACNET_MAX_OBJECT (0x3FF)
/* Array index 0=size of array, n=array element n,  MAX=all array elements */
#define BACNET_ARRAY_ALL UINT32_MAX
typedef uint32_t BACNET_ARRAY_INDEX;
/* For device object property references with no device id defined */
#define BACNET_NO_DEV_ID 0xFFFFFFFFu
#define BACNET_NO_DEV_TYPE OBJECT_NONE
/* Priority Array for commandable objects */
#define BACNET_NO_PRIORITY 0
#define BACNET_MIN_PRIORITY 1
#define BACNET_MAX_PRIORITY 16

#define BACNET_BROADCAST_NETWORK (0xFFFF)

/* Any size MAC address could be received which is less than or
   equal to 7 bytes. Standard even allows 6 bytes max. */
/* ARCNET = 1 byte
   MS/TP = 1 byte
   Ethernet = 6 bytes
   BACnet/IPv4 = 6 bytes
   LonTalk = 7 bytes
   BACnet/IPv6 = 3 bytes (VMAC)
   BACnet/SC = 6 bytes (VMAC)
   */
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
    uint16_t net; /* BACnet network number */
    /* LEN = 0 denotes broadcast MAC ADR and ADR field is absent */
    /* LEN > 0 specifies length of ADR field */
    uint8_t len; /* length of MAC address */
    uint8_t adr[MAX_MAC_LEN]; /* hwaddr (MAC) address */
};
typedef struct BACnet_Device_Address BACNET_ADDRESS;
/* define a MAC address for manipulation */
struct BACnet_MAC_Address {
    uint8_t len; /* length of MAC address */
    uint8_t adr[MAX_MAC_LEN];
};
typedef struct BACnet_MAC_Address BACNET_MAC_ADDRESS;

/* note: with microprocessors having lots more code space than memory,
   it might be better to have a packed encoding with a library to
   easily access the data. */
typedef struct BACnet_Object_Id {
    BACNET_OBJECT_TYPE type;
    uint32_t instance;
} BACNET_OBJECT_ID;

#if !defined(BACNET_MAX_SEGMENTS_ACCEPTED)
#if BACNET_SEGMENTATION_ENABLED
/* note: BACNET_MAX_SEGMENTS_ACCEPTED can be 1..255.
   ASDU in this library is usually sized for 16-bit at 65535 max.
   Therefore, the default here is limited to avoid overflow warnings. */
#define BACNET_MAX_SEGMENTS_ACCEPTED 32
#else
#define BACNET_MAX_SEGMENTS_ACCEPTED 1
#endif
#endif
#if !defined(MAX_APDU)
#define MAX_APDU 1476
#endif
#define MAX_NPDU (1 + 1 + 2 + 1 + MAX_MAC_LEN + 2 + 1 + MAX_MAC_LEN + 1 + 1 + 2)
#define MAX_PDU (MAX_APDU + MAX_NPDU)
/* Application Service Data Unit (ASDU) that has not yet been segmented
   into a protocol data unit (PDU) by the lower layer. */
#define MAX_ASDU ((MAX_APDU * BACNET_MAX_SEGMENTS_ACCEPTED) + MAX_NPDU)

#define BACNET_ID_VALUE(bacnet_object_instance, bacnet_object_type)         \
    ((((bacnet_object_type) & BACNET_MAX_OBJECT) << BACNET_INSTANCE_BITS) | \
     ((bacnet_object_instance) & BACNET_MAX_INSTANCE))
#define BACNET_INSTANCE(bacnet_object_id_num) \
    ((bacnet_object_id_num) & BACNET_MAX_INSTANCE)
#define BACNET_TYPE(bacnet_object_id_num) \
    (((bacnet_object_id_num) >> BACNET_INSTANCE_BITS) & BACNET_MAX_OBJECT)

#define BACNET_STATUS_OK (0)
#define BACNET_STATUS_ERROR (-1)
#define BACNET_STATUS_ABORT (-2)
#define BACNET_STATUS_REJECT (-3)

#endif
