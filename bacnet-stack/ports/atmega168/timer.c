/**************************************************************************
*
* Copyright (C) 2007 Steve Karg <skarg@users.sourceforge.net>
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
*********************************************************************/

#include "hardware.h"

/* Prescaling: 1, 8, 64, 256, 1024 */
#define TIMER_1_PRESCALER 1
/* Count: Timer counts up to 0xFFFF and then signals overflow */
#define TIMER_1_TICKS (FREQ_CPU/TIMER_1_PRESCALER/1000)
#define TIMER_1_COUNT (0xFFFF-TIMER_1_TICKS)
/* Global variable millisecond timer - used by main.c for timers task */
volatile uint8_t Timer_Milliseconds = 0;

/* Configure the Timer */
void timer_initialize(void)
{
    /* Normal Operation */
    TCCR1A = 0;
    /* CS10 = clkI/O/1 (No prescaling) */
    TCCR1B = _BV(CS10);
    /* Clear any TOV1 Flag set when the timer overflowed */
    BIT_CLEAR(TIFR1,TOV1);
    /* Initial value */
    TCNT1 = TIMER_1_COUNT;
    /* Enable the overflow interrupt */
    BIT_SET(TIMSK1,TOIE1);
    /* Clear the Power Reduction Timer/Counter0 */
    BIT_CLEAR(PRR,PRTIM1);
}


/* Timer Overflowed!  Increment the time. */
ISR(TIMER1_OVF_vect)
{
    /* Set the counter for the next interrupt */
    TCNT1 = TIMER_1_COUNT;
    /* Overflow Flag is automatically cleared */
    if (Timer_Milliseconds < 0xFF)
       Timer_Milliseconds++;
}
