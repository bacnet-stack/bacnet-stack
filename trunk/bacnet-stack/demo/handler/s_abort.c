/**************************************************************************
*
* Copyright (C) 2016 Steve Karg <skarg@users.sourceforge.net>
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
#include <errno.h>
#include <string.h>
#include "config.h"
#include "bacdef.h"
#include "bacdcode.h"
#include "abort.h"
#include "address.h"
#include "tsm.h"
#include "dcc.h"
#include "npdu.h"
#include "apdu.h"
#include "device.h"
#include "datalink.h"
#include "iam.h"
#include "txbuf.h"
/* some demo stuff needed */
#include "handlers.h"
#include "client.h"


/** Encodes an Abort message
 * @param buffer The buffer to build the message for sending.
 * @param dest - Destination address to send the message
 * @param src - Source address from which the message originates
 * @param npdu_data - buffer to hold NPDU data encoded
 * @param invoke_id - use to match up a reply
 * @param reason - #BACNET_ABORT_REASON enumeration
 * @param server - true or false
 *
 * @return Size of the message sent (bytes), or a negative value on error.
 */
int abort_encode_pdu(
    uint8_t * buffer,
    BACNET_ADDRESS * dest,
    BACNET_ADDRESS * src,
    BACNET_NPDU_DATA * npdu_data,
    uint8_t invoke_id,
    BACNET_ABORT_REASON reason,
    bool server)
{
    int len = 0;
    int pdu_len = 0;

    /* encode the NPDU portion of the packet */
    npdu_encode_npdu_data(npdu_data, false, MESSAGE_PRIORITY_NORMAL);
    pdu_len = npdu_encode_pdu(&buffer[0], dest, src, npdu_data);

    /* encode the APDU portion of the packet */
    len = abort_encode_apdu(&buffer[pdu_len], invoke_id, reason, server);
    pdu_len += len;

    return pdu_len;
}

/** Sends an Abort message
 * @param buffer The buffer to build the message for sending.
 * @param dest - Destination address to send the message
 * @param invoke_id - use to match up a reply
 * @param reason - #BACNET_ABORT_REASON enumeration
 * @param server - true or false
 *
 * @return Size of the message sent (bytes), or a negative value on error.
 */
int Send_Abort_To_Network(
    uint8_t * buffer,
    BACNET_ADDRESS *dest,
    uint8_t invoke_id,
    BACNET_ABORT_REASON reason,
    bool server)
{
    int pdu_len = 0;
    BACNET_ADDRESS src;
    int bytes_sent = 0;
    BACNET_NPDU_DATA npdu_data;

    datalink_get_my_address(&src);
    pdu_len = abort_encode_pdu(buffer, dest, &src, &npdu_data,
        invoke_id, reason, server);
    bytes_sent = datalink_send_pdu(dest, &npdu_data, &buffer[0], pdu_len);

    return bytes_sent;
}
