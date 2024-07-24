/**
 * @file
 * @brief BACnet application data encode and decode helper functions
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date 2007
 * @copyright SPDX-License-Identifier: MIT
 */
#ifndef BACNET_APP_H
#define BACNET_APP_H

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
/* BACnet Stack defines - first */
#include "bacnet/bacdef.h"
/* BACnet Stack API */
#include "bacnet/bacaction.h"
#include "bacnet/bacdest.h"
#include "bacnet/bacint.h"
#include "bacnet/bacstr.h"
#include "bacnet/datetime.h"
#include "bacnet/lighting.h"
#include "bacnet/bacdevobjpropref.h"
#include "bacnet/hostnport.h"
#include "bacnet/timestamp.h"
#include "bacnet/weeklyschedule.h"
#include "bacnet/calendar_entry.h"
#include "bacnet/special_event.h"

#ifndef BACAPP_PRINT_ENABLED
#if PRINT_ENABLED
#define BACAPP_PRINT_ENABLED
#endif
#endif

/** BACnetScale ::= CHOICE {
        float-scale [0] REAL,
        integer-scale [1] INTEGER
    }
*/
typedef struct BACnetScale {
    bool float_scale;
    union {
        float real_scale;
        int32_t integer_scale;
    } type;
} BACNET_SCALE;

struct BACnet_Application_Data_Value;
typedef struct BACnet_Application_Data_Value {
    bool context_specific;      /* true if context specific data */
    uint8_t context_tag;        /* only used for context specific data */
    uint8_t tag;        /* application tag data type */
    union {
        /* NULL - not needed as it is encoded in the tag alone */
#if defined (BACAPP_BOOLEAN)
        bool Boolean;
#endif
#if defined (BACAPP_UNSIGNED)
        BACNET_UNSIGNED_INTEGER Unsigned_Int;
#endif
#if defined (BACAPP_SIGNED)
        int32_t Signed_Int;
#endif
#if defined (BACAPP_REAL)
        float Real;
#endif
#if defined (BACAPP_DOUBLE)
        double Double;
#endif
#if defined (BACAPP_OCTET_STRING)
        BACNET_OCTET_STRING Octet_String;
#endif
#if defined (BACAPP_CHARACTER_STRING)
        BACNET_CHARACTER_STRING Character_String;
#endif
#if defined (BACAPP_BIT_STRING)
        BACNET_BIT_STRING Bit_String;
#endif
#if defined (BACAPP_ENUMERATED)
        uint32_t Enumerated;
#endif
#if defined (BACAPP_DATE)
        BACNET_DATE Date;
#endif
#if defined (BACAPP_TIME)
        BACNET_TIME Time;
#endif
#if defined (BACAPP_OBJECT_ID)
        BACNET_OBJECT_ID Object_Id;
#endif
#if defined (BACAPP_TIMESTAMP)
        BACNET_TIMESTAMP Time_Stamp;
#endif
#if defined (BACAPP_DATETIME)
        BACNET_DATE_TIME Date_Time;
#endif
#if defined (BACAPP_DATERANGE)
        BACNET_DATE_RANGE Date_Range;
#endif
#if defined (BACAPP_LIGHTING_COMMAND)
        BACNET_LIGHTING_COMMAND Lighting_Command;
#endif
#if defined (BACAPP_XY_COLOR)
        BACNET_XY_COLOR XY_Color;
#endif
#if defined (BACAPP_COLOR_COMMAND)
        BACNET_COLOR_COMMAND Color_Command;
#endif
#if defined (BACAPP_WEEKLY_SCHEDULE)
        BACNET_WEEKLY_SCHEDULE Weekly_Schedule;
#endif
#if defined (BACAPP_HOST_N_PORT)
        BACNET_HOST_N_PORT Host_Address;
#endif
#if defined (BACAPP_DEVICE_OBJECT_PROPERTY_REFERENCE)
        BACNET_DEVICE_OBJECT_PROPERTY_REFERENCE
            Device_Object_Property_Reference;
#endif
#if defined (BACAPP_DEVICE_OBJECT_REFERENCE)
        BACNET_DEVICE_OBJECT_REFERENCE
            Device_Object_Reference;
#endif
#if defined (BACAPP_OBJECT_PROPERTY_REFERENCE)
        BACNET_OBJECT_PROPERTY_REFERENCE
            Object_Property_Reference;
#endif
#if defined (BACAPP_DESTINATION)
        BACNET_DESTINATION Destination;
#endif
#if defined (BACAPP_CALENDAR_ENTRY)
        BACNET_CALENDAR_ENTRY Calendar_Entry;
#endif
#if defined (BACAPP_SPECIAL_EVENT)
        BACNET_SPECIAL_EVENT Special_Event;
#endif
#if defined (BACAPP_BDT_ENTRY)
        BACNET_BDT_ENTRY BDT_Entry;
#endif
#if defined (BACAPP_FDT_ENTRY)
        BACNET_FDT_ENTRY FDT_Entry;
#endif
#if defined (BACAPP_ACTION_COMMAND)
        BACNET_ACTION_LIST Action_Command;
#endif
#if defined (BACAPP_SCALE)
        BACNET_SCALE Scale;
#endif
    } type;
    /* simple linked list if needed */
    struct BACnet_Application_Data_Value *next;
} BACNET_APPLICATION_DATA_VALUE;

