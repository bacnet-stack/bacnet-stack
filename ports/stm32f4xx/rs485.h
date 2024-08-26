/**************************************************************************
 *
 * Copyright (C) 2004 Steve Karg
 *
 * SPDX-License-Identifier: GPL-2.0-or-later WITH GCC-exception-2.0
 *
 *********************************************************************/
#ifndef RS485_H
#define RS485_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

void rs485_rts_enable(bool enable);
bool rs485_rts_enabled(void);
bool rs485_byte_available(uint8_t *data_register);
bool rs485_receive_error(void);

void rs485_bytes_send(uint8_t *buffer, uint16_t nbytes);

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
