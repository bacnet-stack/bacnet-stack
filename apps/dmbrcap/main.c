/**
 * @file
 * @brief Device Management-Backup and Restore tool that generates
 *  a Wireshark PCAP format file from a CreateObject services encoded file.
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date February 2026
 * @copyright SPDX-License-Identifier: GPL-2.0-or-later WITH GCC-exception-2.0
 */
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <time.h>
/* BACnet Stack defines - first */
#include "bacnet/bacdef.h"
#include "bacnet/bacint.h"
#include "bacnet/create_object.h"
#include "bacnet/datetime.h"
#include "bacnet/npdu.h"
#include "bacnet/basic/sys/mstimer.h"
#include "bacnet/basic/sys/filename.h"

/* define our Data Link Type for libPCAP */
#define DLT_CAPTURE_TYPE (1)

static uint8_t MTU_Buffer[1501];
static char Capture_Filename[64] = "dmbr_20260209012345.cap";
static FILE *Capture_File_Handle = NULL; /* stream pointer */
static FILE *Backup_File_Handle = NULL; /* stream pointer */
static long Backup_File_Start_Position;
static long Backup_File_Packet_Counter;

/**
 * @brief Write data to the capture file.
 * @param ptr Pointer to the data to be written
 * @param size Size of each element to write
 * @param nitems Number of items to write
 * @return Number of items successfully written to the capture file
 */
static size_t data_write(const void *ptr, size_t size, size_t nitems)
{
    size_t written = 0;

    if (Capture_File_Handle) {
        written = fwrite(ptr, size, nitems, Capture_File_Handle);
    }

    return written;
}

/**
 * @brief Write header data to the capture file.
 * @param ptr Pointer to the header data to be written
 * @param size Size of each element to write
 * @param nitems Number of items to write
 * @return Number of items successfully written to the capture file
 */
static size_t data_write_header(const void *ptr, size_t size, size_t nitems)
{
    size_t written = 0;

    if (Capture_File_Handle) {
        written = fwrite(ptr, size, nitems, Capture_File_Handle);
    }

    return written;
}

/**
 * @brief Create a new capture filename based on the current date and time.
 *
 * Closes any existing capture file, generates a new filename using the
 * current local date and time, and opens a new capture file for writing.
 */
static void filename_create_new(void)
{
    BACNET_DATE bdate;
    BACNET_TIME btime;
    char *filename = &Capture_Filename[0];
    size_t filename_size = sizeof(Capture_Filename);

    if (Capture_File_Handle) {
        fclose(Capture_File_Handle);
    }
    Capture_File_Handle = NULL;
    datetime_local(&bdate, &btime, NULL, NULL);
    snprintf(
        filename, filename_size, "dmbr_%04d%02d%02d%02d%02d%02d.cap",
        (int)bdate.year, (int)bdate.month, (int)bdate.day, (int)btime.hour,
        (int)btime.min, (int)btime.sec);
    Capture_File_Handle = fopen(filename, "wb");
    if (Capture_File_Handle) {
        fprintf(stdout, "dmbrcap: saving capture to %s\n", filename);
    } else {
        fprintf(
            stderr, "dmbrcap: failed to open %s: %s\n", filename,
            strerror(errno));
    }
}

/**
 * @brief Write the global header to the capture file in libpcap format.
 *
 * Writes the standard libpcap global header including magic number,
 * version information, timezone, timestamp accuracy, snapshot length,
 * and data link type.
 */
static void write_global_header(void)
{
    uint32_t magic_number = 0xa1b2c3d4; /* magic number */
    uint16_t version_major = 2; /* major version number */
    uint16_t version_minor = 4; /* minor version number */
    int32_t thiszone = 0; /* GMT to local correction */
    uint32_t sigfigs = 0; /* accuracy of timestamps */
    uint32_t snaplen = 65535; /* max length of captured packet, in octets */
    uint32_t network = DLT_CAPTURE_TYPE; /* data link type */

    /* create a new file. */
    (void)data_write_header(&magic_number, sizeof(magic_number), 1);
    (void)data_write_header(&version_major, sizeof(version_major), 1);
    (void)data_write_header(&version_minor, sizeof(version_minor), 1);
    (void)data_write_header(&thiszone, sizeof(thiszone), 1);
    (void)data_write_header(&sigfigs, sizeof(sigfigs), 1);
    (void)data_write_header(&snaplen, sizeof(snaplen), 1);
    (void)data_write_header(&network, sizeof(network), 1);
    fflush(Capture_File_Handle);
}

/**
 * @brief Write a packet record to the capture file in libpcap format.
 *
 * Writes a packet record with timestamp and length information followed by
 * the packet data to the capture file in libpcap format.
 * @param packet Pointer to the packet data to write
 * @param packet_len Length of the packet data in bytes
 */
static void write_received_packet(uint8_t *packet, size_t packet_len)
{
    uint32_t ts_msec = 0; /* timestamp milliseconds */
    uint32_t ts_sec = 0; /* timestamp seconds */
    uint32_t ts_usec = 0; /* timestamp microseconds */
    uint32_t incl_len = 0; /* number of octets of packet saved in file */
    uint32_t orig_len = 0; /* actual length of packet */

    ts_msec = mstimer_now();
    ts_sec = ts_msec / 1000;
    ts_usec = (ts_msec % 1000) * 1000;
    (void)data_write(&ts_sec, sizeof(ts_sec), 1);
    (void)data_write(&ts_usec, sizeof(ts_usec), 1);
    incl_len = packet_len;
    (void)data_write(&incl_len, sizeof(incl_len), 1);
    orig_len = packet_len;
    (void)data_write(&orig_len, sizeof(orig_len), 1);
    (void)data_write(packet, packet_len, 1);
}

/**
 * @brief Open a backup file for reading.
 *
 * Opens an existing backup file for reading and initializes the file
 * position tracking for packet extraction.
 * @param filename Path to the backup file to open
 * @return true if the file was successfully opened, false otherwise
 */
static bool open_backup_file(const char *filename)
{
    /* open existing file. */
    Backup_File_Handle = fopen(filename, "rb");
    if (Backup_File_Handle) {
        fprintf(stdout, "dmbrcap: reading backup from %s\n", filename);
        Backup_File_Start_Position = 0;
        return true;
    }
    fprintf(
        stderr, "dmbrcap: failed to open %s: %s\n", filename, strerror(errno));

    return false;
}

/**
 * @brief Extract and convert the next packet from the backup file.
 *
 * Reads the next CreateObject service request from the backup file,
 * encapsulates it in BACnet NPDU and APDU headers with Ethernet framing,
 * and writes the complete packet to the capture file. Advances the file
 * position for the next packet extraction.
 * @return Length of the packet written, or 0 if no more packets are available
 */
static size_t backup_file_packet(void)
{
    BACNET_NPDU_DATA npdu_data = { 0 };
    size_t len = 0, packet_len = 0, apdu_len = 0;
    int decoded_len = 0;
    uint8_t apdu[1500] = { 0 };

    /* Ethernet SNAP Encoding*/
    /* dest MAC */
    MTU_Buffer[0] = 0xFF;
    MTU_Buffer[1] = 0xFF;
    MTU_Buffer[2] = 0xFF;
    MTU_Buffer[3] = 0xFF;
    MTU_Buffer[4] = 0xFF;
    MTU_Buffer[5] = 0xFF;
    /* source MAC */
    MTU_Buffer[6] = 0xFF;
    MTU_Buffer[7] = 0xFF;
    MTU_Buffer[8] = 0xFF;
    MTU_Buffer[9] = 0xFF;
    MTU_Buffer[10] = 0xFF;
    MTU_Buffer[11] = 0xFF;
    /* length - 12, 13 */
    /* Logical-Link Control SNAP */
    /* DSAP for SNAP */
    MTU_Buffer[14] = 0x82;
    /* SSAP for SNAP */
    MTU_Buffer[15] = 0x82;
    /* Control Field for SNAP */
    MTU_Buffer[16] = 0x03;
    /* BACnet NPDU */
    len = npdu_encode_pdu(&MTU_Buffer[17], NULL, NULL, &npdu_data);
    packet_len = 17 + len;
    /* BACnet APDU header */
    MTU_Buffer[packet_len++] = PDU_TYPE_CONFIRMED_SERVICE_REQUEST;
    MTU_Buffer[packet_len++] = encode_max_segs_max_apdu(0, MAX_APDU);
    MTU_Buffer[packet_len++] = Backup_File_Packet_Counter % 256;
    MTU_Buffer[packet_len++] = SERVICE_CONFIRMED_CREATE_OBJECT;
    /* BACnet APDU service data */
    if (Backup_File_Handle) {
        (void)fseek(Backup_File_Handle, Backup_File_Start_Position, SEEK_SET);
        apdu_len =
            fread(apdu, sizeof(uint8_t), sizeof(apdu), Backup_File_Handle);
        if (apdu_len > 0) {
            decoded_len =
                create_object_decode_service_request(apdu, apdu_len, NULL);
            if (decoded_len > 0) {
                apdu_len = decoded_len;
                Backup_File_Start_Position += decoded_len;
                if ((packet_len + apdu_len) > sizeof(MTU_Buffer)) {
                    apdu_len = sizeof(MTU_Buffer) - packet_len;
                }
                memcpy(&MTU_Buffer[packet_len], apdu, apdu_len);
                packet_len += apdu_len;
                /* Ethernet length is data only - not address or length bytes */
                encode_unsigned16(&MTU_Buffer[12], packet_len - 14);
                write_received_packet(MTU_Buffer, packet_len);
            } else {
                packet_len = 0;
            }
        } else {
            packet_len = 0;
        }
    }

    return packet_len;
}

/**
 * @brief Clean up and close all open file handles.
 *
 * Flushes and closes the capture file and backup file handles, preparing
 * the program for exit. This function is registered as an exit handler.
 */
static void cleanup(void)
{
    if (Capture_File_Handle) {
        fflush(Capture_File_Handle);
        fclose(Capture_File_Handle);
    }
    Capture_File_Handle = NULL;
    if (Backup_File_Handle) {
        fclose(Backup_File_Handle);
    }
    Backup_File_Handle = NULL;
}

/**
 * @brief Print the command-line usage information.
 * @param filename The name of the program (typically argv[0])
 */
static void print_usage(const char *filename)
{
    printf("Usage: %s <filename>", filename);
    printf(" [--version][--help]\n");
}

/**
 * @brief Print detailed help information for the program.
 * @param filename The name of the program (typically argv[0])
 */
static void print_help(const char *filename)
{
    printf(
        "%s <filename>\n"
        "convert a backup file into a capture file.\n",
        filename);
    printf("\n");
}

/**
 * @brief Main entry point for the dmbrcap utility.
 *
 * Processes command-line arguments, opens the backup file, initializes the
 * timer system, creates a new capture file with libpcap headers, reads all
 * packets from the backup file and converts them to libpcap format, then
 * closes all files and exits.
 * @param argc Number of command-line arguments
 * @param argv Array of command-line argument strings
 * @return 0 on success, 1 if backup file cannot be opened or is not provided
 */
int main(int argc, char *argv[])
{
    const char *filename = NULL;
    int argi = 0;

    /* decode any command line parameters */
    filename = filename_remove_path(argv[0]);
    for (argi = 1; argi < argc; argi++) {
        if (strcmp(argv[argi], "--help") == 0) {
            print_usage(filename);
            print_help(filename);
            return 0;
        }
        if (strcmp(argv[argi], "--version") == 0) {
            printf("dmbrcap 1.0.0\n");
            printf("Copyright (C) 2026 by Steve Karg\n"
                   "This is free software; see the source for copying "
                   "conditions.\n"
                   "There is NO warranty; not even for MERCHANTABILITY or\n"
                   "FITNESS FOR A PARTICULAR PURPOSE.\n");
            return 0;
        }
        if (!open_backup_file(argv[argi])) {
            return 1;
        }
    }
    if (!Backup_File_Handle) {
        print_usage(filename);
        return 1;
    }
    atexit(cleanup);
    mstimer_init();
    filename_create_new();
    write_global_header();
    while (backup_file_packet() > 0) {
        Backup_File_Packet_Counter++;
    }
    if (Backup_File_Packet_Counter) {
        fprintf(
            stdout, "dmbrcap: wrote %u packets\n",
            (unsigned)Backup_File_Packet_Counter);
    }

    return 0;
}
