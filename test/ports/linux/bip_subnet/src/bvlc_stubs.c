/* SPDX-License-Identifier: MIT */
#include "bacnet/basic/bbmd/h_bbmd.h"

int bvlc_handler(
    BACNET_IP_ADDRESS *addr,
    BACNET_ADDRESS *src,
    uint8_t *npdu,
    uint16_t npdu_len)
{
    (void)addr;
    (void)src;
    (void)npdu;
    (void)npdu_len;

    return 0;
}

int bvlc_broadcast_handler(
    BACNET_IP_ADDRESS *addr,
    BACNET_ADDRESS *src,
    uint8_t *npdu,
    uint16_t npdu_len)
{
    (void)addr;
    (void)src;
    (void)npdu;
    (void)npdu_len;

    return 0;
}

int bvlc_send_pdu(
    const BACNET_ADDRESS *dest,
    const BACNET_NPDU_DATA *npdu_data,
    const uint8_t *pdu,
    unsigned pdu_len)
{
    (void)dest;
    (void)npdu_data;
    (void)pdu;
    (void)pdu_len;

    return 0;
}

void bvlc_init(void)
{
}
