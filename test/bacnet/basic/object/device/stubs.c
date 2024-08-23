/**************************************************************************
 *
 * Copyright (C) 2006 Steve Karg <skarg@users.sourceforge.net>
 *
 * SPDX-License-Identifier: MIT
 *
 *********************************************************************/

/* Binary Input Objects customize for your use */

#include <stdbool.h>
#include <stdint.h>
#include "bacnet/datetime.h"
#include "bacnet/bacdef.h"
#include "bacnet/npdu.h"

void datetime_init(void)
{
}

bool datetime_local(
    BACNET_DATE *bdate,
    BACNET_TIME *btime,
    int16_t *utc_offset_minutes,
    bool *dst_active)
{
    (void)bdate;
    (void)btime;
    (void)utc_offset_minutes;
    (void)dst_active;

    return true;
}

void bip_get_my_address(BACNET_ADDRESS *my_address)
{
    (void)my_address;
}

int bip_send_pdu(
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
