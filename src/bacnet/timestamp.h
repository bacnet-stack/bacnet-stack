/**
 * @file
 * @brief API for BACnetTimestamp encode and decode helper functions
 * @author John Minack <minack@users.sourceforge.net>
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date 2008
 * @copyright SPDX-License-Identifier: MIT
 */
#ifndef BACNET_TIMESTAMP_H_
#define BACNET_TIMESTAMP_H_
#include <stdint.h>
/* BACnet Stack defines - first */
#include "bacnet/bacdef.h"
/* BACnet Stack API */
#include "bacnet/bacdcode.h"

typedef enum {
    TIME_STAMP_TIME = 0,
    TIME_STAMP_SEQUENCE = 1,
    TIME_STAMP_DATETIME = 2
} BACNET_TIMESTAMP_TAG;

typedef uint8_t TYPE_BACNET_TIMESTAMP_TYPE;

typedef struct BACnet_Timestamp {
    TYPE_BACNET_TIMESTAMP_TYPE tag;
    union {
        BACNET_TIME time;
        uint16_t sequenceNum;
        BACNET_DATE_TIME dateTime;
    } value;
} BACNET_TIMESTAMP;

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

BACNET_STACK_EXPORT
void bacapp_timestamp_sequence_set(
    BACNET_TIMESTAMP *dest, uint16_t sequenceNum);

BACNET_STACK_EXPORT
void bacapp_timestamp_time_set(
    BACNET_TIMESTAMP *dest, const BACNET_TIME *btime);

BACNET_STACK_EXPORT
void bacapp_timestamp_datetime_set(
    BACNET_TIMESTAMP *dest, const BACNET_DATE_TIME *bdateTime);

BACNET_STACK_EXPORT
void bacapp_timestamp_copy(BACNET_TIMESTAMP *dest, const BACNET_TIMESTAMP *src);

BACNET_STACK_EXPORT
bool bacapp_timestamp_same(
    const BACNET_TIMESTAMP *value1, const BACNET_TIMESTAMP *value2);

BACNET_STACK_EXPORT
int bacapp_encode_timestamp(uint8_t *apdu, const BACNET_TIMESTAMP *value);
BACNET_STACK_EXPORT
int bacapp_encode_context_timestamp(
    uint8_t *apdu, uint8_t tag_number, const BACNET_TIMESTAMP *value);

BACNET_STACK_EXPORT
int bacnet_timestamp_decode(
    const uint8_t *apdu, uint32_t apdu_size, BACNET_TIMESTAMP *value);
BACNET_STACK_EXPORT
int bacnet_timestamp_context_decode(
    const uint8_t *apdu,
    uint32_t apdu_size,
    uint8_t tag_number,
    BACNET_TIMESTAMP *value);

BACNET_STACK_DEPRECATED("Use bacnet_timestamp_decode() instead")
BACNET_STACK_EXPORT
int bacapp_decode_timestamp(const uint8_t *apdu, BACNET_TIMESTAMP *value);
BACNET_STACK_DEPRECATED("Use bacnet_timestamp_context_decode() instead")
BACNET_STACK_EXPORT
int bacapp_decode_context_timestamp(
    const uint8_t *apdu, uint8_t tag_number, BACNET_TIMESTAMP *value);

BACNET_STACK_EXPORT
bool bacapp_timestamp_init_ascii(
    BACNET_TIMESTAMP *timestamp, const char *ascii);
BACNET_STACK_EXPORT
int bacapp_timestamp_to_ascii(
    char *str, size_t str_size, const BACNET_TIMESTAMP *timestamp);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif
