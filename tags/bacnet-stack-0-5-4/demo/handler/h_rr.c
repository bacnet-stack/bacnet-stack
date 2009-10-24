/**************************************************************************
*
* Copyright (C) 2009 Steve Karg <skarg@users.sourceforge.net>
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
#include "apdu.h"
#include "npdu.h"
#include "abort.h"
#include "readrange.h"

static uint8_t Temp_Buf[MAX_APDU] = { 0 };

/* Encodes the property APDU and returns the length,
   or sets the error, and returns -1 */
int Encode_RR_payload(
    uint8_t * apdu,
    BACNET_READ_RANGE_DATA * pRequest,
    BACNET_ERROR_CLASS * error_class,
    BACNET_ERROR_CODE * error_code)
{
    int apdu_len = -1;

    /* FIXME: Stub function at the moment which just returns something
     * for the sake of testing things out. Need to look at existing objects
     * and see how we can do this for real. 
     */

    pRequest->ItemCount = 6;
    bitstring_init(&pRequest->ResultFlags);
    bitstring_set_bit(&pRequest->ResultFlags, RESULT_FLAG_FIRST_ITEM, true);
    bitstring_set_bit(&pRequest->ResultFlags, RESULT_FLAG_LAST_ITEM, true);
    bitstring_set_bit(&pRequest->ResultFlags, RESULT_FLAG_MORE_ITEMS, false);
    pRequest->FirstSequence = 0;

    apdu_len = 0;
    apdu_len += encode_application_unsigned(&apdu[apdu_len], 1);
    apdu_len += encode_application_unsigned(&apdu[apdu_len], 2);
    apdu_len += encode_application_unsigned(&apdu[apdu_len], 3);
    apdu_len += encode_application_unsigned(&apdu[apdu_len], 4);
    apdu_len += encode_application_unsigned(&apdu[apdu_len], 5);
    apdu_len += encode_application_unsigned(&apdu[apdu_len], 6);

    return apdu_len;
}

void handler_read_range(
    uint8_t * service_request,
    uint16_t service_len,
    BACNET_ADDRESS * src,
    BACNET_CONFIRMED_SERVICE_DATA * service_data)
{
    BACNET_READ_RANGE_DATA data;
    int len = 0;
    int pdu_len = 0;
    BACNET_NPDU_DATA npdu_data;
    bool error = false;
    int bytes_sent = 0;
    BACNET_ERROR_CLASS error_class = ERROR_CLASS_OBJECT;
    BACNET_ERROR_CODE error_code = ERROR_CODE_UNKNOWN_OBJECT;
    BACNET_ADDRESS my_address;

    /* encode the NPDU portion of the packet */
    datalink_get_my_address(&my_address);
    npdu_encode_npdu_data(&npdu_data, false, MESSAGE_PRIORITY_NORMAL);
    pdu_len =
        npdu_encode_pdu(&Handler_Transmit_Buffer[0], src, &my_address,
        &npdu_data);
    if (service_data->segmented_message) {
        /* we don't support segmentation - send an abort */
        len =
            abort_encode_apdu(&Handler_Transmit_Buffer[pdu_len],
            service_data->invoke_id, ABORT_REASON_SEGMENTATION_NOT_SUPPORTED,
            true);
#if PRINT_ENABLED
        fprintf(stderr, "RR: Segmented message.  Sending Abort!\n");
#endif
        goto RR_ABORT;
    }

    len = rr_decode_service_request(service_request, service_len, &data);
#if PRINT_ENABLED
    if (len <= 0)
        fprintf(stderr, "RR: Unable to decode Request!\n");
#endif
    if (len < 0) {
        /* bad decoding - send an abort */
        len =
            abort_encode_apdu(&Handler_Transmit_Buffer[pdu_len],
            service_data->invoke_id, ABORT_REASON_OTHER, true);
#if PRINT_ENABLED
        fprintf(stderr, "RR: Bad Encoding.  Sending Abort!\n");
#endif
        goto RR_ABORT;
    }

    /* assume that there is an error */
    error = true;
    len = Encode_RR_payload(&Temp_Buf[0], &data, &error_class, &error_code);
    if (len >= 0) {
        /* encode the APDU portion of the packet */
        data.application_data = &Temp_Buf[0];
        data.application_data_len = len;
        /* FIXME: probably need a length limitation sent with encode */
        len =
            rr_ack_encode_apdu(&Handler_Transmit_Buffer[pdu_len],
            service_data->invoke_id, &data);
#if PRINT_ENABLED
        fprintf(stderr, "RR: Sending Ack!\n");
#endif
        error = false;
    }
    if (error) {
        if (len == -2) {
            /* BACnet APDU too small to fit data, so proper response is Abort */
            len =
                abort_encode_apdu(&Handler_Transmit_Buffer[pdu_len],
                service_data->invoke_id,
                ABORT_REASON_SEGMENTATION_NOT_SUPPORTED, true);
#if PRINT_ENABLED
            fprintf(stderr, "RR: Reply too big to fit into APDU!\n");
#endif
        } else {
            len =
                bacerror_encode_apdu(&Handler_Transmit_Buffer[pdu_len],
                service_data->invoke_id, SERVICE_CONFIRMED_READ_PROPERTY,
                error_class, error_code);
#if PRINT_ENABLED
            fprintf(stderr, "RR: Sending Error!\n");
#endif
        }
    }
  RR_ABORT:
    pdu_len += len;
    bytes_sent =
        datalink_send_pdu(src, &npdu_data, &Handler_Transmit_Buffer[0],
        pdu_len);
#if PRINT_ENABLED
    if (bytes_sent <= 0)
        fprintf(stderr, "Failed to send PDU (%s)!\n", strerror(errno));
#endif

    return;
}
