/**
 * @file
 * @brief BACnetLogRecord data type encoding and decoding
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date 2025
 * @copyright SPDX-License-Identifier: GPL-2.0-or-later WITH GCC-exception-2.0
 */
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <math.h>
#include <float.h>
/* BACnet Stack defines - first */
#include "bacnet/bacdef.h"
/* BACnet Stack API */
#include "bacnet/bacdcode.h"
#include "bacnet/bacstr.h"
#include "bacnet/bacint.h"
#include "bacnet/bacreal.h"
#include "bacnet/datetime.h"
/* me! */
#include "bacnet/baclog.h"

/**
 * @brief Encode a given BACnetLogRecord value
 * @param  apdu - APDU buffer for storing the encoded data, or NULL for length
 * @param  value - BACNET_LOG_RECORD value
 * @return  number of bytes in the APDU, or 0 on error
 */
int bacnet_log_record_value_encode(
    uint8_t *apdu, const BACNET_LOG_RECORD *value)
{
    int apdu_len = 0, len;
    BACNET_BIT_STRING bitstring;
    uint8_t i = 0;

    if (!value) {
        return 0;
    }
    /* time stamp [0] */
    len = bacapp_encode_context_datetime(apdu, 0, &value->timestamp);
    apdu_len += len;
    if (apdu) {
        apdu += len;
    }
    /* log-datum [1] */
    len = encode_opening_tag(apdu, 1);
    apdu_len += len;
    if (apdu) {
        apdu += len;
    }
    switch (value->tag) {
        case BACNET_LOG_DATUM_STATUS:
            /* log-status [0] BACnetLogStatus */
            bitstring_init(&bitstring);
            bitstring_set_bits_used(&bitstring, 1, 4);
            bitstring_set_octet(&bitstring, 0, value->log_datum.log_status);
            len = encode_context_bitstring(apdu, value->tag, &bitstring);
            break;
        case BACNET_LOG_DATUM_BOOLEAN:
            /* boolean-value [1] BOOLEAN */
            len = encode_context_boolean(
                apdu, value->tag, value->log_datum.boolean_value);
            break;
        case BACNET_LOG_DATUM_REAL:
            /* real-value [2] REAL */
            len = encode_context_real(
                apdu, value->tag, value->log_datum.real_value);
            break;
        case BACNET_LOG_DATUM_ENUMERATED:
            /* enumerated-value [3] ENUMERATED,
             * -- Optionally limited to 32 bits */
            len = encode_context_enumerated(
                apdu, value->tag, value->log_datum.enumerated_value);
            break;
        case BACNET_LOG_DATUM_UNSIGNED:
            /* unsigned-value [4] Unsigned,
             * -- Optionally limited to 32 bits*/
            len = encode_context_unsigned(
                apdu, value->tag, value->log_datum.unsigned_value);
            break;
        case BACNET_LOG_DATUM_SIGNED:
            /* integer-value [5] INTEGER,
             * -- Optionally limited to 32 bits */
            len = encode_context_signed(
                apdu, value->tag, value->log_datum.integer_value);
            break;
        case BACNET_LOG_DATUM_BITSTRING:
            /* bitstring-value [6] BIT STRING,
             * -- Optionally limited to 32 bits */
            bitstring_init(&bitstring);
            bitstring_bits_used_set(
                &bitstring, value->log_datum.bitstring_value.bits_used);
            for (i = 0; i < BACNET_LOG_DATUM_BITSTRING_BYTES_MAX; i++) {
                bitstring_set_octet(
                    &bitstring, i, value->log_datum.bitstring_value.value[i]);
            }
            len = encode_context_bitstring(apdu, value->tag, &bitstring);
            break;
        case BACNET_LOG_DATUM_NULL:
            /* null-value [7] NULL */
            len = encode_context_null(apdu, value->tag);
            break;
        case BACNET_LOG_DATUM_FAILURE:
            /* failure [8] Error */
            len = encode_opening_tag(apdu, value->tag);
            apdu_len += len;
            if (apdu) {
                apdu += len;
            }
            len = encode_application_enumerated(
                apdu, value->log_datum.failure.error_class);
            apdu_len += len;
            if (apdu) {
                apdu += len;
            }
            len = encode_application_enumerated(
                apdu, value->log_datum.failure.error_code);
            apdu_len += len;
            if (apdu) {
                apdu += len;
            }
            len = encode_closing_tag(apdu, value->tag);
            break;

        case BACNET_LOG_DATUM_TIME_CHANGE:
            /* time-change [9] REAL */
            len = encode_context_real(
                apdu, value->tag, value->log_datum.time_change);
            break;

        case BACNET_LOG_DATUM_ANY:
            /* we don't support this option */
            break;

        default:
            break;
    }
    apdu_len += len;
    if (apdu) {
        apdu += len;
    }
    /* log-datum [1] */
    len = encode_closing_tag(apdu, 1);
    apdu_len += len;
    if (apdu) {
        apdu += len;
    }
    /* status-flags [2] BACnetStatusFlags OPTIONAL */
    if (bacnet_log_record_status_flags_bit(value->status_flags, 7)) {
        bitstring_init(&bitstring);
        bitstring_set_bit(
            &bitstring, STATUS_FLAG_IN_ALARM,
            bacnet_log_record_status_flags_bit(
                value->status_flags, STATUS_FLAG_IN_ALARM));
        bitstring_set_bit(
            &bitstring, STATUS_FLAG_FAULT,
            bacnet_log_record_status_flags_bit(
                value->status_flags, STATUS_FLAG_FAULT));
        bitstring_set_bit(
            &bitstring, STATUS_FLAG_OVERRIDDEN,
            bacnet_log_record_status_flags_bit(
                value->status_flags, STATUS_FLAG_OVERRIDDEN));
        bitstring_set_bit(
            &bitstring, STATUS_FLAG_OUT_OF_SERVICE,
            bacnet_log_record_status_flags_bit(
                value->status_flags, STATUS_FLAG_OUT_OF_SERVICE));
        len = encode_context_bitstring(apdu, 2, &bitstring);
        apdu_len += len;
    }

    return apdu_len;
}

