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
#include "bacdef.h"
#include "bacenum.h"
#include "bacapp.h"

typedef struct {
    BACNET_TIME Time;
    BACNET_APPLICATION_DATA_VALUE Value;
} BACNET_TIME_VALUE;


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

    int bacapp_encode_time_value(uint8_t * apdu,
        BACNET_TIME_VALUE * value);

    int bacapp_encode_context_time_value(uint8_t * apdu,
        uint8_t tag_number,
        BACNET_TIME_VALUE * value);

    int bacapp_decode_time_value(uint8_t * apdu,
        BACNET_TIME_VALUE * value);

    int bacapp_decode_context_time_value(uint8_t * apdu,
        uint8_t tag_number,
        BACNET_TIME_VALUE * value);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif
