/**************************************************************************
*
* Copyright (C) 2003 Mark Norton
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
* Description:  Handles the init code for the Microchip microprocessor
*
*********************************************************************/
#include <string.h>
#include <stdint.h>
#include "hardware.h"

/* define this to enable ICD */
/*#define USE_ICD  */

/* Configuration Bits  */
#pragma config OSC = HS
#pragma config PWRT = ON
#pragma config BOR = ON, BORV = 42
#pragma config CCP2MUX = ON
#pragma config STVR = ON
#pragma config LVP = OFF
#pragma config CP0 = OFF
#pragma config CP1 = OFF
#pragma config CP2 = OFF
#pragma config CP3 = OFF
#pragma config CPB = OFF
#pragma config CPD = OFF
#pragma config WRT0 = OFF
#pragma config WRT1 = OFF
#pragma config WRT2 = OFF
#pragma config WRT3 = OFF
#pragma config WRTB = OFF
#pragma config WRTC = OFF
#pragma config WRTD = OFF
#pragma config EBTR0 = OFF
#pragma config EBTR1 = OFF
#pragma config EBTR2 = OFF
#pragma config EBTR3 = OFF
#pragma config EBTRB = OFF

#ifdef USE_ICD
#pragma config WDT = OFF, WDTPS = 128
#pragma config DEBUG = ON
#else
#pragma config WDT = ON, WDTPS = 128
#pragma config DEBUG = OFF
#endif                          /* USE_ICD */

#pragma romdata

/****************************************************************************
* DESCRIPTION: Initializes the PIC, its timers, WDT, etc.
* RETURN:      none
* ALGORITHM:   none
* NOTES:       none
*****************************************************************************/
void init_hardware(void)
{
    /* If the processor gets a power on reset then we can do something. */
    /* We should not get a reset unless there has been  */
    /* some kind of power line disturbance. */
    if (RCONbits.POR) {
        /*do something special! */
    }

    GLOBAL_INT_DISABLE();

    /* Setup PORT A */
    TRISA = PORT_A_TRIS_MASK;

    /* PORT A can have analog inputs or digital IO */
    ADCON1 = NO_ANALOGS;

    /* Setup PORT B */
    TRISB = PORT_B_TRIS_MASK;

    /* Setup PORT C */
    TRISC = PORT_C_TRIS_MASK;

    /* setup zero cross interrupt to trigger on a low to high edge */
    INTCON2bits.INTEDG0 = 1;

    /* setup ABUS */
    /*ABUS_LED_OFF(); */
    /*SSPADD = ABUS_DEFAULT_ADDR; */
    /*SSPCON1 = (ABUS_SLAVE_MASK | ABUS_CLK_ENABLE | ABUS_MODE_SETUP); */
    /*ABUS_Clear_SSPBUF(); */

    /* setup timer2 to reset every 1ms */
    CloseTimer2();
    PR2 = 250;
    OpenTimer2(T2_PS_1_4 & T2_POST_1_5 & 0x7F);

    /* Setup our interrupt priorities ---------> all low priority */
    RCONbits.IPEN = 1;
    IPR1 = 0;
    IPR2 = 0;
    INTCON2bits.TMR0IP = 0;
    INTCON2bits.RBIP = 0;
    INTCON3 = 0;

    /* Enable interrupts */
    TIMER2_INT_ENABLE();
    PERIPHERAL_INT_ENABLE();
    GLOBAL_INT_ENABLE();

/* Turn on the Zero cross interrupt */
    INTCONbits.INT0F = 0;
    INTCONbits.INT0E = 1;
}
