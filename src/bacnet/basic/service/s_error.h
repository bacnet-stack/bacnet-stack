/**
* @file
* @author Steve Karg
* @date October 2019
* @brief Header file for a basic Error message send
*
* @section LICENSE
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
*/
#ifndef SEND_ERROR_MESSAGE_H
#define SEND_ERROR_MESSAGE_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdint.h>
/* BACnet Stack defines - first */
#include "bacnet/bacdef.h"
/* BACnet Stack API */
#include "bacnet/bacapp.h"
#include "bacnet/apdu.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

    BACNET_STACK_EXPORT
    int Send_Error_To_Network(
        uint8_t * buffer,
        BACNET_ADDRESS *dest,
        uint8_t invoke_id,
        BACNET_CONFIRMED_SERVICE service,
        BACNET_ERROR_CLASS error_class,
        BACNET_ERROR_CODE error_code);

    BACNET_STACK_EXPORT
    int error_encode_pdu(
        uint8_t * buffer,
        BACNET_ADDRESS * dest,
        BACNET_ADDRESS * src,
        BACNET_NPDU_DATA * npdu_data,
        uint8_t invoke_id,
        BACNET_CONFIRMED_SERVICE service,
        BACNET_ERROR_CLASS error_class,
        BACNET_ERROR_CODE error_code);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif
