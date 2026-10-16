/**
 * @file
 * @brief Datalink stub that captures the PDU sent by the handler under test
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date 2026
 * @copyright SPDX-License-Identifier: MIT
 */
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include "bacnet/bacdef.h"
#include "bacnet/npdu.h"
#include "bacnet/datalink/datalink.h"

uint8_t Datalink_Stub_PDU[MAX_PDU];
unsigned Datalink_Stub_PDU_Len;

int datalink_send_pdu(
    BACNET_ADDRESS *dest,
    BACNET_NPDU_DATA *npdu_data,
    uint8_t *pdu,
    unsigned pdu_len)
{
    (void)dest;
    (void)npdu_data;
    if (pdu_len > sizeof(Datalink_Stub_PDU)) {
        pdu_len = sizeof(Datalink_Stub_PDU);
    }
    memcpy(Datalink_Stub_PDU, pdu, pdu_len);
    Datalink_Stub_PDU_Len = pdu_len;

    return (int)pdu_len;
}

void datalink_get_my_address(BACNET_ADDRESS *my_address)
{
    if (my_address) {
        memset(my_address, 0, sizeof(*my_address));
    }
}
