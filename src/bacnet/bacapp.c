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
#include <errno.h>
#include <math.h>
#if (__STDC_VERSION__ >= 199901L) && defined(__STDC_ISO_10646__)
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
#include "bacnet/bacaction.h"
#include "bacnet/lighting.h"
#include "bacnet/hostnport.h"
#include "bacnet/weeklyschedule.h"
#include "bacnet/calendar_entry.h"
#include "bacnet/special_event.h"
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
static int bacnet_scale_encode(uint8_t *apdu, BACNET_SCALE *value)
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
bacnet_scale_decode(uint8_t *apdu, size_t apdu_size, BACNET_SCALE *value)
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
static bool bacnet_scale_same(BACNET_SCALE *value1, BACNET_SCALE *value2)
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

#if defined(BACAPP_SHED_LEVEL)
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
static int bacnet_shed_level_encode(uint8_t *apdu, BACNET_SHED_LEVEL *value)
{
    int apdu_len = 0;

    if (!value) {
        return 0;
    }
    switch (value->type) {
        case BACNET_SHED_TYPE_PERCENT:
            apdu_len = encode_context_unsigned(apdu, 0,
                value->value.percent);
            break;
        case BACNET_SHED_TYPE_AMOUNT:
            apdu_len = encode_context_real(apdu, 2,
                value->value.amount);
            break;
        case BACNET_SHED_TYPE_LEVEL:
            apdu_len = encode_context_unsigned(apdu, 1,
                value->value.level);
            break;
        default:
            break;
    }

    return apdu_len;
}
#endif

#if defined(BACAPP_SHED_LEVEL)
/**
 * @brief Decode a BACnetShedLevel value.
 *
 *  BACnetShedLevel ::= CHOICE {
 *      percent [0] Unsigned,
 *      level [1] Unsigned,
 *      amount [2] REAL
 *  }
 *
 * @param apdu - buffer to decode to
 * @param apdu_size - size of the buffer
 * @param value - value to encode
 * @return number of bytes decoded, or BACNET_STATUS_ERROR on error
 */
static int
bacnet_shed_level_decode(uint8_t *apdu, size_t apdu_size, BACNET_SHED_LEVEL *value)
{
    int apdu_len = 0;
    BACNET_TAG tag = { 0 };
    BACNET_UNSIGNED_INTEGER unsigned_value = 0;
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
            /* percent - Unsigned */
            apdu_len = bacnet_unsigned_context_decode(
                apdu, apdu_size, tag.number, &unsigned_value);
            if (apdu_len > 0) {
                value->value.percent = unsigned_value;
                value->type = BACNET_SHED_TYPE_PERCENT;
            }
            break;
        case 1:
            /* level - Unsigned */
            apdu_len = bacnet_unsigned_context_decode(
                apdu, apdu_size, tag.number, &unsigned_value);
            if (apdu_len > 0) {
                value->value.level = unsigned_value;
                value->type = BACNET_SHED_TYPE_LEVEL;
            }
            break;

        case 2:
            apdu_len = bacnet_real_context_decode(
                apdu, apdu_size, tag.number, &real_value);
            if (apdu_len > 0) {
                value->type = BACNET_SHED_TYPE_AMOUNT;
                value->value.amount = real_value;
            }
            break;
        default:
            return BACNET_STATUS_ERROR;
    }

    return apdu_len;
}
#endif

#if defined(BACAPP_SHED_LEVEL)
static bool
bacnet_shed_level_same(BACNET_SHED_LEVEL *value1, BACNET_SHED_LEVEL *value2)
{
    bool status = false;

    if (value1 && value2) {
        status = true;
        if (value1->type != value2->type) {
            status = false;
        } else {
            switch (value1->type) {
                case BACNET_SHED_TYPE_PERCENT:
                    if (value1->value.percent != value2->value.percent) {
                        status = false;
                    }
                    break;
                case BACNET_SHED_TYPE_AMOUNT:
                    if (islessgreater(
                            value1->value.amount, value2->value.amount)) {
                        status = false;
                    }
                    break;
                case BACNET_SHED_TYPE_LEVEL:
                    if (value1->value.level != value2->value.level) {
                        status = false;
                    }
                    break;
                default:
                    break;
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
                apdu_len = encode_application_object_id(
                    apdu, value->type.Object_Id.type,
                    value->type.Object_Id.instance);
                break;
#endif
            case BACNET_APPLICATION_TAG_EMPTYLIST:
                /* Empty data list */
                apdu_len = 0; /* EMPTY */
                break;
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
                apdu_len = bacnet_shed_level_encode(apdu,
                    &value->type.Shed_Level);
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
 *
 * @return Number of octets consumed (could be zero).
 * Parameter value->tag set to MAX_BACNET_APPLICATION_TAG when
 * the number of octets consumed is zero and there is an error
 * in the decoding, or BACNET_STATUS_ERROR/ABORT/REJECT if malformed.
 */
int bacapp_data_decode(
    uint8_t *apdu,
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
    uint8_t *apdu,
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
    uint8_t *apdu, uint32_t apdu_size, BACNET_APPLICATION_DATA_VALUE *value)
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
    uint8_t *new_apdu,
    uint32_t new_apdu_len,
    BACNET_APPLICATION_DATA_VALUE *value)
{
    /* The static variables that store the apdu buffer between function calls */
    static uint8_t *apdu = NULL;
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
    uint8_t *apdu, uint8_t tag_number, uint32_t len_value_type)
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
int bacapp_decode_application_data_len(uint8_t *apdu, unsigned apdu_size)
{
    int len = 0;
    int tag_len = 0;
    int decode_len = 0;
    BACNET_TAG tag = {0};

    if (!bacnet_is_context_specific(apdu, apdu_size)) {
        tag_len = bacnet_tag_decode(apdu, apdu_size, &tag);
        if (tag_len > 0) {
            len += tag_len;
            decode_len = bacnet_application_data_length(tag.number,
                tag.len_value_type);
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
    BACNET_APPLICATION_DATA_VALUE *value)
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
            case BACNET_APPLICATION_TAG_TIMESTAMP:
            case BACNET_APPLICATION_TAG_DATETIME:
            case BACNET_APPLICATION_TAG_DATERANGE:
            case BACNET_APPLICATION_TAG_LIGHTING_COMMAND:
            case BACNET_APPLICATION_TAG_XY_COLOR:
            case BACNET_APPLICATION_TAG_CALENDAR_ENTRY:
            case BACNET_APPLICATION_TAG_SPECIAL_EVENT:
            case BACNET_APPLICATION_TAG_COLOR_COMMAND:
            case BACNET_APPLICATION_TAG_WEEKLY_SCHEDULE:
            case BACNET_APPLICATION_TAG_HOST_N_PORT:
            case BACNET_APPLICATION_TAG_DEVICE_OBJECT_PROPERTY_REFERENCE:
            case BACNET_APPLICATION_TAG_DEVICE_OBJECT_REFERENCE:
            case BACNET_APPLICATION_TAG_OBJECT_PROPERTY_REFERENCE:
            case BACNET_APPLICATION_TAG_DESTINATION:
            case BACNET_APPLICATION_TAG_BDT_ENTRY:
            case BACNET_APPLICATION_TAG_FDT_ENTRY:
            case BACNET_APPLICATION_TAG_ACTION_COMMAND:
            case BACNET_APPLICATION_TAG_SCALE:
            case BACNET_APPLICATION_TAG_SHED_LEVEL:
                /* complex data is enclosed in open/close tags */
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
            default:
                (void)len;
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
    uint8_t *apdu,
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
    uint8_t *apdu,
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
    uint8_t *apdu,
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
            } else if (
                (object_type == OBJECT_DATETIME_PATTERN_VALUE) ||
                (object_type == OBJECT_DATETIME_VALUE)) {
                /* Properties using BACnetDateTime */
                return BACNET_APPLICATION_TAG_DATETIME;
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
            /* FIXME: Properties using BACnetRecipient */
            return -1;

        case PROP_DEVICE_ADDRESS_BINDING:
        case PROP_MANUAL_SLAVE_ADDRESS_BINDING:
        case PROP_SLAVE_ADDRESS_BINDING:
            /* FIXME: BACnetAddressBinding */
            return -1;

        case PROP_ACTION:
            /* BACnetActionCommand */
            return BACNET_APPLICATION_TAG_ACTION_COMMAND;
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

        default:
            return -1;
    }
#else
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
    uint8_t *apdu,
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
#if defined(BACAPP_DATETIME)
        case BACNET_APPLICATION_TAG_DATETIME:
            apdu_len =
                bacnet_datetime_decode(apdu, apdu_size, &value->type.Date_Time);
            break;
#endif
#if defined(BACAPP_TIMESTAMP)
        case BACNET_APPLICATION_TAG_TIMESTAMP:
            apdu_len = bacnet_timestamp_decode(
                apdu, apdu_size, &value->type.Time_Stamp);
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
        default:
            break;
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
    uint8_t *apdu,
    int apdu_size,
    BACNET_APPLICATION_DATA_VALUE *value,
    BACNET_OBJECT_TYPE object_type,
    BACNET_PROPERTY_ID property)
{
    int apdu_len = 0;
    int tag;

    if (bacnet_is_closing_tag(apdu, apdu_size)) {
        if (value) {
            value->tag = BACNET_APPLICATION_TAG_EMPTYLIST;
        }
        apdu_len = 0;
    } else if (property == PROP_PRIORITY_ARRAY) {
        /* BACnetPriorityValue */
        /* special case to reduce complexity - mostly encoded as application
           tagged values, but sometimes encoded as abstract syntax or complex
           data values */
        apdu_len = decode_priority_array_value(
            apdu, apdu_size, value, object_type);
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
    uint8_t *apdu, unsigned apdu_len_max, BACNET_PROPERTY_ID property)
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
    uint8_t *apdu, unsigned apdu_size, BACNET_PROPERTY_ID property)
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

#if defined(BACAPP_SHED_LEVEL)
/**
 * @brief Print a value to a string for EPICS
 * @param str - destination string, or NULL for length only
 * @param str_len - length of the destination string, or 0 for length only
 * @param value - value to be printed
 * @return number of characters written to the string
 */
static int bacapp_snprintf_shed_level(
    char *str, size_t str_len, BACNET_SHED_LEVEL *value)
{
    int length = 0;

    switch (value->type) {
        case BACNET_SHED_TYPE_PERCENT:
            length = bacapp_snprintf(
                str, str_len, "%u%%", (unsigned)value->value.percent);
            break;
        case BACNET_SHED_TYPE_LEVEL:
            length = bacapp_snprintf(
                str, str_len, "%u", (unsigned)value->value.level);
            break;
        case BACNET_SHED_TYPE_AMOUNT:
            length = bacapp_snprintf(
                str, str_len, "%f", (double)value->value.amount);
            break;
        default:
            break;
    }

    return length;
}
#endif

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
            if (value <= BACNET_OBJECT_TYPE_LAST) {
                ret_val = bacapp_snprintf(
                    str, str_len, "%s", bactext_object_type_name(value));
            } else if (value <= BACNET_OBJECT_TYPE_RESERVED_MAX) {
                ret_val = bacapp_snprintf(
                    str, str_len, "reserved %lu", (unsigned long)value);
            } else {
                ret_val = bacapp_snprintf(
                    str, str_len, "proprietary %lu", (unsigned long)value);
            }
            break;
        case PROP_EVENT_STATE:
            ret_val = bacapp_snprintf(
                str, str_len, "%s", bactext_event_state_name(value));
            break;
        case PROP_UNITS:
            if (bactext_engineering_unit_name_proprietary((unsigned)value)) {
                ret_val = bacapp_snprintf(
                    str, str_len, "proprietary %lu", (unsigned long)value);
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
static int bacapp_snprintf_date(char *str, size_t str_len, BACNET_DATE *bdate)
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
static int bacapp_snprintf_time(char *str, size_t str_len, BACNET_TIME *btime)
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
    char *str, size_t str_len, BACNET_OBJECT_ID *object_id)
{
    int ret_val = 0;
    int slen = 0;

    slen = bacapp_snprintf(str, str_len, "(");
    ret_val += bacapp_snprintf_shift(slen, &str, &str_len);
    if (object_id->type <= BACNET_OBJECT_TYPE_LAST) {
        slen = bacapp_snprintf(
            str, str_len, "%s, ", bactext_object_type_name(object_id->type));
    } else if (object_id->type < BACNET_OBJECT_TYPE_RESERVED_MAX) {
        slen = bacapp_snprintf(
            str, str_len, "reserved %u, ", (unsigned)object_id->type);
    } else {
        slen = bacapp_snprintf(
            str, str_len, "proprietary %u, ", (unsigned)object_id->type);
    }
    ret_val += bacapp_snprintf_shift(slen, &str, &str_len);
    slen = bacapp_snprintf(
        str, str_len, "%lu)", (unsigned long)object_id->instance);
    ret_val += slen;

    return ret_val;
}
#endif

#if defined (BACAPP_DATETIME)
/**
 * @brief Print a value to a string for EPICS
 * @param str - destination string, or NULL for length only
 * @param str_len - length of the destination string, or 0 for length only
 * @param value - value to print
 * @return number of characters written
 */
static int
bacapp_snprintf_datetime(char *str, size_t str_len, BACNET_DATE_TIME *value)
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

#if defined(BACAPP_DATERANGE) || defined(BACAPP_CALENDAR_ENTRY)
/**
 * @brief Print a value to a string for EPICS
 * @param str - destination string, or NULL for length only
 * @param str_len - length of the destination string, or 0 for length only
 * @param value - value to print
 * @return number of characters written
 */
static int
bacapp_snprintf_daterange(char *str, size_t str_len, BACNET_DATE_RANGE *value)
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
static int
bacapp_snprintf_weeknday(char *str, size_t str_len, BACNET_WEEKNDAY *value)
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
    char *str, size_t str_len, BACNET_DEVICE_OBJECT_PROPERTY_REFERENCE *value)
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
    char *str, size_t str_len, BACNET_DEVICE_OBJECT_REFERENCE *value)
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
    char *str, size_t str_len, BACNET_OBJECT_PROPERTY_REFERENCE *value)
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
    ret_val += bacapp_snprintf_shift(slen, &str, &str_len);
    ret_val += bacapp_snprintf(str, str_len, "}");

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
    BACNET_WEEKLY_SCHEDULE *ws,
    BACNET_ARRAY_INDEX arrayIndex)
{
    int slen;
    int ret_val = 0;
    int wi, ti;
    BACNET_OBJECT_PROPERTY_VALUE dummyPropValue;
    BACNET_APPLICATION_DATA_VALUE dummyDataValue;

    const char *weekdaynames[7] = { "Mon", "Tue", "Wed", "Thu",
                                    "Fri", "Sat", "Sun" };
    const int loopend = ((arrayIndex == BACNET_ARRAY_ALL) ? 7 : 1);

    /* Find what inner type it uses */
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
        slen = bacapp_snprintf(str, str_len, "(Null; ");
    } else if (inner_tag == -2) {
        slen = bacapp_snprintf(str, str_len, "(MIXED_TYPES; ");
    } else {
        slen = bacapp_snprintf(
            str, str_len, "(%s; ", bactext_application_tag_name(inner_tag));
    }
    ret_val += bacapp_snprintf_shift(slen, &str, &str_len);
    for (wi = 0; wi < loopend; wi++) {
        BACNET_DAILY_SCHEDULE *ds = &ws->weeklySchedule[wi];
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
    char *str, size_t str_len, BACNET_HOST_N_PORT *value)
{
    int slen, len, i;
    char *char_str;
    int ret_val = 0;

    slen = bacapp_snprintf(str, str_len, "{");
    ret_val += bacapp_snprintf_shift(slen, &str, &str_len);
    if (value->host_ip_address) {
        uint8_t *octet_str;
        octet_str = octetstring_value(&value->host.ip_address);
        slen = bacapp_snprintf(
            str, str_len, "%u.%u.%u.%u:%u", (unsigned)octet_str[0],
            (unsigned)octet_str[1], (unsigned)octet_str[2],
            (unsigned)octet_str[3], (unsigned)value->port);
        ret_val += slen;
    } else if (value->host_name) {
        BACNET_CHARACTER_STRING *name;
        name = &value->host.name;
        len = characterstring_length(name);
        char_str = characterstring_value(name);
        slen = bacapp_snprintf(str, str_len, "\"");
        ret_val += bacapp_snprintf_shift(slen, &str, &str_len);
        for (i = 0; i < len; i++) {
            if (isprint(*((unsigned char *)char_str))) {
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
    char *str, size_t str_len, BACNET_CALENDAR_ENTRY *value)
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
    char *str, size_t str_len, BACNET_PRIMITIVE_DATA_VALUE *value)
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
    char *str, size_t str_len, BACNET_DAILY_SCHEDULE *value)
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
    char *str, size_t str_len, BACNET_SPECIAL_EVENT *value)
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
    char *str, size_t str_len, BACNET_ACTION_PROPERTY_VALUE *value)
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
    char *str, size_t str_len, BACNET_ACTION_LIST *value)
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
    char *str, size_t str_len, BACNET_OBJECT_PROPERTY_VALUE *object_value)
{
    size_t len = 0, i = 0;
#if defined(BACAPP_CHARACTER_STRING)
    const char *char_str;
#endif
    const BACNET_APPLICATION_DATA_VALUE *value;
    BACNET_PROPERTY_ID property = PROP_ALL;
    BACNET_OBJECT_TYPE object_type = MAX_BACNET_OBJECT_TYPE;
    int ret_val = 0;
    int slen = 0;
#if (__STDC_VERSION__ >= 199901L) && defined(__STDC_ISO_10646__)
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
                len = octetstring_length(&value->type.Octet_String);
                if (len > 0) {
                    uint8_t *octet_str;
                    octet_str = octetstring_value(&value->type.Octet_String);
                    for (i = 0; i < len; i++) {
                        slen =
                            bacapp_snprintf(str, str_len, "%02X", *octet_str);
                        ret_val += bacapp_snprintf_shift(slen, &str, &str_len);
                        octet_str++;
                    }
                }
                break;
#endif
#if defined(BACAPP_CHARACTER_STRING)
            case BACNET_APPLICATION_TAG_CHARACTER_STRING:
                len = characterstring_length(&value->type.Character_String);
                char_str = characterstring_value(&value->type.Character_String);
                slen = bacapp_snprintf(str, str_len, "\"");
                ret_val += bacapp_snprintf_shift(slen, &str, &str_len);
#if (__STDC_VERSION__ >= 199901L) && defined(__STDC_ISO_10646__)
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
                        if (isprint(*((unsigned char *)char_str))) {
                            slen =
                                bacapp_snprintf(str, str_len, "%c", *char_str);
                        } else {
                            slen = bacapp_snprintf(str, str_len, "%c", '.');
                        }
                        ret_val += bacapp_snprintf_shift(slen, &str, &str_len);
                        char_str++;
                    }
                }
                slen = bacapp_snprintf(str, str_len, "\"");
                ret_val += slen;
                break;
#endif
#if defined(BACAPP_BIT_STRING)
            case BACNET_APPLICATION_TAG_BIT_STRING:
                len = bitstring_bits_used(&value->type.Bit_String);
                slen = bacapp_snprintf(str, str_len, "{");
                ret_val += bacapp_snprintf_shift(slen, &str, &str_len);
                for (i = 0; i < len; i++) {
                    bool bit;
                    bit = bitstring_bit(&value->type.Bit_String, (uint8_t)i);
                    slen = bacapp_snprintf(
                        str, str_len, "%s", bit ? "true" : "false");
                    ret_val += bacapp_snprintf_shift(slen, &str, &str_len);
                    if (i < (len - 1)) {
                        slen = bacapp_snprintf(str, str_len, ",");
                        ret_val += bacapp_snprintf_shift(slen, &str, &str_len);
                    }
                }
                slen = bacapp_snprintf(str, str_len, "}");
                ret_val += slen;
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
                slen = bacapp_timestamp_to_ascii(
                    str, str_len, &value->type.Time_Stamp);
                ret_val += slen;
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
                slen = bacapp_snprintf(str, str_len, "(");
                ret_val += bacapp_snprintf_shift(slen, &str, &str_len);
                slen = bacapp_snprintf(
                    str, str_len, "%s",
                    bactext_color_operation_name(
                        value->type.Color_Command.operation));
                ret_val += bacapp_snprintf_shift(slen, &str, &str_len);
                /* FIXME: add the Lighting Command optional values */
                slen = bacapp_snprintf(str, str_len, ")");
                ret_val += slen;
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
                ret_val = bacapp_snprintf_shed_level(
                    str, str_len, &value->type.Shed_Level);
                break;
#endif
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
#else
bool bacapp_print_value(
    FILE *stream, BACNET_OBJECT_PROPERTY_VALUE *object_value)
{
    (void)stream;
    (void)object_value;
    return false;
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
    char *end;

    if (str[0] == 0) {
        return str;
    }
    end = str + strlen(str) - 1;
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
     Tue: [02:00:00.00 inactive]; ...)

     - the first number is the inner tag (e.g. 1 = boolean, 4 = real, 9 = enum)
     - Day name prefix is optional and ignored.
     - Entries are separated by semicolons.
     - There can be a full week, or only one entry - when using array index to
     modify a single day
     - time-value array can be empty: []
    */

    value->tag = BACNET_APPLICATION_TAG_WEEKLY_SCHEDULE;

    /* Parse the inner tag */
    chunk = strtok(str, ";");
    chunk = ltrim(chunk, "(");
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
        chunk = rtrim(ltrim(chunk, "([ "), " ])");

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
                chunk = trim(chunk, " ");
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
                chunk = ltrim(space + 1, " ");
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
                    return false;
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

#if defined(BACAPP_SIGNED) || defined(BACAPP_BOOLEAN)
static bool strtol_checked(const char *s, long *out)
{
    char *end;
    errno = 0;
    *out = strtol(s, &end, 0);
    if (end == s) {
        /* Conversion was not possible */
        return false;
    }
    if (errno == ERANGE) {
        /* Number too large */
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
        /* Conversion was not possible */
        return false;
    }
    if (errno == ERANGE) {
        /* Number too large */
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
        /* Conversion was not possible */
        return false;
    }
    if (errno == ERANGE) {
        /* Number too large */
        return false;
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

#if defined(BACAPP_SHED_LEVEL)
/**
 * @brief Parse a string into a BACnet Shed Level value
 * @param value [out] The BACnet Shed Level value
 * @param argv [in] The string to parse
 * @return True on success, else False
 */
static bool bacnet_shed_level_from_ascii(
    BACNET_SHED_LEVEL *value, const char *argv)
{
    bool status = false;
    int count;
    unsigned percent, level;
    float amount;
    const char *percentage;
    const char *decimal_point;

    if (!status) {
        percentage = strchr(argv, '%');
        if (percentage) {
            count = sscanf(argv, "%u", &percent);
            if (count == 1) {
                value->type = BACNET_SHED_TYPE_PERCENT;
                value->value.percent = percent;
                status = true;
            }
        }
    }
    if (!status) {
        decimal_point = strchr(argv, '.');
        if (decimal_point) {
            count = sscanf(argv, "%f", &amount);
            if (count == 1) {
                value->type = BACNET_SHED_TYPE_AMOUNT;
                value->value.amount = amount;
                status = true;
            }
        }

    }
    if (!status) {
        count = sscanf(argv, "%u", &level);
        if (count == 1) {
            value->type = BACNET_SHED_TYPE_LEVEL;
            value->value.level = level;
            status = true;
        }
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
                if (strcasecmp(argv, "true") == 0 ||
                    strcasecmp(argv, "active") == 0) {
                    value->type.Boolean = true;
                } else if (
                    strcasecmp(argv, "false") == 0 ||
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
                status = bacnet_scale_from_ascii(
                    &value->type.Scale, argv);
                break;
#endif
#if defined(BACAPP_SHED_LEVEL)
            case BACNET_APPLICATION_TAG_SHED_LEVEL:
                status = bacnet_shed_level_from_ascii(
                    &value->type.Shed_Level, argv);
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
int bacapp_property_value_encode(uint8_t *apdu, BACNET_PROPERTY_VALUE *value)
{
    int len = 0; /* length of each encoding */
    int apdu_len = 0; /* total length of the apdu, return value */
    BACNET_APPLICATION_DATA_VALUE *app_data = NULL;

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
    uint8_t *apdu, uint32_t apdu_size, BACNET_PROPERTY_VALUE *value)
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

/* generic - can be used by other unit tests
   returns true if matching or same, false if different */
bool bacapp_same_value(
    BACNET_APPLICATION_DATA_VALUE *value,
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
