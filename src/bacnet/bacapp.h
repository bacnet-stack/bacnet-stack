/**************************************************************************
 *
 * Copyright (C) 2012 Steve Karg <skarg@users.sourceforge.net>
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *********************************************************************/
#ifndef BACAPP_H
#define BACAPP_H

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include "bacnet/bacnet_stack_exports.h"
#include "bacnet/bacdef.h"
#include "bacnet/bacint.h"
#include "bacnet/bacstr.h"
#include "bacnet/datetime.h"
#include "bacnet/lighting.h"
#include "bacnet/bacdevobjpropref.h"
#include "bacnet/hostnport.h"
#include "bacnet/timestamp.h"

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
#if defined (BACAPP_TYPES_EXTRA)
        BACNET_TIMESTAMP Time_Stamp;
        BACNET_DATE_TIME Date_Time;
        BACNET_LIGHTING_COMMAND Lighting_Command;
        BACNET_COLOR_COMMAND Color_Command;
        BACNET_XY_COLOR XY_Color;
        BACNET_HOST_N_PORT Host_Address;
        BACNET_DEVICE_OBJECT_PROPERTY_REFERENCE
            Device_Object_Property_Reference;
        BACNET_DEVICE_OBJECT_REFERENCE
            Device_Object_Reference;
        BACNET_OBJECT_PROPERTY_REFERENCE
            Object_Property_Reference;
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
    int bacapp_encode_data(
        uint8_t * apdu,
        BACNET_APPLICATION_DATA_VALUE * value);
    BACNET_STACK_EXPORT
    int bacapp_decode_data(
        uint8_t * apdu,
        uint8_t tag_data_type,
        uint32_t len_value_type,
        BACNET_APPLICATION_DATA_VALUE * value);

    BACNET_STACK_EXPORT
    int bacapp_decode_application_data(
        uint8_t * apdu,
        unsigned max_apdu_len,
        BACNET_APPLICATION_DATA_VALUE * value);

    BACNET_STACK_EXPORT
    bool bacapp_decode_application_data_safe(
        uint8_t * new_apdu,
        uint32_t new_apdu_len,
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

    BACNET_STACK_EXPORT
    BACNET_APPLICATION_TAG bacapp_context_tag_type(
        BACNET_PROPERTY_ID property,
        uint8_t tag_number);

    BACNET_STACK_EXPORT
    int bacapp_decode_generic_property(
        uint8_t * apdu,
        int max_apdu_len,
        BACNET_APPLICATION_DATA_VALUE * value,
        BACNET_PROPERTY_ID prop);
    BACNET_STACK_EXPORT
    int bacapp_decode_known_property(uint8_t *apdu,
        int max_apdu_len,
        BACNET_APPLICATION_DATA_VALUE *value,
        BACNET_OBJECT_TYPE object_type,
        BACNET_PROPERTY_ID property);

    BACNET_STACK_EXPORT
    bool bacapp_copy(
        BACNET_APPLICATION_DATA_VALUE * dest_value,
        BACNET_APPLICATION_DATA_VALUE * src_value);

    /* returns the length of data between an opening tag and a closing tag.
       Expects that the first octet contain the opening tag.
       Include a value property identifier for context specific data
       such as the value received in a WriteProperty request */
    BACNET_STACK_EXPORT
    int bacapp_data_len(
        uint8_t * apdu,
        unsigned max_apdu_len,
        BACNET_PROPERTY_ID property);
    BACNET_STACK_EXPORT
    int bacapp_decode_data_len(
        uint8_t * apdu,
        uint8_t tag_data_type,
        uint32_t len_value_type);
    BACNET_STACK_EXPORT
    int bacapp_decode_application_data_len(
        uint8_t * apdu,
        unsigned max_apdu_len);
    BACNET_STACK_EXPORT
    int bacapp_decode_context_data_len(
        uint8_t * apdu,
        unsigned max_apdu_len,
        BACNET_PROPERTY_ID property);

#ifndef BACAPP_PRINT_ENABLED
#if PRINT_ENABLED
#define BACAPP_PRINT_ENABLED
#endif
#endif
    BACNET_STACK_EXPORT
    int bacapp_snprintf_value(
        char *str,
        size_t str_len,
        BACNET_OBJECT_PROPERTY_VALUE * object_value);

#ifdef BACAPP_PRINT_ENABLED
    BACNET_STACK_EXPORT
    bool bacapp_parse_application_data(
        BACNET_APPLICATION_TAG tag_number,
        const char *argv,
        BACNET_APPLICATION_DATA_VALUE * value);
    BACNET_STACK_EXPORT
    bool bacapp_print_value(
        FILE * stream,
        BACNET_OBJECT_PROPERTY_VALUE * value);
#else
/* Provide harmless return values */
#define bacapp_parse_application_data(x,y,z)   false
#define bacapp_print_value(x,y) 			   false
#endif

    BACNET_STACK_EXPORT
    bool bacapp_same_value(
        BACNET_APPLICATION_DATA_VALUE * value,
        BACNET_APPLICATION_DATA_VALUE * test_value);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif
