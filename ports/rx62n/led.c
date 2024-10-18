/**************************************************************************
 *
 * Copyright (C) 2011 Steve Karg <skarg@users.sourceforge.net>
 *
 * SPDX-License-Identifier: MIT
 *
 *********************************************************************/
#include <stdint.h>
#include "hardware.h"
#include "bacnet/basic/sys/mstimer.h"
#include "led.h"

static struct mstimer Off_Delay_Timer[MAX_LEDS];
static bool LED_Status[MAX_LEDS];

#define LED_OFF (0)

/*************************************************************************
 * Description: Turn on an LED
 * Returns: none
 * Notes: none
 *************************************************************************/
void led_on(uint8_t index)
{
    switch (index) {
        case 4:
            R_IO_PORT_Write(LED4, LED_ON);
            break;
        case 5:
            R_IO_PORT_Write(LED5, LED_ON);
            break;
        case 6:
            R_IO_PORT_Write(LED6, LED_ON);
            break;
        case 7:
            R_IO_PORT_Write(LED7, LED_ON);
            break;
        case 8:
            R_IO_PORT_Write(LED8, LED_ON);
            break;
        case 9:
            R_IO_PORT_Write(LED9, LED_ON);
            break;
        case 10:
            R_IO_PORT_Write(LED10, LED_ON);
            break;
        case 11:
            R_IO_PORT_Write(LED11, LED_ON);
            break;
        case 12:
            R_IO_PORT_Write(LED12, LED_ON);
            break;
        case 13:
            R_IO_PORT_Write(LED13, LED_ON);
            break;
        case 14:
            R_IO_PORT_Write(LED14, LED_ON);
            break;
        case 15:
            R_IO_PORT_Write(LED15, LED_ON);
            break;
        default:
            break;
    }
    if (index < MAX_LEDS) {
        LED_Status[index] = LED_ON;
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
        case 4:
            R_IO_PORT_Write(LED4, LED_OFF);
            break;
        case 5:
            R_IO_PORT_Write(LED5, LED_OFF);
            break;
        case 6:
            R_IO_PORT_Write(LED6, LED_OFF);
            break;
        case 7:
            R_IO_PORT_Write(LED7, LED_OFF);
            break;
        case 8:
            R_IO_PORT_Write(LED8, LED_OFF);
            break;
        case 9:
            R_IO_PORT_Write(LED9, LED_OFF);
            break;
        case 10:
            R_IO_PORT_Write(LED10, LED_OFF);
            break;
        case 11:
            R_IO_PORT_Write(LED11, LED_OFF);
            break;
        case 12:
            R_IO_PORT_Write(LED12, LED_OFF);
            break;
        case 13:
            R_IO_PORT_Write(LED13, LED_OFF);
            break;
        case 14:
            R_IO_PORT_Write(LED14, LED_OFF);
            break;
        case 15:
            R_IO_PORT_Write(LED15, LED_OFF);
            break;
        default:
            break;
    }
    if (index < MAX_LEDS) {
        LED_Status[index] = LED_OFF;
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
    bool state = false;

    if (index < MAX_LEDS) {
        state = LED_Status[index];
    }

    return state;
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
        if (mstimer_interval(&Off_Delay_Timer[i]) > 0) {
            if (mstimer_expired(&Off_Delay_Timer[i])) {
                mstimer_set(&Off_Delay_Timer[i], 0);
                led_off(i);
            }
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
    unsigned i = 0;

    for (i = 0; i < MAX_LEDS; i++) {
        led_on_interval(i, 500);
    }
}
