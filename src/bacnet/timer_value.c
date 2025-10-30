/**
 * @file
 * @brief BACnetTimerStateChangeValue data type encoding and decoding
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date October 2025
 * @copyright SPDX-License-Identifier: GPL-2.0-or-later WITH GCC-exception-2.0
 */
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <math.h>
#include <float.h>
/* BACnet Stack defines - first */
#include "bacnet/bacdef.h"
/* BACnet Stack API */
#include "bacnet/bacdcode.h"
#include "bacnet/bacstr.h"
#include "bacnet/bacint.h"
#include "bacnet/bacreal.h"
#include "bacnet/timer_value.h"

/**
 * @brief Encode a context tagged BACnetTimerStateChangeValue with no-value type
 * @param apdu  buffer to be encoded, or NULL for length
 * @param tag_number - context tag number to be encoded
 * @param value The value to be encoded.
 * @return the number of apdu bytes encoded
 */
int bacnet_timer_value_no_value_encode(uint8_t *apdu)
{
    int len = 0;
    int apdu_len = 0;
    const uint8_t tag_number = 0;

    /* no-value[0] Null */
    len = encode_opening_tag(apdu, tag_number);
    apdu_len += len;
    if (apdu) {
        apdu += len;
    }
    len = encode_application_null(apdu);
    apdu_len += len;
    if (apdu) {
        apdu += len;
    }
    len = encode_closing_tag(apdu, tag_number);
    apdu_len += len;

    return apdu_len;
}

/**
 * @brief Decodes a context tagged BACnetTimerStateChangeValue with
 *  no-value type
 * @param apdu - the APDU buffer
 * @param apdu_size - the APDU buffer size
 * @return length of the APDU buffer decoded, or BACNET_STATUS_ERROR
 */
int bacnet_timer_value_no_value_decode(const uint8_t *apdu, uint32_t apdu_size)
{
    int apdu_len = 0;
    int len;
    const uint8_t tag_number = 0;

    /* no-value[0] Null */
    if (!bacnet_is_opening_tag_number(
            &apdu[apdu_len], apdu_size - apdu_len, tag_number, &len)) {
        return BACNET_STATUS_ERROR;
    }
    apdu_len += len;
    len = bacnet_null_application_decode(&apdu[apdu_len], apdu_size - apdu_len);
    if (len < 0) {
        return BACNET_STATUS_ERROR;
    }
    apdu_len += len;
    if (!bacnet_is_closing_tag_number(
            &apdu[apdu_len], apdu_size - apdu_len, tag_number, &len)) {
        return BACNET_STATUS_ERROR;
    }
    apdu_len += len;

    return apdu_len;
}

/**
 * @brief Print a no-value to a string for EPICS
 * @param str - destination string, or NULL for length only
 * @param str_len - length of the destination string, or 0 for length only
 * @return number of characters written
 */
int bacnet_timer_value_no_value_to_ascii(char *str, size_t str_len)
{
    return snprintf(str, str_len, "no-value");
}

/**
 * @brief Parse a string into a application tag value
 * @param tag [out] The application tag value if found
 * @param argv [in] The string to parse
 * @return true on success, else false
 */
bool bacnet_timer_value_no_value_from_ascii(uint8_t *tag, const char *argv)
{
    bool status = false;

    if (bacnet_stricmp(argv, "no-value") == 0) {
        if (tag) {
            *tag = BACNET_APPLICATION_TAG_NO_VALUE;
        }
        status = true;
    }

    return status;
}

/**
 * @brief Encode a given BACnetTimerStateChangeValue
 * @param  apdu - APDU buffer for storing the encoded data, or NULL for length
 * @param  value - BACNET_TIMER_STATE_CHANGE_VALUE value
 * @return  number of bytes in the APDU
 */
