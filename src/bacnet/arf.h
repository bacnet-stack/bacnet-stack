/**************************************************************************
*
* Copyright (C) 2012 Steve Karg <skarg@users.sourceforge.net>
*
* Permission is hereby granted, free of charge, to any person obtaining
* a copy of this software and associated documentation files (the
* "Software"), to deal in the Software without restriction, including
* without limitation the rights to use, copy, modify, merge, publish,
* distribute, sublicense, and/or sell copies of the Software, and to
* permit persons to whom the Software is furnished to do so, subject to
* the following conditions:
*
* The above copyright notice and this permission notice shall be included
* in all copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*********************************************************************/
#ifndef ATOMIC_READ_FILE_H
#define ATOMIC_READ_FILE_H

#include <stdint.h>
#include <stdbool.h>
#include "bacnet/bacnet_stack_exports.h"
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
