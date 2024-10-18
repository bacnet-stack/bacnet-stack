/**************************************************************************
 *
 * Copyright (C) 2009 Steve Karg <skarg@users.sourceforge.net>
 *
 * SPDX-License-Identifier: MIT
 *
 *********************************************************************/
#ifndef LED_H
#define LED_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

void led_on(uint8_t index);
void led_on_interval(uint8_t index, uint16_t interval_ms);
void led_off(uint8_t index);
void led_off_delay(uint8_t index, uint32_t delay_ms);
void led_toggle(uint8_t index);
bool led_state(uint8_t index);
void led_task(void);
void led_init(void);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif
