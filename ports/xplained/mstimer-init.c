/**
 * @file
 * @brief 1ms timer configuration
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date 2013
 * @copyright SPDX-License-Identifier: MIT
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
 * Handles an interrupt from a hardware counter timer, every millisecond
 */
static void mstimer_counter_handler(void)
{
    Millisecond_Counter++;
}

/**
 * Handles an interrupt from an overflow hardware timer, every millisecond
 */
static void mstimer_overflow_handler(void)
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
        mstimer_overflow_handler);
    tc_set_wgm(&MS_TIMER_CALLBACK, TC_WG_NORMAL);
    tc_write_count(&MS_TIMER_CALLBACK, 1);
    period = sysclk_get_peripheral_bus_hz(&MS_TIMER_CALLBACK);
    period /= 1000;
    tc_write_period(&MS_TIMER_CALLBACK, period);
    tc_set_overflow_interrupt_level(&MS_TIMER_CALLBACK, TC_INT_LVL_LO);
    tc_write_clock_source(&MS_TIMER_CALLBACK, TC_CLKSEL_DIV1_gc);
}
