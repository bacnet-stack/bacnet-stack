/**************************************************************************
*
* Copyright (C) 2004 Steve Karg <skarg@users.sourceforge.net>
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
*
*********************************************************************/
#ifndef CONFIG_H
#define CONFIG_H

/* Note: these defines can be defined in your makefile or project 
   or here or not defined and defaults will be used */

/* declare a single physical layer using your compiler define.
   see datalink.h for possible defines. */
#if !(defined(BACDL_ETHERNET) || defined(BACDL_ARCNET) || defined(BACDL_MSTP) || defined(BACDL_BIP) || defined(BACDL_TEST) || defined(BACDL_ALL))
#define BACDL_BIP
#endif

/* optional configuration for BACnet/IP datalink layers */
#if (defined(BACDL_BIP) || defined(BACDL_ALL))
/* other BIP defines (define as 1 to enable): 
    USE_INADDR - uses INADDR_BROADCAST for broadcast and binds using INADDR_ANY 
    USE_CLASSADDR = uses IN_CLASSx_HOST where x=A,B,C or D for broadcast
*/
/* Use BVLC functions (e.g. register as foreign client) */
#if !defined(BVLC_ENABLED)
#define BVLC_ENABLED 1
#endif
/* Use BBMD functions (e.g. acts as a BBMD)*/
#if !defined(BBMD_ENABLED)
#define BBMD_ENABLED 1 
#endif
#endif

/* Optional use macro-receive/send function */
/*#define WITH_MACRO_LINK_FUNCTIONS  */

/* Define your processor architecture as 
   Big Endian (PowerPC,68K,Sparc) or Little Endian (Intel,AVR)
   ARM and MIPS can be either - what is your setup? */
#if !defined(BIG_ENDIAN)
#define BIG_ENDIAN 0
#endif

/* Define your Vendor Identifier assigned by ASHRAE */
#if !defined(BACNET_VENDOR_ID)
#define BACNET_VENDOR_ID 414
#endif
#if !defined(BACNET_VENDOR_NAME)
#define BACNET_VENDOR_NAME "Areal"
#endif

/* Force bigger string names support */
#define MAX_CHARACTER_STRING_BYTES  1470
#define MAX_OCTET_STRING_BYTES      1470

/* D�finir un APDU avantageux */
#define MAX_APDU 1476
/* Nombre d'adresses � m�moriser */
#define MAX_ADDRESS_CACHE 1024
/*#define BACFILE */
/* Fonctions */
#define WITH_SESSION_SYNCHRONISATION
#define WITH_SESSION_LOG
/* TSM Sp�cifique */
#define MAX_TSM_TRANSACTIONS 255

/* Max number of bytes in an APDU. */
/* Typical sizes are 50, 128, 206, 480, 1024, and 1476 octets */
/* This is used in constructing messages and to tell others our limits */
/* 50 is the minimum; adjust to your memory and physical layer constraints */
/* Lon=206, MS/TP=480, ARCNET=480, Ethernet=1476, BACnet/IP=1476 */
#if !defined(MAX_APDU)
    /* #define MAX_APDU 50 */
    /* #define MAX_APDU 1476 */
#if defined(BACDL_BIP)
#define MAX_APDU 1476
/* #define MAX_APDU 128 enable this IP for testing readrange so you get the More Follows flag set */
#elif defined (BACDL_ETHERNET)
#define MAX_APDU 1476
#else
#define MAX_APDU 480
#endif
#endif

/* for confirmed messages, this is the number of transactions */
/* that we hold in a queue waiting for timeout. */
/* Configure to zero if you don't want any confirmed messages */
/* Configure from 1..255 for number of outstanding confirmed */
/* requests available. */
#if !defined(MAX_TSM_TRANSACTIONS)
#define MAX_TSM_TRANSACTIONS 255
#endif

/* for confirmed segmented messages, this is the number of peer
   segmented requests we can stand at the same time. */
#if !defined(MAX_TSM_PEERS)
#define MAX_TSM_PEERS 16
#endif

/* for segmented messages, this is the number of segments accepted */
/* (max memory allocated for one message : MAX_APDU * MAX_SEGMENTS) */
#if !defined(MAX_SEGMENTS_ACCEPTED)
#define MAX_SEGMENTS_ACCEPTED 1024
#endif

/* The address cache is used for binding to BACnet devices */
/* The number of entries corresponds to the number of */
/* devices that might respond to an I-Am on the network. */
/* If your device is a simple server and does not need to bind, */
/* then you don't need to use this. */
#if !defined(MAX_ADDRESS_CACHE)
#define MAX_ADDRESS_CACHE 255
#endif

/* some modules have debugging enabled using PRINT_ENABLED */
#if !defined(PRINT_ENABLED)
#define PRINT_ENABLED 0
#endif

/* BACAPP decodes WriteProperty service requests
   Choose the datatypes that your application supports */
#if !(defined(BACAPP_ALL) || \
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
    defined(BACAPP_OBJECT_ID))
#define BACAPP_ALL
#endif

#if defined (BACAPP_ALL)
#define BACAPP_NULL
#define BACAPP_BOOLEAN
#define BACAPP_UNSIGNED
#define BACAPP_SIGNED
#define BACAPP_REAL
#define BACAPP_DOUBLE
#define BACAPP_OCTET_STRING
#define BACAPP_CHARACTER_STRING
#define BACAPP_BIT_STRING
#define BACAPP_ENUMERATED
#define BACAPP_DATE
#define BACAPP_TIME
#define BACAPP_OBJECT_ID
#define BACAPP_TYPES_EXTRA
#endif

/*
** Set the maximum vector type sizes
*/
#ifndef MAX_BITSTRING_BYTES
#define MAX_BITSTRING_BYTES (15)
#endif

#ifndef MAX_CHARACTER_STRING_BYTES
#define MAX_CHARACTER_STRING_BYTES (MAX_APDU-6)
#endif

#ifndef MAX_OCTET_STRING_BYTES
#define MAX_OCTET_STRING_BYTES (MAX_APDU-6)
#endif

#endif
