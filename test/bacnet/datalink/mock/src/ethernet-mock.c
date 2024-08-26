/*
 * Copyright (c) 2020 Legrand North America, LLC.
 *
 * SPDX-License-Identifier: MIT
 */

#include <zephyr/ztest.h>
#include <stdio.h>
#include <stdlib.h>
#include "bacnet/datalink/ethernet.h"

bool ethernet_valid(void)
{
    return ztest_get_return_value();
}

void ethernet_cleanup(void)
{
}

bool ethernet_init(char *interface_name)
{
    ztest_check_expected_value(interface_name);
    return ztest_get_return_value();
}

int ethernet_send_pdu(
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

uint16_t ethernet_receive(
    BACNET_ADDRESS *src, uint8_t *pdu, uint16_t max_pdu, unsigned timeout)
{
    ztest_check_expected_value(src);
    ztest_check_expected_value(timeout);
    ztest_copy_return_data(pdu, max_pdu);
    return ztest_get_return_value();
}

void ethernet_set_my_address(BACNET_ADDRESS *my_address)
{
    ztest_check_expected_data(my_address, sizeof(BACNET_ADDRESS));
}

void ethernet_get_my_address(BACNET_ADDRESS *my_address)
{
    ztest_copy_return_data(my_address, sizeof(BACNET_ADDRESS));
}

void ethernet_get_broadcast_address(BACNET_ADDRESS *dest)
{
    ztest_copy_return_data(dest, sizeof(BACNET_ADDRESS));
}

void ethernet_debug_address(const char *info, BACNET_ADDRESS *dest)
{
    ztest_check_expected_value(info);
    ztest_check_expected_value(dest);
}

int ethernet_send(uint8_t *mtu, int mtu_len)
{
    ztest_check_expected_data(mtu, mtu_len);
    return ztest_get_return_value();
}