struct BACnet_Access_Error;
typedef struct BACnet_Access_Error {
    BACNET_ERROR_CLASS error_class;
    BACNET_ERROR_CODE error_code;
} BACNET_ACCESS_ERROR;

struct BACnet_Property_Reference;
typedef struct BACnet_Property_Reference {
    BACNET_PROPERTY_ID propertyIdentifier;
    /* optional array index */
    BACNET_ARRAY_INDEX propertyArrayIndex;
    /* either value or error, but not both.
       Use NULL value to indicate error */
    BACNET_APPLICATION_DATA_VALUE *value;
    BACNET_ACCESS_ERROR error;
    /* simple linked list */
    struct BACnet_Property_Reference *next;
} BACNET_PROPERTY_REFERENCE;

struct BACnet_Property_Value;
typedef struct BACnet_Property_Value {
    BACNET_PROPERTY_ID propertyIdentifier;
    BACNET_ARRAY_INDEX propertyArrayIndex;
    BACNET_APPLICATION_DATA_VALUE value;
    uint8_t priority;
    /* simple linked list */
    struct BACnet_Property_Value *next;
} BACNET_PROPERTY_VALUE;

/* used for printing values */
struct BACnet_Object_Property_Value;
typedef struct BACnet_Object_Property_Value {
    BACNET_OBJECT_TYPE object_type;
    uint32_t object_instance;
    BACNET_PROPERTY_ID object_property;
    BACNET_ARRAY_INDEX array_index;
    BACNET_APPLICATION_DATA_VALUE *value;
} BACNET_OBJECT_PROPERTY_VALUE;

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

    BACNET_STACK_EXPORT
    void bacapp_value_list_init(
        BACNET_APPLICATION_DATA_VALUE *value,
        size_t count);
    BACNET_STACK_EXPORT
    void bacapp_property_value_list_init(
        BACNET_PROPERTY_VALUE *value,
        size_t count);

    BACNET_STACK_EXPORT
    void bacapp_property_value_list_link(
        BACNET_PROPERTY_VALUE *value_list,
        size_t count);

    BACNET_STACK_EXPORT
    int bacapp_property_value_encode(
        uint8_t *apdu,
        BACNET_PROPERTY_VALUE *value);
    BACNET_STACK_EXPORT
    int bacapp_property_value_decode(
        uint8_t *apdu,
        uint32_t apdu_size,
        BACNET_PROPERTY_VALUE *value);

    BACNET_STACK_EXPORT
    int bacapp_encode_data(
        uint8_t * apdu,
        BACNET_APPLICATION_DATA_VALUE * value);
    BACNET_STACK_EXPORT
    int bacapp_encode_known_property(
        uint8_t *apdu,
        BACNET_APPLICATION_DATA_VALUE *value,
        BACNET_OBJECT_TYPE object_type,
        BACNET_PROPERTY_ID property);
    BACNET_STACK_EXPORT
    int bacapp_data_decode(
        uint8_t * apdu,
        uint32_t apdu_size,
        uint8_t tag_data_type,
        uint32_t len_value_type,
        BACNET_APPLICATION_DATA_VALUE * value);
    BACNET_STACK_DEPRECATED("Use bacapp_data_decode() instead")
    BACNET_STACK_EXPORT
    int bacapp_decode_data(
        uint8_t * apdu,
        uint8_t tag_data_type,
        uint32_t len_value_type,
        BACNET_APPLICATION_DATA_VALUE * value);

    BACNET_STACK_EXPORT
    int bacapp_decode_application_data(
        uint8_t * apdu,
        uint32_t apdu_size,
        BACNET_APPLICATION_DATA_VALUE * value);

    BACNET_STACK_EXPORT
    bool bacapp_decode_application_data_safe(
        uint8_t * apdu,
        uint32_t apdu_size,
        BACNET_APPLICATION_DATA_VALUE * value);

    BACNET_STACK_EXPORT
    int bacapp_encode_application_data(
        uint8_t * apdu,
        BACNET_APPLICATION_DATA_VALUE * value);

    BACNET_STACK_EXPORT
    int bacapp_decode_context_data(
        uint8_t * apdu,
        unsigned max_apdu_len,
        BACNET_APPLICATION_DATA_VALUE * value,
        BACNET_PROPERTY_ID property);

    BACNET_STACK_DEPRECATED("Use bacapp_encode_known_property() instead")
    BACNET_STACK_EXPORT
    int bacapp_encode_context_data(
        uint8_t * apdu,
        BACNET_APPLICATION_DATA_VALUE * value,
        BACNET_PROPERTY_ID property);

    BACNET_STACK_EXPORT
    int bacapp_encode_context_data_value(
        uint8_t * apdu,
        uint8_t context_tag_number,
        BACNET_APPLICATION_DATA_VALUE * value);

    BACNET_STACK_DEPRECATED("Use bacapp_known_property_tag() instead")
    BACNET_STACK_EXPORT
    BACNET_APPLICATION_TAG bacapp_context_tag_type(
        BACNET_PROPERTY_ID property,
        uint8_t tag_number);
    BACNET_STACK_DEPRECATED("Use bacapp_encode_known_property() instead")
    BACNET_STACK_EXPORT
    int bacapp_decode_generic_property(
        uint8_t * apdu,
        int max_apdu_len,
        BACNET_APPLICATION_DATA_VALUE * value,
        BACNET_PROPERTY_ID prop);
        
    BACNET_STACK_EXPORT
    int bacapp_decode_application_tag_value(
        uint8_t *apdu,
        size_t apdu_size,
        BACNET_APPLICATION_TAG tag,
        BACNET_APPLICATION_DATA_VALUE *value);
    BACNET_STACK_EXPORT
    int bacapp_decode_known_property(uint8_t *apdu,
        int max_apdu_len,
        BACNET_APPLICATION_DATA_VALUE *value,
        BACNET_OBJECT_TYPE object_type,
        BACNET_PROPERTY_ID property);

    BACNET_STACK_EXPORT
    int bacapp_known_property_tag(
        BACNET_OBJECT_TYPE object_type,
        BACNET_PROPERTY_ID property);

    BACNET_STACK_EXPORT
    bool bacapp_copy(
        BACNET_APPLICATION_DATA_VALUE * dest_value,
        BACNET_APPLICATION_DATA_VALUE * src_value);

    BACNET_STACK_DEPRECATED("Use bacnet_enclosed_data_length() instead")
    BACNET_STACK_EXPORT
    int bacapp_data_len(
        uint8_t * apdu,
        unsigned max_apdu_len,
        BACNET_PROPERTY_ID property);
        
    BACNET_STACK_DEPRECATED("Use bacnet_application_data_length() instead")
    BACNET_STACK_EXPORT
    int bacapp_decode_data_len(
        uint8_t * apdu,
        uint8_t tag_data_type,
        uint32_t len_value_type);

    BACNET_STACK_EXPORT
    int bacapp_snprintf_value(
        char *str,
        size_t str_len,
        BACNET_OBJECT_PROPERTY_VALUE * object_value);

    BACNET_STACK_EXPORT
    bool bacapp_parse_application_data(
        BACNET_APPLICATION_TAG tag_number,
        char *argv,
        BACNET_APPLICATION_DATA_VALUE * value);

    BACNET_STACK_EXPORT
    bool bacapp_print_value(
        FILE * stream,
        BACNET_OBJECT_PROPERTY_VALUE * value);

    BACNET_STACK_EXPORT
    bool bacapp_same_value(
        BACNET_APPLICATION_DATA_VALUE * value,
        BACNET_APPLICATION_DATA_VALUE * test_value);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif
