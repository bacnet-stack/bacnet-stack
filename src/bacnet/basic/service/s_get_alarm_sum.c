/**
 * @file
 * @author Daniel Blazevic <daniel.blazevic@gmail.com>
 * @date 2014
 * @brief Get Alarm Summary Request
 *
 * @section LICENSE
 *
 * Copyright (C) 2014 Daniel Blazevic <daniel.blazevic@gmail.com>
 *
 * SPDX-License-Identifier: MIT
 *
 * @section DESCRIPTION
 *
 * The Get Alarm Summary Request is used by a client BACnet-user to
 * obtain a summary of "active alarms." The term "active alarm" refers to
 * BACnet standard objects that have an Event_State property whose value is
 * not equal to NORMAL and a Notify_Type property whose value is ALARM.
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
#include "bacnet/get_alarm_sum.h"
/* basic services, TSM, binding, and datalink */
#include "bacnet/basic/binding/address.h"
#include "bacnet/basic/tsm/tsm.h"
#include "bacnet/datalink/datalink.h"
#include "bacnet/basic/services.h"

uint8_t Send_Get_Alarm_Summary_Address(BACNET_ADDRESS *dest, uint16_t max_apdu)
{
    int len = 0;
    int pdu_len = 0;
    uint8_t invoke_id = 0;
    BACNET_NPDU_DATA npdu_data;
    BACNET_ADDRESS my_address;
#if PRINT_ENABLED
    int bytes_sent = 0;
#endif

    /* is there a tsm available? */
    invoke_id = tsm_next_free_invokeID();
    if (invoke_id) {
        datalink_get_my_address(&my_address);
        /* encode the NPDU portion of the packet */
        npdu_encode_npdu_data(&npdu_data, true, MESSAGE_PRIORITY_NORMAL);

        pdu_len = npdu_encode_pdu(
            &Handler_Transmit_Buffer[0], dest, &my_address, &npdu_data);
        /* encode the APDU portion of the packet */
        len = get_alarm_summary_encode_apdu(
            &Handler_Transmit_Buffer[pdu_len], invoke_id);

        pdu_len += len;
        if ((uint16_t)pdu_len < max_apdu) {
            tsm_set_confirmed_unsegmented_transaction(
                invoke_id, dest, &npdu_data, &Handler_Transmit_Buffer[0],
                (uint16_t)pdu_len);
#if PRINT_ENABLED
            bytes_sent =
#endif
                datalink_send_pdu(
                    dest, &npdu_data, &Handler_Transmit_Buffer[0], pdu_len);
#if PRINT_ENABLED
            if (bytes_sent <= 0) {
                fprintf(
                    stderr, "Failed to Send Get Alarm Summary Request (%s)!\n",
                    strerror(errno));
            }
#endif
        } else {
            tsm_free_invoke_id(invoke_id);
            invoke_id = 0;
#if PRINT_ENABLED
            fprintf(
                stderr,
                "Failed to Send Get Alarm Summary Request "
                "(exceeds destination maximum APDU)!\n");
#endif
        }
    }

    return invoke_id;
}

uint8_t Send_Get_Alarm_Summary(uint32_t device_id)
{
    BACNET_ADDRESS dest;
    unsigned max_apdu = 0;
    uint8_t invoke_id = 0;
    bool status = false;

    /* is the device bound? */
    status = address_get_by_device(device_id, &max_apdu, &dest);
    if (status) {
        invoke_id = Send_Get_Alarm_Summary_Address(&dest, max_apdu);
    }

    return invoke_id;
}
