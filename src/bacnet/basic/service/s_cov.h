/**
* @file
* @author Steve Karg
* @date October 2019
* @brief Header file for a basic UnconfirmedCOVNotification service send
* and SubscribeCOV service send
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
#ifndef SERVICE_SEND_COV_H
#define SERVICE_SEND_COV_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdint.h>
#include "bacnet/bacnet_stack_exports.h"
#include "bacnet/bacapp.h"
#include "bacnet/bacdef.h"
#include "bacnet/bacenum.h"
#include "bacnet/apdu.h"
#include "bacnet/cov.h"
#include "bacnet/npdu.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

    BACNET_STACK_EXPORT
    int Send_UCOV_Notify(
        uint8_t * buffer,
        unsigned buffer_len,
        BACNET_COV_DATA * cov_data);
    BACNET_STACK_EXPORT
    int ucov_notify_encode_pdu(
        uint8_t * buffer,
        unsigned buffer_len,
        BACNET_ADDRESS * dest,
        BACNET_NPDU_DATA * npdu_data,
        BACNET_COV_DATA * cov_data);
    BACNET_STACK_EXPORT
    uint8_t Send_COV_Subscribe(
        uint32_t device_id,
        BACNET_SUBSCRIBE_COV_DATA * cov_data);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif
