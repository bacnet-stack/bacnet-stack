/*####COPYRIGHTBEGIN####
 -------------------------------------------
 Copyright (C) 2008 Steve Karg

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
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <time.h>
/* OS specific include*/
#include "net.h"
/* local includes */
#include "bytes.h"
#include "rs485.h"
#include "crc.h"
#include "mstptext.h"
#include "dlmstp.h"

#if defined(__BORLANDC__)
#include <sys/timeb.h>
#define _timeb timeb
#define _ftime(param) ftime(param)
#endif

#ifndef max
#define max(a,b) (((a) (b)) ? (a) : (b))
#define min(a,b) (((a) < (b)) ? (a) : (b))
#endif

/* local port data - shared with RS-485 */
static volatile struct mstp_port_struct_t MSTP_Port;
/* buffers needed by mstp port struct */
static uint8_t RxBuffer[MAX_MPDU];
static uint8_t TxBuffer[MAX_MPDU];
static uint16_t SilenceTime;
#define INCREMENT_AND_LIMIT_UINT16(x) {if (x < 0xFFFF) x++;}

#if defined (_WIN32)
struct timespec {
    time_t tv_sec;      /* Seconds */
    long tv_nsec;       /* Nanoseconds [0 .. 999999999] */
};

static int gettimeofday(
    struct timeval *tp,
    void *tzp)
{
    struct _timeb timebuffer;

	(void) tzp;

    _ftime(&timebuffer);
    tp->tv_sec = timebuffer.time;
    tp->tv_usec = timebuffer.millitm * 1000;

    return 0;
}
#endif

static uint16_t Timer_Silence(
    void)
{
    return SilenceTime;
}
static void Timer_Silence_Reset(
    void)
{
    SilenceTime = 0;
}

static void dlmstp_millisecond_timer(
    void)
{
    INCREMENT_AND_LIMIT_UINT16(SilenceTime);
}

#if !defined (_WIN32)
void Sleep(
    unsigned long milliseconds)
{
    struct timespec timeOut, remains;

    timeOut.tv_sec = milliseconds / 1000;
    timeOut.tv_nsec = (milliseconds - (timeOut.tv_sec * 1000)) * 10000000;
    nanosleep(&timeOut, &remains);
}
#endif

void *milliseconds_task(
    void *pArg)
{
	(void) pArg;

    for (;;) {
        Sleep(1);
        dlmstp_millisecond_timer();
    }

    return NULL;
}

#if defined(_WIN32)
/*************************************************************************
* Description: Timer task
* Returns: none
* Notes: none
*************************************************************************/
static void milliseconds_task_win32(
    void *pArg)
{
    milliseconds_task(pArg);
}
#endif

/* functions used by the MS/TP state machine to put or get data */
uint16_t MSTP_Put_Receive(
    volatile struct mstp_port_struct_t *mstp_port)
{
    (void) mstp_port;

    return 0;
}

/* for the MS/TP state machine to use for getting data to send */
/* Return: amount of PDU data */
uint16_t MSTP_Get_Send(
    volatile struct mstp_port_struct_t * mstp_port,
    unsigned timeout)
{       /* milliseconds to wait for a packet */
    (void) mstp_port;
    (void) timeout;
    return 0;
}

uint16_t MSTP_Get_Reply(
    volatile struct mstp_port_struct_t * mstp_port,
    unsigned timeout)
{       /* milliseconds to wait for a packet */
    (void) mstp_port;
    (void) timeout;
    return 0;
}

static char Capture_Filename[32] = "mstp_20090123091200.cap";
static FILE *pFile = NULL;      /* stream pointer */

static void filename_create(
    char *filename)
{
    time_t my_time;
    struct tm *today;

    if (filename) {
        my_time = time(NULL);
        today = localtime(&my_time);
        sprintf(filename, "mstp_%04d%02d%02d%02d%02d%02d.cap",
            1900 + today->tm_year, 1 + today->tm_mon, today->tm_mday,
            today->tm_hour, today->tm_min, today->tm_sec);
    }
}

/* write packet to file in libpcap format */
static void write_global_header(
    const char *filename)
{
    uint32_t magic_number = 0xa1b2c3d4; /* magic number */
    uint16_t version_major = 2; /* major version number */
    uint16_t version_minor = 4; /* minor version number */
    int32_t thiszone = 0;       /* GMT to local correction */
    uint32_t sigfigs = 0;       /* accuracy of timestamps */
    uint32_t snaplen = 65535;   /* max length of captured packets, in octets */
    uint32_t network = 165;     /* data link type - BACNET_MS_TP */

    /* create a new file. */
    pFile = fopen(filename, "wb");
    if (pFile) {
        (void) fwrite(&magic_number, sizeof(magic_number), 1, pFile);
        (void) fwrite(&version_major, sizeof(version_major), 1, pFile);
        (void) fwrite(&version_minor, sizeof(version_minor), 1, pFile);
        (void) fwrite(&thiszone, sizeof(thiszone), 1, pFile);
        (void) fwrite(&sigfigs, sizeof(sigfigs), 1, pFile);
        (void) fwrite(&snaplen, sizeof(snaplen), 1, pFile);
        (void) fwrite(&network, sizeof(network), 1, pFile);
        fflush(pFile);
        fprintf(stdout, "mstpcap: saving capture to %s\n", filename);
    } else {
        fprintf(stderr, "mstpcap: failed to open %s: %s\n", filename,
            strerror(errno));
    }
}

static void write_received_packet(
    volatile struct mstp_port_struct_t *mstp_port)
{
    uint32_t ts_sec;    /* timestamp seconds */
    uint32_t ts_usec;   /* timestamp microseconds */
    uint32_t incl_len;  /* number of octets of packet saved in file */
    uint32_t orig_len;  /* actual length of packet */
    uint8_t header[8];  /* MS/TP header */
    struct timeval tv;
    size_t max_data = 0;

    if (pFile) {
        gettimeofday(&tv, NULL);
        ts_sec = tv.tv_sec;
        ts_usec = tv.tv_usec;
        (void) fwrite(&ts_sec, sizeof(ts_sec), 1, pFile);
        (void) fwrite(&ts_usec, sizeof(ts_usec), 1, pFile);
        if (mstp_port->DataLength) {
            max_data = min(mstp_port->InputBufferSize, mstp_port->DataLength);
            incl_len = orig_len = 8 + max_data + 2;
        } else {
            incl_len = orig_len = 8;
        }
        (void) fwrite(&incl_len, sizeof(incl_len), 1, pFile);
        (void) fwrite(&orig_len, sizeof(orig_len), 1, pFile);
        header[0] = 0x55;
        header[1] = 0xFF;
        header[2] = mstp_port->FrameType;
        header[3] = mstp_port->DestinationAddress;
        header[4] = mstp_port->SourceAddress;
        header[5] = HI_BYTE(mstp_port->DataLength);
        header[6] = LO_BYTE(mstp_port->DataLength);
        header[7] = mstp_port->HeaderCRCActual;
        (void) fwrite(header, sizeof(header), 1, pFile);
        if (mstp_port->DataLength) {
            (void) fwrite(mstp_port->InputBuffer, max_data, 1, pFile);
            (void) fwrite((char *) &mstp_port->DataCRCActualMSB, 1, 1, pFile);
            (void) fwrite((char *) &mstp_port->DataCRCActualLSB, 1, 1, pFile);
        }
    } else {
        fprintf(stderr, "mstpcap: failed to open %s: %s\n", Capture_Filename,
            strerror(errno));
    }
}

static void cleanup(
    void)
{
    if (pFile) {
        fflush(pFile);  /* stream pointer */
        fclose(pFile);  /* stream pointer */
    }
    pFile = NULL;
}

#if (!defined(_WIN32))
static void sig_int(
    int signo)
{
    (void) signo;

    cleanup();
    exit(0);
}

void signal_init(
    void)
{
    signal(SIGINT, sig_int);
    signal(SIGHUP, sig_int);
    signal(SIGTERM, sig_int);
}
#endif

void filename_create_new(
    void)
{
    if (pFile) {
        fclose(pFile);
    }
    filename_create(&Capture_Filename[0]);
    write_global_header(&Capture_Filename[0]);
}

/* simple test to packetize the data and print it */
int main(
    int argc,
    char *argv[])
{
    volatile struct mstp_port_struct_t *mstp_port;
    long my_baud = 38400;
    uint32_t packet_count = 0;
#if defined(_WIN32)
    unsigned long hThread = 0;
    uint32_t arg_value = 0;
#else
    int rc = 0;
    pthread_t hThread;
#endif

    /* mimic our pointer in the state machine */
    mstp_port = &MSTP_Port;
    /* initialize our interface */
    if ((argc > 1) && (strcmp(argv[1], "--help") == 0)) {
        printf("mstpcap [interface] [baud]\r\n"
            "Captures MS/TP packets from a serial interface\r\n"
            "and save them to a file. Saves packets in a\r\n"
            "filename mstp_20090123091200.cap that has data and time.\r\n"
            "After receiving 65535 packets, a new file is created.\r\n" "\r\n"
            "Command line options:\r\n" "[interface] - serial interface.\r\n"
            "    defaults to COM4 on  Windows, and /dev/ttyUSB0 on linux.\r\n"
            "[baud] - baud rate.  9600, 19200, 38400, 57600, 115200\r\n"
            "    defaults to 38400.\r\n" "");
        return 0;
    }
    if (argc > 1) {
        RS485_Set_Interface(argv[1]);
    }
    if (argc > 2) {
        my_baud = strtol(argv[2], NULL, 0);
    }
    RS485_Set_Baud_Rate(my_baud);
    RS485_Initialize();
    MSTP_Port.InputBuffer = &RxBuffer[0];
    MSTP_Port.InputBufferSize = sizeof(RxBuffer);
    MSTP_Port.OutputBuffer = &TxBuffer[0];
    MSTP_Port.OutputBufferSize = sizeof(TxBuffer);
    MSTP_Port.This_Station = 127;
    MSTP_Port.Nmax_info_frames = 1;
    MSTP_Port.Nmax_master = 127;
    MSTP_Port.SilenceTimer = Timer_Silence;
    MSTP_Port.SilenceTimerReset = Timer_Silence_Reset;
    MSTP_Init(mstp_port);
    mstp_port->Lurking = true;
    fprintf(stdout, "mstpcap: Using %s for capture at %ld bps.\n",
        RS485_Interface(), (long) RS485_Get_Baud_Rate());
#if defined(_WIN32)
    hThread = _beginthread(milliseconds_task_win32, 4096, &arg_value);
    if (hThread == 0) {
        fprintf(stderr, "Failed to start timer task\n");
    }
    (void) SetThreadPriority(GetCurrentThread(),
        THREAD_PRIORITY_TIME_CRITICAL);
#else
    /* start our MilliSec task */
    rc = pthread_create(&hThread, NULL, milliseconds_task, NULL);
    signal_init();
#endif
    atexit(cleanup);
    filename_create_new();
    /* run forever */
    for (;;) {
        RS485_Check_UART_Data(mstp_port);
        MSTP_Receive_Frame_FSM(mstp_port);
        /* process the data portion of the frame */
        if (mstp_port->ReceivedValidFrame) {
            mstp_port->ReceivedValidFrame = false;
            write_received_packet(mstp_port);
            packet_count++;
        } else if (mstp_port->ReceivedInvalidFrame) {
            mstp_port->ReceivedInvalidFrame = false;
            fprintf(stderr, "ReceivedInvalidFrame\n");
            write_received_packet(mstp_port);
            packet_count++;
        }
        if (!(packet_count % 100)) {
            fprintf(stdout, "\r%hu packets", packet_count);
        }
        if (packet_count >= 65535) {
            filename_create_new();
            packet_count = 0;
        }
    }

    return 0;
}
