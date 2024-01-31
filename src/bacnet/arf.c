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
#include <stdint.h>
#include "bacnet/bacenum.h"
#include "bacnet/bacdcode.h"
#include "bacnet/bacdef.h"
#include "bacnet/arf.h"

/** @file arf.c  Atomic Read File */

/**
 * @brief Encode the AtomicReadFile service request
 *
 *  AtomicReadFile-Request ::= SEQUENCE {
 *      file-identifier BACnetObjectIdentifier,
 *      access-method CHOICE {
 *          stream-access [0] SEQUENCE {
 *              file-start-position INTEGER,
 *              requested-octet-count Unsigned
 *          },
 *          record-access [1] SEQUENCE {
 *              file-start-record INTEGER,
 *              requested-record-count Unsigned
 *          }
 *      }
 *  }
 *
 * @param apdu  Pointer to the buffer for encoded values
 * @param data  Pointer to the service data used for encoding values
 * @return number of bytes encoded
 */
int arf_service_encode_apdu(uint8_t *apdu, BACNET_ATOMIC_READ_FILE_DATA *data)
{
    int apdu_len = 0; /* total length of the apdu, return value */
    int len = 0;

    len = encode_application_object_id(
        apdu, data->object_type, data->object_instance);
    apdu_len += len;
    if (apdu) {
        apdu += len;
    }
    switch (data->access) {
        case FILE_STREAM_ACCESS:
            len = encode_opening_tag(apdu, 0);
            apdu_len += len;
            if (apdu) {
                apdu += len;
            }
            len = encode_application_signed(
                apdu, data->type.stream.fileStartPosition);
            apdu_len += len;
            if (apdu) {
                apdu += len;
            }
            len = encode_application_unsigned(
                apdu, data->type.stream.requestedOctetCount);
            apdu_len += len;
            if (apdu) {
                apdu += len;
            }
            len = encode_closing_tag(apdu, 0);
            apdu_len += len;
            break;
        case FILE_RECORD_ACCESS:
            len = encode_opening_tag(apdu, 1);
            apdu_len += len;
            if (apdu) {
                apdu += len;
            }
            len = encode_application_signed(
                apdu, data->type.record.fileStartRecord);
            apdu_len += len;
            if (apdu) {
                apdu += len;
            }
            len = encode_application_unsigned(
                apdu, data->type.record.RecordCount);
            apdu_len += len;
            if (apdu) {
                apdu += len;
            }
            len = encode_closing_tag(apdu, 1);
            apdu_len += len;
            break;
        default:
            break;
    }

    return apdu_len;
}

/**
 * @brief Encode the AtomicReadFile service request
 * @param apdu Pointer to the buffer for encoded values
 * @param apdu_size number of bytes available in the buffer
 * @param data  Pointer to the service data used for encoding values
 * @return number of bytes encoded, or zero if unable to encode or too large
 */
size_t atomicreadfile_service_request_encode(
    uint8_t *apdu, size_t apdu_size, BACNET_ATOMIC_READ_FILE_DATA *data)
{
    size_t apdu_len = 0; /* total length of the apdu, return value */

    apdu_len = arf_service_encode_apdu(NULL, data);
    if (apdu_len > apdu_size) {
        apdu_len = 0;
    } else {
        apdu_len = arf_service_encode_apdu(apdu, data);
    }

    return apdu_len;
}

/**
 * @brief Encode the AtomicReadFile service request
 * @param apdu  Pointer to the buffer for decoding.
 * @param invoke_id original invoke id for request
 * @param data  Pointer to the property decoded data to be stored
 * @return number of bytes encoded
 */
