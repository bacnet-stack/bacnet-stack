/**
 * @file
 * @brief A basic GetAlarmSummary-Request service handler
 * @author Krzysztof Malorny <malornykrzysztof@gmail.com>
 * @date 2011
 * @copyright SPDX-License-Identifier: MIT
 */
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
/* BACnet Stack defines - first */
#include "bacnet/bacdef.h"
/* BACnet Stack API */
#include "bacnet/bacdcode.h"
#include "bacnet/bacerror.h"
#include "bacnet/apdu.h"
#include "bacnet/npdu.h"
#include "bacnet/abort.h"
/* basic services, TSM, and datalink */
#include "bacnet/basic/tsm/tsm.h"
#include "bacnet/basic/services.h"
#include "bacnet/datalink/datalink.h"

static get_alarm_summary_function Get_Alarm_Summary[MAX_BACNET_OBJECT_TYPE];

void handler_get_alarm_summary_set(
    BACNET_OBJECT_TYPE object_type, get_alarm_summary_function pFunction)
{
    if (object_type < MAX_BACNET_OBJECT_TYPE) {
        Get_Alarm_Summary[object_type] = pFunction;
    }
}

void handler_get_alarm_summary(
    uint8_t *service_request,
    uint16_t service_len,
    BACNET_ADDRESS *src,
    BACNET_CONFIRMED_SERVICE_DATA *service_data)
{
    int len = 0;
    int pdu_len = 0;
    int apdu_len = 0;
    int bytes_sent = 0;
    int alarm_value = 0;
    unsigned i = 0;
    unsigned j = 0;
    bool error = false;
    BACNET_ADDRESS my_address;
    BACNET_NPDU_DATA npdu_data;
    BACNET_GET_ALARM_SUMMARY_DATA getalarm_data;

    (void)service_request;
    (void)service_len;
    /* encode the NPDU portion of the packet */
    datalink_get_my_address(&my_address);
    npdu_encode_npdu_data(&npdu_data, false, MESSAGE_PRIORITY_NORMAL);
    pdu_len = npdu_encode_pdu(
        &Handler_Transmit_Buffer[0], src, &my_address, &npdu_data);
    if (service_data->segmented_message) {
        /* we don't support segmentation - send an abort */
        apdu_len = abort_encode_apdu(
            &Handler_Transmit_Buffer[pdu_len], service_data->invoke_id,
            ABORT_REASON_SEGMENTATION_NOT_SUPPORTED, true);
#if PRINT_ENABLED
        fprintf(stderr, "GetAlarmSummary: Segmented message. Sending Abort!\n");
#endif
        goto GET_ALARM_SUMMARY_ABORT;
    }

    /* init header */
    apdu_len = get_alarm_summary_ack_encode_apdu_init(
        &Handler_Transmit_Buffer[pdu_len], service_data->invoke_id);

    for (i = 0; i < MAX_BACNET_OBJECT_TYPE; i++) {
        if (Get_Alarm_Summary[i]) {
            for (j = 0; j < 0xffff; j++) {
                alarm_value = Get_Alarm_Summary[i](j, &getalarm_data);
                if (alarm_value > 0) {
                    len = get_alarm_summary_ack_encode_apdu_data(
                        &Handler_Transmit_Buffer[pdu_len + apdu_len],
                        service_data->max_resp - apdu_len, &getalarm_data);
                    if (len <= 0) {
                        error = true;
                        goto GET_ALARM_SUMMARY_ERROR;
                    } else {
                        apdu_len += len;
                    }
                } else if (alarm_value < 0) {
                    break;
                }
            }
        }
    }

#if PRINT_ENABLED
    fprintf(stderr, "GetAlarmSummary: Sending response!\n");
#endif

GET_ALARM_SUMMARY_ERROR:
    if (error) {
        if (len == BACNET_STATUS_ABORT) {
            /* BACnet APDU too small to fit data, so proper response is Abort */
            apdu_len = abort_encode_apdu(
                &Handler_Transmit_Buffer[pdu_len], service_data->invoke_id,
                ABORT_REASON_SEGMENTATION_NOT_SUPPORTED, true);
#if PRINT_ENABLED
            fprintf(
                stderr, "GetAlarmSummary: Reply too big to fit into APDU!\n");
#endif
        } else {
            apdu_len = bacerror_encode_apdu(
                &Handler_Transmit_Buffer[pdu_len], service_data->invoke_id,
                SERVICE_CONFIRMED_GET_ALARM_SUMMARY, ERROR_CLASS_PROPERTY,
                ERROR_CODE_OTHER);
#if PRINT_ENABLED
            fprintf(stderr, "GetAlarmSummary: Sending Error!\n");
#endif
        }
    }

GET_ALARM_SUMMARY_ABORT:
    pdu_len += apdu_len;
    bytes_sent = datalink_send_pdu(
        src, &npdu_data, &Handler_Transmit_Buffer[0], pdu_len);
#if PRINT_ENABLED
    if (bytes_sent <= 0) {
        /*fprintf(stderr, "Failed to send PDU (%s)!\n", strerror(errno)); */
    }
#else
    (void)bytes_sent;
#endif

    return;
}