/**
 * @brief Encode a given channel value
 * @param  apdu - APDU buffer for storing the encoded data, or NULL for length
 * @param apdu_size - size of the APDU buffer
 * @param  value - BACNET_LOG_RECORD value
 * @return returns the number of apdu bytes consumed,
 *  or 0 if apdu_size is too small to fit the data
 */
int bacnet_log_record_encode(
    uint8_t *apdu, size_t apdu_size, const BACNET_LOG_RECORD *value)
{
    size_t apdu_len = 0; /* total length of the apdu, return value */

    apdu_len = bacnet_log_record_value_encode(NULL, value);
    if (apdu_len > apdu_size) {
        apdu_len = 0;
    } else {
        apdu_len = bacnet_log_record_value_encode(apdu, value);
    }

    return apdu_len;
}

/**
 * @brief Decode a BACnetLogRecord log-datum value
 * @param  apdu - APDU buffer for decoding
 * @param apdu_size - Count of valid bytes in the buffer
 * @param  tag_data_type - BACNET_LOG_DATUM tag
 * @param  len_value_type - length or value of the tag
 * @param  value - BACNET_LOG_RECORD value to store the decoded data
 * @return  number of bytes decoded (0 or more) or
 *  BACNET_STATUS_ERROR on error
 */
int bacnet_log_record_datum32_decode(
    const uint8_t *apdu,
    size_t apdu_size,
    uint8_t tag_data_type,
    uint32_t len_value_type,
    BACNET_LOG_RECORD *value)
{
    int len = 0;
    uint32_t enum_value = 0;
    float real_value = 0.0;
    BACNET_UNSIGNED_INTEGER unsigned_value = 0;
    int32_t signed_value = 0;
    BACNET_BIT_STRING bit_string = { 0 };
    bool boolean_value = false;
    unsigned i;

    if (!apdu) {
        return BACNET_STATUS_ERROR;
    }
    if (value) {
        value->tag = tag_data_type;
    }
    switch (tag_data_type) {
        case BACNET_LOG_DATUM_STATUS:
            /* log-status [0] BACnetLogStatus */
            len = bacnet_enumerated_decode(
                apdu, apdu_size, len_value_type, &enum_value);
            if (len > 0) {
                if (enum_value > UINT8_MAX) {
                    return BACNET_STATUS_ERROR;
                }
                if (value) {
                    value->log_datum.log_status = (uint8_t)enum_value;
                }
            } else {
                return BACNET_STATUS_ERROR;
            }
            break;
        case BACNET_LOG_DATUM_BOOLEAN:
            /* boolean-value [1] BOOLEAN */
            len = bacnet_boolean_context_value_decode(
                apdu, apdu_size, &boolean_value);
            if (value) {
                value->log_datum.boolean_value = boolean_value;
            }
            break;
        case BACNET_LOG_DATUM_REAL:
            /* real-value [2] REAL */
            len = bacnet_real_decode(
                apdu, apdu_size, len_value_type, &real_value);
            if (len > 0) {
                if (value) {
                    value->log_datum.real_value = real_value;
                }
            } else {
                return BACNET_STATUS_ERROR;
            }
            break;
        case BACNET_LOG_DATUM_ENUMERATED:
            /* enumerated-value [3] ENUMERATED,
             * -- Optionally limited to 32 bits */
            len = bacnet_enumerated_decode(
                apdu, apdu_size, len_value_type, &enum_value);
            if (len > 0) {
                if (value) {
                    value->log_datum.enumerated_value = enum_value;
                }
            } else {
                return BACNET_STATUS_ERROR;
            }
            break;
        case BACNET_LOG_DATUM_UNSIGNED:
            /* unsigned-value [4] Unsigned,
             * -- Optionally limited to 32 bits*/
            len = bacnet_unsigned_decode(
                apdu, apdu_size, len_value_type, &unsigned_value);
            if (len > 0) {
                if (value) {
                    value->log_datum.unsigned_value = unsigned_value;
                }
            } else {
                return BACNET_STATUS_ERROR;
            }
            break;
        case BACNET_LOG_DATUM_SIGNED:
            /* integer-value [5] INTEGER,
             * -- Optionally limited to 32 bits */
            len = bacnet_signed_decode(
                apdu, apdu_size, len_value_type, &signed_value);
            if (len > 0) {
                if (value) {
                    value->log_datum.integer_value = signed_value;
                }
            } else {
                return BACNET_STATUS_ERROR;
            }
            break;
        case BACNET_LOG_DATUM_BITSTRING:
            /* bitstring-value [6] BIT STRING,
             * -- Optionally limited to 32 bits */
            len = bacnet_bitstring_decode(
                apdu, apdu_size, len_value_type, &bit_string);
            if (len > 0) {
                if (bit_string.bits_used > 24) {
                    return BACNET_STATUS_ERROR;
                }
                if (value) {
                    value->log_datum.bitstring_value.bits_used =
                        bit_string.bits_used;
                    for (i = 0; i < BACNET_LOG_DATUM_BITSTRING_BYTES_MAX; i++) {
                        value->log_datum.bitstring_value.value[i] =
                            bit_string.value[i];
                    }
                }
            } else {
                return BACNET_STATUS_ERROR;
            }
            break;
        case BACNET_LOG_DATUM_NULL:
            /* null-value [7] NULL - nothing to do, value is tag */
            break;
        case BACNET_LOG_DATUM_FAILURE:
            /* open/close tagged values are not processed here */
            break;
        case BACNET_LOG_DATUM_TIME_CHANGE:
            /* time-change [9] REAL */
            len = bacnet_real_decode(
                apdu, apdu_size, len_value_type, &real_value);
            if (len > 0) {
                if (value) {
                    value->log_datum.time_change = real_value;
                }
            } else {
                return BACNET_STATUS_ERROR;
            }
            break;
        case BACNET_LOG_DATUM_ANY:
            /* we don't support this option */
            break;

        default:
            break;
    }

    return len;
}

/**
 * @brief Decode a BACnetLogRecord log-datum failure value
 * @param  apdu - APDU buffer for decoding
 * @param apdu_size - Count of valid bytes in the buffer
 * @param  value - BACNET_LOG_RECORD value to store the decoded data
 * @return  number of bytes decoded (0 or more) or
 *  BACNET_STATUS_ERROR on error
 */
int bacnet_log_record_datum_failure_decode(
    const uint8_t *apdu, size_t apdu_size, BACNET_LOG_RECORD *value)
{
    int len = 0;
    int apdu_len = 0;
    uint32_t enum_value = 0;

    if (!apdu) {
        return BACNET_STATUS_ERROR;
    }
    /* failure [8] Error */
    len = bacnet_enumerated_application_decode(
        apdu, apdu_size - apdu_len, &enum_value);
    if (len > 0) {
        apdu_len += len;
        if (enum_value > UINT16_MAX) {
            return BACNET_STATUS_ERROR;
        }
        if (value) {
            value->log_datum.failure.error_class = (uint16_t)enum_value;
        }
    } else {
        return BACNET_STATUS_ERROR;
    }
    len = bacnet_enumerated_application_decode(
        &apdu[len], apdu_size - apdu_len, &enum_value);
    if (len > 0) {
        apdu_len += len;
        if (enum_value > UINT16_MAX) {
            return BACNET_STATUS_ERROR;
        }
        if (value) {
            value->log_datum.failure.error_code = (uint16_t)enum_value;
        }
    } else {
        return BACNET_STATUS_ERROR;
    }
    if (value) {
        value->tag = BACNET_LOG_DATUM_FAILURE;
    }

    return apdu_len;
}

