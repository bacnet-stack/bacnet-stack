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
#ifndef ATOMIC_WRITE_FILE_H
#define ATOMIC_WRITE_FILE_H

#include <stdint.h>
#include <stdbool.h>
/* BACnet Stack defines - first */
#include "bacnet/bacdef.h"
/* BACnet Stack API */
#include "bacnet/bacdcode.h"

#ifndef BACNET_WRITE_FILE_RECORD_COUNT
#define BACNET_WRITE_FILE_RECORD_COUNT 1
#endif

typedef struct BACnet_Atomic_Write_File_Data {
    BACNET_OBJECT_TYPE object_type;
    uint32_t object_instance;
    BACNET_FILE_ACCESS_METHOD access;
    union {
        struct {
            int32_t fileStartPosition;
        } stream;
        struct {
            int32_t fileStartRecord;
            uint32_t returnedRecordCount;
        } record;
    } type;
    BACNET_OCTET_STRING fileData[BACNET_WRITE_FILE_RECORD_COUNT];
} BACNET_ATOMIC_WRITE_FILE_DATA;

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

    BACNET_STACK_EXPORT
    int awf_service_encode_apdu(
        uint8_t *apdu, 
        BACNET_ATOMIC_WRITE_FILE_DATA *data);
    BACNET_STACK_EXPORT
    int atomicwritefile_service_request_encode(
        uint8_t *apdu, 
        size_t apdu_size, 
        BACNET_ATOMIC_WRITE_FILE_DATA *data);

    BACNET_STACK_EXPORT
    int awf_encode_apdu(
        uint8_t * apdu,
        uint8_t invoke_id,
        BACNET_ATOMIC_WRITE_FILE_DATA * data);

    BACNET_STACK_EXPORT
    int awf_decode_service_request(
        uint8_t * apdu,
        unsigned apdu_size,
        BACNET_ATOMIC_WRITE_FILE_DATA * data);
    BACNET_STACK_EXPORT
    int awf_decode_apdu(
        uint8_t * apdu,
        unsigned apdu_size,
        uint8_t * invoke_id,
        BACNET_ATOMIC_WRITE_FILE_DATA * data);

    BACNET_STACK_EXPORT
    int awf_ack_service_encode_apdu(
        uint8_t *apdu, 
        BACNET_ATOMIC_WRITE_FILE_DATA *data);
    BACNET_STACK_EXPORT
    int awf_ack_encode_apdu(
        uint8_t * apdu,
        uint8_t invoke_id,
        BACNET_ATOMIC_WRITE_FILE_DATA * data);

    BACNET_STACK_EXPORT
    int awf_ack_decode_service_request(
        uint8_t * apdu,
        unsigned apdu_size,
        BACNET_ATOMIC_WRITE_FILE_DATA * data);
    BACNET_STACK_EXPORT
    int awf_ack_decode_apdu(
        uint8_t * apdu,
        unsigned apdu_size,
        uint8_t * invoke_id,
        BACNET_ATOMIC_WRITE_FILE_DATA * data);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif
