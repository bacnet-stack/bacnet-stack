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
#include <stdbool.h>
#include <stdint.h>
#include "hardware.h"
#include "timer.h"

/* define various timers in timer.h file */
#ifndef MAX_MILLISECOND_TIMERS
#define MAX_MILLISECOND_TIMERS 2
#endif

/* Timer2 Prescaling: 1, 8, 32, 64, 128, 256, or 1024 */
#define TIMER2_PRESCALER 128
/* Count: Timer counts up to 0xFF and then signals overflow */
#define TIMER2_TICKS (F_CPU/TIMER2_PRESCALER/1000)
#if (TIMER2_TICKS > 0xFF)
#error Timer2 Prescaler value is too small
#endif
#define TIMER2_COUNT (0xFF-TIMER2_TICKS)

/* counter for the various timers */
static volatile unsigned long Millisecond_Counter[MAX_MILLISECOND_TIMERS];

/*************************************************************************
* Description: Timer Interrupt Handler
* Returns: none
* Notes: Global interupts must be enabled
*************************************************************************/
static inline void timer_interrupt_handler(
    void)
{
    unsigned i; /* loop counter */

    /* increment the tick count */
    for (i = 0; i < MAX_MILLISECOND_TIMERS; i++) {
        Millisecond_Counter[i]++;
    }
}

/*************************************************************************
* Description: Timer Interrupt Service Routine - Timer Overflowed!
* Returns: none
* Notes: Global interupts must be enabled
*************************************************************************/
ISR(TIMER2_OVF_vect)
{
    /* Set the counter for the next interrupt */
    TCNT2 = TIMER2_COUNT;
    timer_interrupt_handler();
}

/*************************************************************************
* Description: sets the current time count with a value
* Returns: none
* Notes: none
*************************************************************************/
unsigned long timer_milliseconds_set(
    unsigned index,
    unsigned long value)
{
    uint8_t sreg = 0;   /* holds interrupts pending */
    unsigned long old_value = 0;        /* return value */

    if (index < MAX_MILLISECOND_TIMERS) {
        sreg = SREG;
        __disable_interrupt();
        old_value = Millisecond_Counter[index];
        Millisecond_Counter[index] = value;
        SREG = sreg;
    }

    return old_value;
}

/*************************************************************************
* Description: returns the current millisecond count
* Returns: none
* Notes: none
*************************************************************************/
unsigned long timer_milliseconds(
    unsigned index)
{
    unsigned long timer_value = 0;      /* return value */
    uint8_t sreg = 0;   /* holds interrupts pending */

    if (index < MAX_MILLISECOND_TIMERS) {
        sreg = SREG;
        __disable_interrupt();
        timer_value = Millisecond_Counter[index];
        SREG = sreg;
    }

    return timer_value;
}

/*************************************************************************
* Description: compares the current time count with a value
* Returns: true if the time has elapsed
* Notes: none
*************************************************************************/
bool timer_elapsed_milliseconds(
    unsigned index,
    unsigned long value)
{
    return (timer_milliseconds(index) >= value);
}

/*************************************************************************
* Description: compares the current time count with a value
* Returns: true if the time has elapsed
* Notes: none
*************************************************************************/
bool timer_elapsed_seconds(
    unsigned index,
    unsigned long seconds)
{
    return ((timer_milliseconds(index) / 1000UL) >= seconds);
}

/*************************************************************************
* Description: compares the current time count with a value
* Returns: true if the time has elapsed
* Notes: none
*************************************************************************/
bool timer_elapsed_minutes(
    unsigned index,
    unsigned long minutes)
{
    return ((timer_milliseconds(index) / (1000UL * 60UL)) >= minutes);
}

/*************************************************************************
* Description: Sets the timer counter to zero.
* Returns: none
* Notes: none
*************************************************************************/
unsigned long timer_reset(
    unsigned index)
{
    return timer_milliseconds_set(index, 0);
}

/*************************************************************************
* Description: Initialization for Timer
* Returns: none
* Notes: none
*************************************************************************/
static void timer2_init(
    void)
{
    /* Normal Operation */
    TCCR2A = 0;
    /* Timer2: prescale selections:
       CSn2 CSn1 CSn0 Description
       ---- ---- ---- -----------
       0    0    0  No Clock Source
       0    0    1  No prescaling
       0    1    0  CLKt2s/8
       0    1    1  CLKt2s/32
       1    0    0  CLKt2s/64
       1    0    1  CLKt2s/128
       1    1    0  CLKt2s/256
       1    1    1  CLKt2s/1024
     */
#if (TIMER2_PRESCALER==1)
    TCCR2B = _BV(CS20);
#elif (TIMER2_PRESCALER==8)
    TCCR2B = _BV(CS21);
#elif (TIMER2_PRESCALER==32)
    TCCR2B = _BV(CS21) | _BV(CS20);
#elif (TIMER2_PRESCALER==64)
    TCCR2B = _BV(CS22);
#elif (TIMER2_PRESCALER==128)
    TCCR2B = _BV(CS22) | _BV(CS20);
#elif (TIMER2_PRESCALER==256)
    TCCR2B = _BV(CS22) | _BV(CS21);
#elif (TIMER2_PRESCALER==1024)
    TCCR2B = _BV(CS22) | _BV(CS21) | _BV(CS20);
#else
#error Timer2 Prescale: Invalid Value
#endif
    /* Clear any TOV Flag set when the timer overflowed */
    BIT_CLEAR(TIFR2, TOV2);
    /* Initial value */
    TCNT2 = TIMER2_COUNT;
    /* Enable the overflow interrupt */
    BIT_SET(TIMSK2, TOIE2);
    /* Clear the Power Reduction Timer/Counter0 */
    BIT_CLEAR(PRR, PRTIM2);
}

/*************************************************************************
* Description: Initialization for Timer
* Returns: none
* Notes: none
*************************************************************************/
void timer_init(
    void)
{
    timer2_init();
}
