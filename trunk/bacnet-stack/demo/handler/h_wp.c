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
#include "apdu.h"
#include "npdu.h"
#include "abort.h"
#include "wp.h"

static write_property_function
    Write_Property[MAX_BACNET_OBJECT_TYPE];
                
void handler_write_property_object_set(
    BACNET_OBJECT_TYPE object_type,
    write_property_function pFunction)
{
    if (object_type < MAX_BACNET_OBJECT_TYPE) {
        Write_Property[object_type] = pFunction;
    }
}                

void handler_write_property(
    uint8_t * service_request,
    uint16_t service_len,
    BACNET_ADDRESS * src,
    BACNET_CONFIRMED_SERVICE_DATA * service_data)
{
    BACNET_WRITE_PROPERTY_DATA wp_data;
    int len = 0;
    int pdu_len = 0;
    BACNET_NPDU_DATA npdu_data;
    BACNET_ERROR_CLASS error_class = ERROR_CLASS_OBJECT;
    BACNET_ERROR_CODE error_code = ERROR_CODE_UNKNOWN_OBJECT;
    int bytes_sent = 0;
    BACNET_ADDRESS my_address;
    write_property_function wp_function = NULL;

    /* encode the NPDU portion of the packet */
    datalink_get_my_address(&my_address);
    npdu_encode_npdu_data(&npdu_data, false, MESSAGE_PRIORITY_NORMAL);
    pdu_len =
        npdu_encode_pdu(&Handler_Transmit_Buffer[0], src, &my_address,
        &npdu_data);
#if PRINT_ENABLED
    fprintf(stderr, "WP: Received Request!\n");
#endif
    if (service_data->segmented_message) {
        len =
            abort_encode_apdu(&Handler_Transmit_Buffer[pdu_len],
            service_data->invoke_id, ABORT_REASON_SEGMENTATION_NOT_SUPPORTED,
            true);
#if PRINT_ENABLED
        fprintf(stderr, "WP: Segmented message.  Sending Abort!\n");
#endif
        goto WP_ABORT;
    }   /* decode the service request only */
    len = wp_decode_service_request(service_request, service_len, &wp_data);
#if PRINT_ENABLED
    if (len > 0)
        fprintf(stderr, "WP: type=%u instance=%u property=%u index=%d\n",
            wp_data.object_type, wp_data.object_instance,
            wp_data.object_property, wp_data.array_index);
    else
        fprintf(stderr, "WP: Unable to decode Request!\n");
#endif
    /* bad decoding or something we didn't understand - send an abort */
    if (len <= 0) {
        len =
            abort_encode_apdu(&Handler_Transmit_Buffer[pdu_len],
            service_data->invoke_id, ABORT_REASON_OTHER, true);
#if PRINT_ENABLED
        fprintf(stderr, "WP: Bad Encoding. Sending Abort!\n");
#endif
        goto WP_ABORT;
    }
    if (wp_data.object_type < MAX_BACNET_OBJECT_TYPE) {
        wp_function = Write_Property[wp_data.object_type];
    }
    if (wp_function) {
        if (wp_function(&wp_data, &error_class, &error_code)) {
            len =
                encode_simple_ack(&Handler_Transmit_Buffer[pdu_len],
                service_data->invoke_id, SERVICE_CONFIRMED_WRITE_PROPERTY);
#if PRINT_ENABLED
            fprintf(stderr, "WP: Sending Simple Ack!\n");
#endif
        } else {
            len =
                bacerror_encode_apdu(&Handler_Transmit_Buffer[pdu_len],
                service_data->invoke_id, SERVICE_CONFIRMED_WRITE_PROPERTY,
                error_class, error_code);
#if PRINT_ENABLED
            fprintf(stderr, "WP: Sending Error!\n");
#endif
        }
    } else {
        len =
            bacerror_encode_apdu(&Handler_Transmit_Buffer[pdu_len],
            service_data->invoke_id, SERVICE_CONFIRMED_WRITE_PROPERTY,
            error_class, error_code);
#if PRINT_ENABLED
        fprintf(stderr, "WP: Sending Unknown Object Error!\n");
#endif
    }
  WP_ABORT:
    pdu_len += len;
    bytes_sent =
        datalink_send_pdu(src, &npdu_data, &Handler_Transmit_Buffer[0],
        pdu_len);
#if PRINT_ENABLED
    if (bytes_sent <= 0)
        fprintf(stderr, "WP: Failed to send PDU (%s)!\n", strerror(errno));
#endif

    return;
}
