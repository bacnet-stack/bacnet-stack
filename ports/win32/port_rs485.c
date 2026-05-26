/** @file win32/rs485.c  Provides Windows-specific functions for RS-485
 * @author Steve Karg
 * @date 2004
 * @copyright SPDX-License-Identifier: GPL-2.0-or-later WITH GCC-exception-2.0
 * @note Suggested USB to RS485 devices:
 *  B&B Electronics USOPTL4
 *  SerialGear USB-COMi-SI-M
 *  USB-RS485-WE-1800-BT
 */
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "bacnet/datalink/mstp.h"
#include "bacnet/datalink/dlmstp.h"
#define WIN32_LEAN_AND_MEAN
#define STRICT 1
#include <windows.h>
#include "port_rs485.h"
#include "bacnet/basic/sys/fifo.h"

/* note: details from Serial Communications in Win32 at MSDN */

/**
 * @brief Convert a string to uppercase in place.
 * @param str Pointer to the string to update.
 */
static void strupper(char *str)
{
    char *p;
    for (p = str; *p != '\0'; ++p) {
        *p = (char)toupper(*p);
    }
}

/**
 * @brief Set and normalize the serial interface name.
 * @param port Pointer to the RS-485 port context.
 * @param ifname Interface name string such as COM1.
 * @note For COM ports greater than 9, this stores the required
 * Windows device path format.
 */
void Port_RS485_Set_Interface(bacnet_port_win32_rs485_t *port, char *ifname)
{
    /* For COM ports greater than 9 you have to use a special syntax
       for CreateFileA. The syntax also works for COM ports 1-9. */
    /* http://support.microsoft.com/kb/115831 */
    if (ifname) {
        strupper(ifname);
        if (strncmp("COM", ifname, 3) == 0) {
            if (strlen(ifname) > 3) {
                snprintf(
                    port->RS485_Port_Name, sizeof(port->RS485_Port_Name),
                    "\\\\.\\COM%i", atoi(ifname + 3));
                fprintf(
                    stdout, "Adjusted interface name to %s\r\n",
                    port->RS485_Port_Name);
            }
        }
    }
}

/**
 * @brief Check the serial port to see if port exists
 * @param port_number The COM port number to check
 * @return true if port exists, false otherwise
 * @note This function attempts to open the COM port to determine its existence.
 */
bool Port_RS485_Interface_Valid(unsigned port_number)
{
    HANDLE h = 0;
    DWORD err = 0;
    bool status = false;
    char ifname[255] = "";

    snprintf(ifname, sizeof(ifname), "\\\\.\\COM%u", port_number);
    h = CreateFileA(
        ifname, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
    if (h == INVALID_HANDLE_VALUE) {
        err = GetLastError();
        if ((err == ERROR_ACCESS_DENIED) || (err == ERROR_GEN_FAILURE) ||
            (err == ERROR_SHARING_VIOLATION) || (err == ERROR_SEM_TIMEOUT)) {
            status = true;
        }
    } else {
        status = true;
        CloseHandle(h);
    }

    return status;
}

/**
 * @brief Get the name of the COM port interface
 * @param port The RS485 port structure
 * @return The name of the COM port interface
 */
const char *Port_RS485_Interface(bacnet_port_win32_rs485_t *port)
{
    return port->RS485_Port_Name;
}

/**
 * @brief Print the last error message related to the RS485 port
 * @param port The RS485 port structure
 * @note This function uses the Windows API to retrieve and display the last
 * error message.
 */
void Port_RS485_Print_Error(bacnet_port_win32_rs485_t *port)
{
    LPVOID lpMsgBuf;

    port->RS485_LastError = GetLastError();
    FormatMessage(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM, NULL,
        port->RS485_LastError, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        (LPTSTR)&lpMsgBuf, 0, NULL);
    MessageBox(NULL, lpMsgBuf, "GetLastError", MB_OK | MB_ICONINFORMATION);
    LocalFree(lpMsgBuf);

    return;
}

/**
 * @brief Apply line settings, flow control, and timeouts to the COM port.
 * @param port The RS485 port structure containing the desired settings.
 */
static void Port_RS485_Configure_Status(bacnet_port_win32_rs485_t *port)
{
    DCB dcb = { 0 };
    COMMTIMEOUTS ctNew;

    dcb.DCBlength = sizeof(dcb);
    /* get current DCB settings */
    if (!GetCommState(port->RS485_Handle, &dcb)) {
        fprintf(
            stderr, "Unable to get status from %s\n", port->RS485_Port_Name);
        Port_RS485_Print_Error(port);
        exit(1);
    }

    /* update DCB rate, byte size, parity, and stop bits size */
    dcb.BaudRate = port->RS485_Baud;
    dcb.ByteSize = (unsigned char)port->RS485_ByteSize;
    dcb.Parity = (unsigned char)port->RS485_Parity;
    dcb.StopBits = (unsigned char)port->RS485_StopBits;

    /* update flow control settings */
    dcb.fDtrControl = port->RS485_DTRControl;
    dcb.fRtsControl = port->RS485_RTSControl;
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
    if (!SetCommState(port->RS485_Handle, &dcb)) {
        fprintf(stderr, "Unable to set status on %s\n", port->RS485_Port_Name);
        Port_RS485_Print_Error(port);
    }
    /* configure the time-out parameters for a communications device. */
    /* If an application sets ReadIntervalTimeout and
       ReadTotalTimeoutMultiplier to MAXDWORD and
       sets ReadTotalTimeoutConstant to a value greater
       than zero and less than MAXDWORD, one of the following
       occurs when the ReadFile function is called:
        * If there are any bytes in the input buffer,
          ReadFile returns immediately with the bytes in the buffer.
        * If there are no bytes in the input buffer,
          ReadFile waits until a byte arrives and then returns immediately.
        * If no bytes arrive within the time specified
          by ReadTotalTimeoutConstant, ReadFile times out.

        Constant values are in milliseconds
     */
    ctNew.ReadIntervalTimeout = MAXDWORD;
    ctNew.ReadTotalTimeoutMultiplier = MAXDWORD;
    ctNew.ReadTotalTimeoutConstant = 1;
    ctNew.WriteTotalTimeoutMultiplier = 0;
    ctNew.WriteTotalTimeoutConstant = 0;
    if (!SetCommTimeouts(port->RS485_Handle, &ctNew)) {
        Port_RS485_Print_Error(port);
    }
    /* Get rid of any stray characters */
    if (!PurgeComm(port->RS485_Handle, PURGE_TXABORT | PURGE_RXABORT)) {
        fprintf(stderr, "Unable to purge %s\n", port->RS485_Port_Name);
        Port_RS485_Print_Error(port);
    }
    /* Set the Comm buffer size */
    SetupComm(port->RS485_Handle, DLMSTP_MPDU_MAX, DLMSTP_MPDU_MAX);
    /* raise DTR */
    if (!EscapeCommFunction(port->RS485_Handle, SETDTR)) {
        fprintf(stderr, "Unable to set DTR on %s\n", port->RS485_Port_Name);
        Port_RS485_Print_Error(port);
    }
}

/**
 * @brief Restore timeout settings and close the RS-485 handle.
 * @param port Pointer to the RS-485 port context.
 */
void Port_RS485_Cleanup(bacnet_port_win32_rs485_t *port)
{
    if (!EscapeCommFunction(port->RS485_Handle, CLRDTR)) {
        Port_RS485_Print_Error(port);
    }

    if (!SetCommTimeouts(port->RS485_Handle, &port->RS485_Timeouts)) {
        Port_RS485_Print_Error(port);
    }

    CloseHandle(port->RS485_Handle);
}

/**
 * @brief Open and initialize the RS-485 interface for use.
 * @param port Pointer to the RS-485 port context.
 */
void Port_RS485_Initialize(bacnet_port_win32_rs485_t *port)
{
    port->RS485_Handle = CreateFileA(
        RS485_Port_Name, GENERIC_READ | GENERIC_WRITE, 0, 0, OPEN_EXISTING,
        /*FILE_FLAG_OVERLAPPED */ 0, 0);
    if (port->RS485_Handle == INVALID_HANDLE_VALUE) {
        DWORD err = GetLastError();
        fprintf(
            stderr, "RS485 unable to open %s (Error %lu)\n", RS485_Port_Name,
            err);
        Port_RS485_Print_Error(port);
        exit(1);
    }
    if (!GetCommTimeouts(port->RS485_Handle, &port->RS485_Timeouts)) {
        Port_RS485_Print_Error(port);
    }
    Port_RS485_Configure_Status(port);
#if PRINT_ENABLED
    fprintf(stdout, "RS485 Interface: %s\n", RS485_Port_Name);
    fprintf(stdout, "RS485 Baud Rate %u\n", RS485_Get_Baud_Rate());
    fflush(stdout);
#endif

    return;
}

/**
 * @brief Get the configured RS-485 baud rate in bits per second.
 * @param port Pointer to the RS-485 port context.
 * @return Baud rate as an integer value.
 */
uint32_t Port_RS485_Get_Baud_Rate(bacnet_port_win32_rs485_t *port)
{
    switch (port->RS485_Baud) {
        case CBR_19200:
            return 19200;
        case CBR_38400:
            return 38400;
        case CBR_57600:
            return 57600;
        case CBR_115200:
            return 115200;
        case CBR_110:
            return 110;
        case CBR_300:
            return 300;
        case CBR_600:
            return 600;
        case CBR_1200:
            return 1200;
        case CBR_2400:
            return 2400;
        case CBR_4800:
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
        case CBR_110:
            return 110;
        case CBR_300:
            return 300;
        case CBR_600:
            return 600;
        case CBR_1200:
            return 1200;
        case CBR_2400:
            return 2400;
        case CBR_4800:
            return 4800;
        case CBR_14400:
            return 14400;
        case CBR_56000:
            return 56000;
        case CBR_128000:
            return 128000;
        case CBR_256000:
            return 256000;
        case 76800:
            /* See comments in RS485_Set_Baud_Rate() below
             * also look at definition of CBR_xx in winbase.h
             * some serial drivers will only support the defined
             * baud rates but others will try and configure the
             * requested baud rate (or as close as they can get)
             */
            return 76800;
        case CBR_9600:
        default:
            return 9600;
    }
}

/**
 * @brief Set the RS-485 baud rate.
 * @param port Pointer to the RS-485 port context.
 * @param baud Requested baud rate.
 * @return true if the rate is supported, false otherwise.
 */
bool Port_RS485_Set_Baud_Rate(bacnet_port_win32_rs485_t *port, uint32_t baud)
{
    bool valid = true;

    switch (baud) {
        case 9600:
            port->RS485_Baud = CBR_9600;
            break;
        case 19200:
            port->RS485_Baud = CBR_19200;
            break;
        case 38400:
            port->RS485_Baud = CBR_38400;
            break;
        case 57600:
            port->RS485_Baud = CBR_57600;
            break;
        case 115200:
            port->RS485_Baud = CBR_115200;
            break;
        case 110:
            port->RS485_Baud = CBR_110;
            break;
        case 300:
            port->RS485_Baud = CBR_300;
            break;
        case 600:
            port->RS485_Baud = CBR_600;
            break;
        case 1200:
            port->RS485_Baud = CBR_1200;
            break;
        case 2400:
            port->RS485_Baud = CBR_2400;
            break;
        case 4800:
            port->RS485_Baud = CBR_4800;
            break;
        case 14400:
            port->RS485_Baud = CBR_14400;
            break;
        case 56000:
            port->RS485_Baud = CBR_56000;
            break;
        case 128000:
            port->RS485_Baud = CBR_128000;
            break;
        case 256000:
            port->RS485_Baud = CBR_256000;
            break;
        case 76800:
            /* I'm using the B&B Electronics USOPTL4 USB RS485 adapter
             * on Win 7 and building with VS2008 Express Edition and it
             * seems to work for the most part if I use the following.
             * I get the occasional data errors especially if the devices
             * are transmitting with 1 stop bit (some devices receive with
             * 1 stop bit but effectivly end up transmitting with 2 stop
             * bits, usually because of synchroisation issues in some UARTs
             * which mean that if you wait until the serialiser has finished
             * with the current character and then load the TX buffer it has
             * to wait until the next bit boundary to start transmitting.
             * PMcS
             */
            port->RS485_Baud = 76800;
            break;
        default:
            valid = false;
            break;
    }

    return valid;
}

/**
 * @brief Transmit an MS/TP frame on the serial link.
 * @param port Pointer to the RS-485 port context.
 * @param buffer Frame bytes to transmit.
 * @param nbytes Number of bytes to transmit from buffer.
 * @return Number of bytes written to the serial device.
 */
unsigned RS485_Send_Frame(
    bacnet_port_win32_rs485_t *port,
    const uint8_t *buffer, /* frame to send (up to 501 bytes of data) */
    uint16_t nbytes)
{
    DWORD dwWritten = 0;

    WriteFile(port->RS485_Handle, buffer, nbytes, &dwWritten, NULL);

    return dwWritten;
}

/**
 * @brief Poll the serial port for one received byte.
 * @param port Pointer to the RS-485 port context.
 * @param data Output byte pointer when a character is available.
 * @return true if one byte was read, false otherwise.
 */
bool Port_RS485_Check_UART_Data(bacnet_port_win32_rs485_t *port, char *data)
{
    char lpBuf[1];
    DWORD dwRead = 0;
    bool status = false;

    /* check for data */
    if (!ReadFile(port->RS485_Handle, lpBuf, sizeof(lpBuf), &dwRead, NULL)) {
        if (GetLastError() != ERROR_IO_PENDING) {
            status = false;
        }
    } else {
        if (dwRead) {
            if (data) {
                *data = lpBuf[0];
            }
            status = true;
        }
    }
    return status;
}

/**
 * @brief Print detected COM ports in extcap-compatible format.
 */
void RS485_Print_Ports(void)
{
    unsigned i = 0;

    /* try to open all 255 COM ports */
    for (i = 1; i < 256; i++) {
        if (RS485_Interface_Valid(i)) {
            /* note: format for Wireshark ExtCap */
            printf(
                "interface {value=COM%u}"
                "{display=BACnet MS/TP on COM%u}\n",
                i, i);
        }
    }
}
