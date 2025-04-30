/**
 * @file
 * @brief Default configuration for BACnet Stack library
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date 2004
 * @copyright SPDX-License-Identifier: MIT
 */
#ifndef BACNET_CONFIG_H_
#define BACNET_CONFIG_H_

/**
 * @note configurations are default to values used in the example apps build.
 * Use a local copy named "bacnet-config.h" with settings configured for
 * the product specific needs for code space reductions in your device.
 * Alternately, use a compiler and linker to override these defines.
 */
#if defined(BACNET_CONFIG_H)
#include "bacnet-config.h"
#endif

/* Note: these defines can be defined in your makefile or project
   or here or not defined and defaults will be used */

/* Declare a physical layers using your compiler define. See
   datalink.h for possible defines. */

/* For backward compatibility for old BACDL_ALL */
#if defined(BACDL_ALL)
#define BACDL_ETHERNET
#define BACDL_ARCNET
#define BACDL_MSTP
#define BACDL_BIP
#define BACDL_BIP6
#define BACDL_BSC
#endif

#if defined(BACDL_ETHERNET)
#define BACDL_SOME_DATALINK_ENABLED 1
#endif

#if defined(BACDL_ARCNET)
#if defined(BACDL_SOME_DATALINK_ENABLED)
#define BACDL_MULTIPLE 1
#endif
#define BACDL_SOME_DATALINK_ENABLED 1
#endif

#if defined(BACDL_MSTP)
#if defined(BACDL_SOME_DATALINK_ENABLED)
#define BACDL_MULTIPLE 1
#endif
#define BACDL_SOME_DATALINK_ENABLED 1
#endif

#if defined(BACDL_BIP)
#if defined(BACDL_SOME_DATALINK_ENABLED)
#define BACDL_MULTIPLE 1
#endif
#define BACDL_SOME_DATALINK_ENABLED 1
#endif

#if defined(BACDL_BIP6)
#if defined(BACDL_SOME_DATALINK_ENABLED)
#define BACDL_MULTIPLE 1
#endif
#define BACDL_SOME_DATALINK_ENABLED 1
#endif

#if defined(BACDL_BSC)
#if defined(BACDL_SOME_DATALINK_ENABLED)
#define BACDL_MULTIPLE 1
#endif
#define BACDL_SOME_DATALINK_ENABLED 1
#endif

#if defined(BACDL_CUSTOM)
#if defined(BACDL_SOME_DATALINK_ENABLED)
#define BACDL_MULTIPLE 1
#endif
#endif

#if defined(BACNET_SEGMENTATION_ENABLED)
#define BACNET_SEGMENTATION_ENABLED 1
#endif

#if defined(BACDL_SOME_DATALINK_ENABLED) && defined(BACDL_NONE)
#error "BACDL_NONE is not compatible with other BACDL_ defines"
#elif !defined(BACDL_SOME_DATALINK_ENABLED) && !defined(BACDL_NONE) && \
    !defined(BACDL_TEST)
/* If none of the datalink is enabled let's default to BIP. */
#define BACDL_BIP
#endif

/* optional configuration for BACnet/IP datalink layer */
#if (defined(BACDL_BIP))
#if !defined(BBMD_ENABLED)
#define BBMD_ENABLED 1
#endif
#if !defined(BBMD_CLIENT_ENABLED)
#define BBMD_CLIENT_ENABLED 1
#endif
#endif

/* optional configuration for BACnet/IPv6 datalink layer */
#if defined(BACDL_BIP6)
#if !defined(BBMD6_ENABLED)
#define BBMD6_ENABLED 0
#endif
#endif

/* Enable the Gateway (Routing) functionality here, if desired. */
#if !defined(MAX_NUM_DEVICES)
#ifdef BAC_ROUTING
#define MAX_NUM_DEVICES 32 /* Eg, Gateway + 31 remote devices */
#else
#define MAX_NUM_DEVICES 1 /* Just the one normal BACnet Device Object */
#endif
#endif

/* Define your Vendor Identifier assigned by ASHRAE */
#if !defined(BACNET_VENDOR_ID)
#define BACNET_VENDOR_ID 260
#endif
#if !defined(BACNET_VENDOR_NAME)
#define BACNET_VENDOR_NAME "BACnet Stack at SourceForge"
#endif

/* Max number of bytes in an APDU. */
/* Typical sizes are 50, 128, 206, 480, 1024, and 1476 octets */
/* This is used in constructing messages and to tell others our limits */
/* 50 is the minimum; adjust to your memory and physical layer constraints */
/* Lon=206, MS/TP=480 or 1476, ARCNET=480, Ethernet=1476, BACnet/IP=1476 */
#if !defined(MAX_APDU)
/* #define MAX_APDU 50 */
/* #define MAX_APDU 1476 */
#if defined(BACDL_MULTIPLE)
#define MAX_APDU 1476
#elif defined(BACDL_BIP)
#define MAX_APDU 1476
/* Enable this IP for testing readrange so you get the More Follows flag set */
/* #define MAX_APDU 128 */
#elif defined(BACDL_BIP6)
#define MAX_APDU 1476
#elif defined(BACDL_MSTP) && !defined(BACNET_SECURITY)
/* note: MS/TP extended frames can be up to 1476 bytes */
#define MAX_APDU 1476
#elif defined(BACDL_ETHERNET) && !defined(BACNET_SECURITY)
#define MAX_APDU 1476
#elif defined(BACDL_ETHERNET) && defined(BACNET_SECURITY)
#define MAX_APDU 1420
#elif !defined(BACNET_SECURITY)
#define MAX_APDU 480
#elif defined(BACDL_MSTP) && defined(BACNET_SECURITY)
/* TODO: Is this really 412 or should it be 480? */
#define MAX_APDU 412
#else
#define MAX_APDU 412
#endif
#endif

#ifndef SC_NETPORT_BVLC_MAX
#define SC_NETPORT_BVLC_MAX 1500
#endif
#ifndef SC_NETPORT_NPDU_MAX
#define SC_NETPORT_NPDU_MAX 1500
#endif
#ifndef SC_NETPORT_CONNECT_TIMEOUT
#define SC_NETPORT_CONNECT_TIMEOUT 5
#endif
#ifndef SC_NETPORT_HEARTBEAT_TIMEOUT
#define SC_NETPORT_HEARTBEAT_TIMEOUT 60
#endif
#ifndef SC_NETPORT_DISCONNECT_TIMEOUT
#define SC_NETPORT_DISCONNECT_TIMEOUT 150
#endif
#ifndef SC_NETPORT_RECONNECT_TIME
#define SC_NETPORT_RECONNECT_TIME 2
#endif

/* for confirmed messages, this is the number of transactions */
/* that we hold in a queue waiting for timeout. */
/* Configure to zero if you don't want any confirmed messages */
/* Configure from 1..255 for number of outstanding confirmed */
/* requests available. */
#if !defined(MAX_TSM_TRANSACTIONS)
#define MAX_TSM_TRANSACTIONS 255
#endif
/* The address cache is used for binding to BACnet devices */
/* The number of entries corresponds to the number of */
/* devices that might respond to an I-Am on the network. */
/* If your device is a simple server and does not need to bind, */
/* then you don't need to use this. */
#if !defined(MAX_ADDRESS_CACHE)
#define MAX_ADDRESS_CACHE 255
#endif

#if BACNET_SEGMENTATION_ENABLED
/* for confirmed segmented messages, this is the number of peer
   segmented requests we can stand at the same time. */
#if !defined(MAX_TSM_PEERS)
#define MAX_TSM_PEERS 16
#endif

/* for segmented messages, this is the number of segments accepted */
/* (max memory allocated for one message : MAX_APDU * MAX_SEGMENTS) */
#if !defined(BACNET_MAX_SEGMENTS_ACCEPTED)
#define BACNET_MAX_SEGMENTS_ACCEPTED 255
#endif
#else
#if !defined(BACNET_MAX_SEGMENTS_ACCEPTED)
#define BACNET_MAX_SEGMENTS_ACCEPTED 1
#endif
#endif

/* some modules have debugging enabled using PRINT_ENABLED */
#if !defined(PRINT_ENABLED)
#define PRINT_ENABLED 0
#endif

/* BACAPP decodes WriteProperty service requests
   Choose the datatypes that your application supports */
/* clang-format off */
#if !( \
    defined(BACAPP_ALL) || \
    defined(BACAPP_MINIMAL) || \
    defined(BACAPP_NULL) || \
    defined(BACAPP_BOOLEAN) || \
    defined(BACAPP_UNSIGNED) || \
    defined(BACAPP_SIGNED) || \
    defined(BACAPP_REAL) || \
    defined(BACAPP_DOUBLE) || \
    defined(BACAPP_OCTET_STRING) || \
    defined(BACAPP_CHARACTER_STRING) || \
    defined(BACAPP_BIT_STRING) || \
    defined(BACAPP_ENUMERATED) || \
    defined(BACAPP_DATE) || \
    defined(BACAPP_TIME) || \
    defined(BACAPP_OBJECT_ID) || \
    defined(BACAPP_DATETIME) || \
    defined(BACAPP_DATERANGE) || \
    defined(BACAPP_LIGHTING_COMMAND) || \
    defined(BACAPP_XY_COLOR) || \
    defined(BACAPP_COLOR_COMMAND) || \
    defined(BACAPP_WEEKLY_SCHEDULE) || \
    defined(BACAPP_CALENDAR_ENTRY) || \
    defined(BACAPP_SPECIAL_EVENT) || \
    defined(BACAPP_HOST_N_PORT) || \
    defined(BACAPP_DEVICE_OBJECT_PROPERTY_REFERENCE) || \
    defined(BACAPP_DEVICE_OBJECT_REFERENCE) || \
    defined(BACAPP_OBJECT_PROPERTY_REFERENCE) || \
    defined(BACAPP_DESTINATION) || \
    defined(BACAPP_BDT_ENTRY) || \
    defined(BACAPP_FDT_ENTRY) || \
    defined(BACAPP_ACTION_COMMAND) || \
    defined(BACAPP_SCALE) || \
    defined(BACAPP_SHED_LEVEL) || \
    defined(BACAPP_ACCESS_RULE) || \
    defined(BACAPP_CHANNEL_VALUE) || \
    defined(BACAPP_LOG_RECORD) || \
    defined(BACAPP_SECURE_CONNECT) || \
    defined(BACAPP_TYPES_EXTRA))
#define BACAPP_ALL
#endif
/* clang-format on */

#if defined(BACAPP_ALL)
#define BACAPP_MINIMAL
#define BACAPP_TYPES_EXTRA
#endif

#if defined(BACAPP_MINIMAL)
#define BACAPP_NULL
#define BACAPP_BOOLEAN
#define BACAPP_UNSIGNED
#define BACAPP_SIGNED
#define BACAPP_REAL
#define BACAPP_CHARACTER_STRING
#define BACAPP_OCTET_STRING
#define BACAPP_BIT_STRING
#define BACAPP_ENUMERATED
#define BACAPP_DATE
#define BACAPP_TIME
#define BACAPP_OBJECT_ID
#endif

#if defined(BACAPP_TYPES_EXTRA)
#define BACAPP_DOUBLE
#define BACAPP_TIMESTAMP
#define BACAPP_DATETIME
#define BACAPP_DATERANGE
#define BACAPP_LIGHTING_COMMAND
#define BACAPP_XY_COLOR
#define BACAPP_COLOR_COMMAND
#define BACAPP_WEEKLY_SCHEDULE
#define BACAPP_CALENDAR_ENTRY
#define BACAPP_SPECIAL_EVENT
#define BACAPP_HOST_N_PORT
#define BACAPP_DEVICE_OBJECT_PROPERTY_REFERENCE
#define BACAPP_DEVICE_OBJECT_REFERENCE
#define BACAPP_OBJECT_PROPERTY_REFERENCE
#define BACAPP_DESTINATION
#define BACAPP_BDT_ENTRY
#define BACAPP_FDT_ENTRY
#define BACAPP_ACTION_COMMAND
#define BACAPP_SCALE
#define BACAPP_SHED_LEVEL
#define BACAPP_ACCESS_RULE
#define BACAPP_CHANNEL_VALUE
#define BACAPP_LOG_RECORD
#define BACAPP_SECURE_CONNECT
#endif

/* clang-format off */
#if defined(BACAPP_DOUBLE) || \
    defined(BACAPP_DATETIME) || \
    defined(BACAPP_DATERANGE) || \
    defined(BACAPP_LIGHTING_COMMAND) || \
    defined(BACAPP_XY_COLOR) || \
    defined(BACAPP_COLOR_COMMAND) || \
    defined(BACAPP_WEEKLY_SCHEDULE) || \
    defined(BACAPP_CALENDAR_ENTRY) || \
    defined(BACAPP_SPECIAL_EVENT) || \
    defined(BACAPP_HOST_N_PORT) || \
    defined(BACAPP_DEVICE_OBJECT_PROPERTY_REFERENCE) || \
    defined(BACAPP_DEVICE_OBJECT_REFERENCE) || \
    defined(BACAPP_OBJECT_PROPERTY_REFERENCE) || \
    defined(BACAPP_DESTINATION) || \
    defined(BACAPP_SECURE_CONNECT) || \
    defined(BACAPP_BDT_ENTRY) || \
    defined(BACAPP_FDT_ENTRY) || \
    defined(BACAPP_ACTION_COMMAND) || \
    defined(BACAPP_SCALE) || \
    defined(BACAPP_SHED_LEVEL) || \
    defined(BACAPP_ACCESS_RULE) || \
    defined(BACAPP_CHANNEL_VALUE) || \
    defined(BACAPP_LOG_RECORD)
#define BACAPP_COMPLEX_TYPES
#endif
/* clang-format on */

/*
** Set the maximum vector type sizes
*/
#ifndef MAX_BITSTRING_BYTES
#define MAX_BITSTRING_BYTES (15)
#endif

#ifndef MAX_CHARACTER_STRING_BYTES
#define MAX_CHARACTER_STRING_BYTES (MAX_APDU - 6)
#endif

#ifndef MAX_OCTET_STRING_BYTES
#define MAX_OCTET_STRING_BYTES (MAX_APDU - 6)
#endif

/**
 * @note Control the selection of services etc to enable code size reduction
 * for those compiler suites which do not handle removing of unused functions
 * in modules so well.
 *
 * We will start with the A type services code first as these are least likely
 * to be required in embedded systems using the stack.
 */
#ifndef BACNET_SVC_SERVER
/* default to client-server device for the example apps to build. */
#define BACNET_SVC_SERVER 0
#endif

#if (BACNET_SVC_SERVER == 0)
/* client-server device */
#define BACNET_SVC_I_HAVE_A 1
#define BACNET_SVC_WP_A 1
#define BACNET_SVC_RP_A 1
#define BACNET_SVC_RPM_A 1
#define BACNET_SVC_DCC_A 1
#define BACNET_SVC_RD_A 1
#define BACNET_SVC_TS_A 1
#define BACNET_USE_OCTETSTRING 1
#define BACNET_USE_DOUBLE 1
#define BACNET_USE_SIGNED 1
#endif

/* Do them one by one */
#ifndef BACNET_SVC_I_HAVE_A /* Do we send I_Have requests? */
#define BACNET_SVC_I_HAVE_A 0
#endif

#ifndef BACNET_SVC_WP_A /* Do we send WriteProperty requests? */
#define BACNET_SVC_WP_A 0
#endif

#ifndef BACNET_SVC_RP_A /* Do we send ReadProperty requests? */
#define BACNET_SVC_RP_A 0
#endif

#ifndef BACNET_SVC_RPM_A /* Do we send ReadPropertyMultiple requests? */
#define BACNET_SVC_RPM_A 0
#endif

#ifndef BACNET_SVC_DCC_A /* Do we send DeviceCommunicationControl requests? */
#define BACNET_SVC_DCC_A 0
#endif

#ifndef BACNET_SVC_RD_A /* Do we send ReinitialiseDevice requests? */
#define BACNET_SVC_RD_A 0
#endif

#ifndef BACNET_USE_OCTETSTRING /* Do we need any octet strings? */
#define BACNET_USE_OCTETSTRING 0
#endif

#ifndef BACNET_USE_DOUBLE /* Do we need any doubles? */
#define BACNET_USE_DOUBLE 0
#endif

#ifndef BACNET_USE_SIGNED /* Do we need any signed integers */
#define BACNET_USE_SIGNED 0
#endif

#endif
