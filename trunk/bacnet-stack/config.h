#ifdef CONFIG_H
#define CONFIG_H

// Max number of bytes in an APDU.
// Typical sizes are 50, 128, 206, 480, 1024, and 1476 octets
// This is used in constructing messages and to tell others our limits
// 50 is the minimum; adjust to your memory and physical layer constraints
// Lon=206, MS/TP=480, ARCNET=480, Ethernet=1476
#define MAX_APDU 50

#endif
