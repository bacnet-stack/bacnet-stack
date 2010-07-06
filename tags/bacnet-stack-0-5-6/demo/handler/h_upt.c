/**************************************************************************
*
* Copyright (C) 2009 Steve Karg <skarg@users.sourceforge.net>
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
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include "config.h"
#include "txbuf.h"
#include "bacdef.h"
#include "bacdcode.h"
#include "apdu.h"
#include "npdu.h"
#include "abort.h"
#include "ptransfer.h"

/** @file h_upt.c  Handles Unconfirmed Private Transfer requests. */

void handler_unconfirmed_private_transfer(
    uint8_t * service_request,
    uint16_t service_len,
    BACNET_ADDRESS * src)
{
    BACNET_PRIVATE_TRANSFER_DATA private_data;
    int len = 0;

#if PRINT_ENABLED
    fprintf(stderr, "Received Unconfirmed Private Transfer Request!\n");
#endif
    (void) src;
    len =
        ptransfer_decode_service_request(service_request, service_len,
        &private_data);
    if (len >= 0) {
#if PRINT_ENABLED
        fprintf(stderr,
            "UnconfirmedPrivateTransfer: " "vendorID=%u serviceNumber=%u\n",
            private_data.vendorID, private_data.serviceNumber);
#endif
    }
}
