/**************************************************************************
 *
 * Copyright (C) 2025 Testimony Adams <adamstestimony@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later WITH GCC-exception-2.0
 *
 *********************************************************************/

#ifndef RS485_H
#define RS485_H

#include <stdint.h>
#include <stdbool.h>
#include "pico/stdlib.h"
#include "hardware/uart.h"
#include "hardware/gpio.h"
#include "hardware/regs/uart.h" 

#define RS485_UART_ID       uart1
#define RS485_BAUD_RATE     38400
#define RS485_TX_PIN        8
#define RS485_RX_PIN        9
#define RS485_DE_PIN        10
#define DATA_BIT            8
#define STOP_BIT            1

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

void rs485_rts_enable(bool enable);
bool rs485_rts_enabled(void);
bool rs485_byte_available(uint8_t *data_register);
bool rs485_receive_error(void);

void rs485_bytes_send(const uint8_t *buffer, uint16_t nbytes);

uint32_t rs485_baud_rate(void);
bool rs485_baud_rate_set(uint32_t baud);

uint32_t rs485_silence_milliseconds(void);
void rs485_silence_reset(void);

uint32_t rs485_bytes_transmitted(void);
uint32_t rs485_bytes_received(void);

void rs485_init(void);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif
