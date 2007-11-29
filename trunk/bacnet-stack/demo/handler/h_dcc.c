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
#include "reject.h"
#include "dcc.h"

static char *My_Password = "filister";

void handler_device_communication_control(
    uint8_t * service_request,
    uint16_t service_len,
    BACNET_ADDRESS * src,
    BACNET_CONFIRMED_SERVICE_DATA * service_data)
{
    uint16_t timeDuration = 0;
    BACNET_COMMUNICATION_ENABLE_DISABLE state = COMMUNICATION_ENABLE;
    BACNET_CHARACTER_STRING password;
    int len = 0;
    int pdu_len = 0;
    int bytes_sent = 0;
    BACNET_NPDU_DATA npdu_data;
    BACNET_ADDRESS my_address;

    /* encode the NPDU portion of the reply packet */
    datalink_get_my_address(&my_address);
    npdu_encode_npdu_data(&npdu_data, false, MESSAGE_PRIORITY_NORMAL);
    pdu_len =
        npdu_encode_pdu(&Handler_Transmit_Buffer[0], src, &my_address,
        &npdu_data);
#if PRINT_ENABLED
    fprintf(stderr, "DeviceCommunicationControl!\n");
#endif
    if (service_data->segmented_message) {
        len =
            abort_encode_apdu(&Handler_Transmit_Buffer[pdu_len],
            service_data->invoke_id, ABORT_REASON_SEGMENTATION_NOT_SUPPORTED,
            true);
#if PRINT_ENABLED
        fprintf(stderr,
            "DeviceCommunicationControl: "
            "Sending Abort - segmented message.\n");
#endif
        goto DCC_ABORT;
    }
    /* decode the service request only */
    len =
        dcc_decode_service_request(service_request, service_len, &timeDuration,
        &state, &password);
#if PRINT_ENABLED
    if (len > 0)
        fprintf(stderr,
            "DeviceCommunicationControl: " "timeout=%u state=%u password=%s\n",
            (unsigned) timeDuration, (unsigned) state,
            characterstring_value(&password));
#endif
    /* bad decoding or something we didn't understand - send an abort */
    if (len < 0) {
        len =
            abort_encode_apdu(&Handler_Transmit_Buffer[pdu_len],
            service_data->invoke_id, ABORT_REASON_OTHER, true);
#if PRINT_ENABLED
        fprintf(stderr,
            "DeviceCommunicationControl: "
            "Sending Abort - could not decode.\n");
#endif
        goto DCC_ABORT;
    }
    if (state >= MAX_BACNET_COMMUNICATION_ENABLE_DISABLE) {
        len =
            reject_encode_apdu(&Handler_Transmit_Buffer[pdu_len],
            service_data->invoke_id, REJECT_REASON_UNDEFINED_ENUMERATION);
#if PRINT_ENABLED
        fprintf(stderr,
            "DeviceCommunicationControl: "
            "Sending Reject - undefined enumeration\n");
#endif
    } else {
        if (characterstring_ansi_same(&password, My_Password)) {
            len =
                encode_simple_ack(&Handler_Transmit_Buffer[pdu_len],
                service_data->invoke_id,
                SERVICE_CONFIRMED_DEVICE_COMMUNICATION_CONTROL);
#if PRINT_ENABLED
            fprintf(stderr,
                "DeviceCommunicationControl: " "Sending Simple Ack!\n");
#endif
            dcc_set_status_duration(state, timeDuration);
        } else {
            len =
                bacerror_encode_apdu(&Handler_Transmit_Buffer[pdu_len],
                service_data->invoke_id,
                SERVICE_CONFIRMED_DEVICE_COMMUNICATION_CONTROL,
                ERROR_CLASS_SERVICES, ERROR_CODE_PASSWORD_FAILURE);
#if PRINT_ENABLED
            fprintf(stderr,
                "DeviceCommunicationControl: "
                "Sending Error - password failure.\n");
#endif
        }
    }
  DCC_ABORT:
    pdu_len += len;
    bytes_sent =
        datalink_send_pdu(src, &npdu_data, &Handler_Transmit_Buffer[0],
        pdu_len);
#if PRINT_ENABLED
    if (bytes_sent <= 0)
        fprintf(stderr,
            "DeviceCommunicationControl: " "Failed to send PDU (%s)!\n",
            strerror(errno));
#endif

    return;
}
