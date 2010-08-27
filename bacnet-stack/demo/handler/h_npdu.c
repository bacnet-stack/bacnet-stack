/**************************************************************************
*
* Copyright (C) 2008 Steve Karg <skarg@users.sourceforge.net>
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
#include <stdbool.h>
#include <stdint.h>
#include "bacdef.h"
#include "bacdcode.h"
#include "bacint.h"
#include "bacenum.h"
#include "bits.h"
#include "npdu.h"
#include "apdu.h"
#include "handlers.h"
#include "client.h"

#if PRINT_ENABLED
#include <stdio.h>
#endif

/** @file h_npdu.c  Handles messages at the NPDU level of the BACnet stack. */

void npdu_handler(
    BACNET_ADDRESS * src,       /* source address */
    uint8_t * pdu,      /* PDU data */
    uint16_t pdu_len)
{       /* length PDU  */
    int apdu_offset = 0;
    BACNET_ADDRESS dest = { 0 };
    BACNET_NPDU_DATA npdu_data = { 0 };

    /* only handle the version that we know how to handle */
    if (pdu[0] == BACNET_PROTOCOL_VERSION) {
        apdu_offset = npdu_decode(&pdu[0], &dest, src, &npdu_data);
        if (npdu_data.network_layer_message) {
            /*FIXME: network layer message received!  Handle it! */
#if PRINT_ENABLED
            fprintf(stderr,"NPDU: Network Layer Message discarded!\n");
#endif
        } else if ((apdu_offset > 0) && (apdu_offset <= pdu_len)) {
            if ((dest.net == 0) || (dest.net == BACNET_BROADCAST_NETWORK)) {
                /* only handle the version that we know how to handle */
                /* and we are not a router, so ignore messages with
                   routing information cause they are not for us */
                apdu_handler(src, &pdu[apdu_offset],
                    (uint16_t) (pdu_len - apdu_offset));
            } else {
#if PRINT_ENABLED
                printf("NPDU: DNET=%u.  Discarded!\n", (unsigned) dest.net);
#endif
            }
        }
    } else {
#if PRINT_ENABLED
        printf("NPDU: BACnet Protocol Version=%u.  Discarded!\n",
            (unsigned) pdu[0]);
#endif
    }

    return;
}
