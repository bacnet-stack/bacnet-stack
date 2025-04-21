/**
 * @file
 * @brief BACnetLogRecord data type encoding and decoding
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date 2025
 * @copyright SPDX-License-Identifier: MIT
 */
#ifndef BACNET_LOG_H
#define BACNET_LOG_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
/* BACnet Stack defines - first */
#include "bacnet/bacdef.h"
/* BACnet datatypes */
#include "bacnet/datetime.h"

/* BACnetLogRecord log-datum failure Error */
struct bacnet_log_datum_error {
    uint16_t error_class;
    uint16_t error_code;
};

/* BACnetLogRecord log-datum 32-bit bitstring (maps 24-bits) */
#define BACNET_LOG_DATUM_BITSTRING_BYTES_MAX 3
struct bacnet_log_datum_bitstring {
    uint8_t bits_used;
    uint8_t value[BACNET_LOG_DATUM_BITSTRING_BYTES_MAX];
};

typedef struct BACnetLogRecord {
    BACNET_DATE_TIME timestamp;
    /* only 4 lower bits used; set bit-7 to include this optional data */
    uint8_t status_flags;
    /* tag indicates which datum is used */
    uint8_t tag;
    union {
        uint8_t log_status;
        uint8_t boolean_value;
        float real_value;
        uint32_t enumerated_value;
        uint32_t unsigned_value;
        int32_t integer_value;
        struct bacnet_log_datum_bitstring bitstring_value;
        struct bacnet_log_datum_error failure;
        float time_change;
    } log_datum;
    struct BACnetLogRecord *next;
} BACNET_LOG_RECORD;

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

BACNET_STACK_EXPORT
int bacnet_log_record_value_encode(
    uint8_t *apdu, const BACNET_LOG_RECORD *value);
BACNET_STACK_EXPORT
int bacnet_log_record_decode(
    const uint8_t *apdu, size_t apdu_len, BACNET_LOG_RECORD *value);

BACNET_STACK_EXPORT
int bacnet_log_record_datum32_decode(
    const uint8_t *apdu,
    size_t apdu_size,
    uint8_t tag_data_type,
    uint32_t len_value_type,
    BACNET_LOG_RECORD *value);
BACNET_STACK_EXPORT
int bacnet_log_record_encode(
    uint8_t *apdu, size_t apdu_size, const BACNET_LOG_RECORD *value);

BACNET_STACK_EXPORT
bool bacnet_log_record_datum_from_ascii(
    BACNET_LOG_RECORD *value, const char *argv);
BACNET_STACK_EXPORT
bool bacnet_log_record_copy(
    BACNET_LOG_RECORD *dest, const BACNET_LOG_RECORD *src);
BACNET_STACK_EXPORT
bool bacnet_log_record_same(
    const BACNET_LOG_RECORD *value1, const BACNET_LOG_RECORD *value2);

BACNET_STACK_EXPORT
void bacnet_log_record_datum_bitstring_set(
    struct bacnet_log_datum_bitstring *bit_string,
    uint8_t bit_number,
    bool value);
BACNET_STACK_EXPORT
bool bacnet_log_record_datum_bitstring_same(
    const struct bacnet_log_datum_bitstring *value1,
    const struct bacnet_log_datum_bitstring *value2);

BACNET_STACK_EXPORT
bool bacnet_log_record_status_flags_bit(
    uint8_t status_flags, uint8_t bit_number);
BACNET_STACK_EXPORT
void bacnet_log_record_status_flags_bit_set(
    uint8_t *status_flags, uint8_t bit_number, bool value);

BACNET_STACK_EXPORT
void bacnet_log_record_link_array(BACNET_LOG_RECORD *array, size_t size);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif
