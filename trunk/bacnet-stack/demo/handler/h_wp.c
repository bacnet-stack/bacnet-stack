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
#include "wp.h"
/* demo objects */
#include "device.h"
#include "ai.h"
#include "ao.h"
#include "av.h"
#include "bi.h"
#include "bo.h"
#include "bv.h"
#include "lsp.h"
#include "mso.h"
#if BACFILE
#include "bacfile.h"
#endif

void handler_write_property(uint8_t * service_request,
    uint16_t service_len,
    BACNET_ADDRESS * src, BACNET_CONFIRMED_SERVICE_DATA * service_data)
{
    BACNET_WRITE_PROPERTY_DATA wp_data;
    int len = 0;
    int pdu_len = 0;
    BACNET_NPDU_DATA npdu_data;
    BACNET_ERROR_CLASS error_class = ERROR_CLASS_OBJECT;
    BACNET_ERROR_CODE error_code = ERROR_CODE_UNKNOWN_OBJECT;
    int bytes_sent = 0;
    BACNET_ADDRESS my_address;

    /* decode the service request only */
    len = wp_decode_service_request(service_request,
        service_len, &wp_data);
    /* encode the NPDU portion of the packet */
    datalink_get_my_address(&my_address);
    npdu_encode_npdu_data(&npdu_data, false, MESSAGE_PRIORITY_NORMAL);
    pdu_len = npdu_encode_pdu(&Handler_Transmit_Buffer[0], src, 
        &my_address, &npdu_data);
#if PRINT_ENABLED
    fprintf(stderr, "Received Write-Property Request!\n");
    if (len > 0)
        fprintf(stderr, "type=%u instance=%u property=%u index=%d\n",
            wp_data.object_type,
            wp_data.object_instance,
            wp_data.object_property, wp_data.array_index);
    else
        fprintf(stderr, "Unable to decode Write-Property Request!\n");
#endif
    /* bad decoding or something we didn't understand - send an abort */
    if (len <= 0) {
        len = abort_encode_apdu(&Handler_Transmit_Buffer[pdu_len],
            service_data->invoke_id, ABORT_REASON_OTHER);
#if PRINT_ENABLED
        fprintf(stderr, "Sending Abort!\n");
#endif
    } else if (service_data->segmented_message) {
        len = abort_encode_apdu(&Handler_Transmit_Buffer[pdu_len],
            service_data->invoke_id,
            ABORT_REASON_SEGMENTATION_NOT_SUPPORTED);
#if PRINT_ENABLED
        fprintf(stderr, "Sending Abort!\n");
#endif
    } else {
        switch (wp_data.object_type) {
        case OBJECT_DEVICE:
            if (Device_Write_Property(&wp_data, &error_class, &error_code)) {
                len =
                    encode_simple_ack(&Handler_Transmit_Buffer[pdu_len],
                    service_data->invoke_id,
                    SERVICE_CONFIRMED_WRITE_PROPERTY);
#if PRINT_ENABLED
                fprintf(stderr,
                    "Sending Write Property Simple Ack for Device!\n");
#endif
            } else {
                len =
                    bacerror_encode_apdu(&Handler_Transmit_Buffer[pdu_len],
                    service_data->invoke_id,
                    SERVICE_CONFIRMED_WRITE_PROPERTY, error_class,
                    error_code);
#if PRINT_ENABLED
                fprintf(stderr,
                    "Sending Write Property Error for Device!\n");
#endif
            }
            break;
        case OBJECT_ANALOG_INPUT:
        case OBJECT_BINARY_INPUT:
            error_class = ERROR_CLASS_PROPERTY;
            error_code = ERROR_CODE_WRITE_ACCESS_DENIED;
            len =
                bacerror_encode_apdu(&Handler_Transmit_Buffer[pdu_len],
                service_data->invoke_id, SERVICE_CONFIRMED_WRITE_PROPERTY,
                error_class, error_code);
#if PRINT_ENABLED
            fprintf(stderr, "Sending Write Access Error!\n");
#endif
            break;
        case OBJECT_BINARY_OUTPUT:
            if (Binary_Output_Write_Property(&wp_data, &error_class,
                    &error_code)) {
                len =
                    encode_simple_ack(&Handler_Transmit_Buffer[pdu_len],
                    service_data->invoke_id,
                    SERVICE_CONFIRMED_WRITE_PROPERTY);
#if PRINT_ENABLED
                fprintf(stderr,
                    "Sending Write Property Simple Ack for BO!\n");
#endif
            } else {
                len =
                    bacerror_encode_apdu(&Handler_Transmit_Buffer[pdu_len],
                    service_data->invoke_id,
                    SERVICE_CONFIRMED_WRITE_PROPERTY, error_class,
                    error_code);
#if PRINT_ENABLED
                fprintf(stderr, "Sending Write Access Error for BO!\n");
#endif
            }
            break;
        case OBJECT_BINARY_VALUE:
            if (Binary_Value_Write_Property(&wp_data, &error_class,
                    &error_code)) {
                len =
                    encode_simple_ack(&Handler_Transmit_Buffer[pdu_len],
                    service_data->invoke_id,
                    SERVICE_CONFIRMED_WRITE_PROPERTY);
#if PRINT_ENABLED
                fprintf(stderr,
                    "Sending Write Property Simple Ack for BV!\n");
#endif
            } else {
                len =
                    bacerror_encode_apdu(&Handler_Transmit_Buffer[pdu_len],
                    service_data->invoke_id,
                    SERVICE_CONFIRMED_WRITE_PROPERTY, error_class,
                    error_code);
#if PRINT_ENABLED
                fprintf(stderr, "Sending Write Access Error for BV!\n");
#endif
            }
            break;
        case OBJECT_ANALOG_OUTPUT:
            if (Analog_Output_Write_Property(&wp_data, &error_class,
                    &error_code)) {
                len =
                    encode_simple_ack(&Handler_Transmit_Buffer[pdu_len],
                    service_data->invoke_id,
                    SERVICE_CONFIRMED_WRITE_PROPERTY);
#if PRINT_ENABLED
                fprintf(stderr,
                    "Sending Write Property Simple Ack for AO!\n");
#endif
            } else {
                len =
                    bacerror_encode_apdu(&Handler_Transmit_Buffer[pdu_len],
                    service_data->invoke_id,
                    SERVICE_CONFIRMED_WRITE_PROPERTY, error_class,
                    error_code);
#if PRINT_ENABLED
                fprintf(stderr, "Sending Write Access Error for AO!\n");
#endif
            }
            break;
        case OBJECT_ANALOG_VALUE:
            if (Analog_Value_Write_Property(&wp_data, &error_class,
                    &error_code)) {
                len =
                    encode_simple_ack(&Handler_Transmit_Buffer[pdu_len],
                    service_data->invoke_id,
                    SERVICE_CONFIRMED_WRITE_PROPERTY);
#if PRINT_ENABLED
                fprintf(stderr,
                    "Sending Write Property Simple Ack for AV!\n");
#endif
            } else {
                len =
                    bacerror_encode_apdu(&Handler_Transmit_Buffer[pdu_len],
                    service_data->invoke_id,
                    SERVICE_CONFIRMED_WRITE_PROPERTY, error_class,
                    error_code);
#if PRINT_ENABLED
                fprintf(stderr, "Sending Write Access Error for AV!\n");
#endif
            }
            break;
        case OBJECT_LIFE_SAFETY_POINT:
            if (Life_Safety_Point_Write_Property(&wp_data, &error_class,
                    &error_code)) {
                len =
                    encode_simple_ack(&Handler_Transmit_Buffer[pdu_len],
                    service_data->invoke_id,
                    SERVICE_CONFIRMED_WRITE_PROPERTY);
#if PRINT_ENABLED
                fprintf(stderr,
                    "Sending Write Property Simple Ack for LSP!\n");
#endif
            } else {
                len =
                    bacerror_encode_apdu(&Handler_Transmit_Buffer[pdu_len],
                    service_data->invoke_id,
                    SERVICE_CONFIRMED_WRITE_PROPERTY, error_class,
                    error_code);
#if PRINT_ENABLED
                fprintf(stderr, "Sending Write Access Error for LSP!\n");
#endif
            }
            break;
        case OBJECT_MULTI_STATE_OUTPUT:
            if (Multistate_Output_Write_Property(&wp_data, &error_class,
                    &error_code)) {
                len =
                    encode_simple_ack(&Handler_Transmit_Buffer[pdu_len],
                    service_data->invoke_id,
                    SERVICE_CONFIRMED_WRITE_PROPERTY);
#if PRINT_ENABLED
                fprintf(stderr,
                    "Sending Write Property Simple Ack for MSO!\n");
#endif
            } else {
                len =
                    bacerror_encode_apdu(&Handler_Transmit_Buffer[pdu_len],
                    service_data->invoke_id,
                    SERVICE_CONFIRMED_WRITE_PROPERTY, error_class,
                    error_code);
#if PRINT_ENABLED
                fprintf(stderr, "Sending Write Access Error for MSO!\n");
#endif
            }
            break;
#if BACFILE
        case OBJECT_FILE:
            if (bacfile_write_property(&wp_data, &error_class,
                    &error_code)) {
                len =
                    encode_simple_ack(&Handler_Transmit_Buffer[pdu_len],
                    service_data->invoke_id,
                    SERVICE_CONFIRMED_WRITE_PROPERTY);
#if PRINT_ENABLED
                fprintf(stderr,
                    "Sending Write Property Simple Ack for File!\n");
#endif
            } else {
                len =
                    bacerror_encode_apdu(&Handler_Transmit_Buffer[pdu_len],
                    service_data->invoke_id,
                    SERVICE_CONFIRMED_WRITE_PROPERTY, error_class,
                    error_code);
#if PRINT_ENABLED
                fprintf(stderr, "Sending Write Access Error for File!\n");
#endif
            }
            break;
#endif
        default:
            len =
                bacerror_encode_apdu(&Handler_Transmit_Buffer[pdu_len],
                service_data->invoke_id, SERVICE_CONFIRMED_WRITE_PROPERTY,
                error_class, error_code);
#if PRINT_ENABLED
            fprintf(stderr, "Sending Unknown Object Error!\n");
#endif
            break;
        }
    }
    pdu_len += len;
    bytes_sent = datalink_send_pdu(src, &npdu_data,
        &Handler_Transmit_Buffer[0], pdu_len);
#if PRINT_ENABLED
    if (bytes_sent <= 0)
        fprintf(stderr, "Failed to send PDU (%s)!\n", strerror(errno));
#endif

    return;
}
