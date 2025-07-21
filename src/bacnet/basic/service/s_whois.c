/**
 * @file
 * @brief Send BACnet Who-Is request.
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date 2005
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
#include "bacnet/whois.h"
#include "bacnet/bacenum.h"
/* some demo stuff needed */
#include "bacnet/basic/binding/address.h"
#include "bacnet/basic/object/device.h"
#include "bacnet/basic/services.h"
#include "bacnet/basic/sys/debug.h"
#include "bacnet/basic/tsm/tsm.h"
#include "bacnet/datalink/datalink.h"

/**
 * @brief Send a Who-Is request to a remote network for a specific device,
 * a range, or any device.
 * If low_limit and high_limit both are -1, then the range is unlimited.
 * If low_limit and high_limit have the same non-negative value, then only
 * that device will respond.
 * Otherwise, low_limit must be less than high_limit.
 * @ingroup BIBB-DM-DDB-A
 * @param target_address [in] BACnet address of target router
 * @param low_limit [in] Device Instance Low Range, 0 - 4,194,303 or -1
 * @param high_limit [in] Device Instance High Range, 0 - 4,194,303 or -1
 */
void Send_WhoIs_To_Network(
    BACNET_ADDRESS *target_address, int32_t low_limit, int32_t high_limit)
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
    len = whois_encode_apdu(
        &Handler_Transmit_Buffer[pdu_len], low_limit, high_limit);
    pdu_len += len;
    bytes_sent = datalink_send_pdu(
        target_address, &npdu_data, &Handler_Transmit_Buffer[0], pdu_len);
    if (bytes_sent <= 0) {
        debug_perror("Failed to Send Who-Is Request");
    }
}

/**
 * @brief Send a global Who-Is request for a specific device, a range,
 * or any device.
 * If low_limit and high_limit both are -1, then the range is unlimited.
 * If low_limit and high_limit have the same non-negative value, then only
 * that device will respond.
 * Otherwise, low_limit must be less than high_limit.
 * @ingroup BIBB-DM-DDB-A
 * @param low_limit [in] Device Instance Low Range, 0 - 4,194,303 or -1
 * @param high_limit [in] Device Instance High Range, 0 - 4,194,303 or -1
 */
void Send_WhoIs_Global(int32_t low_limit, int32_t high_limit)
{
    BACNET_ADDRESS dest;

    if (!dcc_communication_enabled()) {
        return;
    }

    /* Who-Is is a global broadcast */
    datalink_get_broadcast_address(&dest);

    Send_WhoIs_To_Network(&dest, low_limit, high_limit);
}

/**
 * @brief Send a local Who-Is request for a specific device, a range,
 * or any device.
 * If low_limit and high_limit both are -1, then the range is unlimited.
 * If low_limit and high_limit have the same non-negative value, then only
 * that device will respond.
 * Otherwise, low_limit must be less than high_limit.
 * @ingroup BIBB-DM-DDB-A
 * @param low_limit [in] Device Instance Low Range, 0 - 4,194,303 or -1
 * @param high_limit [in] Device Instance High Range, 0 - 4,194,303 or -1
 */
void Send_WhoIs_Local(int32_t low_limit, int32_t high_limit)
{
    /* mac_len = 0 is local - set them all to zero! */
    BACNET_ADDRESS dest = { 0 };

    Send_WhoIs_To_Network(&dest, low_limit, high_limit);
}

/**
 * @brief Send a Who-Is request to a remote network for a specific device,
 * a range, or any device.
 * @ingroup BIBB-DM-DDB-A
 * If low_limit and high_limit both are -1, then the range is unlimited.
 * If low_limit and high_limit have the same non-negative value, then only
 * that device will respond.
 * Otherwise, low_limit must be less than high_limit.
 * @param target_address [in] BACnet address of target router
 * @param low_limit [in] Device Instance Low Range, 0 - 4,194,303 or -1
 * @param high_limit [in] Device Instance High Range, 0 - 4,194,303 or -1
 */
void Send_WhoIs_Remote(
    BACNET_ADDRESS *target_address, int32_t low_limit, int32_t high_limit)
{
    if (!dcc_communication_enabled()) {
        return;
    }

    Send_WhoIs_To_Network(target_address, low_limit, high_limit);
}

/**
 * @brief Send a global Who-Is request for a specific device, a range,
 * or any device.
 * @ingroup BIBB-DM-DDB-A
 * This was the original Who-Is broadcast but the code was moved to the more
 * descriptive Send_WhoIs_Global when Send_WhoIs_Local and Send_WhoIsRemote was
 * added.
 * If low_limit and high_limit both are -1, then the range is unlimited.
 * If low_limit and high_limit have the same non-negative value, then only
 * that device will respond.
 * Otherwise, low_limit must be less than high_limit.
 * @param low_limit [in] Device Instance Low Range, 0 - 4,194,303 or -1
 * @param high_limit [in] Device Instance High Range, 0 - 4,194,303 or -1
 */
void Send_WhoIs(int32_t low_limit, int32_t high_limit)
{
    Send_WhoIs_Global(low_limit, high_limit);
}
