/**
 * @file
 * @brief BACnetTimerStateChangeValue data type encoding and decoding API
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date October 2025
 * @copyright SPDX-License-Identifier: MIT
 */
#ifndef BACNET_TIMER_VALUE_H
#define BACNET_TIMER_VALUE_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
/* BACnet Stack defines - first */
#include "bacnet/bacdef.h"
#include "bacnet/lighting.h"

/* BACNET_TIMER_STATE_CHANGE_VALUE decodes WriteProperty service requests
   Choose the datatypes that your application supports */
#if !(                                                                      \
    defined(BACNET_TIMER_NUMERIC) || defined(BACNET_TIMER_NULL) ||          \
    defined(BACNET_TIMER_BOOLEAN) || defined(BACNET_TIMER_UNSIGNED) ||      \
    defined(BACNET_TIMER_SIGNED) || defined(BACNET_TIMER_REAL) ||           \
    defined(BACNET_TIMER_DOUBLE) || defined(BACNET_TIMER_OCTET_STRING) ||   \
    defined(BACNET_TIMER_CHARACTER_STRING) ||                               \
    defined(BACNET_TIMER_BIT_STRING) || defined(BACNET_TIMER_ENUMERATED) || \
    defined(BACNET_TIMER_DATE) || defined(BACNET_TIMER_TIME) ||             \
    defined(BACNET_TIMER_OBJECT_ID) || defined(BACNET_TIMER_NO_VALUE) ||    \
    defined(BACNET_TIMER_DATETIME) ||                                       \
    defined(BACNET_TIMER_LIGHTING_COMMAND) ||                               \
    defined(BACNET_TIMER_XY_COLOR) || defined(BACNET_TIMER_COLOR_COMMAND))
#define BACNET_TIMER_NUMERIC
#endif

#if defined(BACNET_TIMER_NUMERIC)
#define BACNET_TIMER_NULL
#define BACNET_TIMER_BOOLEAN
#define BACNET_TIMER_UNSIGNED
#define BACNET_TIMER_SIGNED
#define BACNET_TIMER_REAL
#define BACNET_TIMER_DOUBLE
#define BACNET_TIMER_ENUMERATED
#define BACNET_TIMER_NO_VALUE
#define BACNET_TIMER_LIGHTING_COMMAND
#define BACNET_TIMER_COLOR_COMMAND
#define BACNET_TIMER_XY_COLOR
#endif

typedef struct BACnet_Timer_State_Change_Value_T {
    uint8_t tag;
    union {
        /* null - no type value needed. It is encoded in the tag alone. */
        /* no-value - no type value needed. It is encoded in the tag alone. */
#if defined(BACNET_TIMER_BOOLEAN)
        bool Boolean;
#endif
#if defined(BACNET_TIMER_UNSIGNED)
        BACNET_UNSIGNED_INTEGER Unsigned_Int;
#endif
#if defined(BACNET_TIMER_SIGNED)
        int32_t Signed_Int;
#endif
#if defined(BACNET_TIMER_REAL)
        float Real;
#endif
#if defined(BACNET_TIMER_DOUBLE)
        double Double;
#endif
#if defined(BACNET_TIMER_OCTET_STRING)
        BACNET_OCTET_STRING Octet_String;
#endif
#if defined(BACNET_TIMER_CHARACTER_STRING)
        BACNET_CHARACTER_STRING Character_String;
#endif
#if defined(BACNET_TIMER_BIT_STRING)
        BACNET_BIT_STRING Bit_String;
#endif
#if defined(BACNET_TIMER_ENUMERATED)
        uint32_t Enumerated;
#endif
#if defined(BACNET_TIMER_DATE)
        BACNET_DATE Date;
#endif
#if defined(BACNET_TIMER_TIME)
        BACNET_TIME Time;
#endif
#if defined(BACNET_TIMER_OBJECT_ID)
        BACNET_OBJECT_ID Object_Id;
#endif
#if defined(BACNET_TIMER_DATETIME)
        BACNET_DATE_TIME Date_Time;
#endif
#if defined(BACNET_TIMER_LIGHTING_COMMAND)
        BACNET_LIGHTING_COMMAND Lighting_Command;
#endif
#if defined(BACNET_TIMER_COLOR_COMMAND)
        BACNET_COLOR_COMMAND Color_Command;
#endif
#if defined(BACNET_TIMER_XY_COLOR)
        BACNET_XY_COLOR XY_Color;
#endif
    } type;
    /* simple linked list if needed */
    struct BACnet_Timer_State_Change_Value_T *next;
} BACNET_TIMER_STATE_CHANGE_VALUE;

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

BACNET_STACK_EXPORT
int bacnet_timer_state_change_value_encode(
    uint8_t *apdu, const BACNET_TIMER_STATE_CHANGE_VALUE *value);
BACNET_STACK_EXPORT
int bacnet_timer_state_change_value_decode(
    const uint8_t *apdu,
    size_t apdu_size,
    uint8_t tag_data_type,
    uint32_t len_value_type,
    BACNET_TIMER_STATE_CHANGE_VALUE *value);

BACNET_STACK_EXPORT
int bacnet_timer_value_decode(
    const uint8_t *apdu,
    size_t apdu_len,
    BACNET_TIMER_STATE_CHANGE_VALUE *value);
BACNET_STACK_EXPORT
int bacnet_timer_value_encode(
    uint8_t *apdu,
    size_t apdu_size,
    const BACNET_TIMER_STATE_CHANGE_VALUE *value);

BACNET_STACK_EXPORT
bool bacnet_timer_value_from_ascii(
    BACNET_TIMER_STATE_CHANGE_VALUE *value, const char *argv);
BACNET_STACK_EXPORT
int bacnet_timer_value_to_ascii(
    const BACNET_TIMER_STATE_CHANGE_VALUE *value, char *str, size_t str_len);

BACNET_STACK_EXPORT
bool bacnet_timer_value_copy(
    BACNET_TIMER_STATE_CHANGE_VALUE *dest,
    const BACNET_TIMER_STATE_CHANGE_VALUE *src);
BACNET_STACK_EXPORT
bool bacnet_timer_value_same(
    const BACNET_TIMER_STATE_CHANGE_VALUE *value1,
    const BACNET_TIMER_STATE_CHANGE_VALUE *value2);

BACNET_STACK_EXPORT
void bacnet_timer_value_link_array(
    BACNET_TIMER_STATE_CHANGE_VALUE *array, size_t size);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif
