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
#include "rp.h"
/* demo objects */
#include "device.h"
#include "ai.h"
#include "av.h"
#include "bi.h"
#include "bv.h"

/* note: this is a minimal handler.  See h_rp.c for another */

static uint8_t Temp_Buf[MAX_APDU] = { 0 };

void handler_read_property(uint8_t * service_request,
    uint16_t service_len,
    BACNET_ADDRESS * src, BACNET_CONFIRMED_SERVICE_DATA * service_data)
{
    BACNET_READ_PROPERTY_DATA data;
    int len = 0;
    int pdu_len = 0;
    BACNET_NPDU_DATA npdu_data;
    bool send = false;
    bool error = false;
    int bytes_sent = 0;
    BACNET_ERROR_CLASS error_class = ERROR_CLASS_OBJECT;
    BACNET_ERROR_CODE error_code = ERROR_CODE_UNKNOWN_OBJECT;
    BACNET_ADDRESS my_address;

    /* encode the NPDU portion of the packet */
    datalink_get_my_address(&my_address);
    npdu_encode_npdu_data(&npdu_data, false, MESSAGE_PRIORITY_NORMAL);
    pdu_len = npdu_encode_pdu(&Handler_Transmit_Buffer[0], src,
        &my_address, &npdu_data);
    if (service_data->segmented_message) {
        /* we don't support segmentation - send an abort */
        len = abort_encode_apdu(&Handler_Transmit_Buffer[pdu_len],
            service_data->invoke_id,
            ABORT_REASON_SEGMENTATION_NOT_SUPPORTED, true);
        goto RP_ABORT;
    }
    len = rp_decode_service_request(service_request, service_len, &data);
    if (len < 0) {
        /* bad decoding - send an abort */
        len = abort_encode_apdu(&Handler_Transmit_Buffer[pdu_len],
            service_data->invoke_id, ABORT_REASON_OTHER, true);
        goto RP_ABORT;
    }
    /* most cases will be error */
    error = true;
    switch (data.object_type) {
    case OBJECT_DEVICE:
        /* FIXME: probably need a length limitation sent with encode */
        if (Device_Valid_Object_Instance_Number(data.object_instance)) {
            len = Device_Encode_Property_APDU(&Temp_Buf[0],
                data.object_property,
                data.array_index, &error_class, &error_code);
            if (len >= 0) {
                /* encode the APDU portion of the packet */
                data.application_data = &Temp_Buf[0];
                data.application_data_len = len;
                /* FIXME: probably need a length limitation sent with encode */
                len =
                    rp_ack_encode_apdu(&Handler_Transmit_Buffer
                    [pdu_len], service_data->invoke_id, &data);
                error = false;
            }
        }
        break;
    case OBJECT_ANALOG_INPUT:
        if (Analog_Input_Valid_Instance(data.object_instance)) {
            len = Analog_Input_Encode_Property_APDU(&Temp_Buf[0],
                data.object_instance,
                data.object_property,
                data.array_index, &error_class, &error_code);
            if (len >= 0) {
                /* encode the APDU portion of the packet */
                data.application_data = &Temp_Buf[0];
                data.application_data_len = len;
                /* FIXME: probably need a length limitation sent with encode */
                len =
                    rp_ack_encode_apdu(&Handler_Transmit_Buffer
                    [pdu_len], service_data->invoke_id, &data);
                error = false;
            }
        }
        break;
    case OBJECT_BINARY_INPUT:
        if (Binary_Input_Valid_Instance(data.object_instance)) {
            len = Binary_Input_Encode_Property_APDU(&Temp_Buf[0],
                data.object_instance,
                data.object_property,
                data.array_index, &error_class, &error_code);
            if (len >= 0) {
                /* encode the APDU portion of the packet */
                data.application_data = &Temp_Buf[0];
                data.application_data_len = len;
                /* FIXME: probably need a length limitation sent with encode */
                len =
                    rp_ack_encode_apdu(&Handler_Transmit_Buffer
                    [pdu_len], service_data->invoke_id, &data);
                error = false;
            }
        }
        break;
    case OBJECT_BINARY_VALUE:
        if (Binary_Value_Valid_Instance(data.object_instance)) {
            len = Binary_Value_Encode_Property_APDU(&Temp_Buf[0],
                data.object_instance,
                data.object_property,
                data.array_index, &error_class, &error_code);
            if (len >= 0) {
                /* encode the APDU portion of the packet */
                data.application_data = &Temp_Buf[0];
                data.application_data_len = len;
                /* FIXME: probably need a length limitation sent with encode */
                len =
                    rp_ack_encode_apdu(&Handler_Transmit_Buffer
                    [pdu_len], service_data->invoke_id, &data);
                error = false;
            }
        }
        break;
    case OBJECT_ANALOG_VALUE:
        if (Analog_Value_Valid_Instance(data.object_instance)) {
            len = Analog_Value_Encode_Property_APDU(&Temp_Buf[0],
                data.object_instance,
                data.object_property,
                data.array_index, &error_class, &error_code);
            if (len >= 0) {
                /* encode the APDU portion of the packet */
                data.application_data = &Temp_Buf[0];
                data.application_data_len = len;
                /* FIXME: probably need a length limitation sent with encode */
                len =
                    rp_ack_encode_apdu(&Handler_Transmit_Buffer
                    [pdu_len], service_data->invoke_id, &data);
                error = false;
            }
        }
        break;
    default:
        break;
    }
    if (error) {
        if (len == -2) {
            /* BACnet APDU too small to fit data, so proper response is Abort */
            len = abort_encode_apdu(&Handler_Transmit_Buffer[pdu_len],
                service_data->invoke_id,
                ABORT_REASON_SEGMENTATION_NOT_SUPPORTED, true);
            goto RP_ABORT;
        }
        len = bacerror_encode_apdu(&Handler_Transmit_Buffer[pdu_len],
            service_data->invoke_id,
            SERVICE_CONFIRMED_READ_PROPERTY, error_class, error_code);
    }
RP_ABORT:
    pdu_len += len;
    bytes_sent = datalink_send_pdu(src, &npdu_data,
        &Handler_Transmit_Buffer[0], pdu_len);

    return;
}
