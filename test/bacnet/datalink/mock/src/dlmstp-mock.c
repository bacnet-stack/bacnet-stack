/*
 * Copyright (c) 2020 Legrand North America, LLC.
 *
 * SPDX-License-Identifier: MIT
 */

#include <ztest.h>
#include <stdio.h>
#include <stdlib.h>
#include "bacnet/datalink/dlmstp.h"

bool dlmstp_init(char *ifname)
{
    ztest_check_expected_value(ifname);
    return ztest_get_return_value();
}

void dlmstp_reset(void)
{    
}

void dlmstp_cleanup(void)
{
}

int dlmstp_send_pdu(BACNET_ADDRESS *dest, BACNET_NPDU_DATA *npdu_data,
                    uint8_t * pdu, unsigned pdu_len)
{
    ztest_check_expected_value(dest);
    ztest_check_expected_value(npdu_data);
    ztest_check_expected_data(pdu, pdu_len);
    return ztest_get_return_value();
}

uint16_t dlmstp_receive(BACNET_ADDRESS *src, uint8_t *pdu, uint16_t max_pdu,
                        unsigned timeout)
{
    ztest_check_expected_value(src);
    ztest_check_expected_value(timeout);
    ztest_copy_return_data(pdu, max_pdu);
    return ztest_get_return_value();
}

void dlmstp_set_max_info_frames(uint8_t max_info_frames)
{
    ztest_check_expected_value(max_info_frames);
}

uint8_t dlmstp_max_info_frames(void)
{
    return ztest_get_return_value();
}

void dlmstp_set_max_master(uint8_t max_master)
{
    ztest_check_expected_value(max_master);
}

uint8_t dlmstp_max_master(void)
{
    return ztest_get_return_value();
}

void dlmstp_set_mac_address(uint8_t my_address)
{
    ztest_check_expected_value(my_address);
}

uint8_t dlmstp_mac_address(void)
{
    return ztest_get_return_value();
}

void dlmstp_get_my_address(BACNET_ADDRESS *my_address)
{
    ztest_copy_return_data(my_address, sizeof(BACNET_ADDRESS));
}

void dlmstp_get_broadcast_address(BACNET_ADDRESS *dest)
{
    ztest_copy_return_data(dest, sizeof(BACNET_ADDRESS));
}

void dlmstp_set_baud_rate(uint32_t baud)
{
    ztest_check_expected_value(baud);
}

uint32_t dlmstp_baud_rate(void)
{
    return ztest_get_return_value();
}

void dlmstp_fill_bacnet_address(BACNET_ADDRESS *src, uint8_t mstp_address)
{
    ztest_check_expected_value(src);
    ztest_check_expected_value(mstp_address);
}

bool dlmstp_sole_master(void)
{
    return ztest_get_return_value();
}

bool dlmstp_send_pdu_queue_empty(void)
{
    return ztest_get_return_value();
}

bool dlmstp_send_pdu_queue_full(void)
{
    return ztest_get_return_value();
}

uint8_t dlmstp_max_info_frames_limit(void)
{
    return ztest_get_return_value();
}

uint8_t dlmstp_max_master_limit(void)
{
    return ztest_get_return_value();
}

void dlmstp_set_frame_rx_complete_callback(
    dlmstp_hook_frame_rx_complete_cb cb_func)
{
    ztest_check_expected_value(cb_func);
}

void dlmstp_set_frame_rx_start_callback(
        dlmstp_hook_frame_rx_start_cb cb_func)
{
    ztest_check_expected_value(cb_func);
}

void dlmstp_reset_statistics(void)
{
}

void dlmstp_fill_statistics(struct dlmstp_statistics *statistics)
{
    ztest_check_expected_value(statistics);
}
