/**************************************************************************
 *
 * Copyright (C) 2011 Steve Karg <skarg@users.sourceforge.net>
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
    NVIC_EnableIRQ(SysTick_IRQn);
}
