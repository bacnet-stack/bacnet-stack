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
#include "bacnet/bactimevalue.h"

/* BACNET_ACTION_VALUE encodes and decodes the Property Value
   in the Action Command. Choose the datatypes that your
   application supports, or add additional. */
#if !(                                                              \
    defined(BACACTION_ALL) || defined(BACACTION_NULL) ||            \
    defined(BACACTION_BOOLEAN) || defined(BACACTION_UNSIGNED) ||    \
    defined(BACACTION_SIGNED) || defined(BACACTION_REAL) ||         \
    defined(BACACTION_DOUBLE) || defined(BACACTION_OCTET_STRING) || \
    defined(BACACTION_ENUMERATED))
#define BACACTION_ALL
#endif

#if defined(BACACTION_ALL)
#define BACACTION_NULL
#define BACACTION_BOOLEAN
#define BACACTION_UNSIGNED
#define BACACTION_SIGNED
#define BACACTION_REAL
#define BACACTION_DOUBLE
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
#if defined(BACACTION_ENUMERATED)
        uint32_t Enumerated;
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
    BACNET_ACTION_PROPERTY_VALUE Value;
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
