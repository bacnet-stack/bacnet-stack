/**
 * @file
 * @brief Send BACnet You-Are request.
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date June 2025
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
#include "bacnet/youare.h"
#include "bacnet/bacenum.h"
/* some demo stuff needed */
#include "bacnet/basic/binding/address.h"
#include "bacnet/basic/object/device.h"
#include "bacnet/basic/services.h"
#include "bacnet/basic/sys/debug.h"
#include "bacnet/basic/tsm/tsm.h"
#include "bacnet/datalink/datalink.h"

/**
 * @brief Send a You-Are service request to a remote network
 * @param target_address [in] BACnet address of the target network
 * @param device_id the Device Object_Identifier to be assigned
 *  in the qualified device.
 * @param vendor_id the identity of the vendor of the device that
 *  is qualified to receive this You-Are service request.
 * @param model_name the model name of the device qualified to receive
 *  the You-Are service request.
 * @param serial_number the serial identifier of the device qualified
 *  to receive the You-Are service request.
 * @param mac_address the device MAC address that is to be configured
 *  in the qualified device.
 * @return number of bytes sent to the network
 */
int Send_You_Are_To_Network(
    BACNET_ADDRESS *target_address,
    uint32_t device_id,
    uint16_t vendor_id,
    const BACNET_CHARACTER_STRING *model_name,
    const BACNET_CHARACTER_STRING *serial_number,
    const BACNET_OCTET_STRING *mac_address)
{
    int len = 0;
    int pdu_len = 0;
    int bytes_sent = 0;
    bool data_expecting_reply = false;
    BACNET_NPDU_DATA npdu_data;
    BACNET_ADDRESS my_address;

    datalink_get_my_address(&my_address);
    /* encode the NPDU portion of the packet */
    npdu_encode_npdu_data(
        &npdu_data, data_expecting_reply, MESSAGE_PRIORITY_NORMAL);
    pdu_len = npdu_encode_pdu(
        &Handler_Transmit_Buffer[0], target_address, &my_address, &npdu_data);
    /* encode the APDU portion of the packet */
    len = you_are_request_service_encode(
        &Handler_Transmit_Buffer[pdu_len], device_id, vendor_id, model_name,
        serial_number, mac_address);
    pdu_len += len;
    bytes_sent = datalink_send_pdu(
        target_address, &npdu_data, &Handler_Transmit_Buffer[0], pdu_len);
    if (bytes_sent <= 0) {
        debug_perror("Failed to Send You-Are-Request");
    }

    return bytes_sent;
}
