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
#include "apdu.h"
#include "npdu.h"
#include "abort.h"
#include "arf.h"
/* demo objects */
#include "device.h"
#include "ai.h"
#include "ao.h"
#include "bacfile.h"

void handler_atomic_read_file(uint8_t * service_request,
    uint16_t service_len,
    BACNET_ADDRESS * src, BACNET_CONFIRMED_SERVICE_DATA * service_data)
{
    BACNET_ATOMIC_READ_FILE_DATA data;
    int len = 0;
    int pdu_len = 0;
    BACNET_ADDRESS my_address;
    bool send = false;
    bool error = false;
    int bytes_sent = 0;
    BACNET_ERROR_CLASS error_class = ERROR_CLASS_OBJECT;
    BACNET_ERROR_CODE error_code = ERROR_CODE_UNKNOWN_OBJECT;

    fprintf(stderr, "Received Atomic-Read-File Request!\n");
    len = arf_decode_service_request(service_request, service_len, &data);
    if (len < 0)
        fprintf(stderr, "Unable to decode Atomic-Read-File Request!\n");
    /* prepare a reply */
    datalink_get_my_address(&my_address);
    /* encode the NPDU portion of the packet */
    pdu_len = npdu_encode_apdu(&Handler_Transmit_Buffer[0], src, &my_address, false,    /* true for confirmed messages */
        MESSAGE_PRIORITY_NORMAL);
    /* bad decoding - send an abort */
    if (len < 0) {
        pdu_len += abort_encode_apdu(&Handler_Transmit_Buffer[pdu_len],
            service_data->invoke_id, ABORT_REASON_OTHER);
        fprintf(stderr, "Sending Abort!\n");
        send = true;
    } else if (service_data->segmented_message) {
        pdu_len += abort_encode_apdu(&Handler_Transmit_Buffer[pdu_len],
            service_data->invoke_id,
            ABORT_REASON_SEGMENTATION_NOT_SUPPORTED);
        fprintf(stderr, "Sending Abort!\n");
        send = true;
    } else {
        if (data.access == FILE_STREAM_ACCESS) {
            if (data.type.stream.requestedOctetCount <
                octetstring_capacity(&data.fileData)) {
                if (bacfile_read_data(&data)) {
                    pdu_len +=
                        arf_ack_encode_apdu(&Handler_Transmit_Buffer
                        [pdu_len], service_data->invoke_id, &data);
                    send = true;
                } else {
                    send = true;
                    error = true;
                }
            } else {
                pdu_len +=
                    abort_encode_apdu(&Handler_Transmit_Buffer[pdu_len],
                    service_data->invoke_id,
                    ABORT_REASON_SEGMENTATION_NOT_SUPPORTED);
                fprintf(stderr, "Sending Abort!\n");
                send = true;
            }
        } else {
            error_class = ERROR_CLASS_SERVICES;
            error_code = ERROR_CODE_INVALID_FILE_ACCESS_METHOD;
            send = true;
            error = true;
        }
    }
    if (error) {
        pdu_len += bacerror_encode_apdu(&Handler_Transmit_Buffer[pdu_len],
            service_data->invoke_id,
            SERVICE_CONFIRMED_ATOMIC_READ_FILE, error_class, error_code);
        fprintf(stderr, "Sending Error!\n");
        send = true;
    }
    if (send) {
        bytes_sent = datalink_send_pdu(src,     /* destination address */
            &Handler_Transmit_Buffer[0], pdu_len);      /* number of bytes of data */
        if (bytes_sent <= 0)
            fprintf(stderr, "Failed to send PDU (%s)!\n", strerror(errno));
    }

    return;
}
