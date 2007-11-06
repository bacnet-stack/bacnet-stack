#ifndef CONFIG_H
#define CONFIG_H

/* Note: these defines can be defined in your makefile or project 
   or here or not defined and defaults will be used */

/* declare a single physical layer using your compiler define.
   see datalink.h for possible defines. */
#if !(defined(BACDL_ETHERNET) || defined(BACDL_ARCNET) || defined(BACDL_MSTP) || defined(BACDL_BIP) || defined(BACDL_TEST))
    #define BACDL_BIP
#endif

/* optional debug info for BACnet/IP datalink layers */
#if defined(BACDL_BIP)
    #if !defined(USE_INADDR)
        #define USE_INADDR 1
    #endif
#endif

/* Define your processor architecture as 
   Big Endian (PowerPC,68K,Sparc) or Little Endian (Intel,AVR)
   ARM and MIPS can be either - what is your setup? */
#if !defined(BIG_ENDIAN)
    #define BIG_ENDIAN 0
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
/* Lon=206, MS/TP=480, ARCNET=480, Ethernet=1476, BACnet/IP=64K */
#if !defined(MAX_APDU)
    /* #define MAX_APDU 50 */
    #define MAX_APDU 480
    /* #define MAX_APDU 1476 */
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

/* some modules have debugging enabled using PRINT_ENABLED */
#if !defined(PRINT_ENABLED)
    #define PRINT_ENABLED 0
#endif

/* BACAPP decodes WriteProperty service requests
   Choose the datatypes that your application supports */
#if defined (BACAPP_ALL)
    #define BACAPP_NULL
    #define BACAPP_BOOLEAN
    #define BACAPP_UNSIGNED
    #define BACAPP_SIGNED
    #define BACAPP_REAL
/* FIXME: not implemented #define BACAPP_DOUBLE */
    #define BACAPP_OCTET_STRING
    #define BACAPP_CHARACTER_STRING
    #define BACAPP_BIT_STRING
    #define BACAPP_ENUMERATED
    #define BACAPP_DATE
    #define BACAPP_TIME
    #define BACAPP_OBJECT_ID
#endif

#endif
