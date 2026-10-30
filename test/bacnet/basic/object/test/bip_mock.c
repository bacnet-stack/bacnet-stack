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

static int (*Mock_Send_PDU)(
    BACNET_ADDRESS *dest,
    BACNET_NPDU_DATA *npdu_data,
    uint8_t *pdu,
    uint16_t pdu_len);

/**
 * @brief Test hook: handle the PDU passed to bip_send_pdu()
 * @param send_pdu - function that handles the PDU and returns the number
 *  of bytes sent, or NULL to restore the default
 */
void bip_send_pdu_callback_set(int (*send_pdu)(
    BACNET_ADDRESS *dest,
    BACNET_NPDU_DATA *npdu_data,
    uint8_t *pdu,
    uint16_t pdu_len))
{
    Mock_Send_PDU = send_pdu;
}

int bip_send_pdu(
    BACNET_ADDRESS *dest,
    BACNET_NPDU_DATA *npdu_data,
    uint8_t *pdu,
    uint16_t pdu_len)
{
    if (Mock_Send_PDU) {
        return Mock_Send_PDU(dest, npdu_data, pdu, pdu_len);
    }
    (void)dest;
    (void)npdu_data;
    (void)pdu;
    (void)pdu_len;

    return 0;
}
