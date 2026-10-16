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
            debug_log_fprintf(
                DEBUG_LOG_DEBUG, stderr, "Failed to open %s for reading!\n",
                pathname);
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
 *  If the 'File Start Position' parameter is either less than 0
 *  or exceeds the actual file size, then an error is returned.
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

    if (fileStartPosition < 0) {
        /* invalid file start position */
        return 0;
    }
    if (filename_path_valid(pathname)) {
        pFile = fopen(pathname, "rb");
        if (pFile) {
            if (fileStartPosition > fsize(pFile)) {
                /* invalid file start position */
                fclose(pFile);
                return 0;
            }
            (void)fseek(pFile, fileStartPosition, SEEK_SET);
            len = fread(fileData, 1, fileDataLen, pFile);
            fclose(pFile);
        } else {
            debug_log_fprintf(
                DEBUG_LOG_DEBUG, stderr, "Failed to open %s for reading!\n",
                pathname);
        }
    }

    return len;
}

/**
 * @brief Writes stream data to a file
 * @param pathname - name of the file to write to
 * @param fileStartPosition - starting position in the file
 *  If the 'File Start Position' parameter exceeds the actual file size,
 *  then the file shall be extended to the size indicated,
 *  but the contents of any intervening octets or records
 *  shall be a local matter.
 *  If this parameter has the special value -1,
 *  then the write operation shall be treated
 *  as an append to the current end of file.
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
        } else if (fileStartPosition > 0) {
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
            debug_log_fprintf(
                DEBUG_LOG_DEBUG, stderr, "Failed to open %s for writing!\n",
                pathname);
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

    if (filename_path_valid(pathname)) {
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
 * @brief Move a block of bytes within a file, which may overlap
 * @param pFile - file opened for update
 * @param from - offset of the block
 * @param to - destination offset of the block
 * @param len - size of the block
 * @param ascending - true to copy from the start of the block (to < from),
 *  false to copy from the end of the block (to > from)
 * @return true if successful
 */
static bool file_move(FILE *pFile, long from, long to, long len, bool ascending)
{
    uint8_t buffer[256];
    long done = 0;
    long remaining;
    long offset;
    size_t chunk;

    while (done < len) {
        remaining = len - done;
        chunk = (remaining > (long)sizeof(buffer)) ? sizeof(buffer)
                                                   : (size_t)remaining;
        offset = ascending ? done : (remaining - (long)chunk);
        if ((fseek(pFile, from + offset, SEEK_SET) != 0) ||
            (fread(buffer, 1, chunk, pFile) != chunk)) {
            return false;
        }
        if ((fseek(pFile, to + offset, SEEK_SET) != 0) ||
            (fwrite(buffer, 1, chunk, pFile) != chunk)) {
            return false;
        }
        done += (long)chunk;
    }

    return true;
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
 * @note Updates the file in place using only C stdio, so a failure
 *  part way through can leave the file partly modified.
 */
static bool record_replace(
    const char *pathname,
    size_t target,
    const uint8_t *fileData,
    size_t fileDataLen)
{
    bool status = false;
    bool found = false;
    bool unterminated = false;
    FILE *pFile;
    size_t records = 0;
    size_t i;
    long pos = 0;
    long start = 0;
    long old_end;
    long size;
    long new_len = (long)fileDataLen + 1;
    int c;
    int last = '\n';

    /* a missing backing file is created by the first write */
    pFile = fopen(pathname, "rb+");
    if (!pFile) {
        pFile = fopen(pathname, "wb+");
    }
    if (!pFile) {
        debug_log_fprintf(
            DEBUG_LOG_DEBUG, stderr, "Failed to open %s for writing!\n",
            pathname);
        return false;
    }
    /* walk the records until the target record starts */
    while ((c = getc(pFile)) != EOF) {
        if (records == target) {
            found = true;
            start = pos;
            break;
        }
        pos++;
        last = c;
        if (c == '\n') {
            records++;
        }
    }
    if (!found) {
        /* append, padding with empty records up to the target */
        unterminated = (last != '\n');
        if (unterminated) {
            records++;
        }
        if ((target - records) > BACNET_FILE_POSIX_MAX_PAD_RECORDS) {
            fclose(pFile);
            return false;
        }
        status = (fseek(pFile, 0L, SEEK_END) == 0);
        if (status && unterminated) {
            status = (putc('\n', pFile) != EOF);
        }
        for (i = records; status && (i < target); i++) {
            status = (putc('\n', pFile) != EOF);
        }
        status = status && record_write(pFile, fileData, fileDataLen);
        if (fclose(pFile) != 0) {
            status = false;
        }
        return status;
    }
    /* find the end of the replaced record, and the end of the file */
    old_end = start;
    if (fseek(pFile, start, SEEK_SET) == 0) {
        while ((c = getc(pFile)) != EOF) {
            old_end++;
            if (c == '\n') {
                break;
            }
        }
    }
    status = (fseek(pFile, 0L, SEEK_END) == 0);
    size = status ? ftell(pFile) : -1L;
    if (size < old_end) {
        fclose(pFile);
        return false;
    }
    /* move the tail only if the record length changes */
    if (new_len > (old_end - start)) {
        status =
            file_move(pFile, old_end, start + new_len, size - old_end, false);
        status = status && (fseek(pFile, start, SEEK_SET) == 0) &&
            record_write(pFile, fileData, fileDataLen);
    } else {
        status = (fseek(pFile, start, SEEK_SET) == 0) &&
            record_write(pFile, fileData, fileDataLen);
        if (new_len < (old_end - start)) {
            status = status &&
                file_move(
                         pFile, old_end, start + new_len, size - old_end, true);
        }
    }
    if (fclose(pFile) != 0) {
        status = false;
    }
    if (status && (new_len < (old_end - start))) {
        /* stdio cannot truncate in place */
        size -= (old_end - start) - new_len;
        status = bacfile_posix_file_size_set(pathname, (size_t)size);
    }

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
        debug_log_fprintf(
            DEBUG_LOG_DEBUG, stderr, "Failed to open %s for writing!\n",
            pathname);
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
    if (status) {
        if (fileDataLen > 0) {
            if (fwrite(fileData, 1, fileDataLen, pFile) != fileDataLen) {
                status = false;
            }
        }
        if (putc('\n', pFile) == EOF) {
            status = false;
        }
    }
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
 * @param fileData - record data to write, without a line terminator
 * @param fileDataLen - size of the record data
 * @return true if successful, false otherwise
 * @note The file format is one newline-terminated record per line.
 *  A record that contains a newline cannot be stored.
 */
bool bacfile_posix_write_record_data(
    const char *pathname,
    int32_t fileStartRecord,
    size_t fileIndexRecord,
    const uint8_t *fileData,
    size_t fileDataLen)
{
    if (!filename_path_valid(pathname) || (fileStartRecord < -1)) {
        return false;
    }
    if (fileDataLen > 0) {
        if (!fileData || memchr(fileData, '\n', fileDataLen)) {
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

    if ((fileStartRecord < 0) || !fileData || !filename_path_valid(pathname)) {
        return false;
    }
    pFile = fopen(pathname, "rb");
    if (!pFile) {
        debug_log_fprintf(
            DEBUG_LOG_DEBUG, stderr, "Failed to open %s for reading!\n",
            pathname);
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
    bacfile_write_stream_data_callback_set(bacfile_posix_write_stream_data);
    bacfile_read_stream_data_callback_set(bacfile_posix_read_stream_data);
    bacfile_write_record_data_callback_set(bacfile_posix_write_record_data);
    bacfile_read_record_data_callback_set(bacfile_posix_read_record_data);
    bacfile_file_size_callback_set(bacfile_posix_file_size);
    bacfile_record_count_callback_set(bacfile_posix_record_count);
    bacfile_file_size_set_callback_set(bacfile_posix_file_size_set);
}
