/*
 * Copyright (c) 2020 Legrand North America, LLC.
 *
 * SPDX-License-Identifier: MIT
 */

#include <zephyr/ztest.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h> /* for standard integer types uint8_t etc. */
#include <stdbool.h> /* for the standard bool type. */
#include "bacnet/datalink/bip6.h"

bool bip6_init(char *ifname)
{
    ztest_check_expected_value(ifname);
    return ztest_get_return_value();
}

void bip6_cleanup(void)
{
}

bool bip6_get_broadcast_addr(BACNET_IP6_ADDRESS *addr)
{
    ztest_copy_return_data(addr, sizeof(BACNET_IP6_ADDRESS));
    return ztest_get_return_value();
}

void bip6_get_my_address(BACNET_ADDRESS *my_address)
{
    ztest_copy_return_data(my_address, sizeof(BACNET_ADDRESS));
}

int bip6_send_pdu(
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

uint16_t bip6_receive(
    BACNET_ADDRESS *src, uint8_t *pdu, uint16_t max_pdu, unsigned timeout)
{
    ztest_check_expected_value(src);
    ztest_check_expected_value(timeout);
    ztest_copy_return_data(pdu, max_pdu);
    return ztest_get_return_value();
}

void bip6_set_interface(char *ifname)
{
    ztest_check_expected_value(ifname);
}

bool bip6_address_match_self(const BACNET_IP6_ADDRESS *addr)
{
    ztest_check_expected_value(addr);
    return ztest_get_return_value();
}

bool bip6_set_addr(const BACNET_IP6_ADDRESS *addr)
{
    ztest_check_expected_data(addr, sizeof(BACNET_IP6_ADDRESS));
    return ztest_get_return_value();
}

bool bip6_get_addr(BACNET_IP6_ADDRESS *addr)
{
    ztest_copy_return_data(addr, sizeof(BACNET_IP6_ADDRESS));
    return ztest_get_return_value();
}

void bip6_set_port(uint16_t port)
{
    ztest_check_expected_value(port);
}

uint16_t bip6_get_port(void)
{
    return ztest_get_return_value();
}

void bip6_get_broadcast_address(BACNET_ADDRESS *my_address)
{
    ztest_copy_return_data(my_address, sizeof(BACNET_ADDRESS));
}

bool bip6_set_broadcast_addr(const BACNET_IP6_ADDRESS *addr)
{
    return ztest_get_return_value();
}

int bip6_send_mpdu(const BACNET_IP6_ADDRESS *dest, const uint8_t *mtu, uint16_t mtu_len)
{
    ztest_check_expected_value(dest);
    ztest_check_expected_data(mtu, mtu_len);
    return ztest_get_return_value();
}

bool bip6_send_pdu_queue_empty(void)
{
    return ztest_get_return_value();
}

void bip6_receive_callback(void)
{
}

void bip6_debug_enable(void)
{
}
