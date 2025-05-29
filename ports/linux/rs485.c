/**************************************************************************
 *
 * Copyright (C) 2007 Steve Karg <skarg@users.sourceforge.net>
 * Updated by Nikola Jelic 2011 <nikola.jelic@euroicc.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later WITH GCC-exception-2.0
 *
 *********************************************************************/
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
#include <unistd.h>
#include <sched.h>
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

#include "dlmstp_port.h"

/* Posix serial programming reference:
http://www.easysw.com/~mike/serial/serial.html */

/* Use ionice wrapper to improve serial performance:
   $ sudo ionice -c 1 -n 0 ./bin/bacserv 12345
*/

/* handle returned from open() */
static int RS485_Handle = -1;
/* baudrate */
static unsigned int RS485_Baud = 38400;
/* serial port name, /dev/ttyS0,
  /dev/ttyUSB0 for USB->RS485 from B&B Electronics USOPTL4 */
static char *RS485_Port_Name = "/dev/ttyUSB0";
/* some terminal I/O have RS-485 specific functionality */
#ifndef RS485MOD
#define RS485MOD 0
#endif
/* serial I/O settings */
static struct termios2 RS485_oldtio2;

/* Ring buffer for incoming bytes, in order to speed up the receiving. */
static FIFO_BUFFER Rx_FIFO;
/* buffer size needs to be a power of 2 */
static uint8_t Rx_Buffer[4096];

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
    return RS485_Baud;
}

/****************************************************************************
 * DESCRIPTION: Returns the baud rate that we are currently running at
 * RETURN:      none
 * ALGORITHM:   none
 * NOTES:       none
 *****************************************************************************/
uint32_t RS485_Get_Port_Baud_Rate(struct mstp_port_struct_t *mstp_port)
{
    SHARED_MSTP_DATA *poSharedData = (SHARED_MSTP_DATA *)mstp_port->UserData;
    if (!poSharedData) {
        return 0;
    }
    return poSharedData->RS485_Baud;
}

/****************************************************************************
 * DESCRIPTION: Sets the baud rate for the chip USART
 * RETURN:      none
 * ALGORITHM:   none
 * NOTES:       none
 *****************************************************************************/
bool RS485_Set_Baud_Rate(uint32_t baud)
{
    RS485_Baud = baud;
    return true;
}

/****************************************************************************
 * DESCRIPTION: Gets RS485 config (e.g. automatic RTS for half-duplex direction)
 * RETURN:      true on success
 * ALGORITHM:   none
 * NOTES:       https://www.kernel.org/doc/Documentation/serial/serial-rs485.txt
 *****************************************************************************/
bool RS485_Get_Config(struct serial_rs485 *config)
{
    return ioctl(RS485_Handle, TIOCGRS485, config) == 0;
}

/****************************************************************************
 * DESCRIPTION: Sets RS485 config (e.g. automatic RTS for half-duplex direction)
 * RETURN:      true on success
 * ALGORITHM:   none
 * NOTES:       https://www.kernel.org/doc/Documentation/serial/serial-rs485.txt
 *****************************************************************************/
bool RS485_Set_Config(const struct serial_rs485 *const config)
{
    return ioctl(RS485_Handle, TIOCSRS485, config) == 0;
}

/****************************************************************************
 * DESCRIPTION: Transmit a frame on the wire
 * RETURN:      none
 * ALGORITHM:   none
 * NOTES:       none
 *****************************************************************************/
