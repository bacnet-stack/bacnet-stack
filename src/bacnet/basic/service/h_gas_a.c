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
#include <assert.h>
/* BACnet Stack defines - first */
#include "bacnet/bacdef.h"
/* BACnet Stack API */
#include "bacnet/bacdcode.h"
#include "bacnet/bacerror.h"
#include "bacnet/apdu.h"
#include "bacnet/npdu.h"
#include "bacnet/abort.h"
#include "bacnet/get_alarm_sum.h"
/* basic objects, services, TSM, and datalink */
#include "bacnet/basic/object/device.h"
#include "bacnet/basic/tsm/tsm.h"
#include "bacnet/basic/services.h"

/** Example function to handle a GetAlarmSummary ACK.
 *
 * @param service_request [in] The contents of the service request.
 * @param service_len [in] The length of the service_request.
 * @param src [in] BACNET_ADDRESS of the source of the message
 * @param service_data [in] The BACNET_CONFIRMED_SERVICE_DATA information
 * decoded from the APDU header of this message.
 */
void get_alarm_summary_ack_handler(
    uint8_t *service_request,
    uint16_t service_len,
    BACNET_ADDRESS *src,
    BACNET_CONFIRMED_SERVICE_ACK_DATA *service_data)
{
    uint16_t apdu_len = 0;
    uint16_t len = 0;
    BACNET_GET_ALARM_SUMMARY_DATA data;

    (void)src;
    (void)service_data;
    while (service_len - len) {
        apdu_len = get_alarm_summary_ack_decode_apdu_data(
            &service_request[len], service_len - len, &data);

        len += apdu_len;

        if (apdu_len > 0) {
            /* FIXME: Add code to process data */
        } else {
            break;
        }
    }
}
