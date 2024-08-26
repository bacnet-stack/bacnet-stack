/**
 * @file
 * @brief BACnet AtomicWriteFile service structures, codecs, and handlers.
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date 2005
 * @copyright SPDX-License-Identifier: GPL-2.0-or-later WITH GCC-exception-2.0
*/
#include <stdint.h>
/* BACnet Stack defines - first */
#include "bacnet/bacdef.h"
/* BACnet Stack API */
#include "bacnet/bacdcode.h"
#include "bacnet/awf.h"

/**
 * @brief Encode the AtomicWriteFile service request
 *
 *  AtomicWriteFile-Request ::= SEQUENCE {
 *      file-identifier BACnetObjectIdentifier,
 *      access-method CHOICE {
 *          stream-access [0] SEQUENCE {
 *              file-start-position INTEGER,
 *              file-data OCTET STRING
 *          },
 *          record-access [1] SEQUENCE {
 *              file-start-record INTEGER,
 *              record-count Unsigned
 *              file-record-data SEQUENCE OF OCTET STRING
 *          }
 *      }
 *  }
 *
 * @param apdu  Pointer to the buffer for encoded values
 * @param data  Pointer to the service data used for encoding values
 * @return number of bytes encoded
 */
int awf_service_encode_apdu(uint8_t *apdu, BACNET_ATOMIC_WRITE_FILE_DATA *data)
{
    int apdu_len = 0; /* total length of the apdu, return value */
    int len = 0;
    uint32_t i = 0;

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
                apdu, data->type.record.returnedRecordCount);
            apdu_len += len;
            if (apdu) {
                apdu += len;
            }
            for (i = 0; i < data->type.record.returnedRecordCount; i++) {
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
 * @brief Encode the AtomicWriteFile service request
 * @param apdu Pointer to the buffer for encoded values
 * @param apdu_size number of bytes available in the buffer
 * @param data  Pointer to the service data used for encoding values
 * @return number of bytes encoded, or zero if unable to encode or too large
 */
int atomicwritefile_service_request_encode(
    uint8_t *apdu, size_t apdu_size, BACNET_ATOMIC_WRITE_FILE_DATA *data)
{
    size_t apdu_len = 0; /* total length of the apdu, return value */

    apdu_len = awf_service_encode_apdu(NULL, data);
    if (apdu_len > apdu_size) {
        apdu_len = 0;
    } else {
        apdu_len = awf_service_encode_apdu(apdu, data);
    }

    return apdu_len;
}

/**
 * @brief Encode the AtomicWriteFile service request
 * @param apdu  Pointer to the buffer for decoding.
 * @param invoke_id original invoke id for request
 * @param data  Pointer to the property decoded data to be stored
 * @return number of bytes encoded
 */
int awf_encode_apdu(
    uint8_t *apdu, uint8_t invoke_id, BACNET_ATOMIC_WRITE_FILE_DATA *data)
{
    int apdu_len = 0; /* total length of the apdu, return value */
    int len = 0;

    if (apdu) {
        apdu[0] = PDU_TYPE_CONFIRMED_SERVICE_REQUEST;
        apdu[1] = encode_max_segs_max_apdu(0, MAX_APDU);
        apdu[2] = invoke_id;
        apdu[3] = SERVICE_CONFIRMED_ATOMIC_WRITE_FILE; /* service choice */
    }
    len = 4;
    apdu_len += len;
    if (apdu) {
        apdu += len;
    }
    len = awf_service_encode_apdu(apdu, data);
    apdu_len += len;

    return apdu_len;
}

/**
 * @brief Decode the AtomicWriteFile service request
 *
 *  AtomicWriteFile-Request ::= SEQUENCE {
 *      file-identifier BACnetObjectIdentifier,
 *      access-method CHOICE {
 *          stream-access [0] SEQUENCE {
 *              file-start-position INTEGER,
 *              file-data OCTET STRING
 *          },
 *          record-access [1] SEQUENCE {
 *              file-start-record INTEGER,
 *              record-count Unsigned
 *              file-record-data SEQUENCE OF OCTET STRING
 *          }
 *      }
 *  }
 *
 * @param apdu  Pointer to the buffer for decoding.
 * @param apdu_size  Count of valid bytes in the buffer.
 * @param data  Pointer to the property decoded data to be stored,
 *  or NULL for length
 *
 * @return number of bytes decoded or BACNET_STATUS_ERROR on error.
 */
int awf_decode_service_request(
    uint8_t *apdu, unsigned apdu_size, BACNET_ATOMIC_WRITE_FILE_DATA *data)
{
    /* return value */
    int apdu_len = 0;
    int len = 0;
    BACNET_OBJECT_TYPE object_type = OBJECT_NONE;
    uint32_t object_instance = 0;
    int32_t signed_integer;
    BACNET_UNSIGNED_INTEGER unsigned_value = 0;
    uint32_t record_count = 0;
    uint32_t i = 0;
    BACNET_OCTET_STRING *octet_string = NULL;

    len = bacnet_object_id_application_decode(
        &apdu[apdu_len], apdu_size - apdu_len, &object_type, &object_instance);
    if (len <= 0) {
        return BACNET_STATUS_ERROR;
    }
    if (data) {
        data->object_type = object_type;
        data->object_instance = object_instance;
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
        /* closing tag */
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
            &apdu[apdu_len], apdu_size - apdu_len, &unsigned_value);
        if (len <= 0) {
            return BACNET_STATUS_ERROR;
        }
        if (data) {
            data->type.record.returnedRecordCount = unsigned_value;
        }
        record_count = unsigned_value;
        apdu_len += len;
        for (i = 0; i < record_count; i++) {
            /* fileData */
            if (i >= BACNET_WRITE_FILE_RECORD_COUNT) {
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
 * @brief Decoding for AtomicWriteFile APDU service data
 * @param apdu  Pointer to the buffer for decoding.
 * @param apdu_size  size of the buffer for decoding.
 * @param invoke_id [in] Invoked service ID.
 * @param data  Pointer to the property data values to be stored,
 *  or NULL for length
 * @return number of bytes decoded, or BACNET_STATUS_ERROR on error
 */
int awf_decode_apdu(uint8_t *apdu,
    unsigned apdu_size,
    uint8_t *invoke_id,
    BACNET_ATOMIC_WRITE_FILE_DATA *data)
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
    if (invoke_id) {
        *invoke_id = apdu[2];
    }
    if (apdu[3] != SERVICE_CONFIRMED_ATOMIC_WRITE_FILE) {
        return BACNET_STATUS_ERROR;
    }
    len = 4;
    apdu_len += len;
    len =
        awf_decode_service_request(&apdu[apdu_len], apdu_size - apdu_len, data);
    if (len <= 0) {
        return BACNET_STATUS_ERROR;
    }
    apdu_len += len;

    return apdu_len;
}

/**
 * @brief Encode the AtomicWriteFile-ACK service request
 *
 *  AtomicWriteFile-ACK ::= CHOICE {
 *      file-start-position [0] INTEGER,
 *      file-start-record [1] INTEGER
 *  }
 *
 * @param apdu  Pointer to the buffer for encoding, or NULL for length
 * @param data  Pointer to the data to be encoded
 * @return number of bytes encoded
 */
int awf_ack_service_encode_apdu(
    uint8_t *apdu, BACNET_ATOMIC_WRITE_FILE_DATA *data)
{
    int apdu_len = 0; /* total length of the apdu, return value */

    switch (data->access) {
        case FILE_STREAM_ACCESS:
            apdu_len = encode_context_signed(
                apdu, 0, data->type.stream.fileStartPosition);
            break;
        case FILE_RECORD_ACCESS:
            apdu_len = encode_context_signed(
                apdu, 1, data->type.record.fileStartRecord);
            break;
        default:
            break;
    }

    return apdu_len;
}

/**
 * @brief Encode the AtomicWriteFile-ACK service request
 * @param apdu  Pointer to the buffer for decoding.
 * @param invoke_id original invoke id for request
 * @param data  Pointer to the property decoded data to be stored
 * @return number of bytes encoded
 */
int awf_ack_encode_apdu(
    uint8_t *apdu, uint8_t invoke_id, BACNET_ATOMIC_WRITE_FILE_DATA *data)
{
    int apdu_len = 0; /* total length of the apdu, return value */
    int len = 0;

    if (apdu) {
        apdu[0] = PDU_TYPE_COMPLEX_ACK;
        apdu[1] = invoke_id;
        apdu[2] = SERVICE_CONFIRMED_ATOMIC_WRITE_FILE; /* service choice */
    }
    len = 3;
    apdu_len += len;
    if (apdu) {
        apdu += len;
    }
    len = awf_ack_service_encode_apdu(apdu, data);
    apdu_len += len;

    return apdu_len;
}

/**
 * @brief Decoding for AtomicWriteFile-ACK APDU service data
 *
 *  AtomicWriteFile-ACK ::= CHOICE {
 *      file-start-position [0] INTEGER,
 *      file-start-record [1] INTEGER
 *  }
 *
 * @param apdu  Pointer to the buffer for decoding.
 * @param apdu_size  size of the buffer for decoding.
 * @param data  Pointer to the property data to be encoded,
 *  or NULL for length
 * @return number of bytes encoded or BACNET_STATUS_ERROR on error.
 */
int awf_ack_decode_service_request(
    uint8_t *apdu, unsigned apdu_size, BACNET_ATOMIC_WRITE_FILE_DATA *data)
{
    int len = 0, apdu_len = 0;
    int32_t signed_integer;
    BACNET_TAG tag = { 0 };

    len = bacnet_tag_decode(apdu, apdu_size, &tag);
    if ((len > 0) && tag.context) {
        if (tag.number == 0) {
            /* file-start-position [0] INTEGER */
            len = bacnet_signed_context_decode(
                &apdu[apdu_len], apdu_size - apdu_len, 0, &signed_integer);
            if (len > 0) {
                if (data) {
                    data->access = FILE_STREAM_ACCESS;
                    data->type.stream.fileStartPosition = signed_integer;
                }
                apdu_len += len;
            } else {
                return BACNET_STATUS_ERROR;
            }
        } else if (tag.number == 1) {
            /* file-start-record [1] INTEGER */
            len = bacnet_signed_context_decode(
                &apdu[apdu_len], apdu_size - apdu_len, 1, &signed_integer);
            if (len > 0) {
                if (data) {
                    data->access = FILE_RECORD_ACCESS;
                    data->type.record.fileStartRecord = signed_integer;
                }
                apdu_len += len;
            } else {
                return BACNET_STATUS_ERROR;
            }
        } else {
            return BACNET_STATUS_ERROR;
        }
    } else {
        return BACNET_STATUS_ERROR;
    }

    return apdu_len;
}

/**
 * @brief Decoding for AtomicWriteFile-ACK APDU service data
 * @param apdu  Pointer to the buffer for decoding.
 * @param apdu_size  size of the buffer for decoding.
 * @param invoke_id [in] Invoked service ID.
 * @param data  Pointer to the property data values to be stored,
 *  or NULL for length
 * @return number of bytes decoded, or BACNET_STATUS_ERROR on error
 */
int awf_ack_decode_apdu(uint8_t *apdu,
    unsigned apdu_size,
    uint8_t *invoke_id,
    BACNET_ATOMIC_WRITE_FILE_DATA *data)
{
    int len = 0;
    int apdu_len = 0;

    if (!apdu) {
        return BACNET_STATUS_ERROR;
    }
    if (apdu_size < 3) {
        return BACNET_STATUS_ERROR;
    }
    /* optional checking - most likely was already done prior to this call */
    if (apdu[0] != PDU_TYPE_COMPLEX_ACK) {
        return BACNET_STATUS_ERROR;
    }
    if (invoke_id) {
        *invoke_id = apdu[1]; /* invoke id - filled in by net layer */
    }
    if (apdu[2] != SERVICE_CONFIRMED_ATOMIC_WRITE_FILE) {
        return BACNET_STATUS_ERROR;
    }
    len = 3;
    apdu_len += len;
    len = awf_ack_decode_service_request(
        &apdu[apdu_len], apdu_size - apdu_len, data);
    if (len <= 0) {
        return BACNET_STATUS_ERROR;
    }

    return apdu_len;
}