int bacnet_timer_value_type_encode(
    uint8_t *apdu, const BACNET_TIMER_STATE_CHANGE_VALUE *value)
{
    int apdu_len = 0;

    if (!value) {
        return 0;
    }
    switch (value->tag) {
        case BACNET_APPLICATION_TAG_NULL:
            apdu_len = encode_application_null(apdu);
            break;
#if defined(BACNET_TIMER_VALUE_BOOLEAN)
        case BACNET_APPLICATION_TAG_BOOLEAN:
            apdu_len = encode_application_boolean(apdu, value->type.Boolean);
            break;
#endif
#if defined(BACNET_TIMER_VALUE_UNSIGNED)
        case BACNET_APPLICATION_TAG_UNSIGNED_INT:
            apdu_len =
                encode_application_unsigned(apdu, value->type.Unsigned_Int);
            break;
#endif
#if defined(BACNET_TIMER_VALUE_SIGNED)
        case BACNET_APPLICATION_TAG_SIGNED_INT:
            apdu_len = encode_application_signed(apdu, value->type.Signed_Int);
            break;
#endif
#if defined(BACNET_TIMER_VALUE_REAL)
        case BACNET_APPLICATION_TAG_REAL:
            apdu_len = encode_application_real(apdu, value->type.Real);
            break;
#endif
#if defined(BACNET_TIMER_VALUE_DOUBLE)
        case BACNET_APPLICATION_TAG_DOUBLE:
            apdu_len = encode_application_double(apdu, value->type.Double);
            break;
#endif
#if defined(BACNET_TIMER_VALUE_OCTET_STRING)
        case BACNET_APPLICATION_TAG_OCTET_STRING:
            apdu_len = encode_application_octet_string(
                apdu, &value->type.Octet_String);
            break;
#endif
#if defined(BACNET_TIMER_VALUE_CHARACTER_STRING)
        case BACNET_APPLICATION_TAG_CHARACTER_STRING:
            apdu_len = encode_application_character_string(
                apdu, &value->type.Character_String);
            break;
#endif
#if defined(BACNET_TIMER_VALUE_BIT_STRING)
        case BACNET_APPLICATION_TAG_BIT_STRING:
            apdu_len =
                encode_application_bitstring(apdu, &value->type.Bit_String);
            break;
#endif
#if defined(BACNET_TIMER_VALUE_ENUMERATED)
        case BACNET_APPLICATION_TAG_ENUMERATED:
            apdu_len =
                encode_application_enumerated(apdu, value->type.Enumerated);
            break;
#endif
#if defined(BACNET_TIMER_VALUE_DATE)
        case BACNET_APPLICATION_TAG_DATE:
            apdu_len = encode_application_date(apdu, &value->type.Date);
            break;
#endif
#if defined(BACNET_TIMER_VALUE_TIME)
        case BACNET_APPLICATION_TAG_TIME:
            apdu_len = encode_application_time(apdu, &value->type.Time);
            break;
#endif
#if defined(BACNET_TIMER_VALUE_OBJECT_ID)
        case BACNET_APPLICATION_TAG_OBJECT_ID:
            apdu_len = encode_application_object_id(
                apdu, (int)value->type.Object_Id.type,
                value->type.Object_Id.instance);
            break;
#endif
#if defined(BACNET_TIMER_VALUE_NO_VALUE)
        case BACNET_APPLICATION_TAG_NO_VALUE:
            /* no-value[0] Null */
            apdu_len = bacnet_timer_value_no_value_encode(apdu);
            break;
#endif
#if defined(BACNET_TIMER_VALUE_CONSTRUCTED_VALUE)
        case BACNET_APPLICATION_TAG_ABSTRACT_SYNTAX:
            /* constructed-value[1] ABSTRACT-SYNTAX.&Type */
            apdu_len = bacnet_constructed_value_context_encode(
                apdu, 1, &value->type.Constructed_Value);
            break;
#endif
#if defined(BACNET_TIMER_VALUE_DATETIME)
        case BACNET_APPLICATION_TAG_DATETIME:
            /* datetime[2] BACnetDateTime */
            apdu_len =
                bacapp_encode_context_datetime(apdu, 2, &value->type.Date_Time);
            break;
#endif
#if defined(BACNET_TIMER_VALUE_LIGHTING_COMMAND)
        case BACNET_APPLICATION_TAG_LIGHTING_COMMAND:
            apdu_len = lighting_command_encode_context(
                apdu, 3, &value->type.Lighting_Command);
            break;
#endif
        default:
            break;
    }

    return apdu_len;
}

