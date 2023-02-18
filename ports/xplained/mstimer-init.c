/*
 * @file
 *
 * @brief 1ms timer configuration
 *
 * Copyright (C) 2009 Steve Karg <skarg@users.sourceforge.net>
 *
 * Created: 10/24/2013 8:58:56 PM
 * Author: Steve
 *
 * @page License
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
 */
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "tc.h"
#include "bacnet/basic/sys/mstimer.h"

/* define which timer for the counter we are using */
#define MS_TIMER_COUNTER TCE0
/* define which timer for the callback we are using */
#define MS_TIMER_CALLBACK TCE1

/* counter for the base timer */
static volatile uint32_t Millisecond_Counter;

/**
 * Handles an interrupt from a hardware millisecond timer
 */
static void my_counter_handler(void)
{
    Millisecond_Counter++;
}

/**
 * Handles an interrupt from a hardware millisecond timer
 */
static void my_callback_handler(void)
{
    /* callback might go too long; prevent re-entrency by disable IRQ source */
    tc_set_overflow_interrupt_level(&MS_TIMER_CALLBACK, TC_INT_LVL_OFF);
    mstimer_callback_handler();
    tc_set_overflow_interrupt_level(&MS_TIMER_CALLBACK, TC_INT_LVL_LO);
}

/**
 * Returns the continuous milliseconds count, which rolls over
 *
 * @return the current milliseconds count
 */
unsigned long mstimer_now(void)
{
    uint32_t timer_value; /* return value */

    tc_set_overflow_interrupt_level(&MS_TIMER_COUNTER, TC_INT_LVL_OFF);
    timer_value = Millisecond_Counter;
    tc_set_overflow_interrupt_level(&MS_TIMER_COUNTER, TC_INT_LVL_LO);

    return timer_value;
}

/**
 * Timer setup for 1 millisecond timer
 */
void mstimer_init(void)
{
    unsigned long period;

    tc_enable(&MS_TIMER_COUNTER);
    tc_set_overflow_interrupt_callback(&MS_TIMER_COUNTER,
        mstimer_counter_handler);
    tc_set_wgm(&MS_TIMER_COUNTER, TC_WG_NORMAL);
    tc_write_count(&MS_TIMER_COUNTER, 1);
    period = sysclk_get_peripheral_bus_hz(&MS_TIMER_COUNTER);
    period /= 1000;
    tc_write_period(&MS_TIMER_COUNTER, period);
    tc_set_overflow_interrupt_level(&MS_TIMER_COUNTER, TC_INT_LVL_LO);
    tc_write_clock_source(&MS_TIMER_COUNTER, TC_CLKSEL_DIV1_gc);

    tc_enable(&MS_TIMER_CALLBACK);
    tc_set_overflow_interrupt_callback(&MS_TIMER_CALLBACK,
        my_callback_handler);
    tc_set_wgm(&MS_TIMER_CALLBACK, TC_WG_NORMAL);
    tc_write_count(&MS_TIMER_CALLBACK, 1);
    period = sysclk_get_peripheral_bus_hz(&MS_TIMER_CALLBACK);
    period /= 1000;
    tc_write_period(&MS_TIMER_CALLBACK, period);
    tc_set_overflow_interrupt_level(&MS_TIMER_CALLBACK, TC_INT_LVL_LO);
    tc_write_clock_source(&MS_TIMER_CALLBACK, TC_CLKSEL_DIV1_gc);
}
