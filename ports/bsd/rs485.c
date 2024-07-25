/*####COPYRIGHTBEGIN####
 -------------------------------------------
 Copyright (C) 2007 Steve Karg <skarg@users.sourceforge.net>
 Updated by Nikola Jelic 2011 <nikola.jelic@euroicc.com>

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
#include <sys/utsname.h>
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>
#include <sched.h>

#include <math.h> /* for calculation of custom divisor */
#include <sys/ioctl.h>
/* for scandir */
#include <dirent.h>
/* for basename */
#include <libgen.h>

/* Local includes */
#include "bacnet/datalink/mstp.h"
#include "rs485.h"
#include "bacnet/basic/sys/fifo.h"

#include <sys/select.h>
#include <sys/time.h>

#include "dlmstp_bsd.h"

/*macOS-darwin includes*/
#include <IOKit/serial/ioss.h>

/* Posix serial programming reference-added by skarg below. Link updated by mpo 03/2024:
https://www.msweet.org/serial/serial.html */

/* Use ionice wrapper to improve serial performance:
  What is the source of this recommendation? -mpo 03/2024
   $ sudo ionice -c 1 -n 0 ./bin/bacserv 12345
*/

/* handle returned from open() */
static int RS485_Handle = -1;
/* baudrate settings are defined in <asm/termbits.h>, 
  which is included by <termios.h> 
  Instead of being an int enum which would be bad/brittle enough,
  they are #defines where the string label resolves to low consecutive
  ordinals depending on the platform
  e.g. on ESP, which doesn't appear to have native 76800 support:
  #define B38400     15
  #define B115200    17
  vs. Apple/Darwin's /usr/include/sys/termios.h:
  #define B76800  76800
  #define B115200 115200
  Anyways, a brittle, platform dependent way to have people thinking the underlying int
  might be the actual baud rate in bps.
  */
static unsigned int RS485_Baud = B115200;

// On macOS/Darwin the serial ports will be named something like
// /dev/cu.usbserial-xxxx
static char *RS485_Port_Name= "/dev/cu.usbserial-7";

/* some terminal I/O have RS-485 specific functionality */
#ifndef RS485MOD
#define RS485MOD 0
#endif
/* serial I/O settings */
// Hold the original termios attributes so they can be restored during cleanup
static struct termios RS485_oldtio;

/* indicator of special baud rate */
static bool RS485_SpecBaud = false;

/* Ring buffer for incoming bytes, in order to speed up the receiving. */
static FIFO_BUFFER Rx_FIFO;
/* buffer size needs to be a power of 2 */
static uint8_t Rx_Buffer[4096];

/**
 openSerialPort and closeSerialPort are both taken/adapted from
 Apple's own developer examples. Specifically, SerialPortSample
 Which can be found at 
 https://developer.apple.com/library/archive/samplecode/SerialPortSample/Introduction/Intro.html
 These in turn expect the Apple specific header IOKit/serial/ioss.h to be present
 They are well commented and  replace the meat of RS485_Initialize and RS485_Cleanup.
**/

static int openSerialPort(const char *bsdPath);
static void closeSerialPort(int fileDescriptor);

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
const char *RS485_Interface(void)
{
    return RS485_Port_Name;
}

/****************************************************************************
 * DESCRIPTION: Returns the baud rate that we are currently running at
 * RETURN:      none
 * ALGORITHM:   none
 * NOTES:       none
 *****************************************************************************/
uint32_t RS485_Get_Baud_Rate(void)
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
            baud = 300;
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
            if (!RS485_SpecBaud) {
                /* Linux asks for custom divisor
                   only when baud is set on 38400 */
                baud = 38400;
            } else {
                baud = 76800;
            }
            break;
        case B57600:
            baud = 57600;
            break;
#ifdef B76800
	 	case B76800:
	 		baud = 76800;
	 		break;
#endif
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
 * DESCRIPTION: Returns the baud rate that we are currently running at
 * RETURN:      none
 * ALGORITHM:   none
 * NOTES:       none
 *****************************************************************************/
uint32_t RS485_Get_Port_Baud_Rate(struct mstp_port_struct_t *mstp_port)
{
    uint32_t baud = 0;
    SHARED_MSTP_DATA *poSharedData = (SHARED_MSTP_DATA *)mstp_port->UserData;
    if (!poSharedData) {
        return 0;
    }
    switch (poSharedData->RS485_Baud) {
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
            baud = 300;
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
            break;
    }

    return baud;
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

    RS485_SpecBaud = false;
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
        case 76800:
#ifdef B76800
	 		RS485_Baud = B76800;
#else
	 		RS485_Baud = B38400;
     		RS485_SpecBaud = true;
#endif
           
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
    struct mstp_port_struct_t *mstp_port, /* port specific data */
    uint8_t *buffer, /* frame to send (up to 501 bytes of data) */
    uint16_t nbytes)
{ /* number of bytes of data (up to 501) */
    uint32_t turnaround_time = Tturnaround * 1000;
    uint32_t baud;
    ssize_t written = 0;
    int greska;
    SHARED_MSTP_DATA *poSharedData = NULL;

    if (mstp_port) {
        poSharedData = (SHARED_MSTP_DATA *)mstp_port->UserData;
    }
    if (!poSharedData) {
        baud = RS485_Get_Baud_Rate();
        /* sleeping for turnaround time is necessary to give other devices
           time to change from sending to receiving state. */
        usleep(turnaround_time / baud);
        /*
           On  success,  the  number of bytes written are returned (zero
           indicates nothing was written).  On error, -1  is  returned,  and
           errno  is  set appropriately.   If  count  is zero and the file
           descriptor refers to a regular file, 0 will be returned without
           causing any other effect.  For a special file, the results are not
           portable.
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
            mstp_port->SilenceTimerReset((void *)mstp_port);
        }
    } else {
        baud = RS485_Get_Port_Baud_Rate(mstp_port);
        /* sleeping for turnaround time is necessary to give other devices
           time to change from sending to receiving state. */
        usleep(turnaround_time / baud);
        /*
           On  success,  the  number of bytes written are returned (zero
           indicates nothing was written).  On error, -1  is  returned,  and
           errno  is  set appropriately.   If  count  is zero and the file
           descriptor refers to a regular file, 0 will be returned without
           causing any other effect.  For a special file, the results are not
           portable.
         */
        written = write(poSharedData->RS485_Handle, buffer, nbytes);
        greska = errno;
        if (written <= 0) {
            printf("write error: %s\n", strerror(greska));
        } else {
            /* wait until all output has been transmitted. */
            tcdrain(poSharedData->RS485_Handle);
        }
        /*  tcdrain(RS485_Handle); */
        /* per MSTP spec, sort of */
        if (mstp_port) {
            mstp_port->SilenceTimerReset((void *)mstp_port);
        }
    }

    return;
}

/****************************************************************************
 * DESCRIPTION: Get a byte of receive data
 * RETURN:      none
 * ALGORITHM:   none
 * NOTES:       none
 *****************************************************************************/
void RS485_Check_UART_Data(struct mstp_port_struct_t *mstp_port)
{
    fd_set input;
    struct timeval waiter;
    uint8_t buf[2048];
    int n;

    SHARED_MSTP_DATA *poSharedData = (SHARED_MSTP_DATA *)mstp_port->UserData;
    if (!poSharedData) {
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
                /* FIFO is giving data - just poll */
                waiter.tv_sec = 0;
                waiter.tv_usec = 0;
            } else {
                /* FIFO is empty - wait a longer time */
                waiter.tv_sec = 0;
                waiter.tv_usec = 5000;
            }
        }
        /* grab bytes and stuff them into the FIFO every time */
        FD_ZERO(&input);
        FD_SET(RS485_Handle, &input);
        n = select(RS485_Handle + 1, &input, NULL, NULL, &waiter);
        if (n < 0) {
            return;
        }
        if (FD_ISSET(RS485_Handle, &input)) {
            n = read(RS485_Handle, buf, sizeof(buf));
            FIFO_Add(&Rx_FIFO, &buf[0], n);
        }
    } else {
        if (mstp_port->ReceiveError == true) {
            /* do nothing but wait for state machine to clear the error */
            /* burning time, so wait a longer time */
            waiter.tv_sec = 0;
            waiter.tv_usec = 5000;
        } else if (mstp_port->DataAvailable == false) {
            /* wait for state machine to read from the DataRegister */
            if (FIFO_Count(&poSharedData->Rx_FIFO) > 0) {
                /* data is available */
                mstp_port->DataRegister = FIFO_Get(&poSharedData->Rx_FIFO);
                mstp_port->DataAvailable = true;
                /* FIFO is giving data - just poll */
                waiter.tv_sec = 0;
                waiter.tv_usec = 0;
            } else {
                /* FIFO is empty - wait a longer time */
                waiter.tv_sec = 0;
                waiter.tv_usec = 5000;
            }
        }
        /* grab bytes and stuff them into the FIFO every time */
        FD_ZERO(&input);
        FD_SET(poSharedData->RS485_Handle, &input);
        n = select(poSharedData->RS485_Handle + 1, &input, NULL, NULL, &waiter);
        if (n < 0) {
            return;
        }
        if (FD_ISSET(poSharedData->RS485_Handle, &input)) {
            n = read(poSharedData->RS485_Handle, buf, sizeof(buf));
            FIFO_Add(&poSharedData->Rx_FIFO, &buf[0], n);
        }
    }
}

void RS485_Cleanup(void)
{
    closeSerialPort(RS485_Handle);
}

/*Trying to be POSIX-ish. Adapting from 
https://github.com/stephane/libmodbus/blob/master/src/modbus-rtu.c*/
void RS485_Initialize(void)
{
    RS485_Handle = openSerialPort(RS485_Port_Name);
    /* ringbuffer */
    FIFO_Init(&Rx_FIFO, Rx_Buffer, sizeof(Rx_Buffer));
}

/* Print in a format for Wireshark ExtCap */
void RS485_Print_Ports(void)
{
    int n;
    struct dirent **namelist;
    const char* sysdir = "/sys/class/tty/";
    struct stat st;
    char buffer[1024];
    char device_dir[1024];
    char *driver_name = NULL;
    int fd = 0;
    bool valid_port = false;
	struct utsname unameData;
	bool macOS = false;
    if (uname(&unameData) < 0) {
       perror("uname");
       exit(EXIT_FAILURE);
    }
    if(strcmp(unameData.sysname,"Darwin") == 0){
    	macOS = true;
    	sysdir = "/dev/";
    }
    /* Scan through /sys/class/tty -
       it contains all tty-devices in the system */
    n = scandir(sysdir, &namelist, NULL, NULL);
    if (n < 0) {
        perror("RS485: scandir");
    } else {
        while (n--) {
        if (strcmp(namelist[n]->d_name, "..") &&
                	strcmp(namelist[n]->d_name, ".")) {
        	if(macOS){
                	if(strncmp(namelist[n]->d_name, "cu.",3) == 0){
                	    printf("%s%s\n", sysdir, namelist[n]->d_name);
                	    valid_port = true;
                	    if (valid_port) {
                	        /* print full absolute file path */
                            printf("interface {value=/dev/%s}"
                                "{display=MS/TP Capture on /dev/%s}\n",
                                namelist[n]->d_name, namelist[n]->d_name);
                        }
                	}
        	}else{
                snprintf(device_dir, sizeof(device_dir), "%s%s/device", sysdir,
                    namelist[n]->d_name);
                /* Stat the devicedir and handle it if it is a symlink */
                if (lstat(device_dir, &st) == 0 && S_ISLNK(st.st_mode)) {
                    memset(buffer, 0, sizeof(buffer));
                    snprintf(device_dir, sizeof(device_dir),
                        "%s%s/device/driver", sysdir, namelist[n]->d_name);
                    if (readlink(device_dir, buffer, sizeof(buffer)) > 0) {
                        valid_port = false;
                        driver_name = basename(buffer);
                        if (strcmp(driver_name, "serial8250") == 0) {
                            /* serial8250-devices must be probed */
                            snprintf(device_dir, sizeof(device_dir), "/dev/%s",
                                namelist[n]->d_name);
                            fd = open(
                                device_dir, O_RDWR | O_NONBLOCK | O_NOCTTY);
                            if (fd >= 0) {
                                //Failed
                                close(fd);
                            }
                        } else {
                            valid_port = true;
                        }
                        if (valid_port) {
                            /* print full absolute file path */
                            printf("interface {value=/dev/%s}"
                                   "{display=MS/TP Capture on /dev/%s}\n",
                                namelist[n]->d_name, namelist[n]->d_name);
                        }
                    }
                }
            }
            free(namelist[n]);
        }
        }
        free(namelist);
    }
}

