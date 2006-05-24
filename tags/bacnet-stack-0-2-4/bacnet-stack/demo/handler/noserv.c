/**************************************************************************
*
* Copyright (C) 2005 Steve Karg <skarg@users.sourceforge.net>
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
#include "txbuf.h"
#include "bacdef.h"
#include "bacdcode.h"
#include "apdu.h"
#include "npdu.h"
#include "reject.h"

void handler_unrecognized_service(uint8_t * service_request,
    uint16_t service_len,
    BACNET_ADDRESS * dest, BACNET_CONFIRMED_SERVICE_DATA * service_data)
{
    BACNET_ADDRESS src;
    int pdu_len = 0;
    int bytes_sent = 0;

    (void) service_request;
    (void) service_len;
    datalink_get_my_address(&src);

    /* encode the NPDU portion of the packet */
    pdu_len = npdu_encode_apdu(&Handler_Transmit_Buffer[0], dest, &src, false,  /* true for confirmed messages */
        MESSAGE_PRIORITY_NORMAL);

    /* encode the APDU portion of the packet */
    pdu_len += reject_encode_apdu(&Handler_Transmit_Buffer[pdu_len],
        service_data->invoke_id, REJECT_REASON_UNRECOGNIZED_SERVICE);

    bytes_sent = datalink_send_pdu(dest,        /* destination address */
        &Handler_Transmit_Buffer[0], pdu_len);  /* number of bytes of data */
    if (bytes_sent > 0)
        fprintf(stderr, "Sent Reject!\n");
    else
        fprintf(stderr, "Failed to Send Reject (%s)!\n", strerror(errno));
}
