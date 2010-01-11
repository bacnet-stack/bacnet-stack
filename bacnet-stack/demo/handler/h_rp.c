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

static uint8_t Temp_Buf[MAX_APDU] = { 0 };

static read_property_function Read_Property[MAX_BACNET_OBJECT_TYPE];

static object_valid_instance_function Valid_Instance[MAX_BACNET_OBJECT_TYPE];

void handler_read_property_object_set(
    BACNET_OBJECT_TYPE object_type,
    read_property_function pFunction1,
    object_valid_instance_function pFunction2)
{
    if (object_type < MAX_BACNET_OBJECT_TYPE) {
        Read_Property[object_type] = pFunction1;
        Valid_Instance[object_type] = pFunction2;
    }
}

/* Encodes the property APDU and returns the length,
   or sets the error, and returns -1 */
int Encode_Property_APDU(
    uint8_t * apdu,
    BACNET_OBJECT_TYPE object_type,
    uint32_t  object_instance,
    BACNET_PROPERTY_ID property,
    int32_t array_index,
    BACNET_ERROR_CLASS * error_class,
    BACNET_ERROR_CODE * error_code)
{
    int apdu_len = -1;
    read_property_function object_rp = NULL;
    object_valid_instance_function object_valid = NULL;

    /* initialize the default return values */
    *error_class = ERROR_CLASS_OBJECT;
    *error_code = ERROR_CODE_UNKNOWN_OBJECT;
    /* handle each object type */
    if (object_type < MAX_BACNET_OBJECT_TYPE) {
        object_rp = Read_Property[object_type];
        object_valid = Valid_Instance[object_type];
    }
    if (object_rp && object_valid && object_valid(object_instance)) {
        apdu_len =
            object_rp(&apdu[0], object_instance, property, array_index,
            error_class, error_code);
    } else {
        *error_class = ERROR_CLASS_OBJECT;
        *error_code = ERROR_CODE_UNSUPPORTED_OBJECT_TYPE;
    }

    return apdu_len;
}

void handler_read_property(
    uint8_t * service_request,
    uint16_t service_len,
    BACNET_ADDRESS * src,
    BACNET_CONFIRMED_SERVICE_DATA * service_data)
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
        len =
            abort_encode_apdu(&Handler_Transmit_Buffer[pdu_len],
            service_data->invoke_id, ABORT_REASON_OTHER, true);
#if PRINT_ENABLED
        fprintf(stderr, "RP: Bad Encoding.  Sending Abort!\n");
#endif
        goto RP_ABORT;
    }

    /* assume that there is an error */
    error = true;
    len =
        Encode_Property_APDU(&Temp_Buf[0], data.object_type,
        data.object_instance, data.object_property, data.array_index,
        &error_class, &error_code);
    if (len >= 0) {
        /* encode the APDU portion of the packet */
        data.application_data = &Temp_Buf[0];
        data.application_data_len = len;
        /* FIXME: probably need a length limitation sent with encode */
        len =
            rp_ack_encode_apdu(&Handler_Transmit_Buffer[pdu_len],
            service_data->invoke_id, &data);
        if (len > service_data->max_resp) {
            /* too big for the sender - send an abort */
            len =
                abort_encode_apdu(&Handler_Transmit_Buffer[pdu_len],
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
            len =
                abort_encode_apdu(&Handler_Transmit_Buffer[pdu_len],
                service_data->invoke_id,
                ABORT_REASON_SEGMENTATION_NOT_SUPPORTED, true);
#if PRINT_ENABLED
            fprintf(stderr, "RP: Reply too big to fit into APDU!\n");
#endif
        } else {
            len =
                bacerror_encode_apdu(&Handler_Transmit_Buffer[pdu_len],
                service_data->invoke_id, SERVICE_CONFIRMED_READ_PROPERTY,
                error_class, error_code);
#if PRINT_ENABLED
            fprintf(stderr, "RP: Sending Error!\n");
#endif
        }
    }
  RP_ABORT:
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


int local_read_property(
    uint8_t * value,
    uint8_t * status,
    BACNET_DEVICE_OBJECT_PROPERTY_REFERENCE *Source,
    BACNET_ERROR_CLASS * error_class,
    BACNET_ERROR_CODE * error_code)
    
{
    int len = 0;

    /* Try to fetch the required property */
    len = Encode_Property_APDU(value, Source->objectIdentifier.type,
        Source->objectIdentifier.instance, Source->propertyIdentifier,
        Source->arrayIndex, error_class, error_code);
        
    if((len >= 0) && (status != NULL)){
        /* Fetch the status flags if required */
        Encode_Property_APDU(status, Source->objectIdentifier.type,
            Source->objectIdentifier.instance, PROP_STATUS_FLAGS,
            0, error_class, error_code);
    }
    
    return(len);
}
