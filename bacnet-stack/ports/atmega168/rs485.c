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

/* The module handles sending data out the RS-485 port */
/* and handles receiving data from the RS-485 port. */
/* Customize this file for your specific hardware */
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include "mstp.h"

/* This file has been customized for use with ATMEGA168 */
#include "hardware.h"

/* baud rate */
static uint32_t RS485_Baud = 38400;

/****************************************************************************
* DESCRIPTION: Initializes the RS485 hardware and variables, and starts in
*              receive mode.
* RETURN:      none
* ALGORITHM:   none
* NOTES:       none
*****************************************************************************/
void RS485_Initialize(void)
{
    /* enable Transmit and Receive */
    UCSR0B = _BV(TXEN0) | _BV(RXEN0); 

    /* Set USART Control and Status Register n C */
    /* Asynchronous USART 8-bit data, No parity, 1 stop */
    /* Set USART Mode Select: UMSELn1 UMSELn0 = 00 for Asynchronous USART */
    /* Set Parity Mode:  UPMn1 UPMn0 = 00 for Parity Disabled */
    /* Set Stop Bit Select: USBSn = 0 for 1 stop bit */
    /* Set Character Size: UCSZn2 UCSZn1 UCSZn0 = 011 for 8-bit */
    /* Clock Polarity: UCPOLn = 0 when asynchronous mode is used. */
    UCSR0C = _BV(UCSZ01) | _BV(UCSZ00);
    /* Clear Power Reduction USART0 */
    BIT_CLEAR(PRR,PRUSART0);

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
            /* 2x speed mode */
            BIT_SET(UCSR0A,U2X0);
            /* configure baud rate */
            UBRR0 = (FREQ_CPU / (8UL * RS485_Baud)) - 1;
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
        turnaround_time = (40UL*1000UL)/RS485_Baud;
        if (!turnaround_time) {
            turnaround_time = 1;
        }
        while (mstp_port->SilenceTimer < turnaround_time) {
            /* do nothing - wait for timer to increment */
        };
    }
    while (nbytes) {
        while (!BIT_CHECK(UCSR0A,UDRE0)) {
            /* do nothing - wait until Tx buffer is empty */
        }
        UDR0 = *buffer;
        buffer++;
        nbytes--;
    }
    while (!BIT_CHECK(UCSR0A,TXC0)) {
        /* do nothing - wait until the entire frame in the 
           Transmit Shift Register has been shifted out */
    }
    /* per MSTP spec */
    if (mstp_port) {
        mstp_port->SilenceTimer = 0;
    }

    return;
}

/* called by timer, interrupt(?) or other thread */
void RS485_Check_UART_Data(struct mstp_port_struct_t *mstp_port)
{
    if (mstp_port->ReceiveError == true) {
        /* wait for state machine to clear this */
    }
    /* wait for state machine to read from the DataRegister */
    else if (mstp_port->DataAvailable == false) {
        /* check for error */
        if (BIT_CHECK(UCSR0A,FE0)) {
            mstp_port->ReceiveError = true;
        }

        if (BIT_CHECK(UCSR0A,DOR0)) {
            mstp_port->ReceiveError = true;
        }
        /* check for data */
        if (BIT_CHECK(UCSR0A,RXC0)) {
            mstp_port->DataRegister = UDR0;
            mstp_port->DataAvailable = true;
        }
    }
}
