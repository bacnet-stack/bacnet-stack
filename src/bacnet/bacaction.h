/**
 * @file
 * @brief API for BACnetActionCommand codec used by Command objects
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date 2024
 * @copyright SPDX-License-Identifier: MIT
 */
#ifndef BACNET_ACTION_H
#define BACNET_ACTION_H
#include <stdbool.h>
#include <stdint.h>
/* BACnet Stack defines - first */
#include "bacnet/bacdef.h"
#include "bacnet/bacdcode.h"
#include "bacnet/bacstr.h"
#include "bacnet/bactimevalue.h"
#include "bacnet/datetime.h"

/* BACNET_ACTION_VALUE encodes and decodes the Property Value
   in the Action Command. Choose the datatypes that your
   application supports, or add additional. */
#if !(                                                                      \
    defined(BACACTION_NUMERIC) || defined(BACACTION_NULL) ||                \
    defined(BACACTION_BOOLEAN) || defined(BACACTION_UNSIGNED) ||            \
    defined(BACACTION_SIGNED) || defined(BACACTION_REAL) ||                 \
    defined(BACACTION_DOUBLE) || defined(BACACTION_OCTET_STRING) ||         \
    defined(BACACTION_CHARACTER_STRING) || defined(BACACTION_BIT_STRING) || \
    defined(BACACTION_ENUMERATED) || defined(BACACTION_DATE) ||             \
    defined(BACACTION_TIME) || defined(BACACTION_OBJECT_ID) ||              \
    defined(BACACTION_VALUE_ALL))
#define BACACTION_NUMERIC
#elif defined(BACACTION_VALUE_ALL)
#undef BACACTION_NUMERIC
#define BACACTION_NUMERIC
#undef BACACTION_OCTET_STRING
#define BACACTION_OCTET_STRING
#undef BACACTION_CHARACTER_STRING
#define BACACTION_CHARACTER_STRING
#undef BACACTION_BIT_STRING
#define BACACTION_BIT_STRING
#undef BACACTION_DATE
#define BACACTION_DATE
#undef BACACTION_TIME
#define BACACTION_TIME
#undef BACACTION_OBJECT_ID
#define BACACTION_OBJECT_ID
#endif

#if defined(BACACTION_NUMERIC)
#undef BACACTION_NULL
#define BACACTION_NULL
#undef BACACTION_BOOLEAN
#define BACACTION_BOOLEAN
#undef BACACTION_UNSIGNED
#define BACACTION_UNSIGNED
#undef BACACTION_SIGNED
#define BACACTION_SIGNED
#undef BACACTION_REAL
#define BACACTION_REAL
#undef BACACTION_DOUBLE
#define BACACTION_DOUBLE
#undef BACACTION_ENUMERATED
#define BACACTION_ENUMERATED
#endif

typedef struct BACnetActionPropertyValue {
    uint8_t tag;
    union {
#if defined(BACACTION_NULL)
        /* NULL - not needed because it is encoded in the tag alone */
#endif
#if defined(BACACTION_BOOLEAN)
        bool Boolean;
#endif
#if defined(BACACTION_UNSIGNED)
        BACNET_UNSIGNED_INTEGER Unsigned_Int;
#endif
#if defined(BACACTION_SIGNED)
        int32_t Signed_Int;
#endif
#if defined(BACACTION_REAL)
        float Real;
#endif
#if defined(BACACTION_DOUBLE)
        double Double;
#endif
#if defined(BACACTION_OCTET_STRING)
        BACNET_OCTET_STRING Octet_String;
#endif
#if defined(BACACTION_CHARACTER_STRING)
        BACNET_CHARACTER_STRING Character_String;
#endif
#if defined(BACACTION_BIT_STRING)
        BACNET_BIT_STRING Bit_String;
#endif
#if defined(BACACTION_ENUMERATED)
        uint32_t Enumerated;
#endif
#if defined(BACACTION_DATE)
        BACNET_DATE Date;
#endif
#if defined(BACACTION_TIME)
        BACNET_TIME Time;
#endif
#if defined(BACACTION_OBJECT_ID)
        BACNET_OBJECT_ID Object_Id;
#endif
    } type;
    /* simple linked list if needed */
    struct BACnetActionPropertyValue *next;
} BACNET_ACTION_PROPERTY_VALUE;

typedef struct bacnet_action_list {
    BACNET_OBJECT_ID Device_Id; /* Optional */
    BACNET_OBJECT_ID Object_Id;
    BACNET_PROPERTY_ID Property_Identifier;
    uint32_t Property_Array_Index; /* Conditional */
    BACNET_CONSTRUCTED_VALUE_TYPE Property_Value;
    uint8_t Priority; /* Conditional */
    uint32_t Post_Delay; /* Optional */
    bool Quit_On_Failure;
    bool Write_Successful;
    struct bacnet_action_list *next;
} BACNET_ACTION_LIST;

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

BACNET_STACK_EXPORT
int bacnet_action_property_value_encode(
    uint8_t *apdu, const BACNET_ACTION_PROPERTY_VALUE *value);
BACNET_STACK_EXPORT
int bacnet_action_property_value_decode(
    const uint8_t *apdu,
    uint32_t apdu_size,
    BACNET_ACTION_PROPERTY_VALUE *value);
BACNET_STACK_EXPORT
bool bacnet_action_property_value_same(
    const BACNET_ACTION_PROPERTY_VALUE *value1,
    const BACNET_ACTION_PROPERTY_VALUE *value2);

BACNET_STACK_EXPORT
int bacnet_action_command_encode(
    uint8_t *apdu, const BACNET_ACTION_LIST *entry);

BACNET_STACK_EXPORT
int bacnet_action_command_decode(
    const uint8_t *apdu, size_t apdu_size, BACNET_ACTION_LIST *entry);

BACNET_STACK_EXPORT
bool bacnet_action_command_same(
    const BACNET_ACTION_LIST *entry1, const BACNET_ACTION_LIST *entry2);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif
