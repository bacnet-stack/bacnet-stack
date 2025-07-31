/**
 * @file
 * @brief Initializes BACnet Zigbee Link Layer interface (Windows).
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date July 2025
 * @copyright SPDX-License-Identifier: MIT
 */
/* BACnet specific */
#include "bacnet/bacdef.h"
#include "bacnet/bacaddr.h"
#include "bacnet/bacint.h"
#include "bacnet/datalink/bzll.h"
#include "bacnet/basic/sys/debug.h"
#include "bacnet/basic/object/device.h"

/**
 * @brief Initialize the datalink
 * @param ifname - the name of the datalink
 */
bool bzll_init(char *ifname)
{
    (void)ifname;

    return true;
}

/**
 * @brief Send a protocol data unit (PDU) to the network
 * @param dest - destination address
 * @param npdu_data - network routing data
 * @param pdu - protocol data unit (PDU) to send
 * @param pdu_len - size of the protocol data unit (PDU)
 */
int bzll_send_pdu(
    BACNET_ADDRESS *dest,
    BACNET_NPDU_DATA *npdu_data,
    uint8_t *pdu,
    unsigned pdu_len)
{
    (void)dest;
    (void)npdu_data;
    (void)pdu;
    (void)pdu_len;

    return 0;
}

/**
 * @brief Poll the datalink queue to see if a packet has arrived
 * @param src - origin address of the packet
 * @param pdu - protocol data unit (PDU) buffer to store received packet
 * @param pdu_size - size of the protocol data unit (PDU) buffer
 * @param timeout - number of milliseconds to wait for a packet
 */
uint16_t bzll_receive(
    BACNET_ADDRESS *src, uint8_t *pdu, uint16_t pdu_size, unsigned timeout)
{
    (void)src;
    (void)pdu;
    (void)pdu_size;
    (void)timeout;

    return 0;
}

/**
 * @brief cleanup the datalink data or connections
 */
void bzll_cleanup(void)
{
    /* nothing to do */
}

/**
 * @brief Initialize the a data link broadcast address
 * @param dest - address to be filled with broadcast designator
 */
void bzll_get_broadcast_address(BACNET_ADDRESS *dest)
{
    int i = 0;

    if (dest) {
        dest->mac_len = 3;
        for (i = 0; i < MAX_MAC_LEN; i++) {
            dest->mac[i] = 0xFF;
        }
        dest->net = BACNET_BROADCAST_NETWORK;
        /* always zero when DNET is broadcast */
        dest->len = 0;
        for (i = 0; i < MAX_MAC_LEN; i++) {
            dest->adr[i] = 0;
        }
    }

    return;
}

/**
 * @brief Set the BACnet address for my interface
 * @param my_address - address to be filled with my interface address
 */
void bzll_get_my_address(BACNET_ADDRESS *my_address)
{
    int i = 0;
    uint32_t instance;

    instance = Device_Object_Instance_Number();
    bacnet_vmac_address_set(my_address, instance);

    return;
}

/**
 * @brief Set the maintenance timer for the datalink
 * @param seconds - number of seconds to set the timer
 */
void bzll_maintenance_timer(uint16_t seconds)
{
    (void)seconds;
}
