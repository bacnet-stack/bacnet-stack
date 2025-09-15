/**
 * @file
 * @brief Send BACnet Who-Is request.
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
#include "bacnet/whoami.h"
#include "bacnet/bacenum.h"
/* some demo stuff needed */
#include "bacnet/basic/binding/address.h"
#include "bacnet/basic/services.h"
#include "bacnet/basic/sys/debug.h"
#include "bacnet/basic/tsm/tsm.h"
#include "bacnet/datalink/datalink.h"

/**
 * @brief Send a Who-Am-I service request to a remote network
 * @param target_address [in] BACnet address of the target network
 * @param vendor_id the identity of the vendor of the device initiating
 *  the Who-Am-I service request.
 * @param model_name the model name of the device initiating the Who-Am-I
 *  service request.
 * @param serial_number the serial identifier of the device initiating
 *  the Who-Am-I service request.
 * @return number of bytes sent to the network
 */
int Send_Who_Am_I_To_Network(
    BACNET_ADDRESS *target_address,
    uint16_t vendor_id,
    const BACNET_CHARACTER_STRING *model_name,
    const BACNET_CHARACTER_STRING *serial_number)
{
    int len = 0;
    int pdu_len = 0;
    int bytes_sent = 0;
    BACNET_NPDU_DATA npdu_data;
    BACNET_ADDRESS my_address;

    datalink_get_my_address(&my_address);
    /* encode the NPDU portion of the packet */
    npdu_encode_npdu_data(&npdu_data, false, MESSAGE_PRIORITY_NORMAL);

    pdu_len = npdu_encode_pdu(
        &Handler_Transmit_Buffer[0], target_address, &my_address, &npdu_data);
    /* encode the APDU portion of the packet */
    len = who_am_i_request_service_encode(
        &Handler_Transmit_Buffer[pdu_len], vendor_id, model_name,
        serial_number);
    pdu_len += len;
    bytes_sent = datalink_send_pdu(
        target_address, &npdu_data, &Handler_Transmit_Buffer[0], pdu_len);
    if (bytes_sent <= 0) {
        debug_perror("Failed to Send Who-Am-I-Request");
    }

    return bytes_sent;
}

/**
 * @brief Send an I-Am broadcast message in response to Who-Is message
 * @param buffer [in] The buffer to use for building and sending the message.
 * @return The number of bytes sent to the network.
 */
int Send_Who_Am_I_Broadcast(
    uint16_t device_vendor_id,
    const char *device_model_name,
    const char *device_serial_number)
{
    int bytes_sent = 0;
    BACNET_CHARACTER_STRING model_name = { 0 }, serial_number = { 0 };
    BACNET_ADDRESS dest = { 0 };

    datalink_get_broadcast_address(&dest);
    characterstring_init_ansi(&model_name, device_model_name);
    characterstring_init_ansi(&serial_number, device_serial_number);
    bytes_sent = Send_Who_Am_I_To_Network(
        &dest, device_vendor_id, &model_name, &serial_number);

    return bytes_sent;
}
