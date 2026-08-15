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
#include "bacnet/bacerror.h"
#include "bacnet/npdu.h"
#include "bacnet/ptransfer.h"
#include "bacnet/reject.h"
/* basic objects, services, TSM, and datalink */
#include "bacnet/basic/services.h"
#include "bacnet/basic/sys/debug.h"
#include "bacnet/basic/tsm/tsm.h"
#include "bacnet/datalink/datalink.h"
#include "bacnet/basic/service/h_upt.h"

/* Confirmed Private Transfer data callback */
static handler_private_transfer_callback_t
    Handler_Confirmed_Private_Transfer_Callback = NULL;

/**
 * @brief Set the callback function for handling confirmed private transfer
 * requests.
 * @param cb The callback function to set.
 */
void handler_confirmed_private_transfer_callback_set(
    handler_private_transfer_callback_t cb)
{
    Handler_Confirmed_Private_Transfer_Callback = cb;
}

int handler_confirmed_private_transfer_encode(
    uint8_t *pdu,
    uint8_t *service_request,
    uint16_t service_len,
    BACNET_ADDRESS *src,
    BACNET_NPDU_DATA *npdu_data,
    BACNET_CONFIRMED_SERVICE_DATA *service_data)
{
    BACNET_PRIVATE_TRANSFER_DATA data = { 0 };
    BACNET_ADDRESS my_address = { 0 };
    BACNET_ERROR_CODE error_code = ERROR_CODE_SUCCESS;
    int len = 0;
    int pdu_len = 0;
    uint8_t *apdu;

    /* encode the NPDU portion of the packet */
    datalink_get_my_address(&my_address);
    npdu_encode_npdu_data(npdu_data, false, service_data->priority);
    apdu = pdu;
    len = npdu_encode_pdu(apdu, src, &my_address, npdu_data);
    pdu_len += len;
    if (apdu) {
        apdu += len;
    }
    /* APDU starts after the NPDU portion */
    if (service_len == 0) {
        error_code = ERROR_CODE_REJECT_MISSING_REQUIRED_PARAMETER;
    } else if (service_data->segmented_message) {
        error_code = ERROR_CODE_ABORT_SEGMENTATION_NOT_SUPPORTED;
    }
    if (error_code == ERROR_CODE_SUCCESS) {
        len = ptransfer_decode_service_request(
            service_request, service_len, &data);
        if (len < 0) {
            error_code = ERROR_CODE_ABORT_OTHER;
        } else {
            if (Handler_Confirmed_Private_Transfer_Callback) {
                error_code =
                    Handler_Confirmed_Private_Transfer_Callback(&data, NULL);
            }
            if (error_code == ERROR_CODE_SUCCESS) {
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
                len = ptransfer_ack_encode_apdu(
                    apdu, service_data->invoke_id, &data);
                pdu_len += len;
            } else {
                len = bacnet_error_encode_apdu(
                    apdu, service_data->invoke_id,
                    SERVICE_CONFIRMED_PRIVATE_TRANSFER, error_code);
                pdu_len += len;
            }
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
            len =
                ptransfer_ack_encode_apdu(apdu, service_data->invoke_id, &data);
            pdu_len += len;
        }
    }
    if (error_code != ERROR_CODE_SUCCESS) {
        len = bacnet_error_encode_apdu(
            apdu, service_data->invoke_id, SERVICE_CONFIRMED_PRIVATE_TRANSFER,
            error_code);
        pdu_len += len;
    }

    return pdu_len;
}

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
    int pdu_len = 0, bytes_sent = 0;
    BACNET_NPDU_DATA npdu_data = { 0 };

    pdu_len = handler_confirmed_private_transfer_encode(
        NULL, service_request, service_len, src, &npdu_data, service_data);
    if (pdu_len > sizeof(Handler_Transmit_Buffer)) {
        debug_perror("CPT: Encoded PDU length exceeds transmit buffer size");
    } else {
        pdu_len = handler_confirmed_private_transfer_encode(
            Handler_Transmit_Buffer, service_request, service_len, src,
            &npdu_data, service_data);
        bytes_sent = datalink_send_pdu(
            src, &npdu_data, &Handler_Transmit_Buffer[0], pdu_len);
        if (bytes_sent <= 0) {
            debug_perror("CPT: Failed to send PDU");
        }
    }
}
