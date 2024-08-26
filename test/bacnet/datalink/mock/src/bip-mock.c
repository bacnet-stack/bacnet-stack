/*
 * Copyright (c) 2020 Legrand North America, LLC.
 *
 * SPDX-License-Identifier: MIT
 */

#include <zephyr/ztest.h>
#include <stdint.h> /* for standard integer types uint8_t etc. */
#include <stdbool.h> /* for the standard bool type. */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "bacnet/datalink/bip.h"

bool bip_init(char *ifname)
{
    ztest_check_expected_value(ifname);
    return ztest_get_return_value();
}

void bip_set_interface(char *ifname)
{
    ztest_check_expected_value(ifname);
}

void bip_cleanup(void)
{
}

bool bip_valid(void)
{
    return ztest_get_return_value();
}

bool bip_get_broadcast_addr(BACNET_IP_ADDRESS *addr)
{
    ztest_copy_return_data(addr, sizeof(BACNET_IP_ADDRESS));
    return ztest_get_return_value();
}

void bip_get_my_address(BACNET_ADDRESS *my_address)
{
    ztest_copy_return_data(my_address, sizeof(BACNET_ADDRESS));
}

int bip_send_pdu(
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

int bip_send_mpdu(BACNET_IP_ADDRESS *dest, uint8_t *mtu, uint16_t mtu_len)
{
    ztest_check_expected_value(dest);
    ztest_check_expected_data(mtu, mtu_len);
    return ztest_get_return_value();
}

uint16_t bip_receive(
    BACNET_ADDRESS *src, uint8_t *pdu, uint16_t max_pdu, unsigned timeout)
{
    ztest_check_expected_value(src);
    ztest_check_expected_value(timeout);
    ztest_copy_return_data(pdu, max_pdu);
    return ztest_get_return_value();
}

void bip_set_port(uint16_t port)
{
    ztest_check_expected_value(port);
}

bool bip_port_changed(void)
{
    return ztest_get_return_value();
}

uint16_t bip_get_port(void)
{
    return ztest_get_return_value();
}

bool bip_set_addr(BACNET_IP_ADDRESS *addr)
{
    ztest_check_expected_data(addr, sizeof(BACNET_IP_ADDRESS));
    return ztest_get_return_value();
}

bool bip_get_addr(BACNET_IP_ADDRESS *addr)
{
    ztest_copy_return_data(addr, sizeof(BACNET_IP_ADDRESS));
    return ztest_get_return_value();
}

bool bip_get_addr_by_name(const char *host_name, BACNET_IP_ADDRESS *addr)
{
    ztest_check_expected_value(host_name);
    ztest_check_expected_value(addr);
    return ztest_get_return_value();
}

void bip_get_broadcast_address(BACNET_ADDRESS *dest)
{
    ztest_copy_return_data(dest, sizeof(BACNET_ADDRESS));
}

bool bip_set_broadcast_addr(BACNET_IP_ADDRESS *addr)
{
    return ztest_get_return_value();
}

bool bip_set_subnet_prefix(uint8_t prefix)
{
    return false;
}

uint8_t bip_get_subnet_prefix(void)
{
    return ztest_get_return_value();
}

void bip_debug_enable(void)
{
}

int bip_get_socket(void)
{
    return ztest_get_return_value();
}

int bip_get_broadcast_socket(void)
{
    return ztest_get_return_value();
}
