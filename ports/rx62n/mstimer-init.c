/**************************************************************************
 *
 * Copyright (C) 2011 Steve Karg <skarg@users.sourceforge.net>
 *
 * SPDX-License-Identifier: MIT
 *********************************************************************/
#include <stdbool.h>
#include <stdint.h>
#include "hardware.h"
#include "bacnet/basic/sys/mstimer.h"

/* counter for the the timer which wraps every 49.7 days */
static volatile uint32_t Millisecond_Counter;
/* forward prototype for interrupt service routine */
void int_cmt0_isr(void);

/*************************************************************************
 * Description: Timer Interrupt Handler
 * Returns: nothing
 * Notes: none
 *************************************************************************/
static void timer_interrupt_handler(void)
{
    Millisecond_Counter++;
}

/*************************************************************************
 * Description: Timer Interrupt Service Routine
 * Returns: nothing
 * Notes: none
 *************************************************************************/
void int_cmt0_isr(void)
{
    timer_interrupt_handler();
}

/*************************************************************************
 * Description: returns the current millisecond count
 * Returns: none
 * Notes: This method only disables the timer overflow interrupt.
 *************************************************************************/
unsigned long mstimer_now(void)
{
    unsigned long timer_value; /* return value */

    timer_value = Millisecond_Counter;

    return timer_value;
}

/*************************************************************************
 * Description: Initialization for Timer
 * Returns: none
 * Notes: none
 *************************************************************************/
void timer_init(void)
{
    /* Declare error flag */
    bool err = true;

    /* CMT is configured for a 1ms interval, and executes the callback
       function CB_CompareMatch on every compare match */
    err &= R_CMT_Create(3, PDL_CMT_PERIOD, 1E-3, int_cmt0_isr, 3);

    /* Halt in while loop when RPDL errors detected */
    while (!err)
        ;
}
