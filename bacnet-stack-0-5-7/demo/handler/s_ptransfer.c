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
#include <stdlib.h>
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
#include "ptransfer.h"
/* some demo stuff needed */
#include "handlers.h"
#include "mydata.h"
#include "session.h"
#include "clientsubscribeinvoker.h"

/** @file s_ptransfer.c  Send a Private Transfer request. */

uint8_t Send_Private_Transfer_Request(
    struct bacnet_session_object *sess,
    struct ClientSubscribeInvoker *subscriber,
    uint32_t device_id,
    uint16_t vendor_id,
    uint32_t service_number,
    char block_number,
    DATABLOCK * block)
{       /* NULL=optional */
    BACNET_ADDRESS dest;
    unsigned max_apdu = 0;
    uint8_t invoke_id = 0;
    bool status = false;
    int len = 0;
    int pdu_len = 0;
    int bytes_sent = 0;
    BACNET_NPDU_DATA npdu_data;
    BACNET_APDU_FIXED_HEADER apdu_fixed_header;
    static uint8_t pt_req_buffer[300];  /* Somewhere to build the request packet */
    BACNET_PRIVATE_TRANSFER_DATA pt_block;
    BACNET_CHARACTER_STRING bsTemp;
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
            SERVICE_CONFIRMED_PRIVATE_TRANSFER, max_apdu);
        /* encode the APDU portion of the packet */

        pt_block.vendorID = vendor_id;
        pt_block.serviceNumber = service_number;
        if (service_number == MY_SVC_READ) {
            len += encode_application_unsigned(&pt_req_buffer[len], block_number);      /* The block number we want to retrieve */
        } else {
            len += encode_application_unsigned(&pt_req_buffer[len], block_number);      /* The block number */
            len += encode_application_unsigned(&pt_req_buffer[len], block->cMyByte1);   /* And Then the block contents */
            len +=
                encode_application_unsigned(&pt_req_buffer[len],
                block->cMyByte2);
            len +=
                encode_application_real(&pt_req_buffer[len], block->fMyReal);
            characterstring_init_ansi(&bsTemp, block->sMyString);
            len +=
                encode_application_character_string(&pt_req_buffer[len],
                &bsTemp);
        }

        pt_block.serviceParameters = &pt_req_buffer[0];
        pt_block.serviceParametersLen = len;
        len =
            ptransfer_encode_apdu(&Handler_Transmit_Buffer[pdu_len], invoke_id,
            &pt_block);
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
            fprintf(stderr, "Failed to Send Private Transfer Request (%s)!\n",
                strerror(errno));
#endif
    }

    return invoke_id;
}
