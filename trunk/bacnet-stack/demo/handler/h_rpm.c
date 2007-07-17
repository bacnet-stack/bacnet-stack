/**************************************************************************
*
* Copyright (C) 2005 Steve Karg <skarg@users.sourceforge.net>
* Enhanced by John Stachler for ReadPropertyMultiple
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
#include "config.h"
#include "txbuf.h"
#include "bacdef.h"
#include "bacdcode.h"
#include "apdu.h"
#include "npdu.h"
#include "abort.h"
#include "rpm.h"
#include "handlers.h"
#include "device.h"
#include "ai.h"
#include "ao.h"
#include "bi.h"

static uint8_t Temp_Buf[MAX_APDU] = { 0 };

/* copy encoded apdu data to tx buffer
   this is needed to make sure we do not go past our apdu buffer size */
bool copy_apdu_data_buffer(int *apdu_len, uint8 len, uint8 npdu_len)
{
    bool copy_status = false;
    
    if ((*apdu_len + len) <= MAX_APDU) {
        memcpy(&Handler_Transmit_Buffer[*apdu_len + npdu_len], &Temp_Buf[0], len);
        *apdu_len += len;
        copy_status = true;
    }
    
    return copy_status;
}

void handler_read_property_multiple(uint8_t * service_request, uint16_t service_len, BACNET_ADDRESS * src, BACNET_CONFIRMED_SERVICE_DATA * service_data)
{
    int len = 0;
    int decode_len = 0;
    int pdu_len = 0;
    BACNET_NPDU_DATA npdu_data;
    bool error = false;
    bool done;
    bool property_found;
    int bytes_sent;
    BACNET_ERROR_CLASS error_class = ERROR_CLASS_OBJECT;
    BACNET_ERROR_CODE error_code = ERROR_CODE_UNKNOWN_OBJECT;
    BACNET_ADDRESS my_address;
    BACNET_OBJECT_TYPE object_type;
    uint32_t object_instance = 0;
    int apdu_len = 0;
    int npdu_len;
    BACNET_PROPERTY_ID object_property, temp_object_property;
    int32_t array_index = 0;
    uint8 index;
    uint8 num_properties;

    /* jps_debug - see if we are utilizing all the buffer */
    /* memset(&Handler_Transmit_Buffer[0], 0xff, MAX_MPDU);*/
    /* encode the NPDU portion of the packet */
    datalink_get_my_address(&my_address);
    npdu_encode_npdu_data(&npdu_data, false, MESSAGE_PRIORITY_NORMAL);
    npdu_len = npdu_encode_pdu(
        &Handler_Transmit_Buffer[0], 
        src, &my_address, &npdu_data);
#if PRINT_ENABLED
    if (service_len <= 0)
        printf("RPM: Unable to decode request!\r\n");
#endif
    /* bad decoding - send an abort */
    if (service_len == 0)
    {
        apdu_len = abort_encode_apdu(
            &Handler_Transmit_Buffer[npdu_len],
            service_data->invoke_id,
            ABORT_REASON_OTHER, true);
#if PRINT_ENABLED
        printf("RPM: Sending Abort!\r\n");
#endif
    } else if (service_data->segmented_message) {
        apdu_len = abort_encode_apdu(
            &Handler_Transmit_Buffer[npdu_len],
            service_data->invoke_id,
            ABORT_REASON_SEGMENTATION_NOT_SUPPORTED, true);
#if PRINT_ENABLED
    printf("RPM: Sending Abort!\r\n");
#endif
    } else {
        /* decode apdu request & encode apdu reply
           encode complex ack, invoke id, service choice */
        apdu_len = rpm_ack_encode_apdu_init(
            &Handler_Transmit_Buffer[npdu_len],
            service_data->invoke_id);
        do
        {
            len = rpm_ack_decode_object_id(
                &service_request[decode_len],
                service_len - decode_len,
                &object_type, &object_instance);
            if (len < 0) {
                break;
            } else {
                decode_len += len;
            }
            len = rpm_ack_encode_apdu_object_begin(
                &Temp_Buf[0],
                object_type, object_instance);
            if (!copy_apdu_data_buffer(&apdu_len, len, npdu_len)) {
                apdu_len = abort_encode_apdu(
                    &Handler_Transmit_Buffer[npdu_len],
                    service_data->invoke_id,
                    ABORT_REASON_SEGMENTATION_NOT_SUPPORTED,
                    true);
                break;
            }
            /* do each property of this object of the RPM request */
            done = true;
            num_properties = 0;
            property_found = false;
            do
            {
                if (done) {
                    len = rpm_decode_object_property(
                        &service_request[decode_len],
                        service_len - decode_len,
                        &object_property,
                        &array_index);
                    if (len < 0) {
                        break;
                    } else {
                        index = 0;
                        decode_len += len;
                        temp_object_property = object_property;
                    }
                }
                /* handle the special properties */
                if ((object_property == PROP_ALL) ||
                    (object_property == PROP_REQUIRED) ||
                    (object_property == PROP_OPTIONAL))
                {
                    done = false;
                    switch(object_type)
                    {
                        case OBJECT_DEVICE:
                           property_found = Device_Property(
                                &index, &temp_object_property,
                                object_property);
                            break;

                        case OBJECT_ANALOG_INPUT:
                            property_found = Analog_Input_Property(
                                &index, &temp_object_property,
                                object_property);
                            break;

                        case OBJECT_BINARY_INPUT:
                            property_found = Binary_Input_Property(
                                &index, &temp_object_property,
                                object_property);
                            break;

                        case OBJECT_ANALOG_OUTPUT:
                            property_found = Analog_Output_Property(
                                &index, &temp_object_property,
                                object_property);
                            break;
                    }
                    if (property_found) {
                        num_properties++;
                    } else {
                        if (num_properties > 0) {
                            done = true;
                            /* check for another property */
                            len = rpm_decode_object_property(
                                &service_request[decode_len],
                                service_len - decode_len,
                                &object_property,
                                &array_index);
                            if (len < 0) {
                                /* check for closing tag */
                                len = rpm_ack_decode_object_end(
                                    &service_request[decode_len],
                                    service_len - decode_len);
                                if (len == 1) {
                                    decode_len++;
                                }
                                break;
                            } else {
                                decode_len += len;
                                temp_object_property = object_property;
                                property_found = true;
                                if ((object_property == PROP_ALL) ||
                                    (object_property == PROP_REQUIRED) ||
                                    (object_property == PROP_OPTIONAL))
                                {
                                    done = false;
                                    index = 0;
                                    switch(object_type)
                                    {
                                        case OBJECT_DEVICE:
                                            property_found =
                                                Device_Property(&index,
                                                    &temp_object_property,
                                                    object_property);
                                            break;

                                        case OBJECT_ANALOG_INPUT:
                                            property_found =
                                                Analog_Input_Property(&index,
                                                    &temp_object_property,
                                                    object_property);
                                            break;

                                        case OBJECT_BINARY_INPUT:
                                            property_found =
                                                Binary_Input_Property(&index,
                                                    &temp_object_property,
                                                    object_property);
                                            break;

                                        case OBJECT_ANALOG_OUTPUT:
                                            property_found =
                                                Analog_Output_Property(&index,
                                                    &temp_object_property,
                                                    object_property);
                                            break;
                                    }
                                }
                            }
                        } else {
                            error = true;
                            done = true;
                        }
                    }
                } else {
                    done = true;
                    property_found = true;
                }
                len = rpm_ack_encode_apdu_object_property(
                    &Temp_Buf[0],
                    temp_object_property,
                    array_index);
                if (!copy_apdu_data_buffer(&apdu_len, len, npdu_len)) {
                    apdu_len = abort_encode_apdu(
                        &Handler_Transmit_Buffer[npdu_len],
                        service_data->invoke_id,
                        ABORT_REASON_SEGMENTATION_NOT_SUPPORTED, true);
                    break;
                }
                len = encode_opening_tag(&Temp_Buf[0], 4);
                if (!copy_apdu_data_buffer(&apdu_len, len, npdu_len)) {
                    apdu_len = abort_encode_apdu(&Handler_Transmit_Buffer[npdu_len], service_data->invoke_id, ABORT_REASON_SEGMENTATION_NOT_SUPPORTED, true);
                    break;
                }
                len = -1;
                switch(object_type) {
                    case OBJECT_DEVICE:
                        if ((object_instance != Device_Object_Instance_Number()) &&
                            ((object_instance != BACNET_MAX_INSTANCE) &&
                             (temp_object_property != PROP_OBJECT_IDENTIFIER))) {
                            error_class = ERROR_CLASS_OBJECT;
                            error_code = ERROR_CODE_UNKNOWN_OBJECT;
                        } else if (!property_found) {
                            error_class = ERROR_CLASS_PROPERTY;
                            error_code = ERROR_CODE_UNKNOWN_PROPERTY;
                        } else {
                            len = Device_Encode_Property_APDU(
                                &Temp_Buf[0], 
                                temp_object_property, 
                                array_index, 
                                &error_class, &error_code);
                        }
                        break;
                    case OBJECT_ANALOG_INPUT:
                        if (!Analog_Input_Valid_Instance(object_instance)) {
                            error_class = ERROR_CLASS_OBJECT;
                            error_code = ERROR_CODE_UNKNOWN_OBJECT;
                        } else if (!property_found) {
                            error_class = ERROR_CLASS_PROPERTY;
                            error_code = ERROR_CODE_UNKNOWN_PROPERTY;
                        } else {
                            len = Analog_Input_Encode_Property_APDU(
                                &Temp_Buf[0], 
                                object_instance, 
                                temp_object_property, 
                                array_index, 
                                &error_class, &error_code);
                        }
                        break;
                    case OBJECT_BINARY_INPUT:
                        if (!Binary_Input_Valid_Instance(object_instance)) {
                            error_class = ERROR_CLASS_OBJECT;
                            error_code = ERROR_CODE_UNKNOWN_OBJECT;
                        } else if (!property_found) {
                            error_class = ERROR_CLASS_PROPERTY;
                            error_code = ERROR_CODE_UNKNOWN_PROPERTY;
                        } else {
                            len = Binary_Input_Encode_Property_APDU(
                                &Temp_Buf[0], 
                                object_instance, 
                                temp_object_property, 
                                array_index, 
                                &error_class, &error_code);
                        }
                        break;
                    case OBJECT_ANALOG_OUTPUT:
                        if (!Analog_Output_Valid_Instance(object_instance)) {
                            error_class = ERROR_CLASS_OBJECT;
                            error_code = ERROR_CODE_UNKNOWN_OBJECT;
                        } else if (!property_found) {
                            error_class = ERROR_CLASS_PROPERTY;
                            error_code = ERROR_CODE_UNKNOWN_PROPERTY;
                        } else {
                            len = Analog_Output_Encode_Property_APDU(
                                &Temp_Buf[0], 
                                object_instance, 
                                temp_object_property, 
                                array_index, 
                                &error_class, &error_code);
                        }
                        break;
                    default:
                        len = -1;
                        error_class = ERROR_CLASS_OBJECT;
                        error_code = ERROR_CODE_UNSUPPORTED_OBJECT_TYPE;
                        break;
                }
                if (len < 0) {
                    len = rpm_ack_encode_apdu_object_property_error(
                        &Temp_Buf[0], 
                        error_class, error_code);
                    apdu_len--;
                    error = true;
                }
                if (!copy_apdu_data_buffer(&apdu_len, len, npdu_len)) {
                    apdu_len = abort_encode_apdu(
                        &Handler_Transmit_Buffer[npdu_len], 
                        service_data->invoke_id, 
                        ABORT_REASON_SEGMENTATION_NOT_SUPPORTED, 
                        true);
                }
                if (!error && property_found) {
                    len = encode_closing_tag(&Temp_Buf[0], 4);
                    if (!copy_apdu_data_buffer(&apdu_len, len, npdu_len)) {
                        apdu_len = abort_encode_apdu(
                            &Handler_Transmit_Buffer[npdu_len],
                            service_data->invoke_id,
                            ABORT_REASON_SEGMENTATION_NOT_SUPPORTED,
                            true);
                        break;
                    }
                } else {
                    error = false;
                }
                /* check for closing tag */
                if (done) {
                    len = rpm_ack_decode_object_end(
                        &service_request[decode_len], 
                        service_len - decode_len);
                    if (len == 1) {
                        decode_len++;
                        break;
                    }
                }
            } while(1);
            len = encode_closing_tag(&Temp_Buf[0], 1);
            if (!copy_apdu_data_buffer(&apdu_len, len, npdu_len)) {
                apdu_len = abort_encode_apdu(
                    &Handler_Transmit_Buffer[npdu_len], 
                    service_data->invoke_id, 
                    ABORT_REASON_SEGMENTATION_NOT_SUPPORTED, 
                    true);
                break;
            }
            if (decode_len >= service_len) {
                break;
            }
        } while(1);
    }
    pdu_len = apdu_len + npdu_len;
    bytes_sent = datalink_send_pdu(
        src, 
        &npdu_data, 
        &Handler_Transmit_Buffer[0], 
        pdu_len);
}
