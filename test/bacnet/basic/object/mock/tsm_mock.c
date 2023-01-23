/**
 * @file
 * @brief mock TSM handler functions
 * @author Steve Karg
 * @date January 2023
 *
 * SPDX-License-Identifier: MIT
 */
#include <zephyr/ztest.h>
#include <bacnet/basic/tsm/tsm.h>

bool tsm_get_transaction_pdu(
    uint8_t invokeID,
    BACNET_ADDRESS * dest,
    BACNET_NPDU_DATA * ndpu_data,
    uint8_t * apdu,
    uint16_t * apdu_len)
{
    (void)invokeID;
    (void)dest;
    (void)ndpu_data;
    (void)apdu;
    (void)apdu_len;

    return false;
}
