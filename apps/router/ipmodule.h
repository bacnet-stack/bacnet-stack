/**
* @file
* @author Andriy Sukhynyuk, Vasyl Tkhir, Andriy Ivasiv
* @date 2012
* @brief Datalink IP module
*
* @section LICENSE
*
* SPDX-License-Identifier: MIT
*/
#ifndef UDPMODULE_H
#define UDPMODULE_H

#include <stdint.h>
#include <stdbool.h>
#include "bacport.h"
#include "portthread.h"
#include "bacnet/datalink/bip.h"

#define MAX_BIP_APDU    1476
#define MAX_BIP_PDU     (MAX_NPDU + MAX_BIP_APDU)
#define MAX_BIP_MPDU    (BIP_HEADER_MAX + MAX_BIP_PDU)
/* Yes, we know this is longer than an Ethernet Frame,
   a UDP payload and an IPv6 packet.
   Grandfathered in from BACnet Ethernet days,
   and we can rely on the lower layers of the
   Ethernet stack to fragment/reassemble the BACnet MPDUs */

typedef struct ip_data {
    int socket;
    uint16_t port;
    struct in_addr local_addr;
    struct in_addr broadcast_addr;
    uint8_t *buff;
    uint16_t max_buff;
} IP_DATA;


void *dl_ip_thread(
    void *pArgs);

bool dl_ip_init(
    ROUTER_PORT * port,
    IP_DATA * data);

int dl_ip_send(
    IP_DATA * data,
    const BACNET_ADDRESS * dest,
    const uint8_t * pdu,
    unsigned pdu_len);

int dl_ip_recv(
    IP_DATA * data,
    MSG_DATA ** msg,    /* on recieve fill up message */
    BACNET_ADDRESS * src,
    unsigned timeout);

void dl_ip_cleanup(
    IP_DATA * data);

#endif /* end of UDPMODULE_H */
