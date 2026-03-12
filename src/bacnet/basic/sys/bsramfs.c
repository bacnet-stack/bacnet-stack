/**
 * @file
 * @brief A static RAM file system BACnet File Object implementation.
 * @author Steve Karg
 * @date 2025
 * @copyright SPDX-License-Identifier: MIT
 */
#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
/* BACnet Stack defines - first */
#include "bacnet/bacdef.h"
#include "bacnet/basic/object/bacfile.h"
#include "bacnet/basic/sys/bsramfs.h"

static struct bacnet_file_sramfs_data *File_List;

bool bacfile_sramfs_add(struct bacnet_file_sramfs_data *file_data)
{
    struct bacnet_file_sramfs_data *head = NULL;

    if (!file_data) {
        return false; /* invalid data */
    }
    head = File_List;
    if (!head) {
        /* first file data */
        File_List = file_data;
    } else {
        /* find the end of the list */
        while (head->next) {
            head = head->next;
        }
        /* add to the end of the list */
        head->next = file_data;
    }

    return true;
}

/**
 * @brief For a given object instance-number, returns the pathname
 * @param  object_instance - object-instance number of the object
 * @return  internal file system path and name, or NULL if not set
 */
static struct bacnet_file_sramfs_data *bacfile_sramfs_open(const char *pathname)
{
    struct bacnet_file_sramfs_data *head;

    if (!pathname || pathname[0] == 0) {
        return NULL; /* invalid pathname */
    }
    head = File_List;
    while (head) {
        if (head->pathname) {
            if (strcmp(head->pathname, pathname) == 0) {
                return head; /* found the file */
            }
        }
        head = head->next;
    }

    return NULL;
}

/**
 * @brief Determines the file size for a given file
 * @param  pathname - name of the file to get the size for
 * @return  file size in bytes, or 0 if not found
 */
const char *bacfile_sramfs_file_data(const char *pathname)
{
    struct bacnet_file_sramfs_data *pFile;
    const char *file_data = NULL;

    pFile = bacfile_sramfs_open(pathname);
    if (pFile) {
        file_data = pFile->data;
    }

    return file_data;
}

/**
 * @brief Determines the file size for a given file
 * @param  pathname - name of the file to get the size for
 * @return  file size in bytes, or 0 if not found
 */
size_t bacfile_sramfs_file_size(const char *pathname)
{
    struct bacnet_file_sramfs_data *pFile;
    size_t file_size = 0;

    pFile = bacfile_sramfs_open(pathname);
    if (pFile) {
        file_size = pFile->size;
    }

    return file_size;
}

/**
 * @brief Reads stream data from a file
 * @param pathname - name of the file to read from
 * @param fileStartPosition - starting position in the file
 * @param fileData - data buffer to read into
 * @param fileDataLen - size of the data buffer
 * @return number of bytes read, or 0 if not successful
 */
size_t bacfile_sramfs_read_stream_data(
    const char *pathname,
    int32_t fileStartPosition,
    uint8_t *fileData,
    size_t fileDataLen)
{
    struct bacnet_file_sramfs_data *pFile;
    size_t len = 0;

    pFile = bacfile_sramfs_open(pathname);
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
 * @brief Get the specific record at index 0..N
 * @param records - string of null-terminated records
 * @param record_index - record index number 0..N of the records
 * @return record, or NULL
 */
static char *record_by_index(char *records, size_t index)
{
    size_t count = 0;
    int len = 0;

    if (records) {
        do {
            len = bacnet_strnlen(records, MAX_OCTET_STRING_BYTES);
            if (len > 0) {
                if (index == count) {
                    return records;
                }
                count++;
                records = records + len + 1;
            }
        } while (len > 0);
    }

    return NULL;
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
bool bacfile_sramfs_read_record_data(
    const char *pathname,
    int32_t fileStartRecord,
    size_t fileIndexRecord,
    uint8_t *fileData,
    size_t fileDataLen)
{
    bool status = false;
    size_t fileSeekRecord;
    struct bacnet_file_sramfs_data *pFile;
    char *record;
    size_t record_len;

    pFile = bacfile_sramfs_open(pathname);
    if (pFile) {
        fileSeekRecord = fileStartRecord + fileIndexRecord;
        /* seek to the start record */
        record = record_by_index(pFile->data, fileSeekRecord);
        if (record) {
            record_len = bacnet_strnlen(record, MAX_OCTET_STRING_BYTES);
            if ((record_len > 0) && (record_len <= fileDataLen)) {
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
void bacfile_sramfs_init(void)
{
    bacfile_read_stream_data_callback_set(bacfile_sramfs_read_stream_data);
    bacfile_read_record_data_callback_set(bacfile_sramfs_read_record_data);
    bacfile_file_size_callback_set(bacfile_sramfs_file_size);
}
