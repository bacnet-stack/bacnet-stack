/**
 * @file
 * @brief A static RAM based BACnet File Object implementation.
 * @author Steve Karg
 * @date July 2025
 * @copyright SPDX-License-Identifier: MIT
 */
#ifndef BACNET_FILE_SRAMFS_H
#define BACNET_FILE_SRAMFS_H
/* BACnet Stack defines - first */
#include "bacnet/bacdef.h"

struct bacnet_file_sramfs_data {
    size_t size; /* size of the file in bytes */
    char *data; /* data buffer */
    char *pathname; /* pathname of the file */
    struct bacnet_file_sramfs_data *next; /* pointer to the next file data */
};

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

BACNET_STACK_EXPORT
const char *bacfile_sramfs_file_data(const char *pathname);
BACNET_STACK_EXPORT
bool bacfile_sramfs_file_data_set(const char *pathname, const char *file_data);
BACNET_STACK_EXPORT
size_t bacfile_sramfs_file_size(const char *pathname);
BACNET_STACK_EXPORT
bool bacfile_sramfs_file_size_set(const char *pathname, size_t file_size);
BACNET_STACK_EXPORT
size_t bacfile_sramfs_read_stream_data(
    const char *pathname,
    int32_t fileStartPosition,
    uint8_t *fileData,
    size_t fileDataLen);
BACNET_STACK_EXPORT
size_t bacfile_sramfs_write_stream_data(
    const char *pathname,
    int32_t fileStartPosition,
    const uint8_t *fileData,
    size_t fileDataLen);
BACNET_STACK_EXPORT
bool bacfile_sramfs_write_record_data(
    const char *pathname,
    int32_t fileStartRecord,
    size_t fileIndexRecord,
    const uint8_t *fileData,
    size_t fileDataLen);
BACNET_STACK_EXPORT
bool bacfile_sramfs_read_record_data(
    const char *pathname,
    int32_t fileStartRecord,
    size_t fileIndexRecord,
    uint8_t *fileData,
    size_t fileDataLen);

BACNET_STACK_EXPORT
bool bacfile_sramfs_add(struct bacnet_file_sramfs_data *file_data);
BACNET_STACK_EXPORT
void bacfile_sramfs_init(void);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif
