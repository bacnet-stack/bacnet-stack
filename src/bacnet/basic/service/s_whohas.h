/**
* @file
* @author Steve Karg
* @date October 2019
* @brief Header file for a basic WhoHas service send
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
#ifndef SEND_WHO_HAS_H
#define SEND_WHO_HAS_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdint.h>
#include "bacnet/bacapp.h"
#include "bacnet/bacdef.h"
#include "bacnet/bacenum.h"
#include "bacnet/apdu.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

    void Send_WhoHas_Name(
        int32_t low_limit,
        int32_t high_limit,
        const char *object_name);
    void Send_WhoHas_Object(int32_t low_limit, int32_t high_limit,
                            BACNET_OBJECT_TYPE object_type,
                            uint32_t object_instance);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif
