/**
 * @file
 * @brief CreateObject service application handlers
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
#include "bacnet/create_object.h"
/* basic objects, services, TSM, and datalink */
#include "bacnet/basic/object/device.h"
#include "bacnet/basic/tsm/tsm.h"
#include "bacnet/basic/services.h"
#include "bacnet/basic/sys/debug.h"
#include "bacnet/datalink/datalink.h"

/**
 * @brief Handler for a CreateObject service request.
 * This handler will be invoked by apdu_handler() if it has been enabled
 * via call to apdu_set_confirmed_handler().
 * This handler builds a response packet, which is
 * - an Abort if
 *   - the message is segmented
 *   - if decoding fails
 * - a SimpleACK if Device_Create_Object() succeeds
 * - an Error if Device_Create_Object() fails
 *
 * @param service_request [in] The contents of the service request.
 * @param service_len [in] The length of the service_request.
 * @param src [in] BACNET_ADDRESS of the source of the message
 * @param service_data [in] The BACNET_CONFIRMED_SERVICE_DATA information
 *                          decoded from the APDU header of this message.
 */
void handler_create_object(uint8_t *service_request,
    uint16_t service_len,
    BACNET_ADDRESS *src,
    BACNET_CONFIRMED_SERVICE_DATA *service_data)
{
    BACNET_CREATE_OBJECT_DATA data = { 0 };
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
    debug_perror("CreateObject: Received Request!\n");
    if (service_data->segmented_message) {
        len = abort_encode_apdu(&Handler_Transmit_Buffer[pdu_len],
            service_data->invoke_id, ABORT_REASON_SEGMENTATION_NOT_SUPPORTED,
            true);
        debug_perror("CreateObject: Segmented message.  Sending Abort!\n");
        status = false;
    }
    if (status) {
        /* decode the service request only */
        len = create_object_decode_service_request(
            service_request, service_len, &data);
        if (len > 0) {
            debug_perror("CreateObject: type=%lu instance=%lu\n",
                (unsigned long)data.object_type,
                (unsigned long)data.object_instance);
        } else {
            debug_perror("CreateObject: Unable to decode request!\n");
        }
        if (len <= 0) {
            /* bad decoding or something we didn't understand */
            if (len == BACNET_STATUS_ABORT) {
                len = abort_encode_apdu(&Handler_Transmit_Buffer[pdu_len],
                    service_data->invoke_id,
                    abort_convert_error_code(data.error_code), true);
                debug_perror("CreateObject: Sending Abort!\n");
            } else if (len == BACNET_STATUS_REJECT) {
                len = reject_encode_apdu(&Handler_Transmit_Buffer[pdu_len],
                    service_data->invoke_id,
                    reject_convert_error_code(data.error_code));
                debug_perror("CreateObject: Sending Reject!\n");
            }
        } else {
            if (Device_Create_Object(&data)) {
                len =
                    create_object_ack_encode(&Handler_Transmit_Buffer[pdu_len],
                        service_data->invoke_id, &data);
                debug_perror("CreateObject: Sending ACK!\n");
            } else {
                len = create_object_error_ack_encode(
                    &Handler_Transmit_Buffer[pdu_len],
                    service_data->invoke_id, &data);
                debug_perror("CreateObject: Sending Error!\n");
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
            "CreateObject: Failed to send PDU (%s)!\n", strerror(errno));
    }

    return;
}
