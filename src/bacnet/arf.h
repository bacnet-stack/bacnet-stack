/**
 * @file
 * @brief BACnet AtomicReadFile service structures, codecs, and handlers.
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date 2007
 * @copyright SPDX-License-Identifier: MIT
 */
#ifndef BACNET_ATOMIC_READ_FILE_H
#define BACNET_ATOMIC_READ_FILE_H
#include <stdint.h>
#include <stdbool.h>
/* BACnet Stack defines - first */
#include "bacnet/bacdef.h"
/* BACnet Stack API */
#include "bacnet/bacdcode.h"
#include "bacnet/bacstr.h"

#ifndef BACNET_READ_FILE_RECORD_COUNT
#define BACNET_READ_FILE_RECORD_COUNT 1
#endif

typedef struct BACnet_Atomic_Read_File_Data {
    /* number type first to avoid enum cast warning on = { 0 } */
    uint32_t object_instance;
    BACNET_OBJECT_TYPE object_type;
    BACNET_FILE_ACCESS_METHOD access;
    union {
        struct {
            int32_t fileStartPosition;
            BACNET_UNSIGNED_INTEGER requestedOctetCount;
        } stream;
        struct {
            int32_t fileStartRecord;
            /* requested or returned record count */
            BACNET_UNSIGNED_INTEGER RecordCount;
        } record;
    } type;
    BACNET_OCTET_STRING fileData[BACNET_READ_FILE_RECORD_COUNT];
    bool endOfFile;
} BACNET_ATOMIC_READ_FILE_DATA;

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* Atomic Read File */
/* encode service */
    BACNET_STACK_EXPORT
    int arf_encode_apdu(
        uint8_t * apdu,
        uint8_t invoke_id,
        BACNET_ATOMIC_READ_FILE_DATA * data);
    BACNET_STACK_EXPORT
    int arf_service_encode_apdu(
        uint8_t *apdu,
        BACNET_ATOMIC_READ_FILE_DATA *data);
    BACNET_STACK_EXPORT
    size_t atomicreadfile_service_request_encode(
        uint8_t *apdu,
        size_t apdu_size,
        BACNET_ATOMIC_READ_FILE_DATA *data);

/* decode the service request only */
    BACNET_STACK_EXPORT
    int arf_decode_service_request(
        uint8_t * apdu,
        unsigned apdu_len,
        BACNET_ATOMIC_READ_FILE_DATA * data);

    BACNET_STACK_EXPORT
    int arf_decode_apdu(
        uint8_t * apdu,
        unsigned apdu_len,
        uint8_t * invoke_id,
        BACNET_ATOMIC_READ_FILE_DATA * data);

/* Atomic Read File Ack */

/* encode service */
    BACNET_STACK_EXPORT
    int arf_ack_encode_apdu(
        uint8_t * apdu,
        uint8_t invoke_id,
        BACNET_ATOMIC_READ_FILE_DATA * data);
    BACNET_STACK_EXPORT
    int arf_ack_service_encode_apdu(
        uint8_t *apdu,
        BACNET_ATOMIC_READ_FILE_DATA *data);

/* decode the service request only */
    BACNET_STACK_EXPORT
    int arf_ack_decode_service_request(
        uint8_t * apdu,
        unsigned apdu_len,
        BACNET_ATOMIC_READ_FILE_DATA * data);

    BACNET_STACK_EXPORT
    int arf_ack_decode_apdu(
        uint8_t * apdu,
        unsigned apdu_len,
        uint8_t * invoke_id,
        BACNET_ATOMIC_READ_FILE_DATA * data);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif
