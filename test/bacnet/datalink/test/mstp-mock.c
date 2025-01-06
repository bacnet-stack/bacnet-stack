/**
 * @file
 * @brief Mock for BACnet MS/TP API
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date November 2024
 * @copyright SPDX-License-Identifier: MIT
 */
#include <zephyr/ztest.h>
#include <stdio.h>
#include <stdlib.h>
#include "bacnet/datalink/mstp.h"

void MSTP_Init(struct mstp_port_struct_t *mstp_port)
{
    ztest_check_expected_value(mstp_port);
}

void MSTP_Receive_Frame_FSM(struct mstp_port_struct_t *mstp_port)
{
    ztest_check_expected_value(mstp_port);
}

bool MSTP_Master_Node_FSM(struct mstp_port_struct_t *mstp_port)
{
    ztest_check_expected_value(mstp_port);
    return ztest_get_return_value();
}

void MSTP_Slave_Node_FSM(struct mstp_port_struct_t *mstp_port)
{
    ztest_check_expected_value(mstp_port);
}

bool MSTP_Line_Active(const struct mstp_port_struct_t *mstp_port)
{
    ztest_check_expected_value(mstp_port);
    return ztest_get_return_value();
}

uint16_t MSTP_Create_Frame(
    uint8_t *buffer,
    uint16_t buffer_len,
    uint8_t frame_type,
    uint8_t destination,
    uint8_t source,
    const uint8_t *data,
    uint16_t data_len)
{
    ztest_check_expected_value(buffer);
    ztest_check_expected_value(buffer_len);
    ztest_check_expected_value(frame_type);
    ztest_check_expected_value(destination);
    ztest_check_expected_value(source);
    ztest_check_expected_data(data, data_len);
    return ztest_get_return_value();
}

void MSTP_Create_And_Send_Frame(
    struct mstp_port_struct_t *mstp_port,
    uint8_t frame_type,
    uint8_t destination,
    uint8_t source,
    const uint8_t *data,
    uint16_t data_len)
{
    ztest_check_expected_value(mstp_port);
    ztest_check_expected_value(frame_type);
    ztest_check_expected_value(destination);
    ztest_check_expected_value(source);
    ztest_check_expected_data(data, data_len);
}

void MSTP_Fill_BACnet_Address(BACNET_ADDRESS *src, uint8_t mstp_address)
{
    ztest_check_expected_value(src);
    ztest_check_expected_value(mstp_address);
}

void MSTP_Zero_Config_UUID_Init(struct mstp_port_struct_t *mstp_port)
{
    ztest_check_expected_value(mstp_port);
}

unsigned MSTP_Zero_Config_Station_Increment(unsigned station)
{
    ztest_check_expected_value(station);
    return ztest_get_return_value();
}

void MSTP_Zero_Config_FSM(struct mstp_port_struct_t *mstp_port)
{
    ztest_check_expected_value(mstp_port);
}
