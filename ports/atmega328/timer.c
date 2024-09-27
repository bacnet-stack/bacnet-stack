/**
 * @brief This module handles the 1 millisecond timer for the mstimer library
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date 2007
 * @copyright SPDX-License-Identifier: MIT
 */
#include <stdint.h>
#include "hardware.h"
#include "bacnet/basic/sys/mstimer.h"

/* Prescaling: 1, 8, 64, 256, 1024 */
#define TIMER_MICROSECONDS 1000UL
#define TIMER_TICKS(p) \
    (((((F_CPU) / (p)) / 1000UL) * (TIMER_MICROSECONDS)) / 1000UL)
#define TIMER_TICKS_MAX 255UL
/* adjust the prescaler for the processor clock */
#if (TIMER_TICKS(1) <= TIMER_TICKS_MAX)
#define TIMER0_PRESCALER 1
#elif (TIMER_TICKS(8) <= TIMER_TICKS_MAX)
#define TIMER0_PRESCALER 8
#elif (TIMER_TICKS(64) <= TIMER_TICKS_MAX)
#define TIMER0_PRESCALER 64
#elif (TIMER_TICKS(256) <= TIMER_TICKS_MAX)
#define TIMER0_PRESCALER 256
#elif (TIMER_TICKS(1024) <= TIMER_TICKS_MAX)
#define TIMER0_PRESCALER 1024
#else
#error "TIMER0: F_CPU too large for timer prescaler."
#endif
#define TIMER0_TICKS TIMER_TICKS(TIMER0_PRESCALER)
/* Timer counts up from count to TIMER_TICKS_MAX and then signals overflow */
#define TIMER0_COUNT (TIMER_TICKS_MAX - TIMER0_TICKS)

/* millisecond time counter  */
static volatile unsigned long Millisecond_Counter;

/* Configure the Timer */
void mstimer_init(void)
{
    /* Normal Operation */
    TCCR1A = 0;
/* CSn2 CSn1 CSn0 Description
   ---- ---- ---- -----------
   0    0    0  No Clock Source
   0    0    1  No prescaling
   0    1    0  CLKio/8
   0    1    1  CLKio/64
   1    0    0  CLKio/256
   1    0    1  CLKio/1024
   1    1    0  Falling Edge of T0 (external)
   1    1    1  Rising Edge of T0 (external)
 */
#if (TIMER0_PRESCALER == 1)
    TCCR0B = _BV(CS00);
#elif (TIMER0_PRESCALER == 8)
    TCCR0B = _BV(CS01);
#elif (TIMER0_PRESCALER == 64)
    TCCR0B = _BV(CS01) | _BV(CS00);
#elif (TIMER0_PRESCALER == 256)
    TCCR0B = _BV(CS02);
#elif (TIMER0_PRESCALER == 1024)
    TCCR0B = _BV(CS02) | _BV(CS00);
#else
#error Timer0 Prescale: Invalid Value
#endif
    /* Clear any TOV1 Flag set when the timer overflowed */
    BIT_CLEAR(TIFR0, TOV0);
    /* Initial value */
    TCNT0 = TIMER0_COUNT;
    /* Enable the overflow interrupt */
    BIT_SET(TIMSK0, TOIE0);
    /* Clear the Power Reduction Timer/Counter0 */
    BIT_CLEAR(PRR, PRTIM0);
}

/* Timer interupt */
/* note: Global interupts must be enabled - sei() */
/* Timer Overflowed!  Increment the time. */
ISR(TIMER0_OVF_vect)
{
    /* Set the counter for the next interrupt */
    TCNT0 = TIMER0_COUNT;
    /* Overflow Flag is automatically cleared */
    /* Update the global timer */
    Millisecond_Counter++;
}

/**
 * Get the current time in milliseconds
 * @return milliseconds
 */
unsigned long mstimer_now(void)
{
    unsigned long milliseconds;

    BIT_CLEAR(TIMSK0, TOIE0);
    milliseconds = Millisecond_Counter;
    BIT_SET(TIMSK0, TOIE0);

    return milliseconds;
}
