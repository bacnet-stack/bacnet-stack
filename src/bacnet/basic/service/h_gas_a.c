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

struct get_alarm_summary_ack_context {
    BACNET_ADDRESS *src;
    uint8_t invoke_id;
};

/**
 * @brief Iterator callback to report one entry to the registered callback
 * @param context [in] the struct get_alarm_summary_ack_context
 * @param data [in] the decoded entry
 */
static void get_alarm_summary_ack_entry(
    void *context, const BACNET_GET_ALARM_SUMMARY_DATA *data)
{
    struct get_alarm_summary_ack_context *ctx = context;

    Get_Alarm_Summary_Ack_Callback(
        ctx->src, ctx->invoke_id, data, ERROR_CODE_SUCCESS);
}

/** Handle a GetAlarmSummary ACK and call the callback for each entry.
 *
 * The whole ACK is validated first. A malformed or truncated ACK
 * results in one callback with NULL data and an error code,
 * and no entries are reported. An empty ACK results in one callback
 * with NULL data and ERROR_CODE_SUCCESS.
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
    int len = 0;
    struct get_alarm_summary_ack_context context = { 0 };

    context.src = src;
    if (service_data) {
        context.invoke_id = service_data->invoke_id;
    }
    if (!Get_Alarm_Summary_Ack_Callback) {
        return;
    }
    len = get_alarm_summary_ack_decode_apdu_iterate(
        service_request, service_len, get_alarm_summary_ack_entry, &context);
    if (len < 0) {
        Get_Alarm_Summary_Ack_Callback(
            src, context.invoke_id, NULL, ERROR_CODE_INVALID_TAG);
    } else if (len == 0) {
        Get_Alarm_Summary_Ack_Callback(
            src, context.invoke_id, NULL, ERROR_CODE_SUCCESS);
    }
}
