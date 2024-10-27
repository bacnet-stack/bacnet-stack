/*
 * Copyright (c) 2020 Legrand North America, LLC.
 *
 * SPDX-License-Identifier: MIT
 */

#include <ztest.h>
#include <stdint.h> /* for standard integer types uint8_t etc. */
#include <stdbool.h> /* for the standard bool type. */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "bacnet/datalink/bsc/bsc-datalink.h"

bool bsc_init(char *ifname)
{
    ztest_check_expected_value(ifname);
    return ztest_get_return_value();
}

void bsc_cleanup(void)
{
}

void bsc_get_my_address(BACNET_ADDRESS *my_address)
{
    ztest_copy_return_data(my_address, sizeof(BACNET_ADDRESS));
}

int bsc_send_pdu(
    BACNET_ADDRESS *dest,
    BACNET_NPDU_DATA *npdu_data,
    uint8_t *pdu,
    unsigned pdu_len)
{
    ztest_check_expected_value(dest);
    ztest_check_expected_value(npdu_data);
    ztest_check_expected_data(pdu, pdu_len);
    return ztest_get_return_value();
}

uint16_t bsc_receive(
    BACNET_ADDRESS *src, uint8_t *pdu, uint16_t max_pdu, unsigned timeout)
{
    ztest_check_expected_value(src);
    ztest_check_expected_value(timeout);
    ztest_copy_return_data(pdu, max_pdu);
    return ztest_get_return_value();
}

void bsc_get_broadcast_address(BACNET_ADDRESS *dest)
{
    ztest_copy_return_data(dest, sizeof(BACNET_ADDRESS));
}
