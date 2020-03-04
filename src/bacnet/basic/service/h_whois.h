/**
* @file
* @author Steve Karg
* @date October 2019
* @brief Header file for a basic Who-Is service handler
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
#ifndef HANDLER_WHO_IS_H
#define HANDLER_WHO_IS_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdint.h>
#include "bacnet/bacnet_stack_exports.h"
#include "bacnet/bacdef.h"
#include "bacnet/bacenum.h"
#include "bacnet/apdu.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

    BACNET_STACK_EXPORT
    void handler_who_is(
        uint8_t * service_request,
        uint16_t service_len,
        BACNET_ADDRESS * src);

    BACNET_STACK_EXPORT
    void handler_who_is_unicast(
        uint8_t * service_request,
        uint16_t service_len,
        BACNET_ADDRESS * src);

    BACNET_STACK_EXPORT
    void handler_who_is_bcast_for_routing(
        uint8_t * service_request,
        uint16_t service_len,
        BACNET_ADDRESS * src);

    BACNET_STACK_EXPORT
    void handler_who_is_unicast_for_routing(
        uint8_t * service_request,
        uint16_t service_len,
        BACNET_ADDRESS * src);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif
