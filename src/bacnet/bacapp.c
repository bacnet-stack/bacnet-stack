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
#include <stdlib.h> /* for strtol */
#include <ctype.h> /* for isalnum */
#include <errno.h>
#ifdef __STDC_ISO_10646__
#include <wchar.h>
#include <wctype.h>
#endif
#include "bacnet/bacenum.h"
#include "bacnet/bacdcode.h"
#include "bacnet/bacint.h"
#include "bacnet/bacreal.h"
#include "bacnet/bacdef.h"
#include "bacnet/bacapp.h"
#include "bacnet/bactext.h"
#include "bacnet/datetime.h"
#include "bacnet/bacstr.h"
#include "bacnet/lighting.h"
#include "bacnet/hostnport.h"
#include "bacnet/weeklyschedule.h"
#include "bacnet/basic/sys/platform.h"

/** @file bacapp.c  Utilities for the BACnet_Application_Data_Value */

/**
 * @brief Encode application data given by a pointer into the APDU.
 * @param apdu - Pointer to the buffer to encode to, or NULL for length
 * @param value - Pointer to the application data value to encode from
 * @return number of bytes encoded
 */
int bacapp_encode_application_data(
    uint8_t *apdu, BACNET_APPLICATION_DATA_VALUE *value)
{
    int apdu_len = 0; /* total length of the apdu, return value */

    if (value) {
        switch (value->tag) {
#if defined(BACAPP_NULL)
            case BACNET_APPLICATION_TAG_NULL:
                if (apdu) {
                    apdu[0] = value->tag;
                }
                apdu_len++;
                break;
#endif
#if defined(BACAPP_BOOLEAN)
            case BACNET_APPLICATION_TAG_BOOLEAN:
                apdu_len =
                    encode_application_boolean(apdu, value->type.Boolean);
                break;
#endif
#if defined(BACAPP_UNSIGNED)
            case BACNET_APPLICATION_TAG_UNSIGNED_INT:
                apdu_len =
                    encode_application_unsigned(apdu, value->type.Unsigned_Int);
                break;
#endif
#if defined(BACAPP_SIGNED)
            case BACNET_APPLICATION_TAG_SIGNED_INT:
                apdu_len =
                    encode_application_signed(apdu, value->type.Signed_Int);
                break;
#endif
#if defined(BACAPP_REAL)
            case BACNET_APPLICATION_TAG_REAL:
                apdu_len = encode_application_real(apdu, value->type.Real);
                break;
#endif
#if defined(BACAPP_DOUBLE)
            case BACNET_APPLICATION_TAG_DOUBLE:
                apdu_len = encode_application_double(apdu, value->type.Double);
                break;
#endif
#if defined(BACAPP_OCTET_STRING)
            case BACNET_APPLICATION_TAG_OCTET_STRING:
                apdu_len = encode_application_octet_string(
                    apdu, &value->type.Octet_String);
                break;
#endif
#if defined(BACAPP_CHARACTER_STRING)
            case BACNET_APPLICATION_TAG_CHARACTER_STRING:
                apdu_len = encode_application_character_string(
                    apdu, &value->type.Character_String);
                break;
#endif
#if defined(BACAPP_BIT_STRING)
            case BACNET_APPLICATION_TAG_BIT_STRING:
                apdu_len =
                    encode_application_bitstring(apdu, &value->type.Bit_String);
                break;
#endif
#if defined(BACAPP_ENUMERATED)
            case BACNET_APPLICATION_TAG_ENUMERATED:
                apdu_len =
                    encode_application_enumerated(apdu, value->type.Enumerated);
                break;
#endif
#if defined(BACAPP_DATE)
            case BACNET_APPLICATION_TAG_DATE:
                apdu_len = encode_application_date(apdu, &value->type.Date);
                break;
#endif
#if defined(BACAPP_TIME)
            case BACNET_APPLICATION_TAG_TIME:
                apdu_len = encode_application_time(apdu, &value->type.Time);
                break;
#endif
#if defined(BACAPP_OBJECT_ID)
            case BACNET_APPLICATION_TAG_OBJECT_ID:
                apdu_len = encode_application_object_id(apdu,
                    value->type.Object_Id.type, value->type.Object_Id.instance);
                break;
#endif
#if defined(BACAPP_TYPES_EXTRA)
            case BACNET_APPLICATION_TAG_EMPTYLIST:
                /* Empty data list */
                apdu_len = 0; /* EMPTY */
                break;

                break;
            case BACNET_APPLICATION_TAG_DATETIME:
                apdu_len = bacapp_encode_datetime(apdu, &value->type.Date_Time);
                break;
            case BACNET_APPLICATION_TAG_LIGHTING_COMMAND:
                /* BACnetLightingCommand */
                apdu_len = lighting_command_encode(
                    apdu, &value->type.Lighting_Command);
                break;
            case BACNET_APPLICATION_TAG_XY_COLOR:
                /* BACnetxyColor */
                apdu_len = xy_color_encode(apdu, &value->type.XY_Color);
                break;
            case BACNET_APPLICATION_TAG_COLOR_COMMAND:
                /* BACnetColorCommand */
                apdu_len =
                    color_command_encode(apdu, &value->type.Color_Command);
                break;
            case BACNET_APPLICATION_TAG_WEEKLY_SCHEDULE:
                /* BACnetWeeklySchedule */
                apdu_len = bacnet_weeklyschedule_encode(
                    apdu, &value->type.Weekly_Schedule);
                break;
            case BACNET_APPLICATION_TAG_HOST_N_PORT:
                /* BACnetHostNPort */
                apdu_len = host_n_port_encode(apdu, &value->type.Host_Address);
                break;
            case BACNET_APPLICATION_TAG_DEVICE_OBJECT_PROPERTY_REFERENCE:
                /* BACnetDeviceObjectPropertyReference */
                apdu_len = bacapp_encode_device_obj_property_ref(
                    apdu, &value->type.Device_Object_Property_Reference);
                break;
            case BACNET_APPLICATION_TAG_DEVICE_OBJECT_REFERENCE:
                /* BACnetDeviceObjectReference */
                apdu_len = bacapp_encode_device_obj_ref(
                    apdu, &value->type.Device_Object_Reference);
                break;
            case BACNET_APPLICATION_TAG_OBJECT_PROPERTY_REFERENCE:
                /* BACnetObjectPropertyReference */
                apdu_len = bacapp_encode_obj_property_ref(
                    apdu, &value->type.Object_Property_Reference);
                break;
#endif
            default:
                break;
        }
    }

    return apdu_len;
}

/**
 * @brief Decode the data and store it into value.
 * @param apdu  Receive buffer
 * @param tag_data_type  Data type of the given tag
 * @param len_value_type  Count of bytes of given tag
 * @param value  Pointer to the application value structure,
 *               used to store the decoded value to.
 *
 * @return Number of octets consumed.
 */
int bacapp_decode_data(uint8_t *apdu,
    uint8_t tag_data_type,
    uint32_t len_value_type,
    BACNET_APPLICATION_DATA_VALUE *value)
{
    int len = 0;

    if (value) {
        switch (tag_data_type) {
#if defined(BACAPP_NULL)
            case BACNET_APPLICATION_TAG_NULL:
                /* nothing else to do */
                break;
#endif
#if defined(BACAPP_BOOLEAN)
            case BACNET_APPLICATION_TAG_BOOLEAN:
                value->type.Boolean = decode_boolean(len_value_type);
                break;
#endif
#if defined(BACAPP_UNSIGNED)
            case BACNET_APPLICATION_TAG_UNSIGNED_INT:
                len = decode_unsigned(
                    apdu, len_value_type, &value->type.Unsigned_Int);
                break;
#endif
#if defined(BACAPP_SIGNED)
            case BACNET_APPLICATION_TAG_SIGNED_INT:
                len = decode_signed(
                    apdu, len_value_type, &value->type.Signed_Int);
                break;
#endif
#if defined(BACAPP_REAL)
            case BACNET_APPLICATION_TAG_REAL:
                len =
                    decode_real_safe(apdu, len_value_type, &(value->type.Real));
                break;
#endif
#if defined(BACAPP_DOUBLE)
            case BACNET_APPLICATION_TAG_DOUBLE:
                len = decode_double_safe(
                    apdu, len_value_type, &(value->type.Double));
                break;
#endif
#if defined(BACAPP_OCTET_STRING)
            case BACNET_APPLICATION_TAG_OCTET_STRING:
                len = decode_octet_string(
                    apdu, len_value_type, &value->type.Octet_String);
                break;
#endif
#if defined(BACAPP_CHARACTER_STRING)
            case BACNET_APPLICATION_TAG_CHARACTER_STRING:
                len = decode_character_string(
                    apdu, len_value_type, &value->type.Character_String);
                break;
#endif
#if defined(BACAPP_BIT_STRING)
            case BACNET_APPLICATION_TAG_BIT_STRING:
                len = decode_bitstring(
                    apdu, len_value_type, &value->type.Bit_String);
                break;
#endif
#if defined(BACAPP_ENUMERATED)
            case BACNET_APPLICATION_TAG_ENUMERATED:
                len = decode_enumerated(
                    apdu, len_value_type, &value->type.Enumerated);
                break;
#endif
#if defined(BACAPP_DATE)
            case BACNET_APPLICATION_TAG_DATE:
                len = decode_date_safe(apdu, len_value_type, &value->type.Date);
                break;
#endif
#if defined(BACAPP_TIME)
            case BACNET_APPLICATION_TAG_TIME:
                len = decode_bacnet_time_safe(
                    apdu, len_value_type, &value->type.Time);
                break;
#endif
#if defined(BACAPP_OBJECT_ID)
            case BACNET_APPLICATION_TAG_OBJECT_ID: {
                BACNET_OBJECT_TYPE object_type = OBJECT_NONE;
                uint32_t instance = 0;
                len = decode_object_id_safe(
                    apdu, len_value_type, &object_type, &instance);
                value->type.Object_Id.type = object_type;
                value->type.Object_Id.instance = instance;
            } break;
#endif
#if defined(BACAPP_TYPES_EXTRA)
            case BACNET_APPLICATION_TAG_DATETIME:
                len = bacapp_decode_datetime(apdu, &value->type.Date_Time);
                break;
            case BACNET_APPLICATION_TAG_LIGHTING_COMMAND:
                len = lighting_command_decode(
                    apdu, len_value_type, &value->type.Lighting_Command);
                break;
            case BACNET_APPLICATION_TAG_XY_COLOR:
                /* BACnetxyColor */
                len = xy_color_decode(
                    apdu, len_value_type, &value->type.XY_Color);
                break;
            case BACNET_APPLICATION_TAG_COLOR_COMMAND:
                /* BACnetColorCommand */
                len = color_command_decode(
                    apdu, len_value_type, NULL, &value->type.Color_Command);
                break;
            case BACNET_APPLICATION_TAG_WEEKLY_SCHEDULE:
                len = bacnet_weeklyschedule_decode(
                    apdu, len_value_type, &value->type.Weekly_Schedule);
                break;
            case BACNET_APPLICATION_TAG_HOST_N_PORT:
                len = host_n_port_decode(
                    apdu, len_value_type, NULL, &value->type.Host_Address);
                break;
            case BACNET_APPLICATION_TAG_DEVICE_OBJECT_PROPERTY_REFERENCE:
                /* BACnetDeviceObjectPropertyReference */
                len = bacapp_decode_device_obj_property_ref(
                    apdu, &value->type.Device_Object_Property_Reference);
                break;
            case BACNET_APPLICATION_TAG_DEVICE_OBJECT_REFERENCE:
                /* BACnetDeviceObjectReference */
                len = bacapp_decode_device_obj_ref(
                    apdu, &value->type.Device_Object_Reference);
                break;
            case BACNET_APPLICATION_TAG_OBJECT_PROPERTY_REFERENCE:
                /* BACnetObjectPropertyReference */
                len = bacapp_decode_obj_property_ref(apdu, len_value_type,
                    &value->type.Object_Property_Reference);
                break;
#endif
            default:
                break;
        }
    }

    if ((len == 0) && (tag_data_type != BACNET_APPLICATION_TAG_NULL) &&
        (tag_data_type != BACNET_APPLICATION_TAG_BOOLEAN) &&
        (tag_data_type != BACNET_APPLICATION_TAG_OCTET_STRING)) {
        /* indicate that we were not able to decode the value */
        if (value) {
            value->tag = MAX_BACNET_APPLICATION_TAG;
        }
    }
    return len;
}

/**
 * @brief Decode the BACnet Application Data
 *
 * @param apdu - buffer of data to be decoded
 * @param apdu_len_max - number of bytes in the buffer
 * @param value - decoded value, if decoded
 *
 * @return the number of apdu bytes consumed, or #BACNET_STATUS_ERROR
 */
int bacapp_decode_application_data(
    uint8_t *apdu, unsigned apdu_len_max, BACNET_APPLICATION_DATA_VALUE *value)
{
    int len = 0;
    int tag_len = 0;
    int decode_len = 0;
    uint8_t tag_number = 0;
    uint32_t len_value_type = 0;

    if (apdu && value && !IS_CONTEXT_SPECIFIC(*apdu)) {
        value->context_specific = false;
        tag_len = bacnet_tag_number_and_value_decode(
            &apdu[0], apdu_len_max, &tag_number, &len_value_type);
        if (tag_len > 0) {
            len += tag_len;
            value->tag = tag_number;
            if ((unsigned)len <= apdu_len_max) {
                decode_len =
                    bacapp_decode_data_len(NULL, tag_number, len_value_type);
                if ((unsigned)decode_len <= (apdu_len_max - len)) {
                    decode_len = bacapp_decode_data(
                        &apdu[len], tag_number, len_value_type, value);
                    if (value->tag != MAX_BACNET_APPLICATION_TAG) {
                        len += decode_len;
                    } else {
                        len = BACNET_STATUS_ERROR;
                    }
                } else {
                    len = BACNET_STATUS_ERROR;
                }
            } else {
                len = BACNET_STATUS_ERROR;
            }
        }
        value->next = NULL;
    }

    return len;
}

/*
** Usage: Similar to strtok. Call function the first time with new_apdu and
*new_adu_len set to apdu buffer
** to be processed. Subsequent calls should pass in NULL.
**
** Returns true if a application message is correctly parsed.
** Returns false if no more application messages are available.
**
** This function is NOT thread safe.
**
** Notes: The _safe suffix is there because the function should be relatively
*safe against buffer overruns.
**
*/

bool bacapp_decode_application_data_safe(uint8_t *new_apdu,
    uint32_t new_apdu_len,
    BACNET_APPLICATION_DATA_VALUE *value)
{
    /* The static variables that store the apdu buffer between function calls */
    static uint8_t *apdu = NULL;
    static uint32_t apdu_len_remaining = 0;
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
        tag_len = bacnet_tag_number_and_value_decode(
            &apdu[apdu_len], apdu_len_remaining, &tag_number, &len_value_type);
        /* If tag_len is zero, then the tag information is truncated */
        if (tag_len) {
            apdu_len += tag_len;
            apdu_len_remaining -= tag_len;
            /* The tag is boolean then len_value_type is interpreted as value,
               not length, so don't bother checking with apdu_len_remaining */
            if (tag_number == BACNET_APPLICATION_TAG_BOOLEAN ||
                len_value_type <= apdu_len_remaining) {
                value->tag = tag_number;
                len = bacapp_decode_data(
                    &apdu[apdu_len], tag_number, len_value_type, value);
                apdu_len += len;
                apdu_len_remaining -= len;

                ret = true;
            }
        }
        value->next = NULL;
    }

    return ret;
}

/**
 * @brief Decode the data to determine the data length
 *  @param apdu  Pointer to the received data.
 *  @param tag_data_type  Data type to be decoded.
 *  @param len_value_type  Length of the data in bytes.
 *  @return Number of bytes decoded.
 */
int bacapp_decode_data_len(
    uint8_t *apdu, uint8_t tag_data_type, uint32_t len_value_type)
{
    int len = 0;

    (void)apdu;
    switch (tag_data_type) {
        case BACNET_APPLICATION_TAG_NULL:
            break;
        case BACNET_APPLICATION_TAG_BOOLEAN:
            break;
        case BACNET_APPLICATION_TAG_UNSIGNED_INT:
        case BACNET_APPLICATION_TAG_SIGNED_INT:
        case BACNET_APPLICATION_TAG_REAL:
        case BACNET_APPLICATION_TAG_DOUBLE:
        case BACNET_APPLICATION_TAG_OCTET_STRING:
        case BACNET_APPLICATION_TAG_CHARACTER_STRING:
        case BACNET_APPLICATION_TAG_BIT_STRING:
        case BACNET_APPLICATION_TAG_ENUMERATED:
        case BACNET_APPLICATION_TAG_DATE:
        case BACNET_APPLICATION_TAG_TIME:
        case BACNET_APPLICATION_TAG_OBJECT_ID:
            len = (int)(~0U >> 1);
            if (len_value_type < (uint32_t)len) {
                len = (int)len_value_type;
            }
            break;
        default:
            break;
    }

    return len;
}

/**
 * @brief Determine the BACnet Application Data number of APDU bytes consumed
 * @param apdu - buffer of data to be decoded
 * @param apdu_len_max - number of bytes in the buffer
 * @return  number of bytes decoded, or zero if errors occur
 */
int bacapp_decode_application_data_len(uint8_t *apdu, unsigned apdu_len_max)
{
    int len = 0;
    int tag_len = 0;
    int decode_len = 0;
    uint8_t tag_number = 0;
    uint32_t len_value_type = 0;

    if (apdu && !IS_CONTEXT_SPECIFIC(*apdu)) {
        tag_len = bacnet_tag_number_and_value_decode(
            &apdu[0], apdu_len_max, &tag_number, &len_value_type);
        if (tag_len > 0) {
            len += tag_len;
            decode_len =
                bacapp_decode_data_len(NULL, tag_number, len_value_type);
            len += decode_len;
        }
    }

    return len;
}

