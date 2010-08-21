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
#include <string.h>
#include "config.h"
#include "config.h"
#include "bacdef.h"
#include "bacdcode.h"
#include "address.h"
#include "dcc.h"
#include "tsm.h"
#include "npdu.h"
#include "apdu.h"
#include "device.h"
#include "datalink.h"
#include "arf.h"
/* some demo stuff needed */
#include "handlers.h"
#include "session.h"
#include "clientsubscribeinvoker.h"
/** @file s_arfs.c  Send part of an Atomic Read File Stream. */

uint8_t Send_Atomic_Read_File_Stream(
    struct bacnet_session_object *sess,
    struct ClientSubscribeInvoker *subscriber,
    uint32_t device_id,
    uint32_t file_instance,
    int fileStartPosition,
    unsigned requestedOctetCount)
{
    BACNET_ADDRESS dest;
    BACNET_ADDRESS my_address;
    BACNET_NPDU_DATA npdu_data;
    unsigned max_apdu = 0;
    uint8_t invoke_id = 0;
    bool status = false;
    int len = 0;
    int pdu_len = 0;
    int bytes_sent = 0;
    BACNET_ATOMIC_READ_FILE_DATA data;
    uint8_t Handler_Transmit_Buffer[MAX_PDU] = { 0 };
    uint8_t segmentation = 0;

    /* if we are forbidden to send, don't send! */
    if (!dcc_communication_enabled(sess))
        return 0;

    /* is the device bound? */
    status =
        address_get_by_device(sess, device_id, &max_apdu, &segmentation,
        &dest);
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
        /* load the data for the encoding */
        data.object_type = OBJECT_FILE;
        data.object_instance = file_instance;
        data.access = FILE_STREAM_ACCESS;
        data.type.stream.fileStartPosition = fileStartPosition;
        data.type.stream.requestedOctetCount = requestedOctetCount;
        /* encode the NPDU portion of the packet */
        sess->datalink_get_my_address(sess, &my_address);
        npdu_encode_npdu_data(&npdu_data, true, MESSAGE_PRIORITY_NORMAL);
        pdu_len =
            npdu_encode_pdu(&Handler_Transmit_Buffer[0], &dest, &my_address,
            &npdu_data);
        len =
            arf_encode_apdu(&Handler_Transmit_Buffer[pdu_len], invoke_id,
            &data);
        pdu_len += len;
        /* will the APDU fit the target device?
           note: if there is a bottleneck router in between
           us and the destination, we won't know unless
           we have a way to check for that and update the
           max_apdu in the address binding table. */
        if ((unsigned) pdu_len < max_apdu) {
            tsm_set_confirmed_unsegmented_transaction(sess, invoke_id, &dest,
                &npdu_data, &Handler_Transmit_Buffer[0], pdu_len);
            bytes_sent =
                sess->datalink_send_pdu(sess, &dest, &npdu_data,
                &Handler_Transmit_Buffer[0], pdu_len);
#if PRINT_ENABLED
            if (bytes_sent <= 0)
                fprintf(stderr,
                    "Failed to Send AtomicReadFile Request (%s)!\n",
                    strerror(errno));
#endif
        } else {
            tsm_free_invoke_id_check(sess, invoke_id, NULL, false);
            invoke_id = 0;
#if PRINT_ENABLED
            fprintf(stderr,
                "Failed to Send AtomicReadFile Request "
                "(payload exceeds destination maximum APDU)!\n");
#endif
        }
    }

    return invoke_id;
}
