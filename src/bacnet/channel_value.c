/**
 * @file
 * @brief BACnet single precision REAL encode and decode functions
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date 2004
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
#include "bacnet/channel_value.h"

/**
 * @brief Encode a given BACnetChanneValue
 * @param  apdu - APDU buffer for storing the encoded data, or NULL for length
 * @param  value - BACNET_CHANNEL_VALUE value
 * @return  number of bytes in the APDU
 */
int bacnet_channel_value_type_encode(
    uint8_t *apdu, const BACNET_CHANNEL_VALUE *value)
{
    int apdu_len = 0;

    if (!value) {
        return 0;
    }
    switch (value->tag) {
        case BACNET_APPLICATION_TAG_NULL:
            apdu_len = encode_application_null(apdu);
            break;
#if defined(CHANNEL_BOOLEAN)
        case BACNET_APPLICATION_TAG_BOOLEAN:
            apdu_len = encode_application_boolean(apdu, value->type.Boolean);
            break;
#endif
#if defined(CHANNEL_UNSIGNED)
        case BACNET_APPLICATION_TAG_UNSIGNED_INT:
            apdu_len =
                encode_application_unsigned(apdu, value->type.Unsigned_Int);
            break;
#endif
#if defined(CHANNEL_SIGNED)
        case BACNET_APPLICATION_TAG_SIGNED_INT:
            apdu_len = encode_application_signed(apdu, value->type.Signed_Int);
            break;
#endif
#if defined(CHANNEL_REAL)
        case BACNET_APPLICATION_TAG_REAL:
            apdu_len = encode_application_real(apdu, value->type.Real);
            break;
#endif
#if defined(CHANNEL_DOUBLE)
        case BACNET_APPLICATION_TAG_DOUBLE:
            apdu_len = encode_application_double(apdu, value->type.Double);
            break;
#endif
#if defined(CHANNEL_OCTET_STRING)
        case BACNET_APPLICATION_TAG_OCTET_STRING:
            apdu_len = encode_application_octet_string(
                apdu, &value->type.Octet_String);
            break;
#endif
#if defined(CHANNEL_CHARACTER_STRING)
        case BACNET_APPLICATION_TAG_CHARACTER_STRING:
            apdu_len = encode_application_character_string(
                apdu, &value->type.Character_String);
            break;
#endif
#if defined(CHANNEL_BIT_STRING)
        case BACNET_APPLICATION_TAG_BIT_STRING:
            apdu_len =
                encode_application_bitstring(apdu, &value->type.Bit_String);
            break;
#endif
#if defined(CHANNEL_ENUMERATED)
        case BACNET_APPLICATION_TAG_ENUMERATED:
            apdu_len =
                encode_application_enumerated(apdu, value->type.Enumerated);
            break;
#endif
#if defined(CHANNEL_DATE)
        case BACNET_APPLICATION_TAG_DATE:
            apdu_len = encode_application_date(apdu, &value->type.Date);
            break;
#endif
#if defined(CHANNEL_TIME)
        case BACNET_APPLICATION_TAG_TIME:
            apdu_len = encode_application_time(apdu, &value->type.Time);
            break;
#endif
#if defined(CHANNEL_OBJECT_ID)
        case BACNET_APPLICATION_TAG_OBJECT_ID:
            apdu_len = encode_application_object_id(
                apdu, (int)value->type.Object_Id.type,
                value->type.Object_Id.instance);
            break;
#endif
#if defined(CHANNEL_LIGHTING_COMMAND)
        case BACNET_APPLICATION_TAG_LIGHTING_COMMAND:
            apdu_len = lighting_command_encode_context(
                apdu, 0, &value->type.Lighting_Command);
            break;
#endif
#if defined(CHANNEL_COLOR_COMMAND)
        case BACNET_APPLICATION_TAG_COLOR_COMMAND:
            apdu_len = color_command_context_encode(
                apdu, 1, &value->type.Color_Command);
            break;
#endif
#if defined(CHANNEL_XY_COLOR)
        case BACNET_APPLICATION_TAG_XY_COLOR:
            apdu_len = xy_color_context_encode(apdu, 2, &value->type.XY_Color);
            break;
#endif
        default:
            break;
    }

    return apdu_len;
}

/**
 * @brief Decode a BACnet channel value
 */
int bacnet_channel_value_type_decode(
    const uint8_t *apdu,
    size_t apdu_size,
    uint8_t tag_data_type,
    uint32_t len_value_type,
    BACNET_CHANNEL_VALUE *value)
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
#if defined(CHANNEL_BOOLEAN)
        case BACNET_APPLICATION_TAG_BOOLEAN:
            value->type.Boolean = decode_boolean(len_value_type);
            break;
#endif
#if defined(CHANNEL_UNSIGNED)
        case BACNET_APPLICATION_TAG_UNSIGNED_INT:
            len = bacnet_unsigned_decode(
                apdu, apdu_size, len_value_type, &value->type.Unsigned_Int);
            break;
#endif
#if defined(CHANNEL_SIGNED)
        case BACNET_APPLICATION_TAG_SIGNED_INT:
            len = bacnet_signed_decode(
                apdu, apdu_size, len_value_type, &value->type.Signed_Int);
            break;
#endif
#if defined(CHANNEL_REAL)
        case BACNET_APPLICATION_TAG_REAL:
            len = bacnet_real_decode(
                apdu, apdu_size, len_value_type, &(value->type.Real));
            break;
#endif
#if defined(CHANNEL_DOUBLE)
        case BACNET_APPLICATION_TAG_DOUBLE:
            len = bacnet_double_decode(
                apdu, apdu_size, len_value_type, &(value->type.Double));
            break;
#endif
#if defined(CHANNEL_OCTET_STRING)
        case BACNET_APPLICATION_TAG_OCTET_STRING:
            len = bacnet_octet_string_decode(
                apdu, apdu_size, len_value_type, &value->type.Octet_String);
            break;
#endif
#if defined(CHANNEL_CHARACTER_STRING)
        case BACNET_APPLICATION_TAG_CHARACTER_STRING:
            len = bacnet_character_string_decode(
                apdu, apdu_size, len_value_type, &value->type.Character_String);
            break;
#endif
#if defined(CHANNEL_BIT_STRING)
        case BACNET_APPLICATION_TAG_BIT_STRING:
            len = bacnet_bitstring_decode(
                apdu, apdu_size, len_value_type, &value->type.Bit_String);
            break;
#endif
#if defined(CHANNEL_ENUMERATED)
        case BACNET_APPLICATION_TAG_ENUMERATED:
            len = bacnet_enumerated_decode(
                apdu, apdu_size, len_value_type, &value->type.Enumerated);
            break;
#endif
#if defined(CHANNEL_DATE)
        case BACNET_APPLICATION_TAG_DATE:
            len = bacnet_date_decode(
                apdu, apdu_size, len_value_type, &value->type.Date);
            break;
#endif
#if defined(CHANNEL_TIME)
        case BACNET_APPLICATION_TAG_TIME:
            len = bacnet_time_decode(
                apdu, apdu_size, len_value_type, &value->type.Time);
            break;
#endif
#if defined(CHANNEL_OBJECT_ID)
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
 * @brief Encode a given channel value
 * @param  apdu - APDU buffer for storing the encoded data, or NULL for length
 * @param apdu_size - size of the APDU buffer
 * @param  value - BACNET_CHANNEL_VALUE value
 * @return  number of bytes in the APDU, or BACNET_STATUS_ERROR
 */
int bacnet_channel_value_encode(
    uint8_t *apdu, size_t apdu_size, const BACNET_CHANNEL_VALUE *value)
{
    size_t apdu_len = 0; /* total length of the apdu, return value */

    apdu_len = bacnet_channel_value_type_encode(NULL, value);
    if (apdu_len > apdu_size) {
        apdu_len = 0;
    } else {
        apdu_len = bacnet_channel_value_type_encode(apdu, value);
    }

    return apdu_len;
}

/**
 * @brief Decode a given channel value
 * @param  apdu - APDU buffer for decoding
 * @param  apdu_size - Count of valid bytes in the buffer
 * @param  value - BACNET_CHANNEL_VALUE value to store the decoded data
 * @return  number of bytes decoded or BACNET_STATUS_ERROR on error
 */
int bacnet_channel_value_decode(
    const uint8_t *apdu, size_t apdu_size, BACNET_CHANNEL_VALUE *value)
{
    int len = 0;
    int apdu_len = 0;
    BACNET_TAG tag = { 0 };

    if (!apdu) {
        return BACNET_STATUS_ERROR;
    }
    if (!value) {
        return BACNET_STATUS_ERROR;
    }
    len = bacnet_tag_decode(&apdu[apdu_len], apdu_size - apdu_len, &tag);
    if (len > 0) {
        apdu_len += len;
        if (tag.application) {
            len = bacnet_channel_value_type_decode(
                &apdu[apdu_len], apdu_size - apdu_len, tag.number,
                tag.len_value_type, value);
            if (len >= 0) {
                apdu_len += len;
            } else {
                return BACNET_STATUS_ERROR;
            }
        } else if (tag.opening) {
            len = 0;
            switch (tag.number) {
                case 0:
                    value->tag = BACNET_APPLICATION_TAG_LIGHTING_COMMAND;
#if defined(CHANNEL_LIGHTING_COMMAND)
                    len = lighting_command_decode(
                        &apdu[apdu_len], apdu_size - apdu_len,
                        &value->type.Lighting_Command);
#endif
                    break;
                case 1:
                    value->tag = BACNET_APPLICATION_TAG_COLOR_COMMAND;
#if defined(CHANNEL_COLOR_COMMAND)
                    len = color_command_decode(
                        &apdu[apdu_len], apdu_size - apdu_len, NULL,
                        &value->type.Color_Command);
#endif
                    break;
                case 2:
                    value->tag = BACNET_APPLICATION_TAG_XY_COLOR;
#if defined(CHANNEL_XY_COLOR)
                    len = xy_color_decode(
                        &apdu[apdu_len], apdu_size - apdu_len,
                        &value->type.XY_Color);
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
 * @brief Compare two BACnetChannelValue values
 * @param value1 [in] The first BACnetChannelValue value
 * @param value2 [in] The second BACnetChannelValue value
 * @return True if the values are the same, else False
 */
bool bacnet_channel_value_same(
    const BACNET_CHANNEL_VALUE *value1, const BACNET_CHANNEL_VALUE *value2)
{
    if (value1->tag != value2->tag) {
        return false;
    }
    switch (value1->tag) {
        case BACNET_APPLICATION_TAG_NULL:
            return true;
#if defined(CHANNEL_BOOLEAN)
        case BACNET_APPLICATION_TAG_BOOLEAN:
            return value1->type.Boolean == value2->type.Boolean;
#endif
#if defined(CHANNEL_UNSIGNED)
        case BACNET_APPLICATION_TAG_UNSIGNED_INT:
            return value1->type.Unsigned_Int == value2->type.Unsigned_Int;
#endif
#if defined(CHANNEL_SIGNED)
        case BACNET_APPLICATION_TAG_SIGNED_INT:
            return value1->type.Signed_Int == value2->type.Signed_Int;
#endif
#if defined(CHANNEL_REAL)
        case BACNET_APPLICATION_TAG_REAL:
            return !islessgreater(value1->type.Real, value2->type.Real);
#endif
#if defined(CHANNEL_DOUBLE)
        case BACNET_APPLICATION_TAG_DOUBLE:
            return !islessgreater(value1->type.Double, value2->type.Double);
#endif
#if defined(CHANNEL_OCTET_STRING)
        case BACNET_APPLICATION_TAG_OCTET_STRING:
            return octetstring_value_same(
                &value1->type.Octet_String, &value2->type.Octet_String);
#endif
#if defined(CHANNEL_CHARACTER_STRING)
        case BACNET_APPLICATION_TAG_CHARACTER_STRING:
            return characterstring_value_same(
                &value1->type.Character_String, &value2->type.Character_String);
#endif
#if defined(CHANNEL_BIT_STRING)
        case BACNET_APPLICATION_TAG_BIT_STRING:
            return bitstring_value_same(
                &value1->type.Bit_String, &value2->type.Bit_String);
#endif
#if defined(CHANNEL_ENUMERATED)
        case BACNET_APPLICATION_TAG_ENUMERATED:
            return value1->type.Enumerated == value2->type.Enumerated;
#endif
#if defined(CHANNEL_DATE)
        case BACNET_APPLICATION_TAG_DATE:
            return date_value_same(&value1->type.Date, &value2->type.Date);
#endif
#if defined(CHANNEL_TIME)
        case BACNET_APPLICATION_TAG_TIME:
            return time_value_same(&value1->type.Time, &value2->type.Time);
#endif
#if defined(CHANNEL_OBJECT_ID)
        case BACNET_APPLICATION_TAG_OBJECT_ID:
            return object_id_value_same(
                &value1->type.Object_Id, &value2->type.Object_Id);
#endif
#if defined(CHANNEL_LIGHTING_COMMAND)
        case BACNET_APPLICATION_TAG_LIGHTING_COMMAND:
            return lighting_command_same(
                &value1->type.Lighting_Command, &value2->type.Lighting_Command);
#endif
#if defined(CHANNEL_COLOR_COMMAND)
        case BACNET_APPLICATION_TAG_COLOR_COMMAND:
            return color_command_same(
                &value1->type.Color_Command, &value2->type.Color_Command);
#endif
#if defined(CHANNEL_XY_COLOR)
        case BACNET_APPLICATION_TAG_XY_COLOR:
            return xy_color_same(
                &value1->type.XY_Color, &value2->type.XY_Color);
#endif
        default:
            break;
    }

    return true;
}

/**
 * @brief Copy a BACnetChannelValue to another
 * @param value1 [in] The first BACnetChannelValue value
 * @param value2 [in] The second BACnetChannelValue value
 * @return true if the value was copied, else false
 */
bool bacnet_channel_value_copy(
    BACNET_CHANNEL_VALUE *dest, const BACNET_CHANNEL_VALUE *src)
{
    if (!dest || !src) {
        return false;
    }
    dest->tag = src->tag;
    switch (src->tag) {
        case BACNET_APPLICATION_TAG_NULL:
            return true;
#if defined(CHANNEL_BOOLEAN)
        case BACNET_APPLICATION_TAG_BOOLEAN:
            dest->type.Boolean = src->type.Boolean;
            return true;
#endif
#if defined(CHANNEL_UNSIGNED)
        case BACNET_APPLICATION_TAG_UNSIGNED_INT:
            dest->type.Unsigned_Int = src->type.Unsigned_Int;
            return true;
#endif
#if defined(CHANNEL_SIGNED)
        case BACNET_APPLICATION_TAG_SIGNED_INT:
            dest->type.Signed_Int = src->type.Signed_Int;
            return true;
#endif
#if defined(CHANNEL_REAL)
        case BACNET_APPLICATION_TAG_REAL:
            dest->type.Real = src->type.Real;
            return true;
#endif
#if defined(CHANNEL_DOUBLE)
        case BACNET_APPLICATION_TAG_DOUBLE:
            dest->type.Double = src->type.Double;
            return true;
#endif
#if defined(CHANNEL_OCTET_STRING)
        case BACNET_APPLICATION_TAG_OCTET_STRING:
            return octetstring_copy(
                &dest->type.Octet_String, &src->type.Octet_String);
#endif
#if defined(CHANNEL_CHARACTER_STRING)
        case BACNET_APPLICATION_TAG_CHARACTER_STRING:
            return characterstring_copy(
                &dest->type.Character_String, &src->type.Character_String);
#endif
#if defined(CHANNEL_BIT_STRING)
        case BACNET_APPLICATION_TAG_BIT_STRING:
            return bitstring_copy(
                &dest->type.Bit_String, &src->type.Bit_String);
#endif
#if defined(CHANNEL_ENUMERATED)
        case BACNET_APPLICATION_TAG_ENUMERATED:
            dest->type.Enumerated = src->type.Enumerated;
            return true;
#endif
#if defined(CHANNEL_DATE)
        case BACNET_APPLICATION_TAG_DATE:
            return datetime_copy_date(&dest->type.Date, &src->type.Date);
#endif
#if defined(CHANNEL_TIME)
        case BACNET_APPLICATION_TAG_TIME:
            return datetime_copy_time(&dest->type.Time, &src->type.Time);
#endif
#if defined(CHANNEL_OBJECT_ID)
        case BACNET_APPLICATION_TAG_OBJECT_ID:
            dest->type.Object_Id.type = src->type.Object_Id.type;
            dest->type.Object_Id.instance = src->type.Object_Id.instance;
            return true;
#endif
#if defined(CHANNEL_LIGHTING_COMMAND)
        case BACNET_APPLICATION_TAG_LIGHTING_COMMAND:
            return lighting_command_copy(
                &dest->type.Lighting_Command, &src->type.Lighting_Command);
#endif
#if defined(CHANNEL_COLOR_COMMAND)
        case BACNET_APPLICATION_TAG_COLOR_COMMAND:
            return color_command_copy(
                &dest->type.Color_Command, &src->type.Color_Command);
#endif
#if defined(CHANNEL_XY_COLOR)
        case BACNET_APPLICATION_TAG_XY_COLOR:
            return xy_color_copy(&dest->type.XY_Color, &src->type.XY_Color);
#endif
        default:
            break;
    }

    return false;
}

/**
 * @brief Parse a string into a BACnetChannelValue structure
 * @param value [out] The BACnetChannelValue value
 * @param argv [in] The string to parse
 * @return true on success, else false
 */
bool bacnet_channel_value_from_ascii(
    BACNET_CHANNEL_VALUE *value, const char *argv)
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
    const char *color_command;
    const char *xy_color;
    const char *real_string;
    const char *double_string;

    if (!value || !argv) {
        return false;
    }
    if (!status) {
        if (strcasecmp(argv, "null") == 0) {
            value->tag = BACNET_APPLICATION_TAG_NULL;
            status = true;
        }
    }
    if (!status) {
        if (strcasecmp(argv, "true") == 0) {
            value->tag = BACNET_APPLICATION_TAG_BOOLEAN;
#if defined(CHANNEL_BOOLEAN)
            value->type.Boolean = true;
#endif
            status = true;
        }
    }
    if (!status) {
        if (strcasecmp(argv, "false") == 0) {
            value->tag = BACNET_APPLICATION_TAG_BOOLEAN;
#if defined(CHANNEL_BOOLEAN)
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
#if defined(CHANNEL_LIGHTING_COMMAND)
            status = lighting_command_from_ascii(
                &value->type.Lighting_Command, argv + 1);
#endif
        }
    }
    if (!status) {
        color_command = strchr(argv, 'C');
        if (!color_command) {
            color_command = strchr(argv, 'c');
        }
        if (color_command) {
            value->tag = BACNET_APPLICATION_TAG_COLOR_COMMAND;
#if defined(CHANNEL_COLOR_COMMAND)
            /* FIXME: add parsing for BACnetColorCommand */
#endif
            status = true;
        }
    }
    if (!status) {
        xy_color = strchr(argv, 'X');
        if (!xy_color) {
            xy_color = strchr(argv, 'x');
        }
        if (xy_color) {
            value->tag = BACNET_APPLICATION_TAG_XY_COLOR;
#if defined(CHANNEL_XY_COLOR)
            status = xy_color_from_ascii(&value->type.XY_Color, argv + 1);
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
#if defined(CHANNEL_REAL)
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
#if defined(CHANNEL_DOUBLE)
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
#if defined(CHANNEL_REAL)
                    value->type.Real = (float)double_value;
#endif
                } else {
                    value->tag = BACNET_APPLICATION_TAG_DOUBLE;
#if defined(CHANNEL_DOUBLE)
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
#if defined(CHANNEL_SIGNED)
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
#if defined(CHANNEL_UNSIGNED)
            value->type.Unsigned_Int = unsigned_value;
#endif
            status = true;
        }
    }

    return status;
}

/**
 * @brief Convert an array of BACnetChannelValue to linked list
 * @param array pointer to element zero of the array
 * @param size number of elements in the array
 */
void bacnet_channel_value_link_array(BACNET_CHANNEL_VALUE *array, size_t size)
{
    size_t i = 0;

    for (i = 0; i < size; i++) {
        if (i > 0) {
            array[i - 1].next = &array[i];
        }
        array[i].next = NULL;
    }
}

/**
 * For a given application value, coerce the encoding, if necessary
 *
 * @param  apdu - buffer to hold the encoding, or NULL for length
 * @param  value - BACNET_APPLICATION_DATA_VALUE value
 * @param  tag - application tag to be coerced, if possible
 *
 * @return  number of bytes in the APDU, or BACNET_STATUS_ERROR if error.
 */
static int channel_value_coerce_data_encode(
    uint8_t *apdu,
    const BACNET_CHANNEL_VALUE *value,
    BACNET_APPLICATION_TAG tag)
{
    int apdu_len = 0; /* total length of the apdu, return value */
    float float_value = 0.0;
    double double_value = 0.0;
    uint32_t unsigned_value = 0;
    int32_t signed_value = 0;
    bool boolean_value = false;

    if (!value) {
        return BACNET_STATUS_ERROR;
    }
    switch (value->tag) {
        case BACNET_APPLICATION_TAG_NULL:
            if ((tag == BACNET_APPLICATION_TAG_LIGHTING_COMMAND) ||
                (tag == BACNET_APPLICATION_TAG_COLOR_COMMAND)) {
                apdu_len = BACNET_STATUS_ERROR;
            } else {
                /* no coercion */
                if (apdu) {
                    *apdu = value->tag;
                }
                apdu_len++;
            }
            break;
#if defined(CHANNEL_BOOLEAN)
        case BACNET_APPLICATION_TAG_BOOLEAN:
            if (tag == BACNET_APPLICATION_TAG_BOOLEAN) {
                apdu_len =
                    encode_application_boolean(apdu, value->type.Boolean);
            } else if (tag == BACNET_APPLICATION_TAG_UNSIGNED_INT) {
                if (value->type.Boolean) {
                    unsigned_value = 1;
                }
                apdu_len = encode_application_unsigned(apdu, unsigned_value);
            } else if (tag == BACNET_APPLICATION_TAG_SIGNED_INT) {
                if (value->type.Boolean) {
                    signed_value = 1;
                }
                apdu_len = encode_application_signed(apdu, signed_value);
            } else if (tag == BACNET_APPLICATION_TAG_REAL) {
                if (value->type.Boolean) {
                    float_value = 1;
                }
                apdu_len = encode_application_real(apdu, float_value);
            } else if (tag == BACNET_APPLICATION_TAG_DOUBLE) {
                if (value->type.Boolean) {
                    double_value = 1;
                }
                apdu_len = encode_application_double(apdu, double_value);
            } else if (tag == BACNET_APPLICATION_TAG_ENUMERATED) {
                if (value->type.Boolean) {
                    unsigned_value = 1;
                }
                apdu_len = encode_application_enumerated(apdu, unsigned_value);
            } else {
                apdu_len = BACNET_STATUS_ERROR;
            }
            break;
#endif
#if defined(CHANNEL_UNSIGNED)
        case BACNET_APPLICATION_TAG_UNSIGNED_INT:
            if (tag == BACNET_APPLICATION_TAG_BOOLEAN) {
                if (value->type.Unsigned_Int) {
                    boolean_value = true;
                }
                apdu_len = encode_application_boolean(apdu, boolean_value);
            } else if (tag == BACNET_APPLICATION_TAG_UNSIGNED_INT) {
                unsigned_value = value->type.Unsigned_Int;
                apdu_len = encode_application_unsigned(apdu, unsigned_value);
            } else if (tag == BACNET_APPLICATION_TAG_SIGNED_INT) {
                if (value->type.Unsigned_Int <= 2147483647) {
                    signed_value = value->type.Unsigned_Int;
                    apdu_len = encode_application_signed(apdu, signed_value);
                } else {
                    apdu_len = BACNET_STATUS_ERROR;
                }
            } else if (tag == BACNET_APPLICATION_TAG_REAL) {
                if (value->type.Unsigned_Int <= 9999999) {
                    float_value = (float)value->type.Unsigned_Int;
                    apdu_len = encode_application_real(apdu, float_value);
                } else {
                    apdu_len = BACNET_STATUS_ERROR;
                }
            } else if (tag == BACNET_APPLICATION_TAG_DOUBLE) {
                double_value = (double)value->type.Unsigned_Int;
                apdu_len = encode_application_double(apdu, double_value);
            } else if (tag == BACNET_APPLICATION_TAG_ENUMERATED) {
                unsigned_value = value->type.Unsigned_Int;
                apdu_len = encode_application_enumerated(apdu, unsigned_value);
            } else {
                apdu_len = BACNET_STATUS_ERROR;
            }
            break;
#endif
#if defined(CHANNEL_SIGNED)
        case BACNET_APPLICATION_TAG_SIGNED_INT:
            if (tag == BACNET_APPLICATION_TAG_BOOLEAN) {
                if (value->type.Signed_Int) {
                    boolean_value = true;
                }
                apdu_len = encode_application_boolean(apdu, boolean_value);
            } else if (tag == BACNET_APPLICATION_TAG_UNSIGNED_INT) {
                if ((value->type.Signed_Int >= 0) &&
                    (value->type.Signed_Int <= 2147483647)) {
                    unsigned_value = value->type.Signed_Int;
                    apdu_len =
                        encode_application_unsigned(apdu, unsigned_value);
                } else {
                    apdu_len = BACNET_STATUS_ERROR;
                }
            } else if (tag == BACNET_APPLICATION_TAG_SIGNED_INT) {
                signed_value = value->type.Signed_Int;
                apdu_len = encode_application_signed(apdu, signed_value);
            } else if (tag == BACNET_APPLICATION_TAG_REAL) {
                if (value->type.Signed_Int <= 9999999) {
                    float_value = (float)value->type.Signed_Int;
                    apdu_len = encode_application_real(apdu, float_value);
                } else {
                    apdu_len = BACNET_STATUS_ERROR;
                }
            } else if (tag == BACNET_APPLICATION_TAG_DOUBLE) {
                double_value = (double)value->type.Signed_Int;
                apdu_len = encode_application_double(apdu, double_value);
            } else if (tag == BACNET_APPLICATION_TAG_ENUMERATED) {
                unsigned_value = value->type.Signed_Int;
                apdu_len = encode_application_enumerated(apdu, unsigned_value);
            } else {
                apdu_len = BACNET_STATUS_ERROR;
            }
            break;
#endif
#if defined(CHANNEL_REAL)
        case BACNET_APPLICATION_TAG_REAL:
            if (tag == BACNET_APPLICATION_TAG_BOOLEAN) {
                if (islessgreater(value->type.Real, 0.0F)) {
                    boolean_value = true;
                }
                apdu_len = encode_application_boolean(apdu, boolean_value);
            } else if (tag == BACNET_APPLICATION_TAG_UNSIGNED_INT) {
                if ((value->type.Real >= 0.0F) &&
                    (value->type.Real <= 2147483000.0F)) {
                    unsigned_value = (uint32_t)value->type.Real;
                    apdu_len =
                        encode_application_unsigned(apdu, unsigned_value);
                } else {
                    apdu_len = BACNET_STATUS_ERROR;
                }
            } else if (tag == BACNET_APPLICATION_TAG_SIGNED_INT) {
                if ((value->type.Real >= -2147483000.0F) &&
                    (value->type.Real <= 214783000.0F)) {
                    signed_value = (int32_t)value->type.Real;
                    apdu_len = encode_application_signed(apdu, signed_value);
                } else {
                    apdu_len = BACNET_STATUS_ERROR;
                }
            } else if (tag == BACNET_APPLICATION_TAG_REAL) {
                float_value = value->type.Real;
                apdu_len = encode_application_real(apdu, float_value);
            } else if (tag == BACNET_APPLICATION_TAG_DOUBLE) {
                double_value = value->type.Real;
                apdu_len = encode_application_double(apdu, double_value);
            } else if (tag == BACNET_APPLICATION_TAG_ENUMERATED) {
                if ((value->type.Real >= 0.0F) &&
                    (value->type.Real <= 2147483000.0F)) {
                    unsigned_value = (uint32_t)value->type.Real;
                    apdu_len =
                        encode_application_enumerated(apdu, unsigned_value);
                } else {
                    apdu_len = BACNET_STATUS_ERROR;
                }
            } else {
                apdu_len = BACNET_STATUS_ERROR;
            }
            break;
#endif
#if defined(CHANNEL_DOUBLE)
        case BACNET_APPLICATION_TAG_DOUBLE:
            if (tag == BACNET_APPLICATION_TAG_BOOLEAN) {
                if (islessgreater(value->type.Double, 0.0)) {
                    boolean_value = true;
                }
                apdu_len = encode_application_boolean(apdu, boolean_value);
            } else if (tag == BACNET_APPLICATION_TAG_UNSIGNED_INT) {
                if ((value->type.Double >= 0.0) &&
                    (value->type.Double <= 2147483000.0)) {
                    unsigned_value = (uint32_t)value->type.Double;
                    apdu_len =
                        encode_application_unsigned(apdu, unsigned_value);
                } else {
                    apdu_len = BACNET_STATUS_ERROR;
                }
            } else if (tag == BACNET_APPLICATION_TAG_SIGNED_INT) {
                if ((value->type.Double >= -2147483000.0) &&
                    (value->type.Double <= 214783000.0)) {
                    signed_value = (int32_t)value->type.Double;
                    apdu_len = encode_application_signed(apdu, signed_value);
                } else {
                    apdu_len = BACNET_STATUS_ERROR;
                }
            } else if (tag == BACNET_APPLICATION_TAG_REAL) {
                if ((value->type.Double >= 3.4E-38) &&
                    (value->type.Double <= 3.4E+38)) {
                    float_value = (float)value->type.Double;
                    apdu_len = encode_application_real(apdu, float_value);
                } else {
                    apdu_len = BACNET_STATUS_ERROR;
                }
            } else if (tag == BACNET_APPLICATION_TAG_DOUBLE) {
                double_value = value->type.Double;
                apdu_len = encode_application_double(apdu, double_value);
            } else if (tag == BACNET_APPLICATION_TAG_ENUMERATED) {
                if ((value->type.Double >= 0.0) &&
                    (value->type.Double <= 2147483000.0)) {
                    unsigned_value = (uint32_t)value->type.Double;
                    apdu_len =
                        encode_application_enumerated(apdu, unsigned_value);
                } else {
                    apdu_len = BACNET_STATUS_ERROR;
                }
            } else {
                apdu_len = BACNET_STATUS_ERROR;
            }
            break;
#endif
#if defined(CHANNEL_ENUMERATED)
        case BACNET_APPLICATION_TAG_ENUMERATED:
            if (tag == BACNET_APPLICATION_TAG_BOOLEAN) {
                if (value->type.Enumerated) {
                    boolean_value = true;
                }
                apdu_len = encode_application_boolean(apdu, boolean_value);
            } else if (tag == BACNET_APPLICATION_TAG_UNSIGNED_INT) {
                unsigned_value = value->type.Enumerated;
                apdu_len = encode_application_unsigned(apdu, unsigned_value);
            } else if (tag == BACNET_APPLICATION_TAG_SIGNED_INT) {
                if (value->type.Enumerated <= 2147483647) {
                    signed_value = value->type.Enumerated;
                    apdu_len = encode_application_signed(apdu, signed_value);
                } else {
                    apdu_len = BACNET_STATUS_ERROR;
                }
            } else if (tag == BACNET_APPLICATION_TAG_REAL) {
                if (value->type.Enumerated <= 9999999) {
                    float_value = (float)value->type.Enumerated;
                    apdu_len = encode_application_real(apdu, float_value);
                } else {
                    apdu_len = BACNET_STATUS_ERROR;
                }
            } else if (tag == BACNET_APPLICATION_TAG_DOUBLE) {
                double_value = (double)value->type.Enumerated;
                apdu_len = encode_application_double(apdu, double_value);
            } else if (tag == BACNET_APPLICATION_TAG_ENUMERATED) {
                unsigned_value = value->type.Enumerated;
                apdu_len = encode_application_enumerated(apdu, unsigned_value);
            } else {
                apdu_len = BACNET_STATUS_ERROR;
            }
            break;
#endif
#if defined(CHANNEL_LIGHTING_COMMAND)
        case BACNET_APPLICATION_TAG_LIGHTING_COMMAND:
            if (tag == BACNET_APPLICATION_TAG_LIGHTING_COMMAND) {
                apdu_len = lighting_command_encode(
                    apdu, &value->type.Lighting_Command);
            } else {
                apdu_len = BACNET_STATUS_ERROR;
            }
            break;
#endif
#if defined(CHANNEL_COLOR_COMMAND)
        case BACNET_APPLICATION_TAG_COLOR_COMMAND:
            if (tag == BACNET_APPLICATION_TAG_COLOR_COMMAND) {
                apdu_len =
                    color_command_encode(apdu, &value->type.Color_Command);
            } else {
                apdu_len = BACNET_STATUS_ERROR;
            }
            break;
#endif
#if defined(CHANNEL_XY_COLOR)
        case BACNET_APPLICATION_TAG_XY_COLOR:
            if (tag == BACNET_APPLICATION_TAG_XY_COLOR) {
                apdu_len = xy_color_encode(apdu, &value->type.XY_Color);
            } else {
                apdu_len = BACNET_STATUS_ERROR;
            }
            break;
#endif
        default:
            apdu_len = BACNET_STATUS_ERROR;
            break;
    }

    return apdu_len;
}

/**
 * For a given application value, coerce the encoding, if necessary
 *
 * @param  apdu - buffer to hold the encoding, or null for length
 * @param  value - BACNET_APPLICATION_DATA_VALUE value
 * @param  tag - application tag to be coerced, if possible
 *
 * @return  number of bytes in the APDU, or BACNET_STATUS_ERROR if error.
 */
int bacnet_channel_value_coerce_data_encode(
    uint8_t *apdu,
    size_t apdu_size,
    const BACNET_CHANNEL_VALUE *value,
    BACNET_APPLICATION_TAG tag)
{
    int len;

    len = channel_value_coerce_data_encode(NULL, value, tag);
    if ((len > 0) && (len <= apdu_size)) {
        len = channel_value_coerce_data_encode(apdu, value, tag);
    } else {
        len = BACNET_STATUS_ERROR;
    }

    return len;
}
