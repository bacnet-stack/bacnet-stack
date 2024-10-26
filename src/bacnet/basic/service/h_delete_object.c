/**
 * @file
 * @brief DeleteObject service application handlers
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date August 2023
 * @copyright SPDX-License-Identifier: MIT
 */
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
/* BACnet Stack defines - first */
#include "bacnet/bacdef.h"
/* BACnet Stack API */
#include "bacnet/bacdcode.h"
#include "bacnet/bacerror.h"
#include "bacnet/apdu.h"
#include "bacnet/npdu.h"
#include "bacnet/abort.h"
#include "bacnet/reject.h"
#include "bacnet/delete_object.h"
/* basic services, TSM, and datalink */
#include "bacnet/basic/tsm/tsm.h"
#include "bacnet/basic/services.h"
#include "bacnet/basic/sys/debug.h"
#include "bacnet/datalink/datalink.h"

/**
 * @brief Handler for a DeleteObject Service request.
 * This handler will be invoked by apdu_handler() if it has been enabled
 * via call to apdu_set_confirmed_handler().
 * This handler builds a response packet, which is
 * - an Abort if
 *   - the message is segmented
 *   - if decoding fails
 * - a SimpleACK if DeleteObject-Request succeeds
 * - an Error if DeleteObject-Request fails
 *
 * @param service_request [in] The contents of the service request.
 * @param service_len [in] The length of the service_request.
 * @param src [in] BACNET_ADDRESS of the source of the message
 * @param service_data [in] The BACNET_CONFIRMED_SERVICE_DATA information
 *  decoded from the APDU header of this message.
 */
void handler_delete_object(
    uint8_t *service_request,
    uint16_t service_len,
    BACNET_ADDRESS *src,
    BACNET_CONFIRMED_SERVICE_DATA *service_data)
{
    BACNET_DELETE_OBJECT_DATA data = { 0 };
    BACNET_NPDU_DATA npdu_data = { 0 };
    BACNET_ADDRESS my_address = { 0 };
    int len = 0;
    bool status = true;
    int pdu_len = 0;
    int bytes_sent = 0;

    /* encode the NPDU portion of the packet */
    datalink_get_my_address(&my_address);
    npdu_encode_npdu_data(&npdu_data, false, MESSAGE_PRIORITY_NORMAL);
    pdu_len = npdu_encode_pdu(
        &Handler_Transmit_Buffer[0], src, &my_address, &npdu_data);
    debug_perror("DeleteObject: Received Request!\n");
    if (service_data->segmented_message) {
        len = abort_encode_apdu(
            &Handler_Transmit_Buffer[pdu_len], service_data->invoke_id,
            ABORT_REASON_SEGMENTATION_NOT_SUPPORTED, true);
        debug_perror("DeleteObject: Segmented message.  Sending Abort!\n");
        status = false;
    }
    if (status) {
        /* decode the service request only */
        len = delete_object_decode_service_request(
            service_request, service_len, &data);
        if (len > 0) {
            debug_perror(
                "DeleteObject: type=%lu instance=%lu\n",
                (unsigned long)data.object_type,
                (unsigned long)data.object_instance);
        } else {
            debug_perror("DeleteObject: Unable to decode request!\n");
        }
        /* bad decoding or something we didn't understand - send an abort */
        if (len <= 0) {
            len = abort_encode_apdu(
                &Handler_Transmit_Buffer[pdu_len], service_data->invoke_id,
                ABORT_REASON_OTHER, true);
            debug_perror("DeleteObject: Bad Encoding. Sending Abort!\n");
            status = false;
        }
        if (status) {
            if (Device_Delete_Object(&data)) {
                len = encode_simple_ack(
                    &Handler_Transmit_Buffer[pdu_len], service_data->invoke_id,
                    SERVICE_CONFIRMED_DELETE_OBJECT);
                debug_perror("DeleteObject: Sending Simple Ack!\n");
            } else {
                len = bacerror_encode_apdu(
                    &Handler_Transmit_Buffer[pdu_len], service_data->invoke_id,
                    SERVICE_CONFIRMED_DELETE_OBJECT, data.error_class,
                    data.error_code);
                debug_perror("DeleteObject: Sending Error!\n");
            }
        }
    }
    if (len > 0) {
        /* Send PDU */
        pdu_len += len;
        bytes_sent = datalink_send_pdu(
            src, &npdu_data, &Handler_Transmit_Buffer[0], pdu_len);
    }
    if (bytes_sent <= 0) {
        debug_perror(
            "DeleteObject: Failed to send PDU (%s)!\n", strerror(errno));
    }

    return;
}
