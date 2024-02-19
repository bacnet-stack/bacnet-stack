/**************************************************************************
*
* Copyright (C) 2008 John Minack
*
* Permission is hereby granted, free of charge, to any person obtaining
* a copy of this software and associated documentation files (the
* "Software"), to deal in the Software without restriction, including
* without limitation the rights to use, copy, modify, merge, publish,
* distribute, sublicense, and/or sell copies of the Software, and to
* permit persons to whom the Software is furnished to do so, subject to
* the following conditions:
*
* The above copyright notice and this permission notice shall be included
* in all copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*********************************************************************/
#ifndef _TIMESTAMP_H_
#define _TIMESTAMP_H_
#include <stdint.h>
#include "bacnet/bacnet_stack_exports.h"
#include "bacnet/basic/sys/platform.h"
#include "bacnet/bacenum.h"
#include "bacnet/bacdcode.h"

typedef enum {
    TIME_STAMP_TIME = 0,
    TIME_STAMP_SEQUENCE = 1,
    TIME_STAMP_DATETIME = 2
} BACNET_TIMESTAMP_TAG;

typedef uint8_t TYPE_BACNET_TIMESTAMP_TYPE;

typedef struct BACnet_Timestamp {
    TYPE_BACNET_TIMESTAMP_TYPE tag;
    union {
        BACNET_TIME time;
        uint16_t sequenceNum;
        BACNET_DATE_TIME dateTime;
    } value;
} BACNET_TIMESTAMP;

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

    BACNET_STACK_EXPORT
    void bacapp_timestamp_sequence_set(
        BACNET_TIMESTAMP * dest,
        uint16_t sequenceNum);

    BACNET_STACK_EXPORT
    void bacapp_timestamp_time_set(
        BACNET_TIMESTAMP * dest,
        BACNET_TIME *btime);

    BACNET_STACK_EXPORT
    void bacapp_timestamp_datetime_set(
        BACNET_TIMESTAMP * dest,
        BACNET_DATE_TIME * bdateTime);

    BACNET_STACK_EXPORT
    void bacapp_timestamp_copy(
        BACNET_TIMESTAMP * dest,
        BACNET_TIMESTAMP * src);

    BACNET_STACK_EXPORT
    bool bacapp_timestamp_same(
        BACNET_TIMESTAMP *value1, 
        BACNET_TIMESTAMP *value2);

    BACNET_STACK_EXPORT
    int bacapp_encode_timestamp(
        uint8_t * apdu,
        BACNET_TIMESTAMP * value);
    BACNET_STACK_EXPORT
    int bacapp_encode_context_timestamp(
        uint8_t * apdu,
        uint8_t tag_number,
        BACNET_TIMESTAMP * value);

    BACNET_STACK_EXPORT
    int bacnet_timestamp_decode(
        uint8_t * apdu,
        uint32_t apdu_size,
        BACNET_TIMESTAMP * value);
    BACNET_STACK_EXPORT
    int bacnet_timestamp_context_decode(
        uint8_t * apdu,
        uint32_t apdu_size,
        uint8_t tag_number,
        BACNET_TIMESTAMP * value);

    BACNET_STACK_DEPRECATED("Use bacnet_timestamp_decode() instead")
    BACNET_STACK_EXPORT
    int bacapp_decode_timestamp(
        uint8_t * apdu,
        BACNET_TIMESTAMP * value);
    BACNET_STACK_DEPRECATED("Use bacnet_timestamp_context_decode() instead")
    BACNET_STACK_EXPORT
    int bacapp_decode_context_timestamp(
        uint8_t * apdu,
        uint8_t tag_number,
        BACNET_TIMESTAMP * value);

    BACNET_STACK_EXPORT
    bool bacapp_timestamp_init_ascii(
        BACNET_TIMESTAMP *timestamp,
        const char *ascii);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif
