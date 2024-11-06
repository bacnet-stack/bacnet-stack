/**************************************************************************
 *
 * Copyright (C) 2009 Steve Karg <skarg@users.sourceforge.net>
 *
 * SPDX-License-Identifier: MIT
 *
 *********************************************************************/
#include <stdint.h>
#include "hardware.h"
#include "bacnet/basic/sys/mstimer.h"
#include "led.h"

#ifndef BDK_VERSION
#define BDK_VERSION 4
#endif

static struct mstimer Off_Delay_Timer[MAX_LEDS];

/*************************************************************************
 * Description: Turn on an LED
 * Returns: none
 * Notes: none
 *************************************************************************/
void led_on(uint8_t index)
{
    switch (index) {
        case 0:
            BIT_SET(PORTD, PD7);
            break;
        case 1:
            BIT_SET(PORTD, PD6);
            break;
        case 2:
#if (BDK_VERSION == 4)
            BIT_SET(PORTB, PB0);
#else
            BIT_SET(PORTC, PC7);
#endif
            break;
        case 3:
#if (BDK_VERSION == 4)
            BIT_SET(PORTB, PB4);
#else
            BIT_SET(PORTC, PC6);
#endif
            break;
        default:
            break;
    }
    if (index < MAX_LEDS) {
        mstimer_set(&Off_Delay_Timer[index], 0);
    }
}

/*************************************************************************
 * Description: Turn off an LED
 * Returns: none
 * Notes: none
 *************************************************************************/
void led_off(uint8_t index)
{
    switch (index) {
        case 0:
            BIT_CLEAR(PORTD, PD7);
            break;
        case 1:
            BIT_CLEAR(PORTD, PD6);
            break;
        case 2:
#if (BDK_VERSION == 4)
            BIT_CLEAR(PORTB, PB0);
#else
            BIT_CLEAR(PORTC, PC7);
#endif
            break;
        case 3:
#if (BDK_VERSION == 4)
            BIT_CLEAR(PORTB, PB4);
#else
            BIT_CLEAR(PORTC, PC6);
#endif
            break;
        default:
            break;
    }
    if (index < MAX_LEDS) {
        mstimer_set(&Off_Delay_Timer[index], 0);
    }
}

/*************************************************************************
 * Description: Get the state of the LED
 * Returns: true if on, false if off.
 * Notes: none
 *************************************************************************/
bool led_state(uint8_t index)
{
    switch (index) {
        case 0:
            return (BIT_CHECK(PIND, PIND7));
        case 1:
            return (BIT_CHECK(PIND, PIND6));
        case 2:
#if (BDK_VERSION == 4)
            return (BIT_CHECK(PINB, PINC0));
#else
            return (BIT_CHECK(PINC, PINC7));
#endif
        case 3:
#if (BDK_VERSION == 4)
            return (BIT_CHECK(PINB, PINC4));
#else
            return (BIT_CHECK(PINC, PINC6));
#endif
        default:
            break;
    }

    return false;
}

/*************************************************************************
 * Description: Toggle the state of the setup LED
 * Returns: none
 * Notes: none
 *************************************************************************/
void led_toggle(uint8_t index)
{
    if (led_state(index)) {
        led_off(index);
    } else {
        led_on(index);
    }
}

/*************************************************************************
 * Description: Delay before going off to give minimum brightness.
 * Returns: none
 * Notes: none
 *************************************************************************/
void led_off_delay(uint8_t index, uint32_t delay_ms)
{
    if (index < MAX_LEDS) {
        mstimer_set(&Off_Delay_Timer[index], delay_ms);
    }
}

/*************************************************************************
 * Description: Turn on, and delay before going off.
 * Returns: none
 * Notes: none
 *************************************************************************/
void led_on_interval(uint8_t index, uint16_t interval_ms)
{
    if (index < MAX_LEDS) {
        led_on(index);
        mstimer_set(&Off_Delay_Timer[index], interval_ms);
    }
}

/*************************************************************************
 * Description: Task for blinking LED
 * Returns: none
 * Notes: none
 *************************************************************************/
void led_task(void)
{
    uint8_t i; /* loop counter */

    for (i = 0; i < MAX_LEDS; i++) {
        if (mstimer_expired(&Off_Delay_Timer[i])) {
            mstimer_set(&Off_Delay_Timer[i], 0);
            led_off(i);
        }
    }
}

/*************************************************************************
 * Description: Initialize the LED hardware
 * Returns: none
 * Notes: none
 *************************************************************************/
void led_init(void)
{
    uint8_t i; /* loop counter */

    /* configure the port pins as outputs */
    BIT_SET(DDRD, DDD7);
    BIT_SET(DDRD, DDD6);
#if (BDK_VERSION == 4)
    BIT_SET(DDRB, DDB0);
    BIT_SET(DDRB, DDB4);
#else
    BIT_SET(DDRC, DDC7);
    BIT_SET(DDRC, DDC6);
#endif
    for (i = 0; i < MAX_LEDS; i++) {
        led_on_interval(i, 500);
    }
}
