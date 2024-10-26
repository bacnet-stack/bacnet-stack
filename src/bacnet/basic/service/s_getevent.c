/**************************************************************************
 *
 * Copyright (C) 2015 bowe
 *
 * SPDX-License-Identifier: MIT
 *
 *********************************************************************/
#include <stddef.h>
#include <stdint.h>
#include <errno.h>
#include <string.h>
/* BACnet Stack defines - first */
#include "bacnet/bacdef.h"
/* BACnet Stack API */
#include "bacnet/bacdcode.h"
#include "bacnet/npdu.h"
#include "bacnet/apdu.h"
#include "bacnet/dcc.h"
#include "bacnet/getevent.h"
/* some demo stuff needed */
#include "bacnet/basic/binding/address.h"
#include "bacnet/basic/tsm/tsm.h"
#include "bacnet/datalink/datalink.h"
#include "bacnet/basic/services.h"

/** @file s_getevent.c  Send a GetEventInformation request. */

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
#if PRINT_ENABLED
    int bytes_sent = 0;
#endif
    uint8_t invoke_id = 0;
    BACNET_NPDU_DATA npdu_data;
    BACNET_ADDRESS my_address;

    datalink_get_my_address(&my_address);
    /* encode the NPDU portion of the packet */
    npdu_encode_npdu_data(&npdu_data, false, MESSAGE_PRIORITY_NORMAL);

    pdu_len = npdu_encode_pdu(
        &Handler_Transmit_Buffer[0], target_address, &my_address, &npdu_data);

    invoke_id = tsm_next_free_invokeID();
    if (invoke_id) {
        /* encode the APDU portion of the packet */
        len = getevent_encode_apdu(
            &Handler_Transmit_Buffer[pdu_len], invoke_id,
            lastReceivedObjectIdentifier);
        pdu_len += len;
#if PRINT_ENABLED
        bytes_sent =
#endif
            datalink_send_pdu(
                target_address, &npdu_data, &Handler_Transmit_Buffer[0],
                pdu_len);
#if PRINT_ENABLED
        if (bytes_sent <= 0) {
            fprintf(
                stderr, "Failed to Send GetEventInformation Request (%s)!\n",
                strerror(errno));
        }
#endif
    } else {
        tsm_free_invoke_id(invoke_id);
        invoke_id = 0;
#if PRINT_ENABLED
        fprintf(
            stderr,
            "Failed to Send GetEventInformation Request "
            "(exceeds destination maximum APDU)!\n");
#endif
    }
    return invoke_id;
}

/** Send a global GetEventInformation request.
 */
uint8_t Send_GetEvent_Global(void)
{
    BACNET_ADDRESS dest;

    if (!dcc_communication_enabled()) {
        return 0;
    }

    datalink_get_broadcast_address(&dest);

    return Send_GetEvent(&dest, NULL);
}