/**
 * @brief Decode a BACnetTimerStateChangeValue
 * @param apdu - the APDU buffer
 * @param apdu_size - the APDU buffer size
 * @param tag_data_type - application tag type expected
 * @param len_value_type - the length or value type of the tag value content.
 * @param value - the value where the decoded data is copied into
 * @return length of the APDU buffer decoded, or BACNET_STATUS_ERROR
 */
int bacnet_timer_value_type_decode(
    const uint8_t *apdu,
    size_t apdu_size,
    uint8_t tag_data_type,
    uint32_t len_value_type,
    BACNET_TIMER_STATE_CHANGE_VALUE *value)
{
    int len = 0;

    if (!apdu) {
        return BACNET_STATUS_ERROR;
    }
    if (!value) {
        return BACNET_STATUS_ERROR;
    }
    switch (tag_data_type) {
        case BACNET_APPLICATION_TAG_NULL:
            /* nothing else to do */
            break;
#if defined(BACNET_TIMER_VALUE_BOOLEAN)
        case BACNET_APPLICATION_TAG_BOOLEAN:
            value->type.Boolean = decode_boolean(len_value_type);
            len = 0;
            break;
#endif
#if defined(BACNET_TIMER_VALUE_UNSIGNED)
        case BACNET_APPLICATION_TAG_UNSIGNED_INT:
            len = bacnet_unsigned_decode(
                apdu, apdu_size, len_value_type, &value->type.Unsigned_Int);
            break;
#endif
#if defined(BACNET_TIMER_VALUE_SIGNED)
        case BACNET_APPLICATION_TAG_SIGNED_INT:
            len = bacnet_signed_decode(
                apdu, apdu_size, len_value_type, &value->type.Signed_Int);
            break;
#endif
#if defined(BACNET_TIMER_VALUE_REAL)
        case BACNET_APPLICATION_TAG_REAL:
            len = bacnet_real_decode(
                apdu, apdu_size, len_value_type, &(value->type.Real));
            break;
#endif
#if defined(BACNET_TIMER_VALUE_DOUBLE)
        case BACNET_APPLICATION_TAG_DOUBLE:
            len = bacnet_double_decode(
                apdu, apdu_size, len_value_type, &(value->type.Double));
            break;
#endif
#if defined(BACNET_TIMER_VALUE_OCTET_STRING)
        case BACNET_APPLICATION_TAG_OCTET_STRING:
            len = bacnet_octet_string_decode(
                apdu, apdu_size, len_value_type, &value->type.Octet_String);
            break;
#endif
#if defined(BACNET_TIMER_VALUE_CHARACTER_STRING)
        case BACNET_APPLICATION_TAG_CHARACTER_STRING:
            len = bacnet_character_string_decode(
                apdu, apdu_size, len_value_type, &value->type.Character_String);
            break;
#endif
#if defined(BACNET_TIMER_VALUE_BIT_STRING)
        case BACNET_APPLICATION_TAG_BIT_STRING:
            len = bacnet_bitstring_decode(
                apdu, apdu_size, len_value_type, &value->type.Bit_String);
            break;
#endif
#if defined(BACNET_TIMER_VALUE_ENUMERATED)
        case BACNET_APPLICATION_TAG_ENUMERATED:
            len = bacnet_enumerated_decode(
                apdu, apdu_size, len_value_type, &value->type.Enumerated);
            break;
#endif
#if defined(BACNET_TIMER_VALUE_DATE)
        case BACNET_APPLICATION_TAG_DATE:
            len = bacnet_date_decode(
                apdu, apdu_size, len_value_type, &value->type.Date);
            break;
#endif
#if defined(BACNET_TIMER_VALUE_TIME)
        case BACNET_APPLICATION_TAG_TIME:
            len = bacnet_time_decode(
                apdu, apdu_size, len_value_type, &value->type.Time);
            break;
#endif
#if defined(BACNET_TIMER_VALUE_OBJECT_ID)
        case BACNET_APPLICATION_TAG_OBJECT_ID:
            len = bacnet_object_id_decode(
                apdu, apdu_size, len_value_type, &value->type.Object_Id.type,
                &value->type.Object_Id.instance);
            break;
#endif
        default:
            len = BACNET_STATUS_ERROR;
            break;
    }
    if ((len == 0) && (tag_data_type != BACNET_APPLICATION_TAG_NULL) &&
        (tag_data_type != BACNET_APPLICATION_TAG_BOOLEAN) &&
        (tag_data_type != BACNET_APPLICATION_TAG_OCTET_STRING)) {
        /* indicate that we were not able to decode the value */
        len = BACNET_STATUS_ERROR;
    }
    if (len != BACNET_STATUS_ERROR) {
        value->tag = tag_data_type;
    }

    return len;
}

