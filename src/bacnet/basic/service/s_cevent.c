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
#include "bacnet/event.h"
#include "bacnet/dcc.h"
/* some demo stuff needed */
#include "bacnet/basic/object/device.h"
#include "bacnet/datalink/datalink.h"
#include "bacnet/basic/tsm/tsm.h"
#include "bacnet/basic/binding/address.h"
#include "bacnet/basic/services.h"

/** @file s_cevent.c  Send a ConfirmedEventNotification Request. */

/** Sends an Confirmed Alarm/Event Notification.
 * @ingroup EVNOTFCN
 *
 * @param pdu [in] the PDU buffer used for sending the message
 * @param pdu_size [in] Size of the PDU buffer
 * @param data [in] The information about the Event to be sent.
 * @param dest [in] BACNET_ADDRESS of the destination device
 * @return invoke id of outgoing message, or 0 if communication is disabled,
 *         or no tsm slot is available.
 */
uint8_t Send_CEvent_Notify_Address(uint8_t *pdu,
    uint16_t pdu_size,
    BACNET_EVENT_NOTIFICATION_DATA *data,
    BACNET_ADDRESS *dest)
{
    int len = 0;
    int pdu_len = 0;
#if PRINT_ENABLED
    int bytes_sent = 0;
#endif
    BACNET_NPDU_DATA npdu_data;
    BACNET_ADDRESS my_address;
    uint8_t invoke_id = 0;

    if (!dcc_communication_enabled()) {
        return 0;
    }
    if (!dest) {
        return 0;
    }
    /* is there a tsm available? */
    invoke_id = tsm_next_free_invokeID();
    if (invoke_id) {
        /* encode the NPDU portion of the packet */
        datalink_get_my_address(&my_address);
        npdu_encode_npdu_data(&npdu_data, true, MESSAGE_PRIORITY_NORMAL);
        pdu_len = npdu_encode_pdu(pdu, dest, &my_address, &npdu_data);
        /* encode the APDU portion of the packet */
        len = cevent_notify_encode_apdu(&pdu[pdu_len], invoke_id, data);
        pdu_len += len;
        /* will it fit in the sender?
           note: if there is a bottleneck router in between
           us and the destination, we won't know unless
           we have a way to check for that and update the
           max_apdu in the address binding table. */
        if ((uint16_t)pdu_len < pdu_size) {
            tsm_set_confirmed_unsegmented_transaction(
                invoke_id, dest, &npdu_data, pdu, (uint16_t)pdu_len);
#if PRINT_ENABLED
            bytes_sent =
#endif
                datalink_send_pdu(dest, &npdu_data, pdu, pdu_len);
#if PRINT_ENABLED
            if (bytes_sent <= 0) {
                fprintf(stderr,
                    "Failed to Send ConfirmedEventNotification Request (%s)!\n",
                    strerror(errno));
            }
#endif
        } else {
            tsm_free_invoke_id(invoke_id);
            invoke_id = 0;
#if PRINT_ENABLED
            fprintf(stderr,
                "Failed to Send ConfirmedEventNotification Request "
                "(exceeds destination maximum APDU)!\n");
#endif
        }
    }

    return invoke_id;
}

/** Sends an Confirmed Alarm/Event Notification.
 * @ingroup EVNOTFCN
 *
 * @param device_id [in] ID of the destination device
 * @param data [in] The information about the Event to be sent.
 * @return invoke id of outgoing message, or 0 if communication is disabled,
 *         or no tsm slot is available.
 */
uint8_t Send_CEvent_Notify(
    uint32_t device_id, BACNET_EVENT_NOTIFICATION_DATA *data)
{
    BACNET_ADDRESS dest = { 0 };
    unsigned max_apdu = 0;
    uint8_t invoke_id = 0;
    bool status = false;

    /* is the device bound? */
    status = address_get_by_device(device_id, &max_apdu, &dest);
    if (status) {
        if (sizeof(Handler_Transmit_Buffer) < max_apdu) {
            max_apdu = sizeof(Handler_Transmit_Buffer);
        }
        invoke_id = Send_CEvent_Notify_Address(
            Handler_Transmit_Buffer, max_apdu, data, &dest);
    }

    return invoke_id;
}
