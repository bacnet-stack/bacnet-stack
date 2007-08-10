/**************************************************************************
*
* Copyright (C) 2007 Steve Karg <skarg@users.sourceforge.net>
* Portions of the AT91SAM7S startup code were developed by James P Lynch.
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
#include "AT91SAM7S256.h"
#include "board.h"
#include "math.h"
#include "stdlib.h"
#include "string.h"
#include "stdbool.h"
/* BACnet */
#include "rs485.h"
#include "datalink.h"
#include "npdu.h"
#include "apdu.h"
#include "dcc.h"
#include "iam.h"
#include "handlers.h"

//  *******************************************************
//   FIXME: use header files?     External References
//  *******************************************************
extern    void LowLevelInit(void);
extern    unsigned enableIRQ(void);
extern    unsigned enableFIQ(void);

extern    void TimerInit(void);
extern volatile unsigned long Timer_Milliseconds;

//  *******************************************************
//  FIXME: use header files?      Global Variables
//  *******************************************************
unsigned int FiqCount = 0;


static unsigned long LED_Timer_1 = 0;
static unsigned long LED_Timer_2 = 0;
static unsigned long LED_Timer_3 = 0;
static unsigned long LED_Timer_4 = 1000;

void millisecond_timer(void)
{
    while (Timer_Milliseconds) {
        Timer_Milliseconds--;
        if (LED_Timer_1) {
            LED_Timer_1--;
        }
        if (LED_Timer_2) {
            LED_Timer_2--;
        }
        if (LED_Timer_3) {
            LED_Timer_3--;
        }
        if (LED_Timer_4) {
            LED_Timer_4--;
        }
        dlmstp_millisecond_timer();
    }
}

int    main (void) {
    unsigned long    IdleCount = 0; // idle loop blink counter
    bool LED3_Off_Enabled = true;
    uint16_t pdu_len = 0;
    BACNET_ADDRESS src; /* source address */
    uint8_t pdu[MAX_MPDU]; /* PDU data */

    // Initialize the Atmel AT91SAM7S256
    // (watchdog, PLL clock, default interrupts, etc.)
    LowLevelInit();
    TimerInit();
    /* Initialize the Parallel I/O Controller A Peripheral Clock */
    volatile AT91PS_PMC    pPMC = AT91C_BASE_PMC;
    pPMC->PMC_PCER = pPMC->PMC_PCSR | (1<<AT91C_ID_PIOA);

    // Set up the LEDs (PA0 - PA3)
    volatile AT91PS_PIO pPIO = AT91C_BASE_PIOA;
    // PIO Enable Register
    // allow PIO to control pins P0 - P3 and pin 19
    pPIO->PIO_PER = LED_MASK | SW1_MASK;
    // PIO Output Enable Register
    // sets pins P0 - P3 to outputs
    pPIO->PIO_OER = LED_MASK;
    // PIO Set Output Data Register
    // turns off the four LEDs
    pPIO->PIO_SODR = LED_MASK;

    // Select PA19 (pushbutton) to be FIQ function (Peripheral B)
    pPIO->PIO_BSR = SW1_MASK;

    // Set up the AIC registers for FIQ (pushbutton SW1)
    volatile AT91PS_AIC pAIC = AT91C_BASE_AIC;
    // Disable FIQ interrupt in
    // AIC Interrupt Disable Command Register
    pAIC->AIC_IDCR = (1<<AT91C_ID_FIQ);
    // Set the interrupt source type in
    // AIC Source Mode Register[0]
    pAIC->AIC_SMR[AT91C_ID_FIQ] =
        (AT91C_AIC_SRCTYPE_INT_EDGE_TRIGGERED);
    // Clear the FIQ interrupt in
    // AIC Interrupt Clear Command Register
    pAIC->AIC_ICCR = (1<<AT91C_ID_FIQ);
    // Remove disable FIQ interrupt in
    // AIC Interrupt Disable Command Register
    pAIC->AIC_IDCR = (0<<AT91C_ID_FIQ);
    // Enable the FIQ interrupt in
    // AIC Interrupt Enable Command Register
    pAIC->AIC_IECR = (1<<AT91C_ID_FIQ);

#if defined(BACDL_MSTP)
    RS485_Set_Baud_Rate(38400);
    dlmstp_set_mac_address(55);
    dlmstp_set_max_master(127);
    dlmstp_set_max_info_frames(1);
    dlmstp_init(NULL);
#endif

#ifndef DLMSTP_TEST
    /* we need to handle who-is to support dynamic device binding */
    apdu_set_unconfirmed_handler(SERVICE_UNCONFIRMED_WHO_IS,
        handler_who_is);
    /* Set the handlers for any confirmed services that we support. */
    /* We must implement read property - it's required! */
    apdu_set_confirmed_handler(SERVICE_CONFIRMED_READ_PROPERTY,
        handler_read_property);
    apdu_set_confirmed_handler(SERVICE_CONFIRMED_REINITIALIZE_DEVICE,
        handler_reinitialize_device);
    apdu_set_confirmed_handler(SERVICE_CONFIRMED_WRITE_PROPERTY,
        handler_write_property);
    /* handle communication so we can shutup when asked */
    apdu_set_confirmed_handler
        (SERVICE_CONFIRMED_DEVICE_COMMUNICATION_CONTROL,
        handler_device_communication_control);
#endif

    // enable interrupts
    enableIRQ();
    enableFIQ();

    // endless blink loop
    while (1) {
        millisecond_timer();
        /* interrupt turns on the LED, we turn it off */
        if  (((pPIO->PIO_ODSR & LED3) == LED3) && (LED3_Off_Enabled))
        {
            LED3_Off_Enabled = false;
            /* wait */
            LED_Timer_3 = 250;
        }
        if (!LED_Timer_3) {
            /* turn LED3 (DS3) off */
            pPIO->PIO_SODR = LED3;
            LED3_Off_Enabled = true;
        }
        if (!LED_Timer_4) {
            if  ((pPIO->PIO_ODSR & LED4) == LED4) {
                // turn LED2 (DS2) on
                pPIO->PIO_CODR = LED4;
            } else {
                // turn LED2 (DS2) off
                pPIO->PIO_SODR = LED4;
            }
            /* wait */
            LED_Timer_4 = 1000;
        }
        // count # of times through the idle loop
        IdleCount++;
        /* BACnet handling */
        pdu_len = datalink_receive(&src, &pdu[0], MAX_MPDU, 0);
        if (pdu_len) {
            pPIO->PIO_CODR = LED3;
#ifndef DLMSTP_TEST
            npdu_handler(&src, &pdu[0], pdu_len);
#endif
        }
    }
}
