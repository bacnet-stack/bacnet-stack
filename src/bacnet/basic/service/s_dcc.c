/**
 * @file
 * @brief  Send a Device Communication Control (DCC) request.
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
/* basic services, TSM, binding, and datalink */
#include "bacnet/basic/binding/address.h"
#include "bacnet/basic/tsm/tsm.h"
#include "bacnet/basic/services.h"
#include "bacnet/basic/sys/debug.h"
#include "bacnet/datalink/datalink.h"

/** Sends a Device Communication Control (DCC) request.
 * @ingroup DMDCC
 *
 * @param device_id [in] The index to the device address in our address cache.
 * @param timeDuration [in] If non-zero, the minutes that the remote device
 *            shall ignore all APDUs except DCC and, if supported, RD APDUs.
 * @param state [in] Choice to Enable or Disable communication.
 * @param password [in] Optional password, up to 20 chars.
 * @return The invokeID of the transmitted message, or 0 on failure.
 */
uint8_t Send_Device_Communication_Control_Request(
    uint32_t device_id,
    uint16_t timeDuration,
    BACNET_COMMUNICATION_ENABLE_DISABLE state,
    const char *password)
{
    BACNET_ADDRESS dest;
    BACNET_ADDRESS my_address;
    unsigned max_apdu = 0;
    uint8_t invoke_id = 0;
    bool status = false;
    int len = 0;
    int pdu_len = 0;
    int bytes_sent = 0;
    BACNET_CHARACTER_STRING password_string;
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
        pdu_len = npdu_encode_pdu(
            &Handler_Transmit_Buffer[0], &dest, &my_address, &npdu_data);
        /* encode the APDU portion of the packet */
        characterstring_init_ansi(&password_string, password);
        len = dcc_encode_apdu(
            &Handler_Transmit_Buffer[pdu_len], invoke_id, timeDuration, state,
            password ? &password_string : NULL);
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
                debug_perror(
                    "Failed to Send DeviceCommunicationControl Request");
            }
        } else {
            tsm_free_invoke_id(invoke_id);
            invoke_id = 0;
            debug_fprintf(
                stderr,
                "Failed to Send DeviceCommunicationControl Request "
                "(exceeds destination maximum APDU)!\n");
        }
    }

    return invoke_id;
}
