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
#include <stddef.h>
#include <stdint.h>
#include <errno.h>
#include <string.h>
#include "config.h"
#include "bacdef.h"
#include "bacdcode.h"
#include "address.h"
#include "tsm.h"
#include "dcc.h"
#include "npdu.h"
#include "apdu.h"
#include "device.h"
#include "datalink.h"
#include "iam.h"
/* some demo stuff needed */
#include "handlers.h"

int iam_encode_pdu(
    uint8_t * buffer,
    BACNET_ADDRESS * dest,
    BACNET_NPDU_DATA * npdu_data)
{
    int len = 0;
    int pdu_len = 0;

    /* I-Am is a global broadcast */
    datalink_get_broadcast_address(dest);
    /* encode the NPDU portion of the packet */
    npdu_encode_npdu_data(npdu_data, false, MESSAGE_PRIORITY_NORMAL);
    pdu_len = npdu_encode_pdu(&buffer[0], dest, NULL, npdu_data);
    /* encode the APDU portion of the packet */
    len =
        iam_encode_apdu(&buffer[pdu_len], Device_Object_Instance_Number(),
        MAX_APDU, SEGMENTATION_NONE, Device_Vendor_Identifier());
    pdu_len += len;

    return pdu_len;
}

void Send_I_Am(
    uint8_t * buffer)
{
    int pdu_len = 0;
    BACNET_ADDRESS dest;
    int bytes_sent = 0;
    BACNET_NPDU_DATA npdu_data;

#if 0
    /* note: there is discussion in the BACnet committee
       that we should allow a device to reply with I-Am 
       so that dynamic binding always work.  If the DCC
       initiator loses the MAC address and routing info, 
       they can never re-enable DCC because they can't 
       find the device with WhoIs/I-Am */
    /* are we are forbidden to send? */
    if (!dcc_communication_enabled())
        return 0;
#endif
    /* encode the data */
    pdu_len = iam_encode_pdu(buffer, &dest, &npdu_data);
    /* send data */
    bytes_sent = datalink_send_pdu(&dest, &npdu_data, &buffer[0], pdu_len);

#if PRINT_ENABLED
    if (bytes_sent <= 0)
        fprintf(stderr, "Failed to Send I-Am Reply (%s)!\n", strerror(errno));
#endif
}
