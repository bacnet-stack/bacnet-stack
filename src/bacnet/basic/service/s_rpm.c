/**************************************************************************
 *
 * Copyright (C) 2008 Steve Karg <skarg@users.sourceforge.net>
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
#include "bacnet/rpm.h"
/* some demo stuff needed */
#include "bacnet/basic/object/device.h"
#include "bacnet/datalink/datalink.h"
#include "bacnet/basic/binding/address.h"
#include "bacnet/basic/tsm/tsm.h"
#include "bacnet/basic/services.h"

/** @file s_rpm.c  Send Read Property Multiple request. */

/** Sends a Read Property Multiple request.
 * @ingroup DSRPM
 *
 * @param pdu [out] Buffer to build the outgoing message into
 * @param max_pdu [in] Length of the pdu buffer.
 * @param device_id [in] ID of the destination device
 * @param read_access_data [in] Ptr to structure with the linked list of
 *        properties to be read.
 * @return invoke id of outgoing message, or 0 if device is not bound or no tsm
 * available
 */
uint8_t Send_Read_Property_Multiple_Request(
    uint8_t *pdu,
    size_t max_pdu,
    uint32_t device_id, /* destination device */
    BACNET_READ_ACCESS_DATA *read_access_data)
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
        len = rpm_encode_apdu(
            &pdu[pdu_len], max_pdu - pdu_len, invoke_id, read_access_data);
        if (len <= 0) {
            return 0;
        }
        pdu_len += len;
        /* is it small enough for the destination to receive?
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
                    "Failed to Send ReadPropertyMultiple Request (%s)!\n",
                    strerror(errno));
            }
#endif
        } else {
            tsm_free_invoke_id(invoke_id);
            invoke_id = 0;
#if PRINT_ENABLED
            fprintf(
                stderr,
                "Failed to Send ReadPropertyMultiple Request "
                "(exceeds destination maximum APDU)!\n");
#endif
        }
    }

    return invoke_id;
}
