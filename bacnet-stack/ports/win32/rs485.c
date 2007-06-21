/*####COPYRIGHTBEGIN####
 -------------------------------------------
 Copyright (C) 2004 Steve Karg

 This program is free software; you can redistribute it and/or
 modify it under the terms of the GNU General Public License
 as published by the Free Software Foundation; either version 2
 of the License, or (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program; if not, write to:
 The Free Software Foundation, Inc.
 59 Temple Place - Suite 330
 Boston, MA  02111-1307
 USA.

 As a special exception, if other files instantiate templates or
 use macros or inline functions from this file, or you compile
 this file and link it with other works to produce a work based
 on this file, this file does not by itself cause the resulting
 work to be covered by the GNU General Public License. However
 the source code for this file must still be made available in
 accordance with section (3) of the GNU General Public License.

 This exception does not invalidate any other reasons why a work
 based on this file might be covered by the GNU General Public
 License.
 -------------------------------------------
####COPYRIGHTEND####*/

/* The module handles sending data out the RS-485 port */
/* and handles receiving data from the RS-485 port. */
/* Customize this file for your specific hardware */
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include "mstp.h"

#define WIN32_LEAN_AND_MEAN
#define STRICT 1
#include <windows.h>

/* details from Serial Communications in Win32 at MSDN */

#define MAX_WRITE_BUFFER        1024
#define MAX_READ_BUFFER         2048

/* Win32 handle for the port */
static HANDLE RS485_Handle;
/* COM port name COM1, COM2, etc  */
static char *RS485_Port_Name = "COM1";
/* baud rate - MS enumerated 
    CBR_110, CBR_300, CBR_600, CBR_1200, CBR_2400,
    CBR_4800, CBR_9600, CBR_14400, CBR_19200, CBR_38400,
    CBR_56000, CBR_57600, CBR_115200, CBR_128000, CBR_256000 */
static DWORD RS485_Baud = CBR_38400;
/* ByteSize in bits: 5, 6, 7, 8 are valid */
static DWORD RS485_ByteSize = 8;
/* Parity - MS enumerated:
    NOPARITY, EVENPARITY, ODDPARITY, MARKPARITY, SPACEPARITY */
static DWORD RS485_Parity = NOPARITY;
/* StopBits - MS enumerated:
    ONESTOPBIT, ONE5STOPBITS, TWOSTOPBITS */
static DWORD RS485_StopBits = ONESTOPBIT;
/* DTRControl - MS enumerated:
    DTR_CONTROL_ENABLE, DTR_CONTROL_DISABLE, DTR_CONTROL_HANDSHAKE */
static DWORD RS485_DTRControl = DTR_CONTROL_DISABLE;
/* RTSControl - MS enumerated:
    RTS_CONTROL_ENABLE, RTS_CONTROL_DISABLE, 
    RTS_CONTROL_HANDSHAKE, RTS_CONTROL_TOGGLE */
static DWORD RS485_RTSControl = RTS_CONTROL_DISABLE;

/* FIXME: GetCommProperties? */

char *RS485_Port_Name(int port)
{
    switch (port) {
        case 2: return "COM2";
        case 3: return "COM3";
        case 4: return "COM4";
        case 5: return "COM5";
        case 6: return "COM6";
        case 7: return "COM7";
        case 8: return "COM8";
        case 9: return "COM9";
        default:
        case 1: return "COM1";
    }
}

int RS485_Port_Number(void)
{
    return RS485_Port;
}

void RS485_Set_Port_Number(int port)
{
    RS485_Port = port;
}

/****************************************************************************
* DESCRIPTION: Initializes the RS485 hardware and variables, and starts in
*              receive mode.
* RETURN:      none
* ALGORITHM:   none
* NOTES:       none
*****************************************************************************/
void RS485_Initialize_Port(void)
{
    DCB dcb = {0};

    RS485_Handle = CreateFile(
        RS485_Port_Name(RS485_Port),  
        GENERIC_READ | GENERIC_WRITE, 
        0, 
        0, 
        OPEN_EXISTING,
        FILE_FLAG_OVERLAPPED,
        0);
    if (RS485_Handle == INVALID_HANDLE_VALUE) {
        /* error opening port; abort */
        exit();
    }
    
    
    dcb.DCBlength = sizeof(dcb);
    /* get current DCB settings */
    if (!GetCommState(RS485_Handle, &dcb)) {
        /* FIXME: error message? */    
        exit();
    }

    /* update DCB rate, byte size, parity, and stop bits size */
    dcb.BaudRate = RS485_Baud_Rate;
    dcb.ByteSize = RS485_ByteSize;
    dcb.Parity   = RS485_Parity;
    dcb.StopBits = RS485_StopBits;

    /* update flow control settings */
    dcb.fDtrControl     = RS485_DTRControl;
    dcb.fRtsControl     = RS485_RTSControl;
    /*
    dcb.fOutxCtsFlow    = CTSOUTFLOW(TTYInfo);
    dcb.fOutxDsrFlow    = DSROUTFLOW(TTYInfo);
    dcb.fDsrSensitivity = DSRINFLOW(TTYInfo);
    dcb.fOutX           = XONXOFFOUTFLOW(TTYInfo);
    dcb.fInX            = XONXOFFINFLOW(TTYInfo);
    dcb.fTXContinueOnXoff = TXAFTERXOFFSENT(TTYInfo);
    dcb.XonChar         = XONCHAR(TTYInfo);
    dcb.XoffChar        = XOFFCHAR(TTYInfo);
    dcb.XonLim          = XONLIMIT(TTYInfo);
    dcb.XoffLim         = XOFFLIMIT(TTYInfo);
    // DCB settings not in the user's control
    dcb.fParity = TRUE;
    */
    if (!SetCommState(RS485_Handle, &dcb)) {
        /* FIXME: message? */
    }

    /*
    if (!SetCommTimeouts(COMDEV(TTYInfo), &(TIMEOUTSNEW(TTYInfo))))
	ErrorReporter("SetCommTimeouts");
    */

    if (!PurgeComm(RS485_Handle, PURGE_TXABORT | PURGE_RXABORT)) {
        /* FIXME: message? */
    }

    /* Set the Comm buffer size */    
    SetupComm(RS485_Handle, MAX_READ_BUFFER, MAX_WRITE_BUFFER);
    
    /* raise DTR */
    if (!EscapeCommFunction(RS485_Handle, SETDTR)) {
        /* FIXME: message? */
    }
    
    
    return TRUE;
}

/****************************************************************************
* DESCRIPTION: Returns the baud rate that we are currently running at
* RETURN:      none
* ALGORITHM:   none
* NOTES:       none
*****************************************************************************/
uint32_t RS485_Get_Baud_Rate(void)
{
    switch (RS485_Baud_Rate) {
        case CBR_9600: return 9600;
        case CBR_19200: return 19200;
        case CBR_38400: return 38400;
        case CBR_57600: return 57600;
        case CBR_115200: return 115200;
        default:
        case CBR_9600: return 9600;
    }
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
        RS485_Baud_Rate = CBR_9600;
        break;    
    case 19200:
        RS485_Baud_Rate = CBR_19200;
        break;    
    case 38400:
        RS485_Baud_Rate = CBR_38400;
        break;    
    case 57600:
        RS485_Baud_Rate = CBR_57600;
        break;    
    case 115200:
        RS485_Baud_Rate = CBR_115200;
        break;    
    default:
        valid = false;
        break;
    }

    if (valid) {
        /* FIXME: store the baud rate */
    }

    return valid;
}

/* Transmits a Frame on the wire */
void RS485_Send_Frame(struct mstp_port_struct_t *mstp_port,     /* port specific data */
    uint8_t * buffer,           /* frame to send (up to 501 bytes of data) */
    uint16_t nbytes)
{                               /* number of bytes of data (up to 501) */

    /* in order to avoid line contention */
    while (mstp_port->Turn_Around_Waiting) {
        /* wait, yield, or whatever */
    }

    /* Disable the receiver, and enable the transmit line driver. */

    while (nbytes) {
        putc(*buffer, stderr);
        buffer++;
        nbytes--;
    }

    /* Wait until the final stop bit of the most significant CRC octet  */
    /* has been transmitted but not more than Tpostdrive. */

    /* Disable the transmit line driver. */

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
        /* check for data */

        /* if error, */
        /* ReceiveError = TRUE; */
        /* return; */

        mstp_port->DataRegister = 0;    /* FIXME: Get this data from UART or buffer */

        /* if data is ready, */
        /* DataAvailable = TRUE; */
        /* return; */
    }
}

#ifdef TEST
#ifdef TEST_RS485
int main(void)
{
    char   	   lpBuf[AMOUNT_TO_READ];
    DWORD 	   dwRead;          // bytes actually read
    OVERLAPPED osReader = {0};  // overlapped structure for read operations

    RS485_Set_Port_Number("COM4");
    RS485_Set_Baud_Rate(38400);    
    RS485_Initialize_Port();


    for (;;) {
        if (!ReadFile(RS485_Handle, lpBuf, sizeof(lpBuf), &dwRead, &osReader)) {
            if (GetLastError() != ERROR_IO_PENDING) {
                /* error in comm */
            }
        } else {
            /* read completed immediately */
            if (dwRead) {
                for (i = 0; i < dwRead; i++) {
                    fprintf(stderr,"%02X ",lpBuf[i]);
                }
                fprintf(stderr,"\n");
            }
            dwRead = 0;
        }
    }
    
    return 0;
}
#endif                          /* TEST_ABORT */
#endif                          /* TEST */

