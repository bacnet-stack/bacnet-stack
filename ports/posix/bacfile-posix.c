/**
 * @file
 * @brief A POSIX BACnet File Object implementation.
 * @author Steve Karg
 * @date 2005
 * @copyright SPDX-License-Identifier: MIT
 */
#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
/* BACnet Stack defines - first */
#include "bacnet/bacdef.h"
#include "bacnet/basic/object/bacfile.h"
#include "bacnet/basic/sys/debug.h"
#include "bacnet/basic/sys/filename.h"
/* me! */
#include "bacfile-posix.h"

#ifndef BACNET_FILE_POSIX_RECORD_SIZE
#define BACNET_FILE_POSIX_RECORD_SIZE MAX_OCTET_STRING_BYTES
#endif

/**
 * @brief Determines the file size for a given file
 * @param  pFile - file handle
 * @return  file size in bytes, or 0 if not found
 */
static long fsize(FILE *pFile)
{
    long size = 0;
    long origin = 0;

    if (pFile) {
        origin = ftell(pFile);
        fseek(pFile, 0L, SEEK_END);
        size = ftell(pFile);
        fseek(pFile, origin, SEEK_SET);
    }
    return (size);
}

/**
 * @brief Determines the file size for a given file
 * @param  pathname - name of the file to get the size for
 * @return  file size in bytes, or 0 if not found
 */
size_t bacfile_posix_file_size(const char *pathname)
{
    FILE *pFile = NULL;
    long file_position = 0;
    size_t file_size = 0;

    if (filename_path_valid(pathname)) {
        pFile = fopen(pathname, "rb");
        if (pFile) {
            file_position = fsize(pFile);
            if (file_position >= 0) {
                file_size = (size_t)file_position;
            }
            fclose(pFile);
        } else {
            debug_printf_stderr("Failed to open %s for reading!\n", pathname);
        }
    }

    return file_size;
}

/**
 * @brief Sets the file size property value
 * @param pathname - name of the file to set the size for
 * @param file_size - value of the file size property
 * @return true if file size is writable
 */
bool bacfile_posix_file_size_set(const char *pathname, size_t file_size)
{
    bool status = false;

    (void)pathname; /* unused parameter */
    (void)file_size; /* unused parameter */
    /* FIXME: add clever POSIX file stuff here */

    return status;
}

/**
 * @brief Reads stream data from a file
 * @param pathname - name of the file to read from
 * @param fileStartPosition - starting position in the file
 * @param fileData - data buffer to read into
 * @param fileDataLen - size of the data buffer
 * @return number of bytes read, or 0 if not successful
 */
size_t bacfile_posix_read_stream_data(
    const char *pathname,
    int32_t fileStartPosition,
    uint8_t *fileData,
    size_t fileDataLen)
{
    FILE *pFile = NULL;
    size_t len = 0;

    if (filename_path_valid(pathname)) {
        pFile = fopen(pathname, "rb");
        if (pFile) {
            (void)fseek(pFile, fileStartPosition, SEEK_SET);
            len = fread(fileData, 1, fileDataLen, pFile);
            fclose(pFile);
        } else {
            debug_printf_stderr("Failed to open %s for reading!\n", pathname);
        }
    }

    return len;
}

/**
 * @brief Writes stream data to a file
 * @param pathname - name of the file to write to
 * @param fileStartPosition - starting position in the file
 * @param fileData - data buffer to write from
 * @param fileDataLen - size of the data buffer
 * @return number of bytes written, or 0 if not successful
 */
size_t bacfile_posix_write_stream_data(
    const char *pathname,
    int32_t fileStartPosition,
    const uint8_t *fileData,
    size_t fileDataLen)
{
    size_t bytes_written = 0;
    FILE *pFile = NULL;

    if (filename_path_valid(pathname)) {
        if (fileStartPosition == 0) {
            /* open the file as a clean slate when starting at 0 */
            pFile = fopen(pathname, "wb");
        } else if (fileStartPosition == -1) {
            /* If 'File Start Position' parameter has the special
               value -1, then the write operation shall be treated
               as an append to the current end of file. */
            pFile = fopen(pathname, "ab+");
        } else {
            /* open for update */
            pFile = fopen(pathname, "rb+");
        }
        if (pFile) {
            if (fileStartPosition != -1) {
                (void)fseek(pFile, fileStartPosition, SEEK_SET);
            }
            bytes_written = fwrite(fileData, 1, fileDataLen, pFile);
            fclose(pFile);
        } else {
            debug_printf_stderr("Failed to open %s for writing!\n", pathname);
        }
    }

    return bytes_written;
}