// Given the path to a serial device, open the device and configure it.
// Return the file descriptor associated with the device.
static int openSerialPort(const char* const bsdPath)
{
    int				fileDescriptor = -1;
    int				handshake;
    struct termios	options;
    
    // Open the serial port read/write, with no controlling terminal, and don't wait for a connection.
    // The O_NONBLOCK flag also causes subsequent I/O on the device to be non-blocking.
    // See open(2) <x-man-page://2/open> for details.
    
    fileDescriptor = open(bsdPath, O_RDWR | O_NOCTTY | O_NONBLOCK);
    if (fileDescriptor == -1) {
        printf("Error opening serial port %s - %s(%d).\n",
               bsdPath, strerror(errno), errno);
        goto error;
    }
    
    // Note that open() follows POSIX semantics: multiple open() calls to the same file will succeed
    // unless the TIOCEXCL ioctl is issued. This will prevent additional opens except by root-owned
    // processes.
    // See tty(4) <x-man-page//4/tty> and ioctl(2) <x-man-page//2/ioctl> for details.
    
    if (ioctl(fileDescriptor, TIOCEXCL) == -1) {
        printf("Error setting TIOCEXCL on %s - %s(%d).\n",
               bsdPath, strerror(errno), errno);
        goto error;
    }
    
    // Now that the device is open, clear the O_NONBLOCK flag so subsequent I/O will block.
    // See fcntl(2) <x-man-page//2/fcntl> for details.
    
    if (fcntl(fileDescriptor, F_SETFL, 0) == -1) {
        printf("Error clearing O_NONBLOCK %s - %s(%d).\n",
               bsdPath, strerror(errno), errno);
        goto error;
    }
    
    // Get the current options and save them so we can restore the default settings later.
    if (tcgetattr(fileDescriptor, &RS485_oldtio) == -1) {
        printf("Error getting tty attributes %s - %s(%d).\n",
               bsdPath, strerror(errno), errno);
        goto error;
    }
    
    // The serial port attributes such as timeouts and baud rate are set by modifying the termios
    // structure and then calling tcsetattr() to cause the changes to take effect. Note that the
    // changes will not become effective without the tcsetattr() call.
    // See tcsetattr(4) <x-man-page://4/tcsetattr> for details.
    
    options = RS485_oldtio;
    
    // Print the current input and output baud rates.
    // See tcsetattr(4) <x-man-page://4/tcsetattr> for details.
    
    printf("Default/current input baud rate is %d\n", (int) cfgetispeed(&options));
    printf("Default/current output baud rate is %d\n", (int) cfgetospeed(&options));
    
    // Set raw input (non-canonical) mode, with reads blocking until either a single character
    // has been received or a one second timeout expires.
    // See tcsetattr(4) <x-man-page://4/tcsetattr> and termios(4) <x-man-page://4/termios> for details.
    
    cfmakeraw(&options);
    options.c_cc[VMIN] = 0;
    options.c_cc[VTIME] = 10;
    
    // The baud rate, word length, and handshake options can be set as follows:
    cfsetspeed(&options, RS485_Baud);		// Set 115200 baud
    options.c_cflag &= ~PARENB;         // No Parity
	options.c_cflag &= ~CSTOPB;     	// 1 Stop Bit
	options.c_cflag &= ~CSIZE;
	options.c_cflag |= CS8;  		    // Use 8 bit words
	// The IOSSIOSPEED ioctl can be used to set arbitrary baud rates
	// other than those specified by POSIX. The driver for the underlying serial hardware
	// ultimately determines which baud rates can be used. This ioctl sets both the input
	// and output speed.
	
	speed_t speed = RS485_Get_Baud_Rate();//
    if (ioctl(fileDescriptor, IOSSIOSPEED, &speed) == -1) {
        printf("Error calling ioctl(..., IOSSIOSPEED, ...) %s - %s(%d).\n",
               bsdPath, strerror(errno), errno);
    }
    
    // Print the new input and output baud rates. Note that the IOSSIOSPEED ioctl interacts with the serial driver
	// directly, bypassing the termios struct. This means that the following two calls will not be able to read
	// the current baud rate if the IOSSIOSPEED ioctl was used but will instead return the speed set by the last call
	// to cfsetspeed.
    
    printf("Input baud rate changed to %d\n", (int) cfgetispeed(&options));
    printf("Output baud rate changed to %d\n", (int) cfgetospeed(&options));
    
    // Cause the new options to take effect immediately.
    if (tcsetattr(fileDescriptor, TCSANOW, &options) == -1) {
        printf("Error setting tty attributes %s - %s(%d).\n",
               bsdPath, strerror(errno), errno);
        goto error;
    }
    
    // To set the modem handshake lines, use the following ioctls.
    // See tty(4) <x-man-page//4/tty> and ioctl(2) <x-man-page//2/ioctl> for details.
    
    // Assert Data Terminal Ready (DTR)
    if (ioctl(fileDescriptor, TIOCSDTR) == -1) {
        printf("Error asserting DTR %s - %s(%d).\n",
               bsdPath, strerror(errno), errno);
    }
    
    // Clear Data Terminal Ready (DTR)
    if (ioctl(fileDescriptor, TIOCCDTR) == -1) {
        printf("Error clearing DTR %s - %s(%d).\n",
               bsdPath, strerror(errno), errno);
    }
    
    // Set the modem lines depending on the bits set in handshake
    handshake = TIOCM_DTR | TIOCM_RTS | TIOCM_CTS | TIOCM_DSR;
    if (ioctl(fileDescriptor, TIOCMSET, &handshake) == -1) {
        printf("Error setting handshake lines %s - %s(%d).\n",
               bsdPath, strerror(errno), errno);
    }
    
    // To read the state of the modem lines, use the following ioctl.
    // See tty(4) <x-man-page//4/tty> and ioctl(2) <x-man-page//2/ioctl> for details.
    
    // Store the state of the modem lines in handshake
    if (ioctl(fileDescriptor, TIOCMGET, &handshake) == -1) {
        printf("Error getting handshake lines %s - %s(%d).\n",
               bsdPath, strerror(errno), errno);
    }
    
    printf("Handshake lines currently set to %d\n", handshake);
	
	unsigned long mics = 1UL;
    
	// Set the receive latency in microseconds. Serial drivers use this value to determine how often to
	// dequeue characters received by the hardware. Most applications don't need to set this value: if an
	// app reads lines of characters, the app can't do anything until the line termination character has been
	// received anyway. The most common applications which are sensitive to read latency are MIDI and IrDA
	// applications.
	
	if (ioctl(fileDescriptor, IOSSDATALAT, &mics) == -1) {
		// set latency to 1 microsecond
        printf("Error setting read latency %s - %s(%d).\n",
               bsdPath, strerror(errno), errno);
        goto error;
	}
    
    // Success
    return fileDescriptor;
    
    // Failure path
error:
    if (fileDescriptor != -1) {
        close(fileDescriptor);
    }
    
    return -1;
}

// Given the file descriptor for a serial device, close that device.
static void closeSerialPort(int fileDescriptor)
{
    // Block until all written output has been sent from the device.
    // Note that this call is simply passed on to the serial device driver.
	// See tcsendbreak(3) <x-man-page://3/tcsendbreak> for details.
    if (tcdrain(fileDescriptor) == -1) {
        printf("Error waiting for drain - %s(%d).\n",
               strerror(errno), errno);
    }
    
    // Traditionally it is good practice to reset a serial port back to
    // the state in which you found it. This is why the original termios struct
    // was saved.
    if (tcsetattr(fileDescriptor, TCSANOW, &RS485_oldtio) == -1) {
        printf("Error resetting tty attributes - %s(%d).\n",
               strerror(errno), errno);
    }
    
    close(fileDescriptor);
}

#ifdef TEST_RS485
#include <string.h>
int main(int argc, char *argv[])
{
    struct mstp_port_struct_t mstp_port = { 0 };
    uint8_t token_buf[8] = { 0x55, 0xFF, 0x00, 0x7E, 0x07, 0x00, 0x00, 0xFD };
    uint8_t pfm_buf[8] = { 0x55, 0xFF, 0x01, 0x67, 0x07, 0x00, 0x00, 0x3E };
    long baud = 38400;
    bool write_token = false;
    bool write_pfm = false;

    /* argv has the "/dev/ttyS0" or some other device */
    if (argc > 1) {
        RS485_Set_Interface(argv[1]);
    }
    if (argc > 2) {
        baud = strtol(argv[2], NULL, 0);
    }
    if (argc > 3) {
        if (strcmp("token", argv[3]) == 0) {
            write_token = true;
        }
        if (strcmp("pfm", argv[3]) == 0) {
            write_pfm = true;
        }
    }
    RS485_Set_Baud_Rate(baud);
    RS485_Initialize();
    for (;;) {
        if (write_token) {
            RS485_Send_Frame(NULL, token_buf, sizeof(token_buf));
            usleep(25000);
        } else if (write_pfm) {
            RS485_Send_Frame(NULL, pfm_buf, sizeof(pfm_buf));
            usleep(100000);
        } else {
            RS485_Check_UART_Data(&mstp_port);
            if (mstp_port.DataAvailable) {
                fprintf(stderr, "%02X ", mstp_port.DataRegister);
                mstp_port.DataAvailable = false;
            }
        }
    }

    return 0;
}
#endif
