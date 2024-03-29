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
#ifndef BACREAL_H
#define BACREAL_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
/* BACnet Stack defines - first */
#include "bacnet/bacdef.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

    BACNET_STACK_EXPORT
    int decode_real_safe(
        uint8_t * apdu,
        uint32_t len_value,
        float *real_value);

    BACNET_STACK_EXPORT
    int decode_real(
        uint8_t * apdu,
        float *real_value);

    BACNET_STACK_EXPORT
    int encode_bacnet_real(
        float value,
        uint8_t * apdu);
    BACNET_STACK_EXPORT
    int decode_double(
        uint8_t * apdu,
        double *real_value);
    BACNET_STACK_EXPORT
    int decode_double_safe(
        uint8_t * apdu,
        uint32_t len_value,
        double *double_value);

    BACNET_STACK_EXPORT
    int encode_bacnet_double(
        double value,
        uint8_t * apdu);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif
