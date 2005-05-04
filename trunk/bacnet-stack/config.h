#ifndef CONFIG_H
#define CONFIG_H

// declare a single physical layer
//#include "ethernet.h"
//#include "arcnet.h"
//#include "mstp.h"

// Max number of bytes in an APDU.
// Typical sizes are 50, 128, 206, 480, 1024, and 1476 octets
// This is used in constructing messages and to tell others our limits
// 50 is the minimum; adjust to your memory and physical layer constraints
// Lon=206, MS/TP=480, ARCNET=480, Ethernet=1476
#define MAX_APDU 50

// for confirmed messages, this is the number of transactions
// that we hold in a queue waiting for timeout.
// Configure to zero if you don't want any confirmed messages
// Configure from 1..255 for number of outstanding confirmed
// requests available.
#define MAX_TSM_TRANSACTIONS 16

// Use only one data link layer
//#define BACDL_ARCNET
//#define BACDL_ETHERNET
//#define BACDL_MSTP
#define BACDL_BIP

#endif
