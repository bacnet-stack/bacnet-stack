/*####COPYRIGHTBEGIN####
 -------------------------------------------
 Copyright (C) 2007 Steve Karg <skarg@users.sourceforge.net>
 Updated by ?????? ????? 2011 <nikola.jelic@euroicc.com>

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

/** @file linux/rs485.c  Provides Linux-specific functions for RS-485 serial. */

/* The module handles sending data out the RS-485 port */
/* and handles receiving data from the RS-485 port. */
/* Customize this file for your specific hardware */
#include <errno.h>
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Linux includes */
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>
#include <sched.h>

/* Local includes */
#include "mstp.h"
#include "rs485.h"
#include "fifo.h"

#include <sys/select.h>
#include <sys/time.h>

/* Posix serial programming reference:
http://www.easysw.com/~mike/serial/serial.html */

/* Use ionice wrapper to improve serial performance:
   $ sudo ionice -c 1 -n 0 ./bin/bacserv 12345
*/

/* handle returned from open() */
static int RS485_Handle = -1;
/* baudrate settings are defined in <asm/termbits.h>, which is
   included by <termios.h> */
static unsigned int RS485_Baud = B38400;
/* serial port name, /dev/ttyS0,
  /dev/ttyUSB0 for USB->RS485 from B&B Electronics USOPTL4 */
static char *RS485_Port_Name = "/dev/ttyUSB0";
/* some terminal I/O have RS-485 specific functionality */
#ifndef RS485MOD
#define RS485MOD 0
#endif
/* serial I/O settings */
static struct termios RS485_oldtio;

/* Ring buffer for incoming bytes, in order to speed up the receiving. */
static FIFO_BUFFER Rx_FIFO;
/* buffer size needs to be a power of 2 */
static uint8_t Rx_Buffer[4096];

#define _POSIX_SOURCE 1 /* POSIX compliant source */

/*********************************************************************
* DESCRIPTION: Configures the interface name
* RETURN:      none
* ALGORITHM:   none
* NOTES:       none
*********************************************************************/
void RS485_Set_Interface(
    char *ifname)
{
    /* note: expects a constant char, or char from the heap */
    if (ifname) {
        RS485_Port_Name = ifname;
    }
}

