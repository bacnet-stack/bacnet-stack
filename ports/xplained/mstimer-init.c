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

/* define which timer counter we are using */
#define MY_TIMER TCE0

/* counter for the base timer */
static volatile uint32_t Millisecond_Counter;
/* callback data head of list */
static volatile struct mstimer_callback_data_t *Callback_Head;

/**
 * Handles the interrupt from the timer
 */
static void my_callback(void)
{
    struct mstimer_callback_data_t *cb;

    Millisecond_Counter++;
    cb = (struct mstimer_callback_data_t *)Callback_Head;
    while (cb) {
        if (mstimer_expired(&cb->timer)) {
            cb->callback();
            if (mstimer_interval(&cb->timer) > 0) {
                mstimer_reset(&cb->timer);
            }
        }
        cb = cb->next;
    }
}

/**
 * Returns the continuous milliseconds count, which rolls over
 *
 * @return the current milliseconds count
 */
unsigned long mstimer_now(void)
{
    uint32_t timer_value; /* return value */

    tc_set_overflow_interrupt_level(&MY_TIMER, TC_INT_LVL_OFF);
    timer_value = Millisecond_Counter;
    tc_set_overflow_interrupt_level(&MY_TIMER, TC_INT_LVL_LO);

    return timer_value;
}

/**
 * Configures and enables a repeating callback function
 *
 * @param new_cb - pointer to #mstimer_callback_data_t
 * @param callback - pointer to a #timer_callback_function function
 * @param milliseconds - how often to call the function
 *
 * @return true if successfully added and enabled
 */
void mstimer_callback(struct mstimer_callback_data_t *new_cb,
    mstimer_callback_function callback,
    unsigned long milliseconds)
{
    struct mstimer_callback_data_t *cb;

    tc_set_overflow_interrupt_level(&MY_TIMER, TC_INT_LVL_OFF);
    if (new_cb) {
        new_cb->callback = callback;
        mstimer_set(&new_cb->timer, milliseconds);
    }
    if (Callback_Head) {
        cb = (struct mstimer_callback_data_t *)Callback_Head;
        while (cb) {
            if (!cb->next) {
                cb->next = new_cb;
                break;
            } else {
                cb = cb->next;
            }
        }
    } else {
        Callback_Head = new_cb;
    }
    tc_set_overflow_interrupt_level(&MY_TIMER, TC_INT_LVL_LO);
}

/**
 * Timer setup for 1 millisecond timer
 */
void mstimer_init(void)
{
    unsigned long period;

    tc_enable(&MY_TIMER);
    tc_set_overflow_interrupt_callback(&MY_TIMER, my_callback);
    tc_set_wgm(&MY_TIMER, TC_WG_NORMAL);
    tc_write_count(&MY_TIMER, 1);
    period = sysclk_get_peripheral_bus_hz(&MY_TIMER);
    period /= 1000;
    tc_write_period(&MY_TIMER, period);
    tc_set_overflow_interrupt_level(&MY_TIMER, TC_INT_LVL_LO);
    tc_write_clock_source(&MY_TIMER, TC_CLKSEL_DIV1_gc);
}
