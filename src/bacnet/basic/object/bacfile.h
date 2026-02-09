/**
 * @file
 * @author Steve Karg
 * @date 2005
 * @brief API for a basic BACnet File Object implementation.
 * @copyright SPDX-License-Identifier: MIT
 */
#ifndef BACNET_BASIC_OBJECT_FILE_H
#define BACNET_BASIC_OBJECT_FILE_H
#include <stdbool.h>
#include <stdint.h>
/* BACnet Stack defines - first */
#include "bacnet/bacdef.h"
/* BACnet Stack API */
#include "bacnet/bacint.h"
#include "bacnet/datetime.h"
#include "bacnet/apdu.h"
#include "bacnet/arf.h"
#include "bacnet/awf.h"
#include "bacnet/rp.h"
#include "bacnet/wp.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

BACNET_STACK_EXPORT
void BACfile_Property_Lists(
    const int32_t **pRequired,
    const int32_t **pOptional,
    const int32_t **pProprietary);
BACNET_STACK_EXPORT
void BACfile_Writable_Property_List(
    uint32_t object_instance, const int32_t **properties);

BACNET_STACK_EXPORT
bool bacfile_object_name(
    uint32_t object_instance, BACNET_CHARACTER_STRING *object_name);
BACNET_STACK_EXPORT
bool bacfile_object_name_set(uint32_t object_instance, const char *new_name);
BACNET_STACK_EXPORT
const char *bacfile_name_ansi(uint32_t object_instance);

BACNET_STACK_EXPORT
bool bacfile_valid_instance(uint32_t object_instance);
BACNET_STACK_EXPORT
unsigned bacfile_count(void);
BACNET_STACK_EXPORT
uint32_t bacfile_index_to_instance(unsigned find_index);
BACNET_STACK_EXPORT
unsigned bacfile_instance_to_index(uint32_t instance);

BACNET_STACK_EXPORT
const char *bacfile_file_type(uint32_t object_instance);
BACNET_STACK_EXPORT
void bacfile_file_type_set(uint32_t object_instance, const char *mime_type);

BACNET_STACK_EXPORT
bool bacfile_archive(uint32_t instance);
BACNET_STACK_EXPORT
bool bacfile_archive_set(uint32_t instance, bool status);

BACNET_STACK_EXPORT
bool bacfile_read_only(uint32_t instance);
BACNET_STACK_EXPORT
bool bacfile_read_only_set(uint32_t object_instance, bool read_only);

BACNET_STACK_EXPORT
bool bacfile_file_access_stream(uint32_t object_instance);
BACNET_STACK_EXPORT
bool bacfile_file_access_stream_set(uint32_t object_instance, bool access);

BACNET_STACK_EXPORT
BACNET_UNSIGNED_INTEGER bacfile_file_size(uint32_t instance);
BACNET_STACK_EXPORT
bool bacfile_file_size_set(
    uint32_t object_instance, BACNET_UNSIGNED_INTEGER file_size);

/* this is one way to match up the invoke ID with */
/* the file ID from the AtomicReadFile request. */
/* Another way would be to store the */
/* invokeID and file instance in a list or table */
/* when the request was sent */
BACNET_STACK_EXPORT
uint32_t bacfile_instance_from_tsm(uint8_t invokeID);

/* handler ACK helper */
BACNET_STACK_EXPORT
bool bacfile_read_stream_data(BACNET_ATOMIC_READ_FILE_DATA *data);
BACNET_STACK_EXPORT
bool bacfile_read_ack_stream_data(
    uint32_t instance, const BACNET_ATOMIC_READ_FILE_DATA *data);
BACNET_STACK_EXPORT
bool bacfile_write_stream_data(BACNET_ATOMIC_WRITE_FILE_DATA *data);
BACNET_STACK_EXPORT
bool bacfile_read_record_data(BACNET_ATOMIC_READ_FILE_DATA *data);
BACNET_STACK_EXPORT
bool bacfile_read_ack_record_data(
    uint32_t instance, const BACNET_ATOMIC_READ_FILE_DATA *data);
BACNET_STACK_EXPORT
bool bacfile_write_record_data(const BACNET_ATOMIC_WRITE_FILE_DATA *data);

/* handling for read property service */
BACNET_STACK_EXPORT
int bacfile_read_property(BACNET_READ_PROPERTY_DATA *rpdata);

/* handling for write property service */
BACNET_STACK_EXPORT
bool bacfile_write_property(BACNET_WRITE_PROPERTY_DATA *wp_data);

BACNET_STACK_EXPORT
const char *bacfile_pathname(uint32_t instance);
BACNET_STACK_EXPORT
void bacfile_pathname_set(uint32_t instance, const char *pathname);
BACNET_STACK_EXPORT
uint32_t bacfile_pathname_instance(const char *pathname);

BACNET_STACK_EXPORT
uint32_t
bacfile_read(uint32_t object_instance, uint8_t *buffer, uint32_t buffer_size);
BACNET_STACK_EXPORT
uint32_t bacfile_write(
    uint32_t object_instance, const uint8_t *buffer, uint32_t buffer_size);
BACNET_STACK_EXPORT
uint32_t bacfile_write_offset(
    uint32_t object_instance,
    int32_t offset,
    const uint8_t *buffer,
    uint32_t buffer_size);

BACNET_STACK_EXPORT
void bacfile_write_stream_data_callback_set(
    size_t (*callback)(const char *, int32_t, const uint8_t *, size_t));
BACNET_STACK_EXPORT
void bacfile_read_stream_data_callback_set(
    size_t (*callback)(const char *, int32_t, uint8_t *, size_t));
BACNET_STACK_EXPORT
void bacfile_write_record_data_callback_set(
    bool (*callback)(const char *, int32_t, size_t, const uint8_t *, size_t));
BACNET_STACK_EXPORT
void bacfile_read_record_data_callback_set(
    bool (*callback)(const char *, int32_t, size_t, uint8_t *, size_t));
BACNET_STACK_EXPORT
void bacfile_file_size_callback_set(size_t (*callback)(const char *));
BACNET_STACK_EXPORT
void bacfile_file_size_set_callback_set(bool (*callback)(const char *, size_t));

BACNET_STACK_EXPORT
void *bacfile_context_get(uint32_t object_instance);
BACNET_STACK_EXPORT
void bacfile_context_set(uint32_t object_instance, void *context);

BACNET_STACK_EXPORT
uint32_t bacfile_create(uint32_t object_instance);
BACNET_STACK_EXPORT
bool bacfile_delete(uint32_t object_instance);
BACNET_STACK_EXPORT
void bacfile_cleanup(void);
BACNET_STACK_EXPORT
void bacfile_init(void);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif
