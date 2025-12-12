/**
 * @file
 * @brief Utilities for the BACnet_Application_Data_Value
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date 2005
 * @copyright SPDX-License-Identifier: GPL-2.0-or-later WITH GCC-exception-2.0
 */
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h> /* for strtol */
#include <ctype.h> /* for isalnum */
#include <math.h>
#if (__STDC_VERSION__ >= 199901L) && defined(__STDC_ISO_10646__)
#include <wchar.h>
#include <wctype.h>
#endif
/* BACnet Stack defines - first */
#include "bacnet/bacdef.h"
/* BACnet Stack API */
#include "bacnet/access_rule.h"
#include "bacnet/bacdcode.h"
#include "bacnet/bacint.h"
#include "bacnet/baclog.h"
#include "bacnet/bacreal.h"
#include "bacnet/bacapp.h"
#include "bacnet/bactext.h"
#include "bacnet/datetime.h"
#include "bacnet/bacstr.h"
#include "bacnet/bacaction.h"
#include "bacnet/lighting.h"
#include "bacnet/hostnport.h"
#include "bacnet/secure_connect.h"
#include "bacnet/weeklyschedule.h"
#include "bacnet/calendar_entry.h"
#include "bacnet/special_event.h"
#include "bacnet/channel_value.h"
#include "bacnet/timer_value.h"
#include "bacnet/basic/sys/platform.h"

#if defined(BACAPP_SCALE)
/**
 * @brief Encode a BACnetScale value.
 *
 *  BACnetScale ::= CHOICE {
 *      float-scale [0] REAL,
 *      integer-scale [1] INTEGER
 *  }
 *
 * @param apdu - buffer to encode to
 * @param value - value to encode
 * @return number of bytes encoded
 */
static int bacnet_scale_encode(uint8_t *apdu, const BACNET_SCALE *value)
{
    int apdu_len = 0;

    if (value) {
        if (value->float_scale) {
            apdu_len += encode_context_real(apdu, 0, value->type.real_scale);
        } else {
            apdu_len +=
                encode_context_signed(apdu, 1, value->type.integer_scale);
        }
    }

    return apdu_len;
}
#endif

#if defined(BACAPP_SCALE)
/**
 * @brief Decode a BACnetScale value.
 *
 *  BACnetScale ::= CHOICE {
 *      float-scale [0] REAL,
 *      integer-scale [1] INTEGER
 *  }
 *
 * @param apdu - buffer to decode to
 * @param apdu_size - size of the buffer
 * @param value - value to encode
 * @return number of bytes decoded, or BACNET_STATUS_ERROR on error
 */
static int
bacnet_scale_decode(const uint8_t *apdu, size_t apdu_size, BACNET_SCALE *value)
{
    int apdu_len = 0;
    BACNET_TAG tag = { 0 };
    int32_t signed_value = 0;
    float real_value = 0.0f;

    if (!apdu) {
        return BACNET_STATUS_ERROR;
    }
    apdu_len = bacnet_tag_decode(apdu, apdu_size, &tag);
    if (apdu_len <= 0) {
        return BACNET_STATUS_ERROR;
    }
    switch (tag.number) {
        case 0:
            apdu_len = bacnet_real_context_decode(
                apdu, apdu_size, tag.number, &real_value);
            if (apdu_len > 0) {
                value->float_scale = true;
                value->type.real_scale = real_value;
            }
            break;
        case 1:
            apdu_len = bacnet_signed_context_decode(
                apdu, apdu_size, tag.number, &signed_value);
            if (apdu_len > 0) {
                value->float_scale = false;
                value->type.integer_scale = signed_value;
            }
            break;
        default:
            return BACNET_STATUS_ERROR;
    }

    return apdu_len;
}
#endif

#if defined(BACAPP_SCALE)
static bool
bacnet_scale_same(const BACNET_SCALE *value1, const BACNET_SCALE *value2)
{
    bool status = false;

    if (value1 && value2) {
        status = true;
        if (value1->float_scale != value2->float_scale) {
            status = false;
        } else {
            if (value1->float_scale) {
                if (islessgreater(
                        value1->type.real_scale, value2->type.real_scale)) {
                    status = false;
                }
            } else {
                if (value1->type.integer_scale != value2->type.integer_scale) {
                    status = false;
                }
            }
        }
    }

    return status;
}
#endif

/**
 * @brief Encode application data given by a pointer into the APDU.
 * @param apdu - Pointer to the buffer to encode to, or NULL for length
 * @param value - Pointer to the application data value to encode from
 * @return number of bytes encoded
 */
int bacapp_encode_application_data(
    uint8_t *apdu, const BACNET_APPLICATION_DATA_VALUE *value)
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
                apdu_len = encode_application_object_id(
                    apdu, value->type.Object_Id.type,
                    value->type.Object_Id.instance);
                break;
#endif
            case BACNET_APPLICATION_TAG_EMPTYLIST:
                /* Empty data list */
                apdu_len = 0; /* EMPTY */
                break;
#if defined(BACAPP_TIMESTAMP)
            case BACNET_APPLICATION_TAG_TIMESTAMP:
                apdu_len =
                    bacapp_encode_timestamp(apdu, &value->type.Time_Stamp);
                break;
#endif
#if defined(BACAPP_DATETIME)
            case BACNET_APPLICATION_TAG_DATETIME:
                apdu_len = bacapp_encode_datetime(apdu, &value->type.Date_Time);
                break;
#endif
#if defined(BACAPP_DATERANGE)
            case BACNET_APPLICATION_TAG_DATERANGE:
                apdu_len =
                    bacnet_daterange_encode(apdu, &value->type.Date_Range);
                break;
#endif
#if defined(BACAPP_LIGHTING_COMMAND)
            case BACNET_APPLICATION_TAG_LIGHTING_COMMAND:
                /* BACnetLightingCommand */
                apdu_len = lighting_command_encode(
                    apdu, &value->type.Lighting_Command);
                break;
#endif
#if defined(BACAPP_XY_COLOR)
            case BACNET_APPLICATION_TAG_XY_COLOR:
                /* BACnetxyColor */
                apdu_len = xy_color_encode(apdu, &value->type.XY_Color);
                break;
#endif
#if defined(BACAPP_COLOR_COMMAND)
            case BACNET_APPLICATION_TAG_COLOR_COMMAND:
                /* BACnetColorCommand */
                apdu_len =
                    color_command_encode(apdu, &value->type.Color_Command);
                break;
#endif
#if defined(BACAPP_WEEKLY_SCHEDULE)
            case BACNET_APPLICATION_TAG_WEEKLY_SCHEDULE:
                /* BACnetWeeklySchedule */
                apdu_len = bacnet_weeklyschedule_encode(
                    apdu, &value->type.Weekly_Schedule);
                break;
#endif
#if defined(BACAPP_CALENDAR_ENTRY)
            case BACNET_APPLICATION_TAG_CALENDAR_ENTRY:
                /* BACnetCalendarEntry */
                apdu_len = bacnet_calendar_entry_encode(
                    apdu, &value->type.Calendar_Entry);
                break;
#endif
#if defined(BACAPP_SPECIAL_EVENT)
            case BACNET_APPLICATION_TAG_SPECIAL_EVENT:
                /* BACnetSpecialEvent */
                apdu_len = bacnet_special_event_encode(
                    apdu, &value->type.Special_Event);
                break;
#endif
#if defined(BACAPP_HOST_N_PORT)
            case BACNET_APPLICATION_TAG_HOST_N_PORT:
                /* BACnetHostNPort */
                apdu_len = host_n_port_encode(apdu, &value->type.Host_Address);
                break;
#endif
#if defined(BACAPP_DEVICE_OBJECT_PROPERTY_REFERENCE)
            case BACNET_APPLICATION_TAG_DEVICE_OBJECT_PROPERTY_REFERENCE:
                /* BACnetDeviceObjectPropertyReference */
                apdu_len = bacapp_encode_device_obj_property_ref(
                    apdu, &value->type.Device_Object_Property_Reference);
                break;
#endif
#if defined(BACAPP_DEVICE_OBJECT_REFERENCE)
            case BACNET_APPLICATION_TAG_DEVICE_OBJECT_REFERENCE:
                /* BACnetDeviceObjectReference */
                apdu_len = bacapp_encode_device_obj_ref(
                    apdu, &value->type.Device_Object_Reference);
                break;
#endif
#if defined(BACAPP_OBJECT_PROPERTY_REFERENCE)
            case BACNET_APPLICATION_TAG_OBJECT_PROPERTY_REFERENCE:
                /* BACnetObjectPropertyReference */
                apdu_len = bacapp_encode_obj_property_ref(
                    apdu, &value->type.Object_Property_Reference);
                break;
#endif
#if defined(BACAPP_DESTINATION)
            case BACNET_APPLICATION_TAG_DESTINATION:
                /* BACnetDestination */
                apdu_len =
                    bacnet_destination_encode(apdu, &value->type.Destination);
                break;
#endif
#if defined(BACAPP_BDT_ENTRY)
            case BACNET_APPLICATION_TAG_BDT_ENTRY:
                /* BACnetBDTEntry */
                apdu_len =
                    bacnet_bdt_entry_encode(apdu, &value->type.BDT_Entry);
                break;
#endif
#if defined(BACAPP_FDT_ENTRY)
            case BACNET_APPLICATION_TAG_FDT_ENTRY:
                /* BACnetFDTEntry */
                apdu_len =
                    bacnet_fdt_entry_encode(apdu, &value->type.FDT_Entry);
                break;
#endif
#if defined(BACAPP_ACTION_COMMAND)
            case BACNET_APPLICATION_TAG_ACTION_COMMAND:
                /* BACnetActionCommand */
                apdu_len = bacnet_action_command_encode(
                    apdu, &value->type.Action_Command);
                break;
#endif
#if defined(BACAPP_SCALE)
            case BACNET_APPLICATION_TAG_SCALE:
                /* BACnetScale */
                apdu_len = bacnet_scale_encode(apdu, &value->type.Scale);
                break;
#endif
#if defined(BACAPP_SHED_LEVEL)
            case BACNET_APPLICATION_TAG_SHED_LEVEL:
                /* BACnetShedLevel */
                apdu_len =
                    bacnet_shed_level_encode(apdu, &value->type.Shed_Level);
                break;
#endif
#if defined(BACAPP_ACCESS_RULE)
            case BACNET_APPLICATION_TAG_ACCESS_RULE:
                /* BACnetAccessRule */
                apdu_len =
                    bacapp_encode_access_rule(apdu, &value->type.Access_Rule);
                break;
#endif
#if defined(BACAPP_CHANNEL_VALUE)
            case BACNET_APPLICATION_TAG_CHANNEL_VALUE:
                /* BACnetChannelValue */
                apdu_len = bacnet_channel_value_type_encode(
                    apdu, &value->type.Channel_Value);
                break;
#endif
#if defined(BACAPP_TIMER_VALUE)
            case BACNET_APPLICATION_TAG_TIMER_VALUE:
                /* BACnetTimerStateChangeValue */
                apdu_len = bacnet_timer_value_type_encode(
                    apdu, &value->type.Timer_Value);
                break;
#endif
#if defined(BACAPP_RECIPIENT)
            case BACNET_APPLICATION_TAG_RECIPIENT:
                apdu_len =
                    bacnet_recipient_encode(apdu, &value->type.Recipient);
                break;
#endif
#if defined(BACAPP_ADDRESS_BINDING)
            case BACNET_APPLICATION_TAG_ADDRESS_BINDING:
                apdu_len = bacnet_address_binding_type_encode(
                    apdu, &value->type.Address_Binding);
                break;
#endif
#if defined(BACAPP_NO_VALUE)
            case BACNET_APPLICATION_TAG_NO_VALUE:
                apdu_len = bacnet_timer_value_no_value_encode(apdu);
                break;
#endif
#if defined(BACAPP_LOG_RECORD)
            case BACNET_APPLICATION_TAG_LOG_RECORD:
                /* BACnetLogRecord */
                apdu_len = bacnet_log_record_value_encode(
                    apdu, &value->type.Log_Record);
                break;
#endif
#if defined(BACAPP_SECURE_CONNECT)
            case BACNET_APPLICATION_TAG_SC_FAILED_CONNECTION_REQUEST:
                apdu_len = bacapp_encode_SCFailedConnectionRequest(
                    apdu, &value->type.SC_Failed_Req);
                break;
            case BACNET_APPLICATION_TAG_SC_HUB_FUNCTION_CONNECTION_STATUS:
                apdu_len = bacapp_encode_SCHubFunctionConnection(
                    apdu, &value->type.SC_Hub_Function_Status);
                break;
            case BACNET_APPLICATION_TAG_SC_DIRECT_CONNECTION_STATUS:
                apdu_len = bacapp_encode_SCDirectConnection(
                    apdu, &value->type.SC_Direct_Status);
                break;
            case BACNET_APPLICATION_TAG_SC_HUB_CONNECTION_STATUS:
                apdu_len = bacapp_encode_SCHubConnection(
                    apdu, &value->type.SC_Hub_Status);
                break;
#endif
            default:
                break;
        }
    }

    return apdu_len;
}

/**
 * @brief Decode application tagged data and store it into value.
 * @param apdu  Receive buffer
 * @param apdu_size Size of the receive buffer
 * @param tag_data_type  Data type of the given tag
 * @param len_value_type  Count of bytes of given tag
 * @param value  Pointer to the application value structure,
 *               used to store the decoded value to.
 * @note Decodes only the 13 primitive application data types!
 *
 * @return Number of octets consumed (could be zero).
 * Parameter value->tag set to MAX_BACNET_APPLICATION_TAG when
 * the number of octets consumed is zero and there is an error
 * in the decoding, or BACNET_STATUS_ERROR/ABORT/REJECT if malformed.
 */
int bacapp_data_decode(
    const uint8_t *apdu,
    uint32_t apdu_size,
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
                len = bacnet_unsigned_decode(
                    apdu, apdu_size, len_value_type, &value->type.Unsigned_Int);
                break;
#endif
#if defined(BACAPP_SIGNED)
            case BACNET_APPLICATION_TAG_SIGNED_INT:
                len = bacnet_signed_decode(
                    apdu, apdu_size, len_value_type, &value->type.Signed_Int);
                break;
#endif
#if defined(BACAPP_REAL)
            case BACNET_APPLICATION_TAG_REAL:
                len = bacnet_real_decode(
                    apdu, apdu_size, len_value_type, &(value->type.Real));
                break;
#endif
#if defined(BACAPP_DOUBLE)
            case BACNET_APPLICATION_TAG_DOUBLE:
                len = bacnet_double_decode(
                    apdu, apdu_size, len_value_type, &(value->type.Double));
                break;
#endif
#if defined(BACAPP_OCTET_STRING)
            case BACNET_APPLICATION_TAG_OCTET_STRING:
                len = bacnet_octet_string_decode(
                    apdu, apdu_size, len_value_type, &value->type.Octet_String);
                break;
#endif
#if defined(BACAPP_CHARACTER_STRING)
            case BACNET_APPLICATION_TAG_CHARACTER_STRING:
                len = bacnet_character_string_decode(
                    apdu, apdu_size, len_value_type,
                    &value->type.Character_String);
                break;
#endif
#if defined(BACAPP_BIT_STRING)
            case BACNET_APPLICATION_TAG_BIT_STRING:
                len = bacnet_bitstring_decode(
                    apdu, apdu_size, len_value_type, &value->type.Bit_String);
                break;
#endif
#if defined(BACAPP_ENUMERATED)
            case BACNET_APPLICATION_TAG_ENUMERATED:
                len = bacnet_enumerated_decode(
                    apdu, apdu_size, len_value_type, &value->type.Enumerated);
                break;
#endif
#if defined(BACAPP_DATE)
            case BACNET_APPLICATION_TAG_DATE:
                len = bacnet_date_decode(
                    apdu, apdu_size, len_value_type, &value->type.Date);
                break;
#endif
#if defined(BACAPP_TIME)
            case BACNET_APPLICATION_TAG_TIME:
                len = bacnet_time_decode(
                    apdu, apdu_size, len_value_type, &value->type.Time);
                break;
#endif
#if defined(BACAPP_OBJECT_ID)
            case BACNET_APPLICATION_TAG_OBJECT_ID: {
                len = bacnet_object_id_decode(
                    apdu, apdu_size, len_value_type,
                    &value->type.Object_Id.type,
                    &value->type.Object_Id.instance);
            } break;
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
 * @brief Decode the data and store it into value.
 * @param apdu  Receive buffer
 * @param tag_data_type  Data type of the given tag
 * @param len_value_type  Count of bytes of given tag
 * @param value  Pointer to the application value structure,
 *               used to store the decoded value to.
 *
 * @return Number of octets consumed
 * @deprecated Use bacapp_data_decode() instead.
 */
int bacapp_decode_data(
    const uint8_t *apdu,
    uint8_t tag_data_type,
    uint32_t len_value_type,
    BACNET_APPLICATION_DATA_VALUE *value)
{
    return bacapp_data_decode(
        apdu, MAX_APDU, tag_data_type, len_value_type, value);
}

/**
 * @brief Decode the BACnet Application Data
 *
 * @param apdu - buffer of data to be decoded
 * @param apdu_len_max - number of bytes in the buffer
 * @param value - decoded value, if decoded
 *
 * @return the number of apdu bytes consumed, 0 on bad args, or
 * BACNET_STATUS_ERROR
 */
int bacapp_decode_application_data(
    const uint8_t *apdu,
    uint32_t apdu_size,
    BACNET_APPLICATION_DATA_VALUE *value)
{
    int len = 0;
    int apdu_len = 0;
    BACNET_TAG tag = { 0 };

    if (!value) {
        return 0;
    }
    len = bacnet_tag_decode(apdu, apdu_size, &tag);
    if ((len > 0) && tag.application) {
        value->context_specific = false;
        value->tag = tag.number;
        apdu_len += len;
        len = bacapp_data_decode(
            &apdu[apdu_len], apdu_size - apdu_len, tag.number,
            tag.len_value_type, value);
        if ((len >= 0) && (value->tag != MAX_BACNET_APPLICATION_TAG)) {
            apdu_len += len;
        } else {
            apdu_len = BACNET_STATUS_ERROR;
        }
        value->next = NULL;
    } else if (apdu && (apdu_size > 0)) {
        apdu_len = BACNET_STATUS_ERROR;
    }

    return apdu_len;
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

bool bacapp_decode_application_data_safe(
    const uint8_t *new_apdu,
    uint32_t new_apdu_len,
    BACNET_APPLICATION_DATA_VALUE *value)
{
    /* The static variables that store the apdu buffer between function calls */
    static const uint8_t *apdu = NULL;
    static uint32_t apdu_len_remaining = 0;
    static uint32_t apdu_len = 0;
    int len = 0;
    int tag_len = 0;
    BACNET_TAG tag = { 0 };

    bool ret = false;

    if (new_apdu != NULL) {
        apdu = new_apdu;
        apdu_len_remaining = new_apdu_len;
        apdu_len = 0;
    }
    if (!value) {
        return ret;
    }
    tag_len = bacnet_tag_decode(&apdu[apdu_len], apdu_len_remaining, &tag);
    if ((tag_len > 0) && tag.application) {
        /* If tag_len is zero, then the tag information is truncated */
        value->context_specific = false;
        apdu_len += tag_len;
        apdu_len_remaining -= tag_len;
        /* The tag is boolean then len_value_type is interpreted as value,
            not length, so don't bother checking with apdu_len_remaining */
        if (tag.number == BACNET_APPLICATION_TAG_BOOLEAN ||
            (tag.len_value_type <= apdu_len_remaining)) {
            value->tag = tag.number;
            len = bacapp_data_decode(
                &apdu[apdu_len], apdu_len_remaining, tag.number,
                tag.len_value_type, value);
            if (value->tag != MAX_BACNET_APPLICATION_TAG) {
                apdu_len += len;
                apdu_len_remaining -= len;
                ret = true;
            } else {
                ret = false;
            }
        }
        value->next = NULL;
    }

    return ret;
}

/**
 * @brief Decode the data to determine the data length
 * @param apdu  Pointer to the received data.
 * @param tag_number  Data type to be decoded.
 * @param len_value_type  Length of the data in bytes.
 * @return datalength for the given tag, or INT_MAX if out of range.
 * @deprecated Use bacnet_application_data_length() instead.
 */
int bacapp_decode_data_len(
    const uint8_t *apdu, uint8_t tag_number, uint32_t len_value_type)
{
    (void)apdu;
    return bacnet_application_data_length(tag_number, len_value_type);
}

/**
 * @brief Determine the BACnet Application Data number of APDU bytes consumed
 * @param apdu - buffer of data to be decoded
 * @param apdu_size - number of bytes in the buffer
 * @return  number of bytes decoded, or zero if errors occur
 * @deprecated Use bacnet_enclosed_data_length() instead.
 */
int bacapp_decode_application_data_len(const uint8_t *apdu, unsigned apdu_size)
{
    int len = 0;
    int tag_len = 0;
    int decode_len = 0;
    BACNET_TAG tag = { 0 };

    if (!bacnet_is_context_specific(apdu, apdu_size)) {
        tag_len = bacnet_tag_decode(apdu, apdu_size, &tag);
        if (tag_len > 0) {
            len += tag_len;
            decode_len =
                bacnet_application_data_length(tag.number, tag.len_value_type);
            len += decode_len;
        }
    }

    return len;
}

/**
 * @brief Encode a BACnet Context tagged data,
 * or enclosed data for complex types
 * @param apdu - buffer to encode to, or NULL for length
 * @param context_tag_number - context tag number
 * @param value - value to encode
 * @return number of bytes encoded
 */
int bacapp_encode_context_data_value(
    uint8_t *apdu,
    uint8_t context_tag_number,
    const BACNET_APPLICATION_DATA_VALUE *value)
{
    int apdu_len = 0; /* total length of the apdu, return value */
    int len;

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
                apdu_len = encode_context_object_id(
                    apdu, context_tag_number, value->type.Object_Id.type,
                    value->type.Object_Id.instance);
                break;
#endif
            default:
                /* non-primitive data is enclosed in open/close tags */
                len = encode_opening_tag(apdu, context_tag_number);
                apdu_len += len;
                if (apdu) {
                    apdu += len;
                }
                len = bacapp_encode_application_data(apdu, value);
                apdu_len += len;
                if (apdu) {
                    apdu += len;
                }
                len = encode_closing_tag(apdu, context_tag_number);
                apdu_len += len;
                break;
        }
    }

    return apdu_len;
}

/**
 * @brief Lookup an application tag for specific context tagged data
 * @param property - object property identifier
 * @param tag_number - context tag number used in the property
 * @return application tag, or MAX_BACNET_APPLICATION_TAG if not known
 * @deprecated Use bacapp_known_property_tag() instead.
 * @note This function was accurate for data constructed as a SEQUENCE,
 * but not for data that used CHOICE. It is deprecated.
 */
BACNET_APPLICATION_TAG
bacapp_context_tag_type(BACNET_PROPERTY_ID property, uint8_t tag_number)
{
    int tag = MAX_BACNET_APPLICATION_TAG;

    (void)tag_number;
    tag = bacapp_known_property_tag(MAX_BACNET_OBJECT_TYPE, property);
    if (tag == -1) {
        tag = MAX_BACNET_APPLICATION_TAG;
    }

    return (BACNET_APPLICATION_TAG)tag;
}

/**
 * @brief Encode a BACnet Context tagged data
 * @param apdu - buffer to encode to, or NULL for length
 * @param value - value to encode from
 * @param property - object property identifier
 * @return number of bytes encoded
 * @deprecated Use bacapp_encode_known_property() instead.
 */
int bacapp_encode_context_data(
    uint8_t *apdu,
    BACNET_APPLICATION_DATA_VALUE *value,
    BACNET_PROPERTY_ID property)
{
    BACNET_OBJECT_TYPE object_type = MAX_BACNET_OBJECT_TYPE;

    return bacapp_encode_known_property(apdu, value, object_type, property);
}

/**
 * @brief Decode context encoded data
 *
 * @param apdu - buffer of data to be decoded
 * @param apdu_size - number of bytes in the buffer
 * @param value - stores the decoded property value
 * @param property - context property identifier
 * @return  number of bytes decoded, or #BACNET_STATUS_ERROR
 * @deprecated Use bacapp_decode_known_property() instead.
 */
int bacapp_decode_context_data(
    const uint8_t *apdu,
    unsigned apdu_size,
    BACNET_APPLICATION_DATA_VALUE *value,
    BACNET_PROPERTY_ID property)
{
    BACNET_OBJECT_TYPE object_type = MAX_BACNET_OBJECT_TYPE;

    return bacapp_decode_known_property(
        apdu, apdu_size, value, object_type, property);
}

#if defined(BACAPP_COMPLEX_TYPES)
/**
 * @brief Context or Application tagged property value decoding
 *
 * @param apdu - buffer of data to be decoded
 * @param apdu_size - number of bytes in the buffer
 * @param value - stores the decoded property value
 * @param property - context property identifier
 * @return  number of bytes decoded, or #BACNET_STATUS_ERROR
 * @deprecated Use bacapp_decode_known_property() instead.
 */
int bacapp_decode_generic_property(
    const uint8_t *apdu,
    int apdu_size,
    BACNET_APPLICATION_DATA_VALUE *value,
    BACNET_PROPERTY_ID prop)
{
    int apdu_len = BACNET_STATUS_ERROR;

    if (apdu && (apdu_size > 0)) {
        if (IS_CONTEXT_SPECIFIC(*apdu)) {
            apdu_len = bacapp_decode_context_data(apdu, apdu_size, value, prop);
        } else {
            apdu_len = bacapp_decode_application_data(apdu, apdu_size, value);
        }
    }

    return apdu_len;
}
#endif

/**
 * @brief Decode BACnetPriorityValue complex data - one element only
 *
 *  BACnetPriorityValue ::= CHOICE {
 *      null NULL,
 *      real REAL,
 *      enumerated ENUMERATED,
 *      unsigned Unsigned,
 *      boolean BOOLEAN,
 *      integer INTEGER,
 *      double Double,
 *      time Time,
 *      characterstring CharacterString,
 *      octetstring OCTET STRING,
 *      bitstring BIT STRING,
 *      date Date,
 *      objectidentifier BACnetObjectIdentifier,
 *      constructed-value [0] ABSTRACT-SYNTAX.&Type,
 *      datetime [1] BACnetDateTime
 *  }
 *
 * @param apdu - buffer of data to be decoded
 * @param apdu_size - number of bytes in the buffer
 * @param value - stores the decoded property value
 * @param property - context property identifier
 * @return  number of bytes decoded, or #BACNET_STATUS_ERROR
 */
static int decode_priority_array_value(
    const uint8_t *apdu,
    unsigned apdu_size,
    BACNET_APPLICATION_DATA_VALUE *value,
    BACNET_OBJECT_TYPE object_type)
{
    int apdu_len = 0;
#if defined(BACAPP_COMPLEX_TYPES)
    int len = 0;
    BACNET_APPLICATION_TAG tag = MAX_BACNET_APPLICATION_TAG;

    if (bacnet_is_opening_tag_number(apdu, apdu_size, 0, &len)) {
        /* constructed-value [0] ABSTRACT-SYNTAX.&Type */
        apdu_len += len;
        /* adjust application tag for complex types */
        if (object_type == OBJECT_COLOR) {
            /* Properties using BACnetxyColor */
            tag = BACNET_APPLICATION_TAG_XY_COLOR;
        } else if (
            (object_type == OBJECT_DATETIME_PATTERN_VALUE) ||
            (object_type == OBJECT_DATETIME_VALUE)) {
            /* Properties using BACnetDateTime */
            tag = BACNET_APPLICATION_TAG_DATETIME;
        }
        if (tag != MAX_BACNET_APPLICATION_TAG) {
            len = bacapp_decode_application_tag_value(
                &apdu[apdu_len], apdu_size - apdu_len, tag, value);
            if (len < 0) {
                return BACNET_STATUS_ERROR;
            }
            apdu_len += len;
        }
        if (!bacnet_is_closing_tag_number(
                &apdu[apdu_len], apdu_size - apdu_len, 0, &len)) {
            return BACNET_STATUS_ERROR;
        }
        apdu_len += len;
    } else if (bacnet_is_opening_tag_number(apdu, apdu_size, 1, &len)) {
        /* datetime [1] BACnetDateTime */
        apdu_len += len;
        /* adjust application tag for complex types */
        tag = BACNET_APPLICATION_TAG_DATETIME;
        len = bacapp_decode_application_tag_value(
            &apdu[apdu_len], apdu_size - apdu_len, tag, value);
        if (len < 0) {
            return BACNET_STATUS_ERROR;
        }
        apdu_len += len;
        if (!bacnet_is_closing_tag_number(
                &apdu[apdu_len], apdu_size - apdu_len, 1, &len)) {
            return BACNET_STATUS_ERROR;
        }
        apdu_len += len;
    } else
#else
    (void)object_type;
#endif
    {
        apdu_len = bacapp_decode_application_data(apdu, apdu_size, value);
    }

    return apdu_len;
}

/**
 * @brief Determine a pseudo application tag for a known property
 * @param object_type - BACNET_OBJECT_TYPE
 * @param property - BACNET_PROPERTY_ID
 * @return pseudo BACNET_APPLICATION_TAG or -1 if not known
 */
int bacapp_known_property_tag(
    BACNET_OBJECT_TYPE object_type, BACNET_PROPERTY_ID property)
{
#if defined(BACAPP_COMPLEX_TYPES)
    switch (property) {
        case PROP_ZONE_MEMBERS:
        case PROP_DOOR_MEMBERS:
        case PROP_SUBORDINATE_LIST:
        case PROP_REPRESENTS:
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
        case PROP_MEMBER_OF:
        case PROP_CREDENTIALS:
        case PROP_ACCOMPANIMENT:
        case PROP_BELONGS_TO:
        case PROP_LAST_ACCESS_POINT:
        case PROP_ENERGY_METER_REF:
        case PROP_TARGET_REFERENCES:
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
        case PROP_SETPOINT_REFERENCE:
        case PROP_INPUT_REFERENCE:
        case PROP_EVENT_ALGORITHM_INHIBIT_REF:
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
            } else if (
                (object_type == OBJECT_DATETIME_PATTERN_VALUE) ||
                (object_type == OBJECT_DATETIME_VALUE)) {
                /* Properties using BACnetDateTime */
                return BACNET_APPLICATION_TAG_DATETIME;
            } else if (object_type == OBJECT_CHANNEL) {
                /* Properties using BACnetChannelValue */
                return BACNET_APPLICATION_TAG_CHANNEL_VALUE;
            }
            /* note: primitive application tagged present-values return '-1' */
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
            return BACNET_APPLICATION_TAG_SPECIAL_EVENT;

        case PROP_DATE_LIST:
            /* BACnetCalendarEntry */
            return BACNET_APPLICATION_TAG_CALENDAR_ENTRY;

        case PROP_ACTIVE_COV_SUBSCRIPTIONS:
            /* FIXME: BACnetCOVSubscription */
            return -1;

        case PROP_EFFECTIVE_PERIOD:
            /* BACnetDateRange (Schedule) */
            return BACNET_APPLICATION_TAG_DATERANGE;

        case PROP_RECIPIENT_LIST:
            /* Properties using BACnetDestination */
            return BACNET_APPLICATION_TAG_DESTINATION;

        case PROP_TIME_SYNCHRONIZATION_RECIPIENTS:
        case PROP_RESTART_NOTIFICATION_RECIPIENTS:
        case PROP_UTC_TIME_SYNCHRONIZATION_RECIPIENTS:
            /* Properties using BACnetRecipient */
            return BACNET_APPLICATION_TAG_RECIPIENT;

        case PROP_DEVICE_ADDRESS_BINDING:
        case PROP_MANUAL_SLAVE_ADDRESS_BINDING:
        case PROP_SLAVE_ADDRESS_BINDING:
            /* BACnetAddressBinding */
            return BACNET_APPLICATION_TAG_ADDRESS_BINDING;

        case PROP_LOG_BUFFER:
            /* BACnetLogRecord */
            return BACNET_APPLICATION_TAG_LOG_RECORD;
        case PROP_ACTION:
            if (object_type == OBJECT_COMMAND) {
                /* BACnetActionCommand */
                return BACNET_APPLICATION_TAG_ACTION_COMMAND;
            }
            return -1;
        case PROP_SCALE:
            /* BACnetScale */
            return BACNET_APPLICATION_TAG_SCALE;
        case PROP_ACTUAL_SHED_LEVEL:
        case PROP_EXPECTED_SHED_LEVEL:
        case PROP_REQUESTED_SHED_LEVEL:
            /* BACnetShedLevel */
            return BACNET_APPLICATION_TAG_SHED_LEVEL;

        case PROP_FD_BBMD_ADDRESS:
        case PROP_BACNET_IP_GLOBAL_ADDRESS:
            /* BACnetHostNPort */
            return BACNET_APPLICATION_TAG_HOST_N_PORT;

        case PROP_BBMD_BROADCAST_DISTRIBUTION_TABLE:
            /* BACnetBDTEntry */
            return BACNET_APPLICATION_TAG_BDT_ENTRY;
        case PROP_BBMD_FOREIGN_DEVICE_TABLE:
            /* BACnetFDTEntry */
            return BACNET_APPLICATION_TAG_FDT_ENTRY;
        case PROP_POSITIVE_ACCESS_RULES:
        case PROP_NEGATIVE_ACCESS_RULES:
            /* BACnetAccessRule */
            return BACNET_APPLICATION_TAG_ACCESS_RULE;

        case PROP_SC_FAILED_CONNECTION_REQUESTS:
            return BACNET_APPLICATION_TAG_SC_FAILED_CONNECTION_REQUEST;

        case PROP_SC_HUB_FUNCTION_CONNECTION_STATUS:
            return BACNET_APPLICATION_TAG_SC_HUB_FUNCTION_CONNECTION_STATUS;

        case PROP_SC_DIRECT_CONNECT_CONNECTION_STATUS:
            return BACNET_APPLICATION_TAG_SC_DIRECT_CONNECTION_STATUS;

        case PROP_SC_PRIMARY_HUB_CONNECTION_STATUS:
        case PROP_SC_FAILOVER_HUB_CONNECTION_STATUS:
            return BACNET_APPLICATION_TAG_SC_HUB_CONNECTION_STATUS;
        case PROP_STATE_CHANGE_VALUES:
            return BACNET_APPLICATION_TAG_TIMER_VALUE;
        default:
            return -1;
    }
#else
    (void)object_type;
    (void)property;
    return -1;
#endif
}

/**
 * @brief Decode the application value according to specific tag
 * @param apdu - buffer of data to be decoded
 * @param apdu_size - number of bytes in the buffer
 * @param tag - BACNET_APPLICATION_TAG to be decoded
 * @param value - stores the decoded property value
 * @return  number of bytes decoded (0..N), or #BACNET_STATUS_ERROR
 */
int bacapp_decode_application_tag_value(
    const uint8_t *apdu,
    size_t apdu_size,
    BACNET_APPLICATION_TAG tag,
    BACNET_APPLICATION_DATA_VALUE *value)
{
    int apdu_len = 0;

    if (!value) {
        return 0;
    }
    value->tag = tag;
    switch (tag) {
#if defined(BACAPP_NULL)
        case BACNET_APPLICATION_TAG_NULL:
            /* nothing else to do */
            break;
#endif
#if defined(BACAPP_BOOLEAN)
        case BACNET_APPLICATION_TAG_BOOLEAN:
            apdu_len = bacnet_boolean_application_decode(
                apdu, apdu_size, &value->type.Boolean);
            if (apdu_len == 0) {
                /* primitive value application decoders return 0
                   when the wrong tag is encountered */
                apdu_len = BACNET_STATUS_ERROR;
            }
            break;
#endif
#if defined(BACAPP_UNSIGNED)
        case BACNET_APPLICATION_TAG_UNSIGNED_INT:
            apdu_len = bacnet_unsigned_application_decode(
                apdu, apdu_size, &value->type.Unsigned_Int);
            if (apdu_len == 0) {
                /* primitive value application decoders return 0
                   when the wrong tag is encountered */
                apdu_len = BACNET_STATUS_ERROR;
            }
            break;
#endif
#if defined(BACAPP_SIGNED)
        case BACNET_APPLICATION_TAG_SIGNED_INT:
            apdu_len = bacnet_signed_application_decode(
                apdu, apdu_size, &value->type.Signed_Int);
            if (apdu_len == 0) {
                /* primitive value application decoders return 0
                   when the wrong tag is encountered */
                apdu_len = BACNET_STATUS_ERROR;
            }
            break;
#endif
#if defined(BACAPP_REAL)
        case BACNET_APPLICATION_TAG_REAL:
            apdu_len = bacnet_real_application_decode(
                apdu, apdu_size, &value->type.Real);
            if (apdu_len == 0) {
                /* primitive value application decoders return 0
                   when the wrong tag is encountered */
                apdu_len = BACNET_STATUS_ERROR;
            }
            break;
#endif
#if defined(BACAPP_DOUBLE)
        case BACNET_APPLICATION_TAG_DOUBLE:
            apdu_len = bacnet_double_application_decode(
                apdu, apdu_size, &value->type.Double);
            if (apdu_len == 0) {
                /* primitive value application decoders return 0
                   when the wrong tag is encountered */
                apdu_len = BACNET_STATUS_ERROR;
            }
            break;
#endif
#if defined(BACAPP_OCTET_STRING)
        case BACNET_APPLICATION_TAG_OCTET_STRING:
            apdu_len = bacnet_octet_string_application_decode(
                apdu, apdu_size, &value->type.Octet_String);
            if (apdu_len == 0) {
                /* primitive value application decoders return 0
                   when the wrong tag is encountered */
                apdu_len = BACNET_STATUS_ERROR;
            }
            break;
#endif
#if defined(BACAPP_CHARACTER_STRING)
        case BACNET_APPLICATION_TAG_CHARACTER_STRING:
            apdu_len = bacnet_character_string_application_decode(
                apdu, apdu_size, &value->type.Character_String);
            if (apdu_len == 0) {
                /* primitive value application decoders return 0
                   when the wrong tag is encountered */
                apdu_len = BACNET_STATUS_ERROR;
            }
            break;
#endif
#if defined(BACAPP_BIT_STRING)
        case BACNET_APPLICATION_TAG_BIT_STRING:
            apdu_len = bacnet_bitstring_application_decode(
                apdu, apdu_size, &value->type.Bit_String);
            if (apdu_len == 0) {
                apdu_len = BACNET_STATUS_ERROR;
            }
            break;
#endif
#if defined(BACAPP_ENUMERATED)
        case BACNET_APPLICATION_TAG_ENUMERATED:
            apdu_len = bacnet_enumerated_application_decode(
                apdu, apdu_size, &value->type.Enumerated);
            if (apdu_len == 0) {
                /* primitive value application decoders return 0
                   when the wrong tag is encountered */
                apdu_len = BACNET_STATUS_ERROR;
            }
            break;
#endif
#if defined(BACAPP_DATE)
        case BACNET_APPLICATION_TAG_DATE:
            apdu_len = bacnet_date_application_decode(
                apdu, apdu_size, &value->type.Date);
            if (apdu_len == 0) {
                /* primitive value application decoders return 0
                   when the wrong tag is encountered */
                apdu_len = BACNET_STATUS_ERROR;
            }
            break;
#endif
#if defined(BACAPP_TIME)
        case BACNET_APPLICATION_TAG_TIME:
            apdu_len = bacnet_time_application_decode(
                apdu, apdu_size, &value->type.Time);
            if (apdu_len == 0) {
                /* primitive value application decoders return 0
                   when the wrong tag is encountered */
                apdu_len = BACNET_STATUS_ERROR;
            }
            break;
#endif
#if defined(BACAPP_OBJECT_ID)
        case BACNET_APPLICATION_TAG_OBJECT_ID:
            apdu_len = bacnet_object_id_application_decode(
                apdu, apdu_size, &value->type.Object_Id.type,
                &value->type.Object_Id.instance);
            if (apdu_len == 0) {
                /* primitive value application decoders return 0
                   when the wrong tag is encountered */
                apdu_len = BACNET_STATUS_ERROR;
            }
            break;
#endif
        case BACNET_APPLICATION_TAG_EMPTYLIST:
            apdu_len = 0;
            break;
#if defined(BACAPP_TIMESTAMP)
        case BACNET_APPLICATION_TAG_TIMESTAMP:
            apdu_len = bacnet_timestamp_decode(
                apdu, apdu_size, &value->type.Time_Stamp);
            break;
#endif
#if defined(BACAPP_DATETIME)
        case BACNET_APPLICATION_TAG_DATETIME:
            apdu_len =
                bacnet_datetime_decode(apdu, apdu_size, &value->type.Date_Time);
            break;
#endif
#if defined(BACAPP_DATERANGE)
        case BACNET_APPLICATION_TAG_DATERANGE:
            apdu_len = bacnet_daterange_decode(
                apdu, apdu_size, &value->type.Date_Range);
            break;
#endif
#if defined(BACAPP_LIGHTING_COMMAND)
        case BACNET_APPLICATION_TAG_LIGHTING_COMMAND:
            /* BACnetLightingCommand */
            apdu_len = lighting_command_decode(
                apdu, apdu_size, &value->type.Lighting_Command);
            break;
#endif
#if defined(BACAPP_XY_COLOR)
        case BACNET_APPLICATION_TAG_XY_COLOR:
            /* BACnetxyColor */
            apdu_len = xy_color_decode(apdu, apdu_size, &value->type.XY_Color);
            break;
#endif
#if defined(BACAPP_COLOR_COMMAND)
        case BACNET_APPLICATION_TAG_COLOR_COMMAND:
            /* BACnetColorCommand */
            apdu_len = color_command_decode(
                apdu, apdu_size, NULL, &value->type.Color_Command);
            break;
#endif
#if defined(BACAPP_WEEKLY_SCHEDULE)
        case BACNET_APPLICATION_TAG_WEEKLY_SCHEDULE:
            /* BACnetWeeklySchedule */
            apdu_len = bacnet_weeklyschedule_decode(
                apdu, apdu_size, &value->type.Weekly_Schedule);
            break;
#endif
#if defined(BACAPP_CALENDAR_ENTRY)
        case BACNET_APPLICATION_TAG_CALENDAR_ENTRY:
            /* BACnetCalendarEntry */
            apdu_len = bacnet_calendar_entry_decode(
                apdu, apdu_size, &value->type.Calendar_Entry);
            break;
#endif
#if defined(BACAPP_SPECIAL_EVENT)
        case BACNET_APPLICATION_TAG_SPECIAL_EVENT:
            /* BACnetSpecialEvent */
            apdu_len = bacnet_special_event_decode(
                apdu, apdu_size, &value->type.Special_Event);
            break;
#endif
#if defined(BACAPP_HOST_N_PORT)
        case BACNET_APPLICATION_TAG_HOST_N_PORT:
            /* BACnetHostNPort */
            apdu_len = host_n_port_decode(
                apdu, apdu_size, NULL, &value->type.Host_Address);
            break;
#endif
#if defined(BACAPP_DEVICE_OBJECT_PROPERTY_REFERENCE)
        case BACNET_APPLICATION_TAG_DEVICE_OBJECT_PROPERTY_REFERENCE:
            /* BACnetDeviceObjectPropertyReference */
            apdu_len = bacnet_device_object_property_reference_decode(
                apdu, apdu_size, &value->type.Device_Object_Property_Reference);
            break;
#endif
#if defined(BACAPP_DEVICE_OBJECT_REFERENCE)
        case BACNET_APPLICATION_TAG_DEVICE_OBJECT_REFERENCE:
            /* BACnetDeviceObjectReference */
            apdu_len = bacnet_device_object_reference_decode(
                apdu, apdu_size, &value->type.Device_Object_Reference);
            break;
#endif
#if defined(BACAPP_OBJECT_PROPERTY_REFERENCE)
        case BACNET_APPLICATION_TAG_OBJECT_PROPERTY_REFERENCE:
            /* BACnetObjectPropertyReference */
            apdu_len = bacapp_decode_obj_property_ref(
                apdu, apdu_size, &value->type.Object_Property_Reference);
            break;
#endif
#if defined(BACAPP_DESTINATION)
        case BACNET_APPLICATION_TAG_DESTINATION:
            /* BACnetDestination */
            apdu_len = bacnet_destination_decode(
                apdu, apdu_size, &value->type.Destination);
            break;
#endif
#if defined(BACAPP_BDT_ENTRY)
        case BACNET_APPLICATION_TAG_BDT_ENTRY:
            /* BACnetBDTEntry */
            apdu_len = bacnet_bdt_entry_decode(
                apdu, apdu_size, NULL, &value->type.BDT_Entry);
            break;
#endif
#if defined(BACAPP_FDT_ENTRY)
        case BACNET_APPLICATION_TAG_FDT_ENTRY:
            /* BACnetFDTEntry */
            apdu_len = bacnet_fdt_entry_decode(
                apdu, apdu_size, NULL, &value->type.FDT_Entry);
            break;
#endif
#if defined(BACAPP_ACTION_COMMAND)
        case BACNET_APPLICATION_TAG_ACTION_COMMAND:
            /* BACnetActionCommand */
            apdu_len = bacnet_action_command_decode(
                apdu, apdu_size, &value->type.Action_Command);
            break;
#endif
#if defined(BACAPP_SCALE)
        case BACNET_APPLICATION_TAG_SCALE:
            /* BACnetScale */
            apdu_len = bacnet_scale_decode(apdu, apdu_size, &value->type.Scale);
            break;
#endif
#if defined(BACAPP_SHED_LEVEL)
        case BACNET_APPLICATION_TAG_SHED_LEVEL:
            /* BACnetShedLevel */
            apdu_len = bacnet_shed_level_decode(
                apdu, apdu_size, &value->type.Shed_Level);
            break;
#endif
#if defined(BACAPP_ACCESS_RULE)
        case BACNET_APPLICATION_TAG_ACCESS_RULE:
            /* BACnetAccessRule */
            apdu_len = bacnet_access_rule_decode(
                apdu, apdu_size, &value->type.Access_Rule);
            break;
#endif
#if defined(BACAPP_CHANNEL_VALUE)
        case BACNET_APPLICATION_TAG_CHANNEL_VALUE:
            /* BACnetChannelValue */
            apdu_len = bacnet_channel_value_decode(
                apdu, apdu_size, &value->type.Channel_Value);
            break;
#endif
#if defined(BACAPP_TIMER_VALUE)
        case BACNET_APPLICATION_TAG_TIMER_VALUE:
            /* BACnetTimerStateChangeValue */
            apdu_len = bacnet_timer_value_decode(
                apdu, apdu_size, &value->type.Timer_Value);
            break;
#endif
#if defined(BACAPP_RECIPIENT)
        case BACNET_APPLICATION_TAG_RECIPIENT:
            apdu_len = bacnet_recipient_decode(
                apdu, apdu_size, &value->type.Recipient);
            break;
#endif
#if defined(BACAPP_ADDRESS_BINDING)
        case BACNET_APPLICATION_TAG_ADDRESS_BINDING:
            apdu_len = bacnet_address_binding_decode(
                apdu, apdu_size, &value->type.Address_Binding);
            break;
#endif
#if defined(BACAPP_LOG_RECORD)
        case BACNET_APPLICATION_TAG_LOG_RECORD:
            /* BACnetLogRecord */
            apdu_len = bacnet_log_record_decode(
                apdu, apdu_size, &value->type.Log_Record);
            break;
#endif
#if defined(BACAPP_SECURE_CONNECT)
        case BACNET_APPLICATION_TAG_SC_FAILED_CONNECTION_REQUEST:
            apdu_len = bacapp_decode_SCFailedConnectionRequest(
                apdu, apdu_size, &value->type.SC_Failed_Req);
            break;
        case BACNET_APPLICATION_TAG_SC_HUB_FUNCTION_CONNECTION_STATUS:
            apdu_len = bacapp_decode_SCHubFunctionConnection(
                apdu, apdu_size, &value->type.SC_Hub_Function_Status);
            break;
        case BACNET_APPLICATION_TAG_SC_DIRECT_CONNECTION_STATUS:
            apdu_len = bacapp_decode_SCDirectConnection(
                apdu, apdu_size, &value->type.SC_Direct_Status);
            break;
        case BACNET_APPLICATION_TAG_SC_HUB_CONNECTION_STATUS:
            apdu_len = bacapp_decode_SCHubConnection(
                apdu, apdu_size, &value->type.SC_Hub_Status);
            break;
#endif
        default:
            break;
    }

    return apdu_len;
}

/**
 * @brief Decodes a well-known, possibly complex property value
 *  or array property.
 *  Used to reverse operations in bacapp_encode_application_data.
 * @note This function is called repeatedly to decode array or lists one
 * element at a time.
 * @param apdu - buffer of data to be decoded
 * @param max_apdu_len - number of bytes in the buffer
 * @param value - stores the decoded property value
 * @param property - context property identifier
 * @return  number of bytes decoded, or BACNET_STATUS_ERROR if errors occur
 * @note number of bytes can be 0 for empty lists, etc.
 */
int bacapp_decode_known_array_property(
    const uint8_t *apdu,
    int apdu_size,
    BACNET_APPLICATION_DATA_VALUE *value,
    BACNET_OBJECT_TYPE object_type,
    BACNET_PROPERTY_ID property,
    uint32_t array_index)
{
    int apdu_len = 0;
    int tag;

    if (bacnet_is_closing_tag(apdu, apdu_size)) {
        if (value) {
            value->tag = BACNET_APPLICATION_TAG_EMPTYLIST;
        }
        apdu_len = 0;
    } else if (array_index == 0) {
        /* Array index 0 is the size of the array and always unsigned int */
        value->tag = BACNET_APPLICATION_TAG_UNSIGNED_INT;
        apdu_len = bacnet_unsigned_application_decode(
            apdu, apdu_size, &value->type.Unsigned_Int);
    } else if (property == PROP_PRIORITY_ARRAY) {
        /* BACnetPriorityValue */
        /* special case to reduce complexity - mostly encoded as application
           tagged values, but sometimes encoded as abstract syntax or complex
           data values */
        apdu_len =
            decode_priority_array_value(apdu, apdu_size, value, object_type);
    } else {
        /* Complex or primitive value?
        Lookup the complex values using their object type and property */
        tag = bacapp_known_property_tag(object_type, property);
        if (tag != -1) {
            apdu_len = bacapp_decode_application_tag_value(
                apdu, apdu_size, tag, value);
        } else {
            apdu_len = bacapp_decode_application_data(apdu, apdu_size, value);
        }
    }

    return apdu_len;
}

/**
 * @brief Decodes a well-known, possibly complex property value
 *  Used to reverse operations in bacapp_encode_application_data.
 * @note This function is called repeatedly to decode array or lists one
 * element at a time.
 * @param apdu - buffer of data to be decoded
 * @param max_apdu_len - number of bytes in the buffer
 * @param value - stores the decoded property value
 * @param property - context property identifier
 * @return  number of bytes decoded, or BACNET_STATUS_ERROR if errors occur
 * @note number of bytes can be 0 for empty lists, etc.
 */
int bacapp_decode_known_property(
    const uint8_t *apdu,
    int apdu_size,
    BACNET_APPLICATION_DATA_VALUE *value,
    BACNET_OBJECT_TYPE object_type,
    BACNET_PROPERTY_ID property)
{
    return bacapp_decode_known_array_property(
        apdu, apdu_size, value, object_type, property, BACNET_ARRAY_ALL);
}

#if defined(BACAPP_COMPLEX_TYPES)
/**
 * @brief Determine the BACnet Context Data number of APDU bytes consumed
 * @param apdu - buffer of data to be decoded
 * @param apdu_len_max - number of bytes in the buffer
 * @param property - context property identifier
 * @return  number of bytes decoded, or zero if errors occur
 * @deprecated use bacnet_enclosed_data_length() instead
 */
int bacapp_decode_context_data_len(
    const uint8_t *apdu, unsigned apdu_len_max, BACNET_PROPERTY_ID property)
{
    int apdu_len = 0, len = 0;
    BACNET_TAG tag = { 0 };

    (void)property;
    len = bacnet_tag_decode(&apdu[0], apdu_len_max, &tag);
    if ((len > 0) && tag.context) {
        apdu_len = len;
        apdu_len += tag.len_value_type;
    }

    return apdu_len;
}
#endif

/**
 * @brief Encode the data and store it into apdu.
 * @param apdu  Buffer to store the encoded data, or NULL for length only
 * @param value  Pointer to the application value structure
 * @return Length of the encoded data in bytes
 */
int bacapp_encode_data(
    uint8_t *apdu, const BACNET_APPLICATION_DATA_VALUE *value)
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

/**
 * @brief Encode the data and store it into apdu.
 * @param apdu  Buffer to store the encoded data, or NULL for length only
 * @param value  Pointer to the application value structure with tag set
 * the specific primitive types (gets overwritten for complex types)
 * @return Length of the encoded data in bytes
 */
int bacapp_encode_known_property(
    uint8_t *apdu,
    BACNET_APPLICATION_DATA_VALUE *value,
    BACNET_OBJECT_TYPE object_type,
    BACNET_PROPERTY_ID property)
{
    int tag;

    tag = bacapp_known_property_tag(object_type, property);
    if (tag != -1) {
        value->tag = tag;
    }

    return bacapp_encode_application_data(apdu, value);
}

/**
 * @brief Copy the application data value
 * @param dest_value  Pointer to the destination application value structure
 * @param src_value  Pointer to the source application value structure
 * @return true on success, else false
 */
bool bacapp_copy(
    BACNET_APPLICATION_DATA_VALUE *dest_value,
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
                octetstring_copy(
                    &dest_value->type.Octet_String,
                    &src_value->type.Octet_String);
                break;
#endif
#if defined(BACAPP_CHARACTER_STRING)
            case BACNET_APPLICATION_TAG_CHARACTER_STRING:
                characterstring_copy(
                    &dest_value->type.Character_String,
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
            default:
                memcpy(
                    &dest_value->type, &src_value->type,
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
 * @param apdu Pointer to the APDU buffer
 * @param apdu_size Bytes valid in the buffer
 * @param property ID of the property to get the length for.
 *
 * @return Length in bytes 0..N, or BACNET_STATUS_ERROR.
 * @deprecated Use bacnet_enclosed_data_length() instead.
 */
int bacapp_data_len(
    const uint8_t *apdu, unsigned apdu_size, BACNET_PROPERTY_ID property)
{
    (void)property;
    return bacnet_enclosed_data_length(apdu, apdu_size);
}

/**
 * @brief Common snprintf() function to suppress CPP check false positives
 * @param buffer - destination string
 * @param count - length of the destination string
 * @param format - format string
 * @return number of characters written
 */
int bacapp_snprintf(char *buffer, size_t count, const char *format, ...)
{
    int length = 0;
    va_list args;

    va_start(args, format);
    /* false positive cppcheck - vsnprintf allows null pointers */
    /* cppcheck-suppress nullPointer */
    /* cppcheck-suppress ctunullpointer */
    length = vsnprintf(buffer, count, format, args);
    va_end(args);

    return length;
}

/**
 * @brief Shift the buffer pointer and decrease the size after an snprintf
 * @param len - number of bytes (excluding terminating NULL byte) from
 * snprintf operation
 * @param buf - pointer to the buffer pointer
 * @param buf_size - pointer to the buffer size
 * @return number of bytes (excluding terminating NULL byte) from snprintf
 */
int bacapp_snprintf_shift(int len, char **buf, size_t *buf_size)
{
    if (buf) {
        if (*buf) {
            *buf += len;
        }
    }
    if (buf_size) {
        if ((*buf_size) >= len) {
            *buf_size -= len;
        } else {
            *buf_size = 0;
        }
    }

    return len;
}

/**
 * @brief Print a value to a string for EPICS
 * @param str - destination string, or NULL for length only
 * @param str_len - length of the destination string, or 0 for length only
 * @param property - property identifier
 * @return number of characters written
 */
static int bacapp_snprintf_property_identifier(
    char *str, size_t str_len, BACNET_PROPERTY_ID property)
{
    int ret_val = 0;
    const char *char_str;

    char_str = bactext_property_name_default(property, NULL);
    if (char_str) {
        ret_val = bacapp_snprintf(str, str_len, "%s", char_str);
    } else {
        ret_val = bacapp_snprintf(str, str_len, "%lu", (unsigned long)property);
    }

    return ret_val;
}

#if defined(BACAPP_NULL)
/**
 * @brief Print an null value to a string for EPICS
 * @param str - destination string, or NULL for length only
 * @param str_len - length of the destination string, or 0 for length only
 * @return number of characters written
 */
static int bacapp_snprintf_null(char *str, size_t str_len)
{
    return bacapp_snprintf(str, str_len, "Null");
}
#endif

#if defined(BACAPP_BOOLEAN)
/**
 * @brief Print an boolean value to a string for EPICS
 * @param str - destination string, or NULL for length only
 * @param str_len - length of the destination string, or 0 for length only
 * @param value - value to print
 * @return number of characters written
 */
static int bacapp_snprintf_boolean(char *str, size_t str_len, bool value)
{
    if (value) {
        return bacapp_snprintf(str, str_len, "TRUE");
    } else {
        return bacapp_snprintf(str, str_len, "FALSE");
    }
}
#endif
#if defined(BACAPP_UNSIGNED)
/**
 * @brief Print an unsigned integer value to a string for EPICS
 * @param str - destination string, or NULL for length only
 * @param str_len - length of the destination string, or 0 for length only
 * @param value - value to print
 * @return number of characters written
 */
static int bacapp_snprintf_unsigned_integer(
    char *str, size_t str_len, BACNET_UNSIGNED_INTEGER value)
{
    return bacapp_snprintf(str, str_len, "%lu", (unsigned long)value);
}
#endif

#if defined(BACAPP_SIGNED)
/**
 * @brief Print an signed integer value to a string for EPICS
 * @param str - destination string, or NULL for length only
 * @param str_len - length of the destination string, or 0 for length only
 * @param value - value to print
 * @return number of characters written
 */
static int
bacapp_snprintf_signed_integer(char *str, size_t str_len, int32_t value)
{
    return bacapp_snprintf(str, str_len, "%ld", (long)value);
}
#endif

#if defined(BACAPP_REAL)
/**
 * @brief Print an real value to a string for EPICS
 * @param str - destination string, or NULL for length only
 * @param str_len - length of the destination string, or 0 for length only
 * @param value - value to print
 * @return number of characters written
 */
static int bacapp_snprintf_real(char *str, size_t str_len, float value)
{
    return bacapp_snprintf(str, str_len, "%f", (double)value);
}
#endif
#if defined(BACAPP_DOUBLE)
/**
 * @brief Print an double value to a string for EPICS
 * @param str - destination string, or NULL for length only
 * @param str_len - length of the destination string, or 0 for length only
 * @param value - value to print
 * @return number of characters written
 */
static int bacapp_snprintf_double(char *str, size_t str_len, double value)
{
    return bacapp_snprintf(str, str_len, "%f", value);
}
#endif

/**
 * @brief Print an octet string value to a string for EPICS
 * @param str - destination string, or NULL for length only
 * @param str_len - length of the destination string, or 0 for length only
 * @param value - octet string value to print
 * @return number of characters written
 */
int bacapp_snprintf_octet_string(
    char *str, size_t str_len, const BACNET_OCTET_STRING *value)
{
    int ret_val = 0;
    int len = 0;
    int slen = 0;
    int i = 0;
    const uint8_t *octet_str;

    slen = bacapp_snprintf(str, str_len, "X'");
    ret_val += bacapp_snprintf_shift(slen, &str, &str_len);
    len = octetstring_length(value);
    if (len > 0) {
        octet_str = octetstring_value((BACNET_OCTET_STRING *)value);
        for (i = 0; i < len; i++) {
            slen = bacapp_snprintf(str, str_len, "%02X", *octet_str);
            ret_val += bacapp_snprintf_shift(slen, &str, &str_len);
            octet_str++;
        }
    }
    slen = bacapp_snprintf(str, str_len, "'");
    ret_val += bacapp_snprintf_shift(slen, &str, &str_len);

    return ret_val;
}

/**
 * @brief Print a character string value to a string for EPICS
 * @param str - destination string, or NULL for length only
 * @param str_len - length of the destination string, or 0 for length only
 * @param value - character string value to print
 * @return number of characters written
 */
int bacapp_snprintf_character_string(
    char *str, size_t str_len, const BACNET_CHARACTER_STRING *value)
{
    int ret_val = 0;
    int len = 0;
    int slen = 0;
    int i = 0;
    const char *char_str;
#if (__STDC_VERSION__ >= 199901L) && defined(__STDC_ISO_10646__)
    /* Wide character (decoded from multi-byte character). */
    wchar_t wc;
    /* Wide character length in bytes. */
    int wclen;
#endif

    len = characterstring_length(value);
    char_str = characterstring_value(value);
    slen = bacapp_snprintf(str, str_len, "\"");
    ret_val += bacapp_snprintf_shift(slen, &str, &str_len);
#if (__STDC_VERSION__ >= 199901L) && defined(__STDC_ISO_10646__)
    if (characterstring_encoding(value) == CHARACTER_UTF8) {
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
            slen = bacapp_snprintf(str, str_len, "%lc", (wint_t)wc);
            ret_val += bacapp_snprintf_shift(slen, &str, &str_len);
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
            if (isprint(*((const unsigned char *)char_str))) {
                slen = bacapp_snprintf(str, str_len, "%c", *char_str);
            } else {
                slen = bacapp_snprintf(str, str_len, "%c", '.');
            }
            ret_val += bacapp_snprintf_shift(slen, &str, &str_len);
            char_str++;
        }
    }
    slen = bacapp_snprintf(str, str_len, "\"");
    ret_val += slen;

    return ret_val;
}

#if defined(BACAPP_BIT_STRING)
/**
 * @brief Print a bit string value to a string for EPICS
 * @param str - destination string, or NULL for length only
 * @param str_len - length of the destination string, or 0 for length only
 * @param value - bit string value to print
 * @return number of characters written
 */
static int bacapp_snprintf_bit_string(
    char *str, size_t str_len, const BACNET_BIT_STRING *value)
{
    int ret_val = 0;
    int len = 0;
    int slen = 0;
    int i = 0;

    len = bitstring_bits_used(value);
    slen = bacapp_snprintf(str, str_len, "{");
    ret_val += bacapp_snprintf_shift(slen, &str, &str_len);
    for (i = 0; i < len; i++) {
        bool bit;
        bit = bitstring_bit(value, (uint8_t)i);
        slen = bacapp_snprintf(str, str_len, "%s", bit ? "true" : "false");
        ret_val += bacapp_snprintf_shift(slen, &str, &str_len);
        if (i < (len - 1)) {
            slen = bacapp_snprintf(str, str_len, ",");
            ret_val += bacapp_snprintf_shift(slen, &str, &str_len);
        }
    }
    slen = bacapp_snprintf(str, str_len, "}");
    ret_val += bacapp_snprintf_shift(slen, &str, &str_len);

    return ret_val;
}
#endif

#if defined(BACAPP_ENUMERATED)
/**
 * @brief Print an enumerated value to a string for EPICS
 * @param str - destination string, or NULL for length only
 * @param str_len - length of the destination string, or 0 for length only
 * @param object_type - object type identifier
 * @param property - object property identifier
 * @param value - enumerated value to print
 * @return number of characters written
 */
static int bacapp_snprintf_enumerated(
    char *str,
    size_t str_len,
    BACNET_OBJECT_TYPE object_type,
    BACNET_PROPERTY_ID property,
    uint32_t value)
{
    int ret_val = 0;

    switch (property) {
        case PROP_PROPERTY_LIST:
            ret_val = bacapp_snprintf_property_identifier(str, str_len, value);
            break;
        case PROP_OBJECT_TYPE:
            if (value <= BACNET_OBJECT_TYPE_RESERVED_MIN) {
                ret_val = bacapp_snprintf(
                    str, str_len, "%s", bactext_object_type_name(value));
            } else if (value <= BACNET_OBJECT_TYPE_RESERVED_MAX) {
                ret_val = bacapp_snprintf(
                    str, str_len, "reserved %lu", (unsigned long)value);
            } else {
                ret_val = bacapp_snprintf(
                    str, str_len, "proprietary-%lu", (unsigned long)value);
            }
            break;
        case PROP_EVENT_STATE:
            ret_val = bacapp_snprintf(
                str, str_len, "%s", bactext_event_state_name(value));
            break;
        case PROP_UNITS:
        case PROP_CONTROLLED_VARIABLE_UNITS:
        case PROP_DERIVATIVE_CONSTANT_UNITS:
        case PROP_INTEGRAL_CONSTANT_UNITS:
        case PROP_PROPORTIONAL_CONSTANT_UNITS:
        case PROP_OUTPUT_UNITS:
        case PROP_CAR_LOAD_UNITS:
            if (bactext_engineering_unit_name_proprietary((unsigned)value)) {
                ret_val = bacapp_snprintf(
                    str, str_len, "proprietary-%lu", (unsigned long)value);
            } else {
                ret_val = bacapp_snprintf(
                    str, str_len, "%s", bactext_engineering_unit_name(value));
            }
            break;
        case PROP_POLARITY:
            ret_val = bacapp_snprintf(
                str, str_len, "%s", bactext_binary_polarity_name(value));
            break;
        case PROP_PRESENT_VALUE:
        case PROP_RELINQUISH_DEFAULT:
            switch (object_type) {
                case OBJECT_BINARY_INPUT:
                case OBJECT_BINARY_OUTPUT:
                case OBJECT_BINARY_VALUE:
                    ret_val = bacapp_snprintf(
                        str, str_len, "%s",
                        bactext_binary_present_value_name(value));
                    break;
                case OBJECT_BINARY_LIGHTING_OUTPUT:
                    ret_val = bacapp_snprintf(
                        str, str_len, "%s",
                        bactext_binary_lighting_pv_name(value));
                    break;
                default:
                    ret_val = bacapp_snprintf(
                        str, str_len, "%lu", (unsigned long)value);
                    break;
            }
            break;
        case PROP_RELIABILITY:
            ret_val = bacapp_snprintf(
                str, str_len, "%s", bactext_reliability_name(value));
            break;
        case PROP_SYSTEM_STATUS:
            ret_val = bacapp_snprintf(
                str, str_len, "%s", bactext_device_status_name(value));
            break;
        case PROP_SEGMENTATION_SUPPORTED:
            ret_val = bacapp_snprintf(
                str, str_len, "%s", bactext_segmentation_name(value));
            break;
        case PROP_NODE_TYPE:
            ret_val = bacapp_snprintf(
                str, str_len, "%s", bactext_node_type_name(value));
            break;
        case PROP_TRANSITION:
            ret_val = bacapp_snprintf(
                str, str_len, "%s", bactext_lighting_transition(value));
            break;
        case PROP_IN_PROGRESS:
            ret_val = bacapp_snprintf(
                str, str_len, "%s", bactext_lighting_in_progress(value));
            break;
        case PROP_LOGGING_TYPE:
            ret_val = bacapp_snprintf(
                str, str_len, "%s", bactext_logging_type_name(value));
            break;
        case PROP_MODE:
        case PROP_ACCEPTED_MODES:
            ret_val = bacapp_snprintf(
                str, str_len, "%s", bactext_life_safety_mode_name(value));
            break;
        case PROP_OPERATION_EXPECTED:
            ret_val = bacapp_snprintf(
                str, str_len, "%s", bactext_life_safety_operation_name(value));
            break;
        case PROP_TRACKING_VALUE:
            switch (object_type) {
                case OBJECT_LIFE_SAFETY_POINT:
                case OBJECT_LIFE_SAFETY_ZONE:
                    ret_val = bacapp_snprintf(
                        str, str_len, "%s",
                        bactext_life_safety_state_name(value));
                    break;
                default:
                    break;
            }
            break;
        case PROP_PROGRAM_CHANGE:
            ret_val = bacapp_snprintf(
                str, str_len, "%s", bactext_program_request_name(value));
            break;
        case PROP_PROGRAM_STATE:
            ret_val = bacapp_snprintf(
                str, str_len, "%s", bactext_program_state_name(value));
            break;
        case PROP_REASON_FOR_HALT:
            ret_val = bacapp_snprintf(
                str, str_len, "%s", bactext_program_error_name(value));
            break;
        case PROP_NETWORK_NUMBER_QUALITY:
            ret_val = bacapp_snprintf(
                str, str_len, "%s", bactext_network_number_quality_name(value));
            break;
        case PROP_NETWORK_TYPE:
            ret_val = bacapp_snprintf(
                str, str_len, "%s", bactext_network_port_type_name(value));
            break;
        case PROP_PROTOCOL_LEVEL:
            ret_val = bacapp_snprintf(
                str, str_len, "%s", bactext_protocol_level_name(value));
            break;
        case PROP_EVENT_TYPE:
            ret_val = bacapp_snprintf(
                str, str_len, "%s", bactext_event_type_name(value));
            break;
        case PROP_NOTIFY_TYPE:
            ret_val = bacapp_snprintf(
                str, str_len, "%s", bactext_notify_type_name(value));
            break;
        case PROP_TIMER_STATE:
            ret_val = bacapp_snprintf(
                str, str_len, "%s", bactext_timer_state_name(value));
            break;
        case PROP_LAST_STATE_CHANGE:
            ret_val = bacapp_snprintf(
                str, str_len, "%s", bactext_timer_transition_name(value));
            break;
        default:
            ret_val =
                bacapp_snprintf(str, str_len, "%lu", (unsigned long)value);
            break;
    }

    return ret_val;
}
#endif

#if defined(BACAPP_DATE)
/**
 * @brief Print a date value to a string for EPICS
 * @param str - destination string, or NULL for length only
 * @param str_len - length of the destination string, or 0 for length only
 * @param bdate - date value to print
 * @return number of characters written
 * @note 135.1-4.4 Notational Rules for Parameter Values
 * (j) dates are represented enclosed in parenthesis:
 *     (Monday, 24-January-1998).
 *     Any "wild card" or unspecified field is shown by an asterisk (X'2A'):
 *     (Monday, *-January-1998).
 *     The omission of day of week implies that the day is unspecified:
 *     (24-January-1998);
 */
static int
bacapp_snprintf_date(char *str, size_t str_len, const BACNET_DATE *bdate)
{
    int ret_val = 0;
    int slen = 0;
    const char *weekday_text, *month_text;

    weekday_text = bactext_day_of_week_name(bdate->wday);
    month_text = bactext_month_name(bdate->month);
    slen = bacapp_snprintf(str, str_len, "%s, %s", weekday_text, month_text);
    ret_val += bacapp_snprintf_shift(slen, &str, &str_len);
    if (bdate->day == 255) {
        slen = bacapp_snprintf(str, str_len, " (unspecified), ");
    } else {
        slen = bacapp_snprintf(str, str_len, " %u, ", (unsigned)bdate->day);
    }
    ret_val += bacapp_snprintf_shift(slen, &str, &str_len);
    if (bdate->year == 2155) {
        slen = bacapp_snprintf(str, str_len, "(unspecified)");
    } else {
        slen = bacapp_snprintf(str, str_len, "%u", (unsigned)bdate->year);
    }
    ret_val += slen;

    return ret_val;
}
#endif

#if defined(BACAPP_LOG_RECORD)
/**
 * @brief Print a date value to a string as numeric
 * @param str - destination string, or NULL for length only
 * @param str_len - length of the destination string, or 0 for length only
 * @param bdate - date value to print
 * @return number of characters written
 * @note Numeric will be in the format: yyyy-mm-dd
 */
static int bacapp_snprintf_date_numeric(
    char *str, size_t str_len, const BACNET_DATE *bdate)
{
    int ret_val = 0;
    int slen = 0;

    if (bdate->year == 2155) {
        slen = bacapp_snprintf(str, str_len, "****-");
    } else {
        slen = bacapp_snprintf(str, str_len, "%04u-", (unsigned)bdate->year);
    }
    ret_val += bacapp_snprintf_shift(slen, &str, &str_len);
    if (bdate->month == 255) {
        slen = bacapp_snprintf(str, str_len, "**-");
    } else {
        slen = bacapp_snprintf(str, str_len, "%02u-", (unsigned)bdate->month);
    }
    ret_val += bacapp_snprintf_shift(slen, &str, &str_len);
    if (bdate->day == 255) {
        slen = bacapp_snprintf(str, str_len, "**");
    } else {
        slen = bacapp_snprintf(str, str_len, "%02u", (unsigned)bdate->day);
    }
    ret_val += bacapp_snprintf_shift(slen, &str, &str_len);

    return ret_val;
}
#endif

#if defined(BACAPP_TIME)
/**
 * @brief Print a time value to a string for EPICS
 * @param str - destination string, or NULL for length only
 * @param str_len - length of the destination string, or 0 for length only
 * @param btime - date value to print
 * @return number of characters written
 * @note 135.1-4.4 Notational Rules for Parameter Values
 * (k) times are represented as hours, minutes, seconds, hundredths
 *     in the format hh:mm:ss.xx: 2:05:44.00, 16:54:59.99.
 *     Any "wild card" field is shown by an asterisk (X'2A'): 16:54:*.*;
 */
static int
bacapp_snprintf_time(char *str, size_t str_len, const BACNET_TIME *btime)
{
    int ret_val = 0;
    int slen = 0;

    if (btime->hour == 255) {
        slen = bacapp_snprintf(str, str_len, "**:");
    } else {
        slen = bacapp_snprintf(str, str_len, "%02u:", (unsigned)btime->hour);
    }
    ret_val += bacapp_snprintf_shift(slen, &str, &str_len);
    if (btime->min == 255) {
        slen = bacapp_snprintf(str, str_len, "**:");
    } else {
        slen = bacapp_snprintf(str, str_len, "%02u:", (unsigned)btime->min);
    }
    ret_val += bacapp_snprintf_shift(slen, &str, &str_len);
    if (btime->sec == 255) {
        slen = bacapp_snprintf(str, str_len, "**.");
    } else {
        slen = bacapp_snprintf(str, str_len, "%02u.", (unsigned)btime->sec);
    }
    ret_val += bacapp_snprintf_shift(slen, &str, &str_len);
    if (btime->hundredths == 255) {
        slen = bacapp_snprintf(str, str_len, "**");
    } else {
        slen =
            bacapp_snprintf(str, str_len, "%02u", (unsigned)btime->hundredths);
    }
    ret_val += slen;

    return ret_val;
}
#endif

#if defined(BACAPP_OBJECT_ID) || defined(BACAPP_DEVICE_OBJECT_REFERENCE)
/**
 * @brief Print a value to a string for EPICS
 * @param str - destination string, or NULL for length only
 * @param str_len - length of the destination string, or 0 for length only
 * @param value - value to print
 * @return number of characters written
 */
static int bacapp_snprintf_object_id(
    char *str, size_t str_len, const BACNET_OBJECT_ID *object_id)
{
    int ret_val = 0;
    int slen = 0;

    slen = bacapp_snprintf(str, str_len, "(");
    ret_val += bacapp_snprintf_shift(slen, &str, &str_len);
    if (object_id->type <= BACNET_OBJECT_TYPE_RESERVED_MIN) {
        slen = bacapp_snprintf(
            str, str_len, "%s, ", bactext_object_type_name(object_id->type));
    } else if (object_id->type < BACNET_OBJECT_TYPE_RESERVED_MAX) {
        slen = bacapp_snprintf(
            str, str_len, "reserved %u, ", (unsigned)object_id->type);
    } else {
        slen = bacapp_snprintf(
            str, str_len, "proprietary-%u, ", (unsigned)object_id->type);
    }
    ret_val += bacapp_snprintf_shift(slen, &str, &str_len);
    slen = bacapp_snprintf(
        str, str_len, "%lu)", (unsigned long)object_id->instance);
    ret_val += slen;

    return ret_val;
}
#endif

#if defined(BACAPP_DATETIME)
/**
 * @brief Print a value to a string for EPICS
 * @param str - destination string, or NULL for length only
 * @param str_len - length of the destination string, or 0 for length only
 * @param value - value to print
 * @return number of characters written
 */
static int bacapp_snprintf_datetime(
    char *str, size_t str_len, const BACNET_DATE_TIME *value)
{
    int ret_val = 0;
    int slen = 0;

    slen = bacapp_snprintf(str, str_len, "{");
    ret_val += bacapp_snprintf_shift(slen, &str, &str_len);
    slen = bacapp_snprintf_date(str, str_len, &value->date);
    ret_val += bacapp_snprintf_shift(slen, &str, &str_len);
    slen = bacapp_snprintf(str, str_len, "-");
    ret_val += bacapp_snprintf_shift(slen, &str, &str_len);
    slen = bacapp_snprintf_time(str, str_len, &value->time);
    ret_val += bacapp_snprintf_shift(slen, &str, &str_len);
    ret_val += bacapp_snprintf(str, str_len, "}");

    return ret_val;
}
#endif

#if defined(BACAPP_LOG_RECORD)
/**
 * @brief Print a value to a string as numeric
 * @param str - destination string, or NULL for length only
 * @param str_len - length of the destination string, or 0 for length only
 * @param value - value to print
 * @return number of characters written
 */
static int bacapp_snprintf_datetime_numeric(
    char *str, size_t str_len, const BACNET_DATE_TIME *value)
{
    int ret_val = 0;
    int slen = 0;

    slen = bacapp_snprintf(str, str_len, "{");
    ret_val += bacapp_snprintf_shift(slen, &str, &str_len);
    slen = bacapp_snprintf_date_numeric(str, str_len, &value->date);
    ret_val += bacapp_snprintf_shift(slen, &str, &str_len);
    slen = bacapp_snprintf(str, str_len, "-");
    ret_val += bacapp_snprintf_shift(slen, &str, &str_len);
    slen = bacapp_snprintf_time(str, str_len, &value->time);
    ret_val += bacapp_snprintf_shift(slen, &str, &str_len);
    ret_val += bacapp_snprintf(str, str_len, "}");

    return ret_val;
}
#endif

#if defined(BACAPP_DATERANGE) || defined(BACAPP_CALENDAR_ENTRY)
/**
 * @brief Print a value to a string for EPICS
 * @param str - destination string, or NULL for length only
 * @param str_len - length of the destination string, or 0 for length only
 * @param value - value to print
 * @return number of characters written
 */
static int bacapp_snprintf_daterange(
    char *str, size_t str_len, const BACNET_DATE_RANGE *value)
{
    int ret_val = 0;
    int slen = 0;

    slen = bacapp_snprintf(str, str_len, "{");
    ret_val += bacapp_snprintf_shift(slen, &str, &str_len);
    slen = bacapp_snprintf_date(str, str_len, &value->startdate);
    ret_val += bacapp_snprintf_shift(slen, &str, &str_len);
    slen = bacapp_snprintf(str, str_len, "..");
    ret_val += bacapp_snprintf_shift(slen, &str, &str_len);
    slen = bacapp_snprintf_date(str, str_len, &value->enddate);
    ret_val += bacapp_snprintf_shift(slen, &str, &str_len);
    ret_val += bacapp_snprintf(str, str_len, "}");

    return ret_val;
}
#endif

#if defined(BACAPP_SPECIAL_EVENT) || defined(BACAPP_CALENDAR_ENTRY)
/**
 * @brief Print a value to a string for EPICS
 * @param str - destination string, or NULL for length only
 * @param str_len - length of the destination string, or 0 for length only
 * @param value - value to print
 * @return number of characters written
 * (j) dates are represented enclosed in parenthesis:
 *     (Monday, 24-January-1998).
 *     Any "wild card" or unspecified field is shown by an asterisk (X'2A'):
 *     (Monday, *-January-1998).
 *     The omission of day of week implies that the day is unspecified:
 *     (24-January-1998);
 */
static int bacapp_snprintf_weeknday(
    char *str, size_t str_len, const BACNET_WEEKNDAY *value)
{
    int ret_val = 0;
    int slen = 0;

    slen = bacapp_snprintf(str, str_len, "{");
    ret_val += bacapp_snprintf_shift(slen, &str, &str_len);
    /* 1=Jan 13=odd 14=even FF=any */
    if (value->month == 255) {
        slen = bacapp_snprintf(str, str_len, "*, ");
    } else if (value->month == 13) {
        slen = bacapp_snprintf(str, str_len, "odd, ");
    } else if (value->month == 14) {
        slen = bacapp_snprintf(str, str_len, "even, ");
    } else {
        slen = bacapp_snprintf(str, str_len, "%u, ", (unsigned)value->month);
    }
    ret_val += bacapp_snprintf_shift(slen, &str, &str_len);
    /* 1=days 1-7, 2=days 8-14, 3=days 15-21, 4=days 22-28,
       5=days 29-31, 6=last 7 days, FF=any week */
    if (value->weekofmonth == 255) {
        slen = bacapp_snprintf(str, str_len, "*, ");
    } else if (value->weekofmonth == 6) {
        slen = bacapp_snprintf(str, str_len, "last, ");
    } else {
        slen =
            bacapp_snprintf(str, str_len, "%u, ", (unsigned)value->weekofmonth);
    }
    ret_val += bacapp_snprintf_shift(slen, &str, &str_len);
    /* 1=Monday-7=Sunday, FF=any */
    if (value->dayofweek == 255) {
        slen = bacapp_snprintf(str, str_len, "*");
    } else {
        slen = bacapp_snprintf(
            str, str_len, "%s", bactext_day_of_week_name(value->dayofweek));
    }
    ret_val += bacapp_snprintf_shift(slen, &str, &str_len);
    ret_val += bacapp_snprintf(str, str_len, "}");

    return ret_val;
}
#endif

#if defined(BACAPP_DEVICE_OBJECT_PROPERTY_REFERENCE)
/**
 * @brief Print a BACnetDeviceObjectPropertyReference for EPICS
 * @param str - destination string, or NULL for length only
 * @param str_len - length of the destination string, or 0 for length only
 * @param value - value to print
 * @return number of characters written
 */
static int bacapp_snprintf_device_object_property_reference(
    char *str,
    size_t str_len,
    const BACNET_DEVICE_OBJECT_PROPERTY_REFERENCE *value)
{
    int slen;
    int ret_val = 0;

    slen = bacapp_snprintf(str, str_len, "{");
    ret_val += bacapp_snprintf_shift(slen, &str, &str_len);
    /* object-identifier       [0] BACnetObjectIdentifier */
    slen = bacapp_snprintf_object_id(str, str_len, &value->objectIdentifier);
    ret_val += bacapp_snprintf_shift(slen, &str, &str_len);
    slen = bacapp_snprintf(str, str_len, ",");
    ret_val += bacapp_snprintf_shift(slen, &str, &str_len);
    /* property-identifier     [1] BACnetPropertyIdentifier */
    slen = bacapp_snprintf_property_identifier(
        str, str_len, value->propertyIdentifier);
    ret_val += bacapp_snprintf_shift(slen, &str, &str_len);
    slen = bacapp_snprintf(str, str_len, ",");
    ret_val += bacapp_snprintf_shift(slen, &str, &str_len);
    /* property-array-index    [2] Unsigned OPTIONAL,*/
    if (value->arrayIndex == BACNET_ARRAY_ALL) {
        slen = bacapp_snprintf(str, str_len, "-1");
    } else {
        slen = bacapp_snprintf(
            str, str_len, "%lu", (unsigned long)value->arrayIndex);
    }
    ret_val += bacapp_snprintf_shift(slen, &str, &str_len);
    slen = bacapp_snprintf(str, str_len, ",");
    ret_val += bacapp_snprintf_shift(slen, &str, &str_len);
    /* device-identifier       [3] BACnetObjectIdentifier OPTIONAL */
    slen = bacapp_snprintf_object_id(str, str_len, &value->deviceIdentifier);
    ret_val += bacapp_snprintf_shift(slen, &str, &str_len);
    ret_val += bacapp_snprintf(str, str_len, "}");

    return ret_val;
}
#endif

#if defined(BACAPP_DEVICE_OBJECT_REFERENCE)
/**
 * @brief Print a BACnetDeviceObjectReference for EPICS
 * @param str - destination string, or NULL for length only
 * @param str_len - length of the destination string, or 0 for length only
 * @param value - value to print
 * @return number of characters written
 */
static int bacapp_snprintf_device_object_reference(
    char *str, size_t str_len, const BACNET_DEVICE_OBJECT_REFERENCE *value)
{
    int slen;
    int ret_val = 0;

    /* BACnetDeviceObjectReference */
    slen = bacapp_snprintf(str, str_len, "{");
    ret_val += bacapp_snprintf_shift(slen, &str, &str_len);
    if (value->deviceIdentifier.type == OBJECT_DEVICE) {
        slen =
            bacapp_snprintf_object_id(str, str_len, &value->deviceIdentifier);
        ret_val += bacapp_snprintf_shift(slen, &str, &str_len);
        slen = bacapp_snprintf(str, str_len, ",");
        ret_val += bacapp_snprintf_shift(slen, &str, &str_len);
    }
    slen = bacapp_snprintf_object_id(str, str_len, &value->objectIdentifier);
    ret_val += bacapp_snprintf_shift(slen, &str, &str_len);
    slen = bacapp_snprintf(str, str_len, "}");
    ret_val += slen;

    return ret_val;
}
#endif

#if defined(BACAPP_OBJECT_PROPERTY_REFERENCE)
/**
 * @brief Print a BACnetObjectPropertyReference for EPICS
 * @param str - destination string, or NULL for length only
 * @param str_len - length of the destination string, or 0 for length only
 * @param value - value to print
 * @return number of characters written
 */
static int bacapp_snprintf_object_property_reference(
    char *str, size_t str_len, const BACNET_OBJECT_PROPERTY_REFERENCE *value)
{
    int slen;
    int ret_val = 0;

    slen = bacapp_snprintf(str, str_len, "{");
    ret_val += bacapp_snprintf_shift(slen, &str, &str_len);
    if (value->object_identifier.type != OBJECT_NONE) {
        /* object-identifier [0] BACnetObjectIdentifier */
        slen =
            bacapp_snprintf_object_id(str, str_len, &value->object_identifier);
        ret_val += bacapp_snprintf_shift(slen, &str, &str_len);
        slen = bacapp_snprintf(str, str_len, ",");
        ret_val += bacapp_snprintf_shift(slen, &str, &str_len);
    }
    /* property-identifier [1] BACnetPropertyIdentifier */
    slen = bacapp_snprintf_property_identifier(
        str, str_len, value->property_identifier);
    ret_val += bacapp_snprintf_shift(slen, &str, &str_len);
    if (value->property_array_index != BACNET_ARRAY_ALL) {
        /* property-array-index [2] Unsigned OPTIONAL */
        slen = bacapp_snprintf(
            str, str_len, ", %lu", (unsigned long)value->property_array_index);
        ret_val += bacapp_snprintf_shift(slen, &str, &str_len);
    }
    ret_val += bacapp_snprintf(str, str_len, "}");

    return ret_val;
}
#endif

#if defined(BACAPP_ACCESS_RULE)
/**
 * @brief Print a value to a string for EPICS
 * @param str - destination string, or NULL for length only
 * @param str_len - length of the destination string, or 0 for length only
 * @param value - value to be printed
 * @return number of characters written to the string
 */
static int bacapp_snprintf_access_rule(
    char *str, size_t str_len, const BACNET_ACCESS_RULE *value)
{
    int slen;
    int ret_val = 0;

    slen = bacapp_snprintf(str, str_len, "{");
    ret_val += bacapp_snprintf_shift(slen, &str, &str_len);
    /*  specified (0), always (1) */
    if (value->time_range_specifier == TIME_RANGE_SPECIFIER_SPECIFIED) {
        slen = bacapp_snprintf(str, str_len, "specified, ");
        ret_val += bacapp_snprintf_shift(slen, &str, &str_len);
        slen = bacapp_snprintf_device_object_property_reference(
            str, str_len, &value->time_range);
        ret_val += bacapp_snprintf_shift(slen, &str, &str_len);
    } else {
        slen = bacapp_snprintf(str, str_len, "always, ");
        ret_val += bacapp_snprintf_shift(slen, &str, &str_len);
    }
    /* specified (0), all (1) */
    if (value->location_specifier == LOCATION_SPECIFIER_SPECIFIED) {
        slen = bacapp_snprintf(str, str_len, "specified, ");
        ret_val += bacapp_snprintf_shift(slen, &str, &str_len);
        slen = bacapp_snprintf_device_object_reference(
            str, str_len, &value->location);
        ret_val += bacapp_snprintf_shift(slen, &str, &str_len);
    } else {
        slen = bacapp_snprintf(str, str_len, "all");
        ret_val += bacapp_snprintf_shift(slen, &str, &str_len);
    }
    slen = bacapp_snprintf(str, str_len, "}");
    ret_val += bacapp_snprintf_shift(slen, &str, &str_len);

    return ret_val;
}
#endif

#if defined(BACAPP_COLOR_COMMAND)
/**
 * @brief Print a value to a string for EPICS
 * @param str - destination string, or NULL for length only
 * @param str_len - length of the destination string, or 0 for length only
 * @param value - value to be printed
 * @return number of characters written to the string
 */
static int bacapp_snprintf_color_command(
    char *str, size_t str_len, const BACNET_COLOR_COMMAND *value)
{
    int slen;
    int ret_val = 0;

    slen = bacapp_snprintf(str, str_len, "{");
    ret_val += bacapp_snprintf_shift(slen, &str, &str_len);
    slen = bacapp_snprintf(str, str_len, "(");
    ret_val += bacapp_snprintf_shift(slen, &str, &str_len);
    slen = bacapp_snprintf(
        str, str_len, "%s", bactext_color_operation_name(value->operation));
    ret_val += bacapp_snprintf_shift(slen, &str, &str_len);
    slen = bacapp_snprintf(str, str_len, ")");
    ret_val += bacapp_snprintf_shift(slen, &str, &str_len);

    /* FIXME: add the Color Command optional values */

    slen = bacapp_snprintf(str, str_len, "}");
    ret_val += bacapp_snprintf_shift(slen, &str, &str_len);
    return ret_val;
}
#endif

#if defined(BACAPP_CHANNEL_VALUE)
/**
 * @brief Print a value to a string for EPICS
 * @param str - destination string, or NULL for length only
 * @param str_len - length of the destination string, or 0 for length only
 * @param value - value to be printed
 * @return number of characters written to the string
 */
static int bacapp_snprintf_channel_value(
    char *str, size_t str_len, const BACNET_CHANNEL_VALUE *value)
{
    int ret_val = 0;

    switch (value->tag) {
        case BACNET_APPLICATION_TAG_NULL:
            ret_val = bacapp_snprintf_null(str, str_len);
            break;
        case BACNET_APPLICATION_TAG_BOOLEAN:
#if defined(CHANNEL_BOOLEAN)
            ret_val =
                bacapp_snprintf_boolean(str, str_len, value->type.Boolean);
#endif
            break;
        case BACNET_APPLICATION_TAG_UNSIGNED_INT:
#if defined(CHANNEL_UNSIGNED)
            ret_val = bacapp_snprintf_unsigned_integer(
                str, str_len, value->type.Unsigned_Int);
#endif
            break;
        case BACNET_APPLICATION_TAG_SIGNED_INT:
#if defined(CHANNEL_SIGNED)
            ret_val = bacapp_snprintf_signed_integer(
                str, str_len, value->type.Signed_Int);
#endif
            break;
        case BACNET_APPLICATION_TAG_REAL:
#if defined(CHANNEL_REAL)
            ret_val = bacapp_snprintf_real(str, str_len, value->type.Real);
#endif
            break;
        case BACNET_APPLICATION_TAG_DOUBLE:
#if defined(CHANNEL_DOUBLE)
            ret_val = bacapp_snprintf_double(str, str_len, value->type.Double);
#endif
            break;
        case BACNET_APPLICATION_TAG_ENUMERATED:
#if defined(CHANNEL_ENUMERATED)
            ret_val = bacapp_snprintf_enumerated(
                str, str_len, OBJECT_COMMAND, PROP_ACTION,
                value->type.Enumerated);
#endif
            break;
        case BACNET_APPLICATION_TAG_LIGHTING_COMMAND:
#if defined(CHANNEL_LIGHTING_COMMAND)
            ret_val = lighting_command_to_ascii(
                &value->type.Lighting_Command, str, str_len);
#endif
            break;
        case BACNET_APPLICATION_TAG_COLOR_COMMAND:
#if defined(CHANNEL_COLOR_COMMAND)
            ret_val = bacapp_snprintf_color_command(
                str, str_len, &value->type.Color_Command);
#endif
            break;
        case BACNET_APPLICATION_TAG_XY_COLOR:
#if defined(CHANNEL_XY_COLOR)
            ret_val = xy_color_to_ascii(&value->type.XY_Color, str, str_len);
#endif
            break;
        default:
            break;
    }

    return ret_val;
}
#endif

#if defined(BACAPP_CHANNEL)
/**
 * @brief For a given application value, copy to the channel value
 * @param  cvalue - BACNET_CHANNEL_VALUE value
 * @param  value - BACNET_APPLICATION_DATA_VALUE value
 * @return  true if values are able to be copied
 */
bool bacapp_channel_value_copy(
    BACNET_CHANNEL_VALUE *cvalue, const BACNET_APPLICATION_DATA_VALUE *value)
{
    bool status = false;

    if (!value || !cvalue) {
        return false;
    }
    switch (value->tag) {
#if defined(BACAPP_NULL)
        case BACNET_APPLICATION_TAG_NULL:
            cvalue->tag = value->tag;
            status = true;
            break;
#endif
#if defined(BACAPP_BOOLEAN) && defined(CHANNEL_BOOLEAN)
        case BACNET_APPLICATION_TAG_BOOLEAN:
            cvalue->tag = value->tag;
            cvalue->type.Boolean = value->type.Boolean;
            status = true;
            break;
#endif
#if defined(BACAPP_UNSIGNED) && defined(CHANNEL_UNSIGNED)
        case BACNET_APPLICATION_TAG_UNSIGNED_INT:
            cvalue->tag = value->tag;
            cvalue->type.Unsigned_Int = value->type.Unsigned_Int;
            status = true;
            break;
#endif
#if defined(BACAPP_SIGNED) && defined(CHANNEL_SIGNED)
        case BACNET_APPLICATION_TAG_SIGNED_INT:
            cvalue->tag = value->tag;
            cvalue->type.Signed_Int = value->type.Signed_Int;
            status = true;
            break;
#endif
#if defined(BACAPP_REAL) && defined(CHANNEL_REAL)
        case BACNET_APPLICATION_TAG_REAL:
            cvalue->tag = value->tag;
            cvalue->type.Real = value->type.Real;
            status = true;
            break;
#endif
#if defined(BACAPP_DOUBLE) && defined(CHANNEL_DOUBLE)
        case BACNET_APPLICATION_TAG_DOUBLE:
            cvalue->tag = value->tag;
            cvalue->type.Double = value->type.Double;
            status = true;
            break;
#endif
#if defined(BACAPP_OCTET_STRING) && defined(CHANNEL_OCTET_STRING)
        case BACNET_APPLICATION_TAG_OCTET_STRING:
            cvalue->tag = value->tag;
            octetstring_copy(
                &cvalue->type.Octet_String, &value->type.Octet_String);
            status = true;
            break;
#endif
#if defined(BACAPP_CHARACTER_STRING) && defined(CHANNEL_CHARACTER_STRING)
        case BACNET_APPLICATION_TAG_CHARACTER_STRING:
            cvalue->tag = value->tag;
            characterstring_copy(
                &cvalue->type.Character_String, &value->type.Character_String);
            status = true;
            break;
#endif
#if defined(BACAPP_BIT_STRING) && defined(CHANNEL_BIT_STRING)
        case BACNET_APPLICATION_TAG_BIT_STRING:
            cvalue->tag = value->tag;
            bitstring_copy(&cvalue->type.Bit_String, &value->type.Bit_String);
            status = true;
            break;
#endif
#if defined(BACAPP_ENUMERATED) && defined(CHANNEL_ENUMERATED)
        case BACNET_APPLICATION_TAG_ENUMERATED:
            cvalue->tag = value->tag;
            cvalue->type.Enumerated = value->type.Enumerated;
            status = true;
            break;
#endif
#if defined(BACAPP_DATE) && defined(CHANNEL_DATE)
        case BACNET_APPLICATION_TAG_DATE:
            cvalue->tag = value->tag;
            datetime_date_copy(&cvalue->type.Date, &value->type.Date);
            apdu_len = encode_application_date(apdu, &value->type.Date);
            status = true;
            break;
#endif
#if defined(BACAPP_TIME) && defined(CHANNEL_TIME)
        case BACNET_APPLICATION_TAG_TIME:
            cvalue->tag = value->tag;
            datetime_time_copy(&cvalue->type.Time, &value->type.Time);
            break;
#endif
#if defined(BACAPP_OBJECT_ID) && defined(CHANNEL_OBJECT_ID)
        case BACNET_APPLICATION_TAG_OBJECT_ID:
            cvalue->tag = value->tag;
            cvalue->type.Object_Id.type = value->type.Object_Id.type;
            cvalue->type.Object_Id.instance = value->type.Object_Id.instance;
            status = true;
            break;
#endif
#if defined(BACAPP_LIGHTING_COMMAND) && defined(CHANNEL_LIGHTING_COMMAND)
        case BACNET_APPLICATION_TAG_LIGHTING_COMMAND:
            cvalue->tag = value->tag;
            lighting_command_copy(
                &cvalue->type.Lighting_Command, &value->type.Lighting_Command);
            status = true;
            break;
#endif
#if defined(BACAPP_COLOR_COMMAND) && defined(CHANNEL_COLOR_COMMAND)
        case BACNET_APPLICATION_TAG_COLOR_COMMAND:
            cvalue->tag = value->tag;
            color_command_copy(
                &cvalue->type.Color_Command, &value->type.Color_Command);
            status = true;
            break;
#endif
#if defined(BACAPP_COLOR_COMMAND) && defined(CHANNEL_XY_COLOR)
        case BACNET_APPLICATION_TAG_XY_COLOR:
            cvalue->tag = value->tag;
            xy_color_copy(&cvalue->type.XY_Color, &value->type.XY_Color);
            status = true;
            break;
#endif
        default:
            break;
    }

    return status;
}
#endif

#if defined(BACAPP_LOG_RECORD)
/**
 * @brief Print a value to a string for EPICS
 * @param str - destination string, or NULL for length only
 * @param str_len - length of the destination string, or 0 for length only
 * @param value - value to be printed
 * @return number of characters written to the string
 */
static int bacapp_snprintf_log_record(
    char *str, size_t str_len, const BACNET_LOG_RECORD *value)
{
    int ret_val = 0, slen;
    BACNET_BIT_STRING bitstring;

    slen = bacapp_snprintf(str, str_len, "{");
    ret_val += bacapp_snprintf_shift(slen, &str, &str_len);
    slen = bacapp_snprintf_datetime_numeric(str, str_len, &value->timestamp);
    ret_val += bacapp_snprintf_shift(slen, &str, &str_len);
    if (value->tag < BACNET_LOG_DATUM_MAX) {
        slen = bacapp_snprintf(
            str, str_len, ", %s:", bactext_log_datum_name(value->tag));
    } else {
        slen =
            bacapp_snprintf(str, str_len, ", <tag=%u>:", (unsigned)value->tag);
    }
    ret_val += bacapp_snprintf_shift(slen, &str, &str_len);
    switch (value->tag) {
        case BACNET_LOG_DATUM_STATUS:
            bitstring_init(&bitstring);
            bitstring_set_bits_used(&bitstring, 1, 8 - 3);
            bitstring_set_octet(&bitstring, 0, value->log_datum.log_status);
            slen = bacapp_snprintf_bit_string(str, str_len, &bitstring);
            break;
        case BACNET_LOG_DATUM_BOOLEAN:
            slen = bacapp_snprintf_boolean(
                str, str_len, value->log_datum.boolean_value);
            break;
        case BACNET_LOG_DATUM_REAL:
            slen =
                bacapp_snprintf_real(str, str_len, value->log_datum.real_value);
            break;
        case BACNET_LOG_DATUM_ENUMERATED:
            slen = bacapp_snprintf(
                str, str_len, "%lu",
                (unsigned long)value->log_datum.enumerated_value);
            break;
        case BACNET_LOG_DATUM_UNSIGNED:
            slen = bacapp_snprintf_unsigned_integer(
                str, str_len, value->log_datum.unsigned_value);
            break;
        case BACNET_LOG_DATUM_SIGNED:
            slen = bacapp_snprintf_signed_integer(
                str, str_len, value->log_datum.integer_value);
            break;
        case BACNET_LOG_DATUM_NULL:
            slen = bacapp_snprintf_null(str, str_len);
            break;
        case BACNET_LOG_DATUM_FAILURE:
            slen = bacapp_snprintf(
                str, str_len, "%s,%s",
                bactext_error_class_name(value->log_datum.failure.error_class),
                bactext_error_code_name(value->log_datum.failure.error_code));
            break;
        case BACNET_LOG_DATUM_TIME_CHANGE:
            slen = bacapp_snprintf_real(
                str, str_len, value->log_datum.time_change);
            break;
        default:
            slen = 0;
            break;
    }
    ret_val += bacapp_snprintf_shift(slen, &str, &str_len);
    /* FIXME: optional status flags */
    slen = bacapp_snprintf(str, str_len, "}");
    ret_val += bacapp_snprintf_shift(slen, &str, &str_len);

    return ret_val;
}
#endif

#if defined(BACAPP_WEEKLY_SCHEDULE)
/**
 * @brief Print a weekly schedule value to a string for EPICS
 * @param str - destination string, or NULL for length only
 * @param str_len - length of the destination string, or 0 for length only
 * @param ws - weekly schedule value to print
 * @param arrayIndex - index of the weekly schedule to print
 * @return number of characters written
 */
static int bacapp_snprintf_weeklyschedule(
    char *str,
    size_t str_len,
    const BACNET_WEEKLY_SCHEDULE *ws,
    BACNET_ARRAY_INDEX arrayIndex)
{
    int slen;
    int ret_val = 0;
    int wi, ti;
    BACNET_OBJECT_PROPERTY_VALUE dummyPropValue;
    BACNET_APPLICATION_DATA_VALUE dummyDataValue = { 0 };

    const char *weekdaynames[7] = { "Mon", "Tue", "Wed", "Thu",
                                    "Fri", "Sat", "Sun" };
    const int loopend = ((arrayIndex == BACNET_ARRAY_ALL) ? 7 : 1);

    /* Find what inner type it uses */
    int inner_tag = -1;
    for (wi = 0; wi < loopend; wi++) {
        const BACNET_DAILY_SCHEDULE *ds = &ws->weeklySchedule[wi];
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
        slen = bacapp_snprintf(str, str_len, "(Null; ");
    } else if (inner_tag == -2) {
        slen = bacapp_snprintf(str, str_len, "(MIXED_TYPES; ");
    } else {
        slen = bacapp_snprintf(
            str, str_len, "(%s; ", bactext_application_tag_name(inner_tag));
    }
    ret_val += bacapp_snprintf_shift(slen, &str, &str_len);
    for (wi = 0; wi < loopend; wi++) {
        const BACNET_DAILY_SCHEDULE *ds = &ws->weeklySchedule[wi];
        if (arrayIndex == BACNET_ARRAY_ALL) {
            slen = bacapp_snprintf(str, str_len, "%s: [", weekdaynames[wi]);
        } else {
            slen = bacapp_snprintf(
                str, str_len, "%s: [",
                (arrayIndex >= 1 && arrayIndex <= 7)
                    ? weekdaynames[arrayIndex - 1]
                    : "???");
        }
        ret_val += bacapp_snprintf_shift(slen, &str, &str_len);
        for (ti = 0; ti < ds->TV_Count; ti++) {
            slen =
                bacapp_snprintf_time(str, str_len, &ds->Time_Values[ti].Time);
            ret_val += bacapp_snprintf_shift(slen, &str, &str_len);
            slen = bacapp_snprintf(str, str_len, " ");
            ret_val += bacapp_snprintf_shift(slen, &str, &str_len);
            bacnet_primitive_to_application_data_value(
                &dummyDataValue, &ds->Time_Values[ti].Value);
            dummyPropValue.value = &dummyDataValue;
            dummyPropValue.object_property = PROP_PRESENT_VALUE;
            dummyPropValue.object_type = OBJECT_SCHEDULE;
            dummyPropValue.array_index = 0;
            slen = bacapp_snprintf_value(str, str_len, &dummyPropValue);
            ret_val += bacapp_snprintf_shift(slen, &str, &str_len);
            if (ti < ds->TV_Count - 1) {
                slen = bacapp_snprintf(str, str_len, ", ");
                ret_val += bacapp_snprintf_shift(slen, &str, &str_len);
            }
        }
        if (wi < loopend - 1) {
            slen = bacapp_snprintf(str, str_len, "]; ");
            ret_val += bacapp_snprintf_shift(slen, &str, &str_len);
        }
    }
    slen = bacapp_snprintf(str, str_len, "])");
    ret_val += slen;
    return ret_val;
}
#endif

#if defined(BACAPP_HOST_N_PORT)
/**
 * @brief Print a value to a string for EPICS
 * @param str - destination string, or NULL for length only
 * @param str_len - length of the destination string, or 0 for length only
 * @param value - value to print
 * @return number of characters written
 */
static int bacapp_snprintf_host_n_port(
    char *str, size_t str_len, const BACNET_HOST_N_PORT *value)
{
    int slen, len, i;
    const char *char_str;
    int ret_val = 0;

    slen = bacapp_snprintf(str, str_len, "{");
    ret_val += bacapp_snprintf_shift(slen, &str, &str_len);
    if (value->host_ip_address) {
        const uint8_t *octet_str;
        octet_str =
            octetstring_value((BACNET_OCTET_STRING *)&value->host.ip_address);
        slen = bacapp_snprintf(
            str, str_len, "%u.%u.%u.%u:%u", (unsigned)octet_str[0],
            (unsigned)octet_str[1], (unsigned)octet_str[2],
            (unsigned)octet_str[3], (unsigned)value->port);
        ret_val += slen;
    } else if (value->host_name) {
        const BACNET_CHARACTER_STRING *name;
        name = &value->host.name;
        len = characterstring_length(name);
        char_str = characterstring_value(name);
        slen = bacapp_snprintf(str, str_len, "\"");
        ret_val += bacapp_snprintf_shift(slen, &str, &str_len);
        for (i = 0; i < len; i++) {
            if (isprint(*((const unsigned char *)char_str))) {
                slen = bacapp_snprintf(str, str_len, "%c", *char_str);
            } else {
                slen = bacapp_snprintf(str, str_len, "%c", '.');
            }
            ret_val += bacapp_snprintf_shift(slen, &str, &str_len);
            char_str++;
        }
        slen = bacapp_snprintf(str, str_len, "\"");
        ret_val += slen;
    }
    slen = bacapp_snprintf(str, str_len, "}");
    ret_val += bacapp_snprintf_shift(slen, &str, &str_len);

    return ret_val;
}
#endif

#if defined(BACAPP_SPECIAL_EVENT) || defined(BACAPP_CALENDAR_ENTRY)
/**
 * @brief Print a value to a string for EPICS
 * @param str - destination string, or NULL for length only
 * @param str_len - length of the destination string, or 0 for length only
 * @param value - value to print
 * @return number of characters written
 */
static int bacapp_snprintf_calendar_entry(
    char *str, size_t str_len, const BACNET_CALENDAR_ENTRY *value)
{
    int slen;
    int ret_val = 0;

    slen = bacapp_snprintf(str, str_len, "{");
    ret_val += bacapp_snprintf_shift(slen, &str, &str_len);
    switch (value->tag) {
        case BACNET_CALENDAR_DATE:
            slen = bacapp_snprintf_date(str, str_len, &value->type.Date);
            ret_val += bacapp_snprintf_shift(slen, &str, &str_len);
            break;
        case BACNET_CALENDAR_DATE_RANGE:
            slen =
                bacapp_snprintf_daterange(str, str_len, &value->type.DateRange);
            ret_val += bacapp_snprintf_shift(slen, &str, &str_len);
            break;
        case BACNET_CALENDAR_WEEK_N_DAY:
            slen =
                bacapp_snprintf_weeknday(str, str_len, &value->type.WeekNDay);
            ret_val += bacapp_snprintf_shift(slen, &str, &str_len);
            break;
        default:
            /* do nothing */
            break;
    }
    slen = bacapp_snprintf(str, str_len, "}");
    ret_val += bacapp_snprintf_shift(slen, &str, &str_len);

    return ret_val;
}
#endif

#if defined(BACAPP_SPECIAL_EVENT)
/**
 * @brief Print a value to a string for EPICS
 * @param str - destination string, or NULL for length only
 * @param str_len - length of the destination string, or 0 for length only
 * @param value - value to print
 * @return number of characters written
 */
static int bacapp_snprintf_primitive_data_value(
    char *str, size_t str_len, const BACNET_PRIMITIVE_DATA_VALUE *value)
{
    int ret_val = 0;

    switch (value->tag) {
#if defined(BACAPP_NULL)
        case BACNET_APPLICATION_TAG_NULL:
            ret_val = bacapp_snprintf_null(str, str_len);
            break;
#endif
#if defined(BACAPP_BOOLEAN)
        case BACNET_APPLICATION_TAG_BOOLEAN:
            ret_val =
                bacapp_snprintf_boolean(str, str_len, value->type.Boolean);
            break;
#endif
#if defined(BACAPP_UNSIGNED)
        case BACNET_APPLICATION_TAG_UNSIGNED_INT:
            ret_val = bacapp_snprintf_unsigned_integer(
                str, str_len, value->type.Unsigned_Int);
            break;
#endif
#if defined(BACAPP_SIGNED)
        case BACNET_APPLICATION_TAG_SIGNED_INT:
            ret_val = bacapp_snprintf_signed_integer(
                str, str_len, value->type.Signed_Int);
            break;
#endif
#if defined(BACAPP_REAL)
        case BACNET_APPLICATION_TAG_REAL:
            ret_val = bacapp_snprintf_real(str, str_len, value->type.Real);
            break;
#endif
#if defined(BACAPP_DOUBLE)
        case BACNET_APPLICATION_TAG_DOUBLE:
            ret_val = bacapp_snprintf_double(str, str_len, value->type.Double);
            break;
#endif
#if defined(BACAPP_ENUMERATED)
        case BACNET_APPLICATION_TAG_ENUMERATED:
            ret_val = bacapp_snprintf_enumerated(
                str, str_len, OBJECT_COMMAND, PROP_ACTION,
                value->type.Enumerated);
            break;
#endif
        case BACNET_APPLICATION_TAG_EMPTYLIST:
            break;
        default:
            break;
    }
    return ret_val;
}
#endif

#if defined(BACAPP_SPECIAL_EVENT)
/**
 * @brief Print a value to a string for EPICS
 * @param str - destination string, or NULL for length only
 * @param str_len - length of the destination string, or 0 for length only
 * @param ws - value to print
 * @param arrayIndex - index of the to print
 * @return number of characters written
 */
static int bacapp_snprintf_daily_schedule(
    char *str, size_t str_len, const BACNET_DAILY_SCHEDULE *value)
{
    int slen;
    int ret_val = 0;
    uint16_t i;

    slen = bacapp_snprintf(str, str_len, "{");
    ret_val += bacapp_snprintf_shift(slen, &str, &str_len);
    for (i = 0; i < value->TV_Count; i++) {
        if (i != 0) {
            slen = bacapp_snprintf(str, str_len, ", ");
            ret_val += bacapp_snprintf_shift(slen, &str, &str_len);
        }
        slen = bacapp_snprintf_time(str, str_len, &value->Time_Values[i].Time);
        ret_val += bacapp_snprintf_shift(slen, &str, &str_len);
        slen = bacapp_snprintf(str, str_len, ",");
        ret_val += bacapp_snprintf_shift(slen, &str, &str_len);
        slen = bacapp_snprintf_primitive_data_value(
            str, str_len, &value->Time_Values[i].Value);
        ret_val += bacapp_snprintf_shift(slen, &str, &str_len);
    }
    slen = bacapp_snprintf(str, str_len, "}");
    ret_val += bacapp_snprintf_shift(slen, &str, &str_len);

    return ret_val;
}
#endif

#if defined(BACAPP_SPECIAL_EVENT)
/**
 * @brief Print a value to a string for EPICS
 * @param str - destination string, or NULL for length only
 * @param str_len - length of the destination string, or 0 for length only
 * @param ws - value to print
 * @param arrayIndex - index of the to print
 * @return number of characters written
 */
static int bacapp_snprintf_special_event(
    char *str, size_t str_len, const BACNET_SPECIAL_EVENT *value)
{
    int slen;
    int ret_val = 0;

    slen = bacapp_snprintf(str, str_len, "{");
    ret_val += bacapp_snprintf_shift(slen, &str, &str_len);
    switch (value->periodTag) {
        case BACNET_SPECIAL_EVENT_PERIOD_CALENDAR_ENTRY:
            slen = bacapp_snprintf_calendar_entry(
                str, str_len, &value->period.calendarEntry);
            ret_val += bacapp_snprintf_shift(slen, &str, &str_len);
            break;
        case BACNET_SPECIAL_EVENT_PERIOD_CALENDAR_REFERENCE:
            slen = bacapp_snprintf_object_id(
                str, str_len, &value->period.calendarReference);
            ret_val += bacapp_snprintf_shift(slen, &str, &str_len);
            break;
        default:
            break;
    }
    slen = bacapp_snprintf_daily_schedule(str, str_len, &value->timeValues);
    ret_val += bacapp_snprintf_shift(slen, &str, &str_len);
    slen = bacapp_snprintf(str, str_len, "}");
    ret_val += bacapp_snprintf_shift(slen, &str, &str_len);

    return ret_val;
}
#endif

#if defined(BACAPP_ACTION_COMMAND)
/**
 * @brief Print a weekly schedule value to a string for EPICS
 * @param str - destination string, or NULL for length only
 * @param str_len - length of the destination string, or 0 for length only
 * @param ws - weekly schedule value to print
 * @param arrayIndex - index of the weekly schedule to print
 * @return number of characters written
 */
static int bacapp_snprintf_action_property_value(
    char *str, size_t str_len, const BACNET_ACTION_PROPERTY_VALUE *value)
{
    int ret_val = 0;

    switch (value->tag) {
#if defined(BACACTION_NULL)
        case BACNET_APPLICATION_TAG_NULL:
            ret_val = bacapp_snprintf_null(str, str_len);
            break;
#endif
#if defined(BACACTION_BOOLEAN)
        case BACNET_APPLICATION_TAG_BOOLEAN:
            ret_val =
                bacapp_snprintf_boolean(str, str_len, value->type.Boolean);
            break;
#endif
#if defined(BACACTION_UNSIGNED)
        case BACNET_APPLICATION_TAG_UNSIGNED_INT:
            ret_val = bacapp_snprintf_unsigned_integer(
                str, str_len, value->type.Unsigned_Int);
            break;
#endif
#if defined(BACACTION_SIGNED)
        case BACNET_APPLICATION_TAG_SIGNED_INT:
            ret_val = bacapp_snprintf_signed_integer(
                str, str_len, value->type.Signed_Int);
            break;
#endif
#if defined(BACACTION_REAL)
        case BACNET_APPLICATION_TAG_REAL:
            ret_val = bacapp_snprintf_real(str, str_len, value->type.Real);
            break;
#endif
#if defined(BACACTION_DOUBLE)
        case BACNET_APPLICATION_TAG_DOUBLE:
            ret_val = bacapp_snprintf_double(str, str_len, value->type.Double);
            break;
#endif
#if defined(BACACTION_ENUMERATED)
        case BACNET_APPLICATION_TAG_ENUMERATED:
            ret_val = bacapp_snprintf_enumerated(
                str, str_len, OBJECT_COMMAND, PROP_ACTION,
                value->type.Enumerated);
            break;
#endif
        case BACNET_APPLICATION_TAG_EMPTYLIST:
            break;
        default:
            break;
    }

    return ret_val;
}
#endif

#if defined(BACAPP_ACTION_COMMAND)
/**
 * @brief Print a value to a string for EPICS
 * @param str - destination string, or NULL for length only
 * @param str_len - length of the destination string, or 0 for length only
 * @param value - value to print
 * @return number of characters written
 */
static int bacapp_snprintf_action_command(
    char *str, size_t str_len, const BACNET_ACTION_LIST *value)
{
    int slen;
    int ret_val = 0;

    slen = bacapp_snprintf(str, str_len, "{");
    ret_val += bacapp_snprintf_shift(slen, &str, &str_len);
    /* deviceIdentifier [0] BACnetObjectIdentifier OPTIONAL */
    slen = bacapp_snprintf_object_id(str, str_len, &value->Device_Id);
    ret_val += bacapp_snprintf_shift(slen, &str, &str_len);
    slen = bacapp_snprintf(str, str_len, ",");
    ret_val += bacapp_snprintf_shift(slen, &str, &str_len);
    /* objectIdentifier [1] BACnetObjectIdentifier */
    slen = bacapp_snprintf_object_id(str, str_len, &value->Device_Id);
    ret_val += bacapp_snprintf_shift(slen, &str, &str_len);
    slen = bacapp_snprintf(str, str_len, ",");
    ret_val += bacapp_snprintf_shift(slen, &str, &str_len);
    /* propertyIdentifier [2] BACnetPropertyIdentifier */
    slen = bacapp_snprintf_property_identifier(
        str, str_len, value->Property_Identifier);
    ret_val += bacapp_snprintf_shift(slen, &str, &str_len);
    slen = bacapp_snprintf(str, str_len, ",");
    ret_val += bacapp_snprintf_shift(slen, &str, &str_len);
    /* propertyArrayIndex [3] Unsigned OPTIONAL */
    if (value->Property_Array_Index == BACNET_ARRAY_ALL) {
        slen = bacapp_snprintf(str, str_len, "-1,");
    } else {
        slen = bacapp_snprintf(
            str, str_len, "%lu,", (unsigned long)value->Property_Array_Index);
    }
    ret_val += bacapp_snprintf_shift(slen, &str, &str_len);
    /* propertyValue [4] ABSTRACT-SYNTAX.&Type */
    slen = bacapp_snprintf_action_property_value(str, str_len, &value->Value);
    ret_val += bacapp_snprintf_shift(slen, &str, &str_len);
    slen = bacapp_snprintf(str, str_len, ",");
    ret_val += bacapp_snprintf_shift(slen, &str, &str_len);
    /* priority [5] Unsigned (1..16) OPTIONAL */
    slen = bacapp_snprintf(str, str_len, "%lu", (unsigned long)value->Priority);
    ret_val += bacapp_snprintf_shift(slen, &str, &str_len);
    slen = bacapp_snprintf(str, str_len, ",");
    ret_val += bacapp_snprintf_shift(slen, &str, &str_len);
    /* postDelay [6] Unsigned OPTIONAL */
    slen =
        bacapp_snprintf(str, str_len, "%lu", (unsigned long)value->Post_Delay);
    ret_val += bacapp_snprintf_shift(slen, &str, &str_len);
    slen = bacapp_snprintf(str, str_len, ",");
    ret_val += bacapp_snprintf_shift(slen, &str, &str_len);
    /* quitOnFailure [7] BOOLEAN */
    slen = bacapp_snprintf_boolean(str, str_len, value->Quit_On_Failure);
    ret_val += bacapp_snprintf_shift(slen, &str, &str_len);
    slen = bacapp_snprintf(str, str_len, ",");
    ret_val += bacapp_snprintf_shift(slen, &str, &str_len);
    /* writeSuccessful [8] BOOLEAN */
    slen = bacapp_snprintf_boolean(str, str_len, value->Write_Successful);
    ret_val += bacapp_snprintf_shift(slen, &str, &str_len);
    slen = bacapp_snprintf(str, str_len, "}");
    ret_val += bacapp_snprintf_shift(slen, &str, &str_len);

    return ret_val;
}
#endif

/**
 * @brief Extract the value into a text string
 * @param str - the buffer to store the extracted value, or NULL for length
 * @param str_len - the size of the buffer, or 0 for length only
 * @param object_value - ptr to BACnet object value from which to extract str
 * @return number of bytes (excluding terminating NULL byte) that were stored
 *  to the output string.
 */
int bacapp_snprintf_value(
    char *str, size_t str_len, const BACNET_OBJECT_PROPERTY_VALUE *object_value)
{
    const BACNET_APPLICATION_DATA_VALUE *value;
    BACNET_PROPERTY_ID property = PROP_ALL;
    BACNET_OBJECT_TYPE object_type = MAX_BACNET_OBJECT_TYPE;
    int ret_val = 0;
#if defined(BACAPP_BDT_ENTRY) || defined(BACAPP_FDT_ENTRY) || \
    defined(BACAPP_SHED_LEVEL)
    int slen = 0;
#endif

    if (object_value && object_value->value) {
        value = object_value->value;
        property = object_value->object_property;
        object_type = object_value->object_type;
        switch (value->tag) {
#if defined(BACAPP_NULL)
            case BACNET_APPLICATION_TAG_NULL:
                ret_val = bacapp_snprintf_null(str, str_len);
                break;
#endif
#if defined(BACAPP_BOOLEAN)
            case BACNET_APPLICATION_TAG_BOOLEAN:
                ret_val =
                    bacapp_snprintf_boolean(str, str_len, value->type.Boolean);
                break;
#endif
#if defined(BACAPP_UNSIGNED)
            case BACNET_APPLICATION_TAG_UNSIGNED_INT:
                ret_val = bacapp_snprintf_unsigned_integer(
                    str, str_len, value->type.Unsigned_Int);
                break;
#endif
#if defined(BACAPP_SIGNED)
            case BACNET_APPLICATION_TAG_SIGNED_INT:
                ret_val = bacapp_snprintf_signed_integer(
                    str, str_len, value->type.Signed_Int);
                break;
#endif
#if defined(BACAPP_REAL)
            case BACNET_APPLICATION_TAG_REAL:
                ret_val = bacapp_snprintf_real(str, str_len, value->type.Real);
                break;
#endif
#if defined(BACAPP_DOUBLE)
            case BACNET_APPLICATION_TAG_DOUBLE:
                ret_val =
                    bacapp_snprintf_double(str, str_len, value->type.Double);
                break;
#endif
#if defined(BACAPP_OCTET_STRING)
            case BACNET_APPLICATION_TAG_OCTET_STRING:
                ret_val = bacapp_snprintf_octet_string(
                    str, str_len, &value->type.Octet_String);
                break;
#endif
#if defined(BACAPP_CHARACTER_STRING)
            case BACNET_APPLICATION_TAG_CHARACTER_STRING:
                ret_val = bacapp_snprintf_character_string(
                    str, str_len, &value->type.Character_String);
                break;
#endif
#if defined(BACAPP_BIT_STRING)
            case BACNET_APPLICATION_TAG_BIT_STRING:
                ret_val = bacapp_snprintf_bit_string(
                    str, str_len, &value->type.Bit_String);
                break;
#endif
#if defined(BACAPP_ENUMERATED)
            case BACNET_APPLICATION_TAG_ENUMERATED:
                ret_val = bacapp_snprintf_enumerated(
                    str, str_len, object_type, property,
                    value->type.Enumerated);
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
                ret_val = bacapp_snprintf_object_id(
                    str, str_len, &value->type.Object_Id);
                break;
#endif
#if defined(BACAPP_TIMESTAMP)
            case BACNET_APPLICATION_TAG_TIMESTAMP:
                ret_val = bacapp_timestamp_to_ascii(
                    str, str_len, &value->type.Time_Stamp);
                break;
#endif
#if defined(BACAPP_DATETIME)
            case BACNET_APPLICATION_TAG_DATETIME:
                ret_val = bacapp_snprintf_datetime(
                    str, str_len, &value->type.Date_Time);
                break;
#endif
#if defined(BACAPP_DATERANGE)
            case BACNET_APPLICATION_TAG_DATERANGE:
                ret_val = bacapp_snprintf_daterange(
                    str, str_len, &value->type.Date_Range);
                break;
#endif
#if defined(BACAPP_LIGHTING_COMMAND)
            case BACNET_APPLICATION_TAG_LIGHTING_COMMAND:
                ret_val = lighting_command_to_ascii(
                    &value->type.Lighting_Command, str, str_len);
                break;
#endif
#if defined(BACAPP_XY_COLOR)
            case BACNET_APPLICATION_TAG_XY_COLOR:
                /* BACnetxyColor */
                ret_val =
                    xy_color_to_ascii(&value->type.XY_Color, str, str_len);
                break;
#endif
#if defined(BACAPP_COLOR_COMMAND)
            case BACNET_APPLICATION_TAG_COLOR_COMMAND:
                /* BACnetColorCommand */
                ret_val = bacapp_snprintf_color_command(
                    str, str_len, &value->type.Color_Command);
                break;
#endif
#if defined(BACAPP_WEEKLY_SCHEDULE)
            case BACNET_APPLICATION_TAG_WEEKLY_SCHEDULE:
                /* BACnetWeeklySchedule */
                ret_val = bacapp_snprintf_weeklyschedule(
                    str, str_len, &value->type.Weekly_Schedule,
                    object_value->array_index);
                break;
#endif
#if defined(BACAPP_HOST_N_PORT)
            case BACNET_APPLICATION_TAG_HOST_N_PORT:
                ret_val = bacapp_snprintf_host_n_port(
                    str, str_len, &value->type.Host_Address);
                break;
#endif
#if defined(BACAPP_DEVICE_OBJECT_PROPERTY_REFERENCE)
            case BACNET_APPLICATION_TAG_DEVICE_OBJECT_PROPERTY_REFERENCE:
                ret_val = bacapp_snprintf_device_object_property_reference(
                    str, str_len,
                    &value->type.Device_Object_Property_Reference);
                break;
#endif
#if defined(BACAPP_DEVICE_OBJECT_REFERENCE)
            case BACNET_APPLICATION_TAG_DEVICE_OBJECT_REFERENCE:
                ret_val = bacapp_snprintf_device_object_reference(
                    str, str_len, &value->type.Device_Object_Reference);
                break;
#endif
#if defined(BACAPP_OBJECT_PROPERTY_REFERENCE)
            case BACNET_APPLICATION_TAG_OBJECT_PROPERTY_REFERENCE:
                ret_val = bacapp_snprintf_object_property_reference(
                    str, str_len, &value->type.Object_Property_Reference);
                break;
#endif
#if defined(BACAPP_SECURE_CONNECT)
            case BACNET_APPLICATION_TAG_SC_FAILED_CONNECTION_REQUEST:
                ret_val = bacapp_snprintf_SCFailedConnectionRequest(
                    str, str_len, &value->type.SC_Failed_Req);
                break;

            case BACNET_APPLICATION_TAG_SC_HUB_FUNCTION_CONNECTION_STATUS:
                ret_val = bacapp_snprintf_SCHubFunctionConnection(
                    str, str_len, &value->type.SC_Hub_Function_Status);
                break;

            case BACNET_APPLICATION_TAG_SC_DIRECT_CONNECTION_STATUS:
                ret_val = bacapp_snprintf_SCDirectConnection(
                    str, str_len, &value->type.SC_Direct_Status);
                break;

            case BACNET_APPLICATION_TAG_SC_HUB_CONNECTION_STATUS:
                ret_val = bacapp_snprintf_SCHubConnection(
                    str, str_len, &value->type.SC_Hub_Status);
                break;

#endif
#if defined(BACAPP_DESTINATION)
            case BACNET_APPLICATION_TAG_DESTINATION:
                ret_val = bacnet_destination_to_ascii(
                    &value->type.Destination, str, str_len);
                break;
#endif
#if defined(BACAPP_CALENDAR_ENTRY)
            case BACNET_APPLICATION_TAG_CALENDAR_ENTRY:
                ret_val = bacapp_snprintf_calendar_entry(
                    str, str_len, &value->type.Calendar_Entry);
                break;
#endif
#if defined(BACAPP_SPECIAL_EVENT)
            case BACNET_APPLICATION_TAG_SPECIAL_EVENT:
                ret_val = bacapp_snprintf_special_event(
                    str, str_len, &value->type.Special_Event);
                break;
#endif
#if defined(BACAPP_BDT_ENTRY)
            case BACNET_APPLICATION_TAG_BDT_ENTRY:
                slen = bacapp_snprintf(str, str_len, "{");
                ret_val += bacapp_snprintf_shift(slen, &str, &str_len);
                slen = bacnet_bdt_entry_to_ascii(
                    str, str_len, &value->type.BDT_Entry);
                ret_val += bacapp_snprintf_shift(slen, &str, &str_len);
                slen = bacapp_snprintf(str, str_len, "}");
                ret_val += bacapp_snprintf_shift(slen, &str, &str_len);
                break;
#endif
#if defined(BACAPP_FDT_ENTRY)
            case BACNET_APPLICATION_TAG_FDT_ENTRY:
                slen = bacapp_snprintf(str, str_len, "{");
                ret_val += bacapp_snprintf_shift(slen, &str, &str_len);
                slen = bacnet_fdt_entry_to_ascii(
                    str, str_len, &value->type.FDT_Entry);
                ret_val += bacapp_snprintf_shift(slen, &str, &str_len);
                slen = bacapp_snprintf(str, str_len, "}");
                ret_val += bacapp_snprintf_shift(slen, &str, &str_len);
                break;
#endif
#if defined(BACAPP_ACTION_COMMAND)
            case BACNET_APPLICATION_TAG_ACTION_COMMAND:
                ret_val = bacapp_snprintf_action_command(
                    str, str_len, &value->type.Action_Command);
                break;
#endif
#if defined(BACAPP_SCALE)
            case BACNET_APPLICATION_TAG_SCALE:
                if (value->type.Scale.float_scale) {
                    ret_val = bacapp_snprintf(
                        str, str_len, "%f",
                        (double)value->type.Scale.type.real_scale);
                } else {
                    ret_val = bacapp_snprintf(
                        str, str_len, "%ld",
                        (long)value->type.Scale.type.integer_scale);
                }
                break;
#endif
#if defined(BACAPP_SHED_LEVEL)
            case BACNET_APPLICATION_TAG_SHED_LEVEL:
                slen = bacapp_snprintf(str, str_len, "{");
                ret_val += bacapp_snprintf_shift(slen, &str, &str_len);
                ret_val = bacapp_snprintf_shed_level(
                    str, str_len, &value->type.Shed_Level);
                ret_val += bacapp_snprintf_shift(slen, &str, &str_len);
                slen = bacapp_snprintf(str, str_len, "}");
                ret_val += bacapp_snprintf_shift(slen, &str, &str_len);
                break;
#endif
#if defined(BACAPP_ACCESS_RULE)
            case BACNET_APPLICATION_TAG_ACCESS_RULE:
                ret_val = bacapp_snprintf_access_rule(
                    str, str_len, &value->type.Access_Rule);
                break;
#endif
#if defined(BACAPP_CHANNEL_VALUE)
            case BACNET_APPLICATION_TAG_CHANNEL_VALUE:
                ret_val = bacapp_snprintf_channel_value(
                    str, str_len, &value->type.Channel_Value);
                break;
#endif
#if defined(BACAPP_TIMER_VALUE)
            case BACNET_APPLICATION_TAG_TIMER_VALUE:
                ret_val = bacnet_timer_value_to_ascii(
                    &value->type.Timer_Value, str, str_len);
                break;
#endif
#if defined(BACAPP_RECIPIENT)
            case BACNET_APPLICATION_TAG_RECIPIENT:
                ret_val = bacnet_recipient_to_ascii(
                    &value->type.Recipient, str, str_len);
                break;
#endif
#if defined(BACAPP_ADDRESS_BINDING)
            case BACNET_APPLICATION_TAG_ADDRESS_BINDING:
                ret_val = bacnet_address_binding_to_ascii(
                    &value->type.Address_Binding, str, str_len);
                break;
#endif
#if defined(BACAPP_NO_VALUE)
            case BACNET_APPLICATION_TAG_NO_VALUE:
                ret_val = bacnet_timer_value_no_value_to_ascii(str, str_len);
                break;
#endif
#if defined(BACAPP_LOG_RECORD)
            case BACNET_APPLICATION_TAG_LOG_RECORD:
                ret_val = bacapp_snprintf_log_record(
                    str, str_len, &value->type.Log_Record);
                break;
#endif
            case BACNET_APPLICATION_TAG_EMPTYLIST:
                ret_val = bacapp_snprintf(str, str_len, "{}");
                break;
            default:
                ret_val = bacapp_snprintf(
                    str, str_len, "UnknownType(tag=%d)", value->tag);
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
    FILE *stream, const BACNET_OBJECT_PROPERTY_VALUE *object_value)
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
        str = calloc(str_len + 1, sizeof(char));
        if (!str) {
            return false;
        }
#endif
        bacapp_snprintf_value(str, str_len + 1, object_value);
        if (stream) {
            if (object_value->object_type == OBJECT_SCHEDULE) {
                switch (object_value->object_property) {
                    case PROP_PRESENT_VALUE:
                    case PROP_SCHEDULE_DEFAULT:
                        fprintf(
                            stream, "[%u] %s", object_value->value->tag, str);
                        break;
                    default:
                        fprintf(stream, "%s", str);
                        break;
                }
            } else {
                fprintf(stream, "%s", str);
            }
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
#else
bool bacapp_print_value(
    FILE *stream, const BACNET_OBJECT_PROPERTY_VALUE *object_value)
{
    (void)stream;
    (void)object_value;
    return false;
}
#endif

#ifdef BACAPP_PRINT_ENABLED
#if defined(BACAPP_WEEKLY_SCHEDULE)
static bool
parse_weeklyschedule(char *str, BACNET_APPLICATION_DATA_VALUE *value)
{
    char *chunk, *comma, *space, *t, *v, *colonpos, *sqpos;
    int daynum = 0, tvnum = 0;
    unsigned int inner_tag;
    BACNET_APPLICATION_DATA_VALUE dummy_value = { 0 };
    BACNET_DAILY_SCHEDULE *dsch;

    /*
     Format:

     (1; Mon: [02:00:00.00 FALSE, 07:35:00.00 active, 07:40:00.00 inactive];
     Tue: [02:00:00.00 inactive, 06:00:00.00 null]; ...)

     - the first number is the inner tag (e.g. 1 = boolean, 4 = real, 9 = enum)
     - Day name prefix is optional and ignored.
     - Entries are separated by semicolons.
     - There can be a full week, or only one entry - when using array index to
     modify a single day
     - time-value array can be empty: []
     - null value overrides the tag for that particular BACNET_TIME_VALUE
    */

    value->tag = BACNET_APPLICATION_TAG_WEEKLY_SCHEDULE;

    /* Parse the inner tag */
    chunk = strtok(str, ";");
    chunk = bacnet_ltrim(chunk, "(");
    if (false ==
        bacapp_parse_application_data(
            BACNET_APPLICATION_TAG_UNSIGNED_INT, chunk, &dummy_value)) {
        /* Try searching it by name */
        if (false == bactext_application_tag_index(chunk, &inner_tag)) {
            return false;
        }
    } else {
        inner_tag = (int)dummy_value.type.Unsigned_Int;
    }

    chunk = strtok(NULL, ";");

    while (chunk != NULL) {
        dsch = &value->type.Weekly_Schedule.weeklySchedule[daynum];

        /* Strip day name prefix, if present */
        colonpos = strchr(chunk, ':');
        sqpos = strchr(chunk, '[');
        if (colonpos && colonpos < sqpos) {
            chunk = colonpos + 1;
        }

        /* Extract the inner list of time-values */
        chunk = bacnet_rtrim(bacnet_ltrim(chunk, "([ "), " ])");

        /* The list can be empty */
        if (chunk[0] != 0) {
            /* loop through the time value pairs */
            tvnum = 0;
            do {
                /* Find the comma delimiter, replace with NUL (like strtok) */
                comma = strchr(chunk, ',');
                if (comma) {
                    *comma = 0;
                }
                /* trim the time-value pair and find the delimiter space */
                chunk = bacnet_trim(chunk, " ");
                space = strchr(chunk, ' ');
                if (!space) {
                    /* malformed time-value pair */
                    return false;
                }
                *space = 0;

                /* Extract time and value */
                t = chunk;
                /* value starts one byte after the space, and there can be */
                /* multiple spaces */
                chunk = bacnet_ltrim(space + 1, " ");
                v = chunk;

                /* Parse time */
                if (false ==
                    bacapp_parse_application_data(
                        BACNET_APPLICATION_TAG_TIME, t, &dummy_value)) {
                    return false;
                }
                dsch->Time_Values[tvnum].Time = dummy_value.type.Time;

                /* Parse value */
                if (false ==
                    bacapp_parse_application_data(inner_tag, v, &dummy_value)) {
                    /* Schedules can be set to active, inactive, or null to
                     * revert to default value */
                    if (bacnet_stricmp(v, "null") == 0) {
                        dummy_value.tag = BACNET_APPLICATION_TAG_NULL;
                    } else {
                        return false;
                    }
                }
                if (BACNET_STATUS_OK !=
                    bacnet_application_to_primitive_data_value(
                        &dsch->Time_Values[tvnum].Value, &dummy_value)) {
                    return false;
                }

                /* Advance past the comma to the next chunk */
                if (comma) {
                    chunk = comma + 1;
                }
                tvnum++;
            } while (comma != NULL);
        }

        dsch->TV_Count = tvnum;

        /* Find the start of the next day */
        chunk = strtok(NULL, ";");
        daynum++;
    }

    if (daynum == 1) {
        value->type.Weekly_Schedule.singleDay = true;
    }

    return true;
}
#endif

#if defined(BACAPP_SCALE)
/**
 * @brief Parse a string into a BACnetScale value
 * @param value [out] The BACnetScale value
 * @param argv [in] The string to parse
 * @return True on success, else False
 */
static bool bacnet_scale_from_ascii(BACNET_SCALE *value, const char *argv)
{
    bool status = false;
    int count;
    unsigned integer_scale;
    float float_scale;
    const char *decimal_point;

    if (!status) {
        decimal_point = strchr(argv, '.');
        if (decimal_point) {
            count = sscanf(argv, "%f", &float_scale);
            if (count == 1) {
                value->float_scale = true;
                value->type.real_scale = float_scale;
                status = true;
            }
        }
    }
    if (!status) {
        count = sscanf(argv, "%u", &integer_scale);
        if (count == 1) {
            value->float_scale = false;
            value->type.integer_scale = integer_scale;
            status = true;
        }
    }

    return status;
}
#endif

#if defined(BACAPP_DEVICE_OBJECT_PROPERTY_REFERENCE)
/**
 * @brief Parse a string into a BACnetDeviceObjectPropertyReference value
 * @param value [out] The BACnetDeviceObjectPropertyReference value
 * @param argv [in] The string to parse
 * @return true on success, else false
 */
static bool device_object_property_reference_from_ascii(
    BACNET_DEVICE_OBJECT_PROPERTY_REFERENCE *value, const char *argv)
{
    bool status = false;
    unsigned long object_type, object_instance;
    unsigned long property_id;
    long array_index;
    unsigned long device_type, device_instance;
    int count;

    if (!value || !argv) {
        return false;
    }
    /* analog-output:4194303,present-value,-1,device:4194303 */
    count = sscanf(
        argv, "%4lu:%7lu,%lu,%ld,%4lu:%7lu", &object_type, &object_instance,
        &property_id, &array_index, &device_type, &device_instance);
    if (count == 6) {
        value->objectIdentifier.type = object_type;
        value->objectIdentifier.instance = object_instance;
        value->propertyIdentifier = property_id;
        if (array_index < 0L) {
            value->arrayIndex = BACNET_ARRAY_ALL;
        } else {
            value->arrayIndex = array_index;
        }
        value->arrayIndex = array_index;
        value->deviceIdentifier.type = device_type;
        value->deviceIdentifier.instance = device_instance;
        status = true;
    }

    return status;
}
#endif
#if defined(BACAPP_DEVICE_OBJECT_REFERENCE)
/**
 * @brief Parse a string into a BACnetDeviceObjectReference value
 * @param value [out] The BACnetDeviceObjectReference value
 * @param argv [in] The string to parse
 * @return true on success, else false
 */
static bool device_object_reference_from_ascii(
    BACNET_DEVICE_OBJECT_REFERENCE *value, const char *argv)
{
    bool status = false;
    unsigned long object_type, object_instance;
    unsigned long device_type, device_instance;
    int count;

    if (!value || !argv) {
        return false;
    }
    /* analog-output:4194303,device:4194303 */
    count = sscanf(
        argv, "%4lu:%7lu,%4lu:%7lu", &object_type, &object_instance,
        &device_type, &device_instance);
    if (count == 4) {
        value->objectIdentifier.type = object_type;
        value->objectIdentifier.instance = object_instance;
        value->deviceIdentifier.type = device_type;
        value->deviceIdentifier.instance = device_instance;
        status = true;
    }

    return status;
}
#endif

#if defined(BACAPP_OBJECT_PROPERTY_REFERENCE)
/**
 * @brief Parse a string into a BACnetObjectPropertyReference value
 * @param value [out] The BACnetObjectPropertyReference value
 * @param argv [in] The string to parse
 * @return true on success, else false
 */
static bool object_property_reference_from_ascii(
    BACNET_OBJECT_PROPERTY_REFERENCE *value, const char *argv)
{
    bool status = false;
    unsigned long object_type, object_instance;
    unsigned long property_id;
    long array_index;
    int count;

    if (!value || !argv) {
        return false;
    }
    /* analog-output:4194303,present-value,-1 */
    count = sscanf(
        argv, "%4lu:%7lu,%lu,%ld", &object_type, &object_instance, &property_id,
        &array_index);
    if (count == 4) {
        value->object_identifier.type = object_type;
        value->object_identifier.instance = object_instance;
        value->property_identifier = property_id;
        if ((array_index >= BACNET_ARRAY_ALL) || (array_index < 0L)) {
            array_index = BACNET_ARRAY_ALL;
        }
        value->property_array_index = array_index;
        status = true;
    }

    return status;
}
#endif

/* used to load the app data struct with the proper data
   converted from a command line argument.
   "argv" is not const to allow using strtok internally. It MAY be modified. */
bool bacapp_parse_application_data(
    BACNET_APPLICATION_TAG tag_number,
    char *argv,
    BACNET_APPLICATION_DATA_VALUE *value)
{
#if defined(BACAPP_TIME)
    int hour, min, sec, hundredths;
#endif
#if defined(BACAPP_DATE)
    int year, month, day, wday;
#endif
    int object_type = 0;
    uint32_t instance = 0;
    bool status = false;
    long long_value = 0;
    BACNET_UNSIGNED_INTEGER unsigned_long_value = 0;
    double double_value = 0.0;
    int count = 0;

    if (value && (tag_number != MAX_BACNET_APPLICATION_TAG)) {
        status = true;
        value->tag = tag_number;
        switch (tag_number) {
#if defined(BACAPP_BOOLEAN)
            case BACNET_APPLICATION_TAG_BOOLEAN:
                if (bacnet_stricmp(argv, "true") == 0 ||
                    bacnet_stricmp(argv, "active") == 0) {
                    value->type.Boolean = true;
                } else if (
                    bacnet_stricmp(argv, "false") == 0 ||
                    bacnet_stricmp(argv, "inactive") == 0) {
                    value->type.Boolean = false;
                } else {
                    status = bacnet_strtol(argv, &long_value);
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
                status = bacnet_string_to_unsigned(argv, &unsigned_long_value);
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
                status = bacnet_strtol(argv, &long_value);
                if (!status || long_value > INT32_MAX ||
                    long_value < INT32_MIN) {
                    return false;
                }
                value->type.Signed_Int = (int32_t)long_value;
                break;
#endif
#if defined(BACAPP_REAL)
            case BACNET_APPLICATION_TAG_REAL:
                status = bacnet_strtod(argv, &double_value);
                if (!status) {
                    return false;
                }
                value->type.Real = (float)double_value;
                break;
#endif
#if defined(BACAPP_DOUBLE)
            case BACNET_APPLICATION_TAG_DOUBLE:
                status = bacnet_strtod(argv, &double_value);
                if (!status) {
                    return false;
                }
                value->type.Double = double_value;
                break;
#endif
#if defined(BACAPP_OCTET_STRING)
            case BACNET_APPLICATION_TAG_OCTET_STRING:
                if (!octetstring_init_ascii_epics(
                        &value->type.Octet_String, argv)) {
                    status = octetstring_init_ascii_hex(
                        &value->type.Octet_String, argv);
                }
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
                status = bacnet_string_to_unsigned(argv, &unsigned_long_value);
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
                    datetime_set_date(
                        &value->type.Date, (uint16_t)year, (uint8_t)month,
                        (uint8_t)day);
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
#if defined(BACAPP_TIMESTAMP)
            case BACNET_APPLICATION_TAG_TIMESTAMP:
                /* BACnetTimeStamp */
                status =
                    bacapp_timestamp_init_ascii(&value->type.Time_Stamp, argv);
                break;
#endif
#if defined(BACAPP_DATETIME)
            case BACNET_APPLICATION_TAG_DATETIME:
                /* BACnetDateTime */
                status = datetime_init_ascii(&value->type.Date_Time, argv);
                break;
#endif
#if defined(BACAPP_LIGHTING_COMMAND)
            case BACNET_APPLICATION_TAG_LIGHTING_COMMAND:
                /* BACnetLightingCommand */
                status = lighting_command_from_ascii(
                    &value->type.Lighting_Command, argv);
                break;
#endif
#if defined(BACAPP_XY_COLOR)
            case BACNET_APPLICATION_TAG_XY_COLOR:
                /* BACnetxyColor */
                status = xy_color_from_ascii(&value->type.XY_Color, argv);
                break;
#endif
#if defined(BACAPP_COLOR_COMMAND)
            case BACNET_APPLICATION_TAG_COLOR_COMMAND:
                /* FIXME: add parsing for BACnetColorCommand */
                break;
#endif
#if defined(BACAPP_WEEKLY_SCHEDULE)
            case BACNET_APPLICATION_TAG_WEEKLY_SCHEDULE:
                status = parse_weeklyschedule(argv, value);
                break;
#endif
#if defined(BACAPP_SPECIAL_EVENT)
            case BACNET_APPLICATION_TAG_SPECIAL_EVENT:
                /* FIXME: add parsing for BACnetSpecialEvent */
                break;
#endif
#if defined(BACAPP_CALENDAR_ENTRY)
            case BACNET_APPLICATION_TAG_CALENDAR_ENTRY:
                /* FIXME: add parsing for BACnetCalendarEntry */
                break;
#endif
#if defined(BACAPP_HOST_N_PORT)
            case BACNET_APPLICATION_TAG_HOST_N_PORT:
                status =
                    host_n_port_from_ascii(&value->type.Host_Address, argv);
                break;
#endif
#if defined(BACAPP_DEVICE_OBJECT_PROPERTY_REFERENCE)
            case BACNET_APPLICATION_TAG_DEVICE_OBJECT_PROPERTY_REFERENCE:
                status = device_object_property_reference_from_ascii(
                    &value->type.Device_Object_Property_Reference, argv);
                break;
#endif
#if defined(BACAPP_DEVICE_OBJECT_REFERENCE)
            case BACNET_APPLICATION_TAG_DEVICE_OBJECT_REFERENCE:
                status = device_object_reference_from_ascii(
                    &value->type.Device_Object_Reference, argv);
                break;
#endif
#if defined(BACAPP_OBJECT_PROPERTY_REFERENCE)
            case BACNET_APPLICATION_TAG_OBJECT_PROPERTY_REFERENCE:
                status = object_property_reference_from_ascii(
                    &value->type.Object_Property_Reference, argv);
                break;
#endif
#if defined(BACAPP_DESTINATION)
            case BACNET_APPLICATION_TAG_DESTINATION:
                status = bacnet_destination_from_ascii(
                    &value->type.Destination, argv);
                break;
#endif
#if defined(BACAPP_BDT_ENTRY)
            case BACNET_APPLICATION_TAG_BDT_ENTRY:
                status =
                    bacnet_bdt_entry_from_ascii(&value->type.BDT_Entry, argv);
                break;
#endif
#if defined(BACAPP_FDT_ENTRY)
            case BACNET_APPLICATION_TAG_FDT_ENTRY:
                status =
                    bacnet_fdt_entry_from_ascii(&value->type.FDT_Entry, argv);
                break;
#endif
#if defined(BACAPP_ACTION_COMMAND)
            case BACNET_APPLICATION_TAG_ACTION_COMMAND:
                /* BACnetActionCommand - not implemented */
                break;
#endif
#if defined(BACAPP_SCALE)
            case BACNET_APPLICATION_TAG_SCALE:
                status = bacnet_scale_from_ascii(&value->type.Scale, argv);
                break;
#endif
#if defined(BACAPP_SHED_LEVEL)
            case BACNET_APPLICATION_TAG_SHED_LEVEL:
                status =
                    bacnet_shed_level_from_ascii(&value->type.Shed_Level, argv);
                break;
#endif
#if defined(BACAPP_ACCESS_RULE)
            case BACNET_APPLICATION_TAG_ACCESS_RULE:
                status = bacnet_access_rule_from_ascii(
                    &value->type.Access_Rule, argv);
                break;
#endif
#if defined(BACAPP_CHANNEL_VALUE)
            case BACNET_APPLICATION_TAG_CHANNEL_VALUE:
                status = bacnet_channel_value_from_ascii(
                    &value->type.Channel_Value, argv);
                break;
#endif
#if defined(BACAPP_TIMER_VALUE)
            case BACNET_APPLICATION_TAG_TIMER_VALUE:
                status = bacnet_timer_value_from_ascii(
                    &value->type.Timer_Value, argv);
                break;
#endif
#if defined(BACAPP_RECIPIENT)
            case BACNET_APPLICATION_TAG_RECIPIENT:
                status =
                    bacnet_recipient_from_ascii(&value->type.Recipient, argv);
                break;
#endif
#if defined(BACAPP_ADDRESS_BINDING)
            case BACNET_APPLICATION_TAG_ADDRESS_BINDING:
                status = bacnet_address_binding_from_ascii(
                    &value->type.Address_Binding, argv);
                break;
#endif
#if defined(BACAPP_NO_VALUE)
            case BACNET_APPLICATION_TAG_NO_VALUE:
                status =
                    bacnet_timer_value_no_value_from_ascii(&value->tag, argv);
                break;
#endif
#if defined(BACAPP_LOG_RECORD)
            case BACNET_APPLICATION_TAG_LOG_RECORD:
                status = bacnet_log_record_datum_from_ascii(
                    &value->type.Log_Record, argv);
                break;
#endif
            default:
                break;
        }
        value->next = NULL;
    }

    return status;
}
#else
bool bacapp_parse_application_data(
    BACNET_APPLICATION_TAG tag_number,
    char *argv,
    BACNET_APPLICATION_DATA_VALUE *value)
{
    (void)tag_number;
    (void)argv;
    (void)value;
    return false;
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
            value->context_specific = false;
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

/**
 * @brief Link an array of BACNET_PROPERTY_VALUE elements.
 * The linked-list is used prior to encoding or decoding
 * the APDU data into the structure.
 *
 * @param value_list - Pointer to the first BACNET_PROPERTY_VALUE element in
 * an array
 * @param count - number of BACNET_PROPERTY_VALUE elements to link
 */
void bacapp_property_value_list_link(
    BACNET_PROPERTY_VALUE *value_list, size_t count)
{
    BACNET_PROPERTY_VALUE *current_value_list = NULL;

    if (value_list) {
        while (count) {
            if (count > 1) {
                current_value_list = value_list;
                value_list++;
                current_value_list->next = value_list;
            } else {
                value_list->next = NULL;
            }
            count--;
        }
    }
}

/**
 * @brief Encode one BACnetPropertyValue value
 *
 *  BACnetPropertyValue ::= SEQUENCE {
 *      property-identifier [0] BACnetPropertyIdentifier,
 *      property-array-index [1] Unsigned OPTIONAL,
 *      -- used only with array datatypes
 *      -- if omitted with an array the entire array is referenced
 *      property-value [2] ABSTRACT-SYNTAX.&Type,
 *      -- any datatype appropriate for the specified property
 *      priority [3] Unsigned (1..16) OPTIONAL
 *      -- used only when property is commandable
 *  }
 *
 * @param apdu Pointer to the buffer for encoded values, or NULL for length
 * @param value Pointer to the service data used for encoding values
 *
 * @return Bytes encoded or zero on error.
 */
int bacapp_property_value_encode(
    uint8_t *apdu, const BACNET_PROPERTY_VALUE *value)
{
    int len = 0; /* length of each encoding */
    int apdu_len = 0; /* total length of the apdu, return value */
    const BACNET_APPLICATION_DATA_VALUE *app_data = NULL;

    if (value) {
        /* tag 0 - propertyIdentifier */
        len = encode_context_enumerated(apdu, 0, value->propertyIdentifier);
        apdu_len += len;
        if (apdu) {
            apdu += len;
        }
        /* tag 1 - propertyArrayIndex OPTIONAL */
        if (value->propertyArrayIndex != BACNET_ARRAY_ALL) {
            len = encode_context_unsigned(apdu, 1, value->propertyArrayIndex);
            apdu_len += len;
            if (apdu) {
                apdu += len;
            }
        }
        /* tag 2 - value */
        /* abstract syntax gets enclosed in a context tag */
        len = encode_opening_tag(apdu, 2);
        apdu_len += len;
        if (apdu) {
            apdu += len;
        }
        app_data = &value->value;
        while (app_data != NULL) {
            len = bacapp_encode_application_data(apdu, app_data);
            apdu_len += len;
            if (apdu) {
                apdu += len;
            }
            app_data = app_data->next;
        }
        len = encode_closing_tag(apdu, 2);
        apdu_len += len;
        if (apdu) {
            apdu += len;
        }
        /* tag 3 - priority OPTIONAL */
        if (value->priority != BACNET_NO_PRIORITY) {
            len = encode_context_unsigned(apdu, 3, value->priority);
            apdu_len += len;
        }
    }

    return apdu_len;
}

/**
 * @brief Encode one BACnetPropertyValue value within context tags
 * @param apdu Pointer to the buffer for encoded values, or NULL for length
 * @param tag_number
 * @param value Pointer to the service data used for encoding values
 * @return Bytes encoded or zero on error.
 */
int bacapp_property_value_context_encode(
    uint8_t *apdu, uint8_t tag_number, const BACNET_PROPERTY_VALUE *value)
{
    int len = 0; /* length of each encoding */
    int apdu_len = 0; /* total length of the apdu, return value */

    len = encode_opening_tag(apdu, tag_number);
    apdu_len += len;
    if (apdu) {
        apdu += len;
    }
    len = bacapp_property_value_encode(apdu, value);
    apdu_len += len;
    if (apdu) {
        apdu += len;
    }
    len = encode_closing_tag(apdu, tag_number);
    apdu_len += len;

    return apdu_len;
}

/**
 * @brief Decode one BACnetPropertyValue value
 *
 *  BACnetPropertyValue ::= SEQUENCE {
 *      property-identifier [0] BACnetPropertyIdentifier,
 *      property-array-index [1] Unsigned OPTIONAL,
 *      -- used only with array datatypes
 *      -- if omitted with an array the entire array is referenced
 *      property-value [2] ABSTRACT-SYNTAX.&Type,
 *      -- any datatype appropriate for the specified property
 *      priority [3] Unsigned (1..16) OPTIONAL
 *      -- used only when property is commandable
 *  }
 *
 * @param apdu Pointer to the buffer of encoded value
 * @param apdu_size Size of the buffer holding the encode value
 * @param value Pointer to the service data used for encoding values
 *
 * @return Bytes decoded or BACNET_STATUS_ERROR on error.
 */
int bacapp_property_value_decode(
    const uint8_t *apdu, uint32_t apdu_size, BACNET_PROPERTY_VALUE *value)
{
    int len = 0;
    int apdu_len = 0;
    int tag_len = 0;
    uint32_t enumerated_value = 0;
    uint32_t len_value_type = 0;
    BACNET_UNSIGNED_INTEGER unsigned_value = 0;
    BACNET_PROPERTY_ID property_identifier = PROP_ALL;
    BACNET_APPLICATION_DATA_VALUE *app_data = NULL;

    /* property-identifier [0] BACnetPropertyIdentifier */
    len = bacnet_enumerated_context_decode(
        &apdu[apdu_len], apdu_size - apdu_len, 0, &enumerated_value);
    if (len > 0) {
        property_identifier = enumerated_value;
        if (value) {
            value->propertyIdentifier = property_identifier;
        }
        apdu_len += len;
    } else {
        return BACNET_STATUS_ERROR;
    }
    /* property-array-index [1] Unsigned OPTIONAL */
    if (bacnet_is_context_tag_number(
            &apdu[apdu_len], apdu_size - apdu_len, 1, &len, &len_value_type)) {
        apdu_len += len;
        len = bacnet_unsigned_decode(
            &apdu[apdu_len], apdu_size - apdu_len, len_value_type,
            &unsigned_value);
        if (len > 0) {
            if (unsigned_value > UINT32_MAX) {
                return BACNET_STATUS_ERROR;
            } else {
                apdu_len += len;
                if (value) {
                    value->propertyArrayIndex = unsigned_value;
                }
            }
        } else {
            return BACNET_STATUS_ERROR;
        }
    } else {
        if (value) {
            value->propertyArrayIndex = BACNET_ARRAY_ALL;
        }
    }
    /* property-value [2] ABSTRACT-SYNTAX.&Type */
    if (bacnet_is_opening_tag_number(
            &apdu[apdu_len], apdu_size - apdu_len, 2, &len)) {
        if (value) {
            apdu_len += len;
            app_data = &value->value;
            while (app_data != NULL) {
                len = bacapp_decode_application_data(
                    &apdu[apdu_len], apdu_size - apdu_len, app_data);
                if (len < 0) {
                    return BACNET_STATUS_ERROR;
                }
                apdu_len += len;
                if (bacnet_is_closing_tag_number(
                        &apdu[apdu_len], apdu_size - apdu_len, 2, &len)) {
                    break;
                }
                app_data = app_data->next;
            }
        } else {
            /* this len function needs to start at the opening tag
               to match opening/closing tags like a stack.
               However, it returns the len between the tags.
               Therefore, store the length of the opening tag first */
            tag_len = len;
            len = bacnet_enclosed_data_length(
                &apdu[apdu_len], apdu_size - apdu_len);
            apdu_len += len;
            /* add the opening tag length to the totals */
            apdu_len += tag_len;
        }
        if (bacnet_is_closing_tag_number(
                &apdu[apdu_len], apdu_size - apdu_len, 2, &len)) {
            apdu_len += len;
        } else {
            return BACNET_STATUS_ERROR;
        }
    } else {
        return BACNET_STATUS_ERROR;
    }
    /* priority [3] Unsigned (1..16) OPTIONAL */
    if (bacnet_is_context_tag_number(
            &apdu[apdu_len], apdu_size - apdu_len, 3, &len, &len_value_type)) {
        apdu_len += len;
        len = bacnet_unsigned_decode(
            &apdu[apdu_len], apdu_size - apdu_len, len_value_type,
            &unsigned_value);
        if (len > 0) {
            if (unsigned_value > UINT8_MAX) {
                return BACNET_STATUS_ERROR;
            } else {
                apdu_len += len;
                if (value) {
                    value->priority = unsigned_value;
                }
            }
        } else {
            return BACNET_STATUS_ERROR;
        }
    } else {
        if (value) {
            value->priority = BACNET_NO_PRIORITY;
        }
    }

    return apdu_len;
}

/**
 * @brief Decode one BACnetPropertyValue value for a specific object type
 *
 *  BACnetPropertyValue ::= SEQUENCE {
 *      property-identifier [0] BACnetPropertyIdentifier,
 *      property-array-index [1] Unsigned OPTIONAL,
 *      -- used only with array datatypes
 *      -- if omitted with an array the entire array is referenced
 *      property-value [2] ABSTRACT-SYNTAX.&Type,
 *      -- any datatype appropriate for the specified property
 *      priority [3] Unsigned (1..16) OPTIONAL
 *      -- used only when property is commandable
 *  }
 *
 * @param apdu Pointer to the buffer of encoded value
 * @param apdu_size Size of the buffer holding the encode value
 * @param value Pointer to the service data used for encoding values
 *
 * @return Bytes decoded or BACNET_STATUS_ERROR on error.
 */
int bacapp_object_property_value_decode(
    const uint8_t *apdu,
    uint32_t apdu_size,
    BACNET_PROPERTY_VALUE *value,
    BACNET_OBJECT_TYPE object_type)
{
    int len = 0;
    int apdu_len = 0;
    int tag_len = 0;
    uint32_t enumerated_value = 0;
    uint32_t len_value_type = 0;
    BACNET_UNSIGNED_INTEGER unsigned_value = 0;
    BACNET_PROPERTY_ID property_identifier = PROP_ALL;
    uint32_t array_index = BACNET_ARRAY_ALL;
    BACNET_APPLICATION_DATA_VALUE *app_data = NULL;

    /* property-identifier [0] BACnetPropertyIdentifier */
    len = bacnet_enumerated_context_decode(
        &apdu[apdu_len], apdu_size - apdu_len, 0, &enumerated_value);
    if (len > 0) {
        property_identifier = enumerated_value;
        if (value) {
            value->propertyIdentifier = property_identifier;
        }
        apdu_len += len;
    } else {
        return BACNET_STATUS_ERROR;
    }
    /* property-array-index [1] Unsigned OPTIONAL */
    if (bacnet_is_context_tag_number(
            &apdu[apdu_len], apdu_size - apdu_len, 1, &len, &len_value_type)) {
        apdu_len += len;
        len = bacnet_unsigned_decode(
            &apdu[apdu_len], apdu_size - apdu_len, len_value_type,
            &unsigned_value);
        if (len > 0) {
            if (unsigned_value > UINT32_MAX) {
                return BACNET_STATUS_ERROR;
            } else {
                apdu_len += len;
                if (value) {
                    value->propertyArrayIndex = unsigned_value;
                    array_index = value->propertyArrayIndex;
                }
            }
        } else {
            return BACNET_STATUS_ERROR;
        }
    } else {
        if (value) {
            value->propertyArrayIndex = BACNET_ARRAY_ALL;
        }
    }
    /* property-value [2] ABSTRACT-SYNTAX.&Type */
    if (bacnet_is_opening_tag_number(
            &apdu[apdu_len], apdu_size - apdu_len, 2, &len)) {
        if (value) {
            apdu_len += len;
            app_data = &value->value;
            while (app_data != NULL) {
                len = bacapp_decode_known_array_property(
                    &apdu[apdu_len], apdu_size - apdu_len, app_data,
                    object_type, property_identifier, array_index);
                if (len < 0) {
                    return BACNET_STATUS_ERROR;
                }
                apdu_len += len;
                if (bacnet_is_closing_tag_number(
                        &apdu[apdu_len], apdu_size - apdu_len, 2, &len)) {
                    break;
                }
                app_data = app_data->next;
            }
        } else {
            /* this len function needs to start at the opening tag
               to match opening/closing tags like a stack.
               However, it returns the len between the tags.
               Therefore, store the length of the opening tag first */
            tag_len = len;
            len = bacnet_enclosed_data_length(
                &apdu[apdu_len], apdu_size - apdu_len);
            apdu_len += len;
            /* add the opening tag length to the totals */
            apdu_len += tag_len;
        }
        if (bacnet_is_closing_tag_number(
                &apdu[apdu_len], apdu_size - apdu_len, 2, &len)) {
            apdu_len += len;
        } else {
            return BACNET_STATUS_ERROR;
        }
    } else {
        return BACNET_STATUS_ERROR;
    }
    /* priority [3] Unsigned (1..16) OPTIONAL */
    if (bacnet_is_context_tag_number(
            &apdu[apdu_len], apdu_size - apdu_len, 3, &len, &len_value_type)) {
        apdu_len += len;
        len = bacnet_unsigned_decode(
            &apdu[apdu_len], apdu_size - apdu_len, len_value_type,
            &unsigned_value);
        if (len > 0) {
            if (unsigned_value > UINT8_MAX) {
                return BACNET_STATUS_ERROR;
            } else {
                apdu_len += len;
                if (value) {
                    value->priority = unsigned_value;
                }
            }
        } else {
            return BACNET_STATUS_ERROR;
        }
    } else {
        if (value) {
            value->priority = BACNET_NO_PRIORITY;
        }
    }

    return apdu_len;
}

/* generic - can be used by other unit tests
   returns true if matching or same, false if different */
bool bacapp_same_value(
    const BACNET_APPLICATION_DATA_VALUE *value,
    const BACNET_APPLICATION_DATA_VALUE *test_value)
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
                if (!islessgreater(test_value->type.Real, value->type.Real)) {
                    status = true;
                }
                break;
#endif
#if defined(BACAPP_DOUBLE)
            case BACNET_APPLICATION_TAG_DOUBLE:
                if (!islessgreater(
                        test_value->type.Double, value->type.Double)) {
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
                status = characterstring_same(
                    &value->type.Character_String,
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
#if defined(BACAPP_DATERANGE)
            case BACNET_APPLICATION_TAG_DATERANGE:
                status = bacnet_daterange_same(
                    &value->type.Date_Range, &test_value->type.Date_Range);
                break;
#endif
#if defined(BACAPP_TIMESTAMP)
            case BACNET_APPLICATION_TAG_TIMESTAMP:
                status = bacapp_timestamp_same(
                    &value->type.Time_Stamp, &test_value->type.Time_Stamp);
                break;
#endif
#if defined(BACAPP_DATETIME)
            case BACNET_APPLICATION_TAG_DATETIME:
                if (datetime_compare(
                        &value->type.Date_Time, &test_value->type.Date_Time) ==
                    0) {
                    status = true;
                }
                break;
#endif
#if defined(BACAPP_LIGHTING_COMMAND)
            case BACNET_APPLICATION_TAG_LIGHTING_COMMAND:
                status = lighting_command_same(
                    &value->type.Lighting_Command,
                    &test_value->type.Lighting_Command);
                break;
#endif
#if defined(BACAPP_XY_COLOR)
            case BACNET_APPLICATION_TAG_XY_COLOR:
                /* BACnetxyColor */
                status = xy_color_same(
                    &value->type.XY_Color, &test_value->type.XY_Color);
                break;
#endif
#if defined(BACAPP_COLOR_COMMAND)
            case BACNET_APPLICATION_TAG_COLOR_COMMAND:
                /* BACnetColorCommand */
                status = color_command_same(
                    &value->type.Color_Command,
                    &test_value->type.Color_Command);
                break;
#endif
#if defined(BACAPP_WEEKLY_SCHEDULE)
            case BACNET_APPLICATION_TAG_WEEKLY_SCHEDULE:
                /* BACnetWeeklySchedule */
                status = bacnet_weeklyschedule_same(
                    &value->type.Weekly_Schedule,
                    &test_value->type.Weekly_Schedule);
                break;
#endif
#if defined(BACAPP_CALENDAR_ENTRY)
            case BACNET_APPLICATION_TAG_CALENDAR_ENTRY:
                /* BACnetCalendarEntry */
                status = bacnet_calendar_entry_same(
                    &value->type.Calendar_Entry,
                    &test_value->type.Calendar_Entry);
                break;
#endif
#if defined(BACAPP_SPECIAL_EVENT)
            case BACNET_APPLICATION_TAG_SPECIAL_EVENT:
                /* BACnetSpecialEvent */
                status = bacnet_special_event_same(
                    &value->type.Special_Event,
                    &test_value->type.Special_Event);
                break;
#endif
#if defined(BACAPP_HOST_N_PORT)
            case BACNET_APPLICATION_TAG_HOST_N_PORT:
                status = host_n_port_same(
                    &value->type.Host_Address, &test_value->type.Host_Address);
                break;
#endif
#if defined(BACAPP_DEVICE_OBJECT_PROPERTY_REFERENCE)
            case BACNET_APPLICATION_TAG_DEVICE_OBJECT_PROPERTY_REFERENCE:
                status = bacnet_device_object_property_reference_same(
                    &value->type.Device_Object_Property_Reference,
                    &test_value->type.Device_Object_Property_Reference);
                break;
#endif
#if defined(BACAPP_DEVICE_OBJECT_REFERENCE)
            case BACNET_APPLICATION_TAG_DEVICE_OBJECT_REFERENCE:
                status = bacnet_device_object_reference_same(
                    &value->type.Device_Object_Reference,
                    &test_value->type.Device_Object_Reference);
                break;
#endif
#if defined(BACAPP_OBJECT_PROPERTY_REFERENCE)
            case BACNET_APPLICATION_TAG_OBJECT_PROPERTY_REFERENCE:
                status = bacnet_object_property_reference_same(
                    &value->type.Object_Property_Reference,
                    &test_value->type.Object_Property_Reference);
                break;
#endif
#if defined(BACAPP_DESTINATION)
            case BACNET_APPLICATION_TAG_DESTINATION:
                status = bacnet_destination_same(
                    &value->type.Destination, &test_value->type.Destination);
                break;
#endif
#if defined(BACAPP_BDT_ENTRY)
            case BACNET_APPLICATION_TAG_BDT_ENTRY:
                status = bacnet_bdt_entry_same(
                    &value->type.BDT_Entry, &test_value->type.BDT_Entry);
                break;
#endif
#if defined(BACAPP_FDT_ENTRY)
            case BACNET_APPLICATION_TAG_FDT_ENTRY:
                status = bacnet_fdt_entry_same(
                    &value->type.FDT_Entry, &test_value->type.FDT_Entry);
                break;
#endif
#if defined(BACAPP_ACTION_COMMAND)
            case BACNET_APPLICATION_TAG_ACTION_COMMAND:
                status = bacnet_action_command_same(
                    &value->type.Action_Command,
                    &test_value->type.Action_Command);
                break;
#endif
#if defined(BACAPP_SCALE)
            case BACNET_APPLICATION_TAG_SCALE:
                status = bacnet_scale_same(
                    &value->type.Scale, &test_value->type.Scale);
                break;
#endif
#if defined(BACAPP_SHED_LEVEL)
            case BACNET_APPLICATION_TAG_SHED_LEVEL:
                status = bacnet_shed_level_same(
                    &value->type.Shed_Level, &test_value->type.Shed_Level);
                break;
#endif
#if defined(BACAPP_ACCESS_RULE)
            case BACNET_APPLICATION_TAG_ACCESS_RULE:
                status = bacnet_access_rule_same(
                    &value->type.Access_Rule, &test_value->type.Access_Rule);
                break;
#endif
#if defined(BACAPP_CHANNEL_VALUE)
            case BACNET_APPLICATION_TAG_CHANNEL_VALUE:
                status = bacnet_channel_value_same(
                    &value->type.Channel_Value,
                    &test_value->type.Channel_Value);
                break;
#endif
#if defined(BACAPP_TIMER_VALUE)
            case BACNET_APPLICATION_TAG_TIMER_VALUE:
                status = bacnet_timer_value_same(
                    &value->type.Timer_Value, &test_value->type.Timer_Value);
                break;
#endif
#if defined(BACAPP_RECIPIENT)
            case BACNET_APPLICATION_TAG_RECIPIENT:
                status = bacnet_recipient_same(
                    &value->type.Recipient, &test_value->type.Recipient);
                break;
#endif
#if defined(BACAPP_ADDRESS_BINDING)
            case BACNET_APPLICATION_TAG_ADDRESS_BINDING:
                status = bacnet_address_binding_same(
                    &value->type.Address_Binding,
                    &test_value->type.Address_Binding);
                break;
#endif
#if defined(BACAPP_NO_VALUE)
            case BACNET_APPLICATION_TAG_NO_VALUE:
                if (value->tag == test_value->tag) {
                    status = true;
                }
                break;
#endif
#if defined(BACAPP_LOG_RECORD)
            case BACNET_APPLICATION_TAG_LOG_RECORD:
                status = bacnet_log_record_same(
                    &value->type.Log_Record, &test_value->type.Log_Record);
                break;
#endif
            case BACNET_APPLICATION_TAG_EMPTYLIST:
                status = true;
                break;
            default:
                status = false;
                break;
        }
    }
    return status;
}

/**
 * @brief Encode a BACnetDeviceObjectPropertyValue into a buffer
 *  BACnetDeviceObjectPropertyValue ::= SEQUENCE {
 *      device-identifier [0] BACnetObjectIdentifier,
 *      object-identifier [1] BACnetObjectIdentifier,
 *      property-identifier [2] BACnetPropertyIdentifier,
 *      property-array-index [3] Unsigned OPTIONAL,
 *      -- used only with array datatype
 *      property-value [4] ABSTRACT-SYNTAX.&Type
 *  }
 * @param apdu - the APDU buffer, or NULL for length
 * @param value - BACnetDeviceObjectPropertyValue
 * @return length of the APDU buffer
 */
int bacapp_device_object_property_value_encode(
    uint8_t *apdu, const BACNET_DEVICE_OBJECT_PROPERTY_VALUE *value)
{
    int len = 0;
    int apdu_len = 0;

    if (!value) {
        return 0;
    }
    len = encode_context_object_id(
        apdu, 0, value->device_identifier.type,
        value->device_identifier.instance);
    apdu_len += len;
    if (apdu) {
        apdu += len;
    }
    len = encode_context_object_id(
        apdu, 1, value->object_identifier.type,
        value->object_identifier.instance);
    apdu_len += len;
    if (apdu) {
        apdu += len;
    }
    len = encode_context_enumerated(apdu, 2, value->property_identifier);
    apdu_len += len;
    if (apdu) {
        apdu += len;
    }
    if (value->property_array_index != BACNET_ARRAY_ALL) {
        len = encode_context_unsigned(apdu, 3, value->property_array_index);
        apdu_len += len;
    }
    len = encode_opening_tag(apdu, 4);
    apdu_len += len;
    if (apdu) {
        apdu += len;
    }
    len = bacapp_encode_application_data(apdu, value->value);
    apdu_len += len;
    if (apdu) {
        apdu += len;
    }
    len = encode_closing_tag(apdu, 4);
    apdu_len += len;

    return apdu_len;
}

/**
 * @brief Decode a BACnetDeviceObjectPropertyValue from a buffer
 *  BACnetDeviceObjectPropertyValue ::= SEQUENCE {
 *      device-identifier [0] BACnetObjectIdentifier,
 *      object-identifier [1] BACnetObjectIdentifier,
 *      property-identifier [2] BACnetPropertyIdentifier,
 *      property-array-index [3] Unsigned OPTIONAL,
 *      -- used only with array datatype
 *      property-value [4] ABSTRACT-SYNTAX.&Type
 *  }
 * @param apdu - the APDU buffer
 * @param apdu_size - the size of the APDU buffer
 * @param value - BACnetDeviceObjectPropertyValue to decode into
 * @return number of bytes decoded or BACNET_STATUS_ERROR on failure.
 */
int bacapp_device_object_property_value_decode(
    uint8_t *apdu,
    uint32_t apdu_size,
    BACNET_DEVICE_OBJECT_PROPERTY_VALUE *value)
{
    int apdu_len = 0;
    int len = 0;
    BACNET_UNSIGNED_INTEGER array_index = 0;
    BACNET_OBJECT_TYPE object_type = 0;
    uint32_t object_instance = 0;
    uint32_t property_identifier = 0;
    BACNET_APPLICATION_DATA_VALUE *property_value = NULL;

    if (!apdu) {
        return BACNET_STATUS_ERROR;
    }
    /* device-identifier [0] BACnetObjectIdentifier */
    len = bacnet_object_id_context_decode(
        &apdu[apdu_len], apdu_size - apdu_len, 0, &object_type,
        &object_instance);
    if (len > 0) {
        apdu_len += len;
        if (value) {
            value->device_identifier.type = object_type;
            value->device_identifier.instance = object_instance;
        }
    } else {
        return BACNET_STATUS_ERROR;
    }
    /* object-identifier [1] BACnetObjectIdentifier */
    len = bacnet_object_id_context_decode(
        &apdu[apdu_len], apdu_size - apdu_len, 1, &object_type,
        &object_instance);
    if (len > 0) {
        apdu_len += len;
        if (value) {
            value->object_identifier.instance = object_instance;
            value->object_identifier.type = object_type;
        }
    } else {
        return len;
    }
    /* property-identifier [2] BACnetPropertyIdentifier */
    len = bacnet_enumerated_context_decode(
        &apdu[apdu_len], apdu_size - apdu_len, 2, &property_identifier);
    if (len > 0) {
        apdu_len += len;
        if (value) {
            value->property_identifier = property_identifier;
        }
    } else {
        return BACNET_STATUS_ERROR;
    }
    /* property-array-index [3] Unsigned OPTIONAL */
    len = bacnet_unsigned_context_decode(
        &apdu[apdu_len], apdu_size - apdu_len, 3, &array_index);
    if (len > 0) {
        apdu_len += len;
        if (value) {
            value->property_array_index = array_index;
        }
    } else if (len < 0) {
        return BACNET_STATUS_ERROR;
    } else {
        /* OPTIONAL - skip apdu_len increment */
        if (value) {
            value->property_array_index = BACNET_ARRAY_ALL;
        }
    }
    if (bacnet_is_opening_tag_number(apdu, apdu_size, 4, &len)) {
        /* property-value [4] ABSTRACT-SYNTAX.&Type */
        apdu_len += len;
        if (value) {
            property_value = value->value;
        }
        len = bacapp_decode_application_data(
            &apdu[apdu_len], apdu_size - apdu_len, property_value);
        if (len > 0) {
            apdu_len += len;
        } else {
            return BACNET_STATUS_ERROR;
        }
        if (bacnet_is_closing_tag_number(apdu, apdu_size, 4, &len)) {
            apdu_len += len;
        } else {
            return BACNET_STATUS_ERROR;
        }
    }

    return apdu_len;
}

/**
 * @brief Compare the complex data of value1 and value2
 * @param value1 - value 1 structure
 * @param value2 - value 2 structure
 * @return true if the values are the same
 */
bool bacapp_device_object_property_value_same(
    const BACNET_DEVICE_OBJECT_PROPERTY_VALUE *value1,
    const BACNET_DEVICE_OBJECT_PROPERTY_VALUE *value2)
{
    const BACNET_APPLICATION_DATA_VALUE *data_value1, *data_value2;
    if (value1 && value2) {
        if (!bacnet_object_id_same(
                value1->device_identifier.type,
                value1->device_identifier.instance,
                value2->device_identifier.type,
                value2->device_identifier.instance)) {
            return false;
        }
        if (!bacnet_object_id_same(
                value1->object_identifier.type,
                value1->object_identifier.instance,
                value2->object_identifier.type,
                value2->object_identifier.instance)) {
            return false;
        }
        if (value1->property_identifier != value2->property_identifier) {
            return false;
        }
        if (value1->property_array_index != value2->property_array_index) {
            return false;
        }
        data_value1 = value1->value;
        data_value2 = value2->value;
        if (!bacapp_same_value(data_value1, data_value2)) {
            return false;
        }
        return true;
    }

    return false;
}

/**
 * @brief Copy the complex data of src to dest
 * @param dest - destination structure
 * @param src - source structure
 */
void bacapp_device_object_property_value_copy(
    BACNET_DEVICE_OBJECT_PROPERTY_VALUE *dest,
    const BACNET_DEVICE_OBJECT_PROPERTY_VALUE *src)
{
    if (dest && src) {
        memcpy(dest, src, sizeof(BACNET_DEVICE_OBJECT_PROPERTY_VALUE));
    }
}
