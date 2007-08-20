/**************************************************************************
*
* Copyright (C) 2007 Steve Karg <skarg@users.sourceforge.net>
* RS-485 initialization on AT91SAM7S inspired by Keil Eletronik serial.c
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

/* The module handles sending data out the RS-485 port */
/* and handles receiving data from the RS-485 port. */
/* Customize this file for your specific hardware */
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include "mstp.h"
#include "timer.h"

/* This file has been customized for use with UART0
   on the AT91SAM7S-EK */
#include "board.h"

/* UART */
static volatile AT91S_USART *RS485_Interface = AT91C_BASE_US0;
/* baud rate */
static int RS485_Baud = 38400;

/****************************************************************************
* DESCRIPTION: Sets the interface - UART0 or UART1 or...
* RETURN:      none
* ALGORITHM:   none
* NOTES:       none
*****************************************************************************/
void RS485_Set_Interface(char *ifname)
{
    RS485_Interface = (volatile AT91S_USART *)ifname;
}

/****************************************************************************
* DESCRIPTION: Initializes the RS485 hardware and variables, and starts in
*              receive mode.
* RETURN:      none
* ALGORITHM:   none
* NOTES:       none
*****************************************************************************/
void RS485_Initialize(void)
{
    /* Enable the USART0 clock in the Power Management Controller */
    volatile AT91PS_PMC pPMC = AT91C_BASE_PMC;
    pPMC->PMC_PCER = pPMC->PMC_PCSR | (1<<AT91C_ID_US0);

    /* Disable and clear USART0 interrupt
       in AIC Interrupt Disable Command Register */
    volatile AT91PS_AIC pAIC = AT91C_BASE_AIC;
    pAIC->AIC_IDCR = (1<<AT91C_ID_US0);
    pAIC->AIC_ICCR = (1<<AT91C_ID_US0);
   
    /* enable the peripheral by disabling the pin in the PIO controller */
    *AT91C_PIOA_PDR = AT91C_PA5_RXD0 | AT91C_PA6_TXD0 | AT91C_PA7_RTS0;

    RS485_Interface->US_CR =
        AT91C_US_RSTRX |          /* Reset Receiver      */
        AT91C_US_RSTTX |          /* Reset Transmitter   */
        AT91C_US_RSTSTA |          /* Clear status register */
        AT91C_US_RXDIS |          /* Receiver Disable    */
        AT91C_US_TXDIS;           /* Transmitter Disable */

    RS485_Interface->US_MR =
        AT91C_US_USMODE_RS485  |  /* RS-485 Mode - RTS auto assert */
        AT91C_US_CLKS_CLOCK    |  /* Clock = MCK */
        AT91C_US_CHRL_8_BITS   |  /* 8-bit Data  */
        AT91C_US_PAR_NONE      |  /* No Parity   */
        AT91C_US_NBSTOP_1_BIT;    /* 1 Stop Bit  */

    /* set the Time Guard to release RTS after x bit times */
    RS485_Interface->US_TTGR = 4;
    
    /* Receiver Time-out disabled */
    RS485_Interface->US_RTOR = 0;

    /* baud rate */
    RS485_Interface->US_BRGR = MCK/16/RS485_Baud;

    RS485_Interface->US_CR =
        AT91C_US_RXEN  |          /* Receiver Enable     */
        AT91C_US_TXEN;            /* Transmitter Enable  */
        
    return;
}

void RS485_Cleanup(void)
{

}

/****************************************************************************
* DESCRIPTION: Returns the baud rate that we are currently running at
* RETURN:      none
* ALGORITHM:   none
* NOTES:       none
*****************************************************************************/
uint32_t RS485_Get_Baud_Rate(void)
{
    return RS485_Baud;
}

/****************************************************************************
* DESCRIPTION: Sets the baud rate for the chip USART
* RETURN:      none
* ALGORITHM:   none
* NOTES:       none
*****************************************************************************/
bool RS485_Set_Baud_Rate(uint32_t baud)
{
    bool valid = true;

    switch (baud) {
        case 9600:
        case 19200:
        case 38400:
        case 57600:
        case 76800:
        case 115200:
            RS485_Baud = baud;
            RS485_Interface->US_BRGR = MCK/16/baud;
            /* FIXME: store the baud rate */
            break;
        default:
            valid = false;
            break;
    }

    return valid;
}

/* Transmits a Frame on the wire */
void RS485_Send_Frame(
    volatile struct mstp_port_struct_t *mstp_port, /* port specific data */
    uint8_t * buffer,  /* frame to send (up to 501 bytes of data) */
    uint16_t nbytes)   /* number of bytes of data (up to 501) */
{
    uint8_t turnaround_time;

    /* delay after reception - per MS/TP spec */
    if (mstp_port) {
        /* wait about 40 bit times since reception */
        turnaround_time = (40*1000)/RS485_Baud;
        if (!turnaround_time) {
            turnaround_time = 1;
        }
        while (mstp_port->SilenceTimer() < turnaround_time) {
            /* do nothing - wait for timer to increment */
        };
    }
    /* toggle LED on send */
    volatile AT91PS_PIO pPIO = AT91C_BASE_PIOA;
    /* LED ON */
    pPIO->PIO_CODR = LED1;
    while (nbytes) {
        while (!(RS485_Interface->US_CSR & AT91C_US_TXRDY)) {
            /* do nothing - wait until Tx buffer is empty */
        }
        RS485_Interface->US_THR = *buffer;
        buffer++;
        nbytes--;
        /* per MSTP spec */
        if (mstp_port) {
            mstp_port->SilenceTimerReset();
        }
    }
    while (!(RS485_Interface->US_CSR & AT91C_US_TXRDY)) {
        /* do nothing - wait until Tx buffer is empty */
    }

    return;
}

/* called by timer, interrupt(?) or other thread */
void RS485_Check_UART_Data(struct mstp_port_struct_t *mstp_port)
{
    volatile AT91PS_PIO pPIO = AT91C_BASE_PIOA;

    if (mstp_port->ReceiveError == true) {
        /* wait for state machine to clear this */
    } else if (mstp_port->DataAvailable == false) {
        /* check for data or error */
        if (RS485_Interface->US_CSR & (AT91C_US_OVRE | AT91C_US_FRAME)) {
            /* clear the error flag */
            RS485_Interface->US_CR = AT91C_US_RSTSTA;
            mstp_port->ReceiveError = true;
            /* LED ON */
            pPIO->PIO_CODR = LED2;
        } else if ( RS485_Interface->US_CSR & AT91C_US_RXRDY) {
            /* data is available */
            mstp_port->DataRegister = RS485_Interface->US_RHR;
            mstp_port->DataAvailable = true;
            /* LED ON */
            pPIO->PIO_CODR = LED2;
        }
    }
}

#ifdef TEST_RS485
static struct mstp_port_struct_t MSTP_Port;

int main(void)
{
    unsigned i = 0;

    RS485_Set_Baud_Rate(38400);
    RS485_Initialize();
    /* receive task */
    for (;;) {
        RS485_Check_UART_Data(&MSTP_Port);
        if (MSTP_Port.DataAvailable) {
                fprintf(stderr,"%02X ",MSTP_Port.DataRegister);
                MSTP_Port.DataAvailable = false;
        }
    }
}
#endif                          /* TEST_ABORT */

