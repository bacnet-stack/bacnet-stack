/**
 * @file
 * @brief Handles Confirmed Private Transfer requests.
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date 2026
 * @copyright SPDX-License-Identifier: MIT
 */
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
/* BACnet Stack defines - first */
#include "bacnet/bacdef.h"
/* BACnet Stack API */
#include "bacnet/abort.h"
#include "bacnet/apdu.h"
#include "bacnet/npdu.h"
#include "bacnet/ptransfer.h"
#include "bacnet/reject.h"
/* basic objects, services, TSM, and datalink */
#include "bacnet/basic/services.h"
#include "bacnet/basic/sys/debug.h"
#include "bacnet/basic/tsm/tsm.h"
#include "bacnet/datalink/datalink.h"
#include "bacnet/basic/service/h_upt.h"

/**
 * @brief Handle a confirmed private transfer request.
 * @param service_request Buffer containing the service request payload.
 * @param service_len Length of the service request payload in bytes.
 * @param src Source BACnet address of the requester.
 * @param service_data Confirmed service metadata for the request.
 */
void handler_confirmed_private_transfer(
    uint8_t *service_request,
    uint16_t service_len,
    BACNET_ADDRESS *src,
    BACNET_CONFIRMED_SERVICE_DATA *service_data)
{
    BACNET_PRIVATE_TRANSFER_DATA data = { 0 };
    BACNET_NPDU_DATA npdu_data = { 0 };
    BACNET_ADDRESS my_address = { 0 };
    int decode_len = 0;
    int npdu_len = 0;
    int apdu_len = 0;
    int pdu_len = 0;
    int bytes_sent = 0;

    if (service_len == 0) {
        apdu_len = reject_encode_apdu(
            &Handler_Transmit_Buffer[0], service_data->invoke_id,
            REJECT_REASON_MISSING_REQUIRED_PARAMETER);
    } else if (service_data->segmented_message) {
        apdu_len = abort_encode_apdu(
            &Handler_Transmit_Buffer[0], service_data->invoke_id,
            ABORT_REASON_SEGMENTATION_NOT_SUPPORTED, true);
    } else {
        decode_len = ptransfer_decode_service_request(
            service_request, service_len, &data);
        if (decode_len < 0) {
            apdu_len = abort_encode_apdu(
                &Handler_Transmit_Buffer[0], service_data->invoke_id,
                ABORT_REASON_OTHER, true);
        } else {
            private_transfer_print_data(&data);
            /* Configure for the ACK */
            datalink_get_my_address(&my_address);
            npdu_encode_npdu_data(&npdu_data, false, service_data->priority);
            npdu_len = npdu_encode_pdu(
                &Handler_Transmit_Buffer[0], src, &my_address, &npdu_data);
            /* The 'Result(-)' parameter shall indicate that the
               service request has failed. The Error Type parameter
               consists of two component parameters:
               (1) the 'Error Class' and (2) the 'Error Code'. */
            /* The 'Result(+)' parameter shall indicate that the
               service request succeeded. Result Block returns a
               conditional parameter of type list of ANY.
               It shall convey any additional results
               from the execution of the requested service.
               Interpretation of these results is a local matter. */
            /* For this example handler, we use the received block.*/
            apdu_len = ptransfer_ack_encode_apdu(
                &Handler_Transmit_Buffer[npdu_len], service_data->invoke_id,
                &data);
            pdu_len = npdu_len + apdu_len;
            bytes_sent = datalink_send_pdu(
                src, &npdu_data, &Handler_Transmit_Buffer[0], pdu_len);
            if (bytes_sent <= 0) {
                debug_perror("CPT: Failed to send PDU");
            }
            return;
        }
    }
    datalink_get_my_address(&my_address);
    npdu_encode_npdu_data(&npdu_data, false, service_data->priority);
    npdu_len = npdu_encode_pdu(
        &Handler_Transmit_Buffer[0], src, &my_address, &npdu_data);
    pdu_len = npdu_len + apdu_len;
    bytes_sent = datalink_send_pdu(
        src, &npdu_data, &Handler_Transmit_Buffer[0], pdu_len);
    if (bytes_sent <= 0) {
        debug_perror("CPT: Failed to send PDU");
    }
}
