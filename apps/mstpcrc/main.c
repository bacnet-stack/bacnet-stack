/**************************************************************************
 *
 * Copyright (C) 2012 Steve Karg
 *
 * SPDX-License-Identifier: GPL-2.0-or-later WITH GCC-exception-2.0
 *
 *********************************************************************/
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <ctype.h>
/* BACnet Stack defines - first */
#include "bacnet/bacdef.h"
/* BACnet Stack API */
#include "bacnet/basic/sys/mstimer.h"
#include "bacnet/datalink/crc.h"
#include "bacnet/version.h"
/* OS specific include*/
#include "bacport.h"

#ifndef max
#define max(a, b) (((a)(b)) ? (a) : (b))
#define min(a, b) (((a) < (b)) ? (a) : (b))
#endif

/* buffer needed by CRC functions */
static uint8_t CRC_Buffer[1512];
static unsigned CRC_Buffer_Len = 0;
/* flags needed for options */
static bool ASCII_Decimal = false;
static unsigned CRC_Size = 8;
/* save to capture file for viewing in Wireshark */
static bool MSTP_Cap = false;
static bool MSTP_Text_File = false;
static char Capture_Filename[64] = "mstp_20090123091200.cap";
static FILE *pFile = NULL; /* stream pointer */
static FILE *pText_File = NULL; /* stream pointer */

/******************************************************************
 * DESCRIPTION:  Takes one of the arguments passed by the main function
 *               and converts it into a buffer value.
 *               argi - single argument in string form.
 * RETURN:       nothing
 * NOTES:        none
 ******************************************************************/
static void Parse_Number(char *argi)
{
    long long_value = 0;

    if (ASCII_Decimal) {
        long_value = strtol(argi, NULL, 10);
    } else {
        long_value = strtol(argi, NULL, 16);
    }
    CRC_Buffer[CRC_Buffer_Len] = (uint8_t)long_value;
    CRC_Buffer_Len++;
}

/******************************************************************
 * DESCRIPTION:  Takes one of the arguments passed by the main function
 *               and sets flags if it matches one of the predefined args.
 * PARAMETERS:   argc (IN) number of arguments.
 *               argv (IN) an array of arguments in string form.
 * RETURN:       number of arguments parsed
 * NOTES:        none
 ******************************************************************/
static void Parse_Arguments(int argc, char *argv[])
{
    int i = 0;
    long long_value = 0;

    for (i = 1; i < argc; i++) {
        if (argv[i][0] == '-') {
            /* numeric arguments */
            if (isdigit(argv[i][1])) {
                long_value = strtol(&argv[i][1], NULL, 10);
                /* dash arguments */
                switch (long_value) {
                    case 8:
                        CRC_Size = 8;
                        break;
                    case 16:
                        CRC_Size = 16;
                        break;
                    case 32:
                        CRC_Size = 32;
                        break;
                    default:
                        break;
                }
            }
            /* dash arguments */
            switch (argv[i][1]) {
                case 'h':
                case 'H':
                    ASCII_Decimal = false;
                    break;
                case 'd':
                case 'D':
                    ASCII_Decimal = true;
                    break;
                case 'm':
                case 'M':
                    MSTP_Cap = true;
                    break;
                case 'f':
                case 'F':
                    MSTP_Text_File = true;
                    break;
                default:
                    break;
            }
        } else {
            if (MSTP_Text_File) {
                /* open existing file. */
                pText_File = fopen(argv[i], "r");
            } else {
                /* should be number values here */
                Parse_Number(argv[i]);
            }
        }
    }
}

static void filename_create(char *filename, size_t filesize)
{
    time_t my_time;
    struct tm *today;

    if (filename) {
        my_time = time(NULL);
        today = localtime(&my_time);
        snprintf(
            filename, filesize, "mstp_%04d%02d%02d%02d%02d%02d.cap",
            1900 + today->tm_year, 1 + today->tm_mon, today->tm_mday,
            today->tm_hour, today->tm_min, today->tm_sec);
    }
}

/* write packet to file in libpcap format */
static void write_global_header(const char *filename)
{
    uint32_t magic_number = 0xa1b2c3d4; /* magic number */
    uint16_t version_major = 2; /* major version number */
    uint16_t version_minor = 4; /* minor version number */
    int32_t thiszone = 0; /* GMT to local correction */
    uint32_t sigfigs = 0; /* accuracy of timestamps */
    uint32_t snaplen = 65535; /* max length of captured packets, in octets */
    uint32_t network = 165; /* data link type - BACNET_MS_TP */

    /* create a new file. */
    pFile = fopen(filename, "wb");
    if (pFile) {
        (void)fwrite(&magic_number, sizeof(magic_number), 1, pFile);
        (void)fwrite(&version_major, sizeof(version_major), 1, pFile);
        (void)fwrite(&version_minor, sizeof(version_minor), 1, pFile);
        (void)fwrite(&thiszone, sizeof(thiszone), 1, pFile);
        (void)fwrite(&sigfigs, sizeof(sigfigs), 1, pFile);
        (void)fwrite(&snaplen, sizeof(snaplen), 1, pFile);
        (void)fwrite(&network, sizeof(network), 1, pFile);
        fflush(pFile);
        fprintf(stdout, "mstpcap: saving capture to %s\n", filename);
    } else {
        fprintf(
            stderr, "mstpcap[header]: failed to open %s: %s\n", filename,
            strerror(errno));
    }
}

static void write_received_packet(uint8_t *buffer, unsigned length)
{
    uint32_t ts_sec; /* timestamp seconds */
    uint32_t ts_usec; /* timestamp microseconds */
    uint32_t incl_len; /* number of octets of packet saved in file */
    uint32_t orig_len; /* actual length of packet */
    struct timeval tv;

    if (pFile) {
        gettimeofday(&tv, NULL);
        ts_sec = tv.tv_sec;
        ts_usec = tv.tv_usec;
        (void)fwrite(&ts_sec, sizeof(ts_sec), 1, pFile);
        (void)fwrite(&ts_usec, sizeof(ts_usec), 1, pFile);
        orig_len = incl_len = length;
        (void)fwrite(&incl_len, sizeof(incl_len), 1, pFile);
        (void)fwrite(&orig_len, sizeof(orig_len), 1, pFile);
        (void)fwrite(buffer, length, 1, pFile);
    } else {
        fprintf(
            stderr, "mstpcrc[packet]: failed to open %s: %s\n",
            Capture_Filename, strerror(errno));
    }
}

static void Write_Pcap(uint8_t *buffer, unsigned length)
{
    filename_create(&Capture_Filename[0], sizeof(Capture_Filename));
    write_global_header(&Capture_Filename[0]);
    write_received_packet(buffer, length);
    if (pFile) {
        fclose(pFile);
    }
}
/* hold 3 ASCII characters per byte of data */
static char Text_Buffer[1024 * 3];
static void Process_Text_File(void)
{
    char *argi = NULL;

    filename_create(&Capture_Filename[0], sizeof(Capture_Filename));
    write_global_header(&Capture_Filename[0]);
    while (fgets(Text_Buffer, sizeof(Text_Buffer), pText_File)) {
        CRC_Buffer_Len = 0;
        do {
            if (!argi) {
                argi = strtok(Text_Buffer, " ");
            } else {
                argi = strtok(NULL, " ");
            }
            if (argi) {
                Parse_Number(argi);
            }
        } while (argi);
        write_received_packet(CRC_Buffer, CRC_Buffer_Len);
    }
    if (pFile) {
        fclose(pFile);
    }
    if (pText_File) {
        fclose(pText_File);
    }
}

/* simple program to CRC the data and print the CRC */
int main(int argc, char *argv[])
{
    /* accumulates the crc value */
    uint8_t crc8 = 0xff;
    uint16_t crc16 = 0xffff;
    unsigned i = 0;

    /* initialize our interface */
    if ((argc > 1) && (strcmp(argv[1], "--help") == 0)) {
        printf(
            "mstpcrc [options] <05 03 01 0D...>\r\n"
            "perform MS/TP CRC on data bytes.\r\n"
            "options:\r\n"
            "[-x] interprete the arguments as ascii hex (default)\r\n"
            "[-d] interprete the argument as ascii decimal\r\n"
            "[-m] Write the bytes to Wireshark capture file\r\n"
            "[-8] calculate the MS/TP 8-bit Header CRC (default)\r\n"
            "[-16] calculate the MS/TP 16-bit Data CRC\r\n"
            "[-32] calculate the MS/TP 32-bit Extended Frame CRC\r\n"
            "[-f filename] read MS/TP capture data from text file\r\n"
            "Note: MS/TP Header CRC does not include the 55 FF preamble.\r\n");
        return 0;
    }
    if ((argc > 1) && (strcmp(argv[1], "--version") == 0)) {
        printf("mstpcap %s\r\n", BACNET_VERSION_TEXT);
        printf(
            "Copyright (C) 2012 by Steve Karg\r\n"
            "This is free software; see the source for copying conditions.\r\n"
            "There is NO warranty; not even for MERCHANTABILITY or\r\n"
            "FITNESS FOR A PARTICULAR PURPOSE.\r\n");
        return 0;
    }
    Parse_Arguments(argc, argv);
    if (MSTP_Text_File) {
        Process_Text_File();
    } else if (CRC_Buffer_Len) {
        if (MSTP_Cap) {
            Write_Pcap(CRC_Buffer, CRC_Buffer_Len);
        } else {
            for (i = 0; i < CRC_Buffer_Len; i++) {
                if (CRC_Size == 8) {
                    crc8 = CRC_Calc_Header(CRC_Buffer[i], crc8);
                } else if (CRC_Size == 16) {
                    crc16 = CRC_Calc_Data(CRC_Buffer[i], crc16);
                }
                if (ASCII_Decimal) {
                    printf("%u\r\n", (unsigned)CRC_Buffer[i]);
                } else {
                    printf("0x%02X\r\n", CRC_Buffer[i]);
                }
            }
            if (CRC_Size == 8) {
                crc8 = ~crc8;
                if (ASCII_Decimal) {
                    printf("%u Header CRC\r\n", (unsigned)crc8);
                } else {
                    printf("0x%02X Header CRC\r\n", crc8);
                }
            } else if (CRC_Size == 16) {
                crc16 = ~crc16;
                if (ASCII_Decimal) {
                    printf("%u Data CRC\r\n", (unsigned)(crc16 & 0xFF));
                    printf("%u Data CRC\r\n", (unsigned)(crc16 >> 8));
                } else {
                    printf("0x%02X Data CRC\r\n", (crc16 & 0xFF));
                    printf("0x%02X Data CRC\r\n", (crc16 >> 8));
                }
            }
        }
    }

    return 1;
}
