/**************************************************************************
 *
 * Copyright (C) 2004 Steve Karg
 *
 * SPDX-License-Identifier: GPL-2.0-or-later WITH GCC-exception-2.0
 *
 *********************************************************************/
#ifndef PORTS_WIN32_RS485_H
#define PORTS_WIN32_RS485_H

#include <stdint.h>
#include "bacport.h"

struct bacnet_port_win32_rs485_t {
    /* Win32 handle for the port */
    HANDLE RS485_Handle;
    /* Original COM Timeouts */
    COMMTIMEOUTS RS485_Timeouts;
    /* COM port name COM1, COM2, etc  */
    char RS485_Port_Name[256] = "COM4";
    /* baud rate - MS enumerated
        CBR_110, CBR_300, CBR_600, CBR_1200, CBR_2400,
        CBR_4800, CBR_9600, CBR_14400, CBR_19200, CBR_38400,
        CBR_56000, CBR_57600, CBR_115200, CBR_128000, CBR_256000 */
    DWORD RS485_Baud = CBR_38400;
    /* ByteSize in bits: 5, 6, 7, 8 are valid */
    DWORD RS485_ByteSize = 8;
    /* Parity - MS enumerated:
        NOPARITY, EVENPARITY, ODDPARITY, MARKPARITY, SPACEPARITY */
    DWORD RS485_Parity = NOPARITY;
    /* StopBits - MS enumerated:
        ONESTOPBIT, ONE5STOPBITS, TWOSTOPBITS */
    DWORD RS485_StopBits = ONESTOPBIT;
    /* DTRControl - MS enumerated:
        DTR_CONTROL_ENABLE, DTR_CONTROL_DISABLE, DTR_CONTROL_HANDSHAKE */
    DWORD RS485_DTRControl = DTR_CONTROL_DISABLE;
    /* RTSControl - MS enumerated:
        RTS_CONTROL_ENABLE, RTS_CONTROL_DISABLE,
        RTS_CONTROL_HANDSHAKE, RTS_CONTROL_TOGGLE */
    DWORD RS485_RTSControl = RTS_CONTROL_DISABLE;
    /* track the last error */
    DWORD RS485_LastError = 0;
};

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

BACNET_STACK_EXPORT
void Port_RS485_Set_Interface(bacnet_port_win32_rs485_t *port, char *ifname);
BACNET_STACK_EXPORT
const char *Port_RS485_Interface(bacnet_port_win32_rs485_t *port);

BACNET_STACK_EXPORT
void Port_RS485_Initialize(bacnet_port_win32_rs485_t *port);
BACNET_STACK_EXPORT
void Port_RS485_Cleanup(bacnet_port_win32_rs485_t *port);

BACNET_STACK_EXPORT
unsigned Port_RS485_Send_Frame(
    bacnet_port_win32_rs485_t *port,
    const uint8_t *buffer,
    uint16_t nbytes);

BACNET_STACK_EXPORT
bool Port_RS485_Check_UART_Data(bacnet_port_win32_rs485_t *port, char *data);

BACNET_STACK_EXPORT
uint32_t Port_RS485_Get_Baud_Rate(bacnet_port_win32_rs485_t *port);
BACNET_STACK_EXPORT
bool Port_RS485_Set_Baud_Rate(bacnet_port_win32_rs485_t *port, uint32_t baud);

BACNET_STACK_EXPORT
void Port_RS485_Print_Error(bacnet_port_win32_rs485_t *port);

BACNET_STACK_EXPORT
bool Port_RS485_Interface_Valid(unsigned port_number);
BACNET_STACK_EXPORT
void Port_RS485_Print_Ports(void);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif
