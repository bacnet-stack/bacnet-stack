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
/* Suggested USB to RS485 devices:
   B&B Electronics USOPTL4
   SerialGear USB-COMi-SI-M
*/

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include "mstp.h"

#define WIN32_LEAN_AND_MEAN
#define STRICT 1
#include <windows.h>

/* details from Serial Communications in Win32 at MSDN */

/* Win32 handle for the port */
HANDLE RS485_Handle;
/* Original COM Timeouts */
static COMMTIMEOUTS RS485_Timeouts;
/* COM port name COM1, COM2, etc  */
static char *RS485_Port_Name = "COM4";
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

/****************************************************************************
* DESCRIPTION: Initializes the RS485 hardware and variables, and starts in
*              receive mode.
* RETURN:      none
* ALGORITHM:   none
* NOTES:       none
*****************************************************************************/
void RS485_Set_Interface(
    char *ifname)
{
    /* note: expects a constant char, or char from the heap */
    if (ifname) {
        RS485_Port_Name = ifname;
    }
}

const char *RS485_Interface(void)
{
    return RS485_Port_Name;
}

static void RS485_Print_Error(
    void)
{
    char *szExtended = "";      /* error string translated from error code */
    DWORD dwExtSize;
    DWORD dwErr;

    dwErr = GetLastError();
    fprintf(stderr, "Error %lu:\r\n", dwErr);
    /* Get error string from system */
    dwExtSize =
        FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM |
        FORMAT_MESSAGE_ALLOCATE_BUFFER | 80, NULL, dwErr,
        MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US), (LPTSTR) szExtended, 0,
        NULL);
    fprintf(stderr, "%s\r\n", szExtended);

    return;
}

static void RS485_Configure_Status(
    void)
{
    DCB dcb = { 0 };
    COMMTIMEOUTS ctNew;


    dcb.DCBlength = sizeof(dcb);
    /* get current DCB settings */
    if (!GetCommState(RS485_Handle, &dcb)) {
        fprintf(stderr, "Unable to get status from %s\n", RS485_Port_Name);
        RS485_Print_Error();
        exit(1);
    }

    /* update DCB rate, byte size, parity, and stop bits size */
    dcb.BaudRate = RS485_Baud;
    dcb.ByteSize = (unsigned char) RS485_ByteSize;
    dcb.Parity = (unsigned char) RS485_Parity;
    dcb.StopBits = (unsigned char) RS485_StopBits;

    /* update flow control settings */
    dcb.fDtrControl = RS485_DTRControl;
    dcb.fRtsControl = RS485_RTSControl;
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
        fprintf(stderr, "Unable to set status on %s\n", RS485_Port_Name);
        RS485_Print_Error();
    }
    /* configure the COM port timeout values */
    ctNew.ReadIntervalTimeout = MAXDWORD;
    ctNew.ReadTotalTimeoutMultiplier = MAXDWORD;
    ctNew.ReadTotalTimeoutConstant = 1000;
    ctNew.WriteTotalTimeoutMultiplier = 0;
    ctNew.WriteTotalTimeoutConstant = 0;
    if (!SetCommTimeouts(RS485_Handle, &ctNew)) {
        RS485_Print_Error();
    }
    /* Get rid of any stray characters */
    if (!PurgeComm(RS485_Handle, PURGE_TXABORT | PURGE_RXABORT)) {
        fprintf(stderr, "Unable to purge %s\n", RS485_Port_Name);
        RS485_Print_Error();
    }
    /* Set the Comm buffer size */
    SetupComm(RS485_Handle, MAX_MPDU, MAX_MPDU);
    /* raise DTR */
    if (!EscapeCommFunction(RS485_Handle, SETDTR)) {
        fprintf(stderr, "Unable to set DTR on %s\n", RS485_Port_Name);
        RS485_Print_Error();
    }
}

/****************************************************************************
* DESCRIPTION: Initializes the RS485 hardware and variables, and starts in
*              receive mode.
* RETURN:      none
* ALGORITHM:   none
* NOTES:       none
*****************************************************************************/
void RS485_Initialize(
    void)
{
    RS485_Handle =
        CreateFile(RS485_Port_Name, GENERIC_READ | GENERIC_WRITE, 0, 0,
        OPEN_EXISTING,
        /*FILE_FLAG_OVERLAPPED */ 0,
        0);
    if (RS485_Handle == INVALID_HANDLE_VALUE) {
        fprintf(stderr, "Unable to open %s\n", RS485_Port_Name);
        RS485_Print_Error();
        exit(1);
    }
    if (!GetCommTimeouts(RS485_Handle, &RS485_Timeouts)) {
        RS485_Print_Error();
    }
    RS485_Configure_Status();

    return;
}

void RS485_Cleanup(
    void)
{
    if (!EscapeCommFunction(RS485_Handle, CLRDTR)) {
        RS485_Print_Error();
    }

    if (!SetCommTimeouts(RS485_Handle, &RS485_Timeouts)) {
        RS485_Print_Error();
    }

    CloseHandle(RS485_Handle);
}

/****************************************************************************
* DESCRIPTION: Returns the baud rate that we are currently running at
* RETURN:      none
* ALGORITHM:   none
* NOTES:       none
*****************************************************************************/
uint32_t RS485_Get_Baud_Rate(
    void)
{
    switch (RS485_Baud) {
        case CBR_19200:
            return 19200;
        case CBR_38400:
            return 38400;
        case CBR_57600:
            return 57600;
        case CBR_115200:
            return 115200;
        default:
        case CBR_9600:
            return 9600;
    }
}

/****************************************************************************
* DESCRIPTION: Sets the baud rate for the chip USART
* RETURN:      none
* ALGORITHM:   none
* NOTES:       none
*****************************************************************************/
bool RS485_Set_Baud_Rate(
    uint32_t baud)
{
    bool valid = true;

    switch (baud) {
        case 9600:
            RS485_Baud = CBR_9600;
            break;
        case 19200:
            RS485_Baud = CBR_19200;
            break;
        case 38400:
            RS485_Baud = CBR_38400;
            break;
        case 57600:
            RS485_Baud = CBR_57600;
            break;
        case 115200:
            RS485_Baud = CBR_115200;
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
void RS485_Send_Frame(
    volatile struct mstp_port_struct_t *mstp_port,      /* port specific data */
    uint8_t * buffer,   /* frame to send (up to 501 bytes of data) */
    uint16_t nbytes)
{       /* number of bytes of data (up to 501) */
    DWORD dwWritten = 0;

    if (mstp_port) {
        uint32_t baud;
        uint8_t turnaround_time;
        baud = RS485_Get_Baud_Rate();
        /* wait about 40 bit times since reception */
        if (baud == 9600)
            turnaround_time = 4;
        else if (baud == 19200)
            turnaround_time = 2;
        else
            turnaround_time = 1;
        while (mstp_port->SilenceTimer() < turnaround_time) {
            /* do nothing - wait for timer to increment */
        };
    }
    WriteFile(RS485_Handle, buffer, nbytes, &dwWritten, NULL);

    /* per MSTP spec, reset SilenceTimer after each byte is sent */
    if (mstp_port) {
        mstp_port->SilenceTimerReset();
    }

    return;
}

/* called by timer, interrupt(?) or other thread */
void RS485_Check_UART_Data(
    volatile struct mstp_port_struct_t *mstp_port)
{
    char lpBuf[1];
    DWORD dwRead = 0;

    if (mstp_port->ReceiveError == true) {
        /* wait for state machine to clear this */
    }
    /* wait for state machine to read from the DataRegister */
    else if (mstp_port->DataAvailable == false) {
        /* check for data */
        if (!ReadFile(RS485_Handle, lpBuf, sizeof(lpBuf), &dwRead, NULL)) {
            if (GetLastError() != ERROR_IO_PENDING) {
                mstp_port->ReceiveError = TRUE;
            }
        } else {
            if (dwRead) {
                mstp_port->DataRegister = lpBuf[0];
                mstp_port->DataAvailable = TRUE;
            }
        }
    }
}

#ifdef TEST_RS485
static void test_transmit_task(
    void *pArg)
{
    char *TxBuf = "BACnet MS/TP";
    size_t len = strlen(TxBuf) + 1;

    while (TRUE) {
        Sleep(1000);
        RS485_Send_Frame(NULL, &TxBuf[0], len);
    }
}

int main(
    void)
{
    unsigned long hThread = 0;
    uint32_t arg_value = 0;
    char lpBuf[1];
    DWORD dwRead = 0;
    unsigned i = 0;

    RS485_Set_Interface("COM4");
    RS485_Set_Baud_Rate(38400);
    RS485_Initialize();
#if 0
    /* create a task for synchronous transmit */
    hThread = _beginthread(test_transmit_task, 4096, &arg_value);
    if (hThread == 0) {
        fprintf(stderr, "Failed to start transmit task\n");
    }
#endif
    /* receive task */
    for (;;) {
        if (!ReadFile(RS485_Handle, lpBuf, sizeof(lpBuf), &dwRead, NULL)) {
            if (GetLastError() != ERROR_IO_PENDING) {
                RS485_Print_Error();
            }
        } else {
            /* print any characters received */
            if (dwRead) {
                for (i = 0; i < dwRead; i++) {
                    fprintf(stderr, "%02X ", lpBuf[i]);
                }
            }
            dwRead = 0;
        }
    }
}
#endif /* TEST_ABORT */