/**
 * Set bits in the BACnetLogRecord datum bitstring (limited to 32-bits).
 *
 * @param bit_string  Pointer to the bit string structure.
 * @param bit_number  Number of the bit 0..23
 * @param value       Value true or false
 */
void bacnet_log_record_datum_bitstring_set(
    struct bacnet_log_datum_bitstring *bit_string,
    uint8_t bit_number,
    bool value)
{
    unsigned byte_number = bit_number / 8;
    uint8_t bit_mask = 1;

    if (bit_string) {
        if (byte_number < BACNET_LOG_DATUM_BITSTRING_BYTES_MAX) {
            /* set max bits used */
            if (bit_string->bits_used < (bit_number + 1)) {
                bit_string->bits_used = bit_number + 1;
            }
            bit_mask = bit_mask << (bit_number - (byte_number * 8));
            if (value) {
                bit_string->value[byte_number] |= bit_mask;
            } else {
                bit_string->value[byte_number] &= (~(bit_mask));
            }
        }
    }
}

/**
 * @brief Compare two BACnetLogRecord bitstring values
 * @param value1 [in] The first BACnetLogRecord bitstring value
 * @param value2 [in] The second BACnetLogRecord bitstring value
 * @return true if the values are the same
 */
bool bacnet_log_record_datum_bitstring_same(
    const struct bacnet_log_datum_bitstring *value1,
    const struct bacnet_log_datum_bitstring *value2)
{
    unsigned i;

    if (value1->bits_used != value2->bits_used) {
        return false;
    }
    for (i = 0; i < BACNET_LOG_DATUM_BITSTRING_BYTES_MAX; i++) {
        if (value1->value[i] != value2->value[i]) {
            return false;
        }
    }
    return true;
}

/**
 * @brief Set bits in the BACnetLogRecord status_flags bitstring
 * @note must be encoded/decoded in same bit order as a BACnetBitString
 * @param status_flags pointer to status flags
 * @param bit_number  Number of the bit 0..7
 * @param value       Value true or false
 */
