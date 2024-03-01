/**************************************************************************
*
* Copyright (C) 2015 Nikola Jelic
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

#ifndef _BAC_TIME_VALUE_H_
#define _BAC_TIME_VALUE_H_

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "bacnet/bacnet_stack_exports.h"
#include "bacnet/basic/sys/platform.h"
#include "bacnet/bacdef.h"
#include "bacnet/bacenum.h"
#include "bacnet/datetime.h"

/**
 * Smaller version of BACnet_Application_Data_Value used in BACnetTimeValue
 *
 * This must be a separate struct to avoid recursive structure.
 * Keeping it small also helps keep the size of BACNET_APPLICATION_DATA_VALUE
 * small. Besides, schedule can't contain complex types.
 */
typedef struct BACnet_Primitive_Data_Value {
    uint8_t tag;        /* application tag data type */
    union {
        /*
         * ATTENTION! If a new type is added here, update
         * `is_data_value_schedule_compatible()` in bactimevalue.c!
         */

        /* NULL - not needed as it is encoded in the tag alone */
#if defined (BACAPP_BOOLEAN)
        bool Boolean;
#endif
#if defined (BACAPP_UNSIGNED)
        BACNET_UNSIGNED_INTEGER Unsigned_Int;
#endif
#if defined (BACAPP_SIGNED)
        int32_t Signed_Int;
#endif
#if defined (BACAPP_REAL)
        float Real;
#endif
#if defined (BACAPP_DOUBLE)
        double Double;
#endif
#if defined (BACAPP_ENUMERATED)
        uint32_t Enumerated;
#endif
    } type;
} BACNET_PRIMITIVE_DATA_VALUE;

typedef struct BACnet_Time_Value {
    BACNET_TIME Time;
    BACNET_PRIMITIVE_DATA_VALUE Value;
} BACNET_TIME_VALUE;


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */
    struct BACnet_Application_Data_Value;

    /** returns 0 if OK, -1 on error */
    BACNET_STACK_EXPORT
    int bacnet_application_to_primitive_data_value(BACNET_PRIMITIVE_DATA_VALUE * dest,
        const struct BACnet_Application_Data_Value * src);

    /** returns 0 if OK, -1 on error */
    BACNET_STACK_EXPORT
    int bacnet_primitive_to_application_data_value(
        struct BACnet_Application_Data_Value * dest,
        const BACNET_PRIMITIVE_DATA_VALUE * src);

    BACNET_STACK_EXPORT
    int bacnet_time_value_encode(uint8_t * apdu,
        BACNET_TIME_VALUE * value);

    BACNET_STACK_DEPRECATED("Use bacnet_time_value_encode() instead")
    BACNET_STACK_EXPORT
    int bacapp_encode_time_value(uint8_t *apdu, BACNET_TIME_VALUE *value);

    BACNET_STACK_EXPORT
    int bacnet_time_value_context_encode(uint8_t * apdu,
        uint8_t tag_number,
        BACNET_TIME_VALUE * value);

    BACNET_STACK_DEPRECATED("Use bacnet_time_value_context_encode() instead")
    BACNET_STACK_EXPORT
    int bacapp_encode_context_time_value(uint8_t *apdu, uint8_t tag_number, BACNET_TIME_VALUE *value);

    BACNET_STACK_DEPRECATED("Use bacnet_time_value_decode() instead")
    BACNET_STACK_EXPORT
    int bacapp_decode_time_value(uint8_t * apdu,
        BACNET_TIME_VALUE * value);

    BACNET_STACK_EXPORT
    int bacnet_time_value_decode(uint8_t *apdu, int max_apdu_len,
        BACNET_TIME_VALUE *value);

    BACNET_STACK_DEPRECATED("Use bacnet_time_value_context_decode() instead")
    BACNET_STACK_EXPORT
    int bacapp_decode_context_time_value(uint8_t * apdu,
        uint8_t tag_number,
        BACNET_TIME_VALUE * value);

    BACNET_STACK_EXPORT
    int bacnet_time_value_context_decode(uint8_t *apdu, int max_apdu_len,
        uint8_t tag_number, BACNET_TIME_VALUE *value);

    /**
     * Decode array of time-values wrapped in a context tag
     * @param apdu
     * @param max_apdu_len
     * @param tag_number - number expected in the context tag; 0 used for DailySchedule
     * @param time_values
     * @param max_time_values - number of time values to encode
     * @param[out] out_count - actual number of time values found
     * @return used bytes, <0 if decoding failed
     */
    BACNET_STACK_EXPORT
    int bacnet_time_values_context_decode(
        uint8_t * apdu,
        int max_apdu_len,
        uint8_t tag_number,
        BACNET_TIME_VALUE *time_values,
        unsigned int max_time_values,
        unsigned int *out_count);

    /**
     * Encode array of time-values wrapped in a context tag
     * @param apdu - output buffer, NULL to just measure length
     * @param max_apdu_len
     * @param tag_number - number to use for the context tag; 0 used for DailySchedule
     * @param time_values
     * @param max_time_values - number of time values to encode
     * @return used bytes, <=0 if encoding failed
     */
    BACNET_STACK_EXPORT
    int bacnet_time_values_context_encode(
        uint8_t * apdu,
        uint8_t tag_number,
        BACNET_TIME_VALUE * time_values,
        unsigned int max_time_values);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* _BAC_TIME_VALUE_H_ */
