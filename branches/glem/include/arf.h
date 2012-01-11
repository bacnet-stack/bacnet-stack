/*####COPYRIGHTBEGIN####
 -------------------------------------------
 Copyright (C) 2005 Steve Karg

 This program is free software; you can redistribute it and/or
 modify it under the terms of the GNU General Public License
 as published by the Free Software Foundation; either version 2
 of the License, or (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program; if not, write to:
 The Free Software Foundation, Inc.
 59 Temple Place - Suite 330
 Boston, MA  02111-1307, USA.

 As a special exception, if other files instantiate templates or
 use macros or inline functions from this file, or you compile
 this file and link it with other works to produce a work based
 on this file, this file does not by itself cause the resulting
 work to be covered by the GNU General Public License. However
 the source code for this file must still be made available in
 accordance with section (3) of the GNU General Public License.

 This exception does not invalidate any other reasons why a work
 based on this file might be covered by the GNU General Public
 License.
 -------------------------------------------
####COPYRIGHTEND####*/
#ifndef ATOMIC_READ_FILE_H
#define ATOMIC_READ_FILE_H

#include <stdint.h>
#include <stdbool.h>
#include "bacdcode.h"
#include "bacstr.h"

typedef struct BACnet_Atomic_Read_File_Data {
    BACNET_OBJECT_TYPE object_type;
    uint32_t object_instance;
    BACNET_FILE_ACCESS_METHOD access;
    union {
        struct {
            int32_t fileStartPosition;
            uint32_t requestedOctetCount;
        } stream;
        struct {
            int32_t fileStartRecord;
            /* requested or returned record count */
            uint32_t RecordCount;
        } record;
    } type;
    BACNET_OCTET_STRING fileData;
    bool endOfFile;
} BACNET_ATOMIC_READ_FILE_DATA;

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* Atomic Read File */
/* encode service */
    int arf_encode_apdu(
        uint8_t * apdu,
		size_t max_apdu,
        uint8_t invoke_id,
        BACNET_ATOMIC_READ_FILE_DATA * data);

/* decode the service request only */
    int arf_decode_service_request(
        uint8_t * apdu,
        unsigned apdu_len,
        BACNET_ATOMIC_READ_FILE_DATA * data);

    int arf_decode_apdu(
        uint8_t * apdu,
        unsigned apdu_len,
        uint8_t * invoke_id,
        BACNET_ATOMIC_READ_FILE_DATA * data);

/* Atomic Read File Ack */

/* encode service */
    int arf_ack_encode_apdu(
        uint8_t * apdu,
		size_t max_apdu,
        uint8_t invoke_id,
        BACNET_ATOMIC_READ_FILE_DATA * data);

/* decode the service request only */
    int arf_ack_decode_service_request(
        uint8_t * apdu,
        unsigned apdu_len,
        BACNET_ATOMIC_READ_FILE_DATA * data);

    int arf_ack_decode_apdu(
        uint8_t * apdu,
        unsigned apdu_len,
        uint8_t * invoke_id,
        BACNET_ATOMIC_READ_FILE_DATA * data);

#ifdef TEST
#include "ctest.h"

    void test_AtomicReadFile(
        Test * pTest);
    void test_AtomicReadFileAck(
        Test * pTest);
#endif

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif
