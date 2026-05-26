/**
 * @file modbus_rtu.c
 * @brief Generic Modbus RTU master driver implementation.
 *
 * Supported: FC01, FC02, FC03, FC04, FC05, FC06
 * CRC16 (polynomial 0xA001, Modbus standard)
 * Platforms: Windows (_WIN32), Linux/Unix/macOS
 *
 * @author Jihye Ahn <jen.ahn@lge.com>
 *
 * @copyright Copyright (c) 2026 LG Electronics Inc.
 * SPDX-License-Identifier: MIT
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

#if defined(_WIN32)
/* Windows includes */
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <process.h>
#else
/* POSIX includes (Linux, Unix, macOS) */
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <time.h>
#include <sys/ioctl.h>
#include <sys/select.h>
#include <termios.h>
#endif /* _WIN32 */

#include "modbus_rtu.h"

/* -----------------------------------------------------------------------
 * Internal state (platform-agnostic)
 * -----------------------------------------------------------------------*/
static MODBUS_PORT g_port;
static int g_port_initialized = 0; /* flag: first-time initialization done */

#if defined(_WIN32)
/* Windows-specific: COM timeouts for the port */
static COMMTIMEOUTS g_original_timeouts = {0};
#endif

/* -----------------------------------------------------------------------
 * CRC16 (Modbus) – Platform-independent
 * -----------------------------------------------------------------------*/
static uint16_t crc16_modbus(const uint8_t *buf, uint16_t len)
{
    uint16_t crc = 0xFFFF;
    uint16_t i;
    int j;
    for (i = 0; i < len; i++) {
        crc ^= buf[i];
        for (j = 0; j < 8; j++) {
            if (crc & 0x0001) {
                crc = (crc >> 1) ^ 0xA001;
            } else {
                crc >>= 1;
            }
        }
    }
    return crc;
}

/* -----------------------------------------------------------------------
 * Platform-specific helper functions
 * -----------------------------------------------------------------------*/

#if defined(_WIN32)
/* ───────── Windows Implementation ───────── */

/**
 * Map baud rate to Windows CBR_* constant
 */
static DWORD baud_to_windows(uint32_t baud)
{
    switch (baud) {
        case 110:
            return CBR_110;
        case 300:
            return CBR_300;
        case 600:
            return CBR_600;
        case 1200:
            return CBR_1200;
        case 2400:
            return CBR_2400;
        case 4800:
            return CBR_4800;
        case 9600:
            return CBR_9600;
        case 14400:
            return CBR_14400;
        case 19200:
            return CBR_19200;
        case 38400:
            return CBR_38400;
        case 57600:
            return CBR_57600;
        case 115200:
            return CBR_115200;
        case 128000:
            return CBR_128000;
        case 256000:
            return CBR_256000;
        default:
            return CBR_9600;
    }
}

/**
 * Format COM port name for Windows (e.g., "COM1" -> "\\\\.\\COM1")
 */
static void format_com_port(const char *in_port, char *out_port, size_t out_size)
{
    if (!in_port || !out_port) {
        return;
    }

    /* Check if it's already formatted */
    if (strncmp(in_port, "\\\\.\\", 4) == 0) {
        strncpy(out_port, in_port, out_size - 1);
        out_port[out_size - 1] = '\0';
        return;
    }

    /* Format as \\.\COMn (required for COM ports > 9) */
    snprintf(out_port, out_size, "\\\\.\\%s", in_port);
}

/**
 * Configure Windows serial port with 8N1, timeout, and buffers
 */
static bool configure_com_port(HANDLE handle, uint32_t baud)
{
    DCB dcb = {0};
    COMMTIMEOUTS ct = {0};

    dcb.DCBlength = sizeof(dcb);

    /* Get current DCB settings */
    if (!GetCommState(handle, &dcb)) {
        fprintf(stderr, "[Modbus] Failed to get COM port state\n");
        return false;
    }

    /* Configure for 8N1 */
    dcb.BaudRate = baud_to_windows(baud);
    dcb.ByteSize = 8;
    dcb.Parity = NOPARITY;
    dcb.StopBits = ONESTOPBIT;
    dcb.fDtrControl = DTR_CONTROL_DISABLE;
    dcb.fRtsControl = RTS_CONTROL_DISABLE;
    dcb.fParity = FALSE;

    if (!SetCommState(handle, &dcb)) {
        fprintf(stderr, "[Modbus] Failed to set COM port state\n");
        return false;
    }

    /* Save original timeouts for later restoration */
    GetCommTimeouts(handle, &g_original_timeouts);

    /* Set timeouts: ReadIntervalTimeout allows partial reads to complete */
    ct.ReadIntervalTimeout = 50;           /* 50 ms inter-character timeout */
    ct.ReadTotalTimeoutMultiplier = 0;
    ct.ReadTotalTimeoutConstant = 500;     /* 500 ms total read timeout */
    ct.WriteTotalTimeoutMultiplier = 0;
    ct.WriteTotalTimeoutConstant = 500;    /* 500 ms total write timeout */

    if (!SetCommTimeouts(handle, &ct)) {
        fprintf(stderr, "[Modbus] Failed to set COM port timeouts\n");
        return false;
    }

    /* Purge any stray data */
    PurgeComm(handle, PURGE_RXABORT | PURGE_TXABORT | PURGE_RXCLEAR | PURGE_TXCLEAR);

    return true;
}

#else
/* ───────── POSIX Implementation (Linux, Unix, macOS) ───────── */

/**
 * Map baud rate to POSIX termios speed_t constant
 */
static speed_t baud_to_speed(uint32_t baud)
{
    switch (baud) {
        case 1200:
            return B1200;
        case 2400:
            return B2400;
        case 4800:
            return B4800;
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
            return B9600;
    }
}

#endif /* _WIN32 */



/* -----------------------------------------------------------------------
 * Open and configure the serial port (cross-platform)
 * -----------------------------------------------------------------------*/
bool Modbus_RTU_Init(const char *port, uint32_t baud, uint8_t slave)
{
#if defined(_WIN32)
    char formatted_port[256];
#else
    struct termios tio;
    speed_t spd;
#endif

    /* Set defaults on first call */
    if (!g_port_initialized) {
#if defined(_WIN32)
        g_port.handle = INVALID_HANDLE_VALUE;
#else
        g_port.fd = -1;
#endif
        g_port_initialized = 1;
    }

    /* Close existing port if already open */
    if (
#if defined(_WIN32)
        g_port.handle != INVALID_HANDLE_VALUE
#else
        g_port.fd >= 0
#endif
    ) {
        Modbus_RTU_Cleanup();
    }

    /* Store port config */
    strncpy(
        g_port.port, port ? port : MODBUS_DEFAULT_PORT,
        sizeof(g_port.port) - 1);
    g_port.port[sizeof(g_port.port) - 1] = '\0';
    g_port.baud = baud ? baud : MODBUS_DEFAULT_BAUD;
    g_port.slave_id = slave ? slave : MODBUS_SENSOR_SLAVE_ID;

#if defined(_WIN32)
    /* ───── Windows Implementation ───── */
    format_com_port(g_port.port, formatted_port, sizeof(formatted_port));

    /* Open the COM port */
    g_port.handle = CreateFileA(
        formatted_port,
        GENERIC_READ | GENERIC_WRITE,
        0,              /* No sharing */
        NULL,           /* Default security */
        OPEN_EXISTING,
        0,              /* Synchronous I/O */
        NULL);

    if (g_port.handle == INVALID_HANDLE_VALUE) {
        DWORD err = GetLastError();
        fprintf(
            stderr,
            "[Modbus] Failed to open port: %s (Error %lu)\n",
            g_port.port, err);
        return false;
    }

    /* Configure serial port */
    if (!configure_com_port(g_port.handle, g_port.baud)) {
        CloseHandle(g_port.handle);
        g_port.handle = INVALID_HANDLE_VALUE;
        return false;
    }

    printf(
        "[Modbus] Port opened: %s @ %u bps, Slave=%u\n",
        g_port.port, g_port.baud, g_port.slave_id);
    return true;

#else
    /* ───── POSIX Implementation ───── */
    /* Open port in non-blocking mode, then switch to blocking */
    g_port.fd = open(g_port.port, O_RDWR | O_NOCTTY | O_NONBLOCK);
    if (g_port.fd < 0) {
        fprintf(
            stderr,
            "[Modbus] Failed to open port: %s (%s)\n",
            g_port.port, strerror(errno));
        return false;
    }

    /* Switch to blocking mode */
    fcntl(g_port.fd, F_SETFL, 0);

    /* Configure for 8N1 */
    memset(&tio, 0, sizeof(tio));
    tio.c_cflag = CS8 | CREAD | CLOCAL;
    tio.c_iflag = IGNPAR;
    tio.c_oflag = 0;
    tio.c_lflag = 0;
    /* Non-blocking read; timeout handled by select() */
    tio.c_cc[VMIN] = 0;
    tio.c_cc[VTIME] = 0;

    spd = baud_to_speed(g_port.baud);
    cfsetispeed(&tio, spd);
    cfsetospeed(&tio, spd);

    if (tcsetattr(g_port.fd, TCSANOW, &tio) < 0) {
        fprintf(
            stderr,
            "[Modbus] Failed to configure port (%s)\n",
            strerror(errno));
        close(g_port.fd);
        g_port.fd = -1;
        return false;
    }

    tcflush(g_port.fd, TCIOFLUSH);

    printf(
        "[Modbus] Port opened: %s @ %u bps, Slave=%u\n",
        g_port.port, g_port.baud, g_port.slave_id);
    return true;

#endif /* _WIN32 */
}

/* -----------------------------------------------------------------------
 * Close and release the port (cross-platform)
 * -----------------------------------------------------------------------*/
void Modbus_RTU_Cleanup(void)
{
#if defined(_WIN32)
    if (g_port.handle != INVALID_HANDLE_VALUE) {
        /* Restore original timeouts */
        SetCommTimeouts(g_port.handle, &g_original_timeouts);
        CloseHandle(g_port.handle);
        g_port.handle = INVALID_HANDLE_VALUE;
        printf("[Modbus] Port closed: %s\n", g_port.port);
    }
#else
    if (g_port.fd >= 0) {
        close(g_port.fd);
        g_port.fd = -1;
        printf("[Modbus] Port closed: %s\n", g_port.port);
    }
#endif
}

/* -----------------------------------------------------------------------
 * Receive bytes with timeout (cross-platform)
 * -----------------------------------------------------------------------*/
static int modbus_recv(uint8_t *buf, int max_len, int timeout_ms)
{
#if defined(_WIN32)
    /* ───── Windows Implementation ───── */
    DWORD bytes_read = 0;

    (void)timeout_ms; /* timeout is handled by COM timeouts set in configure_com_port() */
    if (g_port.handle == INVALID_HANDLE_VALUE) {
        return 0;
    }

    /* ReadFile with timeout already configured in SetCommTimeouts() */
    if (!ReadFile(g_port.handle, buf, (DWORD)max_len, &bytes_read, NULL)) {
        DWORD err = GetLastError();
        if (err != ERROR_IO_PENDING && err != 0) {
            fprintf(
                stderr,
                "[Modbus] ReadFile error: 0x%lx\n", err);
        }
        return 0;
    }

    return (int)bytes_read;

#else
    /* ───── POSIX Implementation ───── */
    int total = 0;
    int ret;
    int n;
    struct timeval tv;
    fd_set fds;

    while (total < max_len) {
        FD_ZERO(&fds);
        FD_SET(g_port.fd, &fds);
        tv.tv_sec = timeout_ms / 1000;
        tv.tv_usec = (timeout_ms % 1000) * 1000;

        ret = select(g_port.fd + 1, &fds, NULL, NULL, &tv);
        if (ret <= 0) {
            /* Timeout or error */
            break;
        }

        n = (int)read(g_port.fd, buf + total, max_len - total);
        if (n <= 0) {
            break;
        }
        total += n;

        /* Wait briefly after the minimum frame size to collect remaining bytes */
        if (total >= 5) {
            usleep(2000); /* 2 ms */
        }
    }
    return total;

#endif /* _WIN32 */
}

/**
 * Send bytes to the serial port (cross-platform)
 */
static bool modbus_send(const uint8_t *buf, int len)
{
#if defined(_WIN32)
    DWORD bytes_written = 0;

    if (g_port.handle == INVALID_HANDLE_VALUE) {
        fprintf(stderr, "[Modbus] Port is not initialized.\n");
        return false;
    }

    /* Purge any pending data before sending */
    PurgeComm(g_port.handle, PURGE_RXCLEAR);

    if (!WriteFile(g_port.handle, buf, (DWORD)len, &bytes_written, NULL)) {
        fprintf(stderr, "[Modbus] WriteFile error\n");
        return false;
    }

    return (int)bytes_written == len;

#else
    if (g_port.fd < 0) {
        fprintf(stderr, "[Modbus] Port is not initialized.\n");
        return false;
    }

    /* Flush any pending data before sending */
    tcflush(g_port.fd, TCIOFLUSH);

    return (write(g_port.fd, buf, len) == len);

#endif /* _WIN32 */
}

