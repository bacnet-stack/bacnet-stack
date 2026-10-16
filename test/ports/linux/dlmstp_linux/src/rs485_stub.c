/* SPDX-License-Identifier: MIT */
/**
 * @file
 * @brief Stub RS-485 layer so ports/linux/dlmstp.c can be unit tested
 *  without real serial hardware.
 */
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include "bacnet/datalink/mstp.h"
#include "rs485.h"

static uint32_t Baud_Rate = 38400;

void RS485_Set_Interface(const char *ifname)
{
    (void)ifname;
}

const char *RS485_Interface(void)
{
    return "stub";
}

void RS485_Initialize(void)
{
    /* nothing to do - no hardware in the unit test */
}

void RS485_Send_Frame(
    struct mstp_port_struct_t *mstp_port,
    const uint8_t *buffer,
    uint16_t nbytes)
{
    (void)mstp_port;
    (void)buffer;
    (void)nbytes;
}

void RS485_Check_UART_Data(struct mstp_port_struct_t *mstp_port)
{
    (void)mstp_port;
    /* no data ever arrives in the unit test */
}

uint32_t RS485_Get_Port_Baud_Rate(struct mstp_port_struct_t *mstp_port)
{
    (void)mstp_port;
    return Baud_Rate;
}

uint32_t RS485_Get_Baud_Rate(void)
{
    return Baud_Rate;
}

bool RS485_Set_Baud_Rate(uint32_t baud)
{
    Baud_Rate = baud;
    return true;
}

bool RS485_Get_Config(struct serial_rs485 *config)
{
    (void)config;
    return false;
}

bool RS485_Set_Config(const struct serial_rs485 *config)
{
    (void)config;
    return false;
}

void RS485_Cleanup(void)
{
    /* nothing to do */
}

void RS485_Print_Ports(void)
{
    /* nothing to do */
}
