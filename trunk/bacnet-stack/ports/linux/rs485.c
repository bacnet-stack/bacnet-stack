/*####COPYRIGHTBEGIN####
 -------------------------------------------
 Copyright (C) 2007 Steve Karg

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
#include <stdlib.h>
#include <strings.h>

/* Linux includes */
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>
#include <sched.h>

/* Local includes */
#include "mstp.h"

/* handle returned from open() */
static int RS485_Handle = -1;
/* baudrate settings are defined in <asm/termbits.h>, which is
   included by <termios.h> */
static unsigned int RS485_Baud = B38400;
/* serial port name, /dev/ttyS0  */
static char *RS485_Port_Name = "/dev/ttyS0";
/* serial I/O settings */
static struct termios RS485_oldtio;

#define _POSIX_SOURCE 1 /* POSIX compliant source */

/*********************************************************************
* DESCRIPTION: Configures the interface name
* RETURN:      none
* ALGORITHM:   none
* NOTES:       none
*********************************************************************/
void RS485_Set_Interface(char *ifname)
{
    /* note: expects a constant char, or char from the heap */
    RS485_Port_Name = ifname;
}

/****************************************************************************
* DESCRIPTION: Returns the baud rate that we are currently running at
* RETURN:      none
* ALGORITHM:   none
* NOTES:       none
*****************************************************************************/
uint32_t RS485_Get_Baud_Rate(void)
{
    switch (RS485_Baud) {
        case B19200: return 19200;
        case B38400: return 38400;
        case B57600: return 57600;
        default:
            case B9600: return 9600;
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
            RS485_Baud = B9600;
            break;
        case 19200:
            RS485_Baud = B19200;
            break;
        case 38400:
            RS485_Baud = B38400;
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


void RS485_Initialize(void)
{
    struct termios newtio;

    /*
        Open device for reading and writing and not as controlling tty
        because we don't want to get killed if linenoise sends CTRL-C.
    */
    RS485_Handle = open(RS485_Port_Name, O_RDWR | O_NOCTTY );
    if (RS485_Handle < 0) {
        perror(RS485_Port_Name);
        exit(-1);
    }

    tcgetattr(RS485_Handle,&RS485_oldtio); /* save current serial port settings */
    bzero(&newtio, sizeof(newtio)); /* clear struct for new port settings */

    /*
        BAUDRATE: Set bps rate. You could also use cfsetispeed and cfsetospeed.
        CRTSCTS : output hardware flow control (only used if the cable has
        all necessary lines. See sect. 7 of Serial-HOWTO)
        CS8     : 8n1 (8bit,no parity,1 stopbit)
        CLOCAL  : local connection, no modem contol
        CREAD   : enable receiving characters
    */
    newtio.c_cflag = RS485_Baud | CRTSCTS | CS8 | CLOCAL | CREAD;


    /*
        IGNPAR  : ignore bytes with parity errors
        ICRNL   : map CR to NL (otherwise a CR input on the other computer
        will not terminate input)
        otherwise make device raw (no other input processing)
    */
    newtio.c_iflag = IGNPAR | ICRNL;
 
    /*
        Raw output.
    */
    newtio.c_oflag = 0;
 
    /*
        ICANON  : enable canonical input
        disable all echo functionality, and don't send signals to calling program
    */
    newtio.c_lflag = ICANON;
    /* 
        now clean the modem line and activate the settings for the port
    */
    tcflush(RS485_Handle, TCIFLUSH);
    tcsetattr(RS485_Handle,TCSANOW,&newtio);
}

/* Transmits a Frame on the wire */
void RS485_Send_Frame(
    struct mstp_port_struct_t *mstp_port, /* port specific data */
    uint8_t * buffer, /* frame to send (up to 501 bytes of data) */
    uint16_t nbytes) /* number of bytes of data (up to 501) */
{
    uint8_t turnaround_time;
    uint32_t baud;
    ssize_t written  = 0;

    if (mstp_port) {
        baud = RS485_Get_Baud_Rate();
        /* wait about 40 bit times since reception */
        if (baud == 9600)
            turnaround_time = 4;
        else if (baud == 19200)
            turnaround_time = 2;
        else
            turnaround_time = 1;
        while (mstp_port->SilenceTimer < turnaround_time) {
            /* do nothing - wait for timer to increment */
            sched_yield();
        };
    }
    /*
    On  success,  the  number of bytes written are returned (zero indicates
    nothing was written).  On error, -1  is  returned,  and  errno  is  set
    appropriately.   If  count  is zero and the file descriptor refers to a
    regular file, 0 will be returned without causing any other effect.  For
    a special file, the results are not portable.
    */
    written = write(RS485_Handle, buffer, nbytes);

    /* per MSTP spec, sort of */
    if (mstp_port) {
        mstp_port->SilenceTimer = 0;
    }

    return;
}

/* called by timer, interrupt(?) or other thread */
void RS485_Check_UART_Data(struct mstp_port_struct_t *mstp_port)
{
    uint8_t buf[1];
    int count;
    
    if (mstp_port->ReceiveError == true) {
        /* wait for state machine to clear this */
    }
    /* wait for state machine to read from the DataRegister */
    else if (mstp_port->DataAvailable == false) {
        /* check for data */
        count = read(RS485_Handle,buf,sizeof(buf));
        /* if error, */
        /* ReceiveError = TRUE; */
        /* return; */
        if (count) {
            mstp_port->DataRegister = buf[0];
            /* if data is ready, */
            mstp_port->DataAvailable = true;
        }
    }
}

#ifdef TEST_RS485
int main(void)
{
    uint8_t buf[8];
    unsigned i = 0;
    int res;

    RS485_Set_Interface("/dev/ttyS0");
    RS485_Set_Baud_Rate(38400);
    RS485_Initialize();

    for (;;) {
        res = read(RS485_Handle,buf,sizeof(buf));
        /* print any characters received */
        if (res) {
            for (i = 0; i < res; i++) {
                fprintf(stderr,"%02X ",buf[i]);
            }
        }
        res = 0;
    }
    /* restore the old port settings */
    tcsetattr(RS485_Handle,TCSANOW,&RS485_oldtio);

    return 0;
}
#endif
