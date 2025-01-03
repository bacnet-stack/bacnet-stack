/**
 * @file
 * @brief A basic Reinitialize Device request handler
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date 2006
 * @copyright SPDX-License-Identifier: MIT
 */
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
/* BACnet Stack defines - first */
#include "bacnet/bacdef.h"
/* BACnet Stack API */
#include "bacnet/bacdcode.h"
#include "bacnet/bacerror.h"
#include "bacnet/apdu.h"
#include "bacnet/npdu.h"
#include "bacnet/abort.h"
#include "bacnet/reject.h"
#include "bacnet/rd.h"
/* basic objects, services, TSM, and datalink */
#include "bacnet/basic/object/device.h"
#include "bacnet/basic/tsm/tsm.h"
#include "bacnet/basic/services.h"
#include "bacnet/basic/sys/debug.h"
#include "bacnet/datalink/datalink.h"

/** Handler for a Reinitialize Device (RD) request.
 * @ingroup DMRD
 * This handler will be invoked by apdu_handler() if it has been enabled
 * by a call to apdu_set_confirmed_handler().
 * This handler builds a response packet, which is
 * - an Abort if
 *   - the message is segmented
 *   - if decoding fails
 * - an Error if
 *   - the RD password is incorrect
 *   - the Reinitialize Device operation fails
 * - a Reject if the request state is invalid
 * - else tries to send a simple ACK for the RD on success.
 *
 * @param service_request [in] The contents of the service request.
 * @param service_len [in] The length of the service_request.
 * @param src [in] BACNET_ADDRESS of the source of the message
 * @param service_data [in] The BACNET_CONFIRMED_SERVICE_DATA information
 *                          decoded from the APDU header of this message.
 */
void handler_reinitialize_device(
    uint8_t *service_request,
    uint16_t service_len,
    BACNET_ADDRESS *src,
    BACNET_CONFIRMED_SERVICE_DATA *service_data)
{
    BACNET_REINITIALIZE_DEVICE_DATA rd_data = { 0 };
    int len = 0;
    int pdu_len = 0;
    BACNET_NPDU_DATA npdu_data = { 0 };
    BACNET_ADDRESS my_address = { 0 };

    /* encode the NPDU portion of the packet */
    datalink_get_my_address(&my_address);
    npdu_encode_npdu_data(&npdu_data, false, service_data->priority);
    pdu_len = npdu_encode_pdu(
        &Handler_Transmit_Buffer[0], src, &my_address, &npdu_data);
    debug_fprintf(stderr, "ReinitializeDevice!\n");
    if (service_len == 0) {
        len = reject_encode_apdu(
            &Handler_Transmit_Buffer[pdu_len], service_data->invoke_id,
            REJECT_REASON_MISSING_REQUIRED_PARAMETER);
        debug_fprintf(
            stderr,
            "ReinitializeDevice: Missing Required Parameter. "
            "Sending Reject!\n");
        goto RD_ABORT;
    } else if (service_data->segmented_message) {
        len = abort_encode_apdu(
            &Handler_Transmit_Buffer[pdu_len], service_data->invoke_id,
            ABORT_REASON_SEGMENTATION_NOT_SUPPORTED, true);
        debug_fprintf(
            stderr, "ReinitializeDevice: Sending Abort - segmented message.\n");
        goto RD_ABORT;
    }
    /* decode the service request only */
    len = rd_decode_service_request(
        service_request, service_len, &rd_data.state, &rd_data.password);
    if (len > 0) {
        debug_fprintf(
            stderr, "ReinitializeDevice: state=%u password=%*s\n",
            (unsigned)rd_data.state,
            (int)characterstring_length(&rd_data.password),
            characterstring_value(&rd_data.password));
    } else {
        debug_fprintf(
            stderr, "ReinitializeDevice: Unable to decode request!\n");
    }
    /* bad decoding or something we didn't understand - send an abort */
    if (len < 0) {
        len = abort_encode_apdu(
            &Handler_Transmit_Buffer[pdu_len], service_data->invoke_id,
            ABORT_REASON_OTHER, true);
        debug_fprintf(
            stderr, "ReinitializeDevice: Sending Abort - could not decode.\n");
        goto RD_ABORT;
    }
    /* check the data from the request */
    if (rd_data.state >= BACNET_REINIT_MAX) {
        len = reject_encode_apdu(
            &Handler_Transmit_Buffer[pdu_len], service_data->invoke_id,
            REJECT_REASON_UNDEFINED_ENUMERATION);
        debug_fprintf(
            stderr,
            "ReinitializeDevice: Sending Reject - undefined enumeration\n");
    } else {
#ifdef BAC_ROUTING
        /* Check to see if the current Device supports this service. */
        len = Routed_Device_Service_Approval(
            SERVICE_SUPPORTED_REINITIALIZE_DEVICE, (int)rd_data.state,
            &Handler_Transmit_Buffer[pdu_len], service_data->invoke_id);
        if (len > 0) {
            goto RD_ABORT;
        }
#endif
        if (Device_Reinitialize(&rd_data)) {
            len = encode_simple_ack(
                &Handler_Transmit_Buffer[pdu_len], service_data->invoke_id,
                SERVICE_CONFIRMED_REINITIALIZE_DEVICE);
            debug_fprintf(stderr, "ReinitializeDevice: Sending Simple Ack!\n");
        } else {
            len = bacerror_encode_apdu(
                &Handler_Transmit_Buffer[pdu_len], service_data->invoke_id,
                SERVICE_CONFIRMED_REINITIALIZE_DEVICE, rd_data.error_class,
                rd_data.error_code);
            debug_fprintf(stderr, "ReinitializeDevice: Sending Error.\n");
        }
    }
RD_ABORT:
    pdu_len += len;
    len = datalink_send_pdu(
        src, &npdu_data, &Handler_Transmit_Buffer[0], pdu_len);
    if (len <= 0) {
        debug_perror("ReinitializeDevice: Failed to send PDU");
    }

    return;
}
