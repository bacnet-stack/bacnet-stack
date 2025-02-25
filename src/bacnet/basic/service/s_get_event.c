/**
 * @file
 * @brief Get Event Request
 * @author Daniel Blazevic <daniel.blazevic@gmail.com>
 * @date 2014
 * @copyright SPDX-License-Identifier: MIT
 * @details The GetEventInformation service is used by a client BACnet-user
 *  to obtain a summary of all "active event states".
 *  The term "active event states" refers to all event-initiating objects
 *  that have an Event_State property whose value is not equal to NORMAL,
 *  or have an Acked_Transitions property, which has at least one of the
 *  bits (TO-OFFNORMAL, TO-FAULT, TONORMAL) set to FALSE.
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
#include "bacnet/basic/sys/debug.h"
#include "bacnet/datalink/datalink.h"
#include "bacnet/basic/services.h"

uint8_t Send_Get_Event_Information_Address(
    BACNET_ADDRESS *dest,
    uint16_t max_apdu,
    const BACNET_OBJECT_ID *lastReceivedObjectIdentifier)
{
    int len = 0;
    int pdu_len = 0;
    uint8_t invoke_id = 0;
    BACNET_NPDU_DATA npdu_data;
    BACNET_ADDRESS my_address;
    int bytes_sent = 0;

    /* is there a tsm available? */
    invoke_id = tsm_next_free_invokeID();
    if (invoke_id) {
        datalink_get_my_address(&my_address);
        /* encode the NPDU portion of the packet */
        npdu_encode_npdu_data(&npdu_data, true, MESSAGE_PRIORITY_NORMAL);
        pdu_len = npdu_encode_pdu(
            &Handler_Transmit_Buffer[0], dest, &my_address, &npdu_data);
        /* encode the APDU portion of the packet */
        len = getevent_encode_apdu(
            &Handler_Transmit_Buffer[pdu_len], invoke_id,
            lastReceivedObjectIdentifier);

        pdu_len += len;
        if ((uint16_t)pdu_len < max_apdu) {
            tsm_set_confirmed_unsegmented_transaction(
                invoke_id, dest, &npdu_data, &Handler_Transmit_Buffer[0],
                (uint16_t)pdu_len);
            bytes_sent = datalink_send_pdu(
                dest, &npdu_data, &Handler_Transmit_Buffer[0], pdu_len);
            if (bytes_sent <= 0) {
                debug_perror("Failed to Send Get Event Information Request");
            }
        } else {
            tsm_free_invoke_id(invoke_id);
            invoke_id = 0;
            debug_fprintf(
                stderr,
                "Failed to Send Get Event Information Request "
                "(exceeds destination maximum APDU)!\n");
        }
    }

    return invoke_id;
}

uint8_t Send_Get_Event_Information(
    uint32_t device_id, const BACNET_OBJECT_ID *lastReceivedObjectIdentifier)
{
    BACNET_ADDRESS dest = { 0 };
    unsigned max_apdu = 0;
    uint8_t invoke_id = 0;
    bool status = false;
#if BACNET_SEGMENTATION_ENABLED
    uint8_t segmentation = 0;
    uint16_t maxsegments = 0;
#endif

    /* is the device bound? */
    status = address_get_by_device(
        device_id, &max_apdu, &dest
#if BACNET_SEGMENTATION_ENABLED
        ,&segmentation, &maxsegments
#endif
    );
    if (status) {
        invoke_id = Send_Get_Event_Information_Address(
            &dest, max_apdu, lastReceivedObjectIdentifier);
    }

    return invoke_id;
}
