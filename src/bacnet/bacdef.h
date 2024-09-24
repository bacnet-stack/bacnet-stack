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
/* config is always first to allow developers to override */
#include "bacnet/config.h"

#if !defined(PRINT)
#ifdef DEBUG_PRINT
#if (__STDC_VERSION__ >= 199901L) || defined(_MSC_VER)
#define PRINT(...)                                             \
    do {                                                       \
        printf("%s:%d::%s(): ", __FILE__, __LINE__, __func__); \
        printf(__VA_ARGS__);                                   \
        printf("\r\n");                                        \
    } while (0)
#else
#include <stdarg.h>
#include <stdio.h>
#ifdef __APPLE__
#define __PRINT(...)
#else
static inline void __PRINT(const char *format, ...)
{
    va_list args;
    va_start(args, format);
    printf("%s:%d(): ", __FILE__, __LINE__);
    vprintf(format, args);
    va_end(args);
}
#endif
#define PRINT __PRINT
#endif
#else
#include <stdarg.h>
static inline void __PRINT(const char *format, ...)
{
    (void)format;
}
#define PRINT __PRINT
#endif
#endif

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
/* Although this stack can implement a later revision,
 * sometimes another revision is desired */
#ifndef BACNET_PROTOCOL_REVISION
#define BACNET_PROTOCOL_REVISION 24
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
/* from 135-2016 version of the BACnet Standard */
#define MAX_ASHRAE_OBJECT_TYPE 60
#define MAX_BACNET_SERVICES_SUPPORTED 44
#elif (BACNET_PROTOCOL_REVISION == 20) || (BACNET_PROTOCOL_REVISION == 21)
/* Addendum 135-2016bd, 135-2016be, 135-2016bi */
#define MAX_ASHRAE_OBJECT_TYPE 63
#define MAX_BACNET_SERVICES_SUPPORTED 47
#elif (BACNET_PROTOCOL_REVISION == 22)
#define MAX_ASHRAE_OBJECT_TYPE 63
#define MAX_BACNET_SERVICES_SUPPORTED 47
#elif (BACNET_PROTOCOL_REVISION == 23)
/* Addendum 135-2020cd */
#define MAX_ASHRAE_OBJECT_TYPE 63
#define MAX_BACNET_SERVICES_SUPPORTED 47
#elif (BACNET_PROTOCOL_REVISION == 24)
/* Addendum 135-2020ca, 135-2020cc, 135-2020bv */
#define MAX_ASHRAE_OBJECT_TYPE 65
#define MAX_BACNET_SERVICES_SUPPORTED 49
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
/* based on 169 alarms currently in App Charlie */
#define BACNET_MAX_SUPPORTED_INSTANCES 200
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
   BACnet/IPv6 = 3 bytes (VMAC) */
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

/* This Struct is for initialisation info coming from Elixir */
typedef struct BACnet_Object_Init_s {
    /* Instance works as an index to the object within the type, the type number
     * is not included in this value. */
    uint32_t Object_Instance;
    char Object_Name[MAX_CHARACTER_STRING_BYTES]; /* the init values are always
                                                     gonna be english ascii */
    char Description[MAX_CHARACTER_STRING_BYTES];
    uint16_t Units;
} BACNET_OBJECT_INIT_T;

typedef struct BACnet_Object_List_Init_s {
    uint32_t length; /* must be less than BACNET_MAX_SUPPORTED_INSTANCES */
    BACNET_OBJECT_INIT_T Object_Init_Values[BACNET_MAX_SUPPORTED_INSTANCES];
} BACNET_OBJECT_LIST_INIT_T;

#define MAX_NPDU (1 + 1 + 2 + 1 + MAX_MAC_LEN + 2 + 1 + MAX_MAC_LEN + 1 + 1 + 2)
#define MAX_PDU (MAX_APDU + MAX_NPDU)

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
