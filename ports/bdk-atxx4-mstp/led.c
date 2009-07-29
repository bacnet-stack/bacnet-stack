/**************************************************************************
*
* Copyright (C) 2009 Steve Karg <skarg@users.sourceforge.net>
*
* Permission is hereby granted, free of charge, to any person obtaining
* a copy of this software and associated documentation files (the
* "Software"), to deal in the Software without restriction, including
* without limitation the rights to use, copy, modify, merge, publish,
* distribute, sublicense, and/or sell copies of the Software, and to
* permit persons to whom the Software is furnished to do so, subject to
* the following conditions:
*
* The above copyright notice and this permission notice shall be included
* in all copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*********************************************************************/
#include <stdint.h>
#include "hardware.h"
#include "timer.h"
#include "led.h"

static uint32_t Off_Delay_Milliseconds_1;
static uint32_t Off_Delay_Milliseconds_2;
static uint32_t Off_Delay_Milliseconds_3;
static uint32_t Off_Delay_Milliseconds_4;

/*************************************************************************
* Description: Turn on an LED
* Returns: none
* Notes: none
*************************************************************************/
void led_on(
    uint8_t index)
{
    switch (index) {
        case LED_1:
            BIT_SET(PORTD, PD7);
            break;
        case LED_2:
            BIT_SET(PORTD, PD6);
            break;
        case LED_3:
            BIT_SET(PORTC, PC7);
            break;
        case LED_4:
            BIT_SET(PORTC, PC6);
            break;
        default:
            break;
    }
}

/*************************************************************************
* Description: Turn off an LED
* Returns: none
* Notes: none
*************************************************************************/
void led_off(
    uint8_t index)
{
    switch (index) {
        case LED_1:
            BIT_CLEAR(PORTD, PD7);
            break;
        case LED_2:
            BIT_CLEAR(PORTD, PD6);
            break;
        case LED_3:
            BIT_CLEAR(PORTC, PC7);
            break;
        case LED_4:
            BIT_CLEAR(PORTC, PC6);
            break;
        default:
            break;
    }
}

/*************************************************************************
* Description: Get the state of the LED
* Returns: true if on, false if off.
* Notes: none
*************************************************************************/
bool led_state(
    uint8_t index)
{
    switch (index) {
        case LED_1:
            return (BIT_CHECK(PIND, PD7));
            break;
        case LED_2:
            return (BIT_CHECK(PIND, PD6));
            break;
        case LED_3:
            return (BIT_CHECK(PINC, PC7));
            break;
        case LED_4:
            return (BIT_CHECK(PINC, PC6));
            break;
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
void led_toggle(
    uint8_t index)
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
void led_off_delay(
    uint8_t index,
    uint32_t delay_ms)
{
    switch (index) {
        case LED_1:
            Off_Delay_Milliseconds_1 = delay_ms;
            timer_reset(TIMER_LED_1);
            break;
        case LED_2:
            Off_Delay_Milliseconds_2 = delay_ms;
            timer_reset(TIMER_LED_2);
            break;
        case LED_3:
            Off_Delay_Milliseconds_3 = delay_ms;
            timer_reset(TIMER_LED_3);
            break;
        case LED_4:
            Off_Delay_Milliseconds_4 = delay_ms;
            timer_reset(TIMER_LED_4);
            break;
        default:
            break;
    }
}

/*************************************************************************
* Description: Task for blinking LED
* Returns: none
* Notes: none
*************************************************************************/
void led_task(
    void)
{
    if (Off_Delay_Milliseconds_1) {
        if (timer_elapsed_milliseconds(TIMER_LED_1, Off_Delay_Milliseconds_1)) {
            Off_Delay_Milliseconds_1 = 0;
            led_off(LED_1);
        }
    }
    if (Off_Delay_Milliseconds_2) {
        if (timer_elapsed_milliseconds(TIMER_LED_2, Off_Delay_Milliseconds_2)) {
            Off_Delay_Milliseconds_2 = 0;
            led_off(LED_2);
        }
    }
    if (Off_Delay_Milliseconds_3) {
        if (timer_elapsed_milliseconds(TIMER_LED_3, Off_Delay_Milliseconds_3)) {
            Off_Delay_Milliseconds_3 = 0;
            led_off(LED_3);
        }
    }
    if (Off_Delay_Milliseconds_4) {
        if (timer_elapsed_milliseconds(TIMER_LED_4, Off_Delay_Milliseconds_4)) {
            Off_Delay_Milliseconds_4 = 0;
            led_off(LED_4);
        }
    }
}

/*************************************************************************
* Description: Initialize the LED hardware
* Returns: none
* Notes: none
*************************************************************************/
void led_init(
    void)
{
    /* configure the port pins as outputs */
    BIT_SET(DDRC, DDC7);
    BIT_SET(DDRC, DDC6);
    BIT_SET(DDRD, DDD7);
    BIT_SET(DDRD, DDD6);
    led_off(LED_1);
    led_off(LED_2);
    led_off(LED_3);
    led_off(LED_4);
}
