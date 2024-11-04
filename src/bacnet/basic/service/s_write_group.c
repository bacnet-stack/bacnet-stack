/**
 * @file
 * @brief Header file for a basic BACnet WriteGroup-Reqeust service send
 * @author Steve Karg
 * @date October 2024
 * @copyright SPDX-License-Identifier: MIT
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
#include "bacnet/write_group.h"
/* some demo stuff needed */
#include "bacnet/basic/binding/address.h"
#include "bacnet/basic/services.h"
#include "bacnet/basic/tsm/tsm.h"
#include "bacnet/datalink/datalink.h"

int Send_Write_Group(BACNET_ADDRESS *dest, const BACNET_WRITE_GROUP_DATA *data)
{
    int len = 0;
    int pdu_len = 0;
    int bytes_sent = 0;
    BACNET_NPDU_DATA npdu_data;
    BACNET_ADDRESS my_address;
    size_t apdu_size;

    if (!dcc_communication_enabled()) {
        return bytes_sent;
    }
    datalink_get_my_address(&my_address);
    /* encode the NPDU portion of the packet */
    npdu_encode_npdu_data(&npdu_data, false, MESSAGE_PRIORITY_NORMAL);
    pdu_len = npdu_encode_pdu(
        &Handler_Transmit_Buffer[0], dest, &my_address, &npdu_data);

    /* encode the APDU portion of the packet */
    Handler_Transmit_Buffer[pdu_len++] = PDU_TYPE_UNCONFIRMED_SERVICE_REQUEST;
    Handler_Transmit_Buffer[pdu_len++] = SERVICE_UNCONFIRMED_WRITE_GROUP;
    apdu_size = sizeof(Handler_Transmit_Buffer) - pdu_len;
    len = bacnet_write_group_service_request_encode(
        &Handler_Transmit_Buffer[pdu_len], apdu_size, data);
    pdu_len += len;
    bytes_sent = datalink_send_pdu(
        dest, &npdu_data, &Handler_Transmit_Buffer[0], pdu_len);
    if (bytes_sent <= 0) {
#if PRINT_ENABLED
        fprintf(
            stderr, "Failed to Send WriteGroup-Request (%s)!\n",
            strerror(errno));
#endif
    }

    return bytes_sent;
}
