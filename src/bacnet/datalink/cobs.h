/**************************************************************************
 *
 * Copyright (C) 2014 Kerry Lynn <kerlyn@ieee.org>
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
#ifndef COBS_H
#define COBS_H

#include <stddef.h>
#include <stdint.h>
#include "bacnet/bacnet_stack_exports.h"

/* number of bytes needed for COBS encoded CRC */
#define COBS_ENCODED_CRC_SIZE 5
/* inclusive extra bytes needed for APDU */
#define COBS_ENCODED_SIZE(a) ((a)+((a)/254)+1)

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

BACNET_STACK_EXPORT
size_t cobs_encode(
    uint8_t *buffer,
    size_t buffer_size,
    const uint8_t *from,
    size_t length,
    uint8_t mask);

BACNET_STACK_EXPORT
size_t cobs_frame_encode(
    uint8_t *buffer,
    size_t buffer_size,
    const uint8_t *from,
    size_t length);

BACNET_STACK_EXPORT
size_t cobs_decode(
    uint8_t *buffer,
    size_t buffer_size,
    const uint8_t *from,
    size_t length,
    uint8_t mask);

BACNET_STACK_EXPORT
size_t cobs_frame_decode(
    uint8_t *buffer,
    size_t buffer_size,
    const uint8_t *from,
    size_t length);

BACNET_STACK_EXPORT
uint32_t cobs_crc32k(
    uint8_t dataValue,
    uint32_t crc);

BACNET_STACK_EXPORT
size_t cobs_crc32k_encode(
    uint8_t *buffer,
    size_t buffer_size,
    uint32_t crc);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif
