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
#include <errno.h>
/* BACnet Stack defines - first */
#include "bacnet/bacdef.h"
/* BACnet Stack API */
#include "bacnet/bacdcode.h"
#include "bacnet/apdu.h"
#include "bacnet/npdu.h"
#include "bacnet/abort.h"
#include "bacnet/lso.h"
/* basic objects, services, TSM, and datalink */
#include "bacnet/basic/services.h"
#include "bacnet/basic/tsm/tsm.h"
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
#if PRINT_ENABLED
    int bytes_sent = 0;
#endif
    BACNET_ADDRESS my_address;

    /* encode the NPDU portion of the packet */
    datalink_get_my_address(&my_address);
    npdu_encode_npdu_data(&npdu_data, false, MESSAGE_PRIORITY_NORMAL);
    pdu_len = npdu_encode_pdu(
        &Handler_Transmit_Buffer[0], src, &my_address, &npdu_data);
    if (service_data->segmented_message) {
        /* we don't support segmentation - send an abort */
        len = abort_encode_apdu(
            &Handler_Transmit_Buffer[pdu_len], service_data->invoke_id,
            ABORT_REASON_SEGMENTATION_NOT_SUPPORTED, true);
#if PRINT_ENABLED
        fprintf(stderr, "LSO: Segmented message.  Sending Abort!\n");
#endif
        goto LSO_ABORT;
    }

    len = lso_decode_service_request(service_request, service_len, &data);
#if PRINT_ENABLED
    if (len <= 0) {
        fprintf(stderr, "LSO: Unable to decode Request!\n");
    }
#endif
    if (len < 0) {
        /* bad decoding - send an abort */
        len = abort_encode_apdu(
            &Handler_Transmit_Buffer[pdu_len], service_data->invoke_id,
            ABORT_REASON_OTHER, true);
#if PRINT_ENABLED
        fprintf(stderr, "LSO: Bad Encoding.  Sending Abort!\n");
#endif
        goto LSO_ABORT;
    }

    /*
     ** Process Life Safety Operation Here
     */
#if PRINT_ENABLED
    fprintf(
        stderr,
        "Life Safety Operation: Received operation %d from process id %lu "
        "for object %lu\n",
        data.operation, (unsigned long)data.processId,
        (unsigned long)data.targetObject.instance);
#endif

    len = encode_simple_ack(
        &Handler_Transmit_Buffer[pdu_len], service_data->invoke_id,
        SERVICE_CONFIRMED_LIFE_SAFETY_OPERATION);
#if PRINT_ENABLED
    fprintf(
        stderr,
        "Life Safety Operation: "
        "Sending Simple Ack!\n");
#endif

LSO_ABORT:
    pdu_len += len;
#if PRINT_ENABLED
    bytes_sent =
#endif
        datalink_send_pdu(
            src, &npdu_data, &Handler_Transmit_Buffer[0], pdu_len);
#if PRINT_ENABLED
    if (bytes_sent <= 0) {
        fprintf(
            stderr,
            "Life Safety Operation: "
            "Failed to send PDU (%s)!\n",
            strerror(errno));
    }
#endif

    return;
}
