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
    bool error = false;
    int bytes_sent = 0;
    BACNET_NPDU_DATA npdu_data;

#if PRINT_ENABLED
    fprintf(stderr, "Received Atomic-Read-File Request!\n");
#endif
    len = arf_decode_service_request(service_request, service_len, &data);
    /* prepare a reply */
    /* bad decoding - send an abort */
    if (len < 0) {
        pdu_len = abort_encode_apdu(&Handler_Transmit_Buffer[0],
            service_data->invoke_id, ABORT_REASON_OTHER);
#if PRINT_ENABLED
        fprintf(stderr, "Bad Encoding. Sending Abort!\n");
#endif
    } else if (service_data->segmented_message) {
        pdu_len = abort_encode_apdu(&Handler_Transmit_Buffer[0],
            service_data->invoke_id,
            ABORT_REASON_SEGMENTATION_NOT_SUPPORTED);
#if PRINT_ENABLED
        fprintf(stderr, "Segmented Message. Sending Abort!\n");
#endif
    } else {
        if (data.access == FILE_STREAM_ACCESS) {
            if (data.type.stream.requestedOctetCount <
                octetstring_capacity(&data.fileData)) {
                if (bacfile_read_data(&data)) {
                    pdu_len =
                        arf_ack_encode_apdu(&Handler_Transmit_Buffer[0],
                        service_data->invoke_id, &data);
                } else {
                    error = true;
                }
            } else {
                pdu_len =
                    abort_encode_apdu(&Handler_Transmit_Buffer[0],
                    service_data->invoke_id,
                    ABORT_REASON_SEGMENTATION_NOT_SUPPORTED);
#if PRINT_ENABLED
                fprintf(stderr, "Too Big To Send. Sending Abort!\n");
#endif
            }
        } else {
            pdu_len =
                bacerror_encode_apdu(&Handler_Transmit_Buffer[pdu_len],
                service_data->invoke_id,
                SERVICE_CONFIRMED_ATOMIC_READ_FILE, ERROR_CLASS_SERVICES,
                ERROR_CODE_INVALID_FILE_ACCESS_METHOD);
#if PRINT_ENABLED
            fprintf(stderr, "Record Access Requested. Sending Error!\n");
#endif
        }
    }
    npdu_encode_confirmed_apdu(&npdu_data, MESSAGE_PRIORITY_NORMAL);
    bytes_sent = datalink_send_pdu(src, &npdu_data,
        &Handler_Transmit_Buffer[0], pdu_len);
#if PRINT_ENABLED
    if (bytes_sent <= 0) {
        fprintf(stderr, "Failed to send PDU (%s)!\n", strerror(errno));
    }
#endif

    return;
}
