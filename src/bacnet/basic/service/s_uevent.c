/**************************************************************************
 *
 * Copyright (C) 2008 John Minack
 *
 * SPDX-License-Identifier: MIT
 *
 *********************************************************************/
#include <stddef.h>
#include <stdint.h>
#include <errno.h>
#include "bacnet/event.h"
/* basic services and datalink */
#include "bacnet/datalink/datalink.h"
#include "bacnet/basic/services.h"

/** @file s_uevent.c  Send an Unconfirmed Event Notification. */

/** Sends an Unconfirmed Alarm/Event Notification.
 * @ingroup EVNOTFCN
 *
 * @param buffer [in,out] The buffer to build the message in for sending.
 * @param data [in] The information about the Event to be sent.
 * @param dest [in] The destination address information (may be a broadcast).
 * @return Size of the message sent (bytes), or a negative value on error.
 */
int Send_UEvent_Notify(
    uint8_t *buffer,
    const BACNET_EVENT_NOTIFICATION_DATA *data,
    BACNET_ADDRESS *dest)
{
    int len = 0;
    int pdu_len = 0;
    int bytes_sent = 0;
    BACNET_NPDU_DATA npdu_data;
    BACNET_ADDRESS my_address;

    datalink_get_my_address(&my_address);
    /* encode the NPDU portion of the packet */
    npdu_encode_npdu_data(&npdu_data, false, MESSAGE_PRIORITY_NORMAL);
    pdu_len = npdu_encode_pdu(buffer, dest, &my_address, &npdu_data);
    /* encode the APDU portion of the packet */
    len = uevent_notify_encode_apdu(&buffer[pdu_len], data);
    pdu_len += len;
    /* send the data */
    bytes_sent = datalink_send_pdu(dest, &npdu_data, &buffer[0], pdu_len);

    return bytes_sent;
}
