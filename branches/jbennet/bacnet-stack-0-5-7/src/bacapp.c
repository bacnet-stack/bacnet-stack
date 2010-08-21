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
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include "bacenum.h"
#include "bacdcode.h"
#include "bacint.h"
#include "bacreal.h"
#include "bacdef.h"
#include "bacapp.h"
#include "bactext.h"
#include "datetime.h"
#include "bacerror.h"
#include "bacdevobjpropref.h"
#include "event.h"
/** @file bacapp.c  Utilities for the BACnet_Application_Data_Value */

int bacapp_encode_application_data(
    uint8_t * apdu,
    int max_apdu_len,
    BACNET_APPLICATION_DATA_VALUE * value)
{
    int apdu_len = 0;   /* total length of the apdu, return value */

    if (value && apdu) {
        switch (value->tag) {
#if defined (BACAPP_NULL)
            case BACNET_APPLICATION_TAG_NULL:
                if (max_apdu_len < 1)   /* Check for overflow */
                    return -1;
                apdu[0] = value->tag;
                apdu_len++;
                break;
#endif
#if defined (BACAPP_BOOLEAN)
            case BACNET_APPLICATION_TAG_BOOLEAN:
                if (max_apdu_len < 1)   /* Check for overflow */
                    return -1;
                apdu_len =
                    encode_application_boolean(&apdu[0], value->type.Boolean);
                break;
#endif
#if defined (BACAPP_UNSIGNED)
            case BACNET_APPLICATION_TAG_UNSIGNED_INT:
                if (max_apdu_len < 5)   /* Worst case limit */
                    return -1;
                apdu_len =
                    encode_application_unsigned(&apdu[0],
                    value->type.Unsigned_Int);
                break;
#endif
#if defined (BACAPP_SIGNED)
            case BACNET_APPLICATION_TAG_SIGNED_INT:
                if (max_apdu_len < 5)   /* Worst case limit */
                    return -1;
                apdu_len =
                    encode_application_signed(&apdu[0],
                    value->type.Signed_Int);
                break;
#endif
#if defined (BACAPP_REAL)
            case BACNET_APPLICATION_TAG_REAL:
                if (max_apdu_len < 5)
                    return -1;
                apdu_len = encode_application_real(&apdu[0], value->type.Real);
                break;
#endif
#if defined (BACAPP_DOUBLE)
            case BACNET_APPLICATION_TAG_DOUBLE:
                if (max_apdu_len < 10)
                    return -1;
                apdu_len =
                    encode_application_double(&apdu[0], value->type.Double);
                break;
#endif
#if defined (BACAPP_OCTET_STRING)
            case BACNET_APPLICATION_TAG_OCTET_STRING:
                if (max_apdu_len <
                    2 + (int) octetstring_length(&value->type.Octet_String))
                    return -1;
                apdu_len =
                    encode_application_octet_string(&apdu[0],
                    &value->type.Octet_String);
                break;
#endif
#if defined (BACAPP_CHARACTER_STRING)
            case BACNET_APPLICATION_TAG_CHARACTER_STRING:
                if (max_apdu_len <
                    3 +
                    (int) characterstring_length(&value->
                        type.Character_String))
                    return -1;
                apdu_len =
                    encode_application_character_string(&apdu[0],
                    &value->type.Character_String);
                break;
#endif
#if defined (BACAPP_BIT_STRING)
            case BACNET_APPLICATION_TAG_BIT_STRING:
                if (max_apdu_len <
                    2 + bitstring_bytes_used(&value->type.Bit_String))
                    return -1;
                apdu_len =
                    encode_application_bitstring(&apdu[0],
                    &value->type.Bit_String);
                break;
#endif
#if defined (BACAPP_ENUMERATED)
            case BACNET_APPLICATION_TAG_ENUMERATED:
                if (max_apdu_len < 5)   /* Worst case limit */
                    return -1;
                apdu_len =
                    encode_application_enumerated(&apdu[0],
                    value->type.Enumerated);
                break;
#endif
#if defined (BACAPP_DATE)
            case BACNET_APPLICATION_TAG_DATE:
                if (max_apdu_len < 5)   /* Worst case limit */
                    return -1;
                apdu_len =
                    encode_application_date(&apdu[0], &value->type.Date);
                break;
#endif
#if defined (BACAPP_TIME)
            case BACNET_APPLICATION_TAG_TIME:
                if (max_apdu_len < 5)   /* Worst case limit */
                    return -1;
                apdu_len =
                    encode_application_time(&apdu[0], &value->type.Time);
                break;
#endif
#if defined (BACAPP_OBJECT_ID)
            case BACNET_APPLICATION_TAG_OBJECT_ID:
                if (max_apdu_len < 5)   /* Worst case limit */
                    return -1;
                apdu_len =
                    encode_application_object_id(&apdu[0],
                    (int) value->type.Object_Id.type,
                    value->type.Object_Id.instance);
                break;
#endif
#if defined (BACAPP_TYPES_EXTRA)

            case BACNET_APPLICATION_TAG_EMPTYLIST:     /* Empty data list */
                apdu_len = 0;   /* EMPTY */
                break;

            case BACNET_APPLICATION_TAG_DATERANGE:     /* BACnetDateRange */
                if (max_apdu_len < 10)
                    return -1;
                apdu_len = encode_daterange(&apdu[0], &value->type.Date_Range);
                break;
            case BACNET_APPLICATION_TAG_DEVICE_OBJECT_PROPERTY_REFERENCE:      /* BACnetDeviceObjectPropertyReference */
                if (max_apdu_len < 20)  /* Worst case */
                    return -1;
                apdu_len =
                    bacapp_encode_device_obj_property_ref(&apdu[0],
                    &value->type.Device_Object_Property_Reference);
                break;
            case BACNET_APPLICATION_TAG_DEVICE_OBJECT_REFERENCE:       /* BACnetDeviceObjectReference */
                if (max_apdu_len < 10)  /* Worst case */
                    return -1;
                apdu_len =
                    bacapp_encode_device_obj_ref(&apdu[0],
                    &value->type.Device_Object_Reference);
                break;
            case BACNET_APPLICATION_TAG_OBJECT_PROPERTY_REFERENCE:     /* BACnetObjectPropertyReference */
                if (max_apdu_len < 15)  /* Worst case */
                    return -1;
                apdu_len =
                    bacapp_encode_obj_property_ref(&apdu[0],
                    &value->type.Object_Property_Reference);
                break;
            case BACNET_APPLICATION_TAG_DATETIME:      /* BACnetDateTime */
                if (max_apdu_len < 10)
                    return -1;
                apdu_len =
                    encode_application_datetime(&apdu[0],
                    &value->type.Date_Time);
                break;
            case BACNET_APPLICATION_TAG_TIMESTAMP:     /* BACnetTimeStamp */
                if (max_apdu_len < 12)  /* Worst case */
                    return -1;
                apdu_len =
                    bacapp_encode_timestamp(&apdu[0], &value->type.TimeStamp);
                break;
            case BACNET_APPLICATION_TAG_RECIPIENT:     /* BACnetRecipient */
                if (max_apdu_len < 20)  /* ~ limit on max MAC address ? - XXX */
                    return -1;
                apdu_len = encode_recipient(&apdu[0], &value->type.Recipient);
                break;
            case BACNET_APPLICATION_TAG_COV_SUBSCRIPTION:      /* BACnetCOVSubscription */
                apdu_len =
                    encode_cov_subscription(&apdu[0], max_apdu_len,
                    &value->type.COV_Subscription);
                break;
            case BACNET_APPLICATION_TAG_CALENDAR_ENTRY:        /* BACnetCalendarEntry */
                apdu_len =
                    encode_calendar_entry(&apdu[0], max_apdu_len,
                    &value->type.Calendar_Entry);
                break;
            case BACNET_APPLICATION_TAG_WEEKLY_SCHEDULE:       /* BACnetWeeklySchedule */
                apdu_len =
                    encode_weekly_schedule(&apdu[0], max_apdu_len,
                    &value->type.Weekly_Schedule);
                break;
            case BACNET_APPLICATION_TAG_SPECIAL_EVENT: /* BACnetSpecialEvent */
                apdu_len =
                    encode_special_event(&apdu[0], max_apdu_len,
                    &value->type.Special_Event);
                break;
            case BACNET_APPLICATION_TAG_DESTINATION:   /* BACnetDestination (Recipient_List) */
                apdu_len =
                    encode_destination(&apdu[0], max_apdu_len,
                    &value->type.Destination);
                break;
#endif

            default:
                break;
        }
    }

    return apdu_len;
}

/* decode the data and store it into value.
   Return the number of octets consumed. */
int bacapp_decode_data(
    uint8_t * apdu,
    uint8_t tag_data_type,
    uint32_t len_value_type,
    BACNET_APPLICATION_DATA_VALUE * value)
{
    int len = 0;

    if (apdu && value) {
        switch (tag_data_type) {
#if defined (BACAPP_NULL)
            case BACNET_APPLICATION_TAG_NULL:
                /* nothing else to do */
                break;
#endif
#if defined (BACAPP_BOOLEAN)
            case BACNET_APPLICATION_TAG_BOOLEAN:
                value->type.Boolean = decode_boolean(len_value_type);
                break;
#endif
#if defined (BACAPP_UNSIGNED)
            case BACNET_APPLICATION_TAG_UNSIGNED_INT:
                len =
                    decode_unsigned(&apdu[0], len_value_type,
                    &value->type.Unsigned_Int);
                break;
#endif
#if defined (BACAPP_SIGNED)
            case BACNET_APPLICATION_TAG_SIGNED_INT:
                len =
                    decode_signed(&apdu[0], len_value_type,
                    &value->type.Signed_Int);
                break;
#endif
#if defined (BACAPP_REAL)
            case BACNET_APPLICATION_TAG_REAL:
                len =
                    decode_real_safe(&apdu[0], len_value_type,
                    &(value->type.Real));
                break;
#endif
#if defined (BACAPP_DOUBLE)
            case BACNET_APPLICATION_TAG_DOUBLE:
                len =
                    decode_double_safe(&apdu[0], len_value_type,
                    &(value->type.Double));
                break;
#endif
#if defined (BACAPP_OCTET_STRING)
            case BACNET_APPLICATION_TAG_OCTET_STRING:
                len =
                    decode_octet_string(&apdu[0], len_value_type,
                    &value->type.Octet_String);
                break;
#endif
#if defined (BACAPP_CHARACTER_STRING)
            case BACNET_APPLICATION_TAG_CHARACTER_STRING:
                len =
                    decode_character_string(&apdu[0], len_value_type,
                    &value->type.Character_String);
                break;
#endif
#if defined (BACAPP_BIT_STRING)
            case BACNET_APPLICATION_TAG_BIT_STRING:
                len =
                    decode_bitstring(&apdu[0], len_value_type,
                    &value->type.Bit_String);
                break;
#endif
#if defined (BACAPP_ENUMERATED)
            case BACNET_APPLICATION_TAG_ENUMERATED:
                len =
                    decode_enumerated(&apdu[0], len_value_type,
                    &value->type.Enumerated);
                break;
#endif
#if defined (BACAPP_DATE)
            case BACNET_APPLICATION_TAG_DATE:
                len =
                    decode_date_safe(&apdu[0], len_value_type,
                    &value->type.Date);
                break;
#endif
#if defined (BACAPP_TIME)
            case BACNET_APPLICATION_TAG_TIME:
                len =
                    decode_bacnet_time_safe(&apdu[0], len_value_type,
                    &value->type.Time);
                break;
#endif
#if defined (BACAPP_OBJECT_ID)
            case BACNET_APPLICATION_TAG_OBJECT_ID:
                {
                    uint16_t object_type = 0;
                    uint32_t instance = 0;
                    len =
                        decode_object_id_safe(&apdu[0], len_value_type,
                        &object_type, &instance);
                    value->type.Object_Id.type = object_type;
                    value->type.Object_Id.instance = instance;
                }
                break;
#endif
            default:
                break;
        }
    }

    if (len == 0 && tag_data_type != BACNET_APPLICATION_TAG_NULL &&
        tag_data_type != BACNET_APPLICATION_TAG_BOOLEAN) {
        value->tag = MAX_BACNET_APPLICATION_TAG;
    }
    return len;
}

int bacapp_decode_application_data(
    uint8_t * apdu,
    unsigned max_apdu_len,
    BACNET_APPLICATION_DATA_VALUE * value)
{
    int len = 0;
    int tag_len = 0;
    uint8_t tag_number = 0;
    uint32_t len_value_type = 0;

    /* FIXME: use max_apdu_len! */
    max_apdu_len = max_apdu_len;
    if (apdu && value && !IS_CONTEXT_SPECIFIC(*apdu)) {
        value->context_specific = false;
        tag_len =
            decode_tag_number_and_value(&apdu[0], &tag_number,
            &len_value_type);
        if (tag_len) {
            len += tag_len;
            value->tag = tag_number;
            len +=
                bacapp_decode_data(&apdu[len], tag_number, len_value_type,
                value);
        }
        value->next = NULL;
    }

    return len;
}

/*
** Usage: Similar to strtok. Call function the first time with new_apdu and new_adu_len set to apdu buffer
** to be processed. Subsequent calls should pass in NULL.
**
** Returns true if a application message is correctly parsed.
** Returns false if no more application messages are available.
**
** This function is NOT thread safe.
**
** Notes: The _safe suffix is there because the function should be relatively safe against buffer overruns.
**
*/

bool bacapp_decode_application_data_safe(
    uint8_t * new_apdu,
    uint32_t new_apdu_len,
    BACNET_APPLICATION_DATA_VALUE * value)
{
    /* The static variables that store the apdu buffer between function calls */
    static uint8_t *apdu = NULL;
    static int32_t apdu_len_remaining = 0;
    static uint32_t apdu_len = 0;
    int len = 0;
    int tag_len = 0;
    uint8_t tag_number = 0;
    uint32_t len_value_type = 0;

    bool ret = false;

    if (new_apdu != NULL) {
        apdu = new_apdu;
        apdu_len_remaining = new_apdu_len;
        apdu_len = 0;
    }

    if (value && apdu_len_remaining > 0 &&
        !IS_CONTEXT_SPECIFIC(apdu[apdu_len])) {
        value->context_specific = false;
        tag_len =
            decode_tag_number_and_value_safe(&apdu[apdu_len],
            apdu_len_remaining, &tag_number, &len_value_type);
        /* If tag_len is zero, then the tag information is truncated */
        if (tag_len) {
            apdu_len += tag_len;
            apdu_len_remaining -= tag_len;
            /* The tag is boolean then len_value_type is interpreted as value, not length, so dont bother
             ** checking with apdu_len_remaining */
            if (tag_number == BACNET_APPLICATION_TAG_BOOLEAN ||
                (int32_t) len_value_type <= apdu_len_remaining) {
                value->tag = tag_number;
                len =
                    bacapp_decode_data(&apdu[apdu_len], tag_number,
                    len_value_type, value);
                apdu_len += len;
                apdu_len_remaining -= len;

                ret = true;
            }
        }
        value->next = NULL;
    }


    return ret;
}


int bacapp_encode_context_data_value(
    uint8_t * apdu,
    uint8_t context_tag_number,
    BACNET_APPLICATION_DATA_VALUE * value)
{
    int apdu_len = 0;   /* total length of the apdu, return value */

    if (value && apdu) {
        switch (value->tag) {
#if defined (BACAPP_NULL)
            case BACNET_APPLICATION_TAG_NULL:
                apdu_len = encode_context_null(&apdu[0], context_tag_number);
                break;
#endif
#if defined (BACAPP_BOOLEAN)
            case BACNET_APPLICATION_TAG_BOOLEAN:
                apdu_len =
                    encode_context_boolean(&apdu[0], context_tag_number,
                    value->type.Boolean);
                break;
#endif
#if defined (BACAPP_UNSIGNED)
            case BACNET_APPLICATION_TAG_UNSIGNED_INT:
                apdu_len =
                    encode_context_unsigned(&apdu[0], context_tag_number,
                    value->type.Unsigned_Int);
                break;
#endif
#if defined (BACAPP_SIGNED)
            case BACNET_APPLICATION_TAG_SIGNED_INT:
                apdu_len =
                    encode_context_signed(&apdu[0], context_tag_number,
                    value->type.Signed_Int);
                break;
#endif
#if defined (BACAPP_REAL)
            case BACNET_APPLICATION_TAG_REAL:
                apdu_len =
                    encode_context_real(&apdu[0], context_tag_number,
                    value->type.Real);
                break;
#endif
#if defined (BACAPP_DOUBLE)
            case BACNET_APPLICATION_TAG_DOUBLE:
                apdu_len =
                    encode_context_double(&apdu[0], context_tag_number,
                    value->type.Double);
                break;
#endif
#if defined (BACAPP_OCTET_STRING)
            case BACNET_APPLICATION_TAG_OCTET_STRING:
                apdu_len =
                    encode_context_octet_string(&apdu[0], context_tag_number,
                    &value->type.Octet_String);
                break;
#endif
#if defined (BACAPP_CHARACTER_STRING)
            case BACNET_APPLICATION_TAG_CHARACTER_STRING:
                apdu_len =
                    encode_context_character_string(&apdu[0],
                    context_tag_number, &value->type.Character_String);
                break;
#endif
#if defined (BACAPP_BIT_STRING)
            case BACNET_APPLICATION_TAG_BIT_STRING:
                apdu_len =
                    encode_context_bitstring(&apdu[0], context_tag_number,
                    &value->type.Bit_String);
                break;
#endif
#if defined (BACAPP_ENUMERATED)
            case BACNET_APPLICATION_TAG_ENUMERATED:
                apdu_len =
                    encode_context_enumerated(&apdu[0], context_tag_number,
                    value->type.Enumerated);
                break;
#endif
#if defined (BACAPP_DATE)
            case BACNET_APPLICATION_TAG_DATE:
                apdu_len =
                    encode_context_date(&apdu[0], context_tag_number,
                    &value->type.Date);
                break;
#endif
#if defined (BACAPP_TIME)
            case BACNET_APPLICATION_TAG_TIME:
                apdu_len =
                    encode_context_time(&apdu[0], context_tag_number,
                    &value->type.Time);
                break;
#endif
#if defined (BACAPP_OBJECT_ID)
            case BACNET_APPLICATION_TAG_OBJECT_ID:
                apdu_len =
                    encode_context_object_id(&apdu[0], context_tag_number,
                    (int) value->type.Object_Id.type,
                    value->type.Object_Id.instance);
                break;
#endif
            default:
                break;
        }
    }

    return apdu_len;
}

/* returns the fixed tag type for certain context tagged properties */
BACNET_APPLICATION_TAG bacapp_context_tag_type(
    BACNET_PROPERTY_ID property,
    uint8_t tag_number)
{
    BACNET_APPLICATION_TAG tag = MAX_BACNET_APPLICATION_TAG;

    switch (property) {
            /* ----------------------------------------------- */
        case PROP_DATE_LIST:
            switch (tag_number) {
                case 0:        /* single calendar date */
                    tag = BACNET_APPLICATION_TAG_DATE;
                    break;
                case 1:        /* range of dates */
                    tag = BACNET_APPLICATION_TAG_DATERANGE;
                    break;
                case 2:        /* selection of weeks, month, and day of month */
                    /*tag = BACNET_APPLICATION_TAG_WEEKNDAY; */
                    break;
                default:
                    break;
            }
            break;
        case PROP_ACTUAL_SHED_LEVEL:
        case PROP_REQUESTED_SHED_LEVEL:
        case PROP_EXPECTED_SHED_LEVEL:
            switch (tag_number) {
                case 0:
                case 1:
                    tag = BACNET_APPLICATION_TAG_UNSIGNED_INT;
                    break;
                case 2:
                    tag = BACNET_APPLICATION_TAG_REAL;
                    break;
                default:
                    break;
            }
            break;
        case PROP_ACTION:
            switch (tag_number) {
                case 0:
                case 1:
                    tag = BACNET_APPLICATION_TAG_OBJECT_ID;
                    break;
                case 2:
                    tag = BACNET_APPLICATION_TAG_ENUMERATED;
                    break;
                case 3:
                case 5:
                case 6:
                    tag = BACNET_APPLICATION_TAG_UNSIGNED_INT;
                    break;
                case 7:
                case 8:
                    tag = BACNET_APPLICATION_TAG_BOOLEAN;
                    break;
                case 4:        /* propertyValue: abstract syntax */
                default:
                    break;
            }
            break;
        case PROP_LIST_OF_GROUP_MEMBERS:
            switch (tag_number) {
                case 0:
                    tag = BACNET_APPLICATION_TAG_OBJECT_ID;
                    break;
                default:
                    break;
            }
            break;

        case PROP_EXCEPTION_SCHEDULE:
            switch (tag_number) {
                case 1:
                    tag = BACNET_APPLICATION_TAG_OBJECT_ID;
                    break;
                case 3:
                    tag = BACNET_APPLICATION_TAG_UNSIGNED_INT;
                    break;
                case 0:        /* calendarEntry: abstract syntax + context */
                case 2:        /* list of BACnetTimeValue: abstract syntax */
                default:
                    break;
            }
            break;
        case PROP_LOG_DEVICE_OBJECT_PROPERTY:
            switch (tag_number) {
                case 0:        /* Object ID */
                case 3:        /* Device ID */
                    tag = BACNET_APPLICATION_TAG_OBJECT_ID;
                    break;
                case 1:        /* Property ID */
                    tag = BACNET_APPLICATION_TAG_ENUMERATED;
                    break;
                case 2:        /* Array index */
                    tag = BACNET_APPLICATION_TAG_UNSIGNED_INT;
                    break;
                default:
                    break;
            }
            break;
        case PROP_SUBORDINATE_LIST:
            /* BACnetARRAY[N] of BACnetDeviceObjectReference */
            switch (tag_number) {
                case 0:        /* Optional Device ID */
                case 1:        /* Object ID */
                    tag = BACNET_APPLICATION_TAG_OBJECT_ID;
                    break;
                default:
                    break;
            }
            break;

        default:
            break;
    }

    return tag;
}

int bacapp_encode_context_data(
    uint8_t * apdu,
    BACNET_APPLICATION_DATA_VALUE * value,
    BACNET_PROPERTY_ID property)
{
    int apdu_len = 0;
    BACNET_APPLICATION_TAG tag_data_type;

    if (value && apdu) {
        tag_data_type = bacapp_context_tag_type(property, value->context_tag);
        if (tag_data_type < MAX_BACNET_APPLICATION_TAG) {
            apdu_len =
                bacapp_encode_context_data_value(&apdu[0], value->context_tag,
                value);
        } else {
            /* FIXME: what now? */
            apdu_len = 0;
        }
        value->next = NULL;
    }

    return apdu_len;
}

/*/ Uses of the short-app-value type (limited 32 bits yet) */
void copy_short_app_to_app_value(
    BACNET_APPLICATION_DATA_VALUE * valuedest,
    BACNET_SHORT_APPLICATION_DATA_VALUE * shortvaluesrc)
{
    memset(valuedest, 0, sizeof(BACNET_APPLICATION_DATA_VALUE));
    valuedest->tag = shortvaluesrc->tag;
    /* 32-bit-copy */
    valuedest->type.Unsigned_Int = shortvaluesrc->type.Unsigned_Int;
}

/*/ Uses of the short-app-value type (limited 32 bits yet) */
void copy_app_to_short_app_value(
    BACNET_SHORT_APPLICATION_DATA_VALUE * shortvaluedest,
    BACNET_APPLICATION_DATA_VALUE * valuesrc)
{
    memset(shortvaluedest, 0, sizeof(BACNET_SHORT_APPLICATION_DATA_VALUE));
    shortvaluedest->tag = valuesrc->tag;
    /* 32-bit-copy */
    shortvaluedest->type.Unsigned_Int = valuesrc->type.Unsigned_Int;
}

/*/ Destroy values and also linked other values */
void bacapp_desallocate_values(
    BACNET_APPLICATION_DATA_VALUE * value)
{
    if (!value)
        return;
    if (value->next)
        bacapp_desallocate_values(value->next);
    free(value);
}

/* auto alloc-and-copy data */
static
BACNET_APPLICATION_DATA_VALUE *bacapp_allocate_new_value(
    bool * needed,
    BACNET_APPLICATION_DATA_VALUE * current,
    BACNET_APPLICATION_DATA_VALUE * temp_value)
{
    BACNET_APPLICATION_DATA_VALUE *nvalue;
    if (!*needed) {
        *needed = true;
        *current = *temp_value;
        nvalue = current;
    } else {
        /* Alloc new data block, link it to current block */
        nvalue = calloc(1, sizeof(BACNET_APPLICATION_DATA_VALUE));
        current->next = nvalue;
        *nvalue = *temp_value;
    }
    /* clear temporary data */
    memset(temp_value, 0, sizeof(BACNET_APPLICATION_DATA_VALUE));
    /* return * last * block */
    while (nvalue->next)
        nvalue = nvalue->next;
    return nvalue;
}

int bacapp_decode_context_data_complex(
    uint8_t * apdu,
    unsigned max_apdu_len,
    uint8_t tag_number,
    BACNET_APPLICATION_DATA_VALUE * value,
    BACNET_PROPERTY_ID prop)
{
    BACNET_APPLICATION_DATA_VALUE tmpvalue = { 0 };
    int len = 0;
    bool allocate_data = false;
    int apdu_len = 0;
    uint8_t inner_tag_number = 0;

#if 0
    FILE *f = fopen("log_decode_context_complex.txt", "a");
    fprintf(f, "CONTEXT DATA :\r\n");
    fprintf(f, "PROPERTY: %d, %s\r\n", prop, bactext_property_name(prop));
    fprintf(f, "TAG: %d\r\n", tag_number);
    fprintf(f,
        "DATA: %02x %02x %02x %02x %02x %02x %02x %02x   %02x %02x %02x %02x %02x %02x %02x %02x\r\n",
        apdu[0], apdu[1], apdu[2], apdu[3], apdu[4], apdu[5], apdu[6], apdu[7],
        apdu[8], apdu[9], apdu[10], apdu[11], apdu[12], apdu[13], apdu[14],
        apdu[15]);
    fclose(f);
#endif

    /* If it's closed : leave */
    while (!decode_is_closing_tag_number(&apdu[apdu_len], tag_number)) {
        /* Context ou pas ! */
        if (IS_CONTEXT_SPECIFIC(apdu[apdu_len])) {
            /* open a new tag area */
            if (decode_is_opening_tag(&apdu[apdu_len])) {
                /* decode new tag  */
                decode_tag_number(&apdu[apdu_len], &inner_tag_number);
                apdu_len++;
                /* Recurse into special structure */
                len =
                    bacapp_decode_context_data_complex(&apdu[apdu_len],
                    max_apdu_len - apdu_len, inner_tag_number, &tmpvalue,
                    prop);
                if (len >= 0) {
                    apdu_len += len;
                    value =
                        bacapp_allocate_new_value(&allocate_data, value,
                        &tmpvalue);
                } else
                    return -1;
                continue;
            } else {
                /* Decode : length/value/type */
                len =
                    bacapp_decode_context_data(&apdu[apdu_len],
                    max_apdu_len - apdu_len, &tmpvalue, prop);
                if (len > 0) {
                    apdu_len += len;
                    value =
                        bacapp_allocate_new_value(&allocate_data, value,
                        &tmpvalue);
                } else
                    return -1;
            }
        } else {
            /* Normal stuff */
            len =
                bacapp_decode_application_data(&apdu[apdu_len],
                max_apdu_len - apdu_len, &tmpvalue);
            if (len > 0) {
                apdu_len += len;
                value =
                    bacapp_allocate_new_value(&allocate_data, value,
                    &tmpvalue);
            } else
                return -1;
        }
    }
    apdu_len++; /* jump closing tag */

    return apdu_len;
}


int bacapp_decode_context_data(
    uint8_t * apdu,
    unsigned max_apdu_len,
    BACNET_APPLICATION_DATA_VALUE * value,
    BACNET_PROPERTY_ID property)
{
    int apdu_len = 0, len = 0;
    int tag_len = 0;
    uint8_t tag_number = 0;
    uint32_t len_value_type = 0;

    /* FIXME: use max_apdu_len! */
    max_apdu_len = max_apdu_len;
    if (apdu && value && IS_CONTEXT_SPECIFIC(*apdu)) {
        value->context_specific = true;
        value->next = NULL;
        tag_len =
            decode_tag_number_and_value(&apdu[0], &tag_number,
            &len_value_type);
        apdu_len = tag_len;
        /* Empty construct : (closing tag) => returns NULL value */
        if (tag_len && (unsigned int) tag_len <= max_apdu_len &&
            !decode_is_closing_tag_number(&apdu[0], tag_number)) {
            value->context_tag = tag_number;


            value->tag = bacapp_context_tag_type(property, tag_number);

            if (value->tag < MAX_BACNET_APPLICATION_TAG) {
                len =
                    bacapp_decode_data(&apdu[apdu_len], value->tag,
                    len_value_type, value);
                apdu_len += len;
            } else if (len_value_type) {
                /* Unknown value : non null size (elementary type) */
                apdu_len += len_value_type;
                /* SHOULD NOT HAPPEN, EXCEPTED WHEN READING UNKNOWN CONTEXTUAL PROPERTY */
            } else {
                /* FIXME: what now? */

                /* Unknown value : (constructed type) */
                /* SHOULD NOT HAPPEN, EXCEPTED WHEN READING UNKNOWN CONTEXTUAL PROPERTY */

                /* Decode more complex data */
                len =
                    bacapp_decode_context_data_complex(&apdu[apdu_len],
                    max_apdu_len - apdu_len, tag_number, value, property);
                if (len < 0)
                    return -1;
                apdu_len += len;
            }

        }
    }

    return apdu_len;
}

/*/ Generic property decoding */
int bacapp_decode_generic_property(
    uint8_t * apdu,
    unsigned max_apdu_len,
    BACNET_APPLICATION_DATA_VALUE * value,
    BACNET_PROPERTY_ID prop)
{
    int len = 0;
    if (IS_CONTEXT_SPECIFIC(*apdu)) {
        len = bacapp_decode_context_data(apdu, max_apdu_len, value, prop);
    } else {
        len = bacapp_decode_application_data(apdu, max_apdu_len, value);
    }
    return len;
}

/* decode one value of a priority array */
int decode_priority_value(
    uint8_t * apdu,
    unsigned max_apdu_len,
    BACNET_APPLICATION_DATA_VALUE * value,
    BACNET_PROPERTY_ID prop)
{
    int val_len = 0;
    uint32_t len_value_type = 0;
    int len = 0;
    bool is_opening_tag;
    uint8_t tag_number;

    if (decode_is_context_tag(apdu, 0) && !decode_is_closing_tag(apdu)) {
        /* Contextual Abstract-syntax & type */
        val_len =
            decode_tag_number_and_value(apdu, &tag_number, &len_value_type);
        is_opening_tag = decode_is_opening_tag(apdu);
        len += val_len;
        val_len =
            bacapp_decode_generic_property(&apdu[len], max_apdu_len - len,
            value, prop);
        if (val_len < 0)
            return -1;
        len += val_len;
        if (is_opening_tag) {
            if (!decode_is_closing_tag_number(apdu, 0))
                return -1;
            len++;
        }
        return len;
    } /* else generic decode */
    else
        return bacapp_decode_generic_property(apdu, max_apdu_len, value, prop);
}

/*/ Decodes a well-known, possibly complex property value */
/* reverse operations in bacapp_encode_application_data */
int bacapp_decode_known_property(
    uint8_t * apdu,
    unsigned max_apdu_len,
    BACNET_APPLICATION_DATA_VALUE * value,
    BACNET_PROPERTY_ID prop)
{
    int len = 0;

    switch (prop) {
            /* Properties using : BACnetDeviceObjectReference (deviceid, objectid) */
        case PROP_MEMBER_OF:
        case PROP_ZONE_MEMBERS:
        case PROP_DOOR_MEMBERS:
        case PROP_SUBORDINATE_LIST:
        case PROP_ACCESS_EVENT_CREDENTIAL:
        case PROP_ACCESS_DOORS:
        case PROP_ZONE_FROM:
        case PROP_ZONE_TO:
        case PROP_CREDENTIALS_IN_ZONE:
        case PROP_LAST_CREDENTIAL_ADDED:
        case PROP_LAST_CREDENTIAL_REMOVED:
        case PROP_ENTRY_POINTS:
        case PROP_EXIT_POINTS:
        case PROP_MEMBERS:
        case PROP_CREDENTIALS:
        case PROP_ACCOMPANIED:
        case PROP_BELONGS_TO:
        case PROP_LAST_ACCESS_POINT:
            value->tag = BACNET_APPLICATION_TAG_DEVICE_OBJECT_REFERENCE;
            if ((len =
                    bacapp_decode_device_obj_ref(apdu,
                        &(value->type.Device_Object_Reference))) > 0)
                return len;
            break;

            /* Properties using : BACnetDateTime */
        case PROP_TIME_OF_ACTIVE_TIME_RESET:
        case PROP_TIME_OF_STATE_COUNT_RESET:
        case PROP_CHANGE_OF_STATE_TIME:
        case PROP_MAXIMUM_VALUE_TIMESTAMP:
        case PROP_MINIMUM_VALUE_TIMESTAMP:
        case PROP_VALUE_CHANGE_TIME:
        case PROP_START_TIME:
        case PROP_STOP_TIME:
        case PROP_MODIFICATION_DATE:
        case PROP_UPDATE_TIME:
        case PROP_COUNT_CHANGE_TIME:
        case PROP_LAST_CREDENTIAL_ADDED_TIME:
        case PROP_LAST_CREDENTIAL_REMOVED_TIME:
        case PROP_ACTIVATION_TIME:
        case PROP_EXPIRY_TIME:
        case PROP_LAST_USE_TIME:
            /* decode a simple BACnetDateTime value */
            value->tag = BACNET_APPLICATION_TAG_DATETIME;
            if ((len =
                    decode_application_datetime(apdu,
                        &value->type.Date_Time)) > 0)
                return len;
            break;
            /* Properties using : BACnetDeviceObjectPropertyReference */
        case PROP_OBJECT_PROPERTY_REFERENCE:
        case PROP_LOG_DEVICE_OBJECT_PROPERTY:
        case PROP_LIST_OF_OBJECT_PROPERTY_REFERENCES:
            value->tag =
                BACNET_APPLICATION_TAG_DEVICE_OBJECT_PROPERTY_REFERENCE;
            if ((len =
                    bacapp_decode_device_obj_property_ref(apdu,
                        &(value->type.Device_Object_Property_Reference))) > 0)
                return len;
            break;
            /* Properties using : BACnetObjectPropertyReference */
            /* see also : BACnetSetpointReference */
        case PROP_MANIPULATED_VARIABLE_REFERENCE:
        case PROP_CONTROLLED_VARIABLE_REFERENCE:
        case PROP_INPUT_REFERENCE:
            /*case PROP_SETPOINT_REFERENCE: */
            value->tag = BACNET_APPLICATION_TAG_OBJECT_PROPERTY_REFERENCE;
            if ((len =
                    bacapp_decode_obj_property_ref(apdu,
                        &value->type.Object_Property_Reference)) > 0)
                return len;
            break;
            /* Properties using : BACnetRecipient */
        case PROP_TIME_SYNCHRONIZATION_RECIPIENTS:
        case PROP_RESTART_NOTIFICATION_RECIPIENTS:
        case PROP_UTC_TIME_SYNCHRONIZATION_RECIPIENTS:
            value->tag = BACNET_APPLICATION_TAG_RECIPIENT;
            if ((len = decode_recipient(apdu, &value->type.Recipient)) > 0)
                return len;
            break;
            /* Properties using : BACnetDestination (Notification class) */
        case PROP_RECIPIENT_LIST:
            value->tag = BACNET_APPLICATION_TAG_DESTINATION;
            if ((len =
                    decode_destination(apdu, max_apdu_len,
                        &value->type.Destination)) > 0)
                return len;
            break;
            /* Properties using : BACnetDateRange  (Schedule) */
        case PROP_EFFECTIVE_PERIOD:
            value->tag = BACNET_APPLICATION_TAG_DATERANGE;
            if ((len =
                    decode_daterange(&apdu[0], &value->type.Date_Range)) > 0)
                return len;
            break;
            /* Properties using : BACnetTimeStamp */
        case PROP_EVENT_TIME_STAMPS:
        case PROP_LAST_RESTORE_TIME:
        case PROP_TIME_OF_DEVICE_RESTART:
        case PROP_ACCESS_EVENT_TIME:
            value->tag = BACNET_APPLICATION_TAG_TIMESTAMP;
            if ((len =
                    bacapp_decode_timestamp(apdu, &value->type.TimeStamp)) > 0)
                return len;
            break;
            /* BACnetCOVSubscription */
        case PROP_ACTIVE_COV_SUBSCRIPTIONS:
            value->tag = BACNET_APPLICATION_TAG_COV_SUBSCRIPTION;
            if ((len =
                    decode_cov_subscription(apdu, max_apdu_len,
                        &value->type.COV_Subscription)) > 0)
                return len;
            break;
            /* Properties using : BACnetCalendarEntry */
        case PROP_DATE_LIST:
            value->tag = BACNET_APPLICATION_TAG_CALENDAR_ENTRY;
            if ((len =
                    decode_calendar_entry(apdu, max_apdu_len,
                        &value->type.Calendar_Entry)) > 0)
                return len;
            break;
            /* [16] BACnetPriorityValue : 16x values (simple property) */
        case PROP_PRIORITY_ARRAY:
            if ((len =
                    decode_priority_value(apdu, max_apdu_len, value,
                        prop)) > 0)
                return len;
            break;
            /* BACnetDailySchedule[7] (Schedule) */
        case PROP_WEEKLY_SCHEDULE:
            value->tag = BACNET_APPLICATION_TAG_WEEKLY_SCHEDULE;
            if ((len =
                    decode_weekly_schedule(&apdu[0], max_apdu_len,
                        &value->type.Weekly_Schedule)) > 0)
                return len;
            break;
            /* BACnetSpecialEvent (Schedule) */
        case PROP_EXCEPTION_SCHEDULE:
            value->tag = BACNET_APPLICATION_TAG_SPECIAL_EVENT;
            if ((len =
                    decode_special_event(&apdu[0], max_apdu_len,
                        &value->type.Special_Event)) > 0)
                return len;
            break;

            /* Properties using : ReadAccessSpecification */
        case PROP_LIST_OF_GROUP_MEMBERS:
            /* XXX - TODO */

            /* BACnetAddressBinding */
        case PROP_DEVICE_ADDRESS_BINDING:
        case PROP_MANUAL_SLAVE_ADDRESS_BINDING:
        case PROP_SLAVE_ADDRESS_BINDING:
            /* XXX - TODO */

            /* Property action (Command object) */
            /* BACnetActionList ::= SEQUENCE{  */
            /*    action [0] SEQUENCE OF BACnetActionCommand  */
            /* BACnetActionCommand ::= SEQUENCE {  */
            /*    deviceIdentifier  [0] BACnetObjectIdentifier OPTIONAL,  */
            /*    objectIdentifier  [1] BACnetObjectIdentifier,  */
            /*    propertyIdentifier  [2] BACnetPropertyIdentifier,  */
            /*    propertyArrayIndex [3] Unsigned OPTIONAL,   --used only with array datatype  */
            /*    propertyValue  [4] ABSTRACT-SYNTAX.&Type,  */
            /*    priority   [5] Unsigned (1..16) OPTIONAL, --used only when property is commandable  */
            /*    postDelay  [6] Unsigned OPTIONAL,  */
            /*    quitOnFailure  [7] BOOLEAN,  */
            /*    writeSuccessful  [8] BOOLEAN  */
        case PROP_ACTION:
            /* XXX - TODO */

        default:
            break;
    }
    /* Decode a "classic" simple property */
    return bacapp_decode_generic_property(apdu, max_apdu_len, value, prop);
}

/*/ Loop through many well-known values */
int bacapp_decode_known_property_until_tag_or_end(
    uint8_t * apdu,
    int max_apdu_len,
    BACNET_APPLICATION_DATA_VALUE * value,
    BACNET_PROPERTY_ID prop,
    uint8_t closing_tag_number,
    bool loop_until_tag)
{
    int len = 0;
    int apdu_len = 0;
    BACNET_APPLICATION_DATA_VALUE *firstvalue = value;
    BACNET_APPLICATION_DATA_VALUE tmpvalue = { 0 };
    bool allocate_data = false;
    /* defaults to an empty list (when we exit without reading any value) */
    value->tag = BACNET_APPLICATION_TAG_EMPTYLIST;

    while (apdu_len < max_apdu_len) {
        /* seen our closing tag ? (don't count it and return) */
        if (loop_until_tag &&
            decode_is_closing_tag_number(&apdu[apdu_len], closing_tag_number))
            break;
        /* read an element */
        len =
            bacapp_decode_known_property(&apdu[apdu_len],
            max_apdu_len - apdu_len, &tmpvalue, prop);
        if (len <= 0) {
            /* desallocate tmp */
            bacapp_desallocate_values(tmpvalue.next);
            /* desallocate previous data */
            bacapp_desallocate_values(firstvalue->next);
            firstvalue->next = NULL;
            return -1;
        }
        apdu_len += len;
        /* allocate new element */
        value = bacapp_allocate_new_value(&allocate_data, value, &tmpvalue);
    }

    /* Okay iff we see the closing tag */
    if (loop_until_tag)
        if (decode_is_closing_tag_number(&apdu[apdu_len], closing_tag_number))
            return apdu_len;
    /* Okay iff we see the end */
    if (!loop_until_tag)
        if (apdu_len >= max_apdu_len)
            return apdu_len;
    /* destroy allocated extra memory */
    bacapp_desallocate_values(firstvalue->next);
    firstvalue->next = NULL;
    return -1;
}

/*/ Loop through many well-known values */
int bacapp_decode_known_property_until_tag(
    uint8_t * apdu,
    int max_apdu_len,
    BACNET_APPLICATION_DATA_VALUE * value,
    BACNET_PROPERTY_ID prop,
    uint8_t closing_tag_number)
{
    return bacapp_decode_known_property_until_tag_or_end(apdu, max_apdu_len,
        value, prop, closing_tag_number, true);
}

/*/ Loop through many well-known values */
int bacapp_decode_known_property_until_end(
    uint8_t * apdu,
    int max_apdu_len,
    BACNET_APPLICATION_DATA_VALUE * value,
    BACNET_PROPERTY_ID prop)
{
    return bacapp_decode_known_property_until_tag_or_end(apdu, max_apdu_len,
        value, prop, 0, false);
}

int bacapp_encode_data(
    uint8_t * apdu,
    int max_apdu_len,
    BACNET_APPLICATION_DATA_VALUE * value)
{
    int apdu_len = 0;   /* total length of the apdu, return value */

    if (value && apdu) {
        if (value->context_specific) {
            apdu_len =
                bacapp_encode_context_data_value(&apdu[0], value->context_tag,
                value);
        } else {
            apdu_len =
                bacapp_encode_application_data(&apdu[0], max_apdu_len, value);
        }
    }

    return apdu_len;
}


bool bacapp_copy(
    BACNET_APPLICATION_DATA_VALUE * dest_value,
    BACNET_APPLICATION_DATA_VALUE * src_value)
{
    bool status = true; /*return value */

    if (dest_value && src_value) {
        dest_value->tag = src_value->tag;
        switch (src_value->tag) {
#if defined (BACAPP_NULL)
            case BACNET_APPLICATION_TAG_NULL:
                break;
#endif
#if defined (BACAPP_BOOLEAN)
            case BACNET_APPLICATION_TAG_BOOLEAN:
                dest_value->type.Boolean = src_value->type.Boolean;
                break;
#endif
#if defined (BACAPP_UNSIGNED)
            case BACNET_APPLICATION_TAG_UNSIGNED_INT:
                dest_value->type.Unsigned_Int = src_value->type.Unsigned_Int;
                break;
#endif
#if defined (BACAPP_SIGNED)
            case BACNET_APPLICATION_TAG_SIGNED_INT:
                dest_value->type.Signed_Int = src_value->type.Signed_Int;
                break;
#endif
#if defined (BACAPP_REAL)
            case BACNET_APPLICATION_TAG_REAL:
                dest_value->type.Real = src_value->type.Real;
                break;
#endif
#if defined (BACAPP_DOUBLE)
            case BACNET_APPLICATION_TAG_DOUBLE:
                dest_value->type.Double = src_value->type.Double;
                break;
#endif
#if defined (BACAPP_OCTET_STRING)
            case BACNET_APPLICATION_TAG_OCTET_STRING:
                octetstring_copy(&dest_value->type.Octet_String,
                    &src_value->type.Octet_String);
                break;
#endif
#if defined (BACAPP_CHARACTER_STRING)
            case BACNET_APPLICATION_TAG_CHARACTER_STRING:
                characterstring_copy(&dest_value->type.Character_String,
                    &src_value->type.Character_String);
                break;
#endif
#if defined (BACAPP_BIT_STRING)
            case BACNET_APPLICATION_TAG_BIT_STRING:
                bitstring_copy(&dest_value->type.Bit_String,
                    &src_value->type.Bit_String);
                break;
#endif
#if defined (BACAPP_ENUMERATED)
            case BACNET_APPLICATION_TAG_ENUMERATED:
                dest_value->type.Enumerated = src_value->type.Enumerated;
                break;
#endif
#if defined (BACAPP_DATE)
            case BACNET_APPLICATION_TAG_DATE:
                datetime_copy_date(&dest_value->type.Date,
                    &src_value->type.Date);
                break;
#endif
#if defined (BACAPP_TIME)
            case BACNET_APPLICATION_TAG_TIME:
                datetime_copy_time(&dest_value->type.Time,
                    &src_value->type.Time);
                break;
#endif
#if defined (BACAPP_OBJECT_ID)
            case BACNET_APPLICATION_TAG_OBJECT_ID:
                dest_value->type.Object_Id.type =
                    src_value->type.Object_Id.type;
                dest_value->type.Object_Id.instance =
                    src_value->type.Object_Id.instance;
                break;
#endif
            default:
                status = false;
                break;
        }
        dest_value->next = src_value->next;
    }

    return status;
}

/* returns the length of data between an opening tag and a closing tag.
   Expects that the first octet contain the opening tag.
   Include a value property identifier for context specific data
   such as the value received in a WriteProperty request */
int bacapp_data_len(
    uint8_t * apdu,
    unsigned max_apdu_len,
    BACNET_PROPERTY_ID property)
{
    int len = 0;
    int total_len = 0;
    int apdu_len = 0;
    uint8_t tag_number = 0;
    uint8_t opening_tag_number = 0;
    uint8_t opening_tag_number_counter = 0;
    uint32_t value = 0;
    BACNET_APPLICATION_DATA_VALUE application_value;

    if (IS_OPENING_TAG(apdu[0])) {
        len =
            decode_tag_number_and_value(&apdu[apdu_len], &tag_number, &value);
        apdu_len += len;
        opening_tag_number = tag_number;
        opening_tag_number_counter = 1;
        while (opening_tag_number_counter) {
            if (IS_OPENING_TAG(apdu[apdu_len])) {
                len =
                    decode_tag_number_and_value(&apdu[apdu_len], &tag_number,
                    &value);
                if (tag_number == opening_tag_number)
                    opening_tag_number_counter++;
            } else if (IS_CLOSING_TAG(apdu[apdu_len])) {
                len =
                    decode_tag_number_and_value(&apdu[apdu_len], &tag_number,
                    &value);
                if (tag_number == opening_tag_number)
                    opening_tag_number_counter--;
            } else if (IS_CONTEXT_SPECIFIC(apdu[apdu_len])) {
                /* context-specific tagged data */
                len =
                    bacapp_decode_context_data(&apdu[apdu_len],
                    max_apdu_len - apdu_len, &application_value, property);
            } else {
                /* application tagged data */
                len =
                    bacapp_decode_application_data(&apdu[apdu_len],
                    max_apdu_len - apdu_len, &application_value);
            }
            apdu_len += len;
            if (opening_tag_number_counter) {
                if (len > 0) {
                    total_len += len;
                } else {
                    /* error: len is not incrementing */
                    total_len = -1;
                    break;
                }
            }
            if ((unsigned) apdu_len > max_apdu_len) {
                /* error: exceeding our buffer limit */
                total_len = -1;
                break;
            }
        }
    }

    return total_len;
}

#ifdef BACAPP_PRINT_ENABLED
bool bacapp_print_value(
    FILE * stream,
    BACNET_APPLICATION_DATA_VALUE * value,
    BACNET_PROPERTY_ID property)
{
    bool status = true; /*return value */
    size_t len = 0, i = 0;
    char *char_str;
    uint8_t *octet_str;

    if (value) {
        switch (value->tag) {
            case BACNET_APPLICATION_TAG_NULL:
                fprintf(stream, "Null");
                break;
            case BACNET_APPLICATION_TAG_BOOLEAN:
                fprintf(stream, "%s", value->type.Boolean ? "TRUE" : "FALSE");
                break;
            case BACNET_APPLICATION_TAG_UNSIGNED_INT:
                fprintf(stream, "%lu",
                    (unsigned long) value->type.Unsigned_Int);
                break;
            case BACNET_APPLICATION_TAG_SIGNED_INT:
                fprintf(stream, "%ld", (long) value->type.Signed_Int);
                break;
            case BACNET_APPLICATION_TAG_REAL:
                fprintf(stream, "%f", (double) value->type.Real);
                break;
#if defined (BACAPP_DOUBLE)
            case BACNET_APPLICATION_TAG_DOUBLE:
                fprintf(stream, "%f", value->type.Double);
                break;
#endif
            case BACNET_APPLICATION_TAG_OCTET_STRING:
                len = octetstring_length(&value->type.Octet_String);
                octet_str = octetstring_value(&value->type.Octet_String);
                for (i = 0; i < len; i++) {
                    fprintf(stream, "%02X", *octet_str);
                    octet_str++;
                }
                break;
            case BACNET_APPLICATION_TAG_CHARACTER_STRING:
                len = characterstring_length(&value->type.Character_String);
                char_str =
                    characterstring_value(&value->type.Character_String);
                fprintf(stream, "\"");
                for (i = 0; i < len; i++) {
                    if (isprint(*((unsigned char *) char_str))) {
                        fprintf(stream, "%c", *char_str);
                    } else {
                        fprintf(stream, ".");
                    }
                    char_str++;
                }
                fprintf(stream, "\"");
                break;
            case BACNET_APPLICATION_TAG_BIT_STRING:
                len = bitstring_bits_used(&value->type.Bit_String);
                fprintf(stream, "{");
                for (i = 0; i < len; i++) {
                    fprintf(stream, "%s",
                        bitstring_bit(&value->type.Bit_String,
                            (uint8_t) i) ? "true" : "false");
                    if (i < len - 1)
                        fprintf(stream, ",");
                }
                fprintf(stream, "}");
                break;
            case BACNET_APPLICATION_TAG_ENUMERATED:
                switch (property) {
                    case PROP_OBJECT_TYPE:
                        if (value->type.Enumerated < MAX_ASHRAE_OBJECT_TYPE) {
                            fprintf(stream, "%s",
                                bactext_object_type_name(value->type.
                                    Enumerated));
                        } else if (value->type.Enumerated < 128) {
                            fprintf(stream, "reserved %lu",
                                (unsigned long) value->type.Enumerated);
                        } else {
                            fprintf(stream, "proprietary %lu",
                                (unsigned long) value->type.Enumerated);
                        }
                        break;
                    case PROP_EVENT_STATE:
                        fprintf(stream, "%s",
                            bactext_event_state_name(value->type.Enumerated));
                        break;
                    case PROP_UNITS:
                        if (value->type.Enumerated < 256) {
                            fprintf(stream, "%s",
                                bactext_engineering_unit_name(value->
                                    type.Enumerated));
                        } else {
                            fprintf(stream, "proprietary %lu",
                                (unsigned long) value->type.Enumerated);
                        }
                        break;
                    case PROP_POLARITY:
                        fprintf(stream, "%s",
                            bactext_binary_polarity_name(value->
                                type.Enumerated));
                        break;
                    case PROP_PRESENT_VALUE:
                        fprintf(stream, "%s",
                            bactext_binary_present_value_name(value->
                                type.Enumerated));
                        break;
                    case PROP_RELIABILITY:
                        fprintf(stream, "%s",
                            bactext_reliability_name(value->type.Enumerated));
                        break;
                    case PROP_SYSTEM_STATUS:
                        fprintf(stream, "%s",
                            bactext_device_status_name(value->
                                type.Enumerated));
                        break;
                    case PROP_SEGMENTATION_SUPPORTED:
                        fprintf(stream, "%s",
                            bactext_segmentation_name(value->type.Enumerated));
                        break;
                    case PROP_NODE_TYPE:
                        fprintf(stream, "%s",
                            bactext_node_type_name(value->type.Enumerated));
                        break;
                    default:
                        fprintf(stream, "%lu",
                            (unsigned long) value->type.Enumerated);
                        break;
                }
                break;
            case BACNET_APPLICATION_TAG_DATE:
                fprintf(stream, "%s, %s ",
                    bactext_day_of_week_name(value->type.Date.wday),
                    bactext_month_name(value->type.Date.month));
                if (value->type.Date.day == 255) {
                    fprintf(stream, "(unspecified), ");
                } else {
                    fprintf(stream, "%u, ", (unsigned) value->type.Date.day);
                }
                if (value->type.Date.year == 255) {
                    fprintf(stream, "(unspecified), ");
                } else {
                    fprintf(stream, "%u", (unsigned) value->type.Date.year);
                }
                break;
            case BACNET_APPLICATION_TAG_TIME:
                if (value->type.Time.hour == 255) {
                    fprintf(stream, "**:");
                } else {
                    fprintf(stream, "%02u:", (unsigned) value->type.Time.hour);
                }
                if (value->type.Time.min == 255) {
                    fprintf(stream, "**:");
                } else {
                    fprintf(stream, "%02u:", (unsigned) value->type.Time.min);
                }
                if (value->type.Time.sec == 255) {
                    fprintf(stream, "**.");
                } else {
                    fprintf(stream, "%02u.", (unsigned) value->type.Time.sec);
                }
                if (value->type.Time.hundredths == 255) {
                    fprintf(stream, "**");
                } else {
                    fprintf(stream, "%02u",
                        (unsigned) value->type.Time.hundredths);
                }
                break;
            case BACNET_APPLICATION_TAG_OBJECT_ID:
                if (value->type.Object_Id.type < MAX_ASHRAE_OBJECT_TYPE) {
                    fprintf(stream, "(%s, %lu)",
                        bactext_object_type_name(value->type.Object_Id.type),
                        (unsigned long) value->type.Object_Id.instance);
                } else if (value->type.Object_Id.type < 128) {
                    fprintf(stream, "(reserved %u, %lu)",
                        (unsigned) value->type.Object_Id.type,
                        (unsigned long) value->type.Object_Id.instance);
                } else {
                    fprintf(stream, "(proprietary %u, %lu)",
                        (unsigned) value->type.Object_Id.type,
                        (unsigned long) value->type.Object_Id.instance);
                }
                break;
            default:
                status = false;
                break;
        }
    }

    return status;
}
#endif

#ifdef BACAPP_PRINT_ENABLED
/* used to load the app data struct with the proper data
   converted from a command line argument */
bool bacapp_parse_application_data(
    BACNET_APPLICATION_TAG tag_number,
    const char *argv,
    BACNET_APPLICATION_DATA_VALUE * value)
{
    int hour, min, sec, hundredths;
    int year, month, day, wday;
    int object_type = 0;
    uint32_t instance = 0;
    bool status = false;
    long long_value = 0;
    unsigned long unsigned_long_value = 0;
    double double_value = 0.0;
    int count = 0;

    if (value && (tag_number < MAX_BACNET_APPLICATION_TAG)) {
        status = true;
        value->tag = tag_number;
        switch (tag_number) {
            case BACNET_APPLICATION_TAG_BOOLEAN:
                long_value = strtol(argv, NULL, 0);
                if (long_value)
                    value->type.Boolean = true;
                else
                    value->type.Boolean = false;
                break;
            case BACNET_APPLICATION_TAG_UNSIGNED_INT:
                unsigned_long_value = strtoul(argv, NULL, 0);
                value->type.Unsigned_Int = unsigned_long_value;
                break;
            case BACNET_APPLICATION_TAG_SIGNED_INT:
                long_value = strtol(argv, NULL, 0);
                value->type.Signed_Int = long_value;
                break;
            case BACNET_APPLICATION_TAG_REAL:
                double_value = strtod(argv, NULL);
                value->type.Real = (float) double_value;
                break;
#if defined (BACAPP_DOUBLE)
            case BACNET_APPLICATION_TAG_DOUBLE:
                double_value = strtod(argv, NULL);
                value->type.Double = double_value;
                break;
#endif
            case BACNET_APPLICATION_TAG_OCTET_STRING:
                status =
                    octetstring_init(&value->type.Octet_String,
                    (uint8_t *) argv, strlen(argv));
                break;
            case BACNET_APPLICATION_TAG_CHARACTER_STRING:
                status =
                    characterstring_init_ansi(&value->type.Character_String,
                    (char *) argv);
                break;
            case BACNET_APPLICATION_TAG_BIT_STRING:
                /* FIXME: how to parse a bit string? */
                status = false;
                bitstring_init(&value->type.Bit_String);
                break;
            case BACNET_APPLICATION_TAG_ENUMERATED:
                unsigned_long_value = strtoul(argv, NULL, 0);
                value->type.Enumerated = unsigned_long_value;
                break;
            case BACNET_APPLICATION_TAG_DATE:
                count =
                    sscanf(argv, "%d/%d/%d:%d", &year, &month, &day, &wday);
                if (count == 3) {
                    datetime_set_date(&value->type.Date, (uint16_t) year,
                        (uint8_t) month, (uint8_t) day);
                } else if (count == 4) {
                    value->type.Date.year = (uint16_t) year;
                    value->type.Date.month = (uint8_t) month;
                    value->type.Date.day = (uint8_t) day;
                    value->type.Date.wday = (uint8_t) wday;
                } else {
                    status = false;
                }
                break;
            case BACNET_APPLICATION_TAG_TIME:
                count =
                    sscanf(argv, "%d:%d:%d.%d", &hour, &min, &sec,
                    &hundredths);
                if (count == 4) {
                    value->type.Time.hour = (uint8_t) hour;
                    value->type.Time.min = (uint8_t) min;
                    value->type.Time.sec = (uint8_t) sec;
                    value->type.Time.hundredths = (uint8_t) hundredths;
                } else if (count == 3) {
                    value->type.Time.hour = (uint8_t) hour;
                    value->type.Time.min = (uint8_t) min;
                    value->type.Time.sec = (uint8_t) sec;
                    value->type.Time.hundredths = 0;
                } else if (count == 2) {
                    value->type.Time.hour = (uint8_t) hour;
                    value->type.Time.min = (uint8_t) min;
                    value->type.Time.sec = 0;
                    value->type.Time.hundredths = 0;
                } else {
                    status = false;
                }
                break;
            case BACNET_APPLICATION_TAG_OBJECT_ID:
                count = sscanf(argv, "%d:%d", &object_type, &instance);
                if (count == 2) {
                    value->type.Object_Id.type = (uint16_t) object_type;
                    value->type.Object_Id.instance = instance;
                } else {
                    status = false;
                }
                break;
            default:
                break;
        }
        value->next = NULL;
    }

    return status;
}
#endif

/** Decode application data for object(s) BACnetEventLogRecord, which can be found in the log-buffer property of a event-log object.
   @param apdu The data to be decoded.
   @param apdu_len The data length to be decoded.
   @param choice_tag_number The contextual type of data decoded (1: event, 0:logstatus, or 2:time-change).
   @param value_timestamp The decoded timestamp information.
   @param value_status_or_time The time-change parameter or the log-status parameter in case of special event-log line.
   @param notification_data The event values for normal event-log lines.
   @return the decoded information length. 
 */
int bacapp_decode_bacneteventlogrecord(
    uint8_t * apdu,
    int apdu_len,
    int *choice_tag_number,
    BACNET_DATE_TIME * value_timestamp,
    BACNET_APPLICATION_DATA_VALUE * value_status_or_time,
    BACNET_EVENT_NOTIFICATION_DATA * notification_data)
{
    /* timestamp [0] BACnetDateTime */
    /* logDatum  [1] CHOICE */
    /* { */
    /*    log-status    [0] BACnetStatusLog */
    /*    notification  [1] ConfirmedEventNotification-Request */
    /*    time-change   [2] REAL */
    /* } */

    int len = 0;
    int val_len = 0;
    uint8_t tag_number = 0;
    int app_type = 0;
    uint32_t len_value_type = 0;
    bool is_opening_tag = false;

    if (apdu_len && apdu && value_timestamp && value_status_or_time &&
        notification_data && choice_tag_number && (apdu_len > 12)) {
        /* tag 0 - timeStamp : BACnetDateTime */
        val_len = bacapp_decode_context_datetime(&apdu[len], 0, value_timestamp);       /* 12 bytes */
        if (val_len > 0) {
            /* jump ahead */
            len += val_len;
        } else
            return -1;

        /* tag 1 - log-datum : Choice */
        if ((len < apdu_len) && decode_is_context_tag(&apdu[len], 1) &&
            decode_is_opening_tag(&apdu[len])) {
            /* jump ahead */
            len++;
            /* Context data : */
            if (IS_CONTEXT_SPECIFIC(apdu[len])) {
                val_len =
                    decode_tag_number_and_value(&apdu[len], &tag_number,
                    &len_value_type);
                is_opening_tag = decode_is_opening_tag(&apdu[len]);
                len += val_len;

                /* Contextual type of data */
                *choice_tag_number = tag_number;

                /* Choice : */
                switch (tag_number) {
                    case 0:    /* log-status */
                        app_type = BACNET_APPLICATION_TAG_BIT_STRING;
                        break;
                    case 2:    /* time change (real) */
                        app_type = BACNET_APPLICATION_TAG_REAL;
                        break;
                    case 1:    /* event */
                        val_len =
                            event_notify_decode_service_request(&apdu[len],
                            apdu_len - len, notification_data);
                        break;
                    default:
                        return -1;
                };

                /* primitive type : decode the value */
                if (app_type < MAX_BACNET_APPLICATION_TAG) {
                    value_status_or_time->tag = app_type;
                    val_len =
                        bacapp_decode_data(&apdu[len], app_type,
                        len_value_type, value_status_or_time);
                }
                /* decoded value ok ? */
                if (val_len > 0)
                    len += val_len;
                else
                    return -1;

                /* if there was an opening tag, decode matching closing tag */
                if (is_opening_tag) {
                    if (decode_is_context_tag(&apdu[len], tag_number) &&
                        decode_is_closing_tag(&apdu[len]))
                        len++;
                    else
                        return -1;      /* Missed closing tag (!) */
                }
            } else
                return -1;

            /* Closing tag */
            if (decode_is_context_tag(&apdu[len], 1) &&
                decode_is_closing_tag(&apdu[len]))
                len++;
            else
                return -1;      /* Missed closing tag (!) */
        } else
            return -1;  /* logdatum is mandatory */

        return len;
    }
    return -1;
}

/** Decode application data for object(s) BACnetLogMultipleRecord, which can be found in the log-buffer property of a trend-log-multiple object.
   @param apdu The data to be decoded.
   @param apdu_len The data length to be decoded.
   @param choice_tag_number The contextual type of data decoded (value, logstatus, or time-change).
   @param value_timestamp The decoded timestamp information.
   @param size_of_values_returned The maximum number of values that can be returns (size of the array: value_logdatum)
   @param value_logdatum Array of the resulting value decoded BACNET_APPLICATION_DATA_VALUE[]. In the case of log-status and time-change, only the first item will be filled.
   @param values_returned The number of values actually read from the multiple-valued line of trending.
   @return the decoded information length.
 */
int bacapp_decode_bacnetlogmultiplerecord(
    uint8_t * apdu,
    int apdu_len,
    int *choice_tag_number,
    BACNET_DATE_TIME * value_timestamp,
    unsigned int size_of_values_returned,
    BACNET_APPLICATION_DATA_VALUE * value_logdatum,
    unsigned int *values_returned)
{
    /* timestamp [0] BACnetDateTime */
    /* logdata   [1] BACnetLogData */

    /* XXX - TODO : Decode BACnetLogData */

    int len = 0;
    int val_len = 0;
    uint8_t tag_number = 0;
    uint32_t len_value_type = 0;
    bool is_opening_tag = false;

    if (apdu_len && apdu && value_timestamp && value_logdatum &&
        choice_tag_number && (apdu_len > 12)) {
        /* tag 0 - timeStamp : BACnetDateTime */
        val_len = bacapp_decode_context_datetime(&apdu[len], 0, value_timestamp);       /* 12 bytes */
        if (val_len > 0) {
            /* jump ahead */
            len += val_len;
        } else
            return -1;

        /* tag 1 - logData : BACnetLogData */
        if ((len < apdu_len) && decode_is_context_tag(&apdu[len], 1) &&
            decode_is_opening_tag(&apdu[len])) {
            /* jump ahead */
            len++;
            /* Context data : */
            if (IS_CONTEXT_SPECIFIC(apdu[len])) {
                val_len =
                    decode_tag_number_and_value(&apdu[len], &tag_number,
                    &len_value_type);
                is_opening_tag = decode_is_opening_tag(&apdu[len]);
                len += val_len;

                /* Contextual type of data */
                *choice_tag_number = tag_number;
/*
            // Choice :
            switch ( tag_number ) {
               case 0: // log-status
               case 6: // bitstring-value
                  app_type = BACNET_APPLICATION_TAG_BIT_STRING;
                  break;
               case 1: // boolean-value
                  app_type = BACNET_APPLICATION_TAG_BOOLEAN;
                  break;
               case 9: // time change (real)
               case 2: // real-value
                  app_type = BACNET_APPLICATION_TAG_REAL;
                  break;
               case 3: // enum-value
                  app_type = BACNET_APPLICATION_TAG_ENUMERATED;
                  break;
               case 4: // unsigned-value
                  app_type = BACNET_APPLICATION_TAG_UNSIGNED_INT;
                  break;
               case 5: // signed-value
                  app_type = BACNET_APPLICATION_TAG_SIGNED_INT;
                  break;
               case 7: // null-value
                  app_type = BACNET_APPLICATION_TAG_NULL;
                  break;
               case 8: // Error (??!?!)
                  app_type = MAX_BACNET_APPLICATION_TAG;
                  value_logdatum->tag = BACNET_APPLICATION_TAG_ERROR;
                  val_len = bacerror_decode_error_class_and_code( &apdu[len], apdu_len - len, & value_logdatum->type.Access_Error.error_class, & value_logdatum->type.Access_Error.error_code );
                  break;
               case 10: // any-value
                  app_type = MAX_BACNET_APPLICATION_TAG;
                  // Application data
                  val_len = bacapp_decode_application_data(&apdu[len], apdu_len - len, value_logdatum );
                  break;
               default:
                  return -1;
            };
            
            // primitive type : decode the value
            if ( app_type < MAX_BACNET_APPLICATION_TAG ) {
               value_logdatum->tag = app_type;
               val_len = bacapp_decode_data(&apdu[len], app_type, len_value_type, value_logdatum);
            }
            */

                /* decoded value ok ? */
                if (val_len > 0)
                    len += val_len;
                else
                    return -1;

                /* if there was an opening tag, decode matching closing tag */
                if (is_opening_tag) {
                    if (decode_is_context_tag(&apdu[len], tag_number) &&
                        decode_is_closing_tag(&apdu[len]))
                        len++;
                    else
                        return -1;      /* Missed closing tag (!) */
                }

            } else
                return -1;

            /* Closing tag */
            if (decode_is_context_tag(&apdu[len], 1) &&
                decode_is_closing_tag(&apdu[len]))
                len++;
            else
                return -1;      /* Missed closing tag (!) */
        } else
            return -1;  /* logdatum is mandatory */

        return len;
    }
    return -1;
}

/** Decode application data for object(s) BACnetLogRecord, which can be found in the log-buffer property of a trend-log object.
   @param apdu The data to be decoded.
   @param apdu_len The data length to be decoded.
   @param choice_tag_number The contextual type of data decoded (value, logstatus, or time-change).
   @param value_timestamp The decoded timestamp information.
   @param value_logdatum The resulting value decoded.
   @param value_statusflags The optional status-flags of the object.
   @return the decoded information length.
 */
int bacapp_decode_bacnetlogrecord(
    uint8_t * apdu,
    int apdu_len,
    int *choice_tag_number,
    BACNET_DATE_TIME * value_timestamp,
    BACNET_APPLICATION_DATA_VALUE * value_logdatum,
    BACNET_BIT_STRING * value_statusflags)
{
    /* timestamp [0] BACnetDateTime */
    /* logdatum [1] Choice (appdata) */
    /* statusFlags [2] BACnetStatusFlags (optional) */

    int len = 0;
    int val_len = 0;
    uint8_t tag_number = 0;
    int app_type = 0;
    uint32_t len_value_type = 0;
    bool is_opening_tag = false;

    if (apdu_len && apdu && value_timestamp && value_logdatum &&
        value_statusflags && choice_tag_number && (apdu_len > 12)) {
        /* tag 0 - timeStamp : BACnetDateTime */
        val_len = bacapp_decode_context_datetime(&apdu[len], 0, value_timestamp);       /* 12 bytes */
        if (val_len > 0) {
            /* jump ahead */
            len += val_len;
        } else
            return -1;

        /* tag 1 - log-datum : Choice */
        if ((len < apdu_len) && decode_is_context_tag(&apdu[len], 1) &&
            decode_is_opening_tag(&apdu[len])) {
            /* jump ahead */
            len++;
            /* Context data : */
            if (IS_CONTEXT_SPECIFIC(apdu[len])) {
                val_len =
                    decode_tag_number_and_value(&apdu[len], &tag_number,
                    &len_value_type);
                is_opening_tag = decode_is_opening_tag(&apdu[len]);
                len += val_len;

                /* Contextual type of data */
                *choice_tag_number = tag_number;

                /* Choice : */
                switch (tag_number) {
                    case 0:    /* log-status */
                    case 6:    /* bitstring-value */
                        app_type = BACNET_APPLICATION_TAG_BIT_STRING;
                        break;
                    case 1:    /* boolean-value */
                        app_type = BACNET_APPLICATION_TAG_BOOLEAN;
                        break;
                    case 9:    /* time change (real) */
                    case 2:    /* real-value */
                        app_type = BACNET_APPLICATION_TAG_REAL;
                        break;
                    case 3:    /* enum-value */
                        app_type = BACNET_APPLICATION_TAG_ENUMERATED;
                        break;
                    case 4:    /* unsigned-value */
                        app_type = BACNET_APPLICATION_TAG_UNSIGNED_INT;
                        break;
                    case 5:    /* signed-value */
                        app_type = BACNET_APPLICATION_TAG_SIGNED_INT;
                        break;
                    case 7:    /* null-value */
                        app_type = BACNET_APPLICATION_TAG_NULL;
                        break;
                    case 8:    /* Error (??!?!) */
                        app_type = MAX_BACNET_APPLICATION_TAG;
                        value_logdatum->tag = BACNET_APPLICATION_TAG_ERROR;
                        val_len =
                            bacerror_decode_error_class_and_code(&apdu[len],
                            apdu_len - len,
                            &value_logdatum->type.Access_Error.error_class,
                            &value_logdatum->type.Access_Error.error_code);
                        break;
                    case 10:   /* any-value */
                        app_type = MAX_BACNET_APPLICATION_TAG;
                        /* Application data */
                        val_len =
                            bacapp_decode_application_data(&apdu[len],
                            apdu_len - len, value_logdatum);
                        break;
                    default:
                        return -1;
                };

                /* primitive type : decode the value */
                if (app_type < MAX_BACNET_APPLICATION_TAG) {
                    value_logdatum->tag = app_type;
                    val_len =
                        bacapp_decode_data(&apdu[len], app_type,
                        len_value_type, value_logdatum);
                }
                /* decoded value ok ? */
                if (val_len > 0)
                    len += val_len;
                else
                    return -1;

                /* if there was an opening tag, decode matching closing tag */
                if (is_opening_tag) {
                    if (decode_is_context_tag(&apdu[len], tag_number) &&
                        decode_is_closing_tag(&apdu[len]))
                        len++;
                    else
                        return -1;      /* Missed closing tag (!) */
                }

            } else
                return -1;

            /* Closing tag */
            if (decode_is_context_tag(&apdu[len], 1) &&
                decode_is_closing_tag(&apdu[len]))
                len++;
            else
                return -1;      /* Missed closing tag (!) */
        } else
            return -1;  /* logdatum is mandatory */

        /* tag 2 - statusFlags (optional) : BACnetStatusFlags */
        if ((len < apdu_len) && decode_is_context_tag(&apdu[len], 2) &&
            !decode_is_closing_tag(&apdu[len])) {
            if (-1 == (val_len =
                    decode_context_bitstring(&apdu[len], 2,
                        value_statusflags)))
                return -1;
            len += val_len;
        } else
            /* Zero bitstring */
            value_statusflags->bits_used = 0;
        return len;
    }
    return -1;
}



#ifdef TEST
#include <assert.h>
#include <string.h>
#include "ctest.h"


#include <assert.h>

/* generic - can be used by other unit tests
   returns true if matching or same, false if different */
bool bacapp_same_value(
    BACNET_APPLICATION_DATA_VALUE * value,
    BACNET_APPLICATION_DATA_VALUE * test_value)
{
    bool status = false;        /*return value */

    /* does the tag match? */
    if (test_value->tag == value->tag)
        status = true;
    if (status) {
        /* second test for same-ness */
        status = false;
        /* does the value match? */
        switch (test_value->tag) {
#if defined (BACAPP_NULL)
            case BACNET_APPLICATION_TAG_NULL:
                status = true;
                break;
#endif
#if defined (BACAPP_BOOLEAN)
            case BACNET_APPLICATION_TAG_BOOLEAN:
                if (test_value->type.Boolean == value->type.Boolean)
                    status = true;
                break;
#endif
#if defined (BACAPP_UNSIGNED)
            case BACNET_APPLICATION_TAG_UNSIGNED_INT:
                if (test_value->type.Unsigned_Int == value->type.Unsigned_Int)
                    status = true;
                break;
#endif
#if defined (BACAPP_SIGNED)
            case BACNET_APPLICATION_TAG_SIGNED_INT:
                if (test_value->type.Signed_Int == value->type.Signed_Int)
                    status = true;
                break;
#endif
#if defined (BACAPP_REAL)
            case BACNET_APPLICATION_TAG_REAL:
                if (test_value->type.Real == value->type.Real)
                    status = true;
                break;
#endif
#if defined (BACAPP_DOUBLE)
            case BACNET_APPLICATION_TAG_DOUBLE:
                if (test_value->type.Double == value->type.Double)
                    status = true;
                break;
#endif
#if defined (BACAPP_ENUMERATED)
            case BACNET_APPLICATION_TAG_ENUMERATED:
                if (test_value->type.Enumerated == value->type.Enumerated)
                    status = true;
                break;
#endif
#if defined (BACAPP_DATE)
            case BACNET_APPLICATION_TAG_DATE:
                if (datetime_compare_date(&test_value->type.Date,
                        &value->type.Date) == 0)
                    status = true;
                break;
#endif
#if defined (BACAPP_TIME)
            case BACNET_APPLICATION_TAG_TIME:
                if (datetime_compare_time(&test_value->type.Time,
                        &value->type.Time) == 0)
                    status = true;
                break;
#endif
#if defined (BACAPP_OBJECT_ID)
            case BACNET_APPLICATION_TAG_OBJECT_ID:
                if ((test_value->type.Object_Id.type ==
                        value->type.Object_Id.type) &&
                    (test_value->type.Object_Id.instance ==
                        value->type.Object_Id.instance)) {
                    status = true;
                }
                break;
#endif
#if defined (BACAPP_CHARACTER_STRING)
            case BACNET_APPLICATION_TAG_CHARACTER_STRING:
                status =
                    characterstring_same(&value->type.Character_String,
                    &test_value->type.Character_String);
                break;
#endif
#if defined (BACAPP_OCTET_STRING)
            case BACNET_APPLICATION_TAG_OCTET_STRING:
                status =
                    octetstring_value_same(&value->type.Octet_String,
                    &test_value->type.Octet_String);
                break;
#endif
#if defined (BACAPP_BIT_STRING)
            case BACNET_APPLICATION_TAG_BIT_STRING:
                status =
                    bitstring_same(&value->type.Bit_String,
                    &test_value->type.Bit_String);
                break;
#endif

            default:
                status = false;
                break;
        }
    }
    return status;
}

void testBACnetApplicationData_Safe(
    Test * pTest)
{
    int i;
    uint8_t apdu[MAX_APDU];
    int len = 0;
    int apdu_len;
    BACNET_APPLICATION_DATA_VALUE input_value[13];
    uint32_t len_segment[13];
    uint32_t single_length_segment;
    BACNET_APPLICATION_DATA_VALUE value;


    for (i = 0; i < 13; i++) {
        input_value[i].tag = (BACNET_APPLICATION_TAG) i;
        input_value[i].context_specific = 0;
        input_value[i].context_tag = 0;
        input_value[i].next = NULL;
        switch (input_value[i].tag) {
            case BACNET_APPLICATION_TAG_NULL:
                /* NULL: no data */
                break;

            case BACNET_APPLICATION_TAG_BOOLEAN:
                input_value[i].type.Boolean = true;
                break;

            case BACNET_APPLICATION_TAG_UNSIGNED_INT:
                input_value[i].type.Unsigned_Int = 0xDEADBEEF;
                break;

            case BACNET_APPLICATION_TAG_SIGNED_INT:
                input_value[i].type.Signed_Int = 0x00C0FFEE;
                break;
            case BACNET_APPLICATION_TAG_REAL:
                input_value[i].type.Real = 3.141592654f;
                break;
            case BACNET_APPLICATION_TAG_DOUBLE:
                input_value[i].type.Double = 2.32323232323;
                break;

            case BACNET_APPLICATION_TAG_OCTET_STRING:
                {
                    uint8_t test_octet[5] = { "Karg" };
                    octetstring_init(&input_value[i].type.Octet_String,
                        test_octet, sizeof(test_octet));
                }
                break;

            case BACNET_APPLICATION_TAG_CHARACTER_STRING:
                characterstring_init_ansi(&input_value[i].type.
                    Character_String, "Hello There!");
                break;

            case BACNET_APPLICATION_TAG_BIT_STRING:
                bitstring_init(&input_value[i].type.Bit_String);
                bitstring_set_bit(&input_value[i].type.Bit_String, 0, true);
                bitstring_set_bit(&input_value[i].type.Bit_String, 1, false);
                bitstring_set_bit(&input_value[i].type.Bit_String, 2, false);
                bitstring_set_bit(&input_value[i].type.Bit_String, 3, true);
                bitstring_set_bit(&input_value[i].type.Bit_String, 4, false);
                bitstring_set_bit(&input_value[i].type.Bit_String, 5, true);
                bitstring_set_bit(&input_value[i].type.Bit_String, 6, true);
                break;

            case BACNET_APPLICATION_TAG_ENUMERATED:
                input_value[i].type.Enumerated = 0x0BADF00D;
                break;

            case BACNET_APPLICATION_TAG_DATE:
                input_value[i].type.Date.day = 10;
                input_value[i].type.Date.month = 9;
                input_value[i].type.Date.wday = 3;
                input_value[i].type.Date.year = 1998;
                break;

            case BACNET_APPLICATION_TAG_TIME:
                input_value[i].type.Time.hour = 12;
                input_value[i].type.Time.hundredths = 56;
                input_value[i].type.Time.min = 20;
                input_value[i].type.Time.sec = 41;
                break;

            case BACNET_APPLICATION_TAG_OBJECT_ID:
                input_value[i].type.Object_Id.instance = 1234;
                input_value[i].type.Object_Id.type = 12;
                break;

            default:
                break;
        }
        single_length_segment =
            bacapp_encode_data(&apdu[len], &input_value[i]);;
        assert(single_length_segment > 0);
        /* len_segment is accumulated length */
        if (i == 0) {
            len_segment[i] = single_length_segment;
        } else {
            len_segment[i] = single_length_segment + len_segment[i - 1];
        }
        len = len_segment[i];
    }
    /*
     ** Start processing packets at processivly truncated lengths
     */

    for (apdu_len = len; apdu_len >= 0; apdu_len--) {
        bool status;
        bool expected_status;
        for (i = 0; i < 14; i++) {
            if (i == 13) {
                expected_status = false;
            } else {

                if (apdu_len < len_segment[i]) {
                    expected_status = false;
                } else {
                    expected_status = true;
                }
            }
            status =
                bacapp_decode_application_data_safe(i == 0 ? apdu : NULL,
                apdu_len, &value);

            ct_test(pTest, status == expected_status);
            if (status) {
                ct_test(pTest, value.tag == i);
                ct_test(pTest, bacapp_same_value(&input_value[i], &value));
                ct_test(pTest, !value.context_specific);
                ct_test(pTest, value.next == NULL);
            } else {
                break;
            }
        }
    }
}


void testBACnetApplicationDataLength(
    Test * pTest)
{
    int apdu_len = 0;   /* total length of the apdu, return value */
    int len = 0;        /* total length of the apdu, return value */
    int test_len = 0;   /* length of the data */
    uint8_t apdu[480] = { 0 };
    BACNET_TIME local_time;
    BACNET_DATE local_date;

    /* create some constructed data */
    /* 1. zero elements */
    test_len = 0;
    apdu_len = 0;
    len = encode_opening_tag(&apdu[apdu_len], 3);
    apdu_len += len;
    len = encode_closing_tag(&apdu[apdu_len], 3);
    apdu_len += len;
    /* verify the length of the data inside the opening/closing tags */
    len =
        bacapp_data_len(&apdu[0], apdu_len,
        PROP_LIST_OF_OBJECT_PROPERTY_REFERENCES);
    ct_test(pTest, test_len == len);

    /* 2. application tagged data, one element */
    test_len = 0;
    apdu_len = 0;
    len = encode_opening_tag(&apdu[apdu_len], 3);
    apdu_len += len;
    len = encode_application_unsigned(&apdu[apdu_len], 4194303);
    test_len += len;
    apdu_len += len;
    len = encode_closing_tag(&apdu[apdu_len], 3);
    apdu_len += len;
    /* verify the length of the data inside the opening/closing tags */
    len = bacapp_data_len(&apdu[0], apdu_len, PROP_OBJECT_IDENTIFIER);
    ct_test(pTest, test_len == len);

    /* 3. application tagged data, multiple elements */
    test_len = 0;
    apdu_len = 0;
    len = encode_opening_tag(&apdu[apdu_len], 3);
    apdu_len += len;
    len = encode_application_null(&apdu[apdu_len]);
    test_len += len;
    apdu_len += len;
    len = encode_application_null(&apdu[apdu_len]);
    test_len += len;
    apdu_len += len;
    len = encode_application_unsigned(&apdu[apdu_len], 1);
    test_len += len;
    apdu_len += len;
    len = encode_application_unsigned(&apdu[apdu_len], 42);
    test_len += len;
    apdu_len += len;
    len = encode_application_unsigned(&apdu[apdu_len], 91);
    test_len += len;
    apdu_len += len;
    len = encode_application_null(&apdu[apdu_len]);
    test_len += len;
    apdu_len += len;
    len = encode_application_null(&apdu[apdu_len]);
    test_len += len;
    apdu_len += len;
    len = encode_application_null(&apdu[apdu_len]);
    test_len += len;
    apdu_len += len;
    len = encode_application_null(&apdu[apdu_len]);
    test_len += len;
    apdu_len += len;
    len = encode_application_null(&apdu[apdu_len]);
    test_len += len;
    apdu_len += len;
    len = encode_application_null(&apdu[apdu_len]);
    test_len += len;
    apdu_len += len;
    len = encode_application_null(&apdu[apdu_len]);
    test_len += len;
    apdu_len += len;
    len = encode_application_null(&apdu[apdu_len]);
    test_len += len;
    apdu_len += len;
    len = encode_application_null(&apdu[apdu_len]);
    test_len += len;
    apdu_len += len;
    len = encode_application_null(&apdu[apdu_len]);
    test_len += len;
    apdu_len += len;
    len = encode_application_null(&apdu[apdu_len]);
    test_len += len;
    apdu_len += len;
    len = encode_closing_tag(&apdu[apdu_len], 3);
    apdu_len += len;
    /* verify the length of the data inside the opening/closing tags */
    len = bacapp_data_len(&apdu[0], apdu_len, PROP_PRIORITY_ARRAY);
    ct_test(pTest, test_len == len);

    /* 4. complex datatype - one element */
    test_len = 0;
    apdu_len = 0;
    len = encode_opening_tag(&apdu[apdu_len], 3);
    apdu_len += len;
    len = encode_opening_tag(&apdu[apdu_len], 3);
    test_len += len;
    apdu_len += len;
    local_date.year = 2006;     /* AD */
    local_date.month = 4;       /* 1=Jan */
    local_date.day = 1; /* 1..31 */
    local_date.wday = 6;        /* 1=Monday */
    len = encode_application_date(&apdu[apdu_len], &local_date);
    test_len += len;
    apdu_len += len;
    local_time.hour = 7;
    local_time.min = 0;
    local_time.sec = 3;
    local_time.hundredths = 1;
    len = encode_application_time(&apdu[apdu_len], &local_time);
    test_len += len;
    apdu_len += len;
    len = encode_closing_tag(&apdu[apdu_len], 3);
    test_len += len;
    apdu_len += len;
    len = encode_closing_tag(&apdu[apdu_len], 3);
    apdu_len += len;
    /* verify the length of the data inside the opening/closing tags */
    len = bacapp_data_len(&apdu[0], apdu_len, PROP_START_TIME);
    ct_test(pTest, test_len == len);

    /* 5. complex datatype - multiple elements */



    /* 6. context tagged data, one element */
    test_len = 0;
    apdu_len = 0;
    len = encode_opening_tag(&apdu[apdu_len], 3);
    apdu_len += len;
    len = encode_context_unsigned(&apdu[apdu_len], 1, 91);
    test_len += len;
    apdu_len += len;
    len = encode_closing_tag(&apdu[apdu_len], 3);
    apdu_len += len;
    /* verify the length of the data inside the opening/closing tags */
    len = bacapp_data_len(&apdu[0], apdu_len, PROP_REQUESTED_SHED_LEVEL);
    ct_test(pTest, test_len == len);
}

static bool testBACnetApplicationDataValue(
    BACNET_APPLICATION_DATA_VALUE * value)
{
    uint8_t apdu[480] = { 0 };
    int len = 0;
    int apdu_len = 0;
    BACNET_APPLICATION_DATA_VALUE test_value;

    apdu_len = bacapp_encode_application_data(&apdu[0], value);
    len = bacapp_decode_application_data(&apdu[0], apdu_len, &test_value);

    return bacapp_same_value(value, &test_value);
}

void testBACnetApplicationData(
    Test * pTest)
{
    BACNET_APPLICATION_DATA_VALUE value;
    bool status = false;


    status =
        bacapp_parse_application_data(BACNET_APPLICATION_TAG_NULL, NULL,
        &value);
    ct_test(pTest, status == true);
    status = testBACnetApplicationDataValue(&value);
    ct_test(pTest, status == true);

    status =
        bacapp_parse_application_data(BACNET_APPLICATION_TAG_BOOLEAN, "1",
        &value);
    ct_test(pTest, status == true);
    ct_test(pTest, value.type.Boolean == true);
    ct_test(pTest, testBACnetApplicationDataValue(&value));

    status =
        bacapp_parse_application_data(BACNET_APPLICATION_TAG_BOOLEAN, "0",
        &value);
    ct_test(pTest, status == true);
    ct_test(pTest, value.type.Boolean == false);
    ct_test(pTest, testBACnetApplicationDataValue(&value));

    status =
        bacapp_parse_application_data(BACNET_APPLICATION_TAG_UNSIGNED_INT, "0",
        &value);
    ct_test(pTest, status == true);
    ct_test(pTest, value.type.Unsigned_Int == 0);
    ct_test(pTest, testBACnetApplicationDataValue(&value));
    status =
        bacapp_parse_application_data(BACNET_APPLICATION_TAG_UNSIGNED_INT,
        "0xFFFF", &value);
    ct_test(pTest, status == true);
    ct_test(pTest, value.type.Unsigned_Int == 0xFFFF);
    ct_test(pTest, testBACnetApplicationDataValue(&value));
    status =
        bacapp_parse_application_data(BACNET_APPLICATION_TAG_UNSIGNED_INT,
        "0xFFFFFFFF", &value);
    ct_test(pTest, status == true);
    ct_test(pTest, value.type.Unsigned_Int == 0xFFFFFFFF);
    ct_test(pTest, testBACnetApplicationDataValue(&value));

    status =
        bacapp_parse_application_data(BACNET_APPLICATION_TAG_SIGNED_INT, "0",
        &value);
    ct_test(pTest, status == true);
    ct_test(pTest, value.type.Signed_Int == 0);
    ct_test(pTest, testBACnetApplicationDataValue(&value));
    status =
        bacapp_parse_application_data(BACNET_APPLICATION_TAG_SIGNED_INT, "-1",
        &value);
    ct_test(pTest, status == true);
    ct_test(pTest, value.type.Signed_Int == -1);
    ct_test(pTest, testBACnetApplicationDataValue(&value));
    status =
        bacapp_parse_application_data(BACNET_APPLICATION_TAG_SIGNED_INT,
        "32768", &value);
    ct_test(pTest, status == true);
    ct_test(pTest, value.type.Signed_Int == 32768);
    ct_test(pTest, testBACnetApplicationDataValue(&value));
    status =
        bacapp_parse_application_data(BACNET_APPLICATION_TAG_SIGNED_INT,
        "-32768", &value);
    ct_test(pTest, status == true);
    ct_test(pTest, value.type.Signed_Int == -32768);
    ct_test(pTest, testBACnetApplicationDataValue(&value));

    status =
        bacapp_parse_application_data(BACNET_APPLICATION_TAG_REAL, "0.0",
        &value);
    ct_test(pTest, status == true);
    ct_test(pTest, testBACnetApplicationDataValue(&value));
    status =
        bacapp_parse_application_data(BACNET_APPLICATION_TAG_REAL, "-1.0",
        &value);
    ct_test(pTest, status == true);
    ct_test(pTest, testBACnetApplicationDataValue(&value));
    status =
        bacapp_parse_application_data(BACNET_APPLICATION_TAG_REAL, "1.0",
        &value);
    ct_test(pTest, status == true);
    ct_test(pTest, testBACnetApplicationDataValue(&value));
    status =
        bacapp_parse_application_data(BACNET_APPLICATION_TAG_REAL, "3.14159",
        &value);
    ct_test(pTest, status == true);
    ct_test(pTest, testBACnetApplicationDataValue(&value));
    status =
        bacapp_parse_application_data(BACNET_APPLICATION_TAG_REAL, "-3.14159",
        &value);
    ct_test(pTest, status == true);
    ct_test(pTest, testBACnetApplicationDataValue(&value));

    status =
        bacapp_parse_application_data(BACNET_APPLICATION_TAG_ENUMERATED, "0",
        &value);
    ct_test(pTest, status == true);
    ct_test(pTest, value.type.Enumerated == 0);
    ct_test(pTest, testBACnetApplicationDataValue(&value));
    status =
        bacapp_parse_application_data(BACNET_APPLICATION_TAG_ENUMERATED,
        "0xFFFF", &value);
    ct_test(pTest, status == true);
    ct_test(pTest, value.type.Enumerated == 0xFFFF);
    ct_test(pTest, testBACnetApplicationDataValue(&value));
    status =
        bacapp_parse_application_data(BACNET_APPLICATION_TAG_ENUMERATED,
        "0xFFFFFFFF", &value);
    ct_test(pTest, status == true);
    ct_test(pTest, value.type.Enumerated == 0xFFFFFFFF);
    ct_test(pTest, testBACnetApplicationDataValue(&value));

    status =
        bacapp_parse_application_data(BACNET_APPLICATION_TAG_DATE,
        "2005/5/22:1", &value);
    ct_test(pTest, status == true);
    ct_test(pTest, value.type.Date.year == 2005);
    ct_test(pTest, value.type.Date.month == 5);
    ct_test(pTest, value.type.Date.day == 22);
    ct_test(pTest, value.type.Date.wday == 1);
    ct_test(pTest, testBACnetApplicationDataValue(&value));

    /* Happy Valentines Day! */
    status =
        bacapp_parse_application_data(BACNET_APPLICATION_TAG_DATE, "2007/2/14",
        &value);
    ct_test(pTest, status == true);
    ct_test(pTest, value.type.Date.year == 2007);
    ct_test(pTest, value.type.Date.month == 2);
    ct_test(pTest, value.type.Date.day == 14);
    ct_test(pTest, value.type.Date.wday == BACNET_WEEKDAY_WEDNESDAY);
    ct_test(pTest, testBACnetApplicationDataValue(&value));

    /* Wildcard Values */
    status =
        bacapp_parse_application_data(BACNET_APPLICATION_TAG_DATE,
        "2155/255/255:255", &value);
    ct_test(pTest, status == true);
    ct_test(pTest, value.type.Date.year == 2155);
    ct_test(pTest, value.type.Date.month == 255);
    ct_test(pTest, value.type.Date.day == 255);
    ct_test(pTest, value.type.Date.wday == 255);
    ct_test(pTest, testBACnetApplicationDataValue(&value));

    status =
        bacapp_parse_application_data(BACNET_APPLICATION_TAG_TIME,
        "23:59:59.12", &value);
    ct_test(pTest, status == true);
    ct_test(pTest, value.type.Time.hour == 23);
    ct_test(pTest, value.type.Time.min == 59);
    ct_test(pTest, value.type.Time.sec == 59);
    ct_test(pTest, value.type.Time.hundredths == 12);
    ct_test(pTest, testBACnetApplicationDataValue(&value));

    status =
        bacapp_parse_application_data(BACNET_APPLICATION_TAG_TIME, "23:59:59",
        &value);
    ct_test(pTest, status == true);
    ct_test(pTest, value.type.Time.hour == 23);
    ct_test(pTest, value.type.Time.min == 59);
    ct_test(pTest, value.type.Time.sec == 59);
    ct_test(pTest, value.type.Time.hundredths == 0);
    ct_test(pTest, testBACnetApplicationDataValue(&value));

    status =
        bacapp_parse_application_data(BACNET_APPLICATION_TAG_TIME, "23:59",
        &value);
    ct_test(pTest, status == true);
    ct_test(pTest, value.type.Time.hour == 23);
    ct_test(pTest, value.type.Time.min == 59);
    ct_test(pTest, value.type.Time.sec == 0);
    ct_test(pTest, value.type.Time.hundredths == 0);
    ct_test(pTest, testBACnetApplicationDataValue(&value));

    status =
        bacapp_parse_application_data(BACNET_APPLICATION_TAG_OBJECT_ID,
        "0:100", &value);
    ct_test(pTest, status == true);
    ct_test(pTest, value.type.Object_Id.type == 0);
    ct_test(pTest, value.type.Object_Id.instance == 100);
    ct_test(pTest, testBACnetApplicationDataValue(&value));

    status =
        bacapp_parse_application_data(BACNET_APPLICATION_TAG_CHARACTER_STRING,
        "Karg!", &value);
    ct_test(pTest, status == true);
    ct_test(pTest, testBACnetApplicationDataValue(&value));

    status =
        bacapp_parse_application_data(BACNET_APPLICATION_TAG_OCTET_STRING,
        "Steve + Patricia", &value);
    ct_test(pTest, status == true);
    ct_test(pTest, testBACnetApplicationDataValue(&value));

    return;
}


#ifdef TEST_BACNET_APPLICATION_DATA
int main(
    void)
{
    Test *pTest;
    bool rc;

    pTest = ct_create("BACnet Application Data", NULL);
    /* individual tests */
    rc = ct_addTestFunction(pTest, testBACnetApplicationData);
    assert(rc);
    rc = ct_addTestFunction(pTest, testBACnetApplicationDataLength);
    assert(rc);
    rc = ct_addTestFunction(pTest, testBACnetApplicationData_Safe);
    assert(rc);

    ct_setStream(pTest, stdout);
    ct_run(pTest);
    (void) ct_report(pTest);
    ct_destroy(pTest);

    return 0;
}
#endif /* TEST_BACNET_APPLICATION_DATA */
#endif /* TEST */
