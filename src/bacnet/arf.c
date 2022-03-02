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

/* encode service */
int arf_encode_apdu(
    uint8_t *apdu, uint8_t invoke_id, BACNET_ATOMIC_READ_FILE_DATA *data)
{
    int apdu_len = 0; /* total length of the apdu, return value */

    if (apdu) {
        apdu[0] = PDU_TYPE_CONFIRMED_SERVICE_REQUEST;
        apdu[1] = encode_max_segs_max_apdu(0, MAX_APDU);
        apdu[2] = invoke_id;
        apdu[3] = SERVICE_CONFIRMED_ATOMIC_READ_FILE; /* service choice */
        apdu_len = 4;
        apdu_len += encode_application_object_id(
            &apdu[apdu_len], data->object_type, data->object_instance);
        switch (data->access) {
            case FILE_STREAM_ACCESS:
                apdu_len += encode_opening_tag(&apdu[apdu_len], 0);
                apdu_len += encode_application_signed(
                    &apdu[apdu_len], data->type.stream.fileStartPosition);
                apdu_len += encode_application_unsigned(
                    &apdu[apdu_len], data->type.stream.requestedOctetCount);
                apdu_len += encode_closing_tag(&apdu[apdu_len], 0);
                break;
            case FILE_RECORD_ACCESS:
                apdu_len += encode_opening_tag(&apdu[apdu_len], 1);
                apdu_len += encode_application_signed(
                    &apdu[apdu_len], data->type.record.fileStartRecord);
                apdu_len += encode_application_unsigned(
                    &apdu[apdu_len], data->type.record.RecordCount);
                apdu_len += encode_closing_tag(&apdu[apdu_len], 1);
                break;
            default:
                break;
        }
    }

    return apdu_len;
}

/* decode the service request only */
int arf_decode_service_request(
    uint8_t *apdu, unsigned apdu_len_max, BACNET_ATOMIC_READ_FILE_DATA *data)
{
    int len = 0;
    int apdu_len = BACNET_STATUS_ERROR;
    BACNET_OBJECT_TYPE object_type = OBJECT_NONE;
    uint32_t object_instance = 0;

    /* check for value pointers */
    if ((apdu_len_max == 0) || (!data)) {
        return BACNET_STATUS_ERROR;
    }
    len = bacnet_object_id_application_decode(
        &apdu[0], apdu_len_max, &object_type, &object_instance);
    if (len <= 0) {
        return BACNET_STATUS_ERROR;
    }
    data->object_type = (BACNET_OBJECT_TYPE)object_type;
    data->object_instance = object_instance;
    apdu_len = len;
    if (apdu_len < apdu_len_max) {
        if (decode_is_opening_tag_number(&apdu[apdu_len], 0)) {
            data->access = FILE_STREAM_ACCESS;
            /* tag number 0 is not extended so only one octet */
            apdu_len++;
            /* fileStartPosition */
            if (apdu_len >= apdu_len_max) {
                return BACNET_STATUS_ERROR;
            }
            len = bacnet_signed_application_decode(&apdu[apdu_len],
                apdu_len_max - apdu_len, &data->type.stream.fileStartPosition);
            if (len <= 0) {
                return BACNET_STATUS_ERROR;
            }
            apdu_len += len;
            /* requestedOctetCount */
            if (apdu_len >= apdu_len_max) {
                return BACNET_STATUS_ERROR;
            }
            len = bacnet_unsigned_application_decode(&apdu[apdu_len],
                apdu_len_max, &data->type.stream.requestedOctetCount);
            if (len <= 0) {
                return BACNET_STATUS_ERROR;
            }
            apdu_len += len;
            /* closing tag */
            if (apdu_len >= apdu_len_max) {
                return BACNET_STATUS_ERROR;
            }
            if (!decode_is_closing_tag_number(&apdu[apdu_len], 0)) {
                return BACNET_STATUS_ERROR;
            }
            /* tag number 0 is not extended so only one octet */
            apdu_len++;
        } else if (decode_is_opening_tag_number(&apdu[len], 1)) {
            data->access = FILE_RECORD_ACCESS;
            /* tag number 1 is not extended so only one octet */
            apdu_len++;
            if (apdu_len >= apdu_len_max) {
                return BACNET_STATUS_ERROR;
            }
            /* fileStartRecord */
            len = bacnet_signed_application_decode(&apdu[apdu_len],
                apdu_len_max - apdu_len, &data->type.record.fileStartRecord);
            if (len <= 0) {
                return BACNET_STATUS_ERROR;
            }
            apdu_len += len;
            if (apdu_len >= apdu_len_max) {
                return BACNET_STATUS_ERROR;
            }
            /* RecordCount */
            len = bacnet_unsigned_application_decode(
                &apdu[apdu_len], apdu_len_max, &data->type.record.RecordCount);
            if (len <= 0) {
                return BACNET_STATUS_ERROR;
            }
            apdu_len += len;
            if (apdu_len >= apdu_len_max) {
                return BACNET_STATUS_ERROR;
            }
            if (!decode_is_closing_tag_number(&apdu[apdu_len], 1)) {
                return BACNET_STATUS_ERROR;
            }
            /* tag number 1 is not extended so only one octet */
            apdu_len++;
        } else {
            return BACNET_STATUS_ERROR;
        }
    } else {
        return BACNET_STATUS_ERROR;
    }

    return apdu_len;
}

int arf_decode_apdu(uint8_t *apdu,
    unsigned apdu_len,
    uint8_t *invoke_id,
    BACNET_ATOMIC_READ_FILE_DATA *data)
{
    int len = 0;
    unsigned offset = 0;

    if (!apdu) {
        return BACNET_STATUS_ERROR;
    }
    /* optional checking - most likely was already done prior to this call */
    if (apdu[0] != PDU_TYPE_CONFIRMED_SERVICE_REQUEST) {
        return BACNET_STATUS_ERROR;
    }
    /*  apdu[1] = encode_max_segs_max_apdu(0, MAX_APDU); */
    *invoke_id = apdu[2]; /* invoke id - filled in by net layer */
    if (apdu[3] != SERVICE_CONFIRMED_ATOMIC_READ_FILE) {
        return BACNET_STATUS_ERROR;
    }
    offset = 4;

    if (apdu_len > offset) {
        len =
            arf_decode_service_request(&apdu[offset], apdu_len - offset, data);
    }

    return len;
}

/* encode service */
int arf_ack_encode_apdu(
    uint8_t *apdu, uint8_t invoke_id, BACNET_ATOMIC_READ_FILE_DATA *data)
{
    int apdu_len = 0; /* total length of the apdu, return value */
    uint32_t i = 0;

    if (apdu) {
        apdu[0] = PDU_TYPE_COMPLEX_ACK;
        apdu[1] = invoke_id;
        apdu[2] = SERVICE_CONFIRMED_ATOMIC_READ_FILE; /* service choice */
        apdu_len = 3;
        /* endOfFile */
        apdu_len +=
            encode_application_boolean(&apdu[apdu_len], data->endOfFile);
        switch (data->access) {
            case FILE_STREAM_ACCESS:
                apdu_len += encode_opening_tag(&apdu[apdu_len], 0);
                apdu_len += encode_application_signed(
                    &apdu[apdu_len], data->type.stream.fileStartPosition);
                apdu_len += encode_application_octet_string(
                    &apdu[apdu_len], &data->fileData[0]);
                apdu_len += encode_closing_tag(&apdu[apdu_len], 0);
                break;
            case FILE_RECORD_ACCESS:
                apdu_len += encode_opening_tag(&apdu[apdu_len], 1);
                apdu_len += encode_application_signed(
                    &apdu[apdu_len], data->type.record.fileStartRecord);
                apdu_len += encode_application_unsigned(
                    &apdu[apdu_len], data->type.record.RecordCount);
                for (i = 0; i < data->type.record.RecordCount; i++) {
                    apdu_len += encode_application_octet_string(
                        &apdu[apdu_len], &data->fileData[i]);
                }
                apdu_len += encode_closing_tag(&apdu[apdu_len], 1);
                break;
            default:
                break;
        }
    }

    return apdu_len;
}

