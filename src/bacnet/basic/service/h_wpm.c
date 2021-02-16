/**************************************************************************
 *
 * Copyright (C) 2011 Krzysztof Malorny <malornykrzysztof@gmail.com>
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
#include "bacnet/config.h"
#include "bacnet/bacdef.h"
#include "bacnet/bacdcode.h"
#include "bacnet/bacerror.h"
#include "bacnet/apdu.h"
#include "bacnet/npdu.h"
#include "bacnet/abort.h"
#include "bacnet/wp.h"
#include "bacnet/reject.h"
#include "bacnet/wpm.h"
/* basic objects, services, TSM, and datalink */
#include "bacnet/basic/object/device.h"
#include "bacnet/basic/tsm/tsm.h"
#include "bacnet/basic/services.h"
#include "bacnet/datalink/datalink.h"

/** @file h_wpm.c  Handles Write Property Multiple requests. */
#if PRINT_ENABLED
#include <stdio.h>
#define PRINTF(...) fprintf(stderr,__VA_ARGS__)
#else
#define PRINTF(...)
#endif

/** Decoding for an object property.
 *
 * @param apdu [in] The contents of the APDU buffer.
 * @param apdu_len [in] The length of the APDU buffer.
 * @param wp_data [out] The BACNET_WRITE_PROPERTY_DATA structure.
 * @param device_write_property - Device object WriteProperty function
 *
 * @return number of bytes decoded, or BACNET_STATUS_REJECT,
 *  or BACNET_STATUS_ERROR
 */
static int write_property_multiple_decode(
    uint8_t *apdu, uint16_t apdu_len,
    BACNET_WRITE_PROPERTY_DATA *wp_data,
    write_property_function device_write_property)
{
    int len = 0;
    int offset = 0;
    uint8_t tag_number = 0;

    /* decode service request */
    do {
        /* decode Object Identifier */
        len = wpm_decode_object_id(&apdu[offset], apdu_len - offset, wp_data);
        if (len > 0) {
            offset += len;
            /* Opening tag 1 - List of Properties */
            if (decode_is_opening_tag_number(&apdu[offset++], 1)) {
                do {
                    /* decode a 'Property Identifier':
                      (3) an optional 'Property Array Index'
                      (4) a 'Property Value'
                      (5) an optional 'Priority' */
                    len =  wpm_decode_object_property(&apdu[offset],
                            apdu_len - offset, wp_data);
                    if (len > 0) {
                        offset += len;
                        PRINTF("WPM: type=%lu instance=%lu property=%lu "
                            "priority=%lu index=%ld\n",
                            (unsigned long)wp_data->object_type,
                            (unsigned long)wp_data->object_instance,
                            (unsigned long)wp_data->object_property,
                            (unsigned long)wp_data->priority,
                            (long)wp_data->array_index);
                        if (device_write_property) {
                            if (device_write_property(wp_data) == false) {
                                return BACNET_STATUS_ERROR;
                            }
                        }
                    } else {
                        PRINTF("WPM: Bad Encoding!\n");
                        return len;
                    }
                    /* Closing tag 1 - List of Properties */
                    if (decode_is_closing_tag_number(&apdu[offset], 1)) {
                        tag_number = 1;
                        offset++;
                    } else {
                        /* it was not tag 1, decode next Property Identifier */
                        tag_number = 0;
                    }
                    /* end decoding List of Properties for "that" object */
                } while (tag_number != 1);
            }
        } else {
            PRINTF("WPM: Bad Encoding!\n");
            return len;
        }
    } while (offset < apdu_len);

    return len;
}

/** Handler for a WriteProperty Service request.
 * @ingroup DSWP
 * This handler will be invoked by apdu_handler() if it has been enabled
 * by a call to apdu_set_confirmed_handler().
 * This handler builds a response packet, which is
 * - an Abort if
 *   - the message is segmented
 *   - if decoding fails
 * - an ACK if Device_Write_Property_Multiple() succeeds
 * - an Error if Device_Write_PropertyMultiple() encounters an error
 *
 * @param service_request [in] The contents of the service request.
 * @param service_len [in] The length of the service_request.
 * @param src [in] BACNET_ADDRESS of the source of the message
 * @param service_data [in] The BACNET_CONFIRMED_SERVICE_DATA information
 *                          decoded from the APDU header of this message.
 */
void handler_write_property_multiple(uint8_t *service_request,
    uint16_t service_len,
    BACNET_ADDRESS *src,
    BACNET_CONFIRMED_SERVICE_DATA *service_data)
{
    int len = 0;
    int apdu_len = 0;
    int npdu_len = 0;
    int pdu_len = 0;
    BACNET_WRITE_PROPERTY_DATA wp_data;
    BACNET_NPDU_DATA npdu_data;
    BACNET_ADDRESS my_address;
    int bytes_sent = 0;

    if (service_data->segmented_message) {
        wp_data.error_code = ERROR_CODE_ABORT_SEGMENTATION_NOT_SUPPORTED;
        len = BACNET_STATUS_ABORT;
        PRINTF("WPM: Segmented message.  Sending Abort!\n");
    } else {
        /* first time - detect malformed request before writing any data */
        len = write_property_multiple_decode(service_request, service_len,
            &wp_data, NULL);
        if (len > 0) {
            len = write_property_multiple_decode(service_request, service_len,
                &wp_data, Device_Write_Property);
        }
    }
    /* encode the confirmed reply */
    datalink_get_my_address(&my_address);
    npdu_encode_npdu_data(&npdu_data, false, MESSAGE_PRIORITY_NORMAL);
    npdu_len = npdu_encode_pdu(
        &Handler_Transmit_Buffer[0], src, &my_address, &npdu_data);
    if (len > 0) {
        apdu_len = wpm_ack_encode_apdu_init(
            &Handler_Transmit_Buffer[npdu_len], service_data->invoke_id);
        PRINTF("WPM: Sending Ack!\n");
    } else {
        /* handle any errors */
        if (len == BACNET_STATUS_ABORT) {
            apdu_len = abort_encode_apdu(&Handler_Transmit_Buffer[npdu_len],
                service_data->invoke_id,
                abort_convert_error_code(wp_data.error_code), true);
            PRINTF("WPM: Sending Abort!\n");
        } else if (len == BACNET_STATUS_ERROR) {
            apdu_len =
                wpm_error_ack_encode_apdu(&Handler_Transmit_Buffer[npdu_len],
                    service_data->invoke_id, &wp_data);
            PRINTF("WPM: Sending Error!\n");
        } else if (len == BACNET_STATUS_REJECT) {
            apdu_len = reject_encode_apdu(&Handler_Transmit_Buffer[npdu_len],
                service_data->invoke_id,
                reject_convert_error_code(wp_data.error_code));
            PRINTF("WPM: Sending Reject!\n");
        }
    }
    pdu_len = npdu_len + apdu_len;
    bytes_sent = datalink_send_pdu(
        src, &npdu_data, &Handler_Transmit_Buffer[0], pdu_len);
    if (bytes_sent <= 0) {
        PRINTF("Failed to send PDU (%s)!\n", strerror(errno));
    }
}