/**
 * @brief Writes record data to a file
 * @param pathname - name of the file to write to
 * @param fileStartRecord - starting record in the file
 * @param fileIndexRecord - index of the record to read
 * @param fileData - data buffer to read into
 * @param fileDataLen - size of the data buffer
 * @return true if successful, false otherwise
 */
bool bacfile_posix_write_record_data(
    const char *pathname,
    int32_t fileStartRecord,
    size_t fileIndexRecord,
    const uint8_t *fileData,
    size_t fileDataLen)
{
    bool status = false;
    FILE *pFile = NULL;
    uint32_t i = 0;
    char dummy_data[BACNET_FILE_POSIX_RECORD_SIZE];
    const char *pData = NULL;
    size_t fileSeekRecord = 0;

    if (filename_path_valid(pathname)) {
        if (fileStartRecord == 0) {
            /* open the file as a clean slate when starting at 0 */
            pFile = fopen(pathname, "wb");
            fileSeekRecord = fileIndexRecord;
        } else if (fileStartRecord == -1) {
            /* If 'File Start Record' parameter has the special
               value -1, then the write operation shall be treated
               as an append to the current end of file. */
            pFile = fopen(pathname, "ab+");
            fileSeekRecord = fileIndexRecord;
        } else {
            /* open for update */
            pFile = fopen(pathname, "rb+");
            fileSeekRecord = fileStartRecord + fileIndexRecord;
        }
        if (pFile) {
            if ((fileStartRecord != -1) && (fileSeekRecord > 0)) {
                /* seek to the start record */
                for (i = 0; i < fileSeekRecord; i++) {
                    pData = fgets(&dummy_data[0], sizeof(dummy_data), pFile);
                    if ((pData == NULL) || feof(pFile)) {
                        break;
                    }
                }
            }
            if (fwrite(fileData, fileDataLen, 1, pFile) == 1) {
                status = true;
            }
            fclose(pFile);
        } else {
            debug_printf_stderr("Failed to open %s for writing!\n", pathname);
        }
    }

    return status;
}

/**
 * @brief Reads record data from a file
 * @param pathname - name of the file to read from
 * @param fileStartRecord - starting record in the file
 * @param fileIndexRecord - index of the record to read
 * @param fileData - data buffer to read into
 * @param fileDataLen - size of the data buffer
 * @return true if successful, false otherwise
 */
bool bacfile_posix_read_record_data(
    const char *pathname,
    int32_t fileStartRecord,
    size_t fileIndexRecord,
    uint8_t *fileData,
    size_t fileDataLen)
{
    bool status = false;
    FILE *pFile = NULL;
    uint32_t i = 0;
    char dummy_data[BACNET_FILE_POSIX_RECORD_SIZE] = { 0 };
    const char *pData = NULL;
    size_t fileSeekRecord = 0;

    if (filename_path_valid(pathname)) {
        pFile = fopen(pathname, "rb");
        if (pFile) {
            fileSeekRecord = fileStartRecord + fileIndexRecord;
            /* seek to the start record */
            for (i = 0; i < fileSeekRecord; i++) {
                pData = fgets(&dummy_data[0], sizeof(dummy_data), pFile);
                if ((pData == NULL) || feof(pFile)) {
                    break;
                }
            }
            if ((i == fileSeekRecord) && (fileDataLen <= sizeof(dummy_data))) {
                /* copy the record data */
                memmove(fileData, &dummy_data[0], fileDataLen);
                status = true;
            }
            fclose(pFile);
        } else {
            debug_printf_stderr("Failed to open %s for reading!\n", pathname);
        }
    }

    return status;
}

/**
 * @brief Initializes the object data
 */
void bacfile_posix_init(void)
{
    bacfile_write_stream_data_callback_set(bacfile_posix_write_stream_data);
    bacfile_read_stream_data_callback_set(bacfile_posix_read_stream_data);
    bacfile_write_record_data_callback_set(bacfile_posix_write_record_data);
    bacfile_read_record_data_callback_set(bacfile_posix_read_record_data);
    bacfile_file_size_callback_set(bacfile_posix_file_size);
    bacfile_file_size_set_callback_set(bacfile_posix_file_size_set);
}
