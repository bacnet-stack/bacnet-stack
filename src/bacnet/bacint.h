/**************************************************************************
*
* Copyright (C) 2012 Steve Karg <skarg@users.sourceforge.net>
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
#ifndef BACINT_H
#define BACINT_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "bacnet/bacnet_stack_exports.h"

#ifdef UINT64_MAX
typedef uint64_t BACNET_UNSIGNED_INTEGER;
#define BACNET_UNSIGNED_INTEGER_MAX UINT64_MAX
#else
typedef uint32_t BACNET_UNSIGNED_INTEGER;
#define BACNET_UNSIGNED_INTEGER_MAX UINT32_MAX
#endif

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

    /* unsigned value encoding and decoding */
    BACNET_STACK_EXPORT
    int encode_unsigned16(
        uint8_t * apdu,
        uint16_t value);
    BACNET_STACK_EXPORT
    int decode_unsigned16(
        uint8_t * apdu,
        uint16_t * value);
    BACNET_STACK_EXPORT
    int encode_unsigned24(
        uint8_t * apdu,
        uint32_t value);
    BACNET_STACK_EXPORT
    int decode_unsigned24(
        uint8_t * apdu,
        uint32_t * value);
    BACNET_STACK_EXPORT
    int encode_unsigned32(
        uint8_t * apdu,
        uint32_t value);
    BACNET_STACK_EXPORT
    int decode_unsigned32(
        uint8_t * apdu,
        uint32_t * value);
#ifdef UINT64_MAX
    BACNET_STACK_EXPORT
    int encode_unsigned40(
        uint8_t * buffer,
        uint64_t value);
    BACNET_STACK_EXPORT
    int decode_unsigned40(
        uint8_t * buffer,
        uint64_t * value);
    BACNET_STACK_EXPORT
    int encode_unsigned48(
        uint8_t * buffer,
        uint64_t value);
    BACNET_STACK_EXPORT
    int decode_unsigned48(
        uint8_t * buffer,
        uint64_t * value);
    BACNET_STACK_EXPORT
    int encode_unsigned56(
        uint8_t * buffer,
        uint64_t value);
    BACNET_STACK_EXPORT
    int decode_unsigned56(
        uint8_t * buffer,
        uint64_t * value);
    BACNET_STACK_EXPORT
    int encode_unsigned64(
        uint8_t * buffer,
        uint64_t value);
    BACNET_STACK_EXPORT
    int decode_unsigned64(
        uint8_t * buffer,
        uint64_t * value);
#endif
    BACNET_STACK_EXPORT
    int bacnet_unsigned_length(
        BACNET_UNSIGNED_INTEGER value);
    /* signed value encoding and decoding */
    BACNET_STACK_EXPORT
    int encode_signed8(
        uint8_t * apdu,
        int8_t value);
    BACNET_STACK_EXPORT
    int decode_signed8(
        uint8_t * apdu,
        int32_t * value);
    BACNET_STACK_EXPORT
    int encode_signed16(
        uint8_t * apdu,
        int16_t value);
    BACNET_STACK_EXPORT
    int decode_signed16(
        uint8_t * apdu,
        int32_t * value);
    BACNET_STACK_EXPORT
    int encode_signed24(
        uint8_t * apdu,
        int32_t value);
    BACNET_STACK_EXPORT
    int decode_signed24(
        uint8_t * apdu,
        int32_t * value);
    BACNET_STACK_EXPORT
    int encode_signed32(
        uint8_t * apdu,
        int32_t value);
    BACNET_STACK_EXPORT
    int decode_signed32(
        uint8_t * apdu,
        int32_t * value);

#ifdef BAC_TEST
#include "ctest.h"
    BACNET_STACK_EXPORT
    void testBACnetIntegers(
        Test * pTest);
#endif

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif
