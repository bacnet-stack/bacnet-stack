/**
 * @file
 * @brief Mock for RS485 driver API
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date November 2024
 * @copyright SPDX-License-Identifier: MIT
 */
#include <zephyr/ztest.h>
#include <stdio.h>
#include <stdlib.h>
#include "bacnet/datalink/dlmstp.h"

void MSTP_RS485_Init(void)
{
}

void MSTP_RS485_Send(const uint8_t *payload, uint16_t payload_len)
{
    ztest_check_expected_data(payload, payload_len);
}

bool MSTP_RS485_Read(uint8_t *buf)
{
    ztest_check_expected_value(buf);
    return ztest_get_return_value();
}

bool MSTP_RS485_Transmitting(void)
{
    return ztest_get_return_value();
}

uint32_t MSTP_RS485_Baud_Rate(void)
{
    return ztest_get_return_value();
}

bool MSTP_RS485_Baud_Rate_Set(uint32_t baud)
{
    ztest_check_expected_value(baud);
    return ztest_get_return_value();
}

uint32_t MSTP_RS485_Silence_Milliseconds(void)
{
    return ztest_get_return_value();
}

void MSTP_RS485_Silence_Reset(void)
{
}
