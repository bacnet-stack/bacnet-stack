/**************************************************************************
 *
 * Copyright (C) 2007 Steve Karg <skarg@users.sourceforge.net>
 *
 * SPDX-License-Identifier: MIT
 *
 *********************************************************************/

#include <stdbool.h>
#include <stdint.h>
#include <string.h> /* for memmove */
#include <stdlib.h>
#include <string.h>
#include "stdint.h"
#include "hardware.h"
/* BACnet */
#include "bacnet/apdu.h"
#include "bacnet/datalink/datalink.h"
#include "bacnet/dcc.h"
#include "bacnet/basic/services.h"
#include "bacnet/basic/services.h"
#include "bacnet/basic/tsm/tsm.h"
#include "rs485.h"

/* chip configuration data */
/* define this to enable ICD */
/* #define USE_ICD */

/* Configuration Bits  */
#pragma config OSC = HS, OSCS = OFF
#pragma config PWRT = ON
#pragma config BOR = ON, BORV = 27
#pragma config CCP2MUX = ON
#pragma config STVR = ON
#pragma config LVP = OFF
#pragma config CP0 = OFF
#pragma config CP1 = OFF
#pragma config CP2 = OFF
#pragma config CP3 = OFF
#pragma config CP4 = OFF
#pragma config CP5 = OFF
#pragma config CP6 = OFF
#pragma config CP7 = OFF
#pragma config CPB = OFF
#pragma config CPD = OFF
#pragma config WRT0 = OFF
#pragma config WRT1 = OFF
#pragma config WRT2 = OFF
#pragma config WRT3 = OFF
#pragma config WRT4 = OFF
#pragma config WRT5 = OFF
#pragma config WRT6 = OFF
#pragma config WRT7 = OFF
#pragma config WRTB = OFF
#pragma config WRTC = OFF
#pragma config WRTD = OFF
#pragma config EBTR0 = OFF
#pragma config EBTR1 = OFF
#pragma config EBTR2 = OFF
#pragma config EBTR3 = OFF
#pragma config EBTR4 = OFF
#pragma config EBTR5 = OFF
#pragma config EBTR6 = OFF
#pragma config EBTR7 = OFF
#pragma config EBTRB = OFF

#ifdef USE_ICD
#pragma config WDT = OFF, WDTPS = 128
#pragma config DEBUG = ON
#else
#pragma config WDT = ON, WDTPS = 128
#pragma config DEBUG = OFF
#endif /* USE_ICD */

volatile uint8_t Milliseconds = 0;
volatile uint8_t Zero_Cross_Timeout = 0;

void Reinitialize(void)
{
    uint8_t i;
    char name = 0;

    _asm reset _endasm return;
}

void Global_Int(enum INT_STATE state)
{
    static uint8_t intstate = 0;

    switch (state) {
        case INT_DISABLED:
            intstate >>= 2;
            intstate |= (INTCON & 0xC0);
            break;
        case INT_ENABLED:
            INTCONbits.GIE = 1;
            INTCONbits.PEIE = 1;
            intstate <<= 2;
            break;
        case INT_RESTORE:
            INTCON |= (intstate & 0xC0);
            intstate <<= 2;
            break;
        default:
            break;
    }
}

void Hardware_Initialize(void)
{
    TRISA = 0x00;
    TRISB = 0x00;
    TRISC = 0x00;
    TRISD = 0x00;
    TRISE = 0x00;
    TRISF = 0x00;
    TRISG = 0x00;
    /* We will use Timer4 as our system tick timer. Our system tick is set
     * to 1ms. Hold off on enabling the int. */
    setup_timer4(5, 250, 5);
    /* Setup our interrupt priorities */
    RCONbits.IPEN = 1;
    IPR1 = 0;
    IPR2 = 0;
    IPR3 = 0;
    /* Setup TMR0 to be high priority */
    INTCON2 = 0xFC;
    INTCON3 = 0;
    /* USART 1 high priority */
    IPR1bits.RC1IP = 1;
    IPR1bits.TX1IP = 1;
    /* Finally enable our ints */
    Global_Int(INT_ENABLED);
}

void Initialize_Variables(void)
{
    /* Check to see if we need to initialize our eeproms */
    ENABLE_TIMER4_INT();
    /* interrupts must be enabled before we read our inputs */
    Global_Int(INT_ENABLED);
    /* Start our time from now */
    Milliseconds = 0;
}

void MainTasks(void)
{
    static uint16_t millisecond_counter = 0;
    /* Handle our millisecond counters */
    while (Milliseconds) {
        millisecond_counter++;
        --Milliseconds;
    }
    /* Handle our seconds counters */
    if (millisecond_counter > 1000) {
        millisecond_counter -= 1000;
        dcc_timer_seconds(1);
    }
}

void main(void)
{
    RCONbits.NOT_POR = 1;
    RCONbits.NOT_RI = 1;
    Hardware_Initialize();
    Initialize_Variables();
    /* initialize BACnet Data Link Layer */
    dlmstp_set_my_address(42);
    dlmstp_set_max_info_frames(1);
    dlmstp_set_max_master(127);
    RS485_Set_Baud_Rate(38400);
    dlmstp_init();
    /* Handle anything that needs to be done on powerup */
    /* Greet the BACnet world! */
    Send_I_Am(&Handler_Transmit_Buffer[0]);
    /* Main loop */
    while (TRUE) {
        RESTART_WDT();
        dlmstp_task();
        MainTasks();
        Global_Int(INT_ENABLED);
        ENABLE_TIMER4_INT();
    }
}