/* decode the service request only */
int arf_ack_decode_service_request(
    uint8_t *apdu, unsigned apdu_len, BACNET_ATOMIC_READ_FILE_DATA *data)
{
    int len = 0;
    int tag_len = 0;
    int decoded_len = 0;
    uint8_t tag_number = 0;
    uint32_t len_value_type = 0;
    uint32_t i = 0;

    /* check for value pointers */
    if (apdu_len && data) {
        len =
            decode_tag_number_and_value(&apdu[0], &tag_number, &len_value_type);
        if (tag_number != BACNET_APPLICATION_TAG_BOOLEAN) {
            return BACNET_STATUS_ERROR;
        }
        data->endOfFile = decode_boolean(len_value_type);
        if (decode_is_opening_tag_number(&apdu[len], 0)) {
            data->access = FILE_STREAM_ACCESS;
            /* a tag number is not extended so only one octet */
            len++;
            /* fileStartPosition */
            tag_len = decode_tag_number_and_value(
                &apdu[len], &tag_number, &len_value_type);
            len += tag_len;
            if (tag_number != BACNET_APPLICATION_TAG_SIGNED_INT) {
                return BACNET_STATUS_ERROR;
            }
            len += decode_signed(&apdu[len], len_value_type,
                &data->type.stream.fileStartPosition);
            /* fileData */
            tag_len = decode_tag_number_and_value(
                &apdu[len], &tag_number, &len_value_type);
            len += tag_len;
            if (tag_number != BACNET_APPLICATION_TAG_OCTET_STRING) {
                return BACNET_STATUS_ERROR;
            }
            decoded_len = decode_octet_string(
                &apdu[len], len_value_type, &data->fileData[0]);
            if ((uint32_t)decoded_len != len_value_type) {
                return BACNET_STATUS_ERROR;
            }
            len += decoded_len;
            if (!decode_is_closing_tag_number(&apdu[len], 0)) {
                return BACNET_STATUS_ERROR;
            }
            /* a tag number is not extended so only one octet */
            len++;
        } else if (decode_is_opening_tag_number(&apdu[len], 1)) {
            data->access = FILE_RECORD_ACCESS;
            /* a tag number is not extended so only one octet */
            len++;
            /* fileStartRecord */
            tag_len = decode_tag_number_and_value(
                &apdu[len], &tag_number, &len_value_type);
            len += tag_len;
            if (tag_number != BACNET_APPLICATION_TAG_SIGNED_INT) {
                return BACNET_STATUS_ERROR;
            }
            len += decode_signed(
                &apdu[len], len_value_type, &data->type.record.fileStartRecord);
            /* returnedRecordCount */
            tag_len = decode_tag_number_and_value(
                &apdu[len], &tag_number, &len_value_type);
            len += tag_len;
            if (tag_number != BACNET_APPLICATION_TAG_UNSIGNED_INT) {
                return BACNET_STATUS_ERROR;
            }
            len += decode_unsigned(
                &apdu[len], len_value_type, &data->type.record.RecordCount);
            for (i = 0; i < data->type.record.RecordCount; i++) {
                /* fileData */
                tag_len = decode_tag_number_and_value(
                    &apdu[len], &tag_number, &len_value_type);
                len += tag_len;
                if (tag_number != BACNET_APPLICATION_TAG_OCTET_STRING) {
                    return BACNET_STATUS_ERROR;
                }
                decoded_len = decode_octet_string(
                    &apdu[len], len_value_type, &data->fileData[i]);
                if ((uint32_t)decoded_len != len_value_type) {
                    return BACNET_STATUS_ERROR;
                }
                len += decoded_len;
            }
            if (!decode_is_closing_tag_number(&apdu[len], 1)) {
                return BACNET_STATUS_ERROR;
            }
            /* a tag number is not extended so only one octet */
            len++;
        } else {
            return BACNET_STATUS_ERROR;
        }
    }

    return len;
}

int arf_ack_decode_apdu(uint8_t *apdu,
    unsigned apdu_len,
    uint8_t *invoke_id,
    BACNET_ATOMIC_READ_FILE_DATA *data)
{
    int len = 0;
    unsigned offset = 0;

    if (!apdu) {
        return BACNET_STATUS_ERROR;
    }
    /* optional checking - most likely was already done prior to this call */
    if (apdu[0] != PDU_TYPE_COMPLEX_ACK) {
        return BACNET_STATUS_ERROR;
    }
    *invoke_id = apdu[1]; /* invoke id - filled in by net layer */
    if (apdu[2] != SERVICE_CONFIRMED_ATOMIC_READ_FILE) {
        return BACNET_STATUS_ERROR;
    }
    offset = 3;

    if (apdu_len > offset) {
        len = arf_ack_decode_service_request(
            &apdu[offset], apdu_len - offset, data);
    }

    return len;
}
