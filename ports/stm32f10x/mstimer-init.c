/**************************************************************************
 *
 * Copyright (C) 2011 Steve Karg <skarg@users.sourceforge.net>
 *
 * SPDX-License-Identifier: MIT
 *
 * Module Description:
 * Generate a periodic timer tick for use by generic timers in the code.
 *
 *************************************************************************/
#include <stdbool.h>
#include <stdint.h>
#include "hardware.h"
#include "bacnet/basic/sys/mstimer.h"

/* counter for the various timers */
static volatile unsigned long Millisecond_Counter;

/**
 * Handles the interrupt from the timer
 */
void SysTick_Handler(void)
{
    /* increment the tick count */
    Millisecond_Counter++;
    /* run any callbacks */
    mstimer_callback_handler();
}

/**
 * Returns the continuous milliseconds count, which rolls over
 *
 * @return the current milliseconds count
 */
unsigned long mstimer_now(void)
{
    return Millisecond_Counter;
}

/**
 * Timer setup for 1 millisecond timer
 */
void mstimer_init(void)
{
    /* Setup SysTick Timer for 1ms interrupts  */
    if (SysTick_Config(SystemCoreClock / 1000)) {
        while (1) {
            /* config error? return  1 = failed, 0 = successful */
        }
    }
}