int arf_encode_apdu(
    uint8_t *apdu, uint8_t invoke_id, BACNET_ATOMIC_READ_FILE_DATA *data)
{
    int apdu_len = 0; /* total length of the apdu, return value */
    int len = 0;

    if (apdu) {
        apdu[0] = PDU_TYPE_CONFIRMED_SERVICE_REQUEST;
        apdu[1] = encode_max_segs_max_apdu(0, MAX_APDU);
        apdu[2] = invoke_id;
        apdu[3] = SERVICE_CONFIRMED_ATOMIC_READ_FILE; /* service choice */
    }
    len = 4;
    apdu_len += len;
    if (apdu) {
        apdu += len;
    }
    len = arf_service_encode_apdu(apdu, data);
    apdu_len += len;

    return apdu_len;
}

/**
 * @brief Decode the AtomicReadFile service request
 *
 *  AtomicReadFile-Request ::= SEQUENCE {
 *      file-identifier BACnetObjectIdentifier,
 *      access-method CHOICE {
 *          stream-access [0] SEQUENCE {
 *              file-start-position INTEGER,
 *              requested-octet-count Unsigned
 *          },
 *          record-access [1] SEQUENCE {
 *              file-start-record INTEGER,
 *              requested-record-count Unsigned
 *          }
 *      }
 *  }
 *
 * @param apdu  Pointer to the buffer for decoding.
 * @param apdu_size  Count of valid bytes in the buffer.
 * @param data  Pointer to the property decoded data to be stored
 *  or NULL for length
 *
 * @return number of bytes decoded or BACNET_STATUS_ERROR on error.
 */
int arf_decode_service_request(
    uint8_t *apdu, unsigned apdu_size, BACNET_ATOMIC_READ_FILE_DATA *data)
{
    int tag_len = 0;
    int apdu_len = 0;
    BACNET_OBJECT_TYPE object_type = OBJECT_NONE;
    uint32_t object_instance = 0;
    int32_t signed_integer;
    BACNET_UNSIGNED_INTEGER unsigned_integer;

    tag_len = bacnet_object_id_application_decode(
        &apdu[apdu_len], apdu_size - apdu_len, &object_type, &object_instance);
    if (tag_len <= 0) {
        return BACNET_STATUS_ERROR;
    }
    if (data) {
        data->object_type = (BACNET_OBJECT_TYPE)object_type;
        data->object_instance = object_instance;
    }
    apdu_len += tag_len;
    if (bacnet_is_opening_tag_number(
            &apdu[apdu_len], apdu_size - apdu_len, 0, &tag_len)) {
        if (data) {
            data->access = FILE_STREAM_ACCESS;
        }
        apdu_len += tag_len;
        /* fileStartPosition */
        tag_len = bacnet_signed_application_decode(
            &apdu[apdu_len], apdu_size - apdu_len, &signed_integer);
        if (tag_len <= 0) {
            return BACNET_STATUS_ERROR;
        }
        if (data) {
            data->type.stream.fileStartPosition = signed_integer;
        }
        apdu_len += tag_len;
        /* requestedOctetCount */
        tag_len = bacnet_unsigned_application_decode(
            &apdu[apdu_len], apdu_size - apdu_len, &unsigned_integer);
        if (tag_len <= 0) {
            return BACNET_STATUS_ERROR;
        }
        if (data) {
            data->type.stream.requestedOctetCount = unsigned_integer;
        }
        apdu_len += tag_len;
        /* closing tag */
        if (!bacnet_is_closing_tag_number(
                &apdu[apdu_len], apdu_size - apdu_len, 0, &tag_len)) {
            return BACNET_STATUS_ERROR;
        }
        apdu_len += tag_len;
    } else if (bacnet_is_opening_tag_number(
                   &apdu[apdu_len], apdu_size - apdu_len, 1, &tag_len)) {
        if (data) {
            data->access = FILE_RECORD_ACCESS;
        }
        apdu_len += tag_len;
        /* fileStartRecord */
        tag_len = bacnet_signed_application_decode(
            &apdu[apdu_len], apdu_size - apdu_len, &signed_integer);
        if (tag_len <= 0) {
            return BACNET_STATUS_ERROR;
        }
        if (data) {
            data->type.record.fileStartRecord = signed_integer;
        }
        apdu_len += tag_len;
        /* RecordCount */
        tag_len = bacnet_unsigned_application_decode(
            &apdu[apdu_len], apdu_size - apdu_len, &unsigned_integer);
        if (tag_len <= 0) {
            return BACNET_STATUS_ERROR;
        }
        if (data) {
            data->type.record.RecordCount = unsigned_integer;
        }
        apdu_len += tag_len;
        if (!bacnet_is_closing_tag_number(
                &apdu[apdu_len], apdu_size - apdu_len, 1, &tag_len)) {
            return BACNET_STATUS_ERROR;
        }
        apdu_len += tag_len;
    } else {
        return BACNET_STATUS_ERROR;
    }

    return apdu_len;
}

/**
 * @brief Decoding for AtomicReadFile APDU service data
 * @param apdu  Pointer to the buffer for decoding.
 * @param apdu_size  size of the buffer for decoding.
 * @param invoke_id [in] Invoked service ID.
 * @param data  Pointer to the property data values to be stored,
 *  or NULL for length
 * @return number of bytes decoded, or BACNET_STATUS_ERROR on error
 */
int arf_decode_apdu(uint8_t *apdu,
    unsigned apdu_size,
    uint8_t *invoke_id,
    BACNET_ATOMIC_READ_FILE_DATA *data)
{
    int apdu_len = 0, len = 0;

    if (!apdu) {
        return BACNET_STATUS_ERROR;
    }
    if (apdu_size < 4) {
        return BACNET_STATUS_ERROR;
    }
    if (apdu[0] != PDU_TYPE_CONFIRMED_SERVICE_REQUEST) {
        return BACNET_STATUS_ERROR;
    }
    /*  apdu[1] = encode_max_segs_max_apdu(0, MAX_APDU); */
    if (invoke_id) {
        *invoke_id = apdu[2]; /* invoke id - filled in by net layer */
    }
    if (apdu[3] != SERVICE_CONFIRMED_ATOMIC_READ_FILE) {
        return BACNET_STATUS_ERROR;
    }
    len = 4;
    apdu_len += len;
    len =
        arf_decode_service_request(&apdu[apdu_len], apdu_size - apdu_len, data);
    if (len <= 0) {
        return BACNET_STATUS_ERROR;
    }
    apdu_len += len;

    return apdu_len;
}

/**
 * @brief Encode the AtomicReadFile-ACK service request
 *
 *  AtomicReadFile-ACK ::= SEQUENCE {
 *      end-of-file BOOLEAN,
 *      access-method CHOICE {
 *          stream-access [0] SEQUENCE {
 *             file-start-position INTEGER,
 *              file-data OCTET STRING
 *          },
 *          record-access [1] SEQUENCE {
 *              file-start-record INTEGER,
 *              returned-record-count Unsigned,
 *              file-record-data SEQUENCE OF OCTET STRING
 *          }
 *      }
 *  }
 *
 * @param apdu  Pointer to the buffer for encoding, or NULL for length
 * @param data  Pointer to the data to be encoded
 * @return number of bytes encoded
 */
int arf_ack_service_encode_apdu(
    uint8_t *apdu, BACNET_ATOMIC_READ_FILE_DATA *data)
{
    int apdu_len = 0; /* total length of the apdu, return value */
    int len = 0;
    uint32_t i = 0;

    /* endOfFile */
    len = encode_application_boolean(apdu, data->endOfFile);
    if (apdu) {
        apdu += len;
    }
    apdu_len += len;
    switch (data->access) {
        case FILE_STREAM_ACCESS:
            len = encode_opening_tag(apdu, 0);
            apdu_len += len;
            if (apdu) {
                apdu += len;
            }
            len = encode_application_signed(
                apdu, data->type.stream.fileStartPosition);
            apdu_len += len;
            if (apdu) {
                apdu += len;
            }
            len = encode_application_octet_string(apdu, &data->fileData[0]);
            apdu_len += len;
            if (apdu) {
                apdu += len;
            }
            len = encode_closing_tag(apdu, 0);
            apdu_len += len;
            break;
        case FILE_RECORD_ACCESS:
            len = encode_opening_tag(apdu, 1);
            apdu_len += len;
            if (apdu) {
                apdu += len;
            }
            len = encode_application_signed(
                apdu, data->type.record.fileStartRecord);
            apdu_len += len;
            if (apdu) {
                apdu += len;
            }
            len = encode_application_unsigned(
                apdu, data->type.record.RecordCount);
            apdu_len += len;
            if (apdu) {
                apdu += len;
            }
            for (i = 0; i < data->type.record.RecordCount; i++) {
                len = encode_application_octet_string(apdu, &data->fileData[i]);
                apdu_len += len;
                if (apdu) {
                    apdu += len;
                }
            }
            len = encode_closing_tag(apdu, 1);
            apdu_len += len;
            break;
        default:
            break;
    }

    return apdu_len;
}

/**
 * @brief Encode the AtomicReadFile-ACK service request
 * @param apdu  Pointer to the buffer for decoding.
 * @param invoke_id original invoke id for request
 * @param data  Pointer to the property decoded data to be stored
 * @return number of bytes encoded
 */
int arf_ack_encode_apdu(
    uint8_t *apdu, uint8_t invoke_id, BACNET_ATOMIC_READ_FILE_DATA *data)
{
    int apdu_len = 0; /* total length of the apdu, return value */
    int len = 0;

    if (apdu) {
        apdu[0] = PDU_TYPE_COMPLEX_ACK;
        apdu[1] = invoke_id;
        apdu[2] = SERVICE_CONFIRMED_ATOMIC_READ_FILE; /* service choice */
    }
    len = 3;
    apdu_len += len;
    if (apdu) {
        apdu += len;
    }
    len = arf_ack_service_encode_apdu(apdu, data);
    apdu_len += len;

    return apdu_len;
}

/**
 * @brief Decoding for AtomicReadFile-ACK APDU service data
 *
 *  AtomicReadFile-ACK ::= SEQUENCE {
 *      end-of-file BOOLEAN,
 *      access-method CHOICE {
 *          stream-access [0] SEQUENCE {
 *             file-start-position INTEGER,
 *              file-data OCTET STRING
 *          },
 *          record-access [1] SEQUENCE {
 *              file-start-record INTEGER,
 *              returned-record-count Unsigned,
 *              file-record-data SEQUENCE OF OCTET STRING
 *          }
 *      }
 *  }
 *
 * @param apdu  Pointer to the buffer for decoding.
 * @param apdu_size  size of the buffer for decoding.
 * @param data  Pointer to the property data to be encoded,
 *  or NULL for length
 * @return Bytes encoded or BACNET_STATUS_ERROR on error.
 */
int arf_ack_decode_service_request(
    uint8_t *apdu, unsigned apdu_size, BACNET_ATOMIC_READ_FILE_DATA *data)
{
    int apdu_len = 0;
    int len = 0;
    bool endOfFile;
    int32_t signed_integer;
    BACNET_UNSIGNED_INTEGER record_count, i;
    BACNET_OCTET_STRING *octet_string = NULL;

    len = bacnet_boolean_application_decode(apdu, apdu_size, &endOfFile);
    if (len <= 0) {
        return BACNET_STATUS_ERROR;
    }
    if (data) {
        data->endOfFile = endOfFile;
    }
    apdu_len += len;
    if (bacnet_is_opening_tag_number(
            &apdu[apdu_len], apdu_size - apdu_len, 0, &len)) {
        if (data) {
            data->access = FILE_STREAM_ACCESS;
        }
        apdu_len += len;
        /* fileStartPosition */
        len = bacnet_signed_application_decode(
            &apdu[apdu_len], apdu_size - apdu_len, &signed_integer);
        if (len <= 0) {
            return BACNET_STATUS_ERROR;
        }
        if (data) {
            data->type.stream.fileStartPosition = signed_integer;
        }
        apdu_len += len;
        /* fileData */
        if (data) {
            octet_string = &data->fileData[0];
        }
        len = bacnet_octet_string_application_decode(
            &apdu[apdu_len], apdu_size - apdu_len, octet_string);
        if (len <= 0) {
            return BACNET_STATUS_ERROR;
        }
        apdu_len += len;
        if (!bacnet_is_closing_tag_number(
                &apdu[apdu_len], apdu_size - apdu_len, 0, &len)) {
            return BACNET_STATUS_ERROR;
        }
        apdu_len += len;
    } else if (bacnet_is_opening_tag_number(
                   &apdu[apdu_len], apdu_size - apdu_len, 1, &len)) {
        if (data) {
            data->access = FILE_RECORD_ACCESS;
        }
        apdu_len += len;
        /* fileStartRecord */
        len = bacnet_signed_application_decode(
            &apdu[apdu_len], apdu_size - apdu_len, &signed_integer);
        if (len <= 0) {
            return BACNET_STATUS_ERROR;
        }
        if (data) {
            data->type.record.fileStartRecord = signed_integer;
        }
        apdu_len += len;
        /* returnedRecordCount */
        len = bacnet_unsigned_application_decode(
            &apdu[apdu_len], apdu_size - apdu_len, &record_count);
        if (len <= 0) {
            return BACNET_STATUS_ERROR;
        }
        if (data) {
            data->type.record.RecordCount = record_count;
        }
        apdu_len += len;
        for (i = 0; i < record_count; i++) {
            /* fileData */
            if (i >= BACNET_READ_FILE_RECORD_COUNT) {
                octet_string = NULL;
            } else if (data) {
                octet_string = &data->fileData[i];
            } else {
                octet_string = NULL;
            }
            len = bacnet_octet_string_application_decode(
                &apdu[apdu_len], apdu_size - apdu_len, octet_string);
            if (len <= 0) {
                return BACNET_STATUS_ERROR;
            }
            apdu_len += len;
        }
        if (!bacnet_is_closing_tag_number(
                &apdu[apdu_len], apdu_size - apdu_len, 1, &len)) {
            return BACNET_STATUS_ERROR;
        }
        apdu_len += len;
    } else {
        return BACNET_STATUS_ERROR;
    }

    return apdu_len;
}

/**
 * @brief Decoding for AtomicReadFile-ACK APDU service data
 * @param apdu  Pointer to the buffer for decoding.
 * @param apdu_size  size of the buffer for decoding.
 * @param invoke_id [in] Invoked service ID.
 * @param data  Pointer to the property data values to be stored,
 *  or NULL for length
 * @return number of bytes decoded, or BACNET_STATUS_ERROR on error
 */
int arf_ack_decode_apdu(uint8_t *apdu,
    unsigned apdu_size,
    uint8_t *invoke_id,
    BACNET_ATOMIC_READ_FILE_DATA *data)
{
    int apdu_len = 0, len = 0;

    if (!apdu) {
        return BACNET_STATUS_ERROR;
    }
    if (apdu_size < 3) {
        return BACNET_STATUS_ERROR;
    }
    if (apdu[0] != PDU_TYPE_COMPLEX_ACK) {
        return BACNET_STATUS_ERROR;
    }
    if (invoke_id) {
        *invoke_id = apdu[1]; /* invoke id - filled in by net layer */
    }
    if (apdu[2] != SERVICE_CONFIRMED_ATOMIC_READ_FILE) {
        return BACNET_STATUS_ERROR;
    }
    len = 3;
    apdu_len += len;
    len = arf_ack_decode_service_request(
        &apdu[apdu_len], apdu_size - apdu_len, data);
    if (len <= 0) {
        return BACNET_STATUS_ERROR;
    }
    apdu_len += len;

    return apdu_len;
}