/**
 * @brief Encode a given BACnetTimerStateChangeValue
 * @param  apdu - APDU buffer for storing the encoded data, or NULL for length
 * @param apdu_size - size of the APDU buffer
 * @param  value - BACNET_TIMER_STATE_CHANGE_VALUE value
 * @return  number of bytes in the APDU, or BACNET_STATUS_ERROR
 */
int bacnet_timer_value_encode(
    uint8_t *apdu,
    size_t apdu_size,
    const BACNET_TIMER_STATE_CHANGE_VALUE *value)
{
    size_t apdu_len = 0; /* total length of the apdu, return value */

    apdu_len = bacnet_timer_value_type_encode(NULL, value);
    if (apdu_len > apdu_size) {
        apdu_len = 0;
    } else {
        apdu_len = bacnet_timer_value_type_encode(apdu, value);
    }

    return apdu_len;
}

/**
 * @brief Decode a given BACnetTimerStateChangeValue
 * @param  apdu - APDU buffer for decoding
 * @param  apdu_size - Count of valid bytes in the buffer
 * @param  value - BACNET_TIMER_STATE_CHANGE_VALUE value to store the decoded
 * data
 * @return  number of bytes decoded or BACNET_STATUS_ERROR on error
 */
int bacnet_timer_value_decode(
    const uint8_t *apdu,
    size_t apdu_size,
    BACNET_TIMER_STATE_CHANGE_VALUE *value)
{
    int len = 0, tag_len = 0;
    int apdu_len = 0;
    BACNET_TAG tag = { 0 };

    if (!apdu) {
        return BACNET_STATUS_ERROR;
    }
    if (!value) {
        return BACNET_STATUS_ERROR;
    }
    tag_len = bacnet_tag_decode(&apdu[apdu_len], apdu_size - apdu_len, &tag);
    if (tag_len > 0) {
        apdu_len += tag_len;
        if (tag.application) {
            len = bacnet_timer_value_type_decode(
                &apdu[apdu_len], apdu_size - apdu_len, tag.number,
                tag.len_value_type, value);
            if (len >= 0) {
                apdu_len += len;
            } else {
                return BACNET_STATUS_ERROR;
            }
        } else if (tag.opening) {
            switch (tag.number) {
                case 0:
                    /* no-value[0] Null */
                    value->tag = BACNET_APPLICATION_TAG_NO_VALUE;
                    if (tag.len_value_type == 0) {
                        len = tag_len;
                    } else {
                        return BACNET_STATUS_ERROR;
                    }
                    break;
                case 1:
                    /* constructed-value[1] ABSTRACT-SYNTAX.&Type */
                    value->tag = BACNET_APPLICATION_TAG_ABSTRACT_SYNTAX;
#if defined(BACNET_TIMER_VALUE_CONSTRUCTED_VALUE)
                    len = bacnet_enclosed_data_length(apdu, apdu_size);
                    len = bacnet_constructed_value_decode(
                        &apdu[apdu_len], apdu_size - apdu_len, len,
                        &value->type.Constructed_Value);
#endif
                    break;
                case 2:
                    /* datetime[2] BACnetDateTime */
                    value->tag = BACNET_APPLICATION_TAG_DATETIME;
#if defined(BACNET_TIMER_VALUE_DATETIME)
                    len = bacnet_datetime_decode(
                        &apdu[apdu_len], apdu_size - apdu_len,
                        &value->type.Date_Time);
#endif
                    break;
                case 3:
                    /* lighting-command[3] BACnetLightingCommand */
                    value->tag = BACNET_APPLICATION_TAG_LIGHTING_COMMAND;
#if defined(BACNET_TIMER_VALUE_LIGHTING_COMMAND)
                    len = lighting_command_decode(
                        &apdu[apdu_len], apdu_size - apdu_len,
                        &value->type.Lighting_Command);
#endif
                    break;
                default:
                    return BACNET_STATUS_ERROR;
            }
            if (len > 0) {
                apdu_len += len;
                if (bacnet_is_closing_tag_number(
                        &apdu[apdu_len], apdu_size - apdu_len, tag.number,
                        &len)) {
                    apdu_len += len;
                } else {
                    return BACNET_STATUS_ERROR;
                }
            } else {
                return BACNET_STATUS_ERROR;
            }
        } else {
            return BACNET_STATUS_ERROR;
        }
    } else {
        return BACNET_STATUS_ERROR;
    }

    return apdu_len;
}

/**
 * @brief Compare two BACnetTimerStateChangeValue values
 * @param value1 [in] The first BACnetTimerStateChangeValue value
 * @param value2 [in] The second BACnetTimerStateChangeValue value
 * @return True if the values are the same, else False
 */
bool bacnet_timer_value_same(
    const BACNET_TIMER_STATE_CHANGE_VALUE *value1,
    const BACNET_TIMER_STATE_CHANGE_VALUE *value2)
{
    if (value1->tag != value2->tag) {
        return false;
    }
    switch (value1->tag) {
        case BACNET_APPLICATION_TAG_NULL:
            return true;
#if defined(BACNET_TIMER_VALUE_BOOLEAN)
        case BACNET_APPLICATION_TAG_BOOLEAN:
            return value1->type.Boolean == value2->type.Boolean;
#endif
#if defined(BACNET_TIMER_VALUE_UNSIGNED)
        case BACNET_APPLICATION_TAG_UNSIGNED_INT:
            return value1->type.Unsigned_Int == value2->type.Unsigned_Int;
#endif
#if defined(BACNET_TIMER_VALUE_SIGNED)
        case BACNET_APPLICATION_TAG_SIGNED_INT:
            return value1->type.Signed_Int == value2->type.Signed_Int;
#endif
#if defined(BACNET_TIMER_VALUE_REAL)
        case BACNET_APPLICATION_TAG_REAL:
            return !islessgreater(value1->type.Real, value2->type.Real);
#endif
#if defined(BACNET_TIMER_VALUE_DOUBLE)
        case BACNET_APPLICATION_TAG_DOUBLE:
            return !islessgreater(value1->type.Double, value2->type.Double);
#endif
#if defined(BACNET_TIMER_VALUE_OCTET_STRING)
        case BACNET_APPLICATION_TAG_OCTET_STRING:
            return octetstring_value_same(
                &value1->type.Octet_String, &value2->type.Octet_String);
#endif
#if defined(BACNET_TIMER_VALUE_CHARACTER_STRING)
        case BACNET_APPLICATION_TAG_CHARACTER_STRING:
            return characterstring_same(
                &value1->type.Character_String, &value2->type.Character_String);
#endif
#if defined(BACNET_TIMER_VALUE_BIT_STRING)
        case BACNET_APPLICATION_TAG_BIT_STRING:
            return bitstring_same(
                &value1->type.Bit_String, &value2->type.Bit_String);
#endif
#if defined(BACNET_TIMER_VALUE_ENUMERATED)
        case BACNET_APPLICATION_TAG_ENUMERATED:
            return value1->type.Enumerated == value2->type.Enumerated;
#endif
#if defined(BACNET_TIMER_VALUE_DATE)
        case BACNET_APPLICATION_TAG_DATE:
            if (datetime_compare_date(&value1->type.Date, &value2->type.Date) ==
                0) {
                return true;
            }
            return false;
#endif
#if defined(BACNET_TIMER_VALUE_TIME)
        case BACNET_APPLICATION_TAG_TIME:
            if (datetime_compare_time(&value1->type.Time, &value2->type.Time) ==
                0) {
                return true;
            }
            return false;
#endif
#if defined(BACNET_TIMER_VALUE_OBJECT_ID)
        case BACNET_APPLICATION_TAG_OBJECT_ID:
            return bacnet_object_id_same(
                value1->type.Object_Id.type, value1->type.Object_Id.instance,
                value2->type.Object_Id.type, value2->type.Object_Id.instance);
#endif
#if defined(BACNET_TIMER_VALUE_DATETIME)
        case BACNET_APPLICATION_TAG_DATETIME:
            return (
                datetime_compare(
                    &value1->type.Date_Time, &value2->type.Date_Time) == 0);
#endif
#if defined(BACNET_TIMER_VALUE_CONSTRUCTED_VALUE)
        case BACNET_APPLICATION_TAG_ABSTRACT_SYNTAX:
            return bacnet_constructed_value_same(
                &value1->type.Constructed_Value,
                &value2->type.Constructed_Value);
#endif
#if defined(BACNET_TIMER_VALUE_LIGHTING_COMMAND)
        case BACNET_APPLICATION_TAG_LIGHTING_COMMAND:
            return lighting_command_same(
                &value1->type.Lighting_Command, &value2->type.Lighting_Command);
#endif
        default:
            break;
    }

    return true;
}

/**
 * @brief Copy a BACnetTimerStateChangeValue to another
 * @param value1 [in] The first BACnetTimerStateChangeValue value
 * @param value2 [in] The second BACnetTimerStateChangeValue value
 * @return true if the value was copied, else false
 */
bool bacnet_timer_value_copy(
    BACNET_TIMER_STATE_CHANGE_VALUE *dest,
    const BACNET_TIMER_STATE_CHANGE_VALUE *src)
{
    if (!dest || !src) {
        return false;
    }
    dest->tag = src->tag;
    switch (src->tag) {
        case BACNET_APPLICATION_TAG_NULL:
        case BACNET_APPLICATION_TAG_NO_VALUE:
            return true;
#if defined(BACNET_TIMER_VALUE_BOOLEAN)
        case BACNET_APPLICATION_TAG_BOOLEAN:
            dest->type.Boolean = src->type.Boolean;
            return true;
#endif
#if defined(BACNET_TIMER_VALUE_UNSIGNED)
        case BACNET_APPLICATION_TAG_UNSIGNED_INT:
            dest->type.Unsigned_Int = src->type.Unsigned_Int;
            return true;
#endif
#if defined(BACNET_TIMER_VALUE_SIGNED)
        case BACNET_APPLICATION_TAG_SIGNED_INT:
            dest->type.Signed_Int = src->type.Signed_Int;
            return true;
#endif
#if defined(BACNET_TIMER_VALUE_REAL)
        case BACNET_APPLICATION_TAG_REAL:
            dest->type.Real = src->type.Real;
            return true;
#endif
#if defined(BACNET_TIMER_VALUE_DOUBLE)
        case BACNET_APPLICATION_TAG_DOUBLE:
            dest->type.Double = src->type.Double;
            return true;
#endif
#if defined(BACNET_TIMER_VALUE_OCTET_STRING)
        case BACNET_APPLICATION_TAG_OCTET_STRING:
            return octetstring_copy(
                &dest->type.Octet_String, &src->type.Octet_String);
#endif
#if defined(BACNET_TIMER_VALUE_CHARACTER_STRING)
        case BACNET_APPLICATION_TAG_CHARACTER_STRING:
            return characterstring_copy(
                &dest->type.Character_String, &src->type.Character_String);
#endif
#if defined(BACNET_TIMER_VALUE_BIT_STRING)
        case BACNET_APPLICATION_TAG_BIT_STRING:
            return bitstring_copy(
                &dest->type.Bit_String, &src->type.Bit_String);
#endif
#if defined(BACNET_TIMER_VALUE_ENUMERATED)
        case BACNET_APPLICATION_TAG_ENUMERATED:
            dest->type.Enumerated = src->type.Enumerated;
            return true;
#endif
#if defined(BACNET_TIMER_VALUE_DATE)
        case BACNET_APPLICATION_TAG_DATE:
            datetime_copy_date(&dest->type.Date, &src->type.Date);
            return true;
#endif
#if defined(BACNET_TIMER_VALUE_TIME)
        case BACNET_APPLICATION_TAG_TIME:
            datetime_copy_time(&dest->type.Time, &src->type.Time);
            return true;
#endif
#if defined(BACNET_TIMER_VALUE_OBJECT_ID)
        case BACNET_APPLICATION_TAG_OBJECT_ID:
            dest->type.Object_Id.type = src->type.Object_Id.type;
            dest->type.Object_Id.instance = src->type.Object_Id.instance;
            return true;
#endif
#if defined(BACNET_TIMER_VALUE_DATETIME)
        case BACNET_APPLICATION_TAG_DATETIME:
            datetime_copy(&dest->type.Date_Time, &src->type.Date_Time);
            return true;
#endif
#if defined(BACNET_TIMER_VALUE_CONSTRUCTED_VALUE)
        case BACNET_APPLICATION_TAG_ABSTRACT_SYNTAX:
            return bacnet_constructed_value_copy(
                &dest->type.Constructed_Value, &src->type.Constructed_Value);
#endif
#if defined(BACNET_TIMER_VALUE_LIGHTING_COMMAND)
        case BACNET_APPLICATION_TAG_LIGHTING_COMMAND:
            return lighting_command_copy(
                &dest->type.Lighting_Command, &src->type.Lighting_Command);
#endif
        default:
            break;
    }

    return false;
}

/**
 * @brief Parse a string into a BACnetTimerStateChangeValue structure
 * @param value [out] The BACnetTimerStateChangeValue value
 * @param argv [in] The string to parse
 * @return true on success, else false
 */
bool bacnet_timer_value_from_ascii(
    BACNET_TIMER_STATE_CHANGE_VALUE *value, const char *argv)
{
    bool status = false;
    int count;
    unsigned long unsigned_value;
    long signed_value;
    float single_value;
    double double_value;
    const char *negative;
    const char *decimal_point;
    const char *lighting_command;
    const char *real_string;
    const char *double_string;

    if (!value || !argv) {
        return false;
    }
    if (!status) {
        if (bacnet_stricmp(argv, "null") == 0) {
            value->tag = BACNET_APPLICATION_TAG_NULL;
            status = true;
        }
    }
    if (!status) {
        status = bacnet_timer_value_no_value_from_ascii(&value->tag, argv);
    }
    if (!status) {
        if (bacnet_stricmp(argv, "true") == 0) {
            value->tag = BACNET_APPLICATION_TAG_BOOLEAN;
#if defined(BACNET_TIMER_VALUE_BOOLEAN)
            value->type.Boolean = true;
#endif
            status = true;
        }
    }
    if (!status) {
        if (bacnet_stricmp(argv, "false") == 0) {
            value->tag = BACNET_APPLICATION_TAG_BOOLEAN;
#if defined(BACNET_TIMER_VALUE_BOOLEAN)
            value->type.Boolean = false;
#endif
            status = true;
        }
    }
    if (!status) {
        lighting_command = strchr(argv, 'L');
        if (!lighting_command) {
            lighting_command = strchr(argv, 'l');
        }
        if (lighting_command) {
            value->tag = BACNET_APPLICATION_TAG_LIGHTING_COMMAND;
#if defined(BACNET_TIMER_VALUE_LIGHTING_COMMAND)
            status = lighting_command_from_ascii(
                &value->type.Lighting_Command, argv + 1);
#endif
        }
    }
    if (!status) {
        real_string = strchr(argv, 'F');
        if (!real_string) {
            real_string = strchr(argv, 'f');
        }
        if (real_string) {
            value->tag = BACNET_APPLICATION_TAG_REAL;
            count = sscanf(argv + 1, "%f", &single_value);
            if (count == 1) {
#if defined(BACNET_TIMER_VALUE_REAL)
                value->type.Real = single_value;
#endif
                status = true;
            }
        }
    }
    if (!status) {
        double_string = strchr(argv, 'D');
        if (!double_string) {
            double_string = strchr(argv, 'd');
        }
        if (double_string) {
            count = sscanf(argv + 1, "%lf", &double_value);
            if (count == 1) {
                value->tag = BACNET_APPLICATION_TAG_DOUBLE;
#if defined(BACNET_TIMER_VALUE_DOUBLE)
                value->type.Double = double_value;
#endif
                status = true;
            }
        }
    }
    if (!status) {
        decimal_point = strchr(argv, '.');
        if (decimal_point) {
            count = sscanf(argv, "%lf", &double_value);
            if (count == 1) {
                if (isgreaterequal(double_value, -FLT_MAX) &&
                    islessequal(double_value, FLT_MAX)) {
                    value->tag = BACNET_APPLICATION_TAG_REAL;
#if defined(BACNET_TIMER_VALUE_REAL)
                    value->type.Real = (float)double_value;
#endif
                } else {
                    value->tag = BACNET_APPLICATION_TAG_DOUBLE;
#if defined(BACNET_TIMER_VALUE_DOUBLE)
                    value->type.Double = double_value;
#endif
                }
                status = true;
            }
        }
    }
    if (!status) {
        negative = strchr(argv, '-');
        if (negative) {
            count = sscanf(argv, "%ld", &signed_value);
            if (count == 1) {
                value->tag = BACNET_APPLICATION_TAG_SIGNED_INT;
#if defined(BACNET_TIMER_VALUE_SIGNED)
                value->type.Signed_Int = signed_value;
#endif
                status = true;
            }
        }
    }
    if (!status) {
        count = sscanf(argv, "%lu", &unsigned_value);
        if (count == 1) {
            value->tag = BACNET_APPLICATION_TAG_UNSIGNED_INT;
#if defined(BACNET_TIMER_VALUE_UNSIGNED)
            value->type.Unsigned_Int = unsigned_value;
#endif
            status = true;
        }
    }

    return status;
}

/**
 * @brief Produce a string from a BACnetTimerStateChangeValue structure
 * @param value [in] The BACnetTimerStateChangeValue value
 * @param str [out] The string to produce, NULL to get length only
 * @param str_size [in] The size of the string buffer
 * @return length of the produced string
 */
int bacnet_timer_value_to_ascii(
    const BACNET_TIMER_STATE_CHANGE_VALUE *value, char *str, size_t str_size)
{
    int str_len = 0;

    if (!value) {
        return 0;
    }
    switch (value->tag) {
        case BACNET_APPLICATION_TAG_NULL:
            str_len = snprintf(str, str_size, "null");
            break;
        case BACNET_APPLICATION_TAG_NO_VALUE:
            str_len = bacnet_timer_value_no_value_to_ascii(str, str_size);
            break;
#if defined(BACNET_TIMER_VALUE_BOOLEAN)
        case BACNET_APPLICATION_TAG_BOOLEAN:
            if (value->type.Boolean) {
                str_len = snprintf(str, str_size, "true");
            } else {
                str_len = snprintf(str, str_size, "false");
            }
            break;
#endif
#if defined(BACNET_TIMER_VALUE_UNSIGNED)
        case BACNET_APPLICATION_TAG_UNSIGNED_INT:
            str_len = snprintf(
                str, str_size, "%lu", (unsigned long)value->type.Unsigned_Int);
            break;
#endif
#if defined(BACNET_TIMER_VALUE_SIGNED)
        case BACNET_APPLICATION_TAG_SIGNED_INT:
            str_len =
                snprintf(str, str_size, "%ld", (long)value->type.Signed_Int);
            break;
#endif
#if defined(BACNET_TIMER_VALUE_REAL)
        case BACNET_APPLICATION_TAG_REAL:
            str_len = snprintf(str, str_size, "%f", (double)value->type.Real);
            break;
#endif
#if defined(BACNET_TIMER_VALUE_DOUBLE)
        case BACNET_APPLICATION_TAG_DOUBLE:
            str_len = snprintf(str, str_size, "%f", value->type.Double);
            break;
#endif
#if defined(BACNET_TIMER_VALUE_LIGHTING_COMMAND)
        case BACNET_APPLICATION_TAG_LIGHTING_COMMAND:
            str_len = lighting_command_to_ascii(
                &value->type.Lighting_Command, str, str_size);
            break;
#endif
        default:
            break;
    }

    return str_len;
}

/**
 * @brief Convert an array of BACnetTimerStateChangeValue to linked list
 * @param array pointer to element zero of the array
 * @param size number of elements in the array
 */
void bacnet_timer_value_link_array(
    BACNET_TIMER_STATE_CHANGE_VALUE *array, size_t size)
{
    size_t i = 0;

    for (i = 0; i < size; i++) {
        if (i > 0) {
            array[i - 1].next = &array[i];
        }
        array[i].next = NULL;
    }
}
