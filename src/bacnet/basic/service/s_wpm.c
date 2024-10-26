/**
 * @file
 * @author Daniel Blazevic <daniel.blazevic@gmail.com>
 * @date 2013
 * @brief Send Write Property Multiple request
 *
 * @section LICENSE
 *
 * Copyright (C) 2013 Daniel Blazevic <daniel.blazevic@gmail.com>
 *
 * SPDX-License-Identifier: MIT
 */
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
#include "bacnet/wpm.h"
/* basic services, TSM, binding, and datalink */
#include "bacnet/datalink/datalink.h"
#include "bacnet/basic/binding/address.h"
#include "bacnet/basic/services.h"
#include "bacnet/basic/tsm/tsm.h"

/** @file s_wpm.c  Send Write Property Multiple request. */

/** Sends a Write Property Multiple request.
 * @param pdu [out] Buffer to build the outgoing message into
 * @param max_pdu [in] Length of the pdu buffer.
 * @param device_id [in] ID of the destination device
 * @param write_access_data [in] Ptr to structure with the linked list of
 *        objects and properties to be written.
 * @return invoke id of outgoing message, or 0 if device is not bound or no tsm
 * available
 */
uint8_t Send_Write_Property_Multiple_Request(
    uint8_t *pdu,
    size_t max_pdu,
    uint32_t device_id,
    BACNET_WRITE_ACCESS_DATA *write_access_data)
{
    BACNET_ADDRESS dest;
    BACNET_ADDRESS my_address;
    unsigned max_apdu = 0;
    uint8_t invoke_id = 0;
    bool status = false;
    int len = 0;
    int pdu_len = 0;
#if PRINT_ENABLED
    int bytes_sent = 0;
#endif
    BACNET_NPDU_DATA npdu_data;

    /* if we are forbidden to send, don't send! */
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
        pdu_len = npdu_encode_pdu(&pdu[0], &dest, &my_address, &npdu_data);
        /* encode the APDU portion of the packet */
        len = wpm_encode_apdu(
            &pdu[pdu_len], max_pdu - pdu_len, invoke_id, write_access_data);
        if (len <= 0) {
            return 0;
        }
        pdu_len += len;
        /* will it fit in the sender?
           note: if there is a bottleneck router in between
           us and the destination, we won't know unless
           we have a way to check for that and update the
           max_apdu in the address binding table. */
        if ((unsigned)pdu_len < max_apdu) {
            tsm_set_confirmed_unsegmented_transaction(
                invoke_id, &dest, &npdu_data, &pdu[0], (uint16_t)pdu_len);
#if PRINT_ENABLED
            bytes_sent =
#endif
                datalink_send_pdu(&dest, &npdu_data, &pdu[0], pdu_len);
#if PRINT_ENABLED
            if (bytes_sent <= 0) {
                fprintf(
                    stderr,
                    "Failed to Send WritePropertyMultiple Request (%s)!\n",
                    strerror(errno));
            }
#endif
        } else {
            tsm_free_invoke_id(invoke_id);
            invoke_id = 0;
#if PRINT_ENABLED
            fprintf(
                stderr,
                "Failed to Send WritePropertyMultiple Request "
                "(exceeds destination maximum APDU)!\n");
#endif
        }
    }

    return invoke_id;
}
