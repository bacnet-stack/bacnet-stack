/**
 * @file
 * @brief BACnetTimeStamp service encode and decode
 * @author John Minack <minack@users.sourceforge.net>
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date 2008
 * @copyright SPDX-License-Identifier: GPL-2.0-or-later WITH GCC-exception-2.0
 */
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include "bacnet/bacapp.h"
#include "bacnet/datetime.h"
#include "bacnet/timestamp.h"

/** Set the sequence number in a timestamp structure.
 *
 * @param dest  Pointer to the destination time stamp structure.
 * @param sequenceNum  Sequence number to be set into the time stamp.
 */
void bacapp_timestamp_sequence_set(BACNET_TIMESTAMP *dest, uint16_t sequenceNum)
{
    if (dest) {
        dest->tag = TIME_STAMP_SEQUENCE;
        dest->value.sequenceNum = sequenceNum;
    }
}

/** Set a timestamp structure with the value given
 * from a time structure.
 *
 * @param dest  Pointer to the destination time stamp structure.
 * @param btime  Pointer to the BACnet time structure.
 */
void bacapp_timestamp_time_set(BACNET_TIMESTAMP *dest, const BACNET_TIME *btime)
{
    if (dest && btime) {
        dest->tag = TIME_STAMP_TIME;
        datetime_copy_time(&dest->value.time, btime);
    }
}

/** Set a timestamp structure with the value given
 * from a date/time structure.
 *
 * @param dest  Pointer to the destination time stamp structure.
 * @param bdateTime  Pointer to the BACnet date/time structure.
 */
void bacapp_timestamp_datetime_set(
    BACNET_TIMESTAMP *dest, const BACNET_DATE_TIME *bdateTime)
{
    if (dest && bdateTime) {
        dest->tag = TIME_STAMP_DATETIME;
        datetime_copy(&dest->value.dateTime, bdateTime);
    }
}

/** Copy a timestamp depending of the tag it holds.
 *
 * @param dest  Pointer to the destination time stamp structure.
 * @param src   Pointer to the source time stamp structure.
 */
void bacapp_timestamp_copy(BACNET_TIMESTAMP *dest, const BACNET_TIMESTAMP *src)
{
    if (dest && src) {
        dest->tag = src->tag;
        switch (src->tag) {
            case TIME_STAMP_TIME:
                datetime_copy_time(&dest->value.time, &src->value.time);
                break;
            case TIME_STAMP_SEQUENCE:
                dest->value.sequenceNum = src->value.sequenceNum;
                break;
            case TIME_STAMP_DATETIME:
                datetime_copy(&dest->value.dateTime, &src->value.dateTime);
                break;
            default:
                break;
        }
    }
}

/**
 * @brief Compare two complex data values
 * @param value1 - complex data value 1 structure
 * @param value2 - complex data value 2 structure
 * @return true if the two complex data values are the same
 */
bool bacapp_timestamp_same(
    const BACNET_TIMESTAMP *value1, const BACNET_TIMESTAMP *value2)
{
    bool status = false;

    if (value1 && value2) {
        if (value1->tag == value2->tag) {
            switch (value1->tag) {
                case TIME_STAMP_TIME:
                    if (datetime_compare_time(
                            &value1->value.time, &value2->value.time) == 0) {
                        status = true;
                    }
                    break;
                case TIME_STAMP_SEQUENCE:
                    if (value1->value.sequenceNum ==
                        value2->value.sequenceNum) {
                        status = true;
                    }
                    break;
                case TIME_STAMP_DATETIME:
                    if (datetime_compare(
                            &value1->value.dateTime, &value2->value.dateTime) ==
                        0) {
                        status = true;
                    }
                    break;
                default:
                    break;
            }
        }
    }

    return status;
}

/**
 * @brief Encode a time stamp.
 *
 *  BACnetTimeStamp ::= CHOICE {
 *      time [0] Time, -- deprecated in version 1 revision 21
 *      sequence-number [1] Unsigned (0..65535),
 *      datetime [2] BACnetDateTime
 *  }
 *
 * @param apdu  Pointer to the APDU buffer.
 * @param value  Pointer to the variable that holds
 *               the time stamp values to be encoded.
 *
 * @return Bytes encoded or 0 on error.
 */
int bacapp_encode_timestamp(uint8_t *apdu, const BACNET_TIMESTAMP *value)
{
    int len = 0; /* length of each encoding */

    if (value) {
        switch (value->tag) {
            case TIME_STAMP_TIME:
                len = encode_context_time(apdu, value->tag, &value->value.time);
                break;

            case TIME_STAMP_SEQUENCE:
                len = encode_context_unsigned(
                    apdu, value->tag, value->value.sequenceNum);
                break;

            case TIME_STAMP_DATETIME:
                len = bacapp_encode_context_datetime(
                    apdu, value->tag, &value->value.dateTime);
                break;

            default:
                len = 0;
                break;
        }
    }

    return len;
}

/** Encode a time stamp for the given tag
 * number, using an opening and closing tag.
 *
 * @param apdu  Pointer to the APDU buffer.
 * @param tag_number  The tag number that shall
 *                    hold the time stamp.
 * @param value  Pointer to the variable that holds
 *               the time stamp values to be encoded.
 *
 * @return Bytes encoded or 0 on error.
 */
int bacapp_encode_context_timestamp(
    uint8_t *apdu, uint8_t tag_number, const BACNET_TIMESTAMP *value)
{
    int len = 0; /* length of each encoding */
    int apdu_len = 0;

    if (value) {
        len = encode_opening_tag(apdu, tag_number);
        apdu_len += len;
        if (apdu) {
            apdu += len;
        }
        len = bacapp_encode_timestamp(apdu, value);
        apdu_len += len;
        if (apdu) {
            apdu += len;
        }
        len = encode_closing_tag(apdu, tag_number);
        apdu_len += len;
    }
    return apdu_len;
}

/**
 * @brief Decode a time stamp from the given buffer.
 *
 *  BACnetTimeStamp ::= CHOICE {
 *      time [0] Time, -- deprecated in version 1 revision 21
 *      sequence-number [1] Unsigned (0..65535),
 *      datetime [2] BACnetDateTime
 *  }
 *
 * @param apdu  Pointer to the APDU buffer.
 * @param apdu_size - the APDU buffer length
 * @param value  Pointer to the variable that shall
 *               take the time stamp values.
 * @return number of bytes decoded, or BACNET_STATUS_ERROR if an error occurs
 */
int bacnet_timestamp_decode(
    const uint8_t *apdu, uint32_t apdu_size, BACNET_TIMESTAMP *value)
{
    int len = 0;
    int apdu_len = 0;
    BACNET_TAG tag = { 0 };
    BACNET_UNSIGNED_INTEGER unsigned_value = 0;
    BACNET_TIME *btime = NULL;
    BACNET_DATE_TIME *bdatetime = NULL;

    if (!apdu) {
        return BACNET_STATUS_ERROR;
    }
    len = bacnet_tag_decode(&apdu[apdu_len], apdu_size - apdu_len, &tag);
    if (len <= 0) {
        return BACNET_STATUS_ERROR;
    }
    if (value) {
        value->tag = tag.number;
    }
    switch (tag.number) {
        case TIME_STAMP_TIME:
            if (value) {
                btime = &value->value.time;
            }
            len = bacnet_time_context_decode(
                &apdu[apdu_len], apdu_size - apdu_len, tag.number, btime);
            if (len <= 0) {
                return BACNET_STATUS_ERROR;
            }
            apdu_len += len;
            break;

        case TIME_STAMP_SEQUENCE:
            len = bacnet_unsigned_context_decode(
                &apdu[apdu_len], apdu_size - apdu_len, tag.number,
                &unsigned_value);
            if (len <= 0) {
                return BACNET_STATUS_ERROR;
            }
            apdu_len += len;
            if (unsigned_value <= UINT16_MAX) {
                if (value) {
                    value->value.sequenceNum = (uint16_t)unsigned_value;
                }
            } else {
                return BACNET_STATUS_ERROR;
            }
            break;
        case TIME_STAMP_DATETIME:
            if (value) {
                bdatetime = &value->value.dateTime;
            }
            len = bacnet_datetime_context_decode(
                &apdu[apdu_len], apdu_size - apdu_len, tag.number, bdatetime);
            if (len <= 0) {
                return BACNET_STATUS_ERROR;
            }
            apdu_len += len;
            break;
        default:
            return BACNET_STATUS_ERROR;
    }

    return apdu_len;
}

#if defined(BACNET_STACK_DEPRECATED_DISABLE)
/**
 * @brief Decode a time stamp from the given buffer.
 * @param apdu  Pointer to the APDU buffer.
 * @param value  Pointer to the variable that shall
 *               take the time stamp values.
 * @return number of bytes decoded, or BACNET_STATUS_ERROR if an error occurs
 * @deprecated use bacnet_timestamp_decode() instead
 */
int bacapp_decode_timestamp(const uint8_t *apdu, BACNET_TIMESTAMP *value)
{
    return bacnet_timestamp_decode(apdu, MAX_APDU, value);
}
#endif

/**
 * @brief Decode a time stamp and check for opening and closing tags.
 * @param apdu  Pointer to the APDU buffer.
 * @param apdu_size - the APDU buffer length
 * @param tag_number  The tag number that shall
 *                    hold the time stamp.
 * @param value  Pointer to the variable that shall
 *               take the time stamp values.
 * @return  number of bytes decoded, zero if tag mismatch,
 * or #BACNET_STATUS_ERROR (-1) if malformed
 */
int bacnet_timestamp_context_decode(
    const uint8_t *apdu,
    uint32_t apdu_size,
    uint8_t tag_number,
    BACNET_TIMESTAMP *value)
{
    int len = 0;
    int apdu_len = 0;

    if (!apdu) {
        return BACNET_STATUS_ERROR;
    }
    if (!bacnet_is_opening_tag_number(
            &apdu[apdu_len], apdu_size - apdu_len, tag_number, &len)) {
        return 0;
    }
    apdu_len += len;
    len = bacnet_timestamp_decode(&apdu[apdu_len], apdu_size - apdu_len, value);
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

#if defined(BACNET_STACK_DEPRECATED_DISABLE)
/**
 * @brief Decode a time stamp and check for opening and closing tags.
 * @param apdu  Pointer to the APDU buffer.
 * @param tag_number  The tag number that shall
 *                    hold the time stamp.
 * @param value  Pointer to the variable that shall
 *               take the time stamp values.
 * @return number of bytes decoded, or BACNET_STATUS_ERROR if an error occurs
 * @deprecated use bacnet_timestamp_context_decode() instead
 */
int bacapp_decode_context_timestamp(
    const uint8_t *apdu, uint8_t tag_number, BACNET_TIMESTAMP *value)
{
    const uint32_t apdu_size = MAX_APDU;
    int len;

    len = bacnet_timestamp_context_decode(apdu, apdu_size, tag_number, value);
    if (len <= 0) {
        len = BACNET_STATUS_ERROR;
    }

    return len;
}
#endif

/**
 * @brief Parse an ascii string for the timestamp
 * @param btime - #BACNET_TIME structure
 * @param ascii - C string with timestamp formatted as one of the following:
 *  time - 23:59:59.99 or 23:59:59 or 23:59 or
 *  datetime - 2021/12/31 or 2021/12/31-23:59:59.99 or 2021/12/31-23:59:59 or
 *             2021/12/31-23:59 2021/12/31-23 or
 *  sequence number: 1234
 * @return true if parsed successfully
 */
bool bacapp_timestamp_init_ascii(BACNET_TIMESTAMP *timestamp, const char *ascii)
{
    bool status = false;
    int hour, min, sec, hundredths;
    int year, month, day;
    int sequence;
    int count = 0;

    count = sscanf(ascii, "%3d:%3d:%3d.%3d", &hour, &min, &sec, &hundredths);
    if (count == 4) {
        timestamp->tag = TIME_STAMP_TIME;
        timestamp->value.time.hour = (uint8_t)hour;
        timestamp->value.time.min = (uint8_t)min;
        timestamp->value.time.sec = (uint8_t)sec;
        timestamp->value.time.hundredths = (uint8_t)hundredths;
        status = true;
    } else if (count == 3) {
        timestamp->tag = TIME_STAMP_TIME;
        timestamp->value.time.hour = (uint8_t)hour;
        timestamp->value.time.min = (uint8_t)min;
        timestamp->value.time.sec = (uint8_t)sec;
        timestamp->value.time.hundredths = 0;
        status = true;
    } else if (count == 2) {
        timestamp->tag = TIME_STAMP_TIME;
        timestamp->value.time.hour = (uint8_t)hour;
        timestamp->value.time.min = (uint8_t)min;
        timestamp->value.time.sec = 0;
        timestamp->value.time.hundredths = 0;
        status = true;
    }
    if (!status) {
        count = sscanf(
            ascii, "%4d/%3d/%3d-%3d:%3d:%3d.%3d", &year, &month, &day, &hour,
            &min, &sec, &hundredths);
        if (count >= 3) {
            timestamp->tag = TIME_STAMP_DATETIME;
            datetime_set_date(
                &timestamp->value.dateTime.date, (uint16_t)year, (uint8_t)month,
                (uint8_t)day);
            if (count >= 7) {
                datetime_set_time(
                    &timestamp->value.dateTime.time, (uint8_t)hour,
                    (uint8_t)min, (uint8_t)sec, (uint8_t)hundredths);
            } else if (count >= 6) {
                datetime_set_time(
                    &timestamp->value.dateTime.time, (uint8_t)hour,
                    (uint8_t)min, (uint8_t)sec, 0);
            } else if (count >= 5) {
                datetime_set_time(
                    &timestamp->value.dateTime.time, (uint8_t)hour,
                    (uint8_t)min, 0, 0);
            } else if (count >= 4) {
                datetime_set_time(
                    &timestamp->value.dateTime.time, (uint8_t)hour, 0, 0, 0);
            } else {
                datetime_set_time(&timestamp->value.dateTime.time, 0, 0, 0, 0);
            }
            status = true;
        }
    }
    if (!status) {
        count = sscanf(ascii, "%5d", &sequence);
        if (count == 1) {
            timestamp->tag = TIME_STAMP_SEQUENCE;
            timestamp->value.sequenceNum = (uint16_t)sequence;
            status = true;
        }
    }

    return status;
}

/**
 * @brief Print the timestamp to a string
 * @param str - pointer to the string, or NULL for length only
 * @param str_size - size of the string, or 0 for length only
 * @param ts - pointer to the timestamp
 * @return number of characters printed
 */
int bacapp_timestamp_to_ascii(
    char *str, size_t str_size, const BACNET_TIMESTAMP *timestamp)
{
    int str_len = 0;

    if (!timestamp) {
        return 0;
    }
    switch (timestamp->tag) {
        case TIME_STAMP_TIME:
            /* 23:59:59.99 */
            str_len = snprintf(
                str, str_size, "%02u:%02u:%02u.%02u",
                (unsigned)timestamp->value.time.hour,
                (unsigned)timestamp->value.time.min,
                (unsigned)timestamp->value.time.sec,
                (unsigned)timestamp->value.time.hundredths);
            break;
        case TIME_STAMP_SEQUENCE:
            /* 65535 */
            str_len = snprintf(
                str, str_size, "%u", (unsigned)timestamp->value.sequenceNum);
            break;
        case TIME_STAMP_DATETIME:
            if (datetime_wildcard_year(&timestamp->value.dateTime.date)) {
                /* 255/12/31-23:59:59.99 */
                str_len = snprintf(
                    str, str_size, "255/%02u/%02u-%02u:%02u:%02u.%02u",
                    (unsigned)timestamp->value.dateTime.date.month,
                    (unsigned)timestamp->value.dateTime.date.day,
                    (unsigned)timestamp->value.dateTime.time.hour,
                    (unsigned)timestamp->value.dateTime.time.min,
                    (unsigned)timestamp->value.dateTime.time.sec,
                    (unsigned)timestamp->value.dateTime.time.hundredths);
            } else {
                /* 2021/12/31-23:59:59.99 */
                str_len = snprintf(
                    str, str_size, "%04u/%02u/%02u-%02u:%02u:%02u.%02u",
                    (unsigned)timestamp->value.dateTime.date.year,
                    (unsigned)timestamp->value.dateTime.date.month,
                    (unsigned)timestamp->value.dateTime.date.day,
                    (unsigned)timestamp->value.dateTime.time.hour,
                    (unsigned)timestamp->value.dateTime.time.min,
                    (unsigned)timestamp->value.dateTime.time.sec,
                    (unsigned)timestamp->value.dateTime.time.hundredths);
            }
            break;
        default:
            break;
    }

    return str_len;
}
