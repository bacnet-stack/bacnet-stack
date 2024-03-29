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
#ifndef BACNET_REJECT_H
#define BACNET_REJECT_H

#include <stdint.h>
#include <stdbool.h>
/* BACnet Stack defines - first */
#include "bacnet/bacdef.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

    BACNET_STACK_EXPORT
    BACNET_REJECT_REASON reject_convert_error_code(
        BACNET_ERROR_CODE error_code);
    BACNET_STACK_EXPORT
    bool reject_valid_error_code(
        BACNET_ERROR_CODE error_code);
    BACNET_STACK_EXPORT
    BACNET_ERROR_CODE reject_convert_to_error_code(
        BACNET_REJECT_REASON reject_code);

    BACNET_STACK_EXPORT
    int reject_encode_apdu(
        uint8_t * apdu,
        uint8_t invoke_id,
        uint8_t reject_reason);

    BACNET_STACK_EXPORT
    int reject_decode_service_request(
        uint8_t * apdu,
        unsigned apdu_len,
        uint8_t * invoke_id,
        uint8_t * reject_reason);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif
