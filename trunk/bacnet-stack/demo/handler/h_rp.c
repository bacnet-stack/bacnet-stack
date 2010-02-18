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
#include "config.h"
#include "txbuf.h"
#include "bacdef.h"
#include "bacdcode.h"
#include "bacerror.h"
#include "bacdevobjpropref.h"
#include "apdu.h"
#include "npdu.h"
#include "abort.h"
#include "rp.h"
/* device object has custom handler for all objects */
#include "device.h"

/** @file h_rp.c  Handles Read Property requests. */

void handler_read_property(
    uint8_t * service_request,
    uint16_t service_len,
    BACNET_ADDRESS * src,
    BACNET_CONFIRMED_SERVICE_DATA * service_data)
{
    BACNET_READ_PROPERTY_DATA rpdata;
    int len = 0;
    int pdu_len = 0;
    int apdu_len = -1;
    int npdu_len = -1;
    BACNET_NPDU_DATA npdu_data;
    bool error = false;
    int bytes_sent = 0;
    BACNET_ADDRESS my_address;

    /* encode the NPDU portion of the packet */
    datalink_get_my_address(&my_address);
    npdu_encode_npdu_data(&npdu_data, false, MESSAGE_PRIORITY_NORMAL);
    npdu_len =
        npdu_encode_pdu(&Handler_Transmit_Buffer[0], src, &my_address,
        &npdu_data);
    if (service_data->segmented_message) {
        /* we don't support segmentation - send an abort */
        apdu_len =
            abort_encode_apdu(&Handler_Transmit_Buffer[npdu_len],
            service_data->invoke_id, ABORT_REASON_SEGMENTATION_NOT_SUPPORTED,
            true);
#if PRINT_ENABLED
        fprintf(stderr, "RP: Segmented message.  Sending Abort!\n");
#endif
        goto RP_ABORT;
    }
    len = rp_decode_service_request(service_request, service_len, &rpdata);
#if PRINT_ENABLED
    if (len <= 0) {
        fprintf(stderr, "RP: Unable to decode Request!\n");
    }
#endif
    if (len < 0) {
        /* bad decoding - send an abort */
        apdu_len =
            abort_encode_apdu(&Handler_Transmit_Buffer[npdu_len],
            service_data->invoke_id, ABORT_REASON_OTHER, true);
#if PRINT_ENABLED
        fprintf(stderr, "RP: Bad Encoding.  Sending Abort!\n");
#endif
        goto RP_ABORT;
    }
    /* assume that there is an error */
    error = true;
    apdu_len = rp_ack_encode_apdu_init(
        &Handler_Transmit_Buffer[npdu_len],
        service_data->invoke_id,
        &rpdata);
    /* configure our storage */
    rpdata.application_data = &Handler_Transmit_Buffer[npdu_len+apdu_len];
    rpdata.application_data_len = 
        sizeof(Handler_Transmit_Buffer) - (npdu_len + apdu_len);
    len =  Device_Read_Property(&rpdata);
    if (len >= 0) {
        apdu_len += len;
        len = rp_ack_encode_apdu_object_property_end(
            &Handler_Transmit_Buffer[npdu_len+apdu_len]);
        apdu_len += len;
        if (apdu_len > service_data->max_resp) {
            /* too big for the sender - send an abort */
            apdu_len =
                abort_encode_apdu(&Handler_Transmit_Buffer[npdu_len],
                service_data->invoke_id, ABORT_REASON_SEGMENTATION_NOT_SUPPORTED,
                true);
#if PRINT_ENABLED
            fprintf(stderr, "RP: Message too large.  Sending Abort!\n");
#endif
            goto RP_ABORT;
        } else {
#if PRINT_ENABLED
            fprintf(stderr, "RP: Sending Ack!\n");
#endif
            error = false;
        }
    }
    if (error) {
        if (len == -2) {
            /* BACnet APDU too small to fit data, so proper response is Abort */
            apdu_len =
                abort_encode_apdu(&Handler_Transmit_Buffer[npdu_len],
                service_data->invoke_id,
                ABORT_REASON_SEGMENTATION_NOT_SUPPORTED, true);
#if PRINT_ENABLED
            fprintf(stderr, "RP: Reply too big to fit into APDU!\n");
#endif
        } else {
            apdu_len =
                bacerror_encode_apdu(&Handler_Transmit_Buffer[npdu_len],
                service_data->invoke_id, SERVICE_CONFIRMED_READ_PROPERTY,
                rpdata.error_class, rpdata.error_code);
#if PRINT_ENABLED
            fprintf(stderr, "RP: Sending Error!\n");
#endif
        }
    }    
  RP_ABORT:
    pdu_len = npdu_len + apdu_len;
    bytes_sent =
        datalink_send_pdu(src, &npdu_data, &Handler_Transmit_Buffer[0], 
        pdu_len);
#if PRINT_ENABLED
    if (bytes_sent <= 0)
        fprintf(stderr, "Failed to send PDU (%s)!\n", strerror(errno));
#endif

    return;
}
