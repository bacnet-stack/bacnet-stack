/**
 * @file
 * @brief A RAM file system BACnet File Object implementation.
 * @author Steve Karg
 * @date 2025
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
#include "bacnet/basic/sys/keylist.h"
#include "bacnet/basic/object/bacfile.h"
#include "bacnet/datalink/cobs.h"

#ifndef FILE_RECORD_SIZE
#define FILE_RECORD_SIZE MAX_OCTET_STRING_BYTES
#endif

/* Key List for storing the object data sorted by instance number  */
static OS_Keylist File_List;

struct file_data {
    size_t size; /* size of the file in bytes */
    char *data; /* data buffer */
    OS_Keylist record_list; /* for record data */
};
#define CRC32K_INITIAL_VALUE (0xFFFFFFFF)

/**
 * @brief Get a CRC32K checksum for a pathname to use as a hash key
 * @param pathname - the pathname to calculate the CRC32K for
 * @return CRC32K value
 */
static uint32_t pathname_crc32k(const char *pathname)
{
    uint32_t crc32K = CRC32K_INITIAL_VALUE;
    size_t len, i;

    len = strlen(pathname);
    for (i = 0; i < len; i++) {
        crc32K = cobs_crc32k(pathname[i], crc32K);
    }

    return crc32K;
}

/**
 * @brief For a given object instance-number, returns the pathname
 * @param  object_instance - object-instance number of the object
 * @return  internal file system path and name, or NULL if not set
 */
static struct file_data *bacfile_ramfs_open(const char *pathname)
{
    struct file_data *pFile = NULL;
    uint32_t crc32K;
    int index;

    if (!pathname || pathname[0] == 0) {
        return NULL; /* invalid pathname */
    }
    crc32K = pathname_crc32k(pathname);
    pFile = Keylist_Data(File_List, crc32K);
    if (!pFile) {
        pFile = calloc(1, sizeof(struct file_data));
        if (pFile) {
            pFile->size = 0;
            pFile->data = NULL;
            index = Keylist_Data_Add(File_List, crc32K, pFile);
            if (index < 0) {
                free(pFile);
                pFile = NULL;
            }
        }
    }

    return pFile;
}

/**
 * @brief Determines the file size for a given file
 * @param  pathname - name of the file to get the size for
 * @return  file size in bytes, or 0 if not found
 */
size_t bacfile_ramfs_file_size(const char *pathname)
{
    struct file_data *pFile;
    size_t file_size = 0;

    pFile = bacfile_ramfs_open(pathname);
    if (pFile) {
        file_size = pFile->size;
    }

    return file_size;
}

/**
 * @brief Sets the file size property value
 * @param pathname - name of the file to set the size for
 * @param file_size - value of the file size property
 * @return true if file size is writable
 */
bool bacfile_ramfs_file_size_set(const char *pathname, size_t new_size)
{
    struct file_data *pFile;
    size_t old_size;
    char *old_data;

    bool status = false;

    pFile = bacfile_ramfs_open(pathname);
    if (pFile) {
        old_size = pFile->size;
        old_data = pFile->data;
        pFile->data = realloc(pFile->data, new_size);
        if (pFile->data) {
            memcpy(pFile->data, old_data, min(old_size, new_size));
        }
        pFile->size = new_size;
        status = true;
    }

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
size_t bacfile_ramfs_read_stream_data(
    const char *pathname,
    int32_t fileStartPosition,
    uint8_t *fileData,
    size_t fileDataLen)
{
    struct file_data *pFile;
    size_t len = 0;

    pFile = bacfile_ramfs_open(pathname);
    if (pFile) {
        if (fileStartPosition + fileDataLen > pFile->size) {
            /* read only up to the end of the file */
            len = pFile->size - fileStartPosition;
        } else {
            len = fileDataLen;
        }
        memcpy(fileData, pFile->data + fileStartPosition, len);
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
size_t bacfile_ramfs_write_stream_data(
    const char *pathname,
    int32_t fileStartPosition,
    const uint8_t *fileData,
    size_t fileDataLen)
{
    size_t bytes_written = 0;
    struct file_data *pFile;
    size_t old_size;

    pFile = bacfile_ramfs_open(pathname);
    if (pFile) {
        if (fileStartPosition == 0) {
            /* open the file as a clean slate when starting at 0 */
            pFile->data = realloc(pFile->data, fileDataLen);
            if (pFile->data) {
                memcpy(pFile->data, fileData, fileDataLen);
                pFile->size = fileDataLen;
                bytes_written = fileDataLen;
            }
        } else if (fileStartPosition == -1) {
            /* If 'File Start Position' parameter has the special
               value -1, then the write operation shall be treated
               as an append to the current end of file. */
            old_size = pFile->size;
            pFile->size += fileDataLen;
            pFile->data = realloc(pFile->data, pFile->size);
            if (pFile->data) {
                memcpy(pFile->data + old_size, fileData, fileDataLen);
                bytes_written = fileDataLen;
            }
        } else {
            /* open for update */
            if (fileStartPosition + fileDataLen > pFile->size) {
                /* extend the file size */
                pFile->data =
                    realloc(pFile->data, fileStartPosition + fileDataLen);
                if (pFile->data) {
                    pFile->size = fileStartPosition + fileDataLen;
                }
            }
            if (pFile->data) {
                memcpy(pFile->data + fileStartPosition, fileData, fileDataLen);
                bytes_written = fileDataLen;
            }
        }
    }

    return bytes_written;
}

/**
 * @brief Count the number of records in a file
 * @param records - string of null-terminated records
 * @return number of records
 */
static size_t record_count(const char *records)
{
    size_t count = 0;
    int len = 0;

    if (records) {
        do {
            len = strlen(records);
            if (len > 0) {
                count++;
                records = records + len + 1;
            }
        } while (len > 0);
    }

    return count;
}

/**
 * @brief Get the specific record at index 0..N
 * @param records - string of null-terminated records
 * @param record_index - record index number 1..N of the records
 * @return record, or NULL
 */
static char *record_by_index(const char *records, size_t index)
{
    size_t count = 0;
    int len = 0;

    if (records) {
        do {
            len = strlen(records);
            if (len > 0) {
                count++;
                if (index == count) {
                    return records;
                }
                records = records + len + 1;
            }
        } while (len > 0);
    }

    return NULL;
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
bool bacfile_ramfs_write_record_data(
    const char *pathname,
    int32_t fileStartRecord,
    size_t fileIndexRecord,
    const uint8_t *fileData,
    size_t fileDataLen)
{
    bool status = false;
    size_t fileSeekRecord;
    size_t fileRecordCount;
    struct file_data *pFile;
    char *record;
    size_t record_len;
    KEY key;

    pFile = bacfile_ramfs_open(pathname);
    if (pFile) {
        /* read the records from the datablock into a Keylist.
           This allows for easy access to the records by index,
           and also allows for adding or inserting new records
           or deleting records. Then loop through the records
           and write the content to the data block. */
        pFile->record_list = Keylist_Create();
        fileRecordCount = record_count(pFile->data);
        for (key = 0; key < fileRecordCount; key++) {
            record = record_by_index(pFile->data, key);
            if (record) {
                Keylist_Data_Add(pFile->record_list, key, record);
            }
        }
        if (fileStartRecord == -1) {
            /* If 'File Start Record' parameter has the special
               value -1, then the write operation shall be treated
               as an append to the current end of file. */
            fileSeekRecord = fileRecordCount;
        } else {
            fileSeekRecord = fileStartRecord + fileIndexRecord;
        }
        key = fileSeekRecord;
        /* fixme: fileData needs to be NULL or newline terminated */
        Keylist_Data_Add(pFile->record_list, key, fileData);
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
bool bacfile_ramfs_read_record_data(
    const char *pathname,
    int32_t fileStartRecord,
    size_t fileIndexRecord,
    uint8_t *fileData,
    size_t fileDataLen)
{
    bool status = false;
    size_t fileSeekRecord;
    struct file_data *pFile;
    char *record;
    size_t record_len;

    pFile = bacfile_ramfs_open(pathname);
    if (pFile) {
        fileSeekRecord = fileStartRecord + fileIndexRecord;
        /* seek to the start record */
        record = record_by_index(pFile->data, fileSeekRecord);
        if (record) {
            record_len = strlen(record);
            if (record_len > 0 && record_len <= fileDataLen) {
                /* copy the record data */
                memcpy(fileData, record, record_len);
                status = true;
            }
        }
    }

    return status;
}

/**
 * @brief Initializes the object data
 */
void bacfile_ramfs_init(void)
{
#if defined(BACFILE)
    bacfile_write_stream_data_callback_set(bacfile_ramfs_write_stream_data);
    bacfile_read_stream_data_callback_set(bacfile_ramfs_read_stream_data);
    bacfile_write_record_data_callback_set(bacfile_ramfs_write_record_data);
    bacfile_read_record_data_callback_set(bacfile_ramfs_read_record_data);
    bacfile_file_size_callback_set(bacfile_ramfs_file_size);
    bacfile_file_size_set_callback_set(bacfile_ramfs_file_size_set);
#endif
    File_List = Keylist_Create();
}
