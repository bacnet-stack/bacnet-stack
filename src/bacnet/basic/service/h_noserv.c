/**
 * @file
 * @brief A basic unrecognized/unsupported service handler
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
#include "bacnet/reject.h"
/* basic objects, services, TSM, and datalink */
#include "bacnet/basic/object/device.h"
#include "bacnet/basic/tsm/tsm.h"
#include "bacnet/basic/services.h"
#include "bacnet/basic/sys/debug.h"
#include "bacnet/datalink/datalink.h"

/** Handler to be invoked when a Service request is received for which no
 *  handler has been defined.
 * @ingroup MISCHNDLR
 * This handler builds a Reject response packet, and sends it.
 *
 * @param service_request [in] The contents of the service request (unused).
 * @param service_len [in] The length of the service_request (unused).
 * @param src [in] BACNET_ADDRESS of the source of the message
 * @param service_data [in] The BACNET_CONFIRMED_SERVICE_DATA information
 *                          decoded from the APDU header of this message.
 */
void handler_unrecognized_service(
    uint8_t *service_request,
    uint16_t service_len,
    BACNET_ADDRESS *src,
    BACNET_CONFIRMED_SERVICE_DATA *service_data)
{
    int len = 0;
    int pdu_len = 0;
    int bytes_sent = 0;
    BACNET_NPDU_DATA npdu_data;
    BACNET_ADDRESS my_address;

    (void)service_request;
    (void)service_len;

    /* encode the NPDU portion of the packet */
    datalink_get_my_address(&my_address);
    npdu_encode_npdu_data(&npdu_data, false, service_data->priority);
    pdu_len = npdu_encode_pdu(
        &Handler_Transmit_Buffer[0], src, &my_address, &npdu_data);
    /* encode the APDU portion of the packet */
    len = reject_encode_apdu(
        &Handler_Transmit_Buffer[pdu_len], service_data->invoke_id,
        REJECT_REASON_UNRECOGNIZED_SERVICE);
    pdu_len += len;
    /* send the data */
    bytes_sent = datalink_send_pdu(
        src, &npdu_data, &Handler_Transmit_Buffer[0], pdu_len);
    if (bytes_sent > 0) {
        debug_print("Sent Reject!\n");
    } else {
        debug_perror("Failed to Send Reject");
    }
}
