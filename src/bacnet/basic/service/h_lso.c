/**
 * @file
 * @brief A basic LifeSafetyOperation service handler
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date 2005
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
#include "bacnet/apdu.h"
#include "bacnet/npdu.h"
#include "bacnet/abort.h"
#include "bacnet/reject.h"
#include "bacnet/lso.h"
/* basic objects, services, TSM, and datalink */
#include "bacnet/basic/services.h"
#include "bacnet/basic/tsm/tsm.h"
#include "bacnet/basic/sys/debug.h"
#include "bacnet/datalink/datalink.h"

void handler_lso(
    uint8_t *service_request,
    uint16_t service_len,
    BACNET_ADDRESS *src,
    BACNET_CONFIRMED_SERVICE_DATA *service_data)
{
    BACNET_LSO_DATA data;
    int len = 0;
    int pdu_len = 0;
    BACNET_NPDU_DATA npdu_data;
    int bytes_sent = 0;
    BACNET_ADDRESS my_address;

    /* encode the NPDU portion of the packet */
    datalink_get_my_address(&my_address);
    npdu_encode_npdu_data(&npdu_data, false, service_data->priority);
    pdu_len = npdu_encode_pdu(
        &Handler_Transmit_Buffer[0], src, &my_address, &npdu_data);
    if (service_len == 0) {
        len = reject_encode_apdu(
            &Handler_Transmit_Buffer[pdu_len], service_data->invoke_id,
            REJECT_REASON_MISSING_REQUIRED_PARAMETER);
        debug_print("LSO: Missing Required Parameter. Sending Reject!\n");
        goto LSO_ABORT;
    } else if (service_data->segmented_message) {
        /* we don't support segmentation - send an abort */
        len = abort_encode_apdu(
            &Handler_Transmit_Buffer[pdu_len], service_data->invoke_id,
            ABORT_REASON_SEGMENTATION_NOT_SUPPORTED, true);
        debug_print("LSO: Segmented message.  Sending Abort!\n");
        goto LSO_ABORT;
    }

    len = lso_decode_service_request(service_request, service_len, &data);
    if (len <= 0) {
        debug_print("LSO: Unable to decode Request!\n");
    }
    if (len < 0) {
        /* bad decoding - send an abort */
        len = abort_encode_apdu(
            &Handler_Transmit_Buffer[pdu_len], service_data->invoke_id,
            ABORT_REASON_OTHER, true);
        debug_print("LSO: Bad Encoding.  Sending Abort!\n");
        goto LSO_ABORT;
    }

    /*
     ** Process Life Safety Operation Here
     */
    debug_fprintf(
        stderr,
        "Life Safety Operation: Received operation %d from process id %lu "
        "for object %lu\n",
        data.operation, (unsigned long)data.processId,
        (unsigned long)data.targetObject.instance);
    len = encode_simple_ack(
        &Handler_Transmit_Buffer[pdu_len], service_data->invoke_id,
        SERVICE_CONFIRMED_LIFE_SAFETY_OPERATION);
    debug_print("Life Safety Operation: "
                "Sending Simple Ack!\n");
LSO_ABORT:
    pdu_len += len;
    bytes_sent = datalink_send_pdu(
        src, &npdu_data, &Handler_Transmit_Buffer[0], pdu_len);
    if (bytes_sent <= 0) {
        debug_perror("Life Safety Operation: Failed to send PDU");
    }

    return;
}
