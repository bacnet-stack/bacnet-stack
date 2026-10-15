/**
 * @file
 * @brief A POSIX BACnet File Object implementation.
 * @author Steve Karg
 * @date July 2025
 * @copyright SPDX-License-Identifier: MIT
 */
#ifndef BACNET_FILE_POSIX_H
#define BACNET_FILE_POSIX_H
/* BACnet Stack defines - first */
#include "bacnet/bacdef.h"

/* maximum number of empty records added by one write past the end of a file;
   0 disables the padding */
#ifndef BACNET_FILE_POSIX_MAX_PAD_RECORDS
#define BACNET_FILE_POSIX_MAX_PAD_RECORDS 1024
#endif

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

BACNET_STACK_EXPORT
bool bacfile_posix_file_path_valid(const char *pathname);
BACNET_STACK_EXPORT
size_t bacfile_posix_file_size(const char *pathname);
BACNET_STACK_EXPORT
size_t bacfile_posix_record_count(const char *pathname);
BACNET_STACK_EXPORT
bool bacfile_posix_file_size_set(const char *pathname, size_t file_size);
BACNET_STACK_EXPORT
size_t bacfile_posix_read_stream_data(
    const char *pathname,
    int32_t fileStartPosition,
    uint8_t *fileData,
    size_t fileDataLen);
BACNET_STACK_EXPORT
size_t bacfile_posix_write_stream_data(
    const char *pathname,
    int32_t fileStartPosition,
    const uint8_t *fileData,
    size_t fileDataLen);
BACNET_STACK_EXPORT
bool bacfile_posix_write_record_data(
    const char *pathname,
    int32_t fileStartRecord,
    size_t fileIndexRecord,
    const uint8_t *fileData,
    size_t fileDataLen);
BACNET_STACK_EXPORT
bool bacfile_posix_read_record_data(
    const char *pathname,
    int32_t fileStartRecord,
    size_t fileIndexRecord,
    uint8_t *fileData,
    size_t fileDataLen);
BACNET_STACK_EXPORT
void bacfile_posix_init(void);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif
