/**
 * @file
 * @brief Send BACnet Who-Has requests
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date 2006
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
#include "bacnet/whohas.h"
/* basic services, TSM, and datalink */
#include "bacnet/basic/services.h"
#include "bacnet/basic/sys/debug.h"
#include "bacnet/basic/tsm/tsm.h"
#include "bacnet/datalink/datalink.h"

/**
 * @brief Send a Who-Has request for a device which has a named Object.
 * @ingroup BIBB-DM-DOB-A
 * If low_limit and high_limit both are -1, then the device ID range is
 * unlimited. If low_limit and high_limit have the same non-negative value, then
 * only that device will respond. Otherwise, low_limit must be less than
 * high_limit for a range.
 * @param low_limit [in] Device Instance Low Range, 0 - 4,194,303 or -1
 * @param high_limit [in] Device Instance High Range, 0 - 4,194,303 or -1
 * @param object_name [in] The Name of the desired Object.
 */
void Send_WhoHas_Name(
    int32_t low_limit, int32_t high_limit, const char *object_name)
{
    int len = 0;
    int pdu_len = 0;
    BACNET_ADDRESS dest;
    int bytes_sent = 0;
    BACNET_WHO_HAS_DATA data;
    BACNET_NPDU_DATA npdu_data;
    BACNET_ADDRESS my_address;

    /* if we are forbidden to send, don't send! */
    if (!dcc_communication_enabled()) {
        return;
    }
    /* Who-Has is a global broadcast */
    datalink_get_broadcast_address(&dest);
    datalink_get_my_address(&my_address);
    /* encode the NPDU portion of the packet */
    npdu_encode_npdu_data(&npdu_data, false, MESSAGE_PRIORITY_NORMAL);
    pdu_len = npdu_encode_pdu(
        &Handler_Transmit_Buffer[0], &dest, &my_address, &npdu_data);

    /* encode the APDU portion of the packet */
    data.low_limit = low_limit;
    data.high_limit = high_limit;
    data.is_object_name = true;
    characterstring_init_ansi(&data.object.name, object_name);
    len = whohas_encode_apdu(&Handler_Transmit_Buffer[pdu_len], &data);
    pdu_len += len;
    /* send the data */
    bytes_sent = datalink_send_pdu(
        &dest, &npdu_data, &Handler_Transmit_Buffer[0], pdu_len);
    if (bytes_sent <= 0) {
        debug_perror("Failed to Send Who-Has Request");
    }
}

/**
 * @brief Send a Who-Has request for a device which has a specific
 * Object type and ID.
 * @ingroup BIBB-DM-DOB-A
 * If low_limit and high_limit both are -1, then the device ID range is
 * unlimited. If low_limit and high_limit have the same non-negative value, then
 * only that device will respond. Otherwise, low_limit must be less than
 * high_limit for a range.
 * @param low_limit [in] Device Instance Low Range, 0 - 4,194,303 or -1
 * @param high_limit [in] Device Instance High Range, 0 - 4,194,303 or -1
 * @param object_type [in] The BACNET_OBJECT_TYPE of the desired Object.
 * @param object_instance [in] The ID of the desired Object.
 */
void Send_WhoHas_Object(
    int32_t low_limit,
    int32_t high_limit,
    BACNET_OBJECT_TYPE object_type,
    uint32_t object_instance)
{
    int len = 0;
    int pdu_len = 0;
    BACNET_ADDRESS dest;
    int bytes_sent = 0;
    BACNET_WHO_HAS_DATA data;
    BACNET_NPDU_DATA npdu_data;
    BACNET_ADDRESS my_address;

    /* if we are forbidden to send, don't send! */
    if (!dcc_communication_enabled()) {
        return;
    }
    /* Who-Has is a global broadcast */
    datalink_get_broadcast_address(&dest);
    datalink_get_my_address(&my_address);
    /* encode the NPDU portion of the packet */
    npdu_encode_npdu_data(&npdu_data, false, MESSAGE_PRIORITY_NORMAL);
    pdu_len = npdu_encode_pdu(
        &Handler_Transmit_Buffer[0], &dest, &my_address, &npdu_data);
    /* encode the APDU portion of the packet */
    data.low_limit = low_limit;
    data.high_limit = high_limit;
    data.is_object_name = false;
    data.object.identifier.type = object_type;
    data.object.identifier.instance = object_instance;
    len = whohas_encode_apdu(&Handler_Transmit_Buffer[pdu_len], &data);
    pdu_len += len;
    bytes_sent = datalink_send_pdu(
        &dest, &npdu_data, &Handler_Transmit_Buffer[0], pdu_len);
    if (bytes_sent <= 0) {
        debug_perror("Failed to Send Who-Has Request");
    }
}
