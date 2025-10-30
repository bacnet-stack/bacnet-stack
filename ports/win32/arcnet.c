/**
 * @file
 * @brief Provides port specific functions for ARCNET
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date October 2025
 * @copyright SPDX-License-Identifier: MIT
 */
#include <stdint.h>
#include "bacnet/bacdef.h"
#include "bacnet/npdu.h"
#include "bacnet/datalink/arcnet.h"

/* my local device data - MAC address */
uint8_t ARCNET_MAC_Address = 0;
/* ARCNET file handle */
static int ARCNET_Sock_FD = -1;
/* Broadcast address */
#define ARCNET_BROADCAST 0

/*
Hints:

When using a PCI20-485D ARCNET card from Contemporary Controls,
you might need to know about the following settings:

Assuming a 20MHz clock on the COM20020 chip:

clockp Clock Prescaler DataRate
------ --------------- --------
0           8          2.5 Mbps
1           16         1.25 Mbps
2           32         625 Kbps
3           64         312.5 Kbps
4           128        156.25Kbps

1. Install the arcnet driver and arcnet raw mode driver
2. The hardware address (MAC address) is set using the dipswitch
   on the back of the card.  0 is broadcast, so don't use 0.
3. The backplane mode on the PCI20-485D card is done in hardware,
   so the driver does not need to do backplane mode.  If you
   use another type of PCI20 card, you could pass in backplane=1 or
   backplane=0 as an option to the modprobe of com20020_pci.
*/

bool arcnet_valid(void)
{
    return (ARCNET_Sock_FD >= 0);
}

void arcnet_cleanup(void)
{
    if (arcnet_valid()) {
        /* close the interface */
    }
    ARCNET_Sock_FD = -1;

    return;
}

bool arcnet_init(char *interface_name)
{
    (void)interface_name;
    return arcnet_valid();
}

/* function to send a PDU out the socket */
/* returns number of bytes sent on success, negative on failure */
int arcnet_send_pdu(
    BACNET_ADDRESS *dest, /* destination address */
    BACNET_NPDU_DATA *npdu_data, /* network information */
    uint8_t *pdu, /* any data to be sent - may be null */
    unsigned pdu_len)
{
    int bytes = 0;

    (void)dest;
    (void)npdu_data;
    (void)pdu;
    (void)pdu_len;

    return bytes;
}

/* receives an framed packet */
/* returns the number of octets in the PDU, or zero on failure */
uint16_t arcnet_receive(
    BACNET_ADDRESS *src, /* source address */
    uint8_t *pdu, /* PDU data */
    uint16_t pdu_size, /* amount of space available in the PDU  */
    unsigned timeout)
{
    (void)src;
    (void)pdu;
    (void)timeout;
    (void)pdu_size;
    return 0;
}

void arcnet_get_my_address(BACNET_ADDRESS *my_address)
{
    int i = 0;

    my_address->mac_len = 1;
    my_address->mac[0] = ARCNET_MAC_Address;
    my_address->net = 0; /* DNET=0 is local only, no routing */
    my_address->len = 0;
    for (i = 0; i < MAX_MAC_LEN; i++) {
        my_address->adr[i] = 0;
    }

    return;
}

void arcnet_get_broadcast_address(BACNET_ADDRESS *dest)
{ /* destination address */
    int i = 0; /* counter */

    if (dest) {
        dest->mac[0] = ARCNET_BROADCAST;
        dest->mac_len = 1;
        dest->net = BACNET_BROADCAST_NETWORK;
        dest->len = 0; /* always zero when DNET is broadcast */
        for (i = 0; i < MAX_MAC_LEN; i++) {
            dest->adr[i] = 0;
        }
    }

    return;
}