void RS485_Send_Frame(
    struct mstp_port_struct_t *mstp_port, /* port specific data */
    const uint8_t *buffer, /* frame to send (up to 501 bytes of data) */
    uint16_t nbytes /* number of bytes of data (up to 501) */)
{
    uint32_t turnaround_time_usec = Tturnaround * 1000000UL;
    uint32_t baud = RS485_Baud;
    int handle = RS485_Handle;
    ssize_t written = 0;
    int greska;
    const SHARED_MSTP_DATA *poSharedData = NULL;

    if (mstp_port && mstp_port->UserData) {
        poSharedData = (SHARED_MSTP_DATA *)mstp_port->UserData;
        baud = poSharedData->RS485_Baud;
        handle = poSharedData->RS485_Handle;
    }

    /* sleeping for turnaround time is necessary to give other devices
       time to change from sending to receiving state. */
    usleep(turnaround_time_usec / baud);
    /*
       On  success,  the  number of bytes written are returned (zero
       indicates nothing was written).  On error, -1  is  returned,  and
       errno  is  set appropriately.   If  count  is zero and the file
       descriptor refers to a regular file, 0 will be returned without
       causing any other effect.  For a special file, the results are not
       portable.
     */
    written = write(handle, buffer, nbytes);
    greska = errno;
    if (written <= 0) {
        printf("write error: %s\n", strerror(greska));
    } else {
        /* wait until all output has been transmitted. */
        termios2_tcdrain(handle);
    }

    /* per MSTP spec, sort of */
    if (mstp_port) {
        mstp_port->SilenceTimerReset((void *)mstp_port);
    }
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
    ssize_t n;
    int handle = RS485_Handle;
    FIFO_BUFFER *fifo = &Rx_FIFO;

    SHARED_MSTP_DATA *poSharedData = (SHARED_MSTP_DATA *)mstp_port->UserData;
    if (poSharedData) {
        handle = poSharedData->RS485_Handle;
        fifo = &poSharedData->Rx_FIFO;
    }

    if (mstp_port->ReceiveError == true) {
        /* do nothing but wait for state machine to clear the error */
        /* burning time, so wait a longer time */
        waiter.tv_sec = 0;
        waiter.tv_usec = 5000;
    } else if (mstp_port->DataAvailable == false) {
        /* wait for state machine to read from the DataRegister */
        if (FIFO_Count(fifo) > 0) {
            /* data is available */
            mstp_port->DataRegister = FIFO_Get(fifo);
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
    FD_SET(handle, &input);
    n = select(handle + 1, &input, NULL, NULL, &waiter);
    if (n < 0) {
        return;
    }
    if (FD_ISSET(handle, &input)) {
        n = read(handle, buf, sizeof(buf));
        FIFO_Add(fifo, &buf[0], n);
    }
}

void RS485_Cleanup(void)
{
    /* restore the old port settings */
    termios2_tcsetattr(RS485_Handle, TCSANOW, &RS485_oldtio2);
    close(RS485_Handle);
}

void RS485_Initialize(void)
{
    struct termios2 newtio;

#if PRINT_ENABLED
    fprintf(stdout, "RS485 Interface: %s\n", RS485_Port_Name);
#endif
    /*
       Open device for reading and writing.
       Blocking mode - more CPU effecient
     */
    RS485_Handle = open(RS485_Port_Name, O_RDWR | O_NOCTTY /*| O_NDELAY */);
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
    termios2_tcgetattr(RS485_Handle, &RS485_oldtio2);
    /* clear struct for new port settings */
    memset(&newtio, 0, sizeof(newtio));
    /*
       BOTHER: Set bps rate.
       https://man7.org/linux/man-pages/man2/TCSETS.2const.html
       CRTSCTS : output hardware flow control (only used if the cable has
       all necessary lines. See sect. 7 of Serial-HOWTO)
       CS8     : 8n1 (8bit,no parity,1 stopbit)
       CLOCAL  : local connection, no modem control
       CREAD   : enable receiving characters
     */
    newtio.c_cflag =
        CS8 | CLOCAL | CREAD | RS485MOD | BOTHER | (BOTHER << IBSHIFT);
    newtio.c_ispeed = RS485_Baud;
    newtio.c_ospeed = RS485_Baud;
    /* Raw input */
    newtio.c_iflag = 0;
    /* Raw output */
    newtio.c_oflag = 0;
    /* no processing */
    newtio.c_lflag = 0;
    /* activate the settings for the port after flushing I/O */
    termios2_tcsetattr(RS485_Handle, TCSAFLUSH, &newtio);
#if PRINT_ENABLED
    fprintf(stdout, "RS485 Baud Rate %u\n", RS485_Get_Baud_Rate());
    fflush(stdout);
#endif
    /* destructor */
    atexit(RS485_Cleanup);
    /* flush any data waiting */
    usleep(200000);
    termios2_tcflush(RS485_Handle, TCIOFLUSH);
    /* ringbuffer */
    FIFO_Init(&Rx_FIFO, Rx_Buffer, sizeof(Rx_Buffer));
}

/* Print in a format for Wireshark ExtCap */
void RS485_Print_Ports(void)
{
    int n;
    struct dirent **namelist;
    const char *sysdir = "/sys/class/tty/";
    struct stat st;
    char buffer[1024];
    char device_dir[1024];
    char *driver_name = NULL;
    int fd = 0;
    bool valid_port = false;
    struct serial_struct serinfo;

    /* Scan through /sys/class/tty -
       it contains all tty-devices in the system */
    n = scandir(sysdir, &namelist, NULL, NULL);
    if (n < 0) {
        perror("RS485: scandir");
    } else {
        while (n--) {
            if (strcmp(namelist[n]->d_name, "..") &&
                strcmp(namelist[n]->d_name, ".")) {
                snprintf(
                    device_dir, sizeof(device_dir), "%s%s/device", sysdir,
                    namelist[n]->d_name);
                /* Stat the devicedir and handle it if it is a symlink */
                if (lstat(device_dir, &st) == 0 && S_ISLNK(st.st_mode)) {
                    memset(buffer, 0, sizeof(buffer));
                    snprintf(
                        device_dir, sizeof(device_dir), "%s%s/device/driver",
                        sysdir, namelist[n]->d_name);
                    if (readlink(device_dir, buffer, sizeof(buffer)) > 0) {
                        valid_port = false;
                        driver_name = basename(buffer);
                        if (strcmp(driver_name, "serial8250") == 0) {
                            /* serial8250-devices must be probed */
                            snprintf(
                                device_dir, sizeof(device_dir), "/dev/%s",
                                namelist[n]->d_name);
                            fd = open(
                                device_dir, O_RDWR | O_NONBLOCK | O_NOCTTY);
                            if (fd >= 0) {
                                /* Get serial_info */
                                if (ioctl(fd, TIOCGSERIAL, &serinfo) == 0) {
                                    /* If device type is not PORT_UNKNOWN */
                                    /* we accept the port */
                                    if (serinfo.type != PORT_UNKNOWN) {
                                        valid_port = true;
                                    }
                                }
                                close(fd);
                            }
                        } else {
                            valid_port = true;
                        }
                        if (valid_port) {
                            /* print full absolute file path */
                            printf(
                                "interface {value=/dev/%s}"
                                "{display=MS/TP Capture on /dev/%s}\n",
                                namelist[n]->d_name, namelist[n]->d_name);
                        }
                    }
                }
            }
            free(namelist[n]);
        }
        free(namelist);
    }
}

int termios2_tcsetattr(
    const int fildes,
    int optional_actions,
    const struct termios2 *const termios2_p)
{
    /* https://man7.org/linux/man-pages/man2/TCSETS.2const.html */

    switch (optional_actions) {
        case TCSANOW:
            optional_actions = TCSETS2;
            break;
        case TCSADRAIN:
            optional_actions = TCSETSW2;
            break;
        case TCSAFLUSH:
            optional_actions = TCSETSF2;
            break;
        default:
            errno = EINVAL;
            return -1;
    };
    return ioctl(fildes, optional_actions, termios2_p);
}

int termios2_tcgetattr(const int fildes, struct termios2 *termios2_p)
{
    /* https://man7.org/linux/man-pages/man2/TCSETS.2const.html */
    return ioctl(fildes, TCGETS2, termios2_p);
}

int termios2_tcflush(const int fildes, const int queue_selector)
{
    /* https://manpages.opensuse.org/Tumbleweed/man-pages/TCFLSH.2const.en.html
     */
    return ioctl(fildes, TCFLSH, queue_selector);
}

int termios2_tcdrain(const int fildes)
{
    /*
    https://man7.org/linux/man-pages/man2/TCSBRK.2const.html
    TCSBRK Equivalent to tcsendbreak(fd, arg).
    Linux treats tcsendbreak(fd,arg) with nonzero arg like tcdrain(fd)
    */
    return ioctl(fildes, TCSBRK, 1);
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
