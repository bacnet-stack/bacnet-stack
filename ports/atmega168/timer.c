/**************************************************************************
 *
 * Copyright (C) 2007 Steve Karg <skarg@users.sourceforge.net>
 *
 * SPDX-License-Identifier: MIT
 *
 *********************************************************************/
#include <stdint.h>
#include "hardware.h"
#include "timer.h"

/* This module is a 1 millisecond timer */

/* Prescaling: 1, 8, 64, 256, 1024 */
#define TIMER_PRESCALER 64
/* Count: Timer0 counts up to 0xFF and then signals overflow */
#define TIMER_TICKS (F_CPU / TIMER_PRESCALER / 1000)
#if (TIMER_TICKS > 0xFF)
#error Timer Prescaler value is too small
#endif
#define TIMER_COUNT (0xFF - TIMER_TICKS)
/* Global variable millisecond timer - used by main.c for timers task */
volatile uint8_t Timer_Milliseconds = 0;
/* MS/TP Silence Timer */
static volatile uint16_t SilenceTime;

/* Configure the Timer */
void Timer_Initialize(void)
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
    TCCR0B = _BV(CS01) | _BV(CS00);
    /* Clear any TOV1 Flag set when the timer overflowed */
    BIT_CLEAR(TIFR0, TOV0);
    /* Initial value */
    TCNT0 = TIMER_COUNT;
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
    TCNT0 = TIMER_COUNT;
    /* Overflow Flag is automatically cleared */
    /* Update the global timer */
    if (Timer_Milliseconds < 0xFF)
        Timer_Milliseconds++;
    if (SilenceTime < 0xFFFF)
        SilenceTime++;
}

/* Public access to the Silence Timer */
uint16_t Timer_Silence(void)
{
    uint16_t timer;

    BIT_CLEAR(TIMSK0, TOIE0);
    timer = SilenceTime;
    BIT_SET(TIMSK0, TOIE0);

    return timer;
}

/* Public reset of the Silence Timer */
void Timer_Silence_Reset(void)
{
    BIT_CLEAR(TIMSK0, TOIE0);
    SilenceTime = 0;
    BIT_SET(TIMSK0, TOIE0);
}
