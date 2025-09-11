/**
 * @file
 * @brief Send a COVNotification or SubscribeCOV-Request.
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date 2008
 * @copyright SPDX-License-Identifier: MIT
 */
#include <stddef.h>
#include <stdint.h>
#include <string.h>
/* BACnet Stack defines - first */
#include "bacnet/bacdef.h"
/* BACnet Stack API */
#include "bacnet/bacdcode.h"
#include "bacnet/dcc.h"
#include "bacnet/npdu.h"
#include "bacnet/apdu.h"
#include "bacnet/cov.h"
/* basic services, TSM, binding, and datalink */
#include "bacnet/basic/binding/address.h"
#include "bacnet/basic/tsm/tsm.h"
#include "bacnet/datalink/datalink.h"

/** Encodes an Unconfirmed COV Notification.
 * @ingroup DSCOV
 *
 * @param buffer [in,out] The buffer to build the message in for sending.
 * @param buffer_len [in] Number of bytes in the buffer
 * @param dest [in] Destination address
 * @param npdu_data [in] Network Layer information
 * @param cov_data [in]  The COV update information to be encoded.
 * @return Size of the message sent (bytes), or a negative value on error.
 */
int ucov_notify_encode_pdu(
    uint8_t *buffer,
    unsigned buffer_len,
    BACNET_ADDRESS *dest,
    BACNET_NPDU_DATA *npdu_data,
    const BACNET_COV_DATA *cov_data)
{
    int len = 0;
    int pdu_len = 0;
    BACNET_ADDRESS my_address;
    datalink_get_my_address(&my_address);

    /* unconfirmed is a broadcast */
    datalink_get_broadcast_address(dest);
    /* encode the NPDU portion of the packet */
    npdu_encode_npdu_data(npdu_data, false, MESSAGE_PRIORITY_NORMAL);
    pdu_len = npdu_encode_pdu(&buffer[0], dest, &my_address, npdu_data);

    /* encode the APDU portion of the packet */
    len = ucov_notify_encode_apdu(
        &buffer[pdu_len], buffer_len - pdu_len, cov_data);
    if (len) {
        pdu_len += len;
    } else {
        pdu_len = 0;
    }

    return pdu_len;
}

/** Sends an Unconfirmed COV Notification.
 * @ingroup DSCOV
 *
 * @param buffer [in,out] The buffer to build the message in for sending.
 * @param buffer_len [in] Number of bytes in the buffer
 * @param cov_data [in]  The COV update information to be encoded.
 * @return Size of the message sent (bytes), or a negative value on error.
 */
int Send_UCOV_Notify(
    uint8_t *buffer, unsigned buffer_len, const BACNET_COV_DATA *cov_data)
{
    int pdu_len = 0;
    BACNET_ADDRESS dest;
    int bytes_sent = 0;
    BACNET_NPDU_DATA npdu_data;

    pdu_len =
        ucov_notify_encode_pdu(buffer, buffer_len, &dest, &npdu_data, cov_data);
    bytes_sent = datalink_send_pdu(&dest, &npdu_data, &buffer[0], pdu_len);

    return bytes_sent;
}

/** Sends a COV Subscription request.
 * @ingroup DSCOV
 *
 * @param device_id [in] ID of the destination device
 * @param cov_data [in]  The COV subscription information to be encoded.
 * @return invoke id of outgoing message, or 0 if communication is disabled or
 *         no slot is available from the tsm for sending.
 */
uint8_t Send_COV_Subscribe(
    uint32_t device_id, const BACNET_SUBSCRIBE_COV_DATA *cov_data)
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
        if (cov_data->covSubscribeToProperty) {
            /* subscribe to 1 property */
            len = cov_subscribe_property_encode_apdu(
                &Handler_Transmit_Buffer[pdu_len],
                sizeof(Handler_Transmit_Buffer) - pdu_len, invoke_id, cov_data);
        } else {
            /* subscribe to object */
            len = cov_subscribe_encode_apdu(
                &Handler_Transmit_Buffer[pdu_len],
                sizeof(Handler_Transmit_Buffer) - pdu_len, invoke_id, cov_data);
        }
        pdu_len += len;
        /* will it fit in the sender?
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
                debug_perror("Failed to Send SubscribeCOV Request");
            }
        } else {
            tsm_free_invoke_id(invoke_id);
            invoke_id = 0;
            debug_fprintf(
                stderr,
                "Failed to Send SubscribeCOV Request "
                "(exceeds destination maximum APDU)!\n");
        }
    }

    return invoke_id;
}
