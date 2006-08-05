/**************************************************************************
*
* Copyright (C) 2003 Mark Norton and Steve Karg
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
* Functional
* Description:  Defines the interrupt service routines (ISR) for the
*               Microchip microprocessor used in the Synergy lighting
*               control project.
*
*********************************************************************/
#include <stdint.h>
#include "hardware.h"
#include "timer.h"

/* interrupt service routines */
extern void RS485_Receive_Interrupt(void);
extern void RS485_Transmit_Interrupt(void);

/****************************************************************************
* DESCRIPTION: High priority interrupt routine
* PARAMETERS:  none
* RETURN:      none
* ALGORITHM:   none
* NOTES:       none
*****************************************************************************/
#pragma interruptlow InterruptHandlerLow save=PROD, section(".tmpdata")
void InterruptHandlerLow(void)
{
    /* check for timer0 interrupt */
    if ((INTCONbits.TMR0IF) && (INTCONbits.TMR0IE)) {
        /* clear interrupt flag */
        INTCONbits.TMR0IF = 0;
        /* call interrupt handler */
    }
    /*check for timer1 interrupt */
    if ((PIR1bits.TMR1IF) && (PIE1bits.TMR1IE)) {
        PIR1bits.TMR1IF = 0;
        /* call interrupt handler */
    }
    /*check for timer2 interrupt */
    if ((PIR1bits.TMR2IF) && (PIE1bits.TMR2IE)) {
        PIR1bits.TMR2IF = 0;
        Timer_Millisecond_Interrupt();
    }
    /*check for timer3 interrupt */
    if ((PIR2bits.TMR3IF) && (PIE2bits.TMR3IE)) {
        PIR2bits.TMR3IF = 0;
        /* call interrupt handler */
    }
    /*check for compare 1 int */
    if ((PIR1bits.CCP1IF) && (PIE1bits.CCP1IE)) {
        PIR1bits.CCP1IF = 0;
        /* call interrupt handler */
    }
    /*check for compare 2 int */
    if ((PIR2bits.CCP2IF) && (PIE2bits.CCP2IE)) {
        PIR2bits.CCP2IF = 0;
        /* call interrupt handler */
    }
    /*check for USART Rx int */
    if ((PIR2bits.EEIF) && (PIE2bits.EEIE)) {
        PIR2bits.EEIF = 0;      /*clear interrupt flag */
        EECON1bits.WREN = 0;    /* disable writes */
        /* call interrupt handler */
    }
    /*check for USART Tx int */
    if ((PIR1bits.TXIF) && (PIE1bits.TXIE)) {
        /* call interrupt handler */
        RS485_Transmit_Interrupt();
    }
    /*check for USART Rx int */
    if ((PIR1bits.RCIF) && (PIE1bits.RCIE)) {
        /* call interrupt handler */
        RS485_Receive_Interrupt();
    }
    /*check for AD int */
    if ((PIR1bits.ADIF) && (PIE1bits.ADIE)) {
        /* call interrupt handler */
        PIR1bits.ADIF = 0;
    }
    /*check for I2C receive int (MSSP int) */
    if ((PIR1bits.SSPIF) && (PIE1bits.SSPIE)) {
        PIR1bits.SSPIF = 0;
        /* call interrupt handler */
    }

    return;
}

/****************************************************************************
* DESCRIPTION: High priority interrupt routine
* PARAMETERS:  none
* RETURN:      none
* ALGORITHM:   none
* NOTES:       don't call functions from this function because registers are
*              not saved, and saving registers is slower.
*****************************************************************************/
#pragma interrupt InterruptHandlerHigh
void InterruptHandlerHigh(void)
{
    /*check for external int */
    if ((INTCONbits.INT0IF) && (INTCONbits.INT0IE)) {
        /* Test to ensure that we are not getting a false trigger on the
           falling edge. Only trigger on Rising edge. */
        if (ZERO_CROSS) {
            /* timer used to determine when power is gone (no zero crosses) */
/*      Power_Timeout = 30; */
/*      if (ABUS_Current_Status.Zerox_Fail) */
/*      { */
/*        ABUS_Flags.SendStatus = TRUE; */
/*        ABUS_Current_Status.Zerox_Fail = FALSE; */
/*      } */
            /* if we get here it means power is good */
/*      System_Flags.PowerFail = FALSE; */
        }
        INTCONbits.INT0IF = 0;
    }
    return;
}

/****************************************************************************
* DESCRIPTION: High priority interrupt vector
* PARAMETERS:  none
* RETURN:      none
* ALGORITHM:   none
* NOTES:       ISRs not here because we would only have 0x10 bytes for code
*****************************************************************************/
#pragma code InterruptVectorHigh = 0x08
void InterruptVectorHigh(void)
{
    _asm goto InterruptHandlerHigh      /*jump to interrupt routine */
 _endasm}
#pragma code
/****************************************************************************
* DESCRIPTION: Low priority interrupt vector
* PARAMETERS:  none
* RETURN:      none
* ALGORITHM:   none
* NOTES:       none
*****************************************************************************/
#pragma code InterruptVectorLow = 0x18
void InterruptVectorLow(void)
{
    _asm goto InterruptHandlerLow       /*jump to interrupt routine */
 _endasm}
#pragma code
