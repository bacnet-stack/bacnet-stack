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
#include "bacnet/bacstr.h"
#include "bacnet/bacdcode.h"
#include "bacnet/bacint.h"
#include "bacnet/datetime.h"
#include "bacnet/lighting.h"

/* BACNET_TIMER_STATE_CHANGE_VALUE decodes WriteProperty service requests
   Define the datatypes that your application supports */
#if !(/* check for any defines, and if none, use NUMERIC */                   \
      defined(BACNET_TIMER_VALUE_NUMERIC) ||                                  \
      defined(BACNET_TIMER_VALUE_ALL) || defined(BACNET_TIMER_VALUE_NULL) ||  \
      defined(BACNET_TIMER_VALUE_BOOLEAN) ||                                  \
      defined(BACNET_TIMER_VALUE_UNSIGNED) ||                                 \
      defined(BACNET_TIMER_VALUE_SIGNED) ||                                   \
      defined(BACNET_TIMER_VALUE_REAL) ||                                     \
      defined(BACNET_TIMER_VALUE_DOUBLE) ||                                   \
      defined(BACNET_TIMER_VALUE_OCTET_STRING) ||                             \
      defined(BACNET_TIMER_VALUE_CHARACTER_STRING) ||                         \
      defined(BACNET_TIMER_VALUE_BIT_STRING) ||                               \
      defined(BACNET_TIMER_VALUE_ENUMERATED) ||                               \
      defined(BACNET_TIMER_VALUE_DATE) || defined(BACNET_TIMER_VALUE_TIME) || \
      defined(BACNET_TIMER_VALUE_OBJECT_ID) ||                                \
      defined(BACNET_TIMER_VALUE_NO_VALUE) ||                                 \
      defined(BACNET_TIMER_VALUE_CONSTRUCTED_VALUE) ||                        \
      defined(BACNET_TIMER_VALUE_DATETIME) ||                                 \
      defined(BACNET_TIMER_VALUE_LIGHTING_COMMAND))
#define BACNET_TIMER_VALUE_NUMERIC
#elif defined(BACNET_TIMER_VALUE_ALL)
#undef BACNET_TIMER_VALUE_NUMERIC
#define BACNET_TIMER_VALUE_NUMERIC
#undef BACNET_TIMER_VALUE_OCTET_STRING
#define BACNET_TIMER_VALUE_OCTET_STRING
#undef BACNET_TIMER_VALUE_CHARACTER_STRING
#define BACNET_TIMER_VALUE_CHARACTER_STRING
#undef BACNET_TIMER_VALUE_BIT_STRING
#define BACNET_TIMER_VALUE_BIT_STRING
#undef BACNET_TIMER_VALUE_DATE
#define BACNET_TIMER_VALUE_DATE
#undef BACNET_TIMER_VALUE_TIME
#define BACNET_TIMER_VALUE_TIME
#undef BACNET_TIMER_VALUE_OBJECT_ID
#define BACNET_TIMER_VALUE_OBJECT_ID
#undef BACNET_TIMER_VALUE_DATETIME
#define BACNET_TIMER_VALUE_DATETIME
#undef BACNET_TIMER_VALUE_CONSTRUCTED_VALUE
#define BACNET_TIMER_VALUE_CONSTRUCTED_VALUE
#undef BACNET_TIMER_VALUE_LIGHTING_COMMAND
#define BACNET_TIMER_VALUE_LIGHTING_COMMAND
#endif

#if defined(BACNET_TIMER_VALUE_NUMERIC)
#undef BACNET_TIMER_VALUE_NULL
#define BACNET_TIMER_VALUE_NULL
#undef BACNET_TIMER_VALUE_BOOLEAN
#define BACNET_TIMER_VALUE_BOOLEAN
#undef BACNET_TIMER_VALUE_UNSIGNED
#define BACNET_TIMER_VALUE_UNSIGNED
#undef BACNET_TIMER_VALUE_SIGNED
#define BACNET_TIMER_VALUE_SIGNED
#undef BACNET_TIMER_VALUE_REAL
#define BACNET_TIMER_VALUE_REAL
#undef BACNET_TIMER_VALUE_DOUBLE
#define BACNET_TIMER_VALUE_DOUBLE
#undef BACNET_TIMER_VALUE_ENUMERATED
#define BACNET_TIMER_VALUE_ENUMERATED
#undef BACNET_TIMER_VALUE_NO_VALUE
#define BACNET_TIMER_VALUE_NO_VALUE
#endif

typedef struct BACnet_Timer_State_Change_Value_T {
    uint8_t tag;
    union {
        /* null - no type value needed. It is encoded in the tag alone. */
        /* no-value - no type value needed. It is encoded in the tag alone. */
#if defined(BACNET_TIMER_VALUE_BOOLEAN)
        bool Boolean;
#endif
#if defined(BACNET_TIMER_VALUE_UNSIGNED)
        BACNET_UNSIGNED_INTEGER Unsigned_Int;
#endif
#if defined(BACNET_TIMER_VALUE_SIGNED)
        int32_t Signed_Int;
#endif
#if defined(BACNET_TIMER_VALUE_REAL)
        float Real;
#endif
#if defined(BACNET_TIMER_VALUE_DOUBLE)
        double Double;
#endif
#if defined(BACNET_TIMER_VALUE_OCTET_STRING)
        BACNET_OCTET_STRING Octet_String;
#endif
#if defined(BACNET_TIMER_VALUE_CHARACTER_STRING)
        BACNET_CHARACTER_STRING Character_String;
#endif
#if defined(BACNET_TIMER_VALUE_BIT_STRING)
        BACNET_BIT_STRING Bit_String;
#endif
#if defined(BACNET_TIMER_VALUE_ENUMERATED)
        uint32_t Enumerated;
#endif
#if defined(BACNET_TIMER_VALUE_DATE)
        BACNET_DATE Date;
#endif
#if defined(BACNET_TIMER_VALUE_TIME)
        BACNET_TIME Time;
#endif
#if defined(BACNET_TIMER_VALUE_OBJECT_ID)
        BACNET_OBJECT_ID Object_Id;
#endif
#if defined(BACNET_TIMER_VALUE_DATETIME)
        BACNET_DATE_TIME Date_Time;
#endif
#if defined(BACNET_TIMER_VALUE_CONSTRUCTED_VALUE)
        BACNET_CONSTRUCTED_VALUE_TYPE Constructed_Value;
#endif
#if defined(BACNET_TIMER_VALUE_LIGHTING_COMMAND)
        BACNET_LIGHTING_COMMAND Lighting_Command;
#endif
    } type;
    /* simple linked list if needed */
    struct BACnet_Timer_State_Change_Value_T *next;
} BACNET_TIMER_STATE_CHANGE_VALUE;

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

BACNET_STACK_EXPORT
int bacnet_timer_value_no_value_encode(uint8_t *apdu);
BACNET_STACK_EXPORT
int bacnet_timer_value_no_value_decode(const uint8_t *apdu, uint32_t apdu_size);
BACNET_STACK_EXPORT
int bacnet_timer_value_no_value_to_ascii(char *str, size_t str_len);
BACNET_STACK_EXPORT
bool bacnet_timer_value_no_value_from_ascii(uint8_t *tag, const char *argv);

BACNET_STACK_EXPORT
int bacnet_timer_value_type_encode(
    uint8_t *apdu, const BACNET_TIMER_STATE_CHANGE_VALUE *value);
BACNET_STACK_EXPORT
int bacnet_timer_value_type_decode(
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
