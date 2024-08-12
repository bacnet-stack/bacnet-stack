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

#define LED_LD1 0
#define LED_LD2 1
#define LED_LD3 2
#define LED_RS485 3
#define LED_MAX 4

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

void led_on(unsigned index);
void led_off(unsigned index);
bool led_state(unsigned index);
void led_toggle(unsigned index);

void led_on_interval(unsigned index, uint16_t interval_ms);
void led_off_delay(unsigned index, uint32_t delay_ms);

void led_task(void);
void led_init(void);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif
