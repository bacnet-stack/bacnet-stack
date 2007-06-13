#ifndef CONFIG_H
#define CONFIG_H

/* declare a single physical layer using your compiler define.
   see datalink.h for possible defines. */

/* Max number of bytes in an APDU. */
/* Typical sizes are 50, 128, 206, 480, 1024, and 1476 octets */
/* This is used in constructing messages and to tell others our limits */
/* 50 is the minimum; adjust to your memory and physical layer constraints */
/* Lon=206, MS/TP=480, ARCNET=480, Ethernet=1476 */
#ifndef MAX_APDU
  #define MAX_APDU 50
  /* #define MAX_APDU 480 */
  /* #define MAX_APDU 1476 */
#endif
/* for confirmed messages, this is the number of transactions */
/* that we hold in a queue waiting for timeout. */
/* Configure to zero if you don't want any confirmed messages */
/* Configure from 1..255 for number of outstanding confirmed */
/* requests available. */
#ifndef MAX_TSM_TRANSACTIONS
  #define MAX_TSM_TRANSACTIONS 255
#endif
/* The address cache is used for binding to BACnet devices */
/* The number of entries corresponds to the number of */
/* devices that might respond to an I-Am on the network. */
/* If your device is a simple server and does not need to bind, */
/* then you don't need to use this. */
#ifndef MAX_ADDRESS_CACHE
  #define MAX_ADDRESS_CACHE 255
#endif

#endif
