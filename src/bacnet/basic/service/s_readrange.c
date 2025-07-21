/**
 * @file
 * @brief Send a ReadRange-Request message
 * @author Peter Mc Shane <petermcs@users.sourceforge.net>
 * @date 2009
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
#include "bacnet/dcc.h"
#include "bacnet/rpm.h"
#include "bacnet/readrange.h"
/* some demo stuff needed */
#include "bacnet/basic/binding/address.h"
#include "bacnet/basic/tsm/tsm.h"
#include "bacnet/basic/object/device.h"
#include "bacnet/datalink/datalink.h"
#include "bacnet/basic/services.h"
#include "bacnet/basic/sys/debug.h"

/**
 * @brief Send a ReadRange-Request message
 * @param device_id [in] ID of the destination device
 * @param data [in] The data representing the ReadRange-Request
 * @return invoke id of outgoing message, or 0 on failure.
 */
uint8_t Send_ReadRange_Request(
    uint32_t device_id, /* destination device */
    const BACNET_READ_RANGE_DATA *read_access_data)
{
    BACNET_ADDRESS dest;
    BACNET_ADDRESS my_address;
    unsigned max_apdu = 0;
    uint8_t invoke_id = 0;
    bool status = false;
    int len = 0;
    int pdu_len = 0;
    int bytes_sent = 0;
    BACNET_NPDU_DATA npdu_data;

    if (!dcc_communication_enabled()) {
        return 0;
    }
    /* is the device bound? */
    status = address_get_by_device(device_id, &max_apdu, &dest);
    /* is there a tsm available? */
    if (status) {
        invoke_id = tsm_next_free_invokeID();
    }
    if (invoke_id) {
        /* encode the NPDU portion of the packet */
        datalink_get_my_address(&my_address);
        npdu_encode_npdu_data(&npdu_data, true, MESSAGE_PRIORITY_NORMAL);
        pdu_len = npdu_encode_pdu(
            &Handler_Transmit_Buffer[0], &dest, &my_address, &npdu_data);

        /* encode the APDU portion of the packet */
        len = rr_encode_apdu(
            &Handler_Transmit_Buffer[pdu_len], invoke_id, read_access_data);
        if (len <= 0) {
            return 0;
        }

        pdu_len += len;
        /* is it small enough for the the destination to receive?
           note: if there is a bottleneck router in between
           us and the destination, we won't know unless
           we have a way to check for that and update the
           max_apdu in the address binding table. */
        if ((unsigned)pdu_len < max_apdu) {
            tsm_set_confirmed_unsegmented_transaction(
                invoke_id, &dest, &npdu_data, &Handler_Transmit_Buffer[0],
                (uint16_t)pdu_len);
            bytes_sent = datalink_send_pdu(
                &dest, &npdu_data, &Handler_Transmit_Buffer[0], pdu_len);
            if (bytes_sent <= 0) {
                debug_perror("Failed to Send ReadRange Request");
            }
        } else {
            tsm_free_invoke_id(invoke_id);
            invoke_id = 0;
            debug_fprintf(
                stderr,
                "Failed to Send ReadRange Request (exceeds destination "
                "maximum APDU)!\n");
        }
    }

    return invoke_id;
}
