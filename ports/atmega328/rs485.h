/**
 * @brief The module handles API for the RS-485 port
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date 2007
 * @copyright SPDX-License-Identifier: MIT
 */
#ifndef RS485_H
#define RS485_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

unsigned long RS485_Timer_Silence(void);

void RS485_Timer_Silence_Reset(void);

void RS485_Initialize(void);

void RS485_Transmitter_Enable(bool enable);

void RS485_Send_Data(
    const uint8_t *buffer, /* data to send */
    uint16_t nbytes); /* number of bytes of data */

bool RS485_ReceiveError(void);
bool RS485_DataAvailable(uint8_t *data);

void RS485_Turnaround_Delay(void);
uint32_t RS485_Get_Baud_Rate(void);
bool RS485_Set_Baud_Rate(uint32_t baud);
uint32_t RS485_Baud_Rate_From_Kilo(uint8_t baud_k);

void RS485_LED_Timers(void);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif
