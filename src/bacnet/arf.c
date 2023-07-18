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
    uint8_t *apdu, unsigned apdu_size, BACNET_ATOMIC_READ_FILE_DATA *data)
{
    int tag_len = 0;
    int apdu_len = BACNET_STATUS_ERROR;
    BACNET_OBJECT_TYPE object_type = OBJECT_NONE;
    uint32_t object_instance = 0;

    /* check for value pointers */
    if ((apdu_size == 0) || (!data)) {
        return BACNET_STATUS_ERROR;
    }
    tag_len = bacnet_object_id_application_decode(
        &apdu[0], apdu_size, &object_type, &object_instance);
    if (tag_len == BACNET_STATUS_ERROR) {
        return BACNET_STATUS_ERROR;
    }
    data->object_type = (BACNET_OBJECT_TYPE)object_type;
    data->object_instance = object_instance;
    apdu_len = tag_len;
    if (apdu_size <= apdu_len) {
        return BACNET_STATUS_ERROR;
    }
    if (bacnet_is_opening_tag_number(
            &apdu[apdu_len], apdu_size - apdu_len, 0, &tag_len)) {
        data->access = FILE_STREAM_ACCESS;
        apdu_len += tag_len;
        /* fileStartPosition */
        if (apdu_size <= apdu_len) {
            return BACNET_STATUS_ERROR;
        }
        tag_len = bacnet_signed_application_decode(&apdu[apdu_len],
            apdu_size - apdu_len, &data->type.stream.fileStartPosition);
        if (tag_len == BACNET_STATUS_ERROR) {
            return BACNET_STATUS_ERROR;
        }
        apdu_len += tag_len;
        /* requestedOctetCount */
        if (apdu_size <= apdu_len) {
            return BACNET_STATUS_ERROR;
        }
        tag_len = bacnet_unsigned_application_decode(&apdu[apdu_len],
            apdu_size - apdu_len, &data->type.stream.requestedOctetCount);
        if (tag_len == BACNET_STATUS_ERROR) {
            return BACNET_STATUS_ERROR;
        }
        apdu_len += tag_len;
        /* closing tag */
        if (apdu_size <= apdu_len) {
            return BACNET_STATUS_ERROR;
        }
        if (!bacnet_is_closing_tag_number(
                &apdu[apdu_len], apdu_size - apdu_len, 0, &tag_len)) {
            return BACNET_STATUS_ERROR;
        }
        apdu_len += tag_len;
    } else if (bacnet_is_opening_tag_number(
                   &apdu[apdu_len], apdu_size - apdu_len, 1, &tag_len)) {
        data->access = FILE_RECORD_ACCESS;
        apdu_len += tag_len;
        if (apdu_size <= apdu_len) {
            return BACNET_STATUS_ERROR;
        }
        /* fileStartRecord */
        tag_len = bacnet_signed_application_decode(&apdu[apdu_len],
            apdu_size - apdu_len, &data->type.record.fileStartRecord);
        if (tag_len == BACNET_STATUS_ERROR) {
            return BACNET_STATUS_ERROR;
        }
        apdu_len += tag_len;
        if (apdu_size <= apdu_len) {
            return BACNET_STATUS_ERROR;
        }
        /* RecordCount */
        tag_len = bacnet_unsigned_application_decode(&apdu[apdu_len],
            apdu_size - apdu_len, &data->type.record.RecordCount);
        if (tag_len == BACNET_STATUS_ERROR) {
            return BACNET_STATUS_ERROR;
        }
        apdu_len += tag_len;
        if (apdu_size <= apdu_len) {
            return BACNET_STATUS_ERROR;
        }
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
    uint8_t *apdu, unsigned apdu_size, BACNET_ATOMIC_READ_FILE_DATA *data)
{
    int apdu_len = 0;
    int tag_len = 0;
    int decoded_len = 0;
    uint8_t tag_number = 0;
    uint32_t len_value_type = 0;
    uint32_t i = 0;

    /* check for value pointers */
    if (apdu_size && data) {
        tag_len =
            bacnet_boolean_application_decode(apdu, apdu_size, &data->endOfFile);
        if (tag_len == BACNET_STATUS_ERROR) {
            return BACNET_STATUS_ERROR;
        }
        apdu_len = tag_len;
        if (bacnet_is_opening_tag_number(&apdu[apdu_len],apdu_size-apdu_len, 0, &tag_len)) {
            data->access = FILE_STREAM_ACCESS;
            apdu_len += tag_len;
            /* fileStartPosition */
            if (apdu_size <= apdu_len) {
                return BACNET_STATUS_ERROR;
            }
            tag_len = bacnet_signed_application_decode(&apdu[apdu_len],
                apdu_size - apdu_len, &data->type.stream.fileStartPosition);
            if (tag_len == BACNET_STATUS_ERROR) {
                return BACNET_STATUS_ERROR;
            }
            apdu_len += tag_len;
            /* fileData */
            if (apdu_size <= apdu_len) {
                return BACNET_STATUS_ERROR;
            }
            tag_len = bacnet_octet_string_application_decode(
                &apdu[apdu_len], apdu_size - apdu_len, &data->fileData[0]);
            if (tag_len == BACNET_STATUS_ERROR) {
                return BACNET_STATUS_ERROR;
            }
            apdu_len += tag_len;
            if (apdu_size <= apdu_len) {
                return BACNET_STATUS_ERROR;
            }
            if (!bacnet_is_closing_tag_number(
                    &apdu[apdu_len], apdu_size - apdu_len, 0, &tag_len)) {
                return BACNET_STATUS_ERROR;
            }
            apdu_len += tag_len;
        } else if (bacnet_is_opening_tag_number(&apdu[apdu_len],apdu_size-apdu_len, 1, &tag_len)) {
            data->access = FILE_RECORD_ACCESS;
            apdu_len += tag_len;
            /* fileStartRecord */
            if (apdu_size <= apdu_len) {
                return BACNET_STATUS_ERROR;
            }
            tag_len = bacnet_signed_application_decode(&apdu[apdu_len],
                apdu_size - apdu_len, &data->type.record.fileStartRecord);
            if (tag_len == BACNET_STATUS_ERROR) {
                return BACNET_STATUS_ERROR;
            }
            apdu_len += tag_len;
            /* returnedRecordCount */
            if (apdu_size <= apdu_len) {
                return BACNET_STATUS_ERROR;
            }
            tag_len = bacnet_unsigned_application_decode(&apdu[apdu_len],
                apdu_size - apdu_len, &data->type.record.RecordCount);
            if (tag_len == BACNET_STATUS_ERROR) {
                return BACNET_STATUS_ERROR;
            }
            apdu_len += tag_len;
            if (apdu_size <= apdu_len) {
                return BACNET_STATUS_ERROR;
            }
            if (data->type.record.RecordCount > BACNET_READ_FILE_RECORD_COUNT) {
                return BACNET_STATUS_ERROR;
            }
            for (i = 0; i < data->type.record.RecordCount; i++) {
                /* fileData */
                tag_len = bacnet_octet_string_application_decode(
                    &apdu[apdu_len], apdu_size - apdu_len, &data->fileData[i]);
                if (tag_len == BACNET_STATUS_ERROR) {
                    return BACNET_STATUS_ERROR;
                }
                apdu_len += tag_len;
                if (apdu_size <= apdu_len) {
                    return BACNET_STATUS_ERROR;
                }
            }
            if (!bacnet_is_closing_tag_number(
                    &apdu[apdu_len], apdu_size - apdu_len, 1, &tag_len)) {
                return BACNET_STATUS_ERROR;
            }
            apdu_len += tag_len;
        } else {
            return BACNET_STATUS_ERROR;
        }
    }

    return apdu_len;
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
