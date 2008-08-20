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
#include <errno.h>
#include <string.h>
#include "config.h"
#include "config.h"
#include "txbuf.h"
#include "bacdef.h"
#include "bacdcode.h"
#include "address.h"
#include "tsm.h"
#include "npdu.h"
#include "apdu.h"
#include "device.h"
#include "datalink.h"
/* some demo stuff needed */
#include "handlers.h"
#include "txbuf.h"

/* find a specific device, or use -1 for limit if you want unlimited */
void Send_WhoIsRouterToNetwork(
    int dnet)
{
    int len = 0;
    int pdu_len = 0;
    BACNET_ADDRESS dest;
    int bytes_sent = 0;
    BACNET_NPDU_DATA npdu_data;

    /* encode the NPDU portion of the packet */
    npdu_data.data_expecting_reply = false;
    npdu_data.protocol_version = BACNET_PROTOCOL_VERSION;
    npdu_data.network_layer_message = true; /* false if APDU */
    npdu_data.network_message_type = NETWORK_MESSAGE_WHO_IS_ROUTER_TO_NETWORK;      
    npdu_data.vendor_id = 0;       /* optional, if net message type is > 0x80 */
    npdu_data.priority = MESSAGE_PRIORITY_NORMAL;
    npdu_data.hop_count = 0;
    pdu_len =
        npdu_encode_pdu(&Handler_Transmit_Buffer[0], NULL, NULL, &npdu_data);
    /* encode the optional DNET portion of the packet */
    if (dnet >= 0) {
        len = encode_unsigned16(&Handler_Transmit_Buffer[pdu_len], dnet);
        pdu_len += len;
#if PRINT_ENABLED
        fprintf(stderr, "Send Who-Is-Router-To-Network Request to %u\n",dnet);
#endif
    } else {
#if PRINT_ENABLED
        fprintf(stderr, "Send Who-Is-Router-To-Network Request\n");
#endif
    }
    /* Who-Is-Router-To-Network is a global broadcast */
    datalink_get_broadcast_address(&dest);
    bytes_sent =
        datalink_send_pdu(&dest, &npdu_data, &Handler_Transmit_Buffer[0],
        pdu_len);
#if PRINT_ENABLED
    if (bytes_sent <= 0)
        fprintf(stderr, "Failed to Send Who-Is-Router-To-Network Request (%s)!\n",
            strerror(errno));
#endif
}