/**
 * Sleep for milliseconds (cross-platform)
 */
static void modbus_sleep_ms(int ms)
{
#if defined(_WIN32)
    Sleep((DWORD)ms);
#else
    usleep(ms * 1000);
#endif
}


/* -----------------------------------------------------------------------
 * FC03 - Read Holding Registers
 * -----------------------------------------------------------------------*/
bool Modbus_Read_Holding_Registers(
    uint8_t slave_id, uint16_t reg_addr, uint8_t count, uint16_t *regs)
{
    uint8_t req[8];
    uint8_t resp[5 + 2 * 125 + 2]; /* maximum response size */
    int resp_len;
    int expected_len;
    uint16_t crc_calc, crc_recv;
    int retry;
    int i;

#if defined(_WIN32)
    if (g_port.handle == INVALID_HANDLE_VALUE) {
        fprintf(stderr, "[Modbus] Port is not initialized.\n");
        return false;
    }
#else
    if (g_port.fd < 0) {
        fprintf(stderr, "[Modbus] Port is not initialized.\n");
        return false;
    }
#endif

    if (!regs || count == 0 || count > 125) {
        return false;
    }

    /* Build request frame */
    req[0] = slave_id;
    req[1] = 0x03; /* FC03 */
    req[2] = (reg_addr >> 8) & 0xFF;
    req[3] = reg_addr & 0xFF;
    req[4] = 0x00;
    req[5] = count;
    crc_calc = crc16_modbus(req, 6);
    req[6] = crc_calc & 0xFF; /* CRC Low byte */
    req[7] = (crc_calc >> 8) & 0xFF; /* CRC High byte */

    expected_len = 5 + 2 * count; /* ID + FC + ByteCount + data*2 + CRC*2 */

    for (retry = 0; retry < MODBUS_MAX_RETRY; retry++) {
        if (!modbus_send(req, 8)) {
            fprintf(stderr, "[Modbus] Write error (retry %d)\n", retry + 1);
            modbus_sleep_ms(50);
            continue;
        }

        resp_len = modbus_recv(resp, expected_len, MODBUS_TIMEOUT_MS);

        if (resp_len < expected_len) {
            fprintf(
                stderr,
                "[Modbus] Short response: received=%d, expected=%d (retry %d)\n",
                resp_len, expected_len, retry + 1);
            modbus_sleep_ms(100);
            continue;
        }

        /* Verify CRC */
        crc_calc = crc16_modbus(resp, resp_len - 2);
        crc_recv = (uint16_t)(resp[resp_len - 2]) |
            ((uint16_t)(resp[resp_len - 1]) << 8);
        if (crc_calc != crc_recv) {
            fprintf(
                stderr,
                "[Modbus] CRC mismatch: calc=0x%04X, recv=0x%04X (retry %d)\n",
                crc_calc, crc_recv, retry + 1);
            modbus_sleep_ms(100);
            continue;
        }

        /* Verify slave ID and function code */
        if (resp[0] != slave_id || resp[1] != 0x03) {
            /* Handle Modbus exception response */
            if (resp[1] == 0x83) {
                fprintf(
                    stderr, "[Modbus] Exception response: code=0x%02X\n",
                    resp[2]);
            } else {
                fprintf(
                    stderr, "[Modbus] Unexpected response: ID=%u FC=%u\n",
                    resp[0], resp[1]);
            }
            return false;
        }

        /* Parse register values */
        for (i = 0; i < count; i++) {
            regs[i] =
                ((uint16_t)resp[3 + 2 * i] << 8) | (uint16_t)resp[4 + 2 * i];
        }
        return true;
    }

    return false;
}

/* -----------------------------------------------------------------------
 * Read temperature and humidity from the sensor (legacy)
 * -----------------------------------------------------------------------*/
bool Modbus_Read_Sensor(uint8_t slave_id, SENSOR_DATA *data)
{
    uint16_t regs[2] = { 0 };

    if (!data) {
        return false;
    }

    data->valid = false;

    /* Read registers 0x0000 (humidity) and 0x0001 (temperature) in one request
     */
    if (!Modbus_Read_Holding_Registers(slave_id, 0x0000, 2, regs)) {
        return false;
    }

    /*
     * Typical sensor scaling:
     *   Humidity    (regs[0]): unit 0.1 %RH  -> divide by 10.0
     *   Temperature (regs[1]): unit 0.1 deg C, signed -> cast to int16_t,
     * divide by 10.0
     */
    data->humidity = (float)(regs[0]) / 10.0f;
    data->temperature = (float)((int16_t)regs[1]) / 10.0f;
    data->valid = true;

    return true;
}


/* -----------------------------------------------------------------------
 * Generic register read – FC03 (Holding) or FC04 (Input)
 * -----------------------------------------------------------------------*/
bool Modbus_Read_Register(
    uint8_t slave_id, uint8_t func_code, uint16_t reg_addr, uint16_t *out_value)
{
    uint8_t req[8];
    uint8_t resp[9]; /* 1+1+1+2+2 = 7 bytes minimum + 2 CRC */
    int resp_len;
    uint16_t crc_calc, crc_recv;
    int retry;

#if defined(_WIN32)
    if (g_port.handle == INVALID_HANDLE_VALUE) {
        fprintf(stderr, "[Modbus] Port not initialized.\n");
        return false;
    }
#else
    if (g_port.fd < 0) {
        fprintf(stderr, "[Modbus] Port not initialized.\n");
        return false;
    }
#endif

    if (!out_value) {
        return false;
    }
    if (func_code != MODBUS_FC_READ_HOLDING_REGS &&
        func_code != MODBUS_FC_READ_INPUT_REGS) {
        fprintf(
            stderr, "[Modbus] Read_Register: invalid func_code 0x%02X\n",
            func_code);
        return false;
    }

    req[0] = slave_id;
    req[1] = func_code;
    req[2] = (reg_addr >> 8) & 0xFF;
    req[3] = reg_addr & 0xFF;
    req[4] = 0x00;
    req[5] = 0x01; /* read 1 register */
    crc_calc = crc16_modbus(req, 6);
    req[6] = crc_calc & 0xFF;
    req[7] = (crc_calc >> 8) & 0xFF;

    for (retry = 0; retry < MODBUS_MAX_RETRY; retry++) {
        if (!modbus_send(req, 8)) {
            fprintf(
                stderr,
                "[Modbus] Write error slave=%u FC=0x%02X addr=0x%04X (retry %d)\n",
                slave_id, func_code, reg_addr, retry + 1);
            modbus_sleep_ms(50);
            continue;
        }

        resp_len = modbus_recv(resp, 7, MODBUS_TIMEOUT_MS);
        if (resp_len < 7) {
            fprintf(
                stderr,
                "[Modbus] Short resp: got=%d expected=7"
                " slave=%u FC=0x%02X addr=0x%04X (retry %d)\n",
                resp_len, slave_id, func_code, reg_addr, retry + 1);
            modbus_sleep_ms(100);
            continue;
        }

        crc_calc = crc16_modbus(resp, 5);
        crc_recv = (uint16_t)(resp[5]) | ((uint16_t)(resp[6]) << 8);
        if (crc_calc != crc_recv) {
            fprintf(
                stderr,
                "[Modbus] CRC mismatch slave=%u FC=0x%02X addr=0x%04X"
                " calc=0x%04X recv=0x%04X (retry %d)\n",
                slave_id, func_code, reg_addr, crc_calc, crc_recv, retry + 1);
            modbus_sleep_ms(100);
            continue;
        }

        if (resp[0] != slave_id || resp[1] != func_code) {
            fprintf(
                stderr,
                "[Modbus] Unexpected ID/FC: expected slave=%u FC=0x%02X,"
                " got slave=%u FC=0x%02X\n",
                slave_id, func_code, resp[0], resp[1]);
            return false;
        }

        *out_value = ((uint16_t)resp[3] << 8) | (uint16_t)resp[4];
        return true;
    }

    return false;
}

/* -----------------------------------------------------------------------
 * Generic bit read – FC01 (Coils) or FC02 (Discrete Inputs)
 * -----------------------------------------------------------------------*/
bool Modbus_Read_Bit(
    uint8_t slave_id, uint8_t func_code, uint16_t bit_addr, uint8_t *out_value)
{
    uint8_t req[8];
    uint8_t resp[8]; /* ID+FC+ByteCount+1byte_data+CRC2 = 6 min */
    int resp_len;
    uint16_t crc_calc, crc_recv;
    int retry;

#if defined(_WIN32)
    if (g_port.handle == INVALID_HANDLE_VALUE) {
        fprintf(stderr, "[Modbus] Port not initialized.\n");
        return false;
    }
#else
    if (g_port.fd < 0) {
        fprintf(stderr, "[Modbus] Port not initialized.\n");
        return false;
    }
#endif

    if (!out_value) {
        return false;
    }
    if (func_code != MODBUS_FC_READ_COILS &&
        func_code != MODBUS_FC_READ_DISCRETE_INPUTS) {
        fprintf(
            stderr, "[Modbus] Read_Bit: invalid func_code 0x%02X\n", func_code);
        return false;
    }

    req[0] = slave_id;
    req[1] = func_code;
    req[2] = (bit_addr >> 8) & 0xFF;
    req[3] = bit_addr & 0xFF;
    req[4] = 0x00;
    req[5] = 0x01; /* read 1 bit */
    crc_calc = crc16_modbus(req, 6);
    req[6] = crc_calc & 0xFF;
    req[7] = (crc_calc >> 8) & 0xFF;

    for (retry = 0; retry < MODBUS_MAX_RETRY; retry++) {
        if (!modbus_send(req, 8)) {
            fprintf(
                stderr,
                "[Modbus] Write error slave=%u FC=0x%02X addr=0x%04X (retry %d)\n",
                slave_id, func_code, bit_addr, retry + 1);
            modbus_sleep_ms(50);
            continue;
        }

        resp_len = modbus_recv(resp, 6, MODBUS_TIMEOUT_MS);
        if (resp_len < 6) {
            fprintf(
                stderr,
                "[Modbus] Short resp: got=%d expected=6"
                " slave=%u FC=0x%02X addr=0x%04X (retry %d)\n",
                resp_len, slave_id, func_code, bit_addr, retry + 1);
            modbus_sleep_ms(100);
            continue;
        }

        crc_calc = crc16_modbus(resp, 4);
        crc_recv = (uint16_t)(resp[4]) | ((uint16_t)(resp[5]) << 8);
        if (crc_calc != crc_recv) {
            fprintf(
                stderr,
                "[Modbus] CRC mismatch slave=%u FC=0x%02X addr=0x%04X"
                " calc=0x%04X recv=0x%04X (retry %d)\n",
                slave_id, func_code, bit_addr, crc_calc, crc_recv, retry + 1);
            modbus_sleep_ms(100);
            continue;
        }

        if (resp[0] != slave_id || resp[1] != func_code) {
            fprintf(
                stderr,
                "[Modbus] Unexpected ID/FC: expected slave=%u FC=0x%02X,"
                " got slave=%u FC=0x%02X\n",
                slave_id, func_code, resp[0], resp[1]);
            return false;
        }

        *out_value = resp[3] & 0x01;
        return true;
    }

    return false;
}

/* -----------------------------------------------------------------------
 * FC06 – Write Single Holding Register
 * -----------------------------------------------------------------------*/
bool Modbus_Write_Register(uint8_t slave_id, uint16_t reg_addr, uint16_t value)
{
    uint8_t req[8];
    uint8_t resp[8];
    int resp_len;
    uint16_t crc_calc, crc_recv;
    int retry;

#if defined(_WIN32)
    if (g_port.handle == INVALID_HANDLE_VALUE) {
        fprintf(stderr, "[Modbus] Port not initialized.\n");
        return false;
    }
#else
    if (g_port.fd < 0) {
        fprintf(stderr, "[Modbus] Port not initialized.\n");
        return false;
    }
#endif

    req[0] = slave_id;
    req[1] = MODBUS_FC_WRITE_SINGLE_REG;
    req[2] = (reg_addr >> 8) & 0xFF;
    req[3] = reg_addr & 0xFF;
    req[4] = (value >> 8) & 0xFF;
    req[5] = value & 0xFF;
    crc_calc = crc16_modbus(req, 6);
    req[6] = crc_calc & 0xFF;
    req[7] = (crc_calc >> 8) & 0xFF;

    for (retry = 0; retry < MODBUS_MAX_RETRY; retry++) {
        if (!modbus_send(req, 8)) {
            fprintf(
                stderr,
                "[Modbus] Write error slave=%u FC=0x06 addr=0x%04X (retry %d)\n",
                slave_id, reg_addr, retry + 1);
            modbus_sleep_ms(50);
            continue;
        }

        /* Echo response: same 8 bytes */
        resp_len = modbus_recv(resp, 8, MODBUS_TIMEOUT_MS);
        if (resp_len < 8) {
            fprintf(
                stderr,
                "[Modbus] Short resp: got=%d expected=8"
                " slave=%u FC=0x06 addr=0x%04X (retry %d)\n",
                resp_len, slave_id, reg_addr, retry + 1);
            modbus_sleep_ms(100);
            continue;
        }

        crc_calc = crc16_modbus(resp, 6);
        crc_recv = (uint16_t)(resp[6]) | ((uint16_t)(resp[7]) << 8);
        if (crc_calc != crc_recv) {
            fprintf(
                stderr,
                "[Modbus] CRC mismatch slave=%u FC=0x06 addr=0x%04X"
                " calc=0x%04X recv=0x%04X (retry %d)\n",
                slave_id, reg_addr, crc_calc, crc_recv, retry + 1);
            modbus_sleep_ms(100);
            continue;
        }

        if (resp[0] == slave_id && resp[1] == MODBUS_FC_WRITE_SINGLE_REG) {
            return true;
        }
    }
    return false;
}

/* -----------------------------------------------------------------------
 * FC05 – Write Single Coil
 * -----------------------------------------------------------------------*/
bool Modbus_Write_Coil(uint8_t slave_id, uint16_t coil_addr, bool on)
{
    uint8_t req[8];
    uint8_t resp[8];
    int resp_len;
    uint16_t crc_calc, crc_recv;
    int retry;
    uint16_t coil_val = on ? 0xFF00 : 0x0000;

#if defined(_WIN32)
    if (g_port.handle == INVALID_HANDLE_VALUE) {
        fprintf(stderr, "[Modbus] Port not initialized.\n");
        return false;
    }
#else
    if (g_port.fd < 0) {
        fprintf(stderr, "[Modbus] Port not initialized.\n");
        return false;
    }
#endif

    req[0] = slave_id;
    req[1] = MODBUS_FC_WRITE_SINGLE_COIL;
    req[2] = (coil_addr >> 8) & 0xFF;
    req[3] = coil_addr & 0xFF;
    req[4] = (coil_val >> 8) & 0xFF;
    req[5] = coil_val & 0xFF;
    crc_calc = crc16_modbus(req, 6);
    req[6] = crc_calc & 0xFF;
    req[7] = (crc_calc >> 8) & 0xFF;

    for (retry = 0; retry < MODBUS_MAX_RETRY; retry++) {
        if (!modbus_send(req, 8)) {
            fprintf(
                stderr,
                "[Modbus] Write error slave=%u FC=0x05 addr=0x%04X (retry %d)\n",
                slave_id, coil_addr, retry + 1);
            modbus_sleep_ms(50);
            continue;
        }

        resp_len = modbus_recv(resp, 8, MODBUS_TIMEOUT_MS);
        if (resp_len < 8) {
            fprintf(
                stderr,
                "[Modbus] Short resp: got=%d expected=8"
                " slave=%u FC=0x05 addr=0x%04X (retry %d)\n",
                resp_len, slave_id, coil_addr, retry + 1);
            modbus_sleep_ms(100);
            continue;
        }

        crc_calc = crc16_modbus(resp, 6);
        crc_recv = (uint16_t)(resp[6]) | ((uint16_t)(resp[7]) << 8);
        if (crc_calc != crc_recv) {
            fprintf(
                stderr,
                "[Modbus] CRC mismatch slave=%u FC=0x05 addr=0x%04X"
                " calc=0x%04X recv=0x%04X (retry %d)\n",
                slave_id, coil_addr, crc_calc, crc_recv, retry + 1);
            modbus_sleep_ms(100);
            continue;
        }

        if (resp[0] == slave_id && resp[1] == MODBUS_FC_WRITE_SINGLE_COIL) {
            return true;
        }
    }
    return false;
}

