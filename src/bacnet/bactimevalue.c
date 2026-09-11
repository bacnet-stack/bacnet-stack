/**
 * @file
 * @brief BACnetTimeValue complex data type encode and decode
 * @author Nikola Jelic <nikola.jelic@euroicc.com>
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date 2015
 * @copyright SPDX-License-Identifier: GPL-2.0-or-later WITH GCC-exception-2.0
 */
#include <stdbool.h>
#include <stdint.h>
#include <string.h> /* memcpy */
#include "bacnet/bacdcode.h"
#include "bacnet/bactimevalue.h"
#include "bacnet/bacapp.h"

static bool is_data_value_schedule_compatible(uint8_t tag)
{
    switch (tag) {
        /* Every member of the union must be listed here to allow decoding */
        case BACNET_APPLICATION_TAG_NULL:
        case BACNET_APPLICATION_TAG_BOOLEAN:
        case BACNET_APPLICATION_TAG_UNSIGNED_INT:
        case BACNET_APPLICATION_TAG_REAL:
        case BACNET_APPLICATION_TAG_ENUMERATED:
            return true;
#if BACNET_USE_SIGNED
        case BACNET_APPLICATION_TAG_SIGNED_INT:
            return true;
#endif
#if BACNET_USE_DOUBLE
        case BACNET_APPLICATION_TAG_DOUBLE:
            return true;
#endif
        default:
            return false;
    }
}

/**
 * @brief Encode the BACnetTimeValue
 *
 * From clause 21. FORMAL DESCRIPTION OF APPLICATION PROTOCOL DATA UNITS
 *
 * BACnetTimeValue ::= SEQUENCE {
 *     time Time,
 *     value ABSTRACT-SYNTAX.&Type
 *     -- any primitive datatype;
 *     -- complex types cannot be decoded
 * }
 *
 * @param apdu - buffer of data to be encoded, or NULL for length
 * @param tag_number - context tag number to be encoded
 * @param value - value to be encoded
 * @return the number of apdu bytes encoded
 */
int bacnet_time_value_encode(uint8_t *apdu, const BACNET_TIME_VALUE *value)
{
    int len;
    int apdu_len = 0;

    if (!value || !is_data_value_schedule_compatible(value->Value.tag)) {
        return BACNET_STATUS_ERROR;
    }
    len = encode_application_time(apdu, &value->Time);
    apdu_len += len;
    if (apdu) {
        apdu += len;
    }
    len = bacnet_primitive_value_encode(apdu, &value->Value);
    apdu_len += len;

    return apdu_len;
}

int bacapp_encode_time_value(uint8_t *apdu, const BACNET_TIME_VALUE *value)
{
    return bacnet_time_value_encode(apdu, value);
}

/**
 * @brief Encode the BACnetTimeValue as Context Tagged
 * as defined in clause 20.2.1 General Rules for Encoding BACnet Tags
 * @param apdu - buffer of data to be encoded, or NULL for length
 * @param tag_number - context tag number to be encoded
 * @param value - value to be encoded
 * @return the number of apdu bytes encoded
 */
int bacnet_time_value_context_encode(
    uint8_t *apdu, uint8_t tag_number, const BACNET_TIME_VALUE *value)
{
    int len;
    int apdu_len = 0;

    len = encode_opening_tag(apdu, tag_number);
    apdu_len += len;
    if (apdu) {
        apdu += len;
    }
    len = bacnet_time_value_encode(apdu, value);
    apdu_len += len;
    if (apdu) {
        apdu += len;
    }
    len = encode_closing_tag(apdu, tag_number);
    apdu_len += len;

    return apdu_len;
}

int bacapp_encode_context_time_value(
    uint8_t *apdu, uint8_t tag_number, const BACNET_TIME_VALUE *value)
{
    return bacnet_time_value_context_encode(apdu, tag_number, value);
}

/**
 * @brief Encode application data given by a pointer into the APDU.
 * @param apdu - Pointer to the buffer to encode to, or NULL for length
 * @param value - Pointer to the application data value to encode from
 * @return number of bytes encoded
 */
int bacnet_primitive_value_encode(
    uint8_t *apdu, const BACNET_PRIMITIVE_DATA_VALUE *value)
{
    int apdu_len = 0; /* total length of the apdu, return value */

    if (value) {
        switch (value->tag) {
            case BACNET_APPLICATION_TAG_NULL:
                if (apdu) {
                    apdu[0] = value->tag;
                }
                apdu_len++;
                break;
            case BACNET_APPLICATION_TAG_BOOLEAN:
                apdu_len =
                    encode_application_boolean(apdu, value->type.Boolean);
                break;
            case BACNET_APPLICATION_TAG_UNSIGNED_INT:
                apdu_len =
                    encode_application_unsigned(apdu, value->type.Unsigned_Int);
                break;
#if BACNET_USE_SIGNED
            case BACNET_APPLICATION_TAG_SIGNED_INT:
                apdu_len =
                    encode_application_signed(apdu, value->type.Signed_Int);
                break;
#endif
            case BACNET_APPLICATION_TAG_REAL:
                apdu_len = encode_application_real(apdu, value->type.Real);
                break;
#if BACNET_USE_DOUBLE
            case BACNET_APPLICATION_TAG_DOUBLE:
                apdu_len = encode_application_double(apdu, value->type.Double);
                break;
#endif
            case BACNET_APPLICATION_TAG_ENUMERATED:
                apdu_len =
                    encode_application_enumerated(apdu, value->type.Enumerated);
                break;
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
int bacnet_primitive_value_application_decode(
    const uint8_t *apdu,
    uint32_t apdu_size,
    uint8_t tag_data_type,
    uint32_t len_value_type,
    BACNET_PRIMITIVE_DATA_VALUE *value)
{
    int len = 0;

    if (value) {
        switch (tag_data_type) {
            case BACNET_APPLICATION_TAG_NULL:
                /* nothing else to do */
                break;
            case BACNET_APPLICATION_TAG_BOOLEAN:
                value->type.Boolean = decode_boolean(len_value_type);
                break;
            case BACNET_APPLICATION_TAG_UNSIGNED_INT:
                len = bacnet_unsigned_decode(
                    apdu, apdu_size, len_value_type, &value->type.Unsigned_Int);
                break;
#if BACNET_USE_SIGNED
            case BACNET_APPLICATION_TAG_SIGNED_INT:
                len = bacnet_signed_decode(
                    apdu, apdu_size, len_value_type, &value->type.Signed_Int);
                break;
#endif
            case BACNET_APPLICATION_TAG_REAL:
                len = bacnet_real_decode(
                    apdu, apdu_size, len_value_type, &(value->type.Real));
                break;
#if BACNET_USE_DOUBLE
            case BACNET_APPLICATION_TAG_DOUBLE:
                len = bacnet_double_decode(
                    apdu, apdu_size, len_value_type, &(value->type.Double));
                break;
#endif
            case BACNET_APPLICATION_TAG_ENUMERATED:
                len = bacnet_enumerated_decode(
                    apdu, apdu_size, len_value_type, &value->type.Enumerated);
                break;
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
 * @param apdu_size - number of bytes in the buffer
 * @param value - decoded value, if decoded
 *
 * @return the number of apdu bytes consumed, 0 on bad args, or
 * BACNET_STATUS_ERROR
 */
int bacnet_primitive_value_decode(
    const uint8_t *apdu, uint32_t apdu_size, BACNET_PRIMITIVE_DATA_VALUE *value)
{
    int len = 0;
    int apdu_len = 0;
    BACNET_TAG tag = { 0 };

    if (!value) {
        return 0;
    }
    len = bacnet_tag_decode(apdu, apdu_size, &tag);
    if ((len > 0) && tag.application) {
        value->tag = tag.number;
        apdu_len += len;
        len = bacnet_primitive_value_application_decode(
            &apdu[apdu_len], apdu_size - apdu_len, tag.number,
            tag.len_value_type, value);
        if ((len >= 0) && (value->tag != MAX_BACNET_APPLICATION_TAG)) {
            apdu_len += len;
        } else {
            apdu_len = BACNET_STATUS_ERROR;
        }
    } else if (apdu && (apdu_size > 0)) {
        apdu_len = BACNET_STATUS_ERROR;
    }

    return apdu_len;
}

/**
 * @brief Compare two BACnetPrimitiveDataValue values
 * @param value [in] First value to compare
 * @param test_value [in] Second value to compare
 * @return true if matching or same, false if different
 */
bool bacnet_primitive_value_same(
    const BACNET_PRIMITIVE_DATA_VALUE *value,
    const BACNET_PRIMITIVE_DATA_VALUE *test_value)
{
    bool status = false; /*return value */

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
            case BACNET_APPLICATION_TAG_NULL:
                status = true;
                break;
            case BACNET_APPLICATION_TAG_BOOLEAN:
                if (test_value->type.Boolean == value->type.Boolean) {
                    status = true;
                }
                break;
            case BACNET_APPLICATION_TAG_UNSIGNED_INT:
                if (test_value->type.Unsigned_Int == value->type.Unsigned_Int) {
                    status = true;
                }
                break;
#if BACNET_USE_SIGNED
            case BACNET_APPLICATION_TAG_SIGNED_INT:
                if (test_value->type.Signed_Int == value->type.Signed_Int) {
                    status = true;
                }
                break;
#endif
            case BACNET_APPLICATION_TAG_REAL:
                if (!islessgreater(test_value->type.Real, value->type.Real)) {
                    status = true;
                }
                break;
#if BACNET_USE_DOUBLE
            case BACNET_APPLICATION_TAG_DOUBLE:
                if (!islessgreater(
                        test_value->type.Double, value->type.Double)) {
                    status = true;
                }
                break;
#endif
            case BACNET_APPLICATION_TAG_ENUMERATED:
                if (test_value->type.Enumerated == value->type.Enumerated) {
                    status = true;
                }
                break;
            default:
                break;
        }
    }

    return status;
}

/**
 * @brief copy a BACnet primitive data value
 * @param dest - destination for the copied value
 * @param src - source value to copy
 * @return true if the copy was successful, false otherwise
 */
bool bacnet_primitive_value_copy(
    BACNET_PRIMITIVE_DATA_VALUE *dest, const BACNET_PRIMITIVE_DATA_VALUE *src)
{
    if (!dest || !src) {
        return false;
    }
    dest->tag = src->tag;
    switch (src->tag) {
        case BACNET_APPLICATION_TAG_NULL:
            /* nothing else to do */
            break;
        case BACNET_APPLICATION_TAG_BOOLEAN:
            dest->type.Boolean = src->type.Boolean;
            break;
        case BACNET_APPLICATION_TAG_UNSIGNED_INT:
            dest->type.Unsigned_Int = src->type.Unsigned_Int;
            break;
#if BACNET_USE_SIGNED
        case BACNET_APPLICATION_TAG_SIGNED_INT:
            dest->type.Signed_Int = src->type.Signed_Int;
            break;
#endif
        case BACNET_APPLICATION_TAG_REAL:
            dest->type.Real = src->type.Real;
            break;
#if BACNET_USE_DOUBLE
        case BACNET_APPLICATION_TAG_DOUBLE:
            dest->type.Double = src->type.Double;
            break;
#endif
        case BACNET_APPLICATION_TAG_ENUMERATED:
            dest->type.Enumerated = src->type.Enumerated;
            break;
        default:
            return false;
    }

    return true;
}

/**
 * @brief decode a BACnetTimeValue
 *
 * @param apdu - buffer of data to be decoded
 * @param apdu_size - number of bytes in the buffer
 * @param value - stores the decoded property value
 * @return  number of bytes decoded, or BACNET_STATUS_ERROR if errors occur
 */
int bacnet_time_value_decode(
    const uint8_t *apdu, int apdu_size, BACNET_TIME_VALUE *value)
{
    int len;
    int apdu_len = 0;

    len = bacnet_time_application_decode(
        &apdu[apdu_len], apdu_size, &value->Time);
    if (len <= 0) {
        return BACNET_STATUS_ERROR;
    }
    apdu_len += len;

    len = bacnet_primitive_value_decode(
        &apdu[apdu_len], apdu_size - apdu_len, &value->Value);
    if (len <= 0) {
        return BACNET_STATUS_ERROR;
    }
    apdu_len += len;

    return apdu_len;
}

int bacapp_decode_time_value(const uint8_t *apdu, BACNET_TIME_VALUE *value)
{
    return bacnet_time_value_decode(apdu, MAX_APDU, value);
}

/**
 * @brief decode a context encoded BACnetTimeValue
 * @param apdu - buffer of data to be decoded
 * @param apdu_size - number of bytes in the buffer
 * @param tag_number - context tag number to match
 * @param value - stores the decoded property value
 * @return number of bytes decoded, or BACNET_STATUS_ERROR if an error occurs
 */
int bacnet_time_value_context_decode(
    const uint8_t *apdu,
    int apdu_size,
    uint8_t tag_number,
    BACNET_TIME_VALUE *value)
{
    int len;
    int apdu_len = 0;

    if (bacnet_is_opening_tag_number(
            &apdu[apdu_len], apdu_size - apdu_len, tag_number, &len)) {
        apdu_len += len;
    } else {
        return BACNET_STATUS_ERROR;
    }
    len =
        bacnet_time_value_decode(&apdu[apdu_len], apdu_size - apdu_len, value);
    if (len > 0) {
        apdu_len += len;
    } else {
        return BACNET_STATUS_ERROR;
    }
    if (bacnet_is_closing_tag_number(
            &apdu[apdu_len], apdu_size - apdu_len, tag_number, &len)) {
        apdu_len += len;
    } else {
        return BACNET_STATUS_ERROR;
    }

    return apdu_len;
}

int bacapp_decode_context_time_value(
    const uint8_t *apdu, uint8_t tag_number, BACNET_TIME_VALUE *value)
{
    return bacnet_time_value_context_decode(apdu, MAX_APDU, tag_number, value);
}

/**
 * @brief decode a context encoded list of BACnetTimeValue
 * @param apdu - buffer of data to be decoded
 * @param apdu_size - number of bytes in the buffer
 * @param tag_number - context tag number to match
 * @param time_values - stores the decoded property values
 * @param max_time_values - number of values to be able to store
 * @return number of bytes decoded, or BACNET_STATUS_ERROR if an error occurs
 */
int bacnet_time_values_context_decode(
    const uint8_t *apdu,
    const int apdu_size,
    const uint8_t tag_number,
    BACNET_TIME_VALUE *time_values,
    const unsigned int max_time_values,
    unsigned int *out_count)
{
    unsigned int j;
    int len;
    int apdu_len = 0;
    unsigned int count_values = 0;
    BACNET_TIME_VALUE dummy;

    /* day-schedule [0] SEQUENCE OF BACnetTimeValue */
    if (bacnet_is_opening_tag_number(
            &apdu[apdu_len], apdu_size - apdu_len, tag_number, &len)) {
        apdu_len += len;
        while (!bacnet_is_closing_tag_number(
            &apdu[apdu_len], apdu_size - apdu_len, tag_number, &len)) {
            if (count_values < max_time_values) {
                len = bacnet_time_value_decode(
                    &apdu[apdu_len], apdu_size - apdu_len,
                    &time_values[count_values++]);
            } else {
                len = bacnet_time_value_decode(
                    &apdu[apdu_len], apdu_size - apdu_len, &dummy);
            }
            if (len < 0) {
                return BACNET_STATUS_ERROR;
            }
            apdu_len += len;
            len = 0;
        }
        /* Zeroing other values */
        for (j = count_values; j < max_time_values; j++) {
            time_values[j].Value.tag = BACNET_APPLICATION_TAG_NULL;
            time_values[j].Value.type.Unsigned_Int = 0;
            time_values[j].Time.hour = 0;
            time_values[j].Time.min = 0;
            time_values[j].Time.sec = 0;
            time_values[j].Time.hundredths = 0;
        }
        /* closing tag */
        if (len > 0) {
            apdu_len += len;
        } else {
            return BACNET_STATUS_ERROR;
        }
        if (out_count) {
            *out_count = count_values;
        }

        return apdu_len;
    }

    return BACNET_STATUS_ERROR;
}

/**
 * @brief Encodes a : [x] SEQUENCE OF BACnetTimeValue into a fixed-size buffer
 * @param apdu - buffer of data to be encoded, or NULL for buffer length
 * @param tag_number - context tag number to be encoded
 * @param value - value to be encoded
 * @return the number of apdu bytes encoded, or BACNET_STATUS_ERROR
 */
int bacnet_time_values_context_encode(
    uint8_t *apdu,
    uint8_t tag_number,
    const BACNET_TIME_VALUE *time_values,
    unsigned int max_time_values)
{
    unsigned int j;
    int apdu_len = 0;
    int len = 0;
    BACNET_TIME t0 = { 0 };

    /* day-schedule [x] SEQUENCE OF BACnetTimeValue */
    len = encode_opening_tag(apdu, tag_number);
    apdu_len += len;
    if (apdu) {
        apdu += len;
    }
    for (j = 0; j < max_time_values; j++) {
        /* Encode only non-null values (NULL,00:00:00.00) */
        if (time_values[j].Value.tag != BACNET_APPLICATION_TAG_NULL ||
            datetime_compare_time(&t0, &time_values[j].Time) != 0) {
            len = bacnet_time_value_encode(apdu, &time_values[j]);
            if (len < 0) {
                return BACNET_STATUS_ERROR;
            }
            apdu_len += len;
            if (apdu) {
                apdu += len;
            }
        }
    }
    /* close tag */
    len = encode_closing_tag(apdu, tag_number);
    apdu_len += len;

    return apdu_len;
}

/**
 * * @brief Compare two BACnetTimeValue values
 * @param a [in] First value to compare
 * @param b [in] Second value to compare
 * @return true if equal, false if not equal
 */
bool bacnet_time_value_same(
    const BACNET_TIME_VALUE *a, const BACNET_TIME_VALUE *b)
{
    if (a == NULL || b == NULL) {
        return false;
    }
    if (a->Time.hour != b->Time.hour) {
        return false;
    }
    if (a->Time.min != b->Time.min) {
        return false;
    }
    if (a->Time.sec != b->Time.sec) {
        return false;
    }
    if (a->Time.hundredths != b->Time.hundredths) {
        return false;
    }

    return bacnet_primitive_value_same(&a->Value, &b->Value);
}

/**
 * @brief Copy a BACnetTimeValue value
 * @param dest [in] destination to copy to
 * @param src [in] source to copy from
 */
void bacnet_time_value_copy(
    BACNET_TIME_VALUE *dest, const BACNET_TIME_VALUE *src)
{
    if (dest == NULL || src == NULL) {
        return;
    }
    memcpy(dest, src, sizeof(BACNET_TIME_VALUE));
}
