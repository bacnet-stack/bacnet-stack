/**************************************************************************
*
* Copyright (C) 2006 John Minack
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
#ifndef LSO_H
#define LSO_H

#include <stdint.h>
#include <stdbool.h>
#include "bacnet/bacnet_stack_exports.h"
#include "bacnet/bacenum.h"
#include "bacnet/bacdef.h"
#include "bacnet/bacstr.h"

typedef struct {
    uint32_t processId;
    BACNET_CHARACTER_STRING requestingSrc;
    BACNET_LIFE_SAFETY_OPERATION operation;
    BACNET_OBJECT_ID targetObject;
    bool use_target:1;
} BACNET_LSO_DATA;

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

    BACNET_STACK_EXPORT
    int lso_encode_apdu(
        uint8_t * apdu,
        uint8_t invoke_id,
        BACNET_LSO_DATA * data);

    BACNET_STACK_EXPORT
    int life_safety_operation_encode(
        uint8_t *apdu, 
        BACNET_LSO_DATA *data);

    BACNET_STACK_EXPORT
    size_t life_safety_operation_request_encode(
        uint8_t *apdu, 
        size_t apdu_size, 
        BACNET_LSO_DATA *data);

    BACNET_STACK_EXPORT
    int lso_decode_service_request(
        uint8_t * apdu,
        unsigned apdu_len,
        BACNET_LSO_DATA * data);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif
