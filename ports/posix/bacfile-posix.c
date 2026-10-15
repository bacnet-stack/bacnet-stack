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
/* me! */
#include "bacfile-posix.h"

/* restrict file paths */
#ifndef BACNET_FILE_POSIX_PATH_RESTRICTED
#define BACNET_FILE_POSIX_PATH_RESTRICTED 1
#endif

/**
 * @brief Validate if pathname is valid by checking for patterns
 *  such as relative paths and absolute paths which are prohibited.
 * @param pathname Path to validate
 * @return true if valid, false if not
 */
bool bacfile_posix_file_path_valid(const char *pathname)
{
    int path_len;

    if (!pathname) {
        return false;
    }
    if (pathname[0] == 0) {
        return false;
    }
#if BACNET_FILE_POSIX_PATH_RESTRICTED
    /* check for relative directory patterns */
    if (strstr(pathname, "..") != NULL) {
        debug_printf_stderr("Relative paths are prohibited: %s\n", pathname);
        return false;
    }
    /* check for absolute paths */
    if (pathname[0] == '/') {
        debug_printf_stderr("Absolute paths are prohibited: %s\n", pathname);
        return false;
    }
    /* check for Windows drive letters (should be relative paths only) */
    path_len = strlen(pathname);
    if (path_len >= 2 && pathname[1] == ':') {
        debug_printf_stderr(
            "Windows drive letters are prohibited: %s\n", pathname);
        return false;
    }

    /* check for consecutive path separators */
    if (strstr(pathname, "//") != NULL || strstr(pathname, "\\\\") != NULL) {
        debug_printf_stderr(
            "Consecutive path separators are prohibited: %s\n", pathname);
        return false;
    }

    /* check for path components that are just dots */
    if (strstr(pathname, "/./") != NULL || strstr(pathname, "\\./") != NULL ||
        strstr(pathname, "/.\\") != NULL || strstr(pathname, "\\.\\") != NULL) {
        debug_printf_stderr(
            "Current directory references are prohibited: %s\n", pathname);
        return false;
    }
#endif

    return true;
}

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

    if (bacfile_posix_file_path_valid(pathname)) {
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

    if (bacfile_posix_file_path_valid(pathname)) {
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

    if (bacfile_posix_file_path_valid(pathname)) {
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
 * @brief Copy or skip one newline-terminated record
 * @param src - source file
 * @param dst - destination file, or NULL to skip the record
 * @return true if a record was found in the source
 * @note The destination record is always newline-terminated.
 */
static bool record_line_copy(FILE *src, FILE *dst)
{
    int c;
    bool found = false;

    while ((c = getc(src)) != EOF) {
        found = true;
        if (c == '\n') {
            break;
        }
        if (dst) {
            (void)putc(c, dst);
        }
    }
    if (found && dst) {
        (void)putc('\n', dst);
    }

    return found;
}

/**
 * @brief Determines the number of records in a file
 * @param pathname - name of the file
 * @return number of records, or 0 if the file is not found
 */
size_t bacfile_posix_record_count(const char *pathname)
{
    FILE *pFile;
    size_t count = 0;

    if (bacfile_posix_file_path_valid(pathname)) {
        pFile = fopen(pathname, "rb");
        if (pFile) {
            while (record_line_copy(pFile, NULL)) {
                count++;
            }
            fclose(pFile);
        }
    }

    return count;
}

/**
 * @brief Write a newline-terminated record at the current file position
 * @return true if successful
 */
static bool
record_write(FILE *pFile, const uint8_t *fileData, size_t fileDataLen)
{
    if ((fileDataLen > 0) &&
        (fwrite(fileData, 1, fileDataLen, pFile) != fileDataLen)) {
        return false;
    }

    return putc('\n', pFile) != EOF;
}

/**
 * @brief Replace a record in a file, padding with empty records
 *  if the file has fewer records than the target record.
 * @param pathname - name of the file to write to
 * @param target - record number, 0..N; fails if padding would exceed
 *  BACNET_FILE_POSIX_MAX_PAD_RECORDS
 * @param fileData - record data
 * @param fileDataLen - size of the record data
 * @return true if successful
 * @note Builds the new file in a temporary file using only C stdio,
 *  and then replaces the original file, so a failure leaves the
 *  original file unchanged. A missing file is created.
 */
static bool record_replace(
    const char *pathname,
    size_t target,
    const uint8_t *fileData,
    size_t fileDataLen)
{
    bool status = true;
    FILE *src;
    FILE *dst;
    char *tmpname;
    size_t records = 0;
    size_t i;
    size_t len;
    uint8_t buffer[256];

    tmpname = malloc(strlen(pathname) + sizeof(".tmp"));
    if (!tmpname) {
        return false;
    }
    strcpy(tmpname, pathname);
    strcat(tmpname, ".tmp");
    /* a missing backing file is treated as an empty file */
    src = fopen(pathname, "rb");
    dst = fopen(tmpname, "wb");
    if (!dst) {
        debug_printf_stderr("Failed to open %s for writing!\n", tmpname);
        if (src) {
            fclose(src);
        }
        free(tmpname);
        return false;
    }
    if (src) {
        /* copy the records before the target record */
        while ((records < target) && record_line_copy(src, dst)) {
            records++;
        }
    }
    if (records < target) {
        /* pad with empty records up to the target */
        if ((target - records) > BACNET_FILE_POSIX_MAX_PAD_RECORDS) {
            status = false;
        }
        for (i = records; status && (i < target); i++) {
            status = (putc('\n', dst) != EOF);
        }
    } else if (src) {
        /* skip the replaced record */
        (void)record_line_copy(src, NULL);
    }
    status = status && record_write(dst, fileData, fileDataLen);
    if (status && src) {
        /* copy the records after the target record */
        while ((len = fread(buffer, 1, sizeof(buffer), src)) > 0) {
            if (fwrite(buffer, 1, len, dst) != len) {
                status = false;
                break;
            }
        }
        if (ferror(src)) {
            status = false;
        }
    }
    if (src) {
        fclose(src);
    }
    if (fclose(dst) != 0) {
        status = false;
    }
    if (status && (rename(tmpname, pathname) != 0)) {
        /* some platforms cannot rename over an existing file */
        status = (remove(pathname) == 0) && (rename(tmpname, pathname) == 0);
    }
    if (!status) {
        (void)remove(tmpname);
    }
    free(tmpname);

    return status;
}

/**
 * @brief Append a record to the end of a file
 * @param pathname - name of the file to write to
 * @param fileData - record data
 * @param fileDataLen - size of the record data
 * @return true if successful
 */
static bool
record_append(const char *pathname, const uint8_t *fileData, size_t fileDataLen)
{
    bool status = false;
    FILE *pFile;
    long size;
    int c;

    pFile = fopen(pathname, "ab+");
    if (!pFile) {
        debug_printf_stderr("Failed to open %s for writing!\n", pathname);
        return false;
    }
    if (fseek(pFile, 0L, SEEK_END) == 0) {
        size = ftell(pFile);
        status = true;
        if (size > 0) {
            /* terminate an unterminated last record */
            if (fseek(pFile, -1L, SEEK_END) == 0) {
                c = getc(pFile);
                if ((c != '\n') && (c != EOF)) {
                    if ((fseek(pFile, 0L, SEEK_END) != 0) ||
                        (putc('\n', pFile) == EOF)) {
                        status = false;
                    }
                }
            }
            if (fseek(pFile, 0L, SEEK_END) != 0) {
                status = false;
            }
        }
    }
    status = status && record_write(pFile, fileData, fileDataLen);
    if (fclose(pFile) != 0) {
        status = false;
    }

    return status;
}

/**
 * @brief Writes record data to a file
 * @param pathname - name of the file to write to
 * @param fileStartRecord - starting record in the file, 0 is the first record.
 *  If 'File Start Record' parameter has the special
 *  value -1, then the write operation shall be treated
 *  as an append to the current end of file,
 *  and fileIndexRecord can be ignored.
 *  If the parameter exceeds the actual number of records,
 *  the file is extended with empty records, up to
 *  BACNET_FILE_POSIX_MAX_PAD_RECORDS, otherwise the write fails.
 * @param fileIndexRecord - index of the record to write
 * @param fileData - record data to write; one trailing newline is ignored
 * @param fileDataLen - size of the record data
 * @return true if successful, false otherwise
 * @note The file format is one newline-terminated record per line.
 *  A record that contains a newline other than a single trailing one
 *  cannot be stored.
 */
bool bacfile_posix_write_record_data(
    const char *pathname,
    int32_t fileStartRecord,
    size_t fileIndexRecord,
    const uint8_t *fileData,
    size_t fileDataLen)
{
    if (!bacfile_posix_file_path_valid(pathname) || (fileStartRecord < -1)) {
        return false;
    }
    if (fileDataLen > 0) {
        if (!fileData) {
            return false;
        }
        /* a single trailing newline is ignored, since one is always added */
        if (fileData[fileDataLen - 1] == '\n') {
            fileDataLen--;
        }
        if ((fileDataLen > 0) && memchr(fileData, '\n', fileDataLen)) {
            return false;
        }
    }
    if (fileStartRecord == -1) {
        /* If 'File Start Record' parameter has the special
           value -1, then the write operation shall be treated
           as an append to the current end of file. */
        return record_append(pathname, fileData, fileDataLen);
    }

    return record_replace(
        pathname, (size_t)fileStartRecord + fileIndexRecord, fileData,
        fileDataLen);
}

/**
 * @brief Reads record data from a file
 * @param pathname - name of the file to read from
 * @param fileStartRecord - starting record in the file, 0 is the first record.
 *  If the 'File Start Record' parameter is less than 0,
 *  then an error is returned.
 * @param fileIndexRecord - index of the record to read
 * @param fileData - data buffer to read into
 * @param fileDataLen - size of the data buffer
 * @return true if the record exists, false if it is
 *  at or past the end of the file, or on error
 * @note The record is returned without its newline terminator, and the
 *  remainder of the buffer is zero filled. A record longer than the
 *  buffer is truncated.
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
    size_t i;
    size_t target;
    size_t len = 0;
    int c;

    if ((fileStartRecord < 0) || !fileData ||
        !bacfile_posix_file_path_valid(pathname)) {
        return false;
    }
    pFile = fopen(pathname, "rb");
    if (!pFile) {
        debug_printf_stderr("Failed to open %s for reading!\n", pathname);
        return false;
    }
    target = (size_t)fileStartRecord + fileIndexRecord;
    status = true;
    for (i = 0; i < target; i++) {
        if (!record_line_copy(pFile, NULL)) {
            status = false;
            break;
        }
    }
    if (status) {
        memset(fileData, 0, fileDataLen);
        status = false;
        while ((c = getc(pFile)) != EOF) {
            status = true;
            if (c == '\n') {
                break;
            }
            if (len < fileDataLen) {
                fileData[len++] = (uint8_t)c;
            }
        }
    }
    fclose(pFile);

    return status;
}

/**
 * @brief Initializes the object data
 */
void bacfile_posix_init(void)
{
#if defined(BACFILE)
    bacfile_write_stream_data_callback_set(bacfile_posix_write_stream_data);
    bacfile_read_stream_data_callback_set(bacfile_posix_read_stream_data);
    bacfile_write_record_data_callback_set(bacfile_posix_write_record_data);
    bacfile_read_record_data_callback_set(bacfile_posix_read_record_data);
    bacfile_file_size_callback_set(bacfile_posix_file_size);
    bacfile_record_count_callback_set(bacfile_posix_record_count);
    bacfile_file_size_set_callback_set(bacfile_posix_file_size_set);
#elif defined(BACDL_BSC)
#error BACFILE is not defined for BACnet/SC!
#endif
}
