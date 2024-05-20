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
            return true;
#if defined(BACAPP_BOOLEAN)
        case BACNET_APPLICATION_TAG_BOOLEAN:
            return true;
#endif
#if defined(BACAPP_UNSIGNED)
        case BACNET_APPLICATION_TAG_UNSIGNED_INT:
            return true;
#endif
#if defined(BACAPP_SIGNED)
        case BACNET_APPLICATION_TAG_SIGNED_INT:
            return true;
#endif
#if defined(BACAPP_REAL)
        case BACNET_APPLICATION_TAG_REAL:
            return true;
#endif
#if defined(BACAPP_DOUBLE)
        case BACNET_APPLICATION_TAG_DOUBLE:
            return true;
#endif
#if defined(BACAPP_ENUMERATED)
        case BACNET_APPLICATION_TAG_ENUMERATED:
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
int bacnet_time_value_encode(uint8_t *apdu, BACNET_TIME_VALUE *value)
{
    int len;
    int apdu_len = 0;
    BACNET_APPLICATION_DATA_VALUE data;

    if (!value || !is_data_value_schedule_compatible(value->Value.tag)) {
        return BACNET_STATUS_ERROR;
    }
    len = encode_application_time(apdu, &value->Time);
    apdu_len += len;
    if (apdu) {
        apdu += len;
    }
    bacnet_primitive_to_application_data_value(&data, &value->Value);
    len = bacapp_encode_application_data(apdu, &data);
    apdu_len += len;

    return apdu_len;
}

int bacapp_encode_time_value(uint8_t *apdu, BACNET_TIME_VALUE *value)
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
    uint8_t *apdu, uint8_t tag_number, BACNET_TIME_VALUE *value)
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
    uint8_t *apdu, uint8_t tag_number, BACNET_TIME_VALUE *value)
{
    return bacnet_time_value_context_encode(apdu, tag_number, value);
}

/**
 * @brief Convert primitive value from application data value
 * @param dest Primitive Data Value
 * @param src Application Data Value
 * @return BACNET_STATUS_OK, or BACNET_STATUS_ERROR if an error occurs
 */
int bacnet_application_to_primitive_data_value(
    struct BACnet_Primitive_Data_Value *dest,
    const struct BACnet_Application_Data_Value *src)
{
    /* make sure the value passed is valid */
    if (!src || !dest || !is_data_value_schedule_compatible(src->tag)) {
        return BACNET_STATUS_ERROR;
    }
    memset(dest, 0, sizeof(struct BACnet_Primitive_Data_Value));
    dest->tag = src->tag;
    memcpy(&dest->type, &src->type, sizeof(dest->type));

    return BACNET_STATUS_OK;
}

/**
 * @brief Convert primitive value to application data value
 * @param dest Application Data Value
 * @param src Primitive Data Value
 * @return BACNET_STATUS_OK, or BACNET_STATUS_ERROR if an error occurs
 */
int bacnet_primitive_to_application_data_value(
    struct BACnet_Application_Data_Value *dest,
    const struct BACnet_Primitive_Data_Value *src)
{
    /* make sure the value passed is valid */
    if (!dest || !src) {
        return BACNET_STATUS_ERROR;
    }
    memset(dest, 0, sizeof(struct BACnet_Application_Data_Value));
    dest->tag = src->tag;
    memcpy(&dest->type, &src->type, sizeof(src->type));

    return BACNET_STATUS_OK;
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
    uint8_t *apdu, int apdu_size, BACNET_TIME_VALUE *value)
{
    int len;
    int apdu_len = 0;
    BACNET_APPLICATION_DATA_VALUE full_data_value = { 0 };

    len = bacnet_time_application_decode(
        &apdu[apdu_len], apdu_size, &value->Time);
    if (len <= 0) {
        return BACNET_STATUS_ERROR;
    }
    apdu_len += len;

    len = bacapp_decode_application_data(
        &apdu[apdu_len], apdu_size - apdu_len, &full_data_value);
    if (len <= 0) {
        return -1;
    }
    if (BACNET_STATUS_OK !=
        bacnet_application_to_primitive_data_value(
            &value->Value, &full_data_value)) {
        return BACNET_STATUS_ERROR;
    }
    apdu_len += len;

    return apdu_len;
}

int bacapp_decode_time_value(uint8_t *apdu, BACNET_TIME_VALUE *value)
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
    uint8_t *apdu, int apdu_size, uint8_t tag_number, BACNET_TIME_VALUE *value)
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
    uint8_t *apdu, uint8_t tag_number, BACNET_TIME_VALUE *value)
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
    uint8_t *apdu,
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
    BACNET_TIME_VALUE *time_values,
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
