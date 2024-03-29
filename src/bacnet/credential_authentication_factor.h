/**************************************************************************
*
* Copyright (C) 2015 Nikola Jelic <nikola.jelic@euroicc.com>
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
*
*********************************************************************/

#ifndef BACNET_CREDENTIAL_AUTHENTICATION_FACTOR_H
#define BACNET_CREDENTIAL_AUTHENTICATION_FACTOR_H

#include <stdbool.h>
#include <stdint.h>
/* BACnet Stack defines - first */
#include "bacnet/bacdef.h"
/* BACnet Stack API */
#include "bacnet/bacapp.h"
#include "bacnet/authentication_factor.h"

typedef struct {
    BACNET_ACCESS_AUTHENTICATION_FACTOR_DISABLE disable;
    BACNET_AUTHENTICATION_FACTOR authentication_factor;
} BACNET_CREDENTIAL_AUTHENTICATION_FACTOR;


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

    BACNET_STACK_EXPORT
    int bacapp_encode_credential_authentication_factor(
        uint8_t * apdu,
        BACNET_CREDENTIAL_AUTHENTICATION_FACTOR * caf);
    BACNET_STACK_EXPORT
    int bacapp_encode_context_credential_authentication_factor(
        uint8_t * apdu,
        uint8_t tag,
        BACNET_CREDENTIAL_AUTHENTICATION_FACTOR * caf);
    BACNET_STACK_EXPORT
    int bacapp_decode_credential_authentication_factor(
        uint8_t * apdu,
        BACNET_CREDENTIAL_AUTHENTICATION_FACTOR * caf);
    BACNET_STACK_EXPORT
    int bacapp_decode_context_credential_authentication_factor(
        uint8_t * apdu,
        uint8_t tag,
        BACNET_CREDENTIAL_AUTHENTICATION_FACTOR * caf);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif
