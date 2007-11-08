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
#include "rp.h"
/* demo objects */
#include "device.h"
#include "ai.h"
#include "ao.h"
#include "av.h"
#include "bi.h"
#include "bo.h"
#include "bv.h"
#include "lc.h"
#include "lsp.h"
#include "mso.h"
#if defined(BACFILE)
    #include "bacfile.h"
#endif

static uint8_t Temp_Buf[MAX_APDU] = { 0 };

/* Encodes the property APDU and returns the length,
   or sets the error, and returns -1 */
int Encode_Property_APDU(
    uint8_t * apdu,
    BACNET_OBJECT_TYPE object_type,
    uint32_t object_instance,
    BACNET_PROPERTY_ID property,
    int32_t array_index,
    BACNET_ERROR_CLASS * error_class,
    BACNET_ERROR_CODE * error_code)
{
    int apdu_len = -1;

    /* initialize the default return values */
    *error_class = ERROR_CLASS_OBJECT;
    *error_code = ERROR_CODE_UNKNOWN_OBJECT;
    /* handle each object type */
    switch(object_type) {
        case OBJECT_DEVICE:
            if (Device_Valid_Object_Instance_Number(object_instance)) {
                apdu_len = Device_Encode_Property_APDU(
                    &apdu[0],
                    property,
                    array_index,
                    error_class, error_code);
            }
            break;
        case OBJECT_ANALOG_INPUT:
            if (Analog_Input_Valid_Instance(object_instance)) {
                apdu_len = Analog_Input_Encode_Property_APDU(
                    &apdu[0],
                    object_instance,
                    property,
                    array_index,
                    error_class, error_code);
            }
            break;
        case OBJECT_ANALOG_OUTPUT:
            if (Analog_Output_Valid_Instance(object_instance)) {
                apdu_len = Analog_Output_Encode_Property_APDU(
                    &apdu[0],
                    object_instance,
                    property,
                    array_index,
                    error_class, error_code);
            }
            break;
        case OBJECT_ANALOG_VALUE:
            if (Analog_Value_Valid_Instance(object_instance)) {
                apdu_len = Analog_Value_Encode_Property_APDU(
                    &apdu[0],
                    object_instance,
                    property,
                    array_index,
                    error_class, error_code);
            }
            break;
        case OBJECT_BINARY_INPUT:
            if (Binary_Input_Valid_Instance(object_instance)) {
                apdu_len = Binary_Input_Encode_Property_APDU(
                    &apdu[0],
                    object_instance,
                    property,
                    array_index,
                    error_class, error_code);
            }
            break;
        case OBJECT_BINARY_OUTPUT:
            if (Binary_Output_Valid_Instance(object_instance)) {
                apdu_len = Binary_Output_Encode_Property_APDU(
                    &apdu[0],
                    object_instance,
                    property,
                    array_index,
                    error_class, error_code);
            }
            break;
        case OBJECT_BINARY_VALUE:
            if (Binary_Value_Valid_Instance(object_instance)) {
                apdu_len = Binary_Value_Encode_Property_APDU(
                    &apdu[0],
                    object_instance,
                    property,
                    array_index,
                    error_class, error_code);
            }
            break;
        case OBJECT_LIFE_SAFETY_POINT:
            if (Life_Safety_Point_Valid_Instance(object_instance)) {
                apdu_len = Life_Safety_Point_Encode_Property_APDU(
                    &apdu[0],
                    object_instance,
                    property,
                    array_index,
                    error_class, error_code);
            }
            break;
        case OBJECT_LOAD_CONTROL:
            if (Load_Control_Valid_Instance(object_instance)) {
                apdu_len = Load_Control_Encode_Property_APDU(
                    &apdu[0],
                    object_instance,
                    property,
                    array_index,
                    error_class, error_code);
            }
            break;
        case OBJECT_MULTI_STATE_OUTPUT:
            if (Multistate_Output_Valid_Instance(object_instance)) {
                apdu_len = Multistate_Output_Encode_Property_APDU(
                    &apdu[0],
                    object_instance,
                    property,
                    array_index,
                    error_class, error_code);
            }
            break;
#if defined(BACFILE)
        case OBJECT_FILE:
            if (bacfile_valid_instance(object_instance)) {
                apdu_len = bacfile_encode_property_apdu(
                    &apdu[0],
                    object_instance,
                    property,
                    array_index,
                    error_class, error_code);
            }
            break;
#endif
        default:
            *error_class = ERROR_CLASS_OBJECT;
            *error_code = ERROR_CODE_UNSUPPORTED_OBJECT_TYPE;
            break;
    }

    return apdu_len;
}

void handler_read_property(uint8_t * service_request,
    uint16_t service_len,
    BACNET_ADDRESS * src, BACNET_CONFIRMED_SERVICE_DATA * service_data)
{
    BACNET_READ_PROPERTY_DATA data;
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
    pdu_len = npdu_encode_pdu(&Handler_Transmit_Buffer[0], src,
        &my_address, &npdu_data);
    if (service_data->segmented_message) {
        /* we don't support segmentation - send an abort */
        len = abort_encode_apdu(&Handler_Transmit_Buffer[pdu_len],
            service_data->invoke_id,
            ABORT_REASON_SEGMENTATION_NOT_SUPPORTED, true);
#if PRINT_ENABLED
        fprintf(stderr, "RP: Segmented message.  Sending Abort!\n");
#endif
        goto RP_ABORT;
    }

    len = rp_decode_service_request(service_request, service_len, &data);
#if PRINT_ENABLED
    if (len <= 0)
        fprintf(stderr, "RP: Unable to decode Request!\n");
#endif
    if (len < 0) {
        /* bad decoding - send an abort */
        len = abort_encode_apdu(&Handler_Transmit_Buffer[pdu_len],
            service_data->invoke_id, ABORT_REASON_OTHER, true);
#if PRINT_ENABLED
        fprintf(stderr, "RP: Bad Encoding.  Sending Abort!\n");
#endif
        goto RP_ABORT;
    }

    /* assume that there is an error */
    error = true;
    len = Encode_Property_APDU(
        &Temp_Buf[0],
        data.object_type,
        data.object_instance,
        data.object_property,
        data.array_index,
        &error_class, &error_code);
    if (len >= 0) {
        /* encode the APDU portion of the packet */
        data.application_data = &Temp_Buf[0];
        data.application_data_len = len;
        /* FIXME: probably need a length limitation sent with encode */
        len =
            rp_ack_encode_apdu(&Handler_Transmit_Buffer[pdu_len],
            service_data->invoke_id, &data);
#if PRINT_ENABLED
        fprintf(stderr,
            "RP: Sending Ack!\n");
#endif
        error = false;
    }
    if (error) {
        if (len == -2) {
            /* BACnet APDU too small to fit data, so proper response is Abort */
            len = abort_encode_apdu(&Handler_Transmit_Buffer[pdu_len],
                service_data->invoke_id,
                ABORT_REASON_SEGMENTATION_NOT_SUPPORTED, true);
#if PRINT_ENABLED
            fprintf(stderr, "RP: Reply too big to fit into APDU!\n");
#endif
        } else {
            len = bacerror_encode_apdu(&Handler_Transmit_Buffer[pdu_len],
                service_data->invoke_id,
                SERVICE_CONFIRMED_READ_PROPERTY, error_class, error_code);
#if PRINT_ENABLED
            fprintf(stderr, "RP: Sending Error!\n");
#endif
        }
    }
RP_ABORT:
    pdu_len += len;
    bytes_sent = datalink_send_pdu(src, &npdu_data,
        &Handler_Transmit_Buffer[0], pdu_len);
#if PRINT_ENABLED
    if (bytes_sent <= 0)
        fprintf(stderr, "Failed to send PDU (%s)!\n", strerror(errno));
#endif

    return;
}
