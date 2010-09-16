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
#include "timer.h"
/* local includes */
#include "bytes.h"
#include "rs485.h"
#include "crc.h"
#include "mstptext.h"
#include "dlmstp.h"

#ifndef max
#define max(a,b) (((a) (b)) ? (a) : (b))
#define min(a,b) (((a) < (b)) ? (a) : (b))
#endif

/* local port data - shared with RS-485 */
static volatile struct mstp_port_struct_t MSTP_Port;
/* buffers needed by mstp port struct */
static uint8_t RxBuffer[MAX_MPDU];
static uint8_t TxBuffer[MAX_MPDU];

/* statistics derived from monitoring the network for each node */
struct mstp_statistics {
    /* counts how many times the node passes the token */
    uint32_t token_count;
    /* counts how many times the node passes the token */
    uint32_t pfm_count;
    /* counts how many times the node gets a second token */
    uint32_t token_retries;
    /* delay after poll for master */
    uint32_t tusage_timeout;
    /* highest number MAC during poll for master */
    uint8_t max_master;
    /* how long it takes a node to pass the token */
    uint32_t token_reply;
    /* how long it takes a node to reply to PFM */
    uint32_t pfm_reply;
    /* how long it takes a node to reply to DER */
    uint32_t der_reply;
    /* how long it takes a node to send a reply post poned */
    uint32_t reply_postponed;
};

static struct mstp_statistics MSTP_Statistics[256];
static uint32_t Invalid_Frame_Count;

static uint32_t timeval_diff_ms(
    struct timeval *old,
    struct timeval *now)
{
    uint32_t ms = 0;

    /* convert to milliseconds */
    ms = (now->tv_sec - old->tv_sec) * 1000 + (now->tv_usec -
        old->tv_usec) / 1000;

    return ms;
}

static void packet_statistics(
    struct timeval *tv,
    volatile struct mstp_port_struct_t *mstp_port)
{
    static struct timeval old_tv = { 0 };
    static uint8_t old_frame = 255;
    static uint8_t old_src = 255;
    static uint8_t old_dst = 255;
    uint8_t frame, src, dst;
    uint32_t delta;

    dst = mstp_port->DestinationAddress;
    src = mstp_port->SourceAddress;
    frame = mstp_port->FrameType;
    switch (frame) {
        case FRAME_TYPE_TOKEN:
            MSTP_Statistics[src].token_count++;
            if (old_frame == FRAME_TYPE_TOKEN) {
                if ((old_dst == dst) && (old_src == src)) {
                    /* repeated token */
                    MSTP_Statistics[dst].token_retries++;
                    /* Tusage_timeout */
                    delta = timeval_diff_ms(&old_tv, tv);
                    if (delta > MSTP_Statistics[src].tusage_timeout) {
                        MSTP_Statistics[src].tusage_timeout = delta;
                    }
                } else if (old_dst == src) {
                    /* token to token response time */
                    delta = timeval_diff_ms(&old_tv, tv);
                    if (delta > MSTP_Statistics[src].token_reply) {
                        MSTP_Statistics[src].token_reply = delta;
                    }
                }
            } else if ((old_frame == FRAME_TYPE_POLL_FOR_MASTER) &&
                (old_src == src)) {
                /* Tusage_timeout */
                delta = timeval_diff_ms(&old_tv, tv);
                if (delta > MSTP_Statistics[src].tusage_timeout) {
                    MSTP_Statistics[src].tusage_timeout = delta;
                }
            }
            break;
        case FRAME_TYPE_POLL_FOR_MASTER:
            MSTP_Statistics[src].pfm_count++;
            if (dst > MSTP_Statistics[src].max_master) {
                MSTP_Statistics[src].max_master = dst;
            }
            if ((old_frame == FRAME_TYPE_POLL_FOR_MASTER) && (old_src == src)) {
                /* Tusage_timeout */
                delta = timeval_diff_ms(&old_tv, tv);
                if (delta > MSTP_Statistics[src].tusage_timeout) {
                    MSTP_Statistics[src].tusage_timeout = delta;
                }
            }
            break;
        case FRAME_TYPE_REPLY_TO_POLL_FOR_MASTER:
            if (old_frame == FRAME_TYPE_POLL_FOR_MASTER) {
                delta = timeval_diff_ms(&old_tv, tv);
                if (delta > MSTP_Statistics[src].pfm_reply) {
                    MSTP_Statistics[src].pfm_reply = delta;
                }
            }
            break;
        case FRAME_TYPE_TEST_REQUEST:
            break;
        case FRAME_TYPE_TEST_RESPONSE:
            break;
        case FRAME_TYPE_BACNET_DATA_EXPECTING_REPLY:
            break;
        case FRAME_TYPE_BACNET_DATA_NOT_EXPECTING_REPLY:
            if ((old_frame == FRAME_TYPE_BACNET_DATA_EXPECTING_REPLY) &&
                (old_dst == src)) {
                /* DER response time */
                delta = timeval_diff_ms(&old_tv, tv);
                if (delta > MSTP_Statistics[src].der_reply) {
                    MSTP_Statistics[src].der_reply = delta;
                }
            }
            break;
        case FRAME_TYPE_REPLY_POSTPONED:
            if ((old_frame == FRAME_TYPE_BACNET_DATA_EXPECTING_REPLY) &&
                (old_dst == src)) {
                /* Postponed response time */
                delta = timeval_diff_ms(&old_tv, tv);
                if (delta > MSTP_Statistics[src].reply_postponed) {
                    MSTP_Statistics[src].reply_postponed = delta;
                }
            }
            break;
        default:
            break;
    }

    /* update the old variables */
    old_dst = dst;
    old_src = src;
    old_frame = frame;
    old_tv.tv_sec = tv->tv_sec;
    old_tv.tv_usec = tv->tv_usec;
}

static void packet_statistics_save(
    void)
{
    unsigned i; /* loop counter */
    unsigned node_count = 0;

    fprintf(stdout, "\r\n");
    /* separate with tabs (8) keep words under 8 characters */
    fprintf(stdout,
        "MAC\tMaxMstr\tTokens\tRetries\tTreply"
        "\tTusage\tTrpfm\tTder\tTpostpd");
    fprintf(stdout, "\r\n");
    for (i = 0; i < 256; i++) {
        /* check for masters or slaves */
        if ((MSTP_Statistics[i].token_count) || (MSTP_Statistics[i].der_reply)
            || (MSTP_Statistics[i].pfm_count)) {
            node_count++;
            fprintf(stdout, "%u\t%u", i,
                (unsigned) MSTP_Statistics[i].max_master);
            fprintf(stdout, "\t%lu\t%lu\t%lu\t%lu",
                (long unsigned int) MSTP_Statistics[i].token_count,
                (long unsigned int) MSTP_Statistics[i].token_retries,
                (long unsigned int) MSTP_Statistics[i].token_reply,
                (long unsigned int) MSTP_Statistics[i].tusage_timeout);
            fprintf(stdout, "\t%lu\t%lu\t%lu",
                (long unsigned int) MSTP_Statistics[i].pfm_reply,
                (long unsigned int) MSTP_Statistics[i].der_reply,
                (long unsigned int) MSTP_Statistics[i].reply_postponed);
            fprintf(stdout, "\r\n");
        }
    }
    fprintf(stdout, "Node Count: %u\r\n", node_count);
    fprintf(stdout, "Invalid Frame Count: %lu\r\n",
        (long unsigned int) Invalid_Frame_Count);
}

static void packet_statistics_clear(
    void)
{
    unsigned i; /* loop counter */

    for (i = 0; i < 256; i++) {
        MSTP_Statistics[i].token_count = 0;
        MSTP_Statistics[i].token_retries = 0;
        MSTP_Statistics[i].tusage_timeout = 0;
        MSTP_Statistics[i].max_master = 0;
        MSTP_Statistics[i].token_reply = 0;
        MSTP_Statistics[i].pfm_reply = 0;
        MSTP_Statistics[i].der_reply = 0;
        MSTP_Statistics[i].reply_postponed = 0;
    }
    Invalid_Frame_Count = 0;
}

static uint16_t Timer_Silence(
    void)
{
    uint32_t delta_time = 0;

    delta_time = timer_milliseconds(TIMER_SILENCE);
    if (delta_time > 0xFFFF) {
        delta_time = 0xFFFF;
    }

    return (uint16_t) delta_time;
}

static void Timer_Silence_Reset(
    void)
{
    timer_reset(TIMER_SILENCE);
}

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
#if defined(_WIN32)
static HANDLE hPipe = NULL;     /* pipe handle */

static void named_pipe_create(
    char *name)
{
    fprintf(stdout, "mstpcap: Creating Named Pipe \"%s\"\n", name);
    hPipe =
        CreateNamedPipe(name, PIPE_ACCESS_OUTBOUND,
        PIPE_TYPE_MESSAGE | PIPE_WAIT, 1, 65536, 65536, 300, NULL);
    if (hPipe == INVALID_HANDLE_VALUE) {
        RS485_Print_Error();
        return;
    }
    ConnectNamedPipe(hPipe, NULL);
}

size_t data_write(
    const void *ptr,
    size_t size,
    size_t nitems)
{
    DWORD cbWritten = 0;
    if (hPipe != INVALID_HANDLE_VALUE) {
        (void) WriteFile(hPipe, /* handle to pipe  */
            ptr,        /* buffer to write from  */
            size * nitems,      /* number of bytes to write  */
            &cbWritten, /* number of bytes written  */
            NULL);      /* not overlapped I/O  */
    }

    return fwrite(ptr, size, nitems, pFile);
}

size_t data_write_header(
    const void *ptr,
    size_t size,
    size_t nitems,
    bool pipe_enable)
{
    DWORD cbWritten = 0;
    if (pipe_enable && (hPipe != INVALID_HANDLE_VALUE)) {
        (void) WriteFile(hPipe, /* handle to pipe  */
            ptr,        /* buffer to write from  */
            size * nitems,      /* number of bytes to write  */
            &cbWritten, /* number of bytes written  */
            NULL);      /* not overlapped I/O  */
    }

    return fwrite(ptr, size, nitems, pFile);
}
#else
static int FD_Pipe = -1;
static void named_pipe_create(
    char *name)
{
    int rv = 0;
    rv = mkfifo(name, 0666);
    if ((rv == -1) && (errno != EEXIST)) {
        perror("Error creating named pipe");
        exit(1);
    }
    FD_Pipe = open(name, O_WRONLY);
    if (FD_Pipe == -1) {
        perror("Error connecting to named pipe");
        exit(1);
    }
}

size_t data_write(
    const void *ptr,
    size_t size,
    size_t nitems)
{
    ssize_t bytes = 0;
    if (FD_Pipe != -1) {
        bytes = write(FD_Pipe, ptr, size * nitems);
        bytes = bytes;
    }
    return fwrite(ptr, size, nitems, pFile);
}

size_t data_write_header(
    const void *ptr,
    size_t size,
    size_t nitems,
    bool pipe_enable)
{
    ssize_t bytes = 0;
    if (pipe_enable && (FD_Pipe != -1)) {
        bytes = write(FD_Pipe, ptr, size * nitems);
        bytes = bytes;
    }
    return fwrite(ptr, size, nitems, pFile);
}
#endif

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
    static bool pipe_enable = true;     /* don't write more than one header */
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
        (void) data_write_header(&magic_number, sizeof(magic_number), 1,
            pipe_enable);
        (void) data_write_header(&version_major, sizeof(version_major), 1,
            pipe_enable);
        (void) data_write_header(&version_minor, sizeof(version_minor), 1,
            pipe_enable);
        (void) data_write_header(&thiszone, sizeof(thiszone), 1, pipe_enable);
        (void) data_write_header(&sigfigs, sizeof(sigfigs), 1, pipe_enable);
        (void) data_write_header(&snaplen, sizeof(snaplen), 1, pipe_enable);
        (void) data_write_header(&network, sizeof(network), 1, pipe_enable);
        fflush(pFile);
        fprintf(stdout, "mstpcap: saving capture to %s\n", filename);
    } else {
        fprintf(stderr, "mstpcap: failed to open %s: %s\n", filename,
            strerror(errno));
    }
    if (pipe_enable) {
        pipe_enable = false;
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
        if ((mstp_port->ReceivedValidFrame) ||
            (mstp_port->ReceivedValidFrameNotForUs)) {
            packet_statistics(&tv, mstp_port);
        }
        (void) data_write(&ts_sec, sizeof(ts_sec), 1);
        (void) data_write(&ts_usec, sizeof(ts_usec), 1);
        if (mstp_port->DataLength) {
            max_data = min(mstp_port->InputBufferSize, mstp_port->DataLength);
            incl_len = orig_len = 8 + max_data + 2;
        } else {
            incl_len = orig_len = 8;
        }
        (void) data_write(&incl_len, sizeof(incl_len), 1);
        (void) data_write(&orig_len, sizeof(orig_len), 1);
        header[0] = 0x55;
        header[1] = 0xFF;
        header[2] = mstp_port->FrameType;
        header[3] = mstp_port->DestinationAddress;
        header[4] = mstp_port->SourceAddress;
        header[5] = HI_BYTE(mstp_port->DataLength);
        header[6] = LO_BYTE(mstp_port->DataLength);
        header[7] = mstp_port->HeaderCRCActual;
        (void) data_write(header, sizeof(header), 1);
        if (mstp_port->DataLength) {
            (void) data_write(mstp_port->InputBuffer, max_data, 1);
            (void) data_write((char *) &mstp_port->DataCRCActualMSB, 1, 1);
            (void) data_write((char *) &mstp_port->DataCRCActualLSB, 1, 1);
        }
    } else {
        fprintf(stderr, "mstpcap: failed to open %s: %s\n", Capture_Filename,
            strerror(errno));
    }
}

static void cleanup(
    void)
{
    packet_statistics_save();
    if (pFile) {
        fflush(pFile);  /* stream pointer */
        fclose(pFile);  /* stream pointer */
    }
    pFile = NULL;
}

#if defined(_WIN32)
static BOOL WINAPI CtrlCHandler(
    DWORD dwCtrlType)
{
    dwCtrlType = dwCtrlType;

    if (hPipe) {
        FlushFileBuffers(hPipe);
        DisconnectNamedPipe(hPipe);
        CloseHandle(hPipe);
    }
    exit(0);
    return TRUE;
}
#else
static void sig_int(
    int signo)
{
    (void) signo;
    if (FD_Pipe != -1) {
        close(FD_Pipe);
    }

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

    /* mimic our pointer in the state machine */
    mstp_port = &MSTP_Port;
    /* initialize our interface */
    if ((argc > 1) && (strcmp(argv[1], "--help") == 0)) {
        printf("mstpcap [interface] [baud] [named pipe]\r\n"
            "Captures MS/TP packets from a serial interface\r\n"
            "and save them to a file. Saves packets in a\r\n"
            "filename mstp_20090123091200.cap that has data and time.\r\n"
            "After receiving 65535 packets, a new file is created.\r\n" "\r\n"
            "Command line options:\r\n" "[interface] - serial interface.\r\n"
            "    defaults to COM4 on  Windows, and /dev/ttyUSB0 on linux.\r\n"
            "[baud] - baud rate.  9600, 19200, 38400, 57600, 115200\r\n"
            "    defaults to 38400.\r\n"
            "[named pipe] - use \\\\.\\pipe\\wireshark as the name\r\n"
            "    and set that name as the interface name in Wireshark\r\n");
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
    fprintf(stdout, "mstpcap: Using %s for capture at %ld bps.\n",
        RS485_Interface(), (long) RS485_Get_Baud_Rate());
    atexit(cleanup);
#if defined(_WIN32)
    SetConsoleMode(GetStdHandle(STD_INPUT_HANDLE), ENABLE_PROCESSED_INPUT);
    SetConsoleCtrlHandler((PHANDLER_ROUTINE) CtrlCHandler, TRUE);
#else
    signal_init();
#endif
    if (argc > 3) {
        named_pipe_create(argv[3]);
    }
    filename_create_new();
    /* run forever */
    for (;;) {
        RS485_Check_UART_Data(mstp_port);
        MSTP_Receive_Frame_FSM(mstp_port);
        /* process the data portion of the frame */
        if (mstp_port->ReceivedValidFrame) {
            write_received_packet(mstp_port);
            mstp_port->ReceivedValidFrame = false;
            packet_count++;
        } else if (mstp_port->ReceivedValidFrameNotForUs) {
            write_received_packet(mstp_port);
            mstp_port->ReceivedValidFrameNotForUs = false;
            packet_count++;
        } else if (mstp_port->ReceivedInvalidFrame) {
            write_received_packet(mstp_port);
            Invalid_Frame_Count++;
            mstp_port->ReceivedInvalidFrame = false;
            packet_count++;
        }
        if (!(packet_count % 100)) {
            fprintf(stdout, "\r%hu packets, %hu invalid frames", packet_count,
                Invalid_Frame_Count);
        }
        if (packet_count >= 65535) {
            packet_statistics_save();
            packet_statistics_clear();
            filename_create_new();
            packet_count = 0;
        }
    }
}
