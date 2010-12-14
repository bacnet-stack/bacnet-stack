/*####COPYRIGHTBEGIN####
 -------------------------------------------
 Copyright (C) 2005 Steve Karg

 This program is free software; you can redistribute it and/or
 modify it under the terms of the GNU General Public License
 as published by the Free Software Foundation; either version 2
 of the License, or (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program; if not, write to:
 The Free Software Foundation, Inc.
 59 Temple Place - Suite 330
 Boston, MA  02111-1307, USA.

 As a special exception, if other files instantiate templates or
 use macros or inline functions from this file, or you compile
 this file and link it with other works to produce a work based
 on this file, this file does not by itself cause the resulting
 work to be covered by the GNU General Public License. However
 the source code for this file must still be made available in
 accordance with section (3) of the GNU General Public License.

 This exception does not invalidate any other reasons why a work
 based on this file might be covered by the GNU General Public
 License.
 -------------------------------------------
####COPYRIGHTEND####*/
#ifndef BACAPP_H
#define BACAPP_H

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include "bacdef.h"
#include "bacstr.h"
#include "datetime.h"
#include "bacdevobjpropref.h"
#include "timestamp.h"
#include "bacothertypes.h"

struct BACnet_Application_Data_Value;
struct BACnet_Event_Notification_Data;

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
        uint32_t Unsigned_Int;
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
#if defined (BACAPP_TYPES_EXTRA)        /* standard composed types : */
        BACNET_DATE_TIME Date_Time;
        BACNET_DATE_RANGE Date_Range;
        BACNET_TIMESTAMP TimeStamp;
        BACNET_WEEKNDAY Weeknday;
        BACNET_ACCESS_ERROR Access_Error;
        BACNET_DESTINATION Destination;
        BACNET_RECIPIENT Recipient;
        BACNET_COV_SUBSCRIPTION COV_Subscription;
                                BACNET_DEVICE_OBJECT_PROPERTY_REFERENCE
            Device_Object_Property_Reference;
        BACNET_DEVICE_OBJECT_REFERENCE Device_Object_Reference;
        BACNET_OBJECT_PROPERTY_REFERENCE Object_Property_Reference;
        BACNET_CALENDAR_ENTRY Calendar_Entry;
        BACNET_WEEKLY_SCHEDULE Weekly_Schedule;
        BACNET_SPECIAL_EVENT Special_Event;
        BACNET_READ_ACCESS_SPECIFICATION Read_Access_Specification;
#endif
#if defined (BACAPP_OBJECT_ID)
        BACNET_OBJECT_ID Object_Id;
#endif
    } type;
    /* simple linked list if needed */
    struct BACnet_Application_Data_Value *next;
} BACNET_APPLICATION_DATA_VALUE;

struct BACnet_Property_Reference;
typedef struct BACnet_Property_Reference {
    BACNET_PROPERTY_ID propertyIdentifier;
    int32_t propertyArrayIndex; /* optional */
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
    int32_t propertyArrayIndex;
    BACNET_APPLICATION_DATA_VALUE value;
    uint8_t priority;
    /* simple linked list */
    struct BACnet_Property_Value *next;
} BACNET_PROPERTY_VALUE;

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */
    int bacapp_encode_data(
        uint8_t * apdu,
        int max_apdu_len,
        BACNET_APPLICATION_DATA_VALUE * value);

    int bacapp_decode_application_data(
        uint8_t * apdu,
        int max_apdu_len,
        BACNET_APPLICATION_DATA_VALUE * value);

    bool bacapp_decode_application_data_safe(
        uint8_t * new_apdu,
        uint32_t new_apdu_len,
        BACNET_APPLICATION_DATA_VALUE * value);

    int bacapp_encode_application_data(
        uint8_t * apdu,
        int max_apdu_len,
        BACNET_APPLICATION_DATA_VALUE * value);

    int bacapp_decode_context_data(
        uint8_t * apdu,
        int max_apdu_len,
        BACNET_APPLICATION_DATA_VALUE * value,
        BACNET_PROPERTY_ID property);

    int bacapp_encode_context_data(
        uint8_t * apdu,
        BACNET_APPLICATION_DATA_VALUE * value,
        BACNET_PROPERTY_ID property);

    int bacapp_encode_context_data_value(
        uint8_t * apdu,
        uint8_t context_tag_number,
        BACNET_APPLICATION_DATA_VALUE * value);

    BACNET_APPLICATION_TAG bacapp_context_tag_type(
        BACNET_PROPERTY_ID property,
        uint8_t tag_number);

    /*/ Destroy values and also linked other values */
    void bacapp_desallocate_values(
        BACNET_APPLICATION_DATA_VALUE * value);

    /*/ Uses of the short-app-value type (limited 32 bits yet) */
    void copy_app_to_short_app_value(
        BACNET_SHORT_APPLICATION_DATA_VALUE * shortvaluedest,
        BACNET_APPLICATION_DATA_VALUE * valuesrc);
    /*/ Uses of the short-app-value type (limited 32 bits yet) */
    void copy_short_app_to_app_value(
        BACNET_APPLICATION_DATA_VALUE * valuedest,
        BACNET_SHORT_APPLICATION_DATA_VALUE * shortvaluesrc);

    /* read a well-known value, or fallback to read app data / context data */
    int bacapp_decode_known_property(
        uint8_t * apdu,
        int max_apdu_len,
        BACNET_APPLICATION_DATA_VALUE * value,
        BACNET_PROPERTY_ID prop);
    /* Loop through many well-known values */
    int bacapp_decode_known_property_until_tag(
        uint8_t * apdu,
        int max_apdu_len,
        BACNET_APPLICATION_DATA_VALUE * value,
        BACNET_PROPERTY_ID prop,
        uint8_t closing_tag_number);
    /*/ Loop through many well-known values */
    int bacapp_decode_known_property_until_end(
        uint8_t * apdu,
        int max_apdu_len,
        BACNET_APPLICATION_DATA_VALUE * value,
        BACNET_PROPERTY_ID prop);

    bool bacapp_copy(
        BACNET_APPLICATION_DATA_VALUE * dest_value,
        BACNET_APPLICATION_DATA_VALUE * src_value);

    /* returns the length of data between an opening tag and a closing tag.
       Expects that the first octet contain the opening tag.
       Include a value property identifier for context specific data
       such as the value received in a WriteProperty request */
    int bacapp_data_len(
        uint8_t * apdu,
        int max_apdu_len,
        BACNET_PROPERTY_ID property);

    /* Decoding a Log-buffer of a trend-log */
    int bacapp_decode_bacnetlogrecord(
        uint8_t * apdu,
        int apdu_len,
        int *choice_tag_number,
        BACNET_DATE_TIME * value_timestamp,
        BACNET_APPLICATION_DATA_VALUE * value_logdatum,
        BACNET_BIT_STRING * value_statusflags);

    /* Decode application data for object(s) BACnetEventLogRecord, which can be found in the log-buffer property of a event-log object. */
    int bacapp_decode_bacneteventlogrecord(
        uint8_t * apdu,
        int apdu_len,
        int *choice_tag_number,
        BACNET_DATE_TIME * value_timestamp,
        BACNET_APPLICATION_DATA_VALUE * value_status_or_time,
        struct BACnet_Event_Notification_Data *notification_data);


#if PRINT_ENABLED
#define BACAPP_PRINT_ENABLED
#else
#ifdef TEST
#define BACAPP_PRINT_ENABLED
#endif
#endif

#ifdef BACAPP_PRINT_ENABLED
    bool bacapp_parse_application_data(
        BACNET_APPLICATION_TAG tag_number,
        const char *argv,
        BACNET_APPLICATION_DATA_VALUE * value);
    bool bacapp_print_value(
        FILE * stream,
        BACNET_APPLICATION_DATA_VALUE * value,
        BACNET_PROPERTY_ID property);
#else
#define bacapp_parse_application_data(x,y,z) {(void)x;(void)y;(void)z;}
#define bacapp_print_value(x,y,z) {(void)x;(void)y;(void)z;}
#endif

#ifdef TEST
#include "ctest.h"
#include "datetime.h"
    bool bacapp_same_value(
        BACNET_APPLICATION_DATA_VALUE * value,
        BACNET_APPLICATION_DATA_VALUE * test_value);

    void testBACnetApplicationDataLength(
        Test * pTest);
    void testBACnetApplicationData(
        Test * pTest);
#endif

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif
