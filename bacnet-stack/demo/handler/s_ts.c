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
#include "dcc.h"
#include "timesync.h"
/* some demo stuff needed */
#include "handlers.h"
#include "txbuf.h"

void Send_TimeSync(BACNET_DATE * bdate, BACNET_TIME * btime)
{
    int pdu_len = 0;
    BACNET_ADDRESS dest;
    int bytes_sent = 0;
    BACNET_NPDU_DATA npdu_data;

    if (!dcc_communication_enabled())
        return;

    /* we could use unicast or broadcast */
    datalink_get_broadcast_address(&dest);
    /* encode the APDU portion of the packet */
    pdu_len = timesync_encode_apdu(&Handler_Transmit_Buffer[0],
        bdate, btime);
    npdu_encode_unconfirmed_apdu(&npdu_data, MESSAGE_PRIORITY_NORMAL);
    /* send it out the datalink */
    bytes_sent = datalink_send_pdu(&dest, &npdu_data,
        &Handler_Transmit_Buffer[0], pdu_len);
#if PRINT_ENABLED
    if (bytes_sent <= 0)
        fprintf(stderr,
            "Failed to Send Time-Synchronization Request (%s)!\n",
            strerror(errno));
#endif
}

void Send_TimeSyncUTC(BACNET_DATE * bdate, BACNET_TIME * btime)
{
    int pdu_len = 0;
    BACNET_ADDRESS dest;
    int bytes_sent = 0;
    BACNET_NPDU_DATA npdu_data;

    if (!dcc_communication_enabled())
        return;

    /* we could use unicast or broadcast */
    datalink_get_broadcast_address(&dest);
    /* encode the APDU portion of the packet */
    pdu_len = timesync_utc_encode_apdu(&Handler_Transmit_Buffer[0],
        bdate, btime);
    npdu_encode_unconfirmed_apdu(&npdu_data, MESSAGE_PRIORITY_NORMAL);
    bytes_sent = datalink_send_pdu(&dest, &npdu_data,
        &Handler_Transmit_Buffer[0], pdu_len);
#if PRINT_ENABLED
    if (bytes_sent <= 0)
        fprintf(stderr,
            "Failed to Send UTC-Time-Synchronization Request (%s)!\n",
            strerror(errno));
#endif
}