/*********************************************************************
* DESCRIPTION: Returns the interface name
* RETURN:      none
* ALGORITHM:   none
* NOTES:       none
*********************************************************************/
const char *RS485_Interface(
    void)
{
    return RS485_Port_Name;
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
    uint32_t baud = 0;

    switch (RS485_Baud) {
        case B0:
            baud = 0;
            break;
        case B50:
            baud = 50;
            break;
        case B75:
            baud = 75;
            break;
        case B110:
            baud = 110;
            break;
        case B134:
            baud = 134;
            break;
        case B150:
            baud = 150;
            break;
        case B200:
            baud = 200;
            break;
        case B300:
            baud =300;
            break;
        case B600:
            baud = 600;
            break;
        case B1200:
            baud = 1200;
            break;
        case B1800:
            baud = 1800;
            break;
        case B2400:
            baud = 2400;
            break;
        case B4800:
            baud = 4800;
            break;
        case B9600:
            baud = 9600;
            break;
        case B19200:
            baud = 19200;
            break;
        case B38400:
            baud = 38400;
            break;
        case B57600:
            baud = 57600;
            break;
        case B115200:
            baud = 115200;
            break;
        case B230400:
            baud = 230400;
            break;
        default:
            baud = 9600;
    }

    return baud;
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
        case 0:
            RS485_Baud = B0;
            break;
        case 50:
            RS485_Baud = B50;
            break;
        case 75:
            RS485_Baud = B75;
            break;
        case 110:
            RS485_Baud = B110;
            break;
        case 134:
            RS485_Baud = B134;
            break;
        case 150:
            RS485_Baud = B150;
            break;
        case 200:
            RS485_Baud = B200;
            break;
        case 300:
            RS485_Baud = B300;
            break;
        case 600:
            RS485_Baud = B600;
            break;
        case 1200:
            RS485_Baud = B1200;
            break;
        case 1800:
            RS485_Baud = B1800;
            break;
        case 2400:
            RS485_Baud = B2400;
            break;
        case 4800:
            RS485_Baud = B4800;
            break;
        case 9600:
            RS485_Baud = B9600;
            break;
        case 19200:
            RS485_Baud = B19200;
            break;
        case 38400:
            RS485_Baud = B38400;
            break;
        case 57600:
            RS485_Baud = B57600;
            break;
        case 115200:
            RS485_Baud = B115200;
            break;
        case 230400:
            RS485_Baud = B230400;
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

/****************************************************************************
* DESCRIPTION: Transmit a frame on the wire
* RETURN:      none
* ALGORITHM:   none
* NOTES:       none
*****************************************************************************/
void RS485_Send_Frame(
    volatile struct mstp_port_struct_t *mstp_port,      /* port specific data */
    uint8_t * buffer,   /* frame to send (up to 501 bytes of data) */
    uint16_t nbytes)
{       /* number of bytes of data (up to 501) */
    uint32_t turnaround_time = Tturnaround * 1000;
    uint32_t baud = RS485_Get_Baud_Rate ();
    ssize_t written = 0;
    int greska;

    /* sleeping for turnaround time is necessary to give other devices 
       time to change from sending to receiving state. */
    usleep(turnaround_time/baud);
    /*
       On  success,  the  number of bytes written are returned (zero indicates
       nothing was written).  On error, -1  is  returned,  and  errno  is  set
       appropriately.   If  count  is zero and the file descriptor refers to a
       regular file, 0 will be returned without causing any other effect.  For
       a special file, the results are not portable.
     */
    written = write(RS485_Handle, buffer, nbytes);
    greska = errno;
    if (written <= 0) {
        printf("write error: %s\n", strerror(greska));
    } else {
        /* wait until all output has been transmitted. */
        tcdrain(RS485_Handle);
    }
    /*  tcdrain(RS485_Handle); */
    /* per MSTP spec, sort of */
    if (mstp_port) {
        mstp_port->SilenceTimerReset();
    }

    return;
}

/****************************************************************************
* DESCRIPTION: Get a byte of receive data
* RETURN:      none
* ALGORITHM:   none
* NOTES:       none
*****************************************************************************/
void RS485_Check_UART_Data(
    volatile struct mstp_port_struct_t *mstp_port)
{
    fd_set input;
    struct timeval waiter;
    uint8_t buf[2048];
    int n;

    if (mstp_port->ReceiveError == true) {
        /* do nothing but wait for state machine to clear the error */
        /* burning time, so wait a longer time */
        waiter.tv_sec = 0;
        waiter.tv_usec = 5000;
    } else if (mstp_port->DataAvailable == false) {
        /* wait for state machine to read from the DataRegister */
        if (FIFO_Count(&Rx_FIFO) > 0) {
            /* data is available */
            mstp_port->DataRegister = FIFO_Get(&Rx_FIFO);
            mstp_port->DataAvailable = true;
            /* FIFO is giving data - don't wait very long */
            waiter.tv_sec = 0;
            waiter.tv_usec = 10;
        } else {
            /* FIFO is empty - wait a longer time */
            waiter.tv_sec = 0;
            waiter.tv_usec = 5000;
        }
    }
    /* grab bytes and stuff them into the FIFO every time */
    FD_ZERO (&input);
    FD_SET (RS485_Handle, &input);
    n = select (RS485_Handle + 1, &input, NULL, NULL, &waiter);
    if (n < 0) {
        return;
    }
    if (FD_ISSET(RS485_Handle, &input)) {
        n = read(RS485_Handle, buf, sizeof(buf));
        FIFO_Add(&Rx_FIFO, &buf[0], n);
    }
}

void RS485_Cleanup(
    void)
{
    /* restore the old port settings */
    tcsetattr(RS485_Handle, TCSANOW, &RS485_oldtio);
    close(RS485_Handle);
}


void RS485_Initialize(
    void)
{
    struct termios newtio;
    printf("RS485: Initializing %s", RS485_Port_Name);
    /*
       Open device for reading and writing.
       Blocking mode - more CPU effecient
     */
    RS485_Handle = open(RS485_Port_Name, O_RDWR | O_NOCTTY /*| O_NDELAY */ );
    if (RS485_Handle < 0) {
        perror(RS485_Port_Name);
        exit(-1);
    }
#if 0
    /* non blocking for the read */
    fcntl(RS485_Handle, F_SETFL, FNDELAY);
#else
    /* efficient blocking for the read */
    fcntl(RS485_Handle, F_SETFL, 0);
#endif
    /* save current serial port settings */
    tcgetattr(RS485_Handle, &RS485_oldtio);
    /* clear struct for new port settings */
    bzero(&newtio, sizeof(newtio));
    /*
       BAUDRATE: Set bps rate. You could also use cfsetispeed and cfsetospeed.
       CRTSCTS : output hardware flow control (only used if the cable has
       all necessary lines. See sect. 7 of Serial-HOWTO)
       CS8     : 8n1 (8bit,no parity,1 stopbit)
       CLOCAL  : local connection, no modem contol
       CREAD   : enable receiving characters
     */
    newtio.c_cflag = RS485_Baud | CS8 | CLOCAL | CREAD | RS485MOD;
    /* Raw input */
    newtio.c_iflag = 0;
    /* Raw output */
    newtio.c_oflag = 0;
    /* no processing */
    newtio.c_lflag = 0;
    /* activate the settings for the port after flushing I/O */
    tcsetattr(RS485_Handle, TCSAFLUSH, &newtio);
    /* destructor */
    atexit(RS485_Cleanup);
    /* flush any data waiting */
    usleep(200000);
    tcflush(RS485_Handle, TCIOFLUSH);
    /* ringbuffer */
    FIFO_Init(&Rx_FIFO, Rx_Buffer, sizeof(Rx_Buffer));
    printf("=success!\n");
}

#ifdef TEST_RS485
#include <string.h>
int main(
    int argc,
    char *argv[])
{
    uint8_t buf[8];
    char *wbuf = { "BACnet!" };
    size_t wlen = strlen(wbuf) + 1;
    unsigned i = 0;
    size_t written = 0;
    int rlen;

    /* argv has the "/dev/ttyS0" or some other device */
    if (argc > 1) {
        RS485_Set_Interface(argv[1]);
    }
    RS485_Set_Baud_Rate(38400);
    RS485_Initialize();

    for (;;) {
        written = write(RS485_Handle, wbuf, wlen);
        rlen = read(RS485_Handle, buf, sizeof(buf));
        /* print any characters received */
        if (rlen > 0) {
            for (i = 0; i < rlen; i++) {
                fprintf(stderr, "%02X ", buf[i]);
            }
        }
    }

    return 0;
}
#endif