int bacapp_encode_context_data_value(uint8_t *apdu,
    uint8_t context_tag_number,
    BACNET_APPLICATION_DATA_VALUE *value)
{
    int apdu_len = 0; /* total length of the apdu, return value */

    if (value) {
        switch (value->tag) {
#if defined(BACAPP_NULL)
            case BACNET_APPLICATION_TAG_NULL:
                apdu_len = encode_context_null(apdu, context_tag_number);
                break;
#endif
#if defined(BACAPP_BOOLEAN)
            case BACNET_APPLICATION_TAG_BOOLEAN:
                apdu_len = encode_context_boolean(
                    apdu, context_tag_number, value->type.Boolean);
                break;
#endif
#if defined(BACAPP_UNSIGNED)
            case BACNET_APPLICATION_TAG_UNSIGNED_INT:
                apdu_len = encode_context_unsigned(
                    apdu, context_tag_number, value->type.Unsigned_Int);
                break;
#endif
#if defined(BACAPP_SIGNED)
            case BACNET_APPLICATION_TAG_SIGNED_INT:
                apdu_len = encode_context_signed(
                    apdu, context_tag_number, value->type.Signed_Int);
                break;
#endif
#if defined(BACAPP_REAL)
            case BACNET_APPLICATION_TAG_REAL:
                apdu_len = encode_context_real(
                    apdu, context_tag_number, value->type.Real);
                break;
#endif
#if defined(BACAPP_DOUBLE)
            case BACNET_APPLICATION_TAG_DOUBLE:
                apdu_len = encode_context_double(
                    apdu, context_tag_number, value->type.Double);
                break;
#endif
#if defined(BACAPP_OCTET_STRING)
            case BACNET_APPLICATION_TAG_OCTET_STRING:
                apdu_len = encode_context_octet_string(
                    apdu, context_tag_number, &value->type.Octet_String);
                break;
#endif
#if defined(BACAPP_CHARACTER_STRING)
            case BACNET_APPLICATION_TAG_CHARACTER_STRING:
                apdu_len = encode_context_character_string(
                    apdu, context_tag_number, &value->type.Character_String);
                break;
#endif
#if defined(BACAPP_BIT_STRING)
            case BACNET_APPLICATION_TAG_BIT_STRING:
                apdu_len = encode_context_bitstring(
                    apdu, context_tag_number, &value->type.Bit_String);
                break;
#endif
#if defined(BACAPP_ENUMERATED)
            case BACNET_APPLICATION_TAG_ENUMERATED:
                apdu_len = encode_context_enumerated(
                    apdu, context_tag_number, value->type.Enumerated);
                break;
#endif
#if defined(BACAPP_DATE)
            case BACNET_APPLICATION_TAG_DATE:
                apdu_len = encode_context_date(
                    apdu, context_tag_number, &value->type.Date);
                break;
#endif
#if defined(BACAPP_TIME)
            case BACNET_APPLICATION_TAG_TIME:
                apdu_len = encode_context_time(
                    apdu, context_tag_number, &value->type.Time);
                break;
#endif
#if defined(BACAPP_OBJECT_ID)
            case BACNET_APPLICATION_TAG_OBJECT_ID:
                apdu_len = encode_context_object_id(apdu, context_tag_number,
                    value->type.Object_Id.type, value->type.Object_Id.instance);
                break;
#endif
#if defined(BACAPP_TYPES_EXTRA)
            case BACNET_APPLICATION_TAG_DATETIME:
                apdu_len = bacapp_encode_context_datetime(
                    apdu, context_tag_number, &value->type.Date_Time);
                break;
            case BACNET_APPLICATION_TAG_LIGHTING_COMMAND:
                apdu_len = lighting_command_encode_context(
                    apdu, context_tag_number, &value->type.Lighting_Command);
                break;
            case BACNET_APPLICATION_TAG_XY_COLOR:
                /* BACnetxyColor */
                apdu_len = xy_color_context_encode(
                    apdu, context_tag_number, &value->type.XY_Color);
                break;
            case BACNET_APPLICATION_TAG_WEEKLY_SCHEDULE:
                /* BACnetWeeklySchedule */
                apdu_len = bacnet_weeklyschedule_context_encode(
                    apdu, context_tag_number, &value->type.Weekly_Schedule);
                break;
            case BACNET_APPLICATION_TAG_COLOR_COMMAND:
                /* BACnetColorCommand */
                apdu_len = color_command_context_encode(
                    apdu, context_tag_number, &value->type.Color_Command);
                break;
            case BACNET_APPLICATION_TAG_HOST_N_PORT:
                apdu_len = host_n_port_context_encode(
                    apdu, context_tag_number, &value->type.Host_Address);
                break;
            case BACNET_APPLICATION_TAG_DEVICE_OBJECT_PROPERTY_REFERENCE:
                /* BACnetDeviceObjectPropertyReference */
                apdu_len = bacapp_encode_context_device_obj_property_ref(apdu,
                    context_tag_number,
                    &value->type.Device_Object_Property_Reference);
                break;
            case BACNET_APPLICATION_TAG_DEVICE_OBJECT_REFERENCE:
                /* BACnetDeviceObjectReference */
                apdu_len = bacapp_encode_context_device_obj_ref(apdu,
                    context_tag_number, &value->type.Device_Object_Reference);
                break;
            case BACNET_APPLICATION_TAG_OBJECT_PROPERTY_REFERENCE:
                /* BACnetObjectPropertyReference */
                apdu_len = bacapp_encode_context_obj_property_ref(apdu,
                    context_tag_number, &value->type.Object_Property_Reference);
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
    BACNET_PROPERTY_ID property, uint8_t tag_number)
{
    BACNET_APPLICATION_TAG tag = MAX_BACNET_APPLICATION_TAG;

    switch (property) {
        case PROP_DATE_LIST:
            switch (tag_number) {
                case 0: /* single calendar date */
                    tag = BACNET_APPLICATION_TAG_DATE;
                    break;
                case 1: /* range of dates */
                    tag = BACNET_APPLICATION_TAG_DATERANGE;
                    break;
                case 2: /* selection of weeks, month, and day of month */
                    tag = BACNET_APPLICATION_TAG_WEEKNDAY;
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
                case 4: /* propertyValue: abstract syntax */
                default:
                    break;
            }
            break;
        case PROP_LIST_OF_GROUP_MEMBERS:
            /* Sequence of ReadAccessSpecification */
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
                case 0: /* calendarEntry: abstract syntax + context */
                case 2: /* list of BACnetTimeValue: abstract syntax */
                default:
                    break;
            }
            break;
        case PROP_LOG_DEVICE_OBJECT_PROPERTY:
        case PROP_OBJECT_PROPERTY_REFERENCE:
            switch (tag_number) {
                case 0: /* Object ID */
                case 3: /* Device ID */
                    tag = BACNET_APPLICATION_TAG_OBJECT_ID;
                    break;
                case 1: /* Property ID */
                    tag = BACNET_APPLICATION_TAG_ENUMERATED;
                    break;
                case 2: /* Array index */
                    tag = BACNET_APPLICATION_TAG_UNSIGNED_INT;
                    break;
                default:
                    break;
            }
            break;
        case PROP_SUBORDINATE_LIST:
            /* BACnetARRAY[N] of BACnetDeviceObjectReference */
            switch (tag_number) {
                case 0: /* Optional Device ID */
                case 1: /* Object ID */
                    tag = BACNET_APPLICATION_TAG_OBJECT_ID;
                    break;
                default:
                    break;
            }
            break;

        case PROP_RECIPIENT_LIST:
            /* List of BACnetDestination */
            switch (tag_number) {
                case 0: /* Device Object ID */
                    tag = BACNET_APPLICATION_TAG_OBJECT_ID;
                    break;
                case 1:
                    /* BACnetRecipient::= CHOICE {
                        device  [0] BACnetObjectIdentifier
                     -->address [1] BACnetAddress
                    }
                    */
                    break;
                default:
                    break;
            }
            break;
        case PROP_ACTIVE_COV_SUBSCRIPTIONS:
            /* BACnetCOVSubscription */
            switch (tag_number) {
                case 0: /* BACnetRecipientProcess */
                    break;
                case 1: /* BACnetObjectPropertyReference */
                    tag = BACNET_APPLICATION_TAG_OBJECT_PROPERTY_REFERENCE;
                    break;
                case 2: /* issueConfirmedNotifications */
                    tag = BACNET_APPLICATION_TAG_BOOLEAN;
                    break;
                case 3: /* timeRemaining */
                    tag = BACNET_APPLICATION_TAG_UNSIGNED_INT;
                    break;
                case 4: /* covIncrement */
                    tag = BACNET_APPLICATION_TAG_REAL;
                    break;
                default:
                    break;
            }
            break;
        case PROP_SETPOINT_REFERENCE:
            switch (tag_number) {
                case 0:
                    tag = BACNET_APPLICATION_TAG_OBJECT_PROPERTY_REFERENCE;
                    break;
                default:
                    break;
            }
            break;
        case PROP_FD_BBMD_ADDRESS:
        case PROP_BACNET_IP_GLOBAL_ADDRESS:
            switch (tag_number) {
                case 0:
                    tag = BACNET_APPLICATION_TAG_HOST_N_PORT;
                    break;
                default:
                    break;
            }
            break;
        case PROP_LIGHTING_COMMAND:
            switch (tag_number) {
                case 0:
                    tag = BACNET_APPLICATION_TAG_LIGHTING_COMMAND;
                    break;
                default:
                    break;
            }
            break;
        case PROP_COLOR_COMMAND:
            switch (tag_number) {
                case 0:
                    tag = BACNET_APPLICATION_TAG_COLOR_COMMAND;
                    break;
                default:
                    break;
            }
            break;
        case PROP_LIST_OF_OBJECT_PROPERTY_REFERENCES:
        case PROP_GROUP_MEMBERS:
            switch (tag_number) {
                case 0:
                    tag =
                        BACNET_APPLICATION_TAG_DEVICE_OBJECT_PROPERTY_REFERENCE;
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

int bacapp_encode_context_data(uint8_t *apdu,
    BACNET_APPLICATION_DATA_VALUE *value,
    BACNET_PROPERTY_ID property)
{
    int apdu_len = 0;
    BACNET_APPLICATION_TAG tag_data_type;

    if (value && apdu) {
        tag_data_type = bacapp_context_tag_type(property, value->context_tag);
        if (tag_data_type != MAX_BACNET_APPLICATION_TAG) {
            apdu_len = bacapp_encode_context_data_value(
                &apdu[0], tag_data_type, value);
        } else {
            /* FIXME: what now? */
            apdu_len = 0;
        }
        value->next = NULL;
    }

    return apdu_len;
}

int bacapp_decode_context_data(uint8_t *apdu,
    unsigned max_apdu_len,
    BACNET_APPLICATION_DATA_VALUE *value,
    BACNET_PROPERTY_ID property)
{
    int apdu_len = 0, len = 0;
    int tag_len = 0;
    uint8_t tag_number = 0;
    uint32_t len_value_type = 0;

    if (apdu && value && IS_CONTEXT_SPECIFIC(*apdu)) {
        value->context_specific = true;
        value->next = NULL;
        tag_len =
            decode_tag_number_and_value(&apdu[0], &tag_number, &len_value_type);
        apdu_len = tag_len;
        /* Empty construct : (closing tag) => returns NULL value */
        if (tag_len && ((unsigned)tag_len <= max_apdu_len) &&
            !decode_is_closing_tag_number(&apdu[0], tag_number)) {
            value->context_tag = tag_number;
            value->tag = bacapp_context_tag_type(property, tag_number);
            if (value->tag != MAX_BACNET_APPLICATION_TAG) {
                len = bacapp_decode_data(
                    &apdu[apdu_len], value->tag, len_value_type, value);
                apdu_len += len;
            } else if (len_value_type) {
                /* Unknown value : non null size (elementary type) */
                apdu_len += len_value_type;
                /* SHOULD NOT HAPPEN, EXCEPTED WHEN READING UNKNOWN CONTEXTUAL
                 * PROPERTY */
            } else {
                apdu_len = BACNET_STATUS_ERROR;
            }
        } else if (tag_len == 1) {
            /* and is a Closing tag */
            /* Don't advance over that closing tag. */
            apdu_len = 0;
        }
    }

    return apdu_len;
}

#if defined(BACAPP_TYPES_EXTRA)
/**
 * @brief Context or Application tagged property value decoding
 *
 * @param apdu - buffer of data to be decoded
 * @param max_apdu_len - number of bytes in the buffer
 * @param value - stores the decoded property value
 * @param property - context property identifier
 * @return  number of bytes decoded, or ERROR if errors occur
 */
int bacapp_decode_generic_property(uint8_t *apdu,
    int max_apdu_len,
    BACNET_APPLICATION_DATA_VALUE *value,
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
#endif

#if defined(BACAPP_TYPES_EXTRA)
/* decode one value of a priority array */
static int decode_priority_value(uint8_t *apdu,
    unsigned max_apdu_len,
    BACNET_APPLICATION_DATA_VALUE *value,
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
        val_len = bacapp_decode_generic_property(
            &apdu[len], max_apdu_len - len, value, prop);
        if (val_len < 0) {
            return BACNET_STATUS_ERROR;
        }
        len += val_len;
        if (is_opening_tag) {
            if (!decode_is_closing_tag_number(apdu, 0)) {
                return BACNET_STATUS_ERROR;
            }
            len++;
        }
    } else {
        len = bacapp_decode_generic_property(apdu, max_apdu_len, value, prop);
    }

    return len;
}
#endif

#if defined(BACAPP_TYPES_EXTRA)
int bacapp_known_property_tag(
    BACNET_OBJECT_TYPE object_type, BACNET_PROPERTY_ID property)
{
    switch (property) {
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
        case PROP_ACCOMPANIMENT:
        case PROP_BELONGS_TO:
        case PROP_LAST_ACCESS_POINT:
            /* Properties using BACnetDeviceObjectReference */
            return BACNET_APPLICATION_TAG_DEVICE_OBJECT_REFERENCE;

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
        case PROP_EXPIRATION_TIME:
        case PROP_LAST_USE_TIME:
            /* Properties using BACnetDateTime value */
            return BACNET_APPLICATION_TAG_DATETIME;

        case PROP_OBJECT_PROPERTY_REFERENCE:
        case PROP_LOG_DEVICE_OBJECT_PROPERTY:
        case PROP_LIST_OF_OBJECT_PROPERTY_REFERENCES:
            /* Properties using BACnetDeviceObjectPropertyReference */
            return BACNET_APPLICATION_TAG_DEVICE_OBJECT_PROPERTY_REFERENCE;

        case PROP_MANIPULATED_VARIABLE_REFERENCE:
        case PROP_CONTROLLED_VARIABLE_REFERENCE:
        case PROP_INPUT_REFERENCE:
            /* Properties using BACnetObjectPropertyReference */
            return BACNET_APPLICATION_TAG_OBJECT_PROPERTY_REFERENCE;

        case PROP_EVENT_TIME_STAMPS:
        case PROP_LAST_RESTORE_TIME:
        case PROP_TIME_OF_DEVICE_RESTART:
        case PROP_ACCESS_EVENT_TIME:
            /* Properties using BACnetTimeStamp */
            return BACNET_APPLICATION_TAG_TIMESTAMP;

        case PROP_DEFAULT_COLOR:
            /* Properties using BACnetxyColor */
            return BACNET_APPLICATION_TAG_XY_COLOR;

        case PROP_TRACKING_VALUE:
        case PROP_PRESENT_VALUE:
            if (object_type == OBJECT_COLOR) {
                /* Properties using BACnetxyColor */
                return BACNET_APPLICATION_TAG_XY_COLOR;
            }
            return -1;

        case PROP_COLOR_COMMAND:
            /* Properties using BACnetColorCommand */
            return BACNET_APPLICATION_TAG_COLOR_COMMAND;

        case PROP_LIGHTING_COMMAND:
            /* Properties using BACnetLightingCommand */
            return BACNET_APPLICATION_TAG_LIGHTING_COMMAND;

        case PROP_WEEKLY_SCHEDULE:
            /* BACnetWeeklySchedule ([7] BACnetDailySchedule*/
            return BACNET_APPLICATION_TAG_WEEKLY_SCHEDULE;

        case PROP_PRIORITY_ARRAY:
            /* [16] BACnetPriorityValue : 16x values (simple property) */
            return -1;

        case PROP_LIST_OF_GROUP_MEMBERS:
            /* Properties using ReadAccessSpecification */
            return -1;

        case PROP_EXCEPTION_SCHEDULE:
            /* BACnetSpecialEvent (Schedule) */
            return -1;

        case PROP_DATE_LIST:
            /* FIXME: Properties using : BACnetCalendarEntry */
            return -1;

        case PROP_ACTIVE_COV_SUBSCRIPTIONS:
            /* FIXME: BACnetCOVSubscription */
            return -1;

        case PROP_EFFECTIVE_PERIOD:
            /* FIXME: Properties using BACnetDateRange  (Schedule) */
            return -1;

        case PROP_RECIPIENT_LIST:
            /* FIXME: Properties using BACnetDestination */
            return -1;

        case PROP_TIME_SYNCHRONIZATION_RECIPIENTS:
        case PROP_RESTART_NOTIFICATION_RECIPIENTS:
        case PROP_UTC_TIME_SYNCHRONIZATION_RECIPIENTS:
            /* FIXME: Properties using BACnetRecipient */
            return -1;

        case PROP_DEVICE_ADDRESS_BINDING:
        case PROP_MANUAL_SLAVE_ADDRESS_BINDING:
        case PROP_SLAVE_ADDRESS_BINDING:
            /* FIXME: BACnetAddressBinding */
            return -1;

        case PROP_ACTION:
            return -1;

        default:
            return -1;
    }
}

/**
 * @brief Decodes a well-known, possibly complex property value
 *  Used to reverse operations in bacapp_encode_application_data
 * @param apdu - buffer of data to be decoded
 * @param max_apdu_len - number of bytes in the buffer
 * @param value - stores the decoded property value
 * @param property - context property identifier
 * @return  number of bytes decoded, or ERROR if errors occur
 */
int bacapp_decode_known_property(uint8_t *apdu,
    int max_apdu_len,
    BACNET_APPLICATION_DATA_VALUE *value,
    BACNET_OBJECT_TYPE object_type,
    BACNET_PROPERTY_ID property)
{
    int len = 0;

    // NOTE:
    //   When adding impl for a new prop, also add its tag
    //   to bacapp_known_property_tag()

    int tag = bacapp_known_property_tag(object_type, property);
    if (tag != -1) {
        value->tag = tag;
    }

    switch (property) {
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
        case PROP_ACCOMPANIMENT:
        case PROP_BELONGS_TO:
        case PROP_LAST_ACCESS_POINT:
            /* Properties using BACnetDeviceObjectReference */
            len = bacapp_decode_device_obj_ref(
                apdu, &value->type.Device_Object_Reference);
            break;

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
        case PROP_EXPIRATION_TIME:
        case PROP_LAST_USE_TIME:
            /* Properties using BACnetDateTime value */
            len = bacapp_decode_datetime(apdu, &value->type.Date_Time);
            break;

        case PROP_OBJECT_PROPERTY_REFERENCE:
        case PROP_LOG_DEVICE_OBJECT_PROPERTY:
        case PROP_LIST_OF_OBJECT_PROPERTY_REFERENCES:
            /* Properties using BACnetDeviceObjectPropertyReference */
            len = bacapp_decode_device_obj_property_ref(
                apdu, &value->type.Device_Object_Property_Reference);
            break;

        case PROP_MANIPULATED_VARIABLE_REFERENCE:
        case PROP_CONTROLLED_VARIABLE_REFERENCE:
        case PROP_INPUT_REFERENCE:
            /* Properties using BACnetObjectPropertyReference */
            len = bacapp_decode_obj_property_ref(
                apdu, max_apdu_len, &value->type.Object_Property_Reference);
            break;

        case PROP_EVENT_TIME_STAMPS:
        case PROP_LAST_RESTORE_TIME:
        case PROP_TIME_OF_DEVICE_RESTART:
        case PROP_ACCESS_EVENT_TIME:
            /* Properties using BACnetTimeStamp */
            len = bacapp_decode_timestamp(apdu, &value->type.Time_Stamp);
            break;

        case PROP_DEFAULT_COLOR:
            /* Properties using BACnetxyColor */
            len = xy_color_decode(apdu, max_apdu_len, &value->type.XY_Color);
            break;

        case PROP_TRACKING_VALUE:
        case PROP_PRESENT_VALUE:
            if (object_type == OBJECT_COLOR) {
                /* Properties using BACnetxyColor */
                len =
                    xy_color_decode(apdu, max_apdu_len, &value->type.XY_Color);
            } else {
                /* Decode a "classic" simple property */
                len = bacapp_decode_generic_property(
                    apdu, max_apdu_len, value, property);
            }
            break;

        case PROP_COLOR_COMMAND:
            /* Properties using BACnetColorCommand */
            len = color_command_decode(
                apdu, max_apdu_len, NULL, &value->type.Color_Command);
            break;

        case PROP_LIGHTING_COMMAND:
            /* Properties using BACnetLightingCommand */
            len = lighting_command_decode(
                apdu, max_apdu_len, &value->type.Lighting_Command);
            break;

        case PROP_PRIORITY_ARRAY:
            /* [16] BACnetPriorityValue : 16x values (simple property) */
            len = decode_priority_value(apdu, max_apdu_len, value, property);
            break;

        case PROP_WEEKLY_SCHEDULE:
            /* BACnetWeeklySchedule ([7] BACnetDailySchedule*/
            len = bacnet_weeklyschedule_decode(
                apdu, max_apdu_len, &value->type.Weekly_Schedule);
            break;

            /* properties without a specific decoder - fall through to default
             */

        case PROP_LIST_OF_GROUP_MEMBERS:
            /* Properties using ReadAccessSpecification */
        case PROP_EXCEPTION_SCHEDULE:
            /* BACnetSpecialEvent (Schedule) */
        case PROP_DATE_LIST:
            /* FIXME: Properties using : BACnetCalendarEntry */
        case PROP_ACTIVE_COV_SUBSCRIPTIONS:
            /* FIXME: BACnetCOVSubscription */
        case PROP_EFFECTIVE_PERIOD:
            /* FIXME: Properties using BACnetDateRange  (Schedule) */
        case PROP_RECIPIENT_LIST:
            /* FIXME: Properties using BACnetDestination */
        case PROP_TIME_SYNCHRONIZATION_RECIPIENTS:
        case PROP_RESTART_NOTIFICATION_RECIPIENTS:
        case PROP_UTC_TIME_SYNCHRONIZATION_RECIPIENTS:
            /* FIXME: Properties using BACnetRecipient */
        case PROP_DEVICE_ADDRESS_BINDING:
        case PROP_MANUAL_SLAVE_ADDRESS_BINDING:
        case PROP_SLAVE_ADDRESS_BINDING:
            /* FIXME: BACnetAddressBinding */
        case PROP_ACTION:
        default:
            /* Decode a "classic" simple property */
            len = bacapp_decode_generic_property(
                apdu, max_apdu_len, value, property);
            break;
    }

    return len;
}
#endif

#if defined(BACAPP_TYPES_EXTRA)
/**
 * @brief Determine the BACnet Context Data number of APDU bytes consumed
 *
 * @param apdu - buffer of data to be decoded
 * @param apdu_len_max - number of bytes in the buffer
 * @param property - context property identifier
 *
 * @return  number of bytes decoded, or zero if errors occur
 */
int bacapp_decode_context_data_len(
    uint8_t *apdu, unsigned apdu_len_max, BACNET_PROPERTY_ID property)
{
    int apdu_len = 0, len = 0;
    int tag_len = 0;
    uint8_t tag_number = 0;
    uint32_t len_value_type = 0;
    uint8_t tag = 0;

    if (apdu && IS_CONTEXT_SPECIFIC(*apdu)) {
        tag_len = bacnet_tag_number_and_value_decode(
            &apdu[0], apdu_len_max, &tag_number, &len_value_type);
        if (tag_len) {
            apdu_len = tag_len;
            tag = bacapp_context_tag_type(property, tag_number);
            if (tag != MAX_BACNET_APPLICATION_TAG) {
                len = bacapp_decode_data_len(NULL, tag, len_value_type);
                apdu_len += len;
            } else {
                apdu_len += len_value_type;
            }
        }
    }

    return apdu_len;
}
#endif

int bacapp_encode_data(uint8_t *apdu, BACNET_APPLICATION_DATA_VALUE *value)
{
    int apdu_len = 0; /* total length of the apdu, return value */

    if (value) {
        if (value->context_specific) {
            apdu_len = bacapp_encode_context_data_value(
                apdu, value->context_tag, value);
        } else {
            apdu_len = bacapp_encode_application_data(apdu, value);
        }
    }

    return apdu_len;
}

bool bacapp_copy(BACNET_APPLICATION_DATA_VALUE *dest_value,
    BACNET_APPLICATION_DATA_VALUE *src_value)
{
    bool status = false; /* return value, assume failure */

    if (dest_value && src_value) {
        status = true; /* assume successful for now */
        dest_value->tag = src_value->tag;
        switch (src_value->tag) {
#if defined(BACAPP_NULL)
            case BACNET_APPLICATION_TAG_NULL:
                break;
#endif
#if defined(BACAPP_BOOLEAN)
            case BACNET_APPLICATION_TAG_BOOLEAN:
                dest_value->type.Boolean = src_value->type.Boolean;
                break;
#endif
#if defined(BACAPP_UNSIGNED)
            case BACNET_APPLICATION_TAG_UNSIGNED_INT:
                dest_value->type.Unsigned_Int = src_value->type.Unsigned_Int;
                break;
#endif
#if defined(BACAPP_SIGNED)
            case BACNET_APPLICATION_TAG_SIGNED_INT:
                dest_value->type.Signed_Int = src_value->type.Signed_Int;
                break;
#endif
#if defined(BACAPP_REAL)
            case BACNET_APPLICATION_TAG_REAL:
                dest_value->type.Real = src_value->type.Real;
                break;
#endif
#if defined(BACAPP_DOUBLE)
            case BACNET_APPLICATION_TAG_DOUBLE:
                dest_value->type.Double = src_value->type.Double;
                break;
#endif
#if defined(BACAPP_OCTET_STRING)
            case BACNET_APPLICATION_TAG_OCTET_STRING:
                octetstring_copy(&dest_value->type.Octet_String,
                    &src_value->type.Octet_String);
                break;
#endif
#if defined(BACAPP_CHARACTER_STRING)
            case BACNET_APPLICATION_TAG_CHARACTER_STRING:
                characterstring_copy(&dest_value->type.Character_String,
                    &src_value->type.Character_String);
                break;
#endif
#if defined(BACAPP_BIT_STRING)
            case BACNET_APPLICATION_TAG_BIT_STRING:
                bitstring_copy(
                    &dest_value->type.Bit_String, &src_value->type.Bit_String);
                break;
#endif
#if defined(BACAPP_ENUMERATED)
            case BACNET_APPLICATION_TAG_ENUMERATED:
                dest_value->type.Enumerated = src_value->type.Enumerated;
                break;
#endif
#if defined(BACAPP_DATE)
            case BACNET_APPLICATION_TAG_DATE:
                datetime_copy_date(
                    &dest_value->type.Date, &src_value->type.Date);
                break;
#endif
#if defined(BACAPP_TIME)
            case BACNET_APPLICATION_TAG_TIME:
                datetime_copy_time(
                    &dest_value->type.Time, &src_value->type.Time);
                break;
#endif
#if defined(BACAPP_OBJECT_ID)
            case BACNET_APPLICATION_TAG_OBJECT_ID:
                dest_value->type.Object_Id.type =
                    src_value->type.Object_Id.type;
                dest_value->type.Object_Id.instance =
                    src_value->type.Object_Id.instance;
                break;
#endif
#if defined(BACAPP_TYPES_EXTRA)
            case BACNET_APPLICATION_TAG_LIGHTING_COMMAND:
                status =
                    lighting_command_copy(&dest_value->type.Lighting_Command,
                        &src_value->type.Lighting_Command);
                break;
            case BACNET_APPLICATION_TAG_HOST_N_PORT:
                status = host_n_port_copy(&dest_value->type.Host_Address,
                    &src_value->type.Host_Address);
                break;
#endif
            default:
                memcpy(&dest_value->type, &src_value->type,
                    sizeof(src_value->type));
                status = true;
                break;
        }
        dest_value->next = src_value->next;
    }

    return status;
}

/**
 * @brief Returns the length of data between an opening tag and a closing tag.
 * Expects that the first octet contain the opening tag.
 * Include a value property identifier for context specific data
 * such as the value received in a WriteProperty request.
 *
 * @param Pointer to the APDU buffer
 * @param apdu_len_max Bytes valid in the buffer
 * @param property ID of the property to get the length for.
 *
 * @return Length in bytes or BACNET_STATUS_ERROR.
 */
int bacapp_data_len(
    uint8_t *apdu, unsigned apdu_len_max, BACNET_PROPERTY_ID property)
{
    int len = 0;
    int total_len = 0;
    int apdu_len = 0;
    uint8_t tag_number = 0;
    uint8_t opening_tag_number = 0;
    uint8_t opening_tag_number_counter = 0;
    uint32_t value = 0;

    if (IS_OPENING_TAG(apdu[0])) {
        len = bacnet_tag_number_and_value_decode(
            &apdu[apdu_len], apdu_len_max - apdu_len, &tag_number, &value);
        apdu_len += len;
        opening_tag_number = tag_number;
        opening_tag_number_counter = 1;
        while (opening_tag_number_counter) {
            if (IS_OPENING_TAG(apdu[apdu_len])) {
                len = bacnet_tag_number_and_value_decode(&apdu[apdu_len],
                    apdu_len_max - apdu_len, &tag_number, &value);
                if (tag_number == opening_tag_number) {
                    opening_tag_number_counter++;
                }
            } else if (IS_CLOSING_TAG(apdu[apdu_len])) {
                len = bacnet_tag_number_and_value_decode(&apdu[apdu_len],
                    apdu_len_max - apdu_len, &tag_number, &value);
                if (tag_number == opening_tag_number) {
                    opening_tag_number_counter--;
                }
            } else if (IS_CONTEXT_SPECIFIC(apdu[apdu_len])) {
#if defined(BACAPP_TYPES_EXTRA)
                /* context-specific tagged data */
                len = bacapp_decode_context_data_len(
                    &apdu[apdu_len], apdu_len_max - apdu_len, property);
#endif
            } else {
                /* application tagged data */
                len = bacapp_decode_application_data_len(
                    &apdu[apdu_len], apdu_len_max - apdu_len);
            }
            apdu_len += len;
            if (opening_tag_number_counter) {
                if (len > 0) {
                    total_len += len;
                } else {
                    /* error: len is not incrementing */
                    total_len = BACNET_STATUS_ERROR;
                    break;
                }
            }
            if ((unsigned)apdu_len > apdu_len_max) {
                /* error: exceeding our buffer limit */
                total_len = BACNET_STATUS_ERROR;
                break;
            }
        }
    }

    return total_len;
}

#if defined(BACAPP_DATE)
/* 135.1-4.4 Notational Rules for Parameter Values
(j)
dates are represented enclosed in parenthesis:
(Monday, 24-January-1998).
Any "wild card" or unspecified field is shown by an asterisk (X'2A'):
(Monday, *-January-1998).
The omission of day of week implies that the day is unspecified:
(24-January-1998);
*/
static int bacapp_snprintf_date(char *str, size_t str_len, BACNET_DATE *bdate)
{
    int ret_val = 0;
    int slen = 0;

    /* false positive cppcheck - snprintf allows null pointers */
    /* cppcheck-suppress nullPointer */
    /* cppcheck-suppress ctunullpointer */
    slen = snprintf(str, str_len, "%s, %s",
        bactext_day_of_week_name(bdate->wday),
        bactext_month_name(bdate->month));
    if (str) {
        str += slen;
        if (str_len >= slen) {
            str_len -= slen;
        } else {
            str_len = 0;
        }
    }
    ret_val += slen;
    if (bdate->day == 255) {
        slen = snprintf(str, str_len, " (unspecified), ");
    } else {
        slen = snprintf(str, str_len, " %u, ", (unsigned)bdate->day);
    }
    if (str) {
        str += slen;
        if (str_len >= slen) {
            str_len -= slen;
        } else {
            str_len = 0;
        }
    }
    ret_val += slen;
    if (bdate->year == 2155) {
        slen = snprintf(str, str_len, "(unspecified)");
    } else {
        slen = snprintf(str, str_len, "%u", (unsigned)bdate->year);
    }
    ret_val += slen;

    return ret_val;
}
#endif

#if defined(BACAPP_TIME)
/* 135.1-4.4 Notational Rules for Parameter Values
(k)
times are represented as hours, minutes, seconds, hundredths in the format
hh:mm:ss.xx: 2:05:44.00, 16:54:59.99. Any "wild card" field is shown by an
asterisk (X'2A'): 16:54:*.*; */
static int bacapp_snprintf_time(char *str, size_t str_len, BACNET_TIME *btime)
{
    int ret_val = 0;
    int slen = 0;

    if (btime->hour == 255) {
        slen = snprintf(str, str_len, "**:");
    } else {
        /* false positive cppcheck - snprintf allows null pointers */
        /* cppcheck-suppress nullPointer */
        slen = snprintf(str, str_len, "%02u:", (unsigned)btime->hour);
    }
    if (str) {
        str += slen;
        if (str_len >= slen) {
            str_len -= slen;
        } else {
            str_len = 0;
        }
    }
    ret_val += slen;
    if (btime->min == 255) {
        slen = snprintf(str, str_len, "**:");
    } else {
        slen = snprintf(str, str_len, "%02u:", (unsigned)btime->min);
    }
    if (str) {
        str += slen;
        if (str_len >= slen) {
            str_len -= slen;
        } else {
            str_len = 0;
        }
    }
    ret_val += slen;
    if (btime->sec == 255) {
        slen = snprintf(str, str_len, "**.");
    } else {
        slen = snprintf(str, str_len, "%02u.", (unsigned)btime->sec);
    }
    if (str) {
        str += slen;
        if (str_len >= slen) {
            str_len -= slen;
        } else {
            str_len = 0;
        }
    }
    ret_val += slen;
    if (btime->hundredths == 255) {
        slen = snprintf(str, str_len, "**");
    } else {
        slen = snprintf(str, str_len, "%02u", (unsigned)btime->hundredths);
    }
    ret_val += slen;

    return ret_val;
}
#endif

#if defined(BACAPP_TYPES_EXTRA)
static int bacapp_snprintf_weeklyschedule(char *str,
    size_t str_len,
    BACNET_WEEKLY_SCHEDULE *ws,
    BACNET_ARRAY_INDEX arrayIndex)
{
    int slen;
    int ret_val = 0;
    int wi, ti;
    BACNET_OBJECT_PROPERTY_VALUE dummyPropValue;
    BACNET_APPLICATION_DATA_VALUE dummyDataValue;

    const char *weekdaynames[7] = { "Mon", "Tue", "Wed", "Thu", "Fri", "Sat",
        "Sun" };
    const int loopend = ((arrayIndex == BACNET_ARRAY_ALL) ? 7 : 1);

    // Find what inner type it uses
    int inner_tag = -1;
    for (wi = 0; wi < loopend; wi++) {
        BACNET_DAILY_SCHEDULE *ds = &ws->weeklySchedule[wi];
        for (ti = 0; ti < ds->TV_Count; ti++) {
            int tag = ds->Time_Values[ti].Value.tag;
            if (inner_tag == -1) {
                inner_tag = tag;
            } else if (inner_tag != tag) {
                inner_tag = -2;
            }
        }
    }

    if (inner_tag == -1) {
        slen = snprintf(str, str_len, "(Null; ");
    } else if (inner_tag == -2) {
        slen = snprintf(str, str_len, "(MIXED_TYPES; ");
    } else {
        slen = snprintf(
            str, str_len, "(%s; ", bactext_application_tag_name(inner_tag));
    }
    ret_val += slen;
    if (str) {
        str += slen;
        if (str_len >= slen) {
            str_len -= slen;
        } else {
            str_len = 0;
        }
    }

    for (wi = 0; wi < loopend; wi++) {
        BACNET_DAILY_SCHEDULE *ds = &ws->weeklySchedule[wi];
        if (arrayIndex == BACNET_ARRAY_ALL) {
            slen = snprintf(str, str_len, "%s: [", weekdaynames[wi]);
        } else {
            slen = snprintf(str, str_len, "%s: [",
                (arrayIndex >= 1 && arrayIndex <= 7)
                    ? weekdaynames[arrayIndex - 1]
                    : "???");
        }
        ret_val += slen;
        if (str) {
            str += slen;
            if (str_len >= slen) {
                str_len -= slen;
            } else {
                str_len = 0;
            }
        }

        for (ti = 0; ti < ds->TV_Count; ti++) {
            slen =
                bacapp_snprintf_time(str, str_len, &ds->Time_Values[ti].Time);
            ret_val += slen;
            if (str) {
                str += slen;
                if (str_len >= slen) {
                    str_len -= slen;
                } else {
                    str_len = 0;
                }
            }

            slen = snprintf(str, str_len, " ");
            ret_val += slen;
            if (str) {
                str += slen;
                if (str_len >= slen) {
                    str_len -= slen;
                } else {
                    str_len = 0;
                }
            }

            bacnet_primitive_to_application_data_value(
                &dummyDataValue, &ds->Time_Values[ti].Value);
            dummyPropValue.value = &dummyDataValue;
            dummyPropValue.object_property = PROP_PRESENT_VALUE;
            dummyPropValue.object_type = OBJECT_SCHEDULE;

            slen = bacapp_snprintf_value(str, str_len, &dummyPropValue);
            ret_val += slen;
            if (str) {
                str += slen;
                if (str_len >= slen) {
                    str_len -= slen;
                } else {
                    str_len = 0;
                }
            }

            if (ti < ds->TV_Count - 1) {
                slen = snprintf(str, str_len, ", ");
                ret_val += slen;
                if (str) {
                    str += slen;
                    if (str_len >= slen) {
                        str_len -= slen;
                    } else {
                        str_len = 0;
                    }
                }
            }
        }

        if (wi < loopend - 1) {
            slen = snprintf(str, str_len, "]; ");
            ret_val += slen;
            if (str) {
                str += slen;
                if (str_len >= slen) {
                    str_len -= slen;
                } else {
                    str_len = 0;
                }
            }
        }
    }
    slen = snprintf(str, str_len, "])");
    ret_val += slen;
    return ret_val;
}
#endif

/**
 * @brief Extract the value into a text string
 * @param str - the buffer to store the extracted value, or NULL for length
 * @param str_len - the size of the buffer
 * @param object_value - ptr to BACnet object value from which to extract str
 * @return number of bytes (excluding terminating NULL byte) that were stored
 *  to the output string.
 */
int bacapp_snprintf_value(
    char *str, size_t str_len, BACNET_OBJECT_PROPERTY_VALUE *object_value)
{
    size_t len = 0, i = 0;
    char *char_str;
    BACNET_APPLICATION_DATA_VALUE *value;
    BACNET_PROPERTY_ID property = PROP_ALL;
    BACNET_OBJECT_TYPE object_type = MAX_BACNET_OBJECT_TYPE;
    int ret_val = 0;
    int slen = 0;
#if defined(BACAPP_OCTET_STRING) || defined(BACAPP_TYPES_EXTRA)
    uint8_t *octet_str;
#endif
#ifdef __STDC_ISO_10646__
    /* Wide character (decoded from multi-byte character). */
    wchar_t wc;
    /* Wide character length in bytes. */
    int wclen;
#endif

    if (object_value && object_value->value) {
        value = object_value->value;
        property = object_value->object_property;
        object_type = object_value->object_type;
        switch (value->tag) {
#if defined(BACAPP_NULL)
            case BACNET_APPLICATION_TAG_NULL:
                ret_val = snprintf(str, str_len, "Null");
                break;
#endif
#if defined(BACAPP_BOOLEAN)
            case BACNET_APPLICATION_TAG_BOOLEAN:
                ret_val = (value->type.Boolean)
                    ? snprintf(str, str_len, "TRUE")
                    : snprintf(str, str_len, "FALSE");
                break;
#endif
#if defined(BACAPP_UNSIGNED)
            case BACNET_APPLICATION_TAG_UNSIGNED_INT:
                ret_val = snprintf(str, str_len, "%lu",
                    (unsigned long)value->type.Unsigned_Int);
                break;
#endif
#if defined(BACAPP_SIGNED)
            case BACNET_APPLICATION_TAG_SIGNED_INT:
                ret_val =
                    snprintf(str, str_len, "%ld", (long)value->type.Signed_Int);
                break;
#endif
#if defined(BACAPP_REAL)
            case BACNET_APPLICATION_TAG_REAL:
                ret_val =
                    snprintf(str, str_len, "%f", (double)value->type.Real);
                break;
#endif
#if defined(BACAPP_DOUBLE)
            case BACNET_APPLICATION_TAG_DOUBLE:
                ret_val = snprintf(str, str_len, "%f", value->type.Double);
                break;
#endif
#if defined(BACAPP_OCTET_STRING)
            case BACNET_APPLICATION_TAG_OCTET_STRING:
                len = octetstring_length(&value->type.Octet_String);
                octet_str = octetstring_value(&value->type.Octet_String);
                for (i = 0; i < len; i++) {
                    slen = snprintf(str, str_len, "%02X", *octet_str);
                    octet_str++;
                    if (str) {
                        str += slen;
                        if (str_len >= slen) {
                            str_len -= slen;
                        } else {
                            str_len = 0;
                        }
                    }
                    ret_val += slen;
                }
                break;
#endif
#if defined(BACAPP_CHARACTER_STRING)
            case BACNET_APPLICATION_TAG_CHARACTER_STRING:
                len = characterstring_length(&value->type.Character_String);
                char_str = characterstring_value(&value->type.Character_String);
                slen = snprintf(str, str_len, "\"");
                if (str) {
                    str += slen;
                    if (str_len >= slen) {
                        str_len -= slen;
                    } else {
                        str_len = 0;
                    }
                }
                ret_val += slen;
#ifdef __STDC_ISO_10646__
                if (characterstring_encoding(&value->type.Character_String) ==
                    CHARACTER_UTF8) {
                    while (len > 0) {
                        wclen = mbtowc(&wc, char_str, MB_CUR_MAX);
                        if (wclen == -1) {
                            /* Encoding error, reset state: */
                            mbtowc(NULL, NULL, MB_CUR_MAX);
                            /* After handling an invalid byte,
                               retry with the next one. */
                            wclen = 1;
                            wc = L'?';
                        } else {
                            if (!iswprint(wc)) {
                                wc = L'.';
                            }
                        }
                        /* For portability, cast wchar_t to wint_t */
                        slen = snprintf(str, str_len, "%lc", (wint_t)wc);
                        if (str) {
                            str += slen;
                            if (str_len >= slen) {
                                str_len -= slen;
                            } else {
                                str_len = 0;
                            }
                        }
                        ret_val += slen;
                        if (len > wclen) {
                            len -= wclen;
                            char_str += wclen;
                        } else {
                            len = 0;
                        }
                    }
                } else
#endif
                {
                    for (i = 0; i < len; i++) {
                        if (isprint(*((unsigned char *)char_str))) {
                            slen = snprintf(str, str_len, "%c", *char_str);
                        } else {
                            slen = snprintf(str, str_len, "%c", '.');
                        }
                        if (str) {
                            str += slen;
                            if (str_len >= slen) {
                                str_len -= slen;
                            } else {
                                str_len = 0;
                            }
                        }
                        ret_val += slen;
                        char_str++;
                    }
                }
                slen = snprintf(str, str_len, "\"");
                ret_val += slen;
                break;
#endif
#if defined(BACAPP_BIT_STRING)
            case BACNET_APPLICATION_TAG_BIT_STRING:
                len = bitstring_bits_used(&value->type.Bit_String);
                slen = snprintf(str, str_len, "{");
                if (str) {
                    str += slen;
                    if (str_len >= slen) {
                        str_len -= slen;
                    } else {
                        str_len = 0;
                    }
                }
                ret_val += slen;
                for (i = 0; i < len; i++) {
                    bool bit;
                    bit = bitstring_bit(&value->type.Bit_String, (uint8_t)i);
                    slen = snprintf(str, str_len, "%s", bit ? "true" : "false");
                    if (str) {
                        str += slen;
                        if (str_len >= slen) {
                            str_len -= slen;
                        } else {
                            str_len = 0;
                        }
                    }
                    ret_val += slen;
                    if (i < (len - 1)) {
                        slen = snprintf(str, str_len, ",");
                        if (str) {
                            str += slen;
                            if (str_len >= slen) {
                                str_len -= slen;
                            } else {
                                str_len = 0;
                            }
                        }
                        ret_val += slen;
                    }
                }
                slen = snprintf(str, str_len, "}");
                ret_val += slen;
                break;
#endif
#if defined(BACAPP_ENUMERATED)
            case BACNET_APPLICATION_TAG_ENUMERATED:
                switch (property) {
                    case PROP_PROPERTY_LIST:
                        char_str = (char *)bactext_property_name_default(
                            value->type.Enumerated, NULL);
                        if (char_str) {
                            ret_val = snprintf(str, str_len, "%s", char_str);
                        } else {
                            ret_val = snprintf(str, str_len, "%lu",
                                (unsigned long)value->type.Enumerated);
                        }
                        break;
                    case PROP_OBJECT_TYPE:
                        if (value->type.Enumerated <= BACNET_OBJECT_TYPE_LAST) {
                            ret_val = snprintf(str, str_len, "%s",
                                bactext_object_type_name(
                                    value->type.Enumerated));
                        } else if (value->type.Enumerated <=
                            BACNET_OBJECT_TYPE_RESERVED_MAX) {
                            ret_val = snprintf(str, str_len, "reserved %lu",
                                (unsigned long)value->type.Enumerated);
                        } else {
                            ret_val = snprintf(str, str_len, "proprietary %lu",
                                (unsigned long)value->type.Enumerated);
                        }
                        break;
                    case PROP_EVENT_STATE:
                        ret_val = snprintf(str, str_len, "%s",
                            bactext_event_state_name(value->type.Enumerated));
                        break;
                    case PROP_UNITS:
                        if (bactext_engineering_unit_name_proprietary(
                                (unsigned)value->type.Enumerated)) {
                            ret_val = snprintf(str, str_len, "proprietary %lu",
                                (unsigned long)value->type.Enumerated);
                        } else {
                            ret_val = snprintf(str, str_len, "%s",
                                bactext_engineering_unit_name(
                                    value->type.Enumerated));
                        }
                        break;
                    case PROP_POLARITY:
                        ret_val = snprintf(str, str_len, "%s",
                            bactext_binary_polarity_name(
                                value->type.Enumerated));
                        break;
                    case PROP_PRESENT_VALUE:
                    case PROP_RELINQUISH_DEFAULT:
                        if (object_type < OBJECT_PROPRIETARY_MIN) {
                            ret_val = snprintf(str, str_len, "%s",
                                bactext_binary_present_value_name(
                                    value->type.Enumerated));
                        } else {
                            ret_val = snprintf(str, str_len, "%lu",
                                (unsigned long)value->type.Enumerated);
                        }
                        break;
                    case PROP_RELIABILITY:
                        ret_val = snprintf(str, str_len, "%s",
                            bactext_reliability_name(value->type.Enumerated));
                        break;
                    case PROP_SYSTEM_STATUS:
                        ret_val = snprintf(str, str_len, "%s",
                            bactext_device_status_name(value->type.Enumerated));
                        break;
                    case PROP_SEGMENTATION_SUPPORTED:
                        ret_val = snprintf(str, str_len, "%s",
                            bactext_segmentation_name(value->type.Enumerated));
                        break;
                    case PROP_NODE_TYPE:
                        ret_val = snprintf(str, str_len, "%s",
                            bactext_node_type_name(value->type.Enumerated));
                        break;
                    default:
                        ret_val = snprintf(str, str_len, "%lu",
                            (unsigned long)value->type.Enumerated);
                        break;
                }
                break;
#endif
#if defined(BACAPP_DATE)
            case BACNET_APPLICATION_TAG_DATE:
                ret_val = bacapp_snprintf_date(str, str_len, &value->type.Date);
                break;
#endif
#if defined(BACAPP_TIME)
            case BACNET_APPLICATION_TAG_TIME:
                ret_val = bacapp_snprintf_time(str, str_len, &value->type.Time);
                break;
#endif
#if defined(BACAPP_OBJECT_ID)
            case BACNET_APPLICATION_TAG_OBJECT_ID:
                slen = snprintf(str, str_len, "(");
                if (str) {
                    str += slen;
                    if (str_len >= slen) {
                        str_len -= slen;
                    } else {
                        str_len = 0;
                    }
                }
                ret_val += slen;
                if (value->type.Object_Id.type <= BACNET_OBJECT_TYPE_LAST) {
                    slen = snprintf(str, str_len, "%s, ",
                        bactext_object_type_name(value->type.Object_Id.type));
                } else if (value->type.Object_Id.type <
                    BACNET_OBJECT_TYPE_RESERVED_MAX) {
                    slen = snprintf(str, str_len, "reserved %u, ",
                        (unsigned)value->type.Object_Id.type);
                } else {
                    slen = snprintf(str, str_len, "proprietary %u, ",
                        (unsigned)value->type.Object_Id.type);
                }
                if (str) {
                    str += slen;
                    if (str_len >= slen) {
                        str_len -= slen;
                    } else {
                        str_len = 0;
                    }
                }
                ret_val += slen;
                slen = snprintf(str, str_len, "%lu)",
                    (unsigned long)value->type.Object_Id.instance);
                ret_val += slen;
                break;
#endif
#if defined(BACAPP_TYPES_EXTRA)
            case BACNET_APPLICATION_TAG_DATETIME:
                slen = bacapp_snprintf_date(str, str_len, &value->type.Date);
                ret_val += slen;
                if (str) {
                    str += slen;
                    if (str_len >= slen) {
                        str_len -= slen;
                    } else {
                        str_len = 0;
                    }
                }
                slen = bacapp_snprintf_time(str, str_len, &value->type.Time);
                ret_val += slen;
                break;
            case BACNET_APPLICATION_TAG_TIMESTAMP:
                //ISO 8601 format
                slen = snprintf(str, str_len, "%04u-%02u-%02uT%02u:%02u:%02u.%03u",
                    (unsigned) value->type.Time_Stamp.value.dateTime.date.year,
                    (unsigned) value->type.Time_Stamp.value.dateTime.date.month,
                    (unsigned) value->type.Time_Stamp.value.dateTime.date.day,
                    (unsigned) value->type.Time_Stamp.value.dateTime.time.hour,
                    (unsigned) value->type.Time_Stamp.value.dateTime.time.min,
                    (unsigned) value->type.Time_Stamp.value.dateTime.time.sec,
                    (unsigned) value->type.Time_Stamp.value.dateTime.time.hundredths);
                ret_val += slen;
                break;
            case BACNET_APPLICATION_TAG_LIGHTING_COMMAND:
                slen = snprintf(str, str_len, "(");
                if (str) {
                    str += slen;
                    if (str_len >= slen) {
                        str_len -= slen;
                    } else {
                        str_len = 0;
                    }
                }
                ret_val += slen;
                slen = snprintf(str, str_len, "%s",
                    bactext_lighting_operation_name(
                        value->type.Lighting_Command.operation));
                if (str) {
                    str += slen;
                    if (str_len >= slen) {
                        str_len -= slen;
                    } else {
                        str_len = 0;
                    }
                }
                ret_val += slen;
                /* FIXME: add the Lighting Command optional values */
                slen = snprintf(str, str_len, ")");
                ret_val += slen;
                break;
            case BACNET_APPLICATION_TAG_XY_COLOR:
                /* BACnetxyColor */
                ret_val = snprintf(str, str_len, "(%f,%f)",
                    value->type.XY_Color.x_coordinate,
                    value->type.XY_Color.x_coordinate);
                break;
            case BACNET_APPLICATION_TAG_COLOR_COMMAND:
                /* BACnetColorCommand */
                slen = snprintf(str, str_len, "(");
                if (str) {
                    str += slen;
                    if (str_len >= slen) {
                        str_len -= slen;
                    } else {
                        str_len = 0;
                    }
                }
                ret_val += slen;
                slen = snprintf(str, str_len, "%s",
                    bactext_color_operation_name(
                        value->type.Color_Command.operation));
                if (str) {
                    str += slen;
                    if (str_len >= slen) {
                        str_len -= slen;
                    } else {
                        str_len = 0;
                    }
                }
                ret_val += slen;
                /* FIXME: add the Lighting Command optional values */
                slen = snprintf(str, str_len, ")");
                ret_val += slen;
                break;
            case BACNET_APPLICATION_TAG_WEEKLY_SCHEDULE:
                /* BACnetWeeklySchedule */
                ret_val = bacapp_snprintf_weeklyschedule(str, str_len,
                    &value->type.Weekly_Schedule, object_value->array_index);
                break;
            case BACNET_APPLICATION_TAG_HOST_N_PORT:
                if (value->type.Host_Address.host_ip_address) {
                    octet_str = octetstring_value(
                        &value->type.Host_Address.host.ip_address);
                    slen = snprintf(str, str_len, "%u.%u.%u.%u:%u",
                        (unsigned)octet_str[0], (unsigned)octet_str[1],
                        (unsigned)octet_str[2], (unsigned)octet_str[3],
                        (unsigned)value->type.Host_Address.port);
                    ret_val += slen;
                } else if (value->type.Host_Address.host_name) {
                    BACNET_CHARACTER_STRING *name;
                    name = &value->type.Host_Address.host.name;
                    len = characterstring_length(name);
                    char_str = characterstring_value(name);
                    slen = snprintf(str, str_len, "\"");
                    if (str) {
                        str += slen;
                        if (str_len >= slen) {
                            str_len -= slen;
                        } else {
                            str_len = 0;
                        }
                    }
                    ret_val += slen;
                    for (i = 0; i < len; i++) {
                        if (isprint(*((unsigned char *)char_str))) {
                            slen = snprintf(str, str_len, "%c", *char_str);
                        } else {
                            slen = snprintf(str, str_len, "%c", '.');
                        }
                        char_str++;
                        if (str) {
                            str += slen;
                            if (str_len >= slen) {
                                str_len -= slen;
                            } else {
                                str_len = 0;
                            }
                        }
                        ret_val += slen;
                    }
                    slen = snprintf(str, str_len, "\"");
                    ret_val += slen;
                }
                break;
#endif
            default:
                ret_val =
                    snprintf(str, str_len, "UnknownType(tag=%d)", value->tag);
                break;
        }
    }

    return ret_val;
}

#ifdef BACAPP_PRINT_ENABLED
/**
 * Print the extracted value from the requested BACnet object property to the
 * specified stream. If stream is NULL, do not print anything. If extraction
 * failed, do not print anything. Return the status of the extraction.
 *
 * @param stream - the I/O stream send the printed value.
 * @param object_value - ptr to BACnet object value from which to extract str
 *
 * @return true if the value was sent to the stream
 */
bool bacapp_print_value(
    FILE *stream, BACNET_OBJECT_PROPERTY_VALUE *object_value)
{
    bool retval = false;
    int str_len = 0;

    /* get the string length first */
    str_len = bacapp_snprintf_value(NULL, 0, object_value);
    if (str_len > 0) {
#if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 199901L
        char str[str_len + 1];
#else
        char *str;
        str = calloc(sizeof(char), str_len + 1);
        if (!str) {
            return false;
        }
#endif
        bacapp_snprintf_value(str, str_len + 1, object_value);
        if (stream) {
            fprintf(stream, "%s", str);
        }
#if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 199901L
        /* nothing to do with stack based RAM */
#else
        if (str) {
            free(str);
        }
#endif
        retval = true;
    }

    return retval;
}
#endif

#ifdef BACAPP_PRINT_ENABLED
static char *ltrim(char *str, const char *trimmedchars)
{
    if (str[0] == 0) {
        return str;
    }
    while (strchr(trimmedchars, *str)) {
        str++;
    }
    return str;
}

static char *rtrim(char *str, const char *trimmedchars)
{
    if (str[0] == 0) {
        return str;
    }
    char *end = str + strlen(str) - 1;
    while (strchr(trimmedchars, *end)) {
        *end = 0;
        if (end == str)
            break;
        end--;
    }
    return str;
}

static char *trim(char *str, const char *trimmedchars)
{
    return ltrim(rtrim(str, trimmedchars), trimmedchars);
}

#if defined(BACAPP_TYPES_EXTRA)
static bool parse_weeklyschedule(
    char *str, BACNET_APPLICATION_DATA_VALUE *value)
{
    char *chunk, *comma, *space, *t, *v;
    int daynum = 0, tvnum = 0;
    unsigned int inner_tag;
    BACNET_APPLICATION_DATA_VALUE dummy_value = { 0 };
    BACNET_DAILY_SCHEDULE *dsch;

    /*
     Format:

     (1; Mon: [02:00:00.00 FALSE, 07:35:00.00 active, 07:40:00.00 inactive];
     Tue: [02:00:00.00 inactive]; ...)

     - the first number is the inner tag (e.g. 1 = boolean, 4 = real, 9 = enum)
     - Day name prefix is optional and ignored.
     - Entries are separated by semicolons.
     - There can be a full week, or only one entry - when using array index to
     modify a single day
     - time-value array can be empty: []
    */

    value->tag = BACNET_APPLICATION_TAG_WEEKLY_SCHEDULE;

    // Parse the inner tag
    chunk = strtok(str, ";");
    chunk = ltrim(chunk, "(");
    if (false ==
        bacapp_parse_application_data(
            BACNET_APPLICATION_TAG_UNSIGNED_INT, chunk, &dummy_value)) {
        // Try searching it by name
        if (false == bactext_application_tag_index(chunk, &inner_tag)) {
            return false;
        }
    } else {
        inner_tag = (int)dummy_value.type.Unsigned_Int;
    }

    chunk = strtok(NULL, ";");

    while (chunk != NULL) {
        dsch = &value->type.Weekly_Schedule.weeklySchedule[daynum];

        // Strip day name prefix, if present
        char *colonpos = strchr(chunk, ':');
        char *sqpos = strchr(chunk, '[');
        if (colonpos && colonpos < sqpos) {
            chunk = colonpos + 1;
        }

        // Extract the inner list of time-values
        chunk = rtrim(ltrim(chunk, "([ "), " ])");

        // The list can be empty
        if (chunk[0] != 0) {
            // loop through the time value pairs
            tvnum = 0;
            do {
                // Find the comma delimiter, replace with NUL (like strtok)
                comma = strchr(chunk, ',');
                if (comma) {
                    *comma = 0;
                }
                // trim the time-value pair and find the delimiter space
                chunk = trim(chunk, " ");
                space = strchr(chunk, ' ');
                if (!space) {
                    // malformed time-value pair
                    return false;
                }
                *space = 0;

                // Extract time and value
                t = chunk;
                // value starts one byte after the space, and there can be
                // multiple spaces
                chunk = ltrim(space + 1, " ");
                v = chunk;

                // Parse time
                if (false ==
                    bacapp_parse_application_data(
                        BACNET_APPLICATION_TAG_TIME, t, &dummy_value)) {
                    return false;
                }
                dsch->Time_Values[tvnum].Time = dummy_value.type.Time;

                // Parse value
                if (false ==
                    bacapp_parse_application_data(inner_tag, v, &dummy_value)) {
                    return false;
                }
                if (BACNET_STATUS_OK !=
                    bacnet_application_to_primitive_data_value(
                        &dsch->Time_Values[tvnum].Value, &dummy_value)) {
                    return false;
                }

                // Advance past the comma to the next chunk
                if (comma) {
                    chunk = comma + 1;
                }
                tvnum++;
            } while (comma != NULL);
        }

        dsch->TV_Count = tvnum;

        // Find the start of the next day
        chunk = strtok(NULL, ";");
        daynum++;
    }

    if (daynum == 1) {
        value->type.Weekly_Schedule.singleDay = true;
    }

    return true;
}
#endif

#if defined(BACAPP_SIGNED) || defined(BACAPP_BOOLEAN)
static bool strtol_checked(const char *s, long *out)
{
    char *end;
    errno = 0;
    *out = strtol(s, &end, 0);
    if (end == s) {
        // Conversion was not possible
        return false;
    }
    if (errno == ERANGE) {
        // Number too large
        return false;
    }
    return true;
}
#endif

#if defined(BACAPP_UNSIGNED) || defined(BACAPP_ENUMERATED)
static bool strtoul_checked(const char *s, BACNET_UNSIGNED_INTEGER *out)
{
    char *end;
    errno = 0;
    *out = strtoul(s, &end, 0);
    if (end == s) {
        // Conversion was not possible
        return false;
    }
    if (errno == ERANGE) {
        // Number too large
        return false;
    }
    return true;
}
#endif

#if defined(BACAPP_REAL) || defined(BACAPP_DOUBLE)
static bool strtod_checked(const char *s, double *out)
{
    char *end;
    errno = 0;
    *out = strtod(s, &end);
    if (end == s) {
        // Conversion was not possible
        return false;
    }
    if (errno == ERANGE) {
        // Number too large
        return false;
    }
    return true;
}
#endif

/* used to load the app data struct with the proper data
   converted from a command line argument.
   "argv" is not const to allow using strtok internally. It MAY be modified. */
bool bacapp_parse_application_data(BACNET_APPLICATION_TAG tag_number,
    char *argv,
    BACNET_APPLICATION_DATA_VALUE *value)
{
    int hour, min, sec, hundredths;
    int year, month, day, wday;
    int object_type = 0;
    uint32_t instance = 0;
    bool status = false;
    long long_value = 0;
    BACNET_UNSIGNED_INTEGER unsigned_long_value = 0;
    double double_value = 0.0;
    int count = 0;
#if defined(BACAPP_TYPES_EXTRA)
    unsigned a[4] = { 0 }, p = 0;
    float x, y;
#endif

    if (value && (tag_number != MAX_BACNET_APPLICATION_TAG)) {
        status = true;
        value->tag = tag_number;
        switch (tag_number) {
#if defined(BACAPP_BOOLEAN)
            case BACNET_APPLICATION_TAG_BOOLEAN:
                if (strcasecmp(argv, "true") == 0 ||
                    strcasecmp(argv, "active") == 0) {
                    value->type.Boolean = true;
                } else if (strcasecmp(argv, "false") == 0 ||
                    strcasecmp(argv, "inactive") == 0) {
                    value->type.Boolean = false;
                } else {
                    status = strtol_checked(argv, &long_value);
                    if (!status) {
                        return false;
                    }
                    if (long_value) {
                        value->type.Boolean = true;
                    } else {
                        value->type.Boolean = false;
                    }
                }
                break;
#endif
#if defined(BACAPP_UNSIGNED)
            case BACNET_APPLICATION_TAG_UNSIGNED_INT:
                status = strtoul_checked(argv, &unsigned_long_value);
                if (!status) {
                    return false;
                }
                if (unsigned_long_value > BACNET_UNSIGNED_INTEGER_MAX) {
                    return false;
                }
                value->type.Unsigned_Int = unsigned_long_value;
                break;
#endif
#if defined(BACAPP_SIGNED)
            case BACNET_APPLICATION_TAG_SIGNED_INT:
                status = strtol_checked(argv, &long_value);
                if (!status || long_value > INT32_MAX ||
                    long_value < INT32_MIN) {
                    return false;
                }
                value->type.Signed_Int = (int32_t)long_value;
                break;
#endif
#if defined(BACAPP_REAL)
            case BACNET_APPLICATION_TAG_REAL:
                status = strtod_checked(argv, &double_value);
                if (!status) {
                    return false;
                }
                value->type.Real = (float)double_value;
                break;
#endif
#if defined(BACAPP_DOUBLE)
            case BACNET_APPLICATION_TAG_DOUBLE:
                status = strtod_checked(argv, &double_value);
                if (!status) {
                    return false;
                }
                value->type.Double = double_value;
                break;
#endif
#if defined(BACAPP_OCTET_STRING)
            case BACNET_APPLICATION_TAG_OCTET_STRING:
                status =
                    octetstring_init_ascii_hex(&value->type.Octet_String, argv);
                break;
#endif
#if defined(BACAPP_CHARACTER_STRING)
            case BACNET_APPLICATION_TAG_CHARACTER_STRING:
                status = characterstring_init_ansi(
                    &value->type.Character_String, (char *)argv);
                break;
#endif
#if defined(BACAPP_BIT_STRING)
            case BACNET_APPLICATION_TAG_BIT_STRING:
                status = bitstring_init_ascii(&value->type.Bit_String, argv);
                break;
#endif
#if defined(BACAPP_ENUMERATED)
            case BACNET_APPLICATION_TAG_ENUMERATED:
                status = strtoul_checked(argv, &unsigned_long_value);
                if (!status || unsigned_long_value > UINT32_MAX) {
                    return false;
                }
                value->type.Enumerated = (uint32_t)unsigned_long_value;
                break;
#endif
#if defined(BACAPP_DATE)
            case BACNET_APPLICATION_TAG_DATE:
                count =
                    sscanf(argv, "%4d/%3d/%3d:%3d", &year, &month, &day, &wday);
                if (count == 3) {
                    datetime_set_date(&value->type.Date, (uint16_t)year,
                        (uint8_t)month, (uint8_t)day);
                } else if (count == 4) {
                    value->type.Date.year = (uint16_t)year;
                    value->type.Date.month = (uint8_t)month;
                    value->type.Date.day = (uint8_t)day;
                    value->type.Date.wday = (uint8_t)wday;
                } else {
                    status = false;
                }
                break;
#endif
#if defined(BACAPP_TIME)
            case BACNET_APPLICATION_TAG_TIME:
                count = sscanf(
                    argv, "%3d:%3d:%3d.%3d", &hour, &min, &sec, &hundredths);
                if (count == 4) {
                    value->type.Time.hour = (uint8_t)hour;
                    value->type.Time.min = (uint8_t)min;
                    value->type.Time.sec = (uint8_t)sec;
                    value->type.Time.hundredths = (uint8_t)hundredths;
                } else if (count == 3) {
                    value->type.Time.hour = (uint8_t)hour;
                    value->type.Time.min = (uint8_t)min;
                    value->type.Time.sec = (uint8_t)sec;
                    value->type.Time.hundredths = 0;
                } else if (count == 2) {
                    value->type.Time.hour = (uint8_t)hour;
                    value->type.Time.min = (uint8_t)min;
                    value->type.Time.sec = 0;
                    value->type.Time.hundredths = 0;
                } else {
                    status = false;
                }
                break;
#endif
#if defined(BACAPP_OBJECT_ID)
            case BACNET_APPLICATION_TAG_OBJECT_ID:
                count = sscanf(argv, "%4d:%7u", &object_type, &instance);
                if (count == 2) {
                    value->type.Object_Id.type = (uint16_t)object_type;
                    value->type.Object_Id.instance = instance;
                } else {
                    status = false;
                }
                break;
#endif
#if defined(BACAPP_TYPES_EXTRA)
            case BACNET_APPLICATION_TAG_LIGHTING_COMMAND:
                /* FIXME: add parsing for lighting command */
                break;
            case BACNET_APPLICATION_TAG_XY_COLOR:
                /* BACnetxyColor */
                count = sscanf(argv, "%f,%f", &x, &y);
                if (count == 2) {
                    value->type.XY_Color.x_coordinate = x;
                    value->type.XY_Color.y_coordinate = y;
                } else {
                    status = false;
                }
                break;
            case BACNET_APPLICATION_TAG_COLOR_COMMAND:
                /* FIXME: add parsing for BACnetColorCommand */
                break;
            case BACNET_APPLICATION_TAG_WEEKLY_SCHEDULE:
                status = parse_weeklyschedule(argv, value);
                break;
            case BACNET_APPLICATION_TAG_HOST_N_PORT:
                count = sscanf(argv, "%3u.%3u.%3u.%3u:%5u", &a[0], &a[1], &a[2],
                    &a[3], &p);
                if ((count == 4) || (count == 5)) {
                    uint8_t address[4];
                    value->type.Host_Address.host_ip_address = true;
                    value->type.Host_Address.host_name = false;
                    address[0] = (uint8_t)a[0];
                    address[1] = (uint8_t)a[1];
                    address[2] = (uint8_t)a[2];
                    address[3] = (uint8_t)a[3];
                    octetstring_init(
                        &value->type.Host_Address.host.ip_address, address, 4);
                    if (count == 4) {
                        value->type.Host_Address.port = 0xBAC0U;
                    } else {
                        value->type.Host_Address.port = (uint16_t)p;
                    }
                    status = true;
                } else {
                    status = false;
                }
                break;
#endif
            default:
                break;
        }
        value->next = NULL;
    }

    return status;
}
#endif /* BACAPP_PRINT_ENABLED */

/**
 * Initialize an array (or single) #BACNET_APPLICATION_DATA_VALUE
 *
 * @param value - one or more #BACNET_APPLICATION_DATA_VALUE elements
 * @param count - number of #BACNET_APPLICATION_DATA_VALUE elements
 */
void bacapp_value_list_init(BACNET_APPLICATION_DATA_VALUE *value, size_t count)
{
    size_t i = 0;

    if (value && count) {
        for (i = 0; i < count; i++) {
            value->tag = BACNET_APPLICATION_TAG_NULL;
            value->context_specific = 0;
            value->context_tag = 0;
            if ((i + 1) < count) {
                value->next = value + 1;
            } else {
                value->next = NULL;
            }
            value++;
        }
    }
}

/**
 * Initialize an array (or single) #BACNET_PROPERTY_VALUE
 *
 * @param value - one or more #BACNET_PROPERTY_VALUE elements
 * @param count - number of #BACNET_PROPERTY_VALUE elements
 */
void bacapp_property_value_list_init(BACNET_PROPERTY_VALUE *value, size_t count)
{
    size_t i = 0;

    if (value && count) {
        for (i = 0; i < count; i++) {
            value->propertyIdentifier = MAX_BACNET_PROPERTY_ID;
            value->propertyArrayIndex = BACNET_ARRAY_ALL;
            value->priority = BACNET_NO_PRIORITY;
            bacapp_value_list_init(&value->value, 1);
            if ((i + 1) < count) {
                value->next = value + 1;
            } else {
                value->next = NULL;
            }
            value++;
        }
    }
}

/* generic - can be used by other unit tests
   returns true if matching or same, false if different */
bool bacapp_same_value(BACNET_APPLICATION_DATA_VALUE *value,
    BACNET_APPLICATION_DATA_VALUE *test_value)
{
    bool status = false; /*return value */

    /* does the tag match? */
    if ((value == NULL) || (test_value == NULL)) {
        return false;
    }
    if (test_value->tag == value->tag) {
        status = true;
    }
    if (status) {
        /* second test for same-ness */
        status = false;
        /* does the value match? */
        switch (test_value->tag) {
#if defined(BACAPP_NULL)
            case BACNET_APPLICATION_TAG_NULL:
                status = true;
                break;
#endif
#if defined(BACAPP_BOOLEAN)
            case BACNET_APPLICATION_TAG_BOOLEAN:
                if (test_value->type.Boolean == value->type.Boolean) {
                    status = true;
                }
                break;
#endif
#if defined(BACAPP_UNSIGNED)
            case BACNET_APPLICATION_TAG_UNSIGNED_INT:
                if (test_value->type.Unsigned_Int == value->type.Unsigned_Int) {
                    status = true;
                }
                break;
#endif
#if defined(BACAPP_SIGNED)
            case BACNET_APPLICATION_TAG_SIGNED_INT:
                if (test_value->type.Signed_Int == value->type.Signed_Int) {
                    status = true;
                }
                break;
#endif
#if defined(BACAPP_REAL)
            case BACNET_APPLICATION_TAG_REAL:
                if (test_value->type.Real == value->type.Real) {
                    status = true;
                }
                break;
#endif
#if defined(BACAPP_DOUBLE)
            case BACNET_APPLICATION_TAG_DOUBLE:
                if (test_value->type.Double == value->type.Double) {
                    status = true;
                }
                break;
#endif
#if defined(BACAPP_ENUMERATED)
            case BACNET_APPLICATION_TAG_ENUMERATED:
                if (test_value->type.Enumerated == value->type.Enumerated) {
                    status = true;
                }
                break;
#endif
#if defined(BACAPP_DATE)
            case BACNET_APPLICATION_TAG_DATE:
                if (datetime_compare_date(
                        &test_value->type.Date, &value->type.Date) == 0) {
                    status = true;
                }
                break;
#endif
#if defined(BACAPP_TIME)
            case BACNET_APPLICATION_TAG_TIME:
                if (datetime_compare_time(
                        &test_value->type.Time, &value->type.Time) == 0) {
                    status = true;
                }
                break;
#endif
#if defined(BACAPP_OBJECT_ID)
            case BACNET_APPLICATION_TAG_OBJECT_ID:
                if ((test_value->type.Object_Id.type ==
                        value->type.Object_Id.type) &&
                    (test_value->type.Object_Id.instance ==
                        value->type.Object_Id.instance)) {
                    status = true;
                }
                break;
#endif
#if defined(BACAPP_CHARACTER_STRING)
            case BACNET_APPLICATION_TAG_CHARACTER_STRING:
                status = characterstring_same(&value->type.Character_String,
                    &test_value->type.Character_String);
                break;
#endif
#if defined(BACAPP_OCTET_STRING)
            case BACNET_APPLICATION_TAG_OCTET_STRING:
                status = octetstring_value_same(
                    &value->type.Octet_String, &test_value->type.Octet_String);
                break;
#endif
#if defined(BACAPP_BIT_STRING)
            case BACNET_APPLICATION_TAG_BIT_STRING:
                status = bitstring_same(
                    &value->type.Bit_String, &test_value->type.Bit_String);
                break;
#endif
#if defined(BACAPP_TYPES_EXTRA)
            case BACNET_APPLICATION_TAG_DATETIME:
                if (datetime_compare(&value->type.Date_Time,
                        &test_value->type.Date_Time) == 0) {
                    status = true;
                }
                break;
            case BACNET_APPLICATION_TAG_LIGHTING_COMMAND:
                status = lighting_command_same(&value->type.Lighting_Command,
                    &test_value->type.Lighting_Command);
                break;
            case BACNET_APPLICATION_TAG_XY_COLOR:
                /* BACnetxyColor */
                status = xy_color_same(
                    &value->type.XY_Color, &test_value->type.XY_Color);
                break;
            case BACNET_APPLICATION_TAG_COLOR_COMMAND:
                /* BACnetColorCommand */
                status = color_command_same(&value->type.Color_Command,
                    &test_value->type.Color_Command);
                break;
            case BACNET_APPLICATION_TAG_WEEKLY_SCHEDULE:
                /* BACnetWeeklySchedule */
                status =
                    bacnet_weeklyschedule_same(&value->type.Weekly_Schedule,
                        &test_value->type.Weekly_Schedule);
                break;
            case BACNET_APPLICATION_TAG_HOST_N_PORT:
                status = host_n_port_same(
                    &value->type.Host_Address, &value->type.Host_Address);
                break;
#endif
            default:
                status = false;
                break;
        }
    }
    return status;
}
