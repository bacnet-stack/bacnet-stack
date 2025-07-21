/**
 * @file
 * @brief Send an UnconfirmedPrivateTransfer-Request.
 * @author Steve Karg <skarg@users.sourceforge.net>
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
#include "bacnet/ptransfer.h"
/* some demo stuff needed */
#include "bacnet/basic/binding/address.h"
#include "bacnet/basic/object/device.h"
#include "bacnet/basic/services.h"
#include "bacnet/basic/tsm/tsm.h"
#include "bacnet/datalink/datalink.h"
#include "bacnet/basic/sys/debug.h"

/**
 * @brief Sends an UnconfirmedPrivateTransfer-Request.
 * @ingroup BIBB-PT-A
 * @param dest [in] The destination address information (may be a broadcast).
 * @param data [in] The information about the private transfer to be sent.
 * @return Size of the message sent (bytes), or a negative value on error.
 */
int Send_UnconfirmedPrivateTransfer(
    BACNET_ADDRESS *dest, const BACNET_PRIVATE_TRANSFER_DATA *data)
{
    int len = 0;
    int pdu_len = 0;
    int bytes_sent = 0;
    BACNET_NPDU_DATA npdu_data;
    BACNET_ADDRESS my_address;

    if (!dcc_communication_enabled()) {
        return bytes_sent;
    }
    datalink_get_my_address(&my_address);
    /* encode the NPDU portion of the packet */
    npdu_encode_npdu_data(&npdu_data, false, MESSAGE_PRIORITY_NORMAL);
    pdu_len = npdu_encode_pdu(
        &Handler_Transmit_Buffer[0], dest, &my_address, &npdu_data);
    /* encode the APDU portion of the packet */
    len = uptransfer_encode_apdu(&Handler_Transmit_Buffer[pdu_len], data);
    pdu_len += len;
    bytes_sent = datalink_send_pdu(
        dest, &npdu_data, &Handler_Transmit_Buffer[0], pdu_len);
    if (bytes_sent <= 0) {
        debug_perror("Failed to Send UnconfirmedPrivateTransfer Request");
    }

    return bytes_sent;
}
