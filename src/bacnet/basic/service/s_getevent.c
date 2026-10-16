/**
 * @file
 * @brief Send a GetEventInformation request.
 * @author Boris Weitsch <bow2@users.sourceforge.net>
 * @date 2015
 * @copyright SPDX-License-Identifier: MIT
 */
#include <stddef.h>
#include <stdint.h>
#include <string.h>
/* BACnet Stack defines - first */
#include "bacnet/bacdef.h"
/* BACnet Stack API */
#include "bacnet/bacdcode.h"
#include "bacnet/npdu.h"
#include "bacnet/apdu.h"
#include "bacnet/getevent.h"
/* some demo stuff needed */
#include "bacnet/basic/binding/address.h"
#include "bacnet/basic/tsm/tsm.h"
#include "bacnet/basic/object/device.h"
#include "bacnet/datalink/datalink.h"
#include "bacnet/basic/services.h"
#include "bacnet/basic/sys/debug.h"

/** Send a GetEventInformation request to a remote network for a specific
 * device, a range, or any device.
 * @param target_address [in] BACnet address of target or broadcast
 */
uint8_t Send_GetEvent(
    BACNET_ADDRESS *target_address,
    const BACNET_OBJECT_ID *lastReceivedObjectIdentifier)
{
    int len = 0;
    int pdu_len = 0;
    int bytes_sent = 0;
    uint8_t invoke_id = 0;
    BACNET_NPDU_DATA npdu_data;
    BACNET_ADDRESS my_address;

    invoke_id = tsm_next_free_invokeID();
    if (invoke_id) {
        datalink_get_my_address(&my_address);
        /* encode the NPDU portion of the packet */
        npdu_encode_npdu_data(&npdu_data, true, MESSAGE_PRIORITY_NORMAL);
        pdu_len = npdu_encode_pdu(
            &Handler_Transmit_Buffer[0], target_address, &my_address,
            &npdu_data);
        /* encode the APDU portion of the packet */
        len = getevent_encode_apdu(
            &Handler_Transmit_Buffer[pdu_len], invoke_id,
            lastReceivedObjectIdentifier);
        pdu_len += len;
        /* register the destination so that the reply is accepted */
        tsm_set_confirmed_unsegmented_transaction(
            invoke_id, target_address, &npdu_data, &Handler_Transmit_Buffer[0],
            (uint16_t)pdu_len);
        bytes_sent = datalink_send_pdu(
            target_address, &npdu_data, &Handler_Transmit_Buffer[0], pdu_len);
        if (bytes_sent <= 0) {
            debug_perror("Failed to Send GetEventInformation Request");
        }
    } else {
        debug_fprintf(
            stderr,
            "Failed to Send GetEventInformation Request "
            "(no free invoke ID)!\n");
    }
    return invoke_id;
}
