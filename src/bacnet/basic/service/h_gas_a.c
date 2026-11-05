/**
 * @file
 * @brief GetAlarmSummary ACK service handling
 * @details The GetAlarmSummary ACK service handler is used by a client
 * BACnet-user to obtain a summary of "active alarms."
 * The term "active alarm" refers to BACnet standard objects
 * that have an Event_State property whose value is
 * not equal to NORMAL and a Notify_Type property whose
 * value is ALARM.
 * @author Daniel Blazevic <daniel.blazevic@gmail.com>
 * @date 2013
 * @copyright SPDX-License-Identifier: MIT
 */
#include <stdio.h>
/* BACnet Stack defines - first */
#include "bacnet/bacdef.h"
/* BACnet Stack API */
#include "bacnet/bacdcode.h"
#include "bacnet/bacerror.h"
#include "bacnet/bacstr.h"
#include "bacnet/bactext.h"
#include "bacnet/apdu.h"
#include "bacnet/npdu.h"
#include "bacnet/abort.h"
#include "bacnet/get_alarm_sum.h"
/* basic objects, services, TSM, and datalink */
#include "bacnet/basic/object/device.h"
#include "bacnet/basic/tsm/tsm.h"
#include "bacnet/basic/services.h"

static get_alarm_summary_ack_callback_t Get_Alarm_Summary_Ack_Callback;

/**
 * @brief Set the callback function for each decoded GetAlarmSummary ACK entry
 * @param cb [in] The callback function, or NULL to disable
 */
void get_alarm_summary_ack_callback_set(get_alarm_summary_ack_callback_t cb)
{
    Get_Alarm_Summary_Ack_Callback = cb;
}

/**
 * @brief Print one GetAlarmSummary ACK entry to stdout
 * @param data [in] The decoded GetAlarmSummary ACK entry
 */
void get_alarm_summary_ack_print_data(const BACNET_GET_ALARM_SUMMARY_DATA *data)
{
    uint8_t i;
    uint8_t bits_used;

    if (!data) {
        return;
    }
    printf(
        "%s %lu %s {", bactext_object_type_name(data->objectIdentifier.type),
        (unsigned long)data->objectIdentifier.instance,
        bactext_event_state_name(data->alarmState));
    bits_used = bitstring_bits_used(&data->acknowledgedTransitions);
    for (i = 0; i < bits_used; i++) {
        printf(
            "%s%s", i ? "," : "",
            bitstring_bit(&data->acknowledgedTransitions, i) ? "true"
                                                             : "false");
    }
    printf("}\n");
}

/** Handle a GetAlarmSummary ACK and call the callback for each entry.
 *
 * @param service_request [in] The contents of the service request.
 * @param service_len [in] The length of the service_request.
 * @param src [in] BACNET_ADDRESS of the source of the message
 * @param service_data [in] The BACNET_CONFIRMED_SERVICE_ACK_DATA information
 * decoded from the APDU header of this message.
 */
void get_alarm_summary_ack_handler(
    uint8_t *service_request,
    uint16_t service_len,
    BACNET_ADDRESS *src,
    BACNET_CONFIRMED_SERVICE_ACK_DATA *service_data)
{
    uint16_t len = 0;
    uint8_t invoke_id = 0;
    BACNET_GET_ALARM_SUMMARY_DATA data = { 0 };

    if (service_data) {
        invoke_id = service_data->invoke_id;
    }
    while (len < service_len) {
        int apdu_len = get_alarm_summary_ack_decode_apdu_data(
            &service_request[len], service_len - len, &data);
        if ((apdu_len <= 0) || (apdu_len > (service_len - len))) {
            break;
        }
        len += (uint16_t)apdu_len;
        if (Get_Alarm_Summary_Ack_Callback) {
            Get_Alarm_Summary_Ack_Callback(src, invoke_id, &data);
        }
    }
}
