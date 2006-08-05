/**************************************************************************
*
* Copyright (C) 2006 Steve Karg <skarg@users.sourceforge.net>
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
/* some demo stuff needed */
#include "handlers.h"
#include "txbuf.h"

uint8_t Send_Device_Communication_Control_Request(uint32_t device_id, uint16_t timeDuration,    /* 0=optional */
    BACNET_COMMUNICATION_ENABLE_DISABLE state, char *password)
{                               /* NULL=optional */
    BACNET_ADDRESS dest;
    BACNET_ADDRESS my_address;
    unsigned max_apdu = 0;
    uint8_t invoke_id = 0;
    bool status = false;
    int pdu_len = 0;
    int bytes_sent = 0;
    BACNET_CHARACTER_STRING password_string;

    if (!dcc_communication_enabled())
        return 0;

    /* is the device bound? */
    status = address_get_by_device(device_id, &max_apdu, &dest);
    /* is there a tsm available? */
    if (status)
        status = tsm_transaction_available();
    if (status) {
        datalink_get_my_address(&my_address);
        pdu_len = npdu_encode_apdu(&Handler_Transmit_Buffer[0], &dest, &my_address, true,       /* true for confirmed messages */
            MESSAGE_PRIORITY_NORMAL);

        invoke_id = tsm_next_free_invokeID();
        /* load the data for the encoding */
        characterstring_init_ansi(&password_string, password);
        pdu_len += dcc_encode_apdu(&Handler_Transmit_Buffer[pdu_len],
            invoke_id,
            timeDuration, state, password ? &password_string : NULL);
        /* will it fit in the sender?
           note: if there is a bottleneck router in between
           us and the destination, we won't know unless
           we have a way to check for that and update the
           max_apdu in the address binding table. */
        if ((unsigned) pdu_len < max_apdu) {
            tsm_set_confirmed_unsegmented_transaction(invoke_id,
                &dest, &Handler_Transmit_Buffer[0], pdu_len);
            bytes_sent = datalink_send_pdu(&dest,       /* destination address */
                &Handler_Transmit_Buffer[0], pdu_len);  /* number of bytes of data */
            if (bytes_sent <= 0)
                fprintf(stderr,
                    "Failed to Send DeviceCommunicationControl Request (%s)!\n",
                    strerror(errno));
        } else
            fprintf(stderr,
                "Failed to Send DeviceCommunicationControl Request "
                "(exceeds destination maximum APDU)!\n");
    }

    return invoke_id;
}
