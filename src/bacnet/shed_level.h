/**
 * @file
 * @brief API for BACnetShedLevel complex data type encode and decode
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date December 2025
 * @copyright SPDX-License-Identifier: MIT
 */
#ifndef BACNET_SHED_LEVEL_H
#define BACNET_SHED_LEVEL_H

#include <stdint.h>
#include <stdbool.h>
/* BACnet Stack defines - first */
#include "bacnet/bacdef.h"
#include "bacnet/bacint.h"

/* The shed levels for the LEVEL choice of BACnetShedLevel. */
typedef struct BACnetShedLevel {
    BACNET_SHED_LEVEL_TYPE type;
    union {
        BACNET_UNSIGNED_INTEGER level;
        BACNET_UNSIGNED_INTEGER percent;
        float amount;
    } value;
} BACNET_SHED_LEVEL;

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/** Decode Special Event */
BACNET_STACK_EXPORT
int bacnet_shed_level_decode(
    const uint8_t *apdu, size_t apdu_size, BACNET_SHED_LEVEL *value);

/** Encode Special Event */
BACNET_STACK_EXPORT
int bacnet_shed_level_encode(uint8_t *apdu, const BACNET_SHED_LEVEL *value);

BACNET_STACK_EXPORT
bool bacnet_shed_level_same(
    const BACNET_SHED_LEVEL *value1, const BACNET_SHED_LEVEL *value2);

BACNET_STACK_EXPORT
bool bacnet_shed_level_copy(
    BACNET_SHED_LEVEL *dest, const BACNET_SHED_LEVEL *src);

BACNET_STACK_EXPORT
int bacapp_snprintf_shed_level(
    char *str, size_t str_len, const BACNET_SHED_LEVEL *value);

BACNET_STACK_EXPORT
bool bacnet_shed_level_from_ascii(BACNET_SHED_LEVEL *value, const char *argv);

BACNET_STACK_EXPORT
bool bacnet_shed_level_init(
    BACNET_SHED_LEVEL *value, BACNET_SHED_LEVEL_TYPE type, float level);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif
