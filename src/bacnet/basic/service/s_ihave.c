/**************************************************************************
 *
 * Copyright (C) 2006 Steve Karg <skarg@users.sourceforge.net>
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
#include "bacnet/ihave.h"
/* some demo stuff needed */
#include "bacnet/basic/binding/address.h"
#include "bacnet/basic/tsm/tsm.h"
#include "bacnet/basic/object/device.h"
#include "bacnet/datalink/datalink.h"
#include "bacnet/basic/services.h"

/** @file s_ihave.c  Send an I-Have (property) message. */

/** Broadcast an I Have message.
 * @ingroup DMDOB
 *
 * @param device_id [in] My device ID.
 * @param object_type [in] The BACNET_OBJECT_TYPE that I Have.
 * @param object_instance [in] The Object ID that I Have.
 * @param object_name [in] The Name of the Object I Have.
 */
void Send_I_Have(
    uint32_t device_id,
    BACNET_OBJECT_TYPE object_type,
    uint32_t object_instance,
    const BACNET_CHARACTER_STRING *object_name)
{
    int len = 0;
    int pdu_len = 0;
    BACNET_ADDRESS dest;
    int bytes_sent = 0;
    BACNET_I_HAVE_DATA data;
    BACNET_NPDU_DATA npdu_data;
    BACNET_ADDRESS my_address;

    datalink_get_my_address(&my_address);
    /* if we are forbidden to send, don't send! */
    if (!dcc_communication_enabled()) {
        return;
    }
    /* Who-Has is a global broadcast */
    datalink_get_broadcast_address(&dest);
    /* encode the NPDU portion of the packet */
    npdu_encode_npdu_data(&npdu_data, false, MESSAGE_PRIORITY_NORMAL);
    pdu_len = npdu_encode_pdu(
        &Handler_Transmit_Buffer[0], &dest, &my_address, &npdu_data);

    /* encode the APDU portion of the packet */
    data.device_id.type = OBJECT_DEVICE;
    data.device_id.instance = device_id;
    data.object_id.type = object_type;
    data.object_id.instance = object_instance;
    characterstring_copy(&data.object_name, object_name);
    len = ihave_encode_apdu(&Handler_Transmit_Buffer[pdu_len], &data);
    pdu_len += len;
    /* send the data */
    bytes_sent = datalink_send_pdu(
        &dest, &npdu_data, &Handler_Transmit_Buffer[0], pdu_len);
    if (bytes_sent <= 0) {
#if PRINT_ENABLED
        fprintf(stderr, "Failed to Send I-Have Reply (%s)!\n", strerror(errno));
#endif
    }
}
