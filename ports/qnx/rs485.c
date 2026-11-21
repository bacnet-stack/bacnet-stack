/**
 * @file
 * @brief RS-485 serial interface for BACnet MS/TP on QNX Neutrino (QNX 8.0)
 *
 * The Linux port relies on termios2 (BOTHER) and the TIOCSRS485 ioctl, and
 * the BSD port on Apple IOKit; neither exists on QNX. This port uses:
 *  - POSIX termios with numeric speeds (QNX speed_t is the baud rate itself,
 *    so non-standard MS/TP rates such as 76800 work without BOTHER),
 *  - devctl(DCMD_CHR_SERCTL) to drive RTS as the RS-485 transmitter enable
 *    when the transceiver has no automatic direction control,
 *  - readcond() to drain the receive buffer without blocking,
 *  - clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME) for the MS/TP
 *    turnaround delay, with the achieved delay recorded for diagnostics.
 *
 * Serial devices on QNX are provided by drivers such as devc-ser8250
 * (/dev/ser1, /dev/ser2, ...) or devc-serusb (/dev/serusb1).
 *
 * The non-QNX branches let the same file be compiled and exercised on a
 * POSIX host (e.g. Linux with a pseudo-terminal pair) for testing.
 *
 * @author Vihar N
 * @date 2026
 * @copyright SPDX-License-Identifier: GPL-2.0-or-later WITH GCC-exception-2.0
 */
#include <errno.h>
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/select.h>
#include <sys/time.h>
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>
#include <dirent.h>
#include <signal.h>
#include <sys/ioctl.h>
#ifdef __QNX__
#include <devctl.h>
#include <sys/dcmd_chr.h>
#endif

#include "bacnet/datalink/mstp.h"
#include "bacnet/basic/sys/fifo.h"
#include "bacnet/basic/sys/debug.h"
#include "rs485.h"

/* handle returned from open() */
static int RS485_Handle = -1;
/* numeric baud rate */
static uint32_t RS485_Baud = 38400;
/* serial port name: /dev/ser1 (devc-ser8250), /dev/serusb1 (devc-serusb) */
static const char *RS485_Port_Name = "/dev/ser1";
/* original serial port settings */
static struct termios RS485_oldtio;
static bool RS485_oldtio_valid = false;
/* drive the transmitter enable from RTS */
static bool RS485_RTS_Control = false;
/* statistics */
static RS485_STATISTICS RS485_Stats;
/* set from the SIGUSR1 handler: print statistics from the MS/TP thread */
static volatile sig_atomic_t RS485_Stats_Requested = 0;

/* Ring buffer for incoming bytes. Buffer size needs to be a power of 2 */
static FIFO_BUFFER Rx_FIFO;
static uint8_t Rx_Buffer[NEXT_POWER_OF_2(4096)];

void RS485_Set_Interface(const char *ifname)
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

uint32_t RS485_Get_Baud_Rate(void)
{
    return RS485_Baud;
}

uint32_t RS485_Get_Port_Baud_Rate(struct mstp_port_struct_t *mstp_port)
{
    (void)mstp_port;
    return RS485_Baud;
}

void RS485_Set_RTS_Direction_Control(bool enable)
{
    RS485_RTS_Control = enable;
}

bool RS485_RTS_Direction_Control(void)
{
    return RS485_RTS_Control;
}

void RS485_Statistics(RS485_STATISTICS *stats)
{
    if (stats) {
        *stats = RS485_Stats;
    }
}

#ifndef __QNX__
/* POSIX host build: map a numeric rate onto the termios constants */
static speed_t rs485_speed(uint32_t baud)
{
    switch (baud) {
        case 9600:
            return B9600;
        case 19200:
            return B19200;
        case 38400:
            return B38400;
        case 57600:
            return B57600;
        case 115200:
            return B115200;
        default:
            return B38400;
    }
}
#endif

/**
 * @brief Configure the port for MS/TP: 8 data bits, no parity, 1 stop bit,
 *  raw mode, no flow control.
 */
static bool rs485_configure(int handle, uint32_t baud)
{
    struct termios tio;

    if (tcgetattr(handle, &tio) != 0) {
        return false;
    }
    cfmakeraw(&tio);
    tio.c_cflag &= ~(CSIZE | PARENB | CSTOPB);
    tio.c_cflag |= CS8 | CLOCAL | CREAD;
#ifdef IHFLOW
    /* QNX hardware flow control flags must be off: RTS is used for
       the transmitter enable, not for flow control */
    tio.c_cflag &= ~(IHFLOW | OHFLOW);
#endif
#ifdef CRTSCTS
    tio.c_cflag &= ~CRTSCTS;
#endif
    tio.c_iflag &= ~(IXON | IXOFF | IXANY);
    /* return immediately from read() with whatever is available */
    tio.c_cc[VMIN] = 0;
    tio.c_cc[VTIME] = 0;
#ifdef __QNX__
    /* QNX: speed_t is the numeric baud rate, so any rate the UART
       supports (including MS/TP's 76800) can be requested directly */
    cfsetispeed(&tio, (speed_t)baud);
    cfsetospeed(&tio, (speed_t)baud);
#else
    cfsetispeed(&tio, rs485_speed(baud));
    cfsetospeed(&tio, rs485_speed(baud));
#endif
    return tcsetattr(handle, TCSANOW, &tio) == 0;
}

bool RS485_Set_Baud_Rate(uint32_t baud)
{
    switch (baud) {
        case 9600:
        case 19200:
        case 38400:
        case 57600:
        case 76800:
        case 115200:
            break;
        default:
            return false;
    }
    RS485_Baud = baud;
    if (RS485_Handle >= 0) {
        return rs485_configure(RS485_Handle, baud);
    }
    return true;
}

/**
 * @brief Assert or release the RS-485 transmitter enable (RTS line)
 */
static void rs485_rts(int handle, bool on)
{
    if (!RS485_RTS_Control || (handle < 0)) {
        return;
    }
#ifdef __QNX__
    {
        int ctl = _CTL_RTS_CHG | (on ? _CTL_RTS : 0);
        (void)devctl(handle, DCMD_CHR_SERCTL, &ctl, sizeof(ctl), NULL);
    }
#elif defined(TIOCMBIS) && defined(TIOCM_RTS)
    {
        int bits = TIOCM_RTS;
        (void)ioctl(handle, on ? TIOCMBIS : TIOCMBIC, &bits);
    }
#else
    (void)on;
#endif
}

static uint64_t timespec_usec(const struct timespec *ts)
{
    return ((uint64_t)ts->tv_sec * 1000000ULL) +
        ((uint64_t)ts->tv_nsec / 1000ULL);
}

/**
 * @brief Sleep until an absolute monotonic deadline, retrying on signals
 */
static void sleep_until(const struct timespec *deadline)
{
    while (clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, deadline, NULL) ==
           EINTR) {
        /* retry */
    }
}

void RS485_Send_Frame(
    struct mstp_port_struct_t *mstp_port,
    const uint8_t *buffer,
    uint16_t nbytes)
{
    struct timespec start, deadline, now;
    uint64_t turnaround_usec;
    uint64_t achieved;
    ssize_t written;
    size_t total = 0;

    if ((RS485_Handle < 0) || (buffer == NULL) || (nbytes == 0)) {
        return;
    }
    /* Tturnaround: 40 bit times, so other nodes can release the bus */
    turnaround_usec = ((uint64_t)Tturnaround * 1000000ULL) / RS485_Baud;
    clock_gettime(CLOCK_MONOTONIC, &start);
    deadline = start;
    deadline.tv_nsec += (long)(turnaround_usec * 1000ULL);
    while (deadline.tv_nsec >= 1000000000L) {
        deadline.tv_nsec -= 1000000000L;
        deadline.tv_sec++;
    }
    sleep_until(&deadline);
    clock_gettime(CLOCK_MONOTONIC, &now);
    achieved = timespec_usec(&now) - timespec_usec(&start);
    RS485_Stats.turnaround_usec_last = (uint32_t)achieved;
    if (achieved > RS485_Stats.turnaround_usec_max) {
        RS485_Stats.turnaround_usec_max = (uint32_t)achieved;
    }

    rs485_rts(RS485_Handle, true);
    while (total < nbytes) {
        written = write(RS485_Handle, buffer + total, nbytes - total);
        if (written < 0) {
            if (errno == EINTR) {
                continue;
            }
            RS485_Stats.write_errors++;
            debug_fprintf(stderr, "RS485: write error: %s\n", strerror(errno));
            break;
        }
        total += (size_t)written;
    }
    /* wait until all output has left the UART before releasing the bus */
    (void)tcdrain(RS485_Handle);
    if (RS485_RTS_Control) {
        /* tcdrain() can return when the FIFO is empty but the last
           character is still in the shift register: allow one more
           character time (10 bits) before dropping the driver enable */
        clock_gettime(CLOCK_MONOTONIC, &deadline);
        deadline.tv_nsec += (long)((10ULL * 1000000000ULL) / RS485_Baud);
        while (deadline.tv_nsec >= 1000000000L) {
            deadline.tv_nsec -= 1000000000L;
            deadline.tv_sec++;
        }
        sleep_until(&deadline);
    }
    rs485_rts(RS485_Handle, false);
    RS485_Stats.frames_sent++;
    RS485_Stats.bytes_sent += (uint32_t)total;

    /* per MSTP spec, sort of */
    if (mstp_port) {
        mstp_port->SilenceTimerReset((void *)mstp_port);
    }
}

static void rs485_print_statistics(void)
{
    uint32_t target_usec =
        (uint32_t)(((uint64_t)Tturnaround * 1000000ULL) / RS485_Baud);

    fprintf(
        stderr,
        "RS485 %s @ %lu bps: frames=%lu tx=%lu rx=%lu write_errors=%lu\n"
        "RS485 Tturnaround target=%luus last=%luus max=%luus\n",
        RS485_Port_Name, (unsigned long)RS485_Baud,
        (unsigned long)RS485_Stats.frames_sent,
        (unsigned long)RS485_Stats.bytes_sent,
        (unsigned long)RS485_Stats.bytes_received,
        (unsigned long)RS485_Stats.write_errors, (unsigned long)target_usec,
        (unsigned long)RS485_Stats.turnaround_usec_last,
        (unsigned long)RS485_Stats.turnaround_usec_max);
}

static void rs485_on_sigusr1(int sig)
{
    (void)sig;
    RS485_Stats_Requested = 1;
}

/**
 * @brief Read whatever bytes are waiting, without blocking
 */
static ssize_t rs485_read_available(int handle, uint8_t *buf, size_t size)
{
#ifdef __QNX__
    /* min=0, time=0, timeout=0: return immediately with what is buffered */
    return readcond(handle, buf, (int)size, 0, 0, 0);
#else
    return read(handle, buf, size);
#endif
}

void RS485_Check_UART_Data(struct mstp_port_struct_t *mstp_port)
{
    fd_set input;
    struct timeval waiter;
    uint8_t buf[2048];
    ssize_t n;

    if (RS485_Handle < 0) {
        return;
    }
    if (RS485_Stats_Requested) {
        RS485_Stats_Requested = 0;
        rs485_print_statistics();
    }
    waiter.tv_sec = 0;
    waiter.tv_usec = 5000;
    if (mstp_port->ReceiveError == true) {
        /* do nothing but wait for state machine to clear the error */
    } else if (mstp_port->DataAvailable == false) {
        /* wait for state machine to read from the DataRegister */
        if (FIFO_Count(&Rx_FIFO) > 0) {
            /* data is available */
            mstp_port->DataRegister = FIFO_Get(&Rx_FIFO);
            mstp_port->DataAvailable = true;
            /* FIFO is giving data - just poll */
            waiter.tv_sec = 0;
            waiter.tv_usec = 0;
        }
    }
    /* grab bytes and stuff them into the FIFO every time */
    FD_ZERO(&input);
    FD_SET(RS485_Handle, &input);
    n = select(RS485_Handle + 1, &input, NULL, NULL, &waiter);
    if (n <= 0) {
        return;
    }
    if (FD_ISSET(RS485_Handle, &input)) {
        n = rs485_read_available(RS485_Handle, buf, sizeof(buf));
        if (n > 0) {
            FIFO_Add(&Rx_FIFO, &buf[0], (unsigned)n);
            RS485_Stats.bytes_received += (uint32_t)n;
        }
    }
}

void RS485_Cleanup(void)
{
    const char *env = getenv("BACNET_RS485_STATS");

    if (env && (env[0] == '1') && (RS485_Handle >= 0)) {
        rs485_print_statistics();
    }
    if (RS485_Handle >= 0) {
        rs485_rts(RS485_Handle, false);
        if (RS485_oldtio_valid) {
            (void)tcsetattr(RS485_Handle, TCSANOW, &RS485_oldtio);
        }
        close(RS485_Handle);
        RS485_Handle = -1;
    }
}

void RS485_Initialize(void)
{
    const char *env;

    env = getenv("BACNET_RS485_RTS");
    if (env && (env[0] == '1')) {
        RS485_RTS_Control = true;
    }
    env = getenv("BACNET_RS485_STATS");
    if (env && (env[0] == '1')) {
        /* kill -USR1 (QNX: slay -s SIGUSR1) prints the statistics */
        signal(SIGUSR1, rs485_on_sigusr1);
    }
    debug_printf("RS485 Interface: %s\n", RS485_Port_Name);
    RS485_Handle = open(RS485_Port_Name, O_RDWR | O_NOCTTY);
    if (RS485_Handle < 0) {
        perror(RS485_Port_Name);
        exit(-1);
    }
    /* save current serial port settings */
    if (tcgetattr(RS485_Handle, &RS485_oldtio) == 0) {
        RS485_oldtio_valid = true;
    }
    if (!rs485_configure(RS485_Handle, RS485_Baud)) {
        fprintf(
            stderr, "RS485: unable to configure %s at %lu bps: %s\n",
            RS485_Port_Name, (unsigned long)RS485_Baud, strerror(errno));
    }
    rs485_rts(RS485_Handle, false);
    /* destructor */
    atexit(RS485_Cleanup);
    /* flush any data waiting */
    usleep(200000);
    (void)tcflush(RS485_Handle, TCIOFLUSH);
    /* ringbuffer */
    FIFO_Init(&Rx_FIFO, Rx_Buffer, sizeof(Rx_Buffer));
    memset(&RS485_Stats, 0, sizeof(RS485_Stats));
}

/* Print in a format for Wireshark ExtCap */
void RS485_Print_Ports(void)
{
    DIR *dir;
    struct dirent *entry;

    dir = opendir("/dev");
    if (dir == NULL) {
        return;
    }
    while ((entry = readdir(dir)) != NULL) {
        if ((strncmp(entry->d_name, "ser", 3) == 0) ||
            (strncmp(entry->d_name, "serusb", 6) == 0)) {
            printf(
                "interface {value=/dev/%s}"
                "{display=MS/TP Capture on /dev/%s}\n",
                entry->d_name, entry->d_name);
        }
    }
    closedir(dir);
}

#ifdef TEST_RS485
#include <signal.h>
static volatile sig_atomic_t Run = 1;
static void on_signal(int sig)
{
    (void)sig;
    Run = 0;
}

/* Simple loopback / sniffer test: rs485 /dev/ser1 38400 */
int main(int argc, char *argv[])
{
    uint8_t frame[8] = { 0x55, 0xFF, 0x00, 0x7F, 0x01, 0x00, 0x00, 0x00 };
    uint8_t buf[512];
    ssize_t n;

    if (argc > 1) {
        RS485_Set_Interface(argv[1]);
    }
    if (argc > 2) {
        RS485_Set_Baud_Rate((uint32_t)strtoul(argv[2], NULL, 10));
    }
    signal(SIGINT, on_signal);
    RS485_Initialize();
    RS485_Send_Frame(NULL, frame, sizeof(frame));
    while (Run) {
        n = rs485_read_available(RS485_Handle, buf, sizeof(buf));
        if (n > 0) {
            for (ssize_t i = 0; i < n; i++) {
                printf("%02X ", buf[i]);
            }
            fflush(stdout);
        }
        usleep(10000);
    }
    return 0;
}
#endif
