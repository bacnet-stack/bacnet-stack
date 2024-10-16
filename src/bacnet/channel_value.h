/**
 * @file
 * @brief BACnet single precision REAL encode and decode functions
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date 2012
 * @copyright SPDX-License-Identifier: MIT
 */
#ifndef BACNET_CHANNEL_VALUE_H
#define BACNET_CHANNEL_VALUE_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
/* BACnet Stack defines - first */
#include "bacnet/bacdef.h"
#include "bacnet/lighting.h"
#include "bacnet/lighting.h"

/* BACNET_CHANNEL_VALUE decodes WriteProperty service requests
   Choose the datatypes that your application supports */
#if !(                                                                  \
    defined(CHANNEL_NUMERIC) || defined(CHANNEL_NULL) ||                \
    defined(CHANNEL_BOOLEAN) || defined(CHANNEL_UNSIGNED) ||            \
    defined(CHANNEL_SIGNED) || defined(CHANNEL_REAL) ||                 \
    defined(CHANNEL_DOUBLE) || defined(CHANNEL_OCTET_STRING) ||         \
    defined(CHANNEL_CHARACTER_STRING) || defined(CHANNEL_BIT_STRING) || \
    defined(CHANNEL_ENUMERATED) || defined(CHANNEL_DATE) ||             \
    defined(CHANNEL_TIME) || defined(CHANNEL_OBJECT_ID) ||              \
    defined(CHANNEL_LIGHTING_COMMAND) || defined(CHANNEL_XY_COLOR) ||   \
    defined(CHANNEL_COLOR_COMMAND))
#define CHANNEL_NUMERIC
#endif

#if defined(CHANNEL_NUMERIC)
#define CHANNEL_NULL
#define CHANNEL_BOOLEAN
#define CHANNEL_UNSIGNED
#define CHANNEL_SIGNED
#define CHANNEL_REAL
#define CHANNEL_DOUBLE
#define CHANNEL_ENUMERATED
#define CHANNEL_LIGHTING_COMMAND
#define CHANNEL_COLOR_COMMAND
#define CHANNEL_XY_COLOR
#endif

typedef struct BACnet_Channel_Value_t {
    uint8_t tag;
    union {
        /* NULL - not needed as it is encoded in the tag alone */
#if defined(CHANNEL_BOOLEAN)
        bool Boolean;
#endif
#if defined(CHANNEL_UNSIGNED)
        BACNET_UNSIGNED_INTEGER Unsigned_Int;
#endif
#if defined(CHANNEL_SIGNED)
        int32_t Signed_Int;
#endif
#if defined(CHANNEL_REAL)
        float Real;
#endif
#if defined(CHANNEL_DOUBLE)
        double Double;
#endif
#if defined(CHANNEL_OCTET_STRING)
        BACNET_OCTET_STRING Octet_String;
#endif
#if defined(CHANNEL_CHARACTER_STRING)
        BACNET_CHARACTER_STRING Character_String;
#endif
#if defined(CHANNEL_BIT_STRING)
        BACNET_BIT_STRING Bit_String;
#endif
#if defined(CHANNEL_ENUMERATED)
        uint32_t Enumerated;
#endif
#if defined(CHANNEL_DATE)
        BACNET_DATE Date;
#endif
#if defined(CHANNEL_TIME)
        BACNET_TIME Time;
#endif
#if defined(CHANNEL_OBJECT_ID)
        BACNET_OBJECT_ID Object_Id;
#endif
#if defined(CHANNEL_LIGHTING_COMMAND)
        BACNET_LIGHTING_COMMAND Lighting_Command;
#endif
#if defined(CHANNEL_COLOR_COMMAND)
        BACNET_COLOR_COMMAND Color_Command;
#endif
#if defined(CHANNEL_XY_COLOR)
        BACNET_XY_COLOR XY_Color;
#endif
    } type;
    /* simple linked list if needed */
    struct BACnet_Channel_Value_t *next;
} BACNET_CHANNEL_VALUE;

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

BACNET_STACK_EXPORT
int bacnet_channel_value_type_encode(
    uint8_t *apdu, const BACNET_CHANNEL_VALUE *value);
BACNET_STACK_EXPORT
int bacnet_channel_value_type_decode(
    const uint8_t *apdu,
    size_t apdu_size,
    uint8_t tag_data_type,
    uint32_t len_value_type,
    BACNET_CHANNEL_VALUE *value);

BACNET_STACK_EXPORT
int bacnet_channel_value_decode(
    const uint8_t *apdu, size_t apdu_len, BACNET_CHANNEL_VALUE *value);
BACNET_STACK_EXPORT
int bacnet_channel_value_encode(
    uint8_t *apdu, size_t apdu_size, const BACNET_CHANNEL_VALUE *value);

BACNET_STACK_EXPORT
bool bacnet_channel_value_from_ascii(
    BACNET_CHANNEL_VALUE *value, const char *argv);
BACNET_STACK_EXPORT
bool bacnet_channel_value_copy(
    BACNET_CHANNEL_VALUE *dest, const BACNET_CHANNEL_VALUE *src);
BACNET_STACK_EXPORT
bool bacnet_channel_value_same(
    const BACNET_CHANNEL_VALUE *value1, const BACNET_CHANNEL_VALUE *value2);

BACNET_STACK_EXPORT
void bacnet_channel_value_link_array(BACNET_CHANNEL_VALUE *array, size_t size);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif
