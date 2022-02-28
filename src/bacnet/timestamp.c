/*####COPYRIGHTBEGIN####
 -------------------------------------------
 Copyright (C) 2008 John Minack

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
#include <assert.h>
#include "bacnet/bacapp.h"
#include "bacnet/timestamp.h"

/** @file timestamp.c  Encode/Decode BACnet Timestamps  */

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
 * @param btime  Pointer to the BACNet time structure.
 */
void bacapp_timestamp_time_set(BACNET_TIMESTAMP *dest, BACNET_TIME *btime)
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
 * @param bdateTime  Pointer to the BACNet date/time structure.
 */
void bacapp_timestamp_datetime_set(
    BACNET_TIMESTAMP *dest, BACNET_DATE_TIME *bdateTime)
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
void bacapp_timestamp_copy(BACNET_TIMESTAMP *dest, BACNET_TIMESTAMP *src)
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

/** Encode a time stamp.
 *
 * @param apdu  Pointer to the APDU buffer.
 * @param value  Pointer to the variable that holds
 *               the time stamp values to be encoded.
 *
 * @return Bytes encoded or 0 on error.
 */
int bacapp_encode_timestamp(uint8_t *apdu, BACNET_TIMESTAMP *value)
{
    int len = 0; /* length of each encoding */

    if (value && apdu) {
        switch (value->tag) {
            case TIME_STAMP_TIME:
                len = encode_context_time(&apdu[0], 0, &value->value.time);
                break;

            case TIME_STAMP_SEQUENCE:
                len = encode_context_unsigned(
                    &apdu[0], 1, value->value.sequenceNum);
                break;

            case TIME_STAMP_DATETIME:
                len = bacapp_encode_context_datetime(
                    &apdu[0], 2, &value->value.dateTime);
                break;

            default:
                len = 0;
                assert(len);
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
    uint8_t *apdu, uint8_t tag_number, BACNET_TIMESTAMP *value)
{
    int len = 0; /* length of each encoding */
    int apdu_len = 0;

    if (value && apdu) {
        len = encode_opening_tag(&apdu[apdu_len], tag_number);
        apdu_len += len;
        len = bacapp_encode_timestamp(&apdu[apdu_len], value);
        apdu_len += len;
        len = encode_closing_tag(&apdu[apdu_len], tag_number);
        apdu_len += len;
    }
    return apdu_len;
}

/** Decode a time stamp from the given buffer.
 *
 * @param apdu  Pointer to the APDU buffer.
 * @param value  Pointer to the variable that shall
 *               take the time stamp values.
 *
 * @return Bytes decoded or -1 on error.
 */
int bacapp_decode_timestamp(uint8_t *apdu, BACNET_TIMESTAMP *value)
{
    int len = 0;
    int section_len;
    uint32_t len_value_type;
    BACNET_UNSIGNED_INTEGER unsigned_value;

    if (apdu && value) {
        section_len = decode_tag_number_and_value(
            &apdu[len], &value->tag, &len_value_type);

        if (-1 == section_len) {
            return -1;
        }
        switch (value->tag) {
            case TIME_STAMP_TIME:
                if ((section_len = decode_context_bacnet_time(&apdu[len],
                         TIME_STAMP_TIME, &value->value.time)) == -1) {
                    return -1;
                } else {
                    len += section_len;
                }
                break;

            case TIME_STAMP_SEQUENCE:
                if ((section_len = decode_context_unsigned(&apdu[len],
                         TIME_STAMP_SEQUENCE, &unsigned_value)) == -1) {
                    return -1;
                } else {
                    if (unsigned_value <= 0xffff) {
                        len += section_len;
                        value->value.sequenceNum = (uint16_t)unsigned_value;
                    } else {
                        return -1;
                    }
                }
                break;

            case TIME_STAMP_DATETIME:
                if ((section_len = bacapp_decode_context_datetime(&apdu[len],
                         TIME_STAMP_DATETIME, &value->value.dateTime)) == -1) {
                    return -1;
                } else {
                    len += section_len;
                }
                break;

            default:
                return -1;
        }
    }

    return len;
}

/** Decode a time stamp and check for
 * opening and closing tags.
 *
 * @param apdu  Pointer to the APDU buffer.
 * @param tag_number  The tag number that shall
 *                    hold the time stamp.
 * @param value  Pointer to the variable that shall
 *               take the time stamp values.
 *
 * @return Bytes decoded or -1 on error.
 */
int bacapp_decode_context_timestamp(
    uint8_t *apdu, uint8_t tag_number, BACNET_TIMESTAMP *value)
{
    int len = 0;
    int section_len;

    if (decode_is_opening_tag_number(&apdu[len], tag_number)) {
        len++;
        section_len = bacapp_decode_timestamp(&apdu[len], value);
        if (section_len > 0) {
            len += section_len;
            if (decode_is_closing_tag_number(&apdu[len], tag_number)) {
                len++;
            } else {
                return -1;
            }
        }
    }
    return len;
}

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

    count = sscanf(
        ascii, "%3d:%3d:%3d.%3d", &hour, &min, &sec, &hundredths);
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
        count =
            sscanf(ascii, "%4d/%3d/%3d-%3d:%3d:%3d.%3d",
            &year, &month, &day, &hour, &min, &sec, &hundredths);
        if (count >= 3) {
            timestamp->tag = TIME_STAMP_DATETIME;
            datetime_set_date(&timestamp->value.dateTime.date,
                (uint16_t)year, (uint8_t)month, (uint8_t)day);
            if (count >= 7) {
                datetime_set_time(&timestamp->value.dateTime.time,
                    (uint8_t)hour, (uint8_t)min, (uint8_t)sec,
                    (uint8_t)hundredths);
            } else if (count >= 6) {
                datetime_set_time(&timestamp->value.dateTime.time,
                    (uint8_t)hour, (uint8_t)min, (uint8_t)sec, 0);
            } else if (count >= 5) {
                datetime_set_time(&timestamp->value.dateTime.time,
                    (uint8_t)hour, (uint8_t)min, 0, 0);
            } else if (count >= 4) {
                datetime_set_time(&timestamp->value.dateTime.time,
                    (uint8_t)hour, 0, 0, 0);
            } else {
                datetime_set_time(&timestamp->value.dateTime.time, 0, 0, 0, 0);
            }
            status = true;
        }
    }
    if (!status) {
        count = sscanf(ascii, "%4d", &sequence);
        if (count == 1) {
            timestamp->tag = TIME_STAMP_DATETIME;
            timestamp->value.sequenceNum = (uint16_t)sequence;
            status = true;
        }
    }

    return status;
}
