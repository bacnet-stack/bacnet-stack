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
#include "bacdef.h"
#include "bacdcode.h"
#include "address.h"
#include "tsm.h"
#include "npdu.h"
#include "apdu.h"
#include "device.h"
#include "datalink.h"
#include "dcc.h"
#include "rd.h"
/* some demo stuff needed */
#include "handlers.h"
#include "session.h"
#include "clientsubscribeinvoker.h"

/** @file s_rd.c  Send a Reinitialize Device request. */

/** Sends a Reinitialize Device (RD) request.
 * @ingroup DMRD
 * 
 * @param device_id [in] The index to the device address in our address cache.
 * @param state [in] Specifies the desired state of the device after reinitialization.
 * @param password [in] Optional password, up to 20 chars.
 * @return The invokeID of the transmitted message, or 0 on failure.
 */
uint8_t Send_Reinitialize_Device_Request(
    struct bacnet_session_object *sess,
    struct ClientSubscribeInvoker *subscriber,
    uint32_t device_id,
    BACNET_REINITIALIZED_STATE state,
    BACNET_CHARACTER_STRING * password_string)
{
    BACNET_ADDRESS dest;
    unsigned max_apdu = 0;
    uint8_t invoke_id = 0;
    bool status = false;
    int len = 0;
    int pdu_len = 0;
    int bytes_sent = 0;
    BACNET_NPDU_DATA npdu_data;
    BACNET_APDU_FIXED_HEADER apdu_fixed_header;
    uint8_t Handler_Transmit_Buffer[MAX_PDU] = { 0 };
    uint8_t segmentation = 0;
    uint32_t maxsegments = 0;

    /* if we are forbidden to send, don't send! */
    if (!dcc_communication_enabled(sess))
        return 0;

    /* is the device bound? */
    status =
        address_get_by_device(sess, device_id, &max_apdu, &segmentation,
        &maxsegments, &dest);
    /* is there a tsm available? */
    if (status)
        invoke_id = tsm_next_free_invokeID(sess);
    if (invoke_id) {
        /* if a client subscriber is provided, then associate the invokeid with that client
           otherwise another thread might receive a message with this invokeid before we return
           from this function */
        if (subscriber && subscriber->SubscribeInvokeId) {
            subscriber->SubscribeInvokeId(invoke_id, subscriber->context);
        }
        /* encode the NPDU portion of the packet */
        npdu_encode_npdu_data(&npdu_data, true, MESSAGE_PRIORITY_NORMAL);
        apdu_init_fixed_header(&apdu_fixed_header,
            PDU_TYPE_CONFIRMED_SERVICE_REQUEST, invoke_id,
            SERVICE_CONFIRMED_REINITIALIZE_DEVICE, max_apdu);
        /* encode the APDU portion of the packet */
        len =
            rd_encode_apdu(&Handler_Transmit_Buffer[pdu_len], invoke_id, state,
            password_string);
        pdu_len += len;
        /* Send data to the peer device, respecting APDU sizes, destination size,
           and segmented or unsegmented data sending possibilities */
        bytes_sent =
            tsm_set_confirmed_transaction(sess, invoke_id, &dest, &npdu_data,
            &apdu_fixed_header, &Handler_Transmit_Buffer[0], pdu_len);

        if (bytes_sent <= 0)
            invoke_id = 0;

#if PRINT_ENABLED
        if (bytes_sent <= 0)
            fprintf(stderr,
                "Failed to Send ReinitializeDevice Request (%s)!\n",
                strerror(errno));
#endif
    }

    return invoke_id;
}
