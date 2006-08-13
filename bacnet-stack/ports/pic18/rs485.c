/**************************************************************************
*
* Copyright (C) 2005 Steve Karg <skarg@users.sourceforge.net>
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
#include "hardware.h"
#include "mstp.h"

/* public port info */
extern volatile struct mstp_port_struct_t MSTP_Port;

static uint32_t RS485_Baud_Rate = 9600;

/* UART transmission buffer and index */
static volatile uint8_t RS485_Tx_Buffer[MAX_MPDU];
static volatile uint8_t RS485_Tx_Index = 0;
static volatile uint8_t RS485_Tx_Length = 0;
static volatile char RS485_Tx_Postdrive_Delay = 0;
static struct {
    unsigned TransmitStart:1;   /* TRUE if we are requested to transmit */
    unsigned TransmitComplete:1;        /* TRUE if we are finished transmitting frame */
} RS485_Flags;

/* Duplicate of the RCSTA reg used due to the double buffering of the */
/* fifo.  Reading the RCREG reg will cause the second RCSTA reg to be */
/* loaded if there is one. */
struct _rcstabits {
    unsigned char RX9D:1;
    unsigned char OERR:1;
    unsigned char FERR:1;
    unsigned char ADDEN:1;
    unsigned char CREN:1;
    unsigned char SREN:1;
    unsigned char RX9:1;
    unsigned char SPEN:1;
};

volatile static enum {
    RS485_STATE_IDLE = 0,
    RS485_STATE_RX_DATA = 1,
    RS485_STATE_RX_CHECKSUM = 2,
    RS485_STATE_RX_PROCESS = 3,
    RS485_STATE_TX_DATA = 4,
    RS485_STATE_WAIT_FOR_ACK = 5,
    RS485_STATE_WAIT_COMPLETE = 6,
    RS485_STATE_TX_GLOBAL_ACK = 7,
    RS485_STATE_TX_POSTDRIVE_DELAY = 8,
    RS485_STATE_ERROR = 9,
    RS485_STATE_RX_TEST = 10,
    RS485_STATE_RX_TEST_EEPROM = 11,
    RS485_STATE_RX_TEST_DELAY = 12,
    RS485_STATE_TX_TEST_WAIT = 13,
    RS485_STATE_TX_TEST = 14
} RS485_State;

/****************************************************************************
* DESCRIPTION: Transmits a frame using the UART
* RETURN:      none
* ALGORITHM:   none
* NOTES:       none
*****************************************************************************/
void RS485_Send_Frame(volatile struct mstp_port_struct_t *mstp_port,    /* port specific data */
    uint8_t * buffer,           /* frame to send (up to 501 bytes of data) */
    uint16_t nbytes)
{                               /* number of bytes of data (up to 501) */
    /* do we check for tx buffer in-use? */
    /* Or do we just stop the in-progress transmission? */
    /* Drop any transmission in progress, but don't worry about */
    /* cleaning up the hardware - the start routine will handle that */

    /* Disable the interrupt since it depends on the global transmit buffer. */
    USART_TX_INT_DISABLE();
    switch (RS485_State) {
    case RS485_STATE_TX_DATA:
    case RS485_STATE_WAIT_FOR_ACK:
    case RS485_STATE_WAIT_COMPLETE:
    case RS485_STATE_TX_GLOBAL_ACK:
        RS485_State = RS485_STATE_IDLE;
        break;
    }

    /* load the frame */
    RS485_Tx_Length = 0;
    while (buffer && nbytes) {
        RS485_Tx_Buffer[RS485_Tx_Length] = *buffer;
        buffer++;
        nbytes--;
        RS485_Tx_Length++;
        /* check bounds - should this error be indicated somehow? */
        /* perhaps not send the message? */
        if (RS485_Tx_Length >= MAX_MPDU)
            break;
    }
    /* signal the task to start sending when it is ready */
    RS485_Flags.TransmitStart = TRUE;

    return;
}

/****************************************************************************
* DESCRIPTION: Processes the next RS485 byte for transmit
* RETURN:      none
* ALGORITHM:   none
* NOTES:       Called by interrupt service routine (ISR)
*****************************************************************************/
void RS485_Transmit_Interrupt(void)
{
    uint8_t data;               /* data byte to send */

    switch (RS485_State) {
    case RS485_STATE_TX_DATA:
        RS485_Tx_Index++;
        if (RS485_Tx_Index < RS485_Tx_Length) {
            data = RS485_Tx_Buffer[RS485_Tx_Index];
            USART_TRANSMIT(data);
            MSTP_Port.SilenceTimer = 0;
        } else {
            /* wait until the last bit is sent */
            while (!USART_TX_EMPTY());
            RS485_TRANSMIT_DISABLE();
            /* wait 2 characters after sending (min=15 bit times) */
            RS485_Tx_Postdrive_Delay = 2;
            RS485_State = RS485_STATE_TX_POSTDRIVE_DELAY;
            USART_TRANSMIT(0);
        }
        break;
    case RS485_STATE_TX_POSTDRIVE_DELAY:
        /* after the message is sent, we wait a certain  */
        /* number of character times to get a delay */
        if (RS485_Tx_Postdrive_Delay) {
            RS485_Tx_Postdrive_Delay--;
            if (RS485_Tx_Postdrive_Delay == 0)
                RS485_State = RS485_STATE_WAIT_COMPLETE;
            USART_TRANSMIT(0);
        } else
            RS485_State = RS485_STATE_WAIT_COMPLETE;
        break;
    case RS485_STATE_WAIT_COMPLETE:
        /* wait until the last delay bit is shifted */
        while (!USART_TX_EMPTY());
        USART_TX_INT_DISABLE();
        RS485_Flags.TransmitComplete = TRUE;
        RS485_State = RS485_STATE_IDLE;
        USART_RX_SETUP();
        break;
    default:
        break;
    }

    return;
}

/****************************************************************************
* DESCRIPTION: Processes the RS485 message to be sent
* RETURN:      none
* ALGORITHM:   none
* NOTES:       none
*****************************************************************************/
void RS485_Process_Tx_Message(void)
{
    if (RS485_Flags.TransmitComplete)
        RS485_Flags.TransmitComplete = FALSE;
    /* start a new transmisstion if we are ready */
    if (RS485_Flags.TransmitStart && (RS485_State == RS485_STATE_IDLE)) {
        /* Disable the receiver */
        USART_RX_INT_DISABLE();
        USART_CONTINUOUS_RX_DISABLE();
        /* Enable the transmit line driver and interrupts */
        RS485_TRANSMIT_ENABLE();
        RS485_State = RS485_STATE_TX_DATA;
        /* Configure the ISR handler for an outgoing message */
        RS485_Tx_Index = 0;
        /* update the flags for beginning a send */
        RS485_Flags.TransmitComplete = FALSE;
        RS485_Flags.TransmitStart = FALSE;
        /* send the first byte */
        USART_TRANSMIT(RS485_Tx_Buffer[0]);
        USART_TX_SETUP();
    }

    return;
}

/****************************************************************************
* DESCRIPTION: Checks for data on the receive UART, and handles errors
* RETURN:      none
* ALGORITHM:   none
* NOTES:       none
*****************************************************************************/
void RS485_Check_UART_Data(volatile struct mstp_port_struct_t *mstp_port)
{
    struct _rcstabits rcstabits;        /* reading it more than once gets wrong data */

    /* check for data */
    if (USART_RX_COMPLETE()) {
        /* Read the data and the Rx status reg */
        rcstabits = USART_RX_STATUS();
        mstp_port->DataRegister = USART_RECEIVE();

        /* Check for buffer overrun error */
        if (rcstabits.OERR) {
            /* clear the error */
            USART_CONTINUOUS_RX_DISABLE();
            USART_CONTINUOUS_RX_ENABLE();
            /* let the state machine know */
            mstp_port->ReceiveError = TRUE;
        }
        /* Check for framing errors */
        else if (USART_RX_FRAME_ERROR()) {
            /* let the state machine know */
            mstp_port->FramingError = TRUE;
            mstp_port->ReceiveError = TRUE;
        }
        /* We read a good byte */
        else {
            /* state machine will clear this */
            mstp_port->DataAvailable = TRUE;
        }
    }

    return;
}

/****************************************************************************
* DESCRIPTION: Receives a data byte from the USART
* RETURN:      none
* ALGORITHM:   none
* NOTES:       none
*****************************************************************************/
void RS485_Receive_Interrupt(void)
{
    /* get as many bytes as we can get */
    for (;;) {
        RS485_Check_UART_Data(&MSTP_Port);
        if (MSTP_Port.ReceiveError || MSTP_Port.DataAvailable)
            MSTP_Receive_Frame_FSM(&MSTP_Port);
        else
            break;
    }

    return;
}

/****************************************************************************
* DESCRIPTION: Returns the baud rate that we are currently running at
* RETURN:      none
* ALGORITHM:   none
* NOTES:       none
*****************************************************************************/
uint32_t RS485_Get_Baud_Rate(void)
{
    return RS485_Baud_Rate;
}

/****************************************************************************
* DESCRIPTION: Sets the baud rate for the chip USART
* RETURN:      none
* ALGORITHM:   none
* NOTES:       none
*****************************************************************************/
void RS485_Set_Baud_Rate(uint32_t baud)
{
    if (baud < 19200)
        RS485_Baud_Rate = 9600;
    else if (baud < 38400)
        RS485_Baud_Rate = 19200;
    else if (baud < 57600)
        RS485_Baud_Rate = 38400;
    else if (baud < 57600)
        RS485_Baud_Rate = 57600;
    else if (baud < 115200)
        RS485_Baud_Rate = 76800;
    else
        RS485_Baud_Rate = 115200;

}

void RS485_Initialize_Baud(void)
{
    /* setup USART Baud Rate Generator */
    /* see BAUD RATES FOR ASYNCHRONOUS MODE in Data Book */
    /* Fosc=20MHz
       BRGH=1              BRGH=0
       Rate    SPBRG       Rate    SPBRG
       ------- -----       ------- -----
       9615   129          9469    32
       19230    64         19530    15
       37878    32         78130     3
       56818    21        104200     2
       113630    10        312500     0
       250000     4
       625000     1
       1250000     0
     */
    switch (RS485_Baud_Rate) {
    case 19200:
        SPBRG = 64;
        TXSTAbits.BRGH = 1;
        break;
    case 38400:
        SPBRG = 32;
        TXSTAbits.BRGH = 1;
        break;
    case 57600:
        SPBRG = 21;
        TXSTAbits.BRGH = 1;
        break;
    case 76800:
        SPBRG = 3;
        TXSTAbits.BRGH = 0;
        break;
    case 115200:
        SPBRG = 10;
        TXSTAbits.BRGH = 1;
        break;
    case 9600:
    default:
        SPBRG = 129;
        TXSTAbits.BRGH = 1;
        break;
    }
    /* select async mode */
    TXSTAbits.SYNC = 0;
    /* serial port enable */
    RCSTAbits.SPEN = 1;
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
    RS485_Initialize_Baud();
    /* configure interrupts */
    USART_TX_INT_DISABLE();
    USART_RX_INT_ENABLE();
    /* configure USART for receiving */
    /* since the TX will handle setting up for transmit */
    USART_CONTINUOUS_RX_ENABLE();
    /* since we are using RS485,
       we need to explicitly say
       transmit enable or not */
    RS485_TRANSMIT_DISABLE();
}