void bacnet_log_record_status_flags_bit_set(
    uint8_t *status_flags, uint8_t bit_number, bool value)
{
    uint8_t bit_mask = 1;

    if (bit_number > 7) {
        return;
    }
    bit_mask = bit_mask << bit_number;
    if (value) {
        *status_flags |= bit_mask;
    } else {
        *status_flags &= (~(bit_mask));
    }
}

/**
 * @brief Return the value of a single bit out of the status_flags bit string.
 * @note must be encoded/decoded in same bit order as a BACnetBitString
 * @param bit_string  Pointer to the bit string structure.
 * @param bit_number  Number of the bit 0..7
 * @return Value 0/1
 */
bool bacnet_log_record_status_flags_bit(
    uint8_t status_flags, uint8_t bit_number)
{
    bool value = false;
    uint8_t bit_mask = 1;

    if (bit_number > 7) {
        return false;
    }
    bit_mask = bit_mask << bit_number;
    if (status_flags & bit_mask) {
        value = true;
    }

    return value;
}

/**
 * @brief Decode a given channel value
 * @param  apdu - APDU buffer for decoding
 * @param  apdu_size - Count of valid bytes in the buffer
 * @param  value - BACNET_LOG_RECORD value to store the decoded data
 * @return  number of bytes decoded or BACNET_STATUS_ERROR on error
 */
int bacnet_log_record_decode(
    const uint8_t *apdu, size_t apdu_size, BACNET_LOG_RECORD *value)
{
    int len = 0;
    int apdu_len = 0;
    BACNET_TAG tag = { 0 };
    BACNET_DATE_TIME timestamp = { 0 };
    BACNET_BIT_STRING status_flags = { 0 };

    if (!apdu) {
        return BACNET_STATUS_ERROR;
    }
    /* time stamp [0] */
    len = bacnet_datetime_context_decode(
        &apdu[apdu_len], apdu_size - apdu_len, 0, &timestamp);
    if (len > 0) {
        apdu_len += len;
        if (value) {
            datetime_copy(&value->timestamp, &timestamp);
        }
    } else {
        return BACNET_STATUS_ERROR;
    }
    /* log-datum [1] - opening */
    if (bacnet_is_opening_tag_number(
            &apdu[apdu_len], apdu_size - apdu_len, 1, &len)) {
        apdu_len += len;
    } else {
        return BACNET_STATUS_ERROR;
    }
    len = bacnet_tag_decode(&apdu[apdu_len], apdu_size - apdu_len, &tag);
    if (len > 0) {
        apdu_len += len;
        if (tag.opening) {
            if (tag.number == BACNET_LOG_DATUM_FAILURE) {
                len = bacnet_log_record_datum_failure_decode(
                    &apdu[apdu_len], apdu_size - apdu_len, value);
                if (len > 0) {
                    apdu_len += len;
                    if (bacnet_is_closing_tag_number(
                            &apdu[apdu_len], apdu_size - apdu_len, tag.number,
                            &len)) {
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
        } else if (tag.context) {
            len = bacnet_log_record_datum32_decode(
                &apdu[apdu_len], apdu_size - apdu_len, tag.number,
                tag.len_value_type, value);
            if (len >= 0) {
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
    /* log-datum [1] - closing */
    if (bacnet_is_closing_tag_number(
            &apdu[apdu_len], apdu_size - apdu_len, 1, &len)) {
        apdu_len += len;
    } else {
        return BACNET_STATUS_ERROR;
    }
    /* status-flags [2] BACnetStatusFlags OPTIONAL */
    len = bacnet_bitstring_context_decode(
        &apdu[apdu_len], apdu_size - apdu_len, 2, &status_flags);
    if (len > 0) {
        apdu_len += len;
        if (status_flags.bits_used > 4) {
            return BACNET_STATUS_ERROR;
        }
        if (value) {
            value->status_flags = status_flags.value[0];
            bacnet_log_record_status_flags_bit_set(
                &value->status_flags, 7, true);
        }
    } else if (len == 0) {
        if (value) {
            /* no status flags */
            value->status_flags = 0;
        }
    } else if (len < 0) {
        return BACNET_STATUS_ERROR;
    }

    return apdu_len;
}

/**
 * @brief Compare two BACnetLogRecord values
 * @param value1 [in] The first BACnetLogRecord value
 * @param value2 [in] The second BACnetLogRecord value
 * @return true if the values are the same
 */
bool bacnet_log_record_same(
    const BACNET_LOG_RECORD *value1, const BACNET_LOG_RECORD *value2)
{
    int diff;

    if (!value1 || !value2) {
        return false;
    }
    if (value1->tag != value2->tag) {
        return false;
    }
    diff = datetime_compare(&value1->timestamp, &value2->timestamp);
    if (diff != 0) {
        return false;
    }
    if (BIT_CHECK(value1->status_flags, 7)) {
        /* optional status flags */
        if (BITMASK_CHECK(value1->status_flags, 0x0F) !=
            BITMASK_CHECK(value2->status_flags, 0x0F)) {
            return false;
        }
    }
    switch (value1->tag) {
        case BACNET_LOG_DATUM_NULL:
            return true;
        case BACNET_LOG_DATUM_BOOLEAN:
            return value1->log_datum.boolean_value ==
                value2->log_datum.boolean_value;
        case BACNET_LOG_DATUM_UNSIGNED:
            return value1->log_datum.unsigned_value ==
                value2->log_datum.unsigned_value;
        case BACNET_LOG_DATUM_SIGNED:
            return value1->log_datum.integer_value ==
                value2->log_datum.integer_value;
        case BACNET_LOG_DATUM_REAL:
            return !islessgreater(
                value1->log_datum.real_value, value2->log_datum.real_value);
        case BACNET_LOG_DATUM_BITSTRING:
            return bacnet_log_record_datum_bitstring_same(
                &value1->log_datum.bitstring_value,
                &value2->log_datum.bitstring_value);
        case BACNET_LOG_DATUM_ENUMERATED:
            return value1->log_datum.enumerated_value ==
                value2->log_datum.enumerated_value;
        case BACNET_LOG_DATUM_STATUS:
            return value1->log_datum.log_status == value2->log_datum.log_status;
        case BACNET_LOG_DATUM_TIME_CHANGE:
            return !islessgreater(
                value1->log_datum.time_change, value2->log_datum.time_change);
        default:
            break;
    }

    return false;
}

/**
 * @brief Copy a BACnetLogDatum bitstring value
 * @param dest [out] The destination BACnetLogDatum bitstring value
 * @param src [in] The source BACnetLogDatum bitstring value
 * @return true if the value was copied, else false
 */
static bool log_datum_bitstring_copy(
    struct bacnet_log_datum_bitstring *dest,
    const struct bacnet_log_datum_bitstring *src)
{
    unsigned i;

    if (!dest || !src) {
        return false;
    }
    dest->bits_used = src->bits_used;
    for (i = 0; i < BACNET_LOG_DATUM_BITSTRING_BYTES_MAX; i++) {
        dest->value[i] = src->value[i];
    }
    return true;
}

/**
 * @brief Copy a BACnetLogRecord to another
 * @param value1 [in] The first BACnetLogRecord value
 * @param value2 [in] The second BACnetLogRecord value
 * @return true if the value was copied, else false
 */
bool bacnet_log_record_copy(
    BACNET_LOG_RECORD *dest, const BACNET_LOG_RECORD *src)
{
    if (!dest || !src) {
        return false;
    }
    dest->tag = src->tag;
    dest->status_flags = src->status_flags;
    datetime_copy(&dest->timestamp, &src->timestamp);
    switch (src->tag) {
        case BACNET_LOG_DATUM_NULL:
            return true;
        case BACNET_LOG_DATUM_BOOLEAN:
            dest->log_datum.boolean_value = src->log_datum.boolean_value;
            return true;
        case BACNET_LOG_DATUM_UNSIGNED:
            dest->log_datum.unsigned_value = src->log_datum.unsigned_value;
            return true;
        case BACNET_LOG_DATUM_SIGNED:
            dest->log_datum.integer_value = src->log_datum.integer_value;
            return true;
        case BACNET_LOG_DATUM_REAL:
            dest->log_datum.real_value = src->log_datum.real_value;
            return true;
        case BACNET_LOG_DATUM_BITSTRING:
            return log_datum_bitstring_copy(
                &dest->log_datum.bitstring_value,
                &src->log_datum.bitstring_value);
        case BACNET_LOG_DATUM_ENUMERATED:
            dest->log_datum.enumerated_value = src->log_datum.enumerated_value;
            return true;
        case BACNET_LOG_DATUM_STATUS:
            dest->log_datum.log_status = src->log_datum.log_status;
            return true;
        case BACNET_LOG_DATUM_TIME_CHANGE:
            dest->log_datum.time_change = src->log_datum.time_change;
            return true;
        default:
            break;
    }

    return false;
}

/**
 * @brief Parse a string into a BACnetLogRecord structure
 * @param value [out] The BACnetLogRecord value
 * @param argv [in] The string to parse
 * @return true on success, else false
 */
bool bacnet_log_record_datum_from_ascii(
    BACNET_LOG_RECORD *value, const char *argv)
{
    bool status = false;
    int count;
    unsigned long unsigned_value;
    long signed_value;
    float single_value;
    double double_value;
    const char *negative;
    const char *decimal_point;
    const char *real_string;

    if (!value || !argv) {
        return false;
    }
    if (!status) {
        if (bacnet_stricmp(argv, "null") == 0) {
            value->tag = BACNET_LOG_DATUM_NULL;
            status = true;
        }
    }
    if (!status) {
        if (bacnet_stricmp(argv, "true") == 0) {
            value->tag = BACNET_LOG_DATUM_BOOLEAN;
            value->log_datum.boolean_value = true;
            status = true;
        }
    }
    if (!status) {
        if (bacnet_stricmp(argv, "false") == 0) {
            value->tag = BACNET_LOG_DATUM_BOOLEAN;
            value->log_datum.boolean_value = false;
            status = true;
        }
    }
    if (!status) {
        /* time_change */
        real_string = strchr(argv, 'T');
        if (!real_string) {
            real_string = strchr(argv, 't');
        }
        if (real_string) {
            value->tag = BACNET_LOG_DATUM_TIME_CHANGE;
            count = sscanf(argv + 1, "%f", &single_value);
            if (count == 1) {
                value->log_datum.real_value = single_value;
                status = true;
            }
        }
    }
    if (!status) {
        decimal_point = strchr(argv, '.');
        if (decimal_point) {
            count = sscanf(argv, "%lf", &double_value);
            if (count == 1) {
                if (isgreaterequal(double_value, -FLT_MAX) &&
                    islessequal(double_value, FLT_MAX)) {
                    value->tag = BACNET_LOG_DATUM_REAL;
                    value->log_datum.real_value = (float)double_value;
                    status = true;
                }
            }
        }
    }
    if (!status) {
        negative = strchr(argv, '-');
        if (negative) {
            count = sscanf(argv, "%ld", &signed_value);
            if (count == 1) {
                value->tag = BACNET_LOG_DATUM_SIGNED;
                value->log_datum.integer_value = signed_value;
                status = true;
            }
        }
    }
    if (!status) {
        count = sscanf(argv, "%lu", &unsigned_value);
        if (count == 1) {
            value->tag = BACNET_LOG_DATUM_UNSIGNED;
            value->log_datum.unsigned_value = unsigned_value;
            status = true;
        }
    }

    return status;
}

/**
 * @brief Convert an array of BACnetLogRecord to linked list
 * @param array pointer to element zero of the array
 * @param size number of elements in the array
 */
void bacnet_log_record_link_array(BACNET_LOG_RECORD *array, size_t size)
{
    size_t i = 0;

    for (i = 0; i < size; i++) {
        if (i > 0) {
            array[i - 1].next = &array[i];
        }
        array[i].next = NULL;
    }
}
