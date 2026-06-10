/**
 * @file
 * @brief mock BACnet/IP datalink functions
 * @author Steve Karg
 * @date September 2025
 * @copyright SPDX-License-Identifier: MIT
 */
#include <zephyr/ztest.h>
#include <bacnet/basic/tsm/tsm.h>

void bip_get_my_address(BACNET_ADDRESS *my_address)
{
    if (my_address) {
        my_address->mac_len = 0;
    }
}

int bip_send_pdu(
    BACNET_ADDRESS *dest,
    BACNET_NPDU_DATA *npdu_data,
    uint8_t *pdu,
    uint16_t pdu_len)
{
    (void)dest;
    (void)npdu_data;
    (void)pdu;
    (void)pdu_len;

    return 0;
}
