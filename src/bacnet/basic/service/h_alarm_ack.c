/**
 * @file
 * @brief A basic AlarmAcknowledgment service Handler
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @author Krzysztof Malorny <malornykrzysztof@gmail.com>
 * @author John Minack <minack@users.sourceforge.net>
 * @date 2005, 2011
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
#include "bacnet/bactext.h"
#include "bacnet/apdu.h"
#include "bacnet/npdu.h"
#include "bacnet/abort.h"
#include "bacnet/alarm_ack.h"
#include "bacnet/reject.h"
/* basic objects, services, TSM, and datalink */
#include "bacnet/basic/object/device.h"
#include "bacnet/basic/tsm/tsm.h"
#include "bacnet/basic/sys/debug.h"
#include "bacnet/basic/services.h"
#include "bacnet/datalink/datalink.h"

/** @file h_alarm_ack.c  Handles Alarm Acknowledgment. */

static alarm_ack_function Alarm_Ack[MAX_BACNET_OBJECT_TYPE];

void handler_alarm_ack_set(
    BACNET_OBJECT_TYPE object_type, alarm_ack_function pFunction)
{
    if (object_type < MAX_BACNET_OBJECT_TYPE) {
        Alarm_Ack[object_type] = pFunction;
    }
}

/** Handler for an Alarm/Event Acknowledgement.
 * @ingroup ALMACK
 * This handler will be invoked by apdu_handler() if it has been enabled
 * by a call to apdu_set_confirmed_handler().
 * This handler builds a response packet, which is
 * - an Abort if
 *   - the message is segmented
 *   - if decoding fails
 * - Otherwise, sends a simple ACK
 *
 * @param service_request [in] The contents of the service request.
 * @param service_len [in] The length of the service_request.
 * @param src [in] BACNET_ADDRESS of the source of the message
 * @param service_data [in] The BACNET_CONFIRMED_SERVICE_DATA information
 *                          decoded from the APDU header of this message.
 */
void handler_alarm_ack(
    uint8_t *service_request,
    uint16_t service_len,
    BACNET_ADDRESS *src,
    BACNET_CONFIRMED_SERVICE_DATA *service_data)
{
    int len = 0;
    int pdu_len = 0;
    int bytes_sent = 0;
    int ack_result = 0;
    BACNET_ADDRESS my_address;
    BACNET_NPDU_DATA npdu_data;
    BACNET_ALARM_ACK_DATA data;
    BACNET_ERROR_CODE error_code;

    /* encode the NPDU portion of the packet */
    datalink_get_my_address(&my_address);
    npdu_encode_npdu_data(&npdu_data, false, service_data->priority);
    pdu_len = npdu_encode_pdu(
        &Handler_Transmit_Buffer[0], src, &my_address, &npdu_data);
    if (service_len == 0) {
        len = reject_encode_apdu(
            &Handler_Transmit_Buffer[pdu_len], service_data->invoke_id,
            REJECT_REASON_MISSING_REQUIRED_PARAMETER);
        debug_print("Alarm Ack: Missing Required Parameter. Sending Reject!\n");
        goto AA_ABORT;
    } else if (service_data->segmented_message) {
        /* we don't support segmentation - send an abort */
        len = abort_encode_apdu(
            &Handler_Transmit_Buffer[pdu_len], service_data->invoke_id,
            ABORT_REASON_SEGMENTATION_NOT_SUPPORTED, true);
        debug_print("Alarm Ack: Segmented message.  Sending Abort!\n");
        goto AA_ABORT;
    }

    len = alarm_ack_decode_service_request(service_request, service_len, &data);
    if (len <= 0) {
        debug_print("Alarm Ack: Unable to decode Request!\n");
    }
    if (len < 0) {
        /* bad decoding - send an abort */
        len = abort_encode_apdu(
            &Handler_Transmit_Buffer[pdu_len], service_data->invoke_id,
            ABORT_REASON_OTHER, true);
        debug_print("Alarm Ack: Bad Encoding.  Sending Abort!\n");
        goto AA_ABORT;
    }
    debug_fprintf(
        stderr,
        "Alarm Ack Operation: Received acknowledge for object id (%d, %lu) "
        "from %s for process id %lu \n",
        data.eventObjectIdentifier.type,
        (unsigned long)data.eventObjectIdentifier.instance,
        data.ackSource.value, (unsigned long)data.ackProcessIdentifier);
    if (!Device_Valid_Object_Id(
            data.eventObjectIdentifier.type,
            data.eventObjectIdentifier.instance)) {
        len = bacerror_encode_apdu(
            &Handler_Transmit_Buffer[pdu_len], service_data->invoke_id,
            SERVICE_CONFIRMED_ACKNOWLEDGE_ALARM, ERROR_CLASS_OBJECT,
            ERROR_CODE_UNKNOWN_OBJECT);
    } else if (Alarm_Ack[data.eventObjectIdentifier.type]) {
        ack_result =
            Alarm_Ack[data.eventObjectIdentifier.type](&data, &error_code);
        switch (ack_result) {
            case 1:
                len = encode_simple_ack(
                    &Handler_Transmit_Buffer[pdu_len], service_data->invoke_id,
                    SERVICE_CONFIRMED_ACKNOWLEDGE_ALARM);
                debug_print("Alarm Acknowledge: Sending Simple Ack!\n");
                break;

            case -1:
                len = bacerror_encode_apdu(
                    &Handler_Transmit_Buffer[pdu_len], service_data->invoke_id,
                    SERVICE_CONFIRMED_ACKNOWLEDGE_ALARM, ERROR_CLASS_OBJECT,
                    error_code);
                debug_fprintf(
                    stderr, "Alarm Acknowledge: error %s!\n",
                    bactext_error_code_name(error_code));
                break;

            default:
                len = abort_encode_apdu(
                    &Handler_Transmit_Buffer[pdu_len], service_data->invoke_id,
                    ABORT_REASON_OTHER, true);
                debug_print("Alarm Acknowledge: abort other!\n");
                break;
        }
    } else {
        len = bacerror_encode_apdu(
            &Handler_Transmit_Buffer[pdu_len], service_data->invoke_id,
            SERVICE_CONFIRMED_ACKNOWLEDGE_ALARM, ERROR_CLASS_OBJECT,
            ERROR_CODE_NO_ALARM_CONFIGURED);
        debug_print("Alarm Acknowledge: No Alarm Configured!\n");
    }

AA_ABORT:
    pdu_len += len;
    bytes_sent = datalink_send_pdu(
        src, &npdu_data, &Handler_Transmit_Buffer[0], pdu_len);
    if (bytes_sent <= 0) {
        debug_perror("Alarm Acknowledge: Failed to send PDU");
    }

    return;
}
