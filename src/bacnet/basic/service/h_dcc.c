/**
 * @file
 * @brief Handles Device Communication Control request.
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date 2006
 * @copyright SPDX-License-Identifier: MIT
 */
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "bacnet/bacdef.h"
#include "bacnet/bacdcode.h"
#include "bacnet/bacerror.h"
#include "bacnet/apdu.h"
#include "bacnet/npdu.h"
#include "bacnet/abort.h"
#include "bacnet/reject.h"
#include "bacnet/dcc.h"
/* basic objects, services, TSM, and datalink */
#include "bacnet/basic/object/device.h"
#include "bacnet/basic/tsm/tsm.h"
#include "bacnet/basic/services.h"
#include "bacnet/basic/sys/debug.h"
#include "bacnet/datalink/datalink.h"

/* The byte length of a UTF-8 character can vary.
 * In UTF-8, the number of bytes used to represent a character can range from 1
 * to 4 bytes. Commonly used characters in the ASCII set are represented by 1
 * byte,xi while other Unicode characters may require 2, 3, or 4 bytes. Let's
 * add space for the null '\0' termination byte.
 * */
static char My_Password[20 * 4 + 1] = "filister";

/** Sets (non-volatile hold) the password to be used for DCC requests.
 * @param new_password [in] The new DCC password, of up to 31 characters.
 */
void handler_dcc_password_set(const char *new_password)
{
    size_t i = 0; /* loop counter */

    if (new_password) {
        for (i = 0; i < (sizeof(My_Password) - 1); i++) {
            My_Password[i] = new_password[i];
            My_Password[i + 1] = 0;
            if (new_password[i] == 0) {
                break;
            }
        }
    } else {
        for (i = 0; i < sizeof(My_Password); i++) {
            My_Password[i] = 0;
        }
    }
}

/** Gets (non-volatile hold) the password to be used for DCC requests.
 * @return DCC password
 */
char *handler_dcc_password(void)
{
    return My_Password;
}

/** Handler for a Device Communication Control (DCC) request.
 * @ingroup DMDCC
 * This handler will be invoked by apdu_handler() if it has been enabled
 * by a call to apdu_set_confirmed_handler().
 * This handler builds a response packet, which is
 * - an Abort if
 *   - the message is segmented
 *   - if decoding fails
 *   - if not a known DCC state
 * - an Error if the DCC password is incorrect
 * - else tries to send a simple ACK for the DCC on success,
 *   and sets the DCC state requested.
 *
 * @param service_request [in] The contents of the service request.
 * @param service_len [in] The length of the service_request.
 * @param src [in] BACNET_ADDRESS of the source of the message
 * @param service_data [in] The BACNET_CONFIRMED_SERVICE_DATA information
 *                          decoded from the APDU header of this message.
 */
void handler_device_communication_control(
    uint8_t *service_request,
    uint16_t service_len,
    BACNET_ADDRESS *src,
    BACNET_CONFIRMED_SERVICE_DATA *service_data)
{
    uint16_t timeDuration = 0;
    BACNET_COMMUNICATION_ENABLE_DISABLE state = COMMUNICATION_ENABLE;
    BACNET_CHARACTER_STRING password;
    int len = 0;
    int pdu_len = 0;
    BACNET_NPDU_DATA npdu_data;
    BACNET_ADDRESS my_address;

    /* encode the NPDU portion of the reply packet */
    datalink_get_my_address(&my_address);
    npdu_encode_npdu_data(&npdu_data, false, service_data->priority);
    pdu_len = npdu_encode_pdu(
        &Handler_Transmit_Buffer[0], src, &my_address, &npdu_data);
    debug_print("DeviceCommunicationControl!\n");
    if (service_len == 0) {
        len = reject_encode_apdu(
            &Handler_Transmit_Buffer[pdu_len], service_data->invoke_id,
            REJECT_REASON_MISSING_REQUIRED_PARAMETER);
        debug_print("DeviceCommunicationControl: "
                    "Missing Required Parameter. Sending Reject!\n");
        goto DCC_FAILURE;
    } else if (service_data->segmented_message) {
        len = abort_encode_apdu(
            &Handler_Transmit_Buffer[pdu_len], service_data->invoke_id,
            ABORT_REASON_SEGMENTATION_NOT_SUPPORTED, true);
        debug_print("DeviceCommunicationControl: "
                    "Sending Abort - segmented message.\n");
        goto DCC_FAILURE;
    }
    /* decode the service request only */
    len = dcc_decode_service_request(
        service_request, service_len, &timeDuration, &state, &password);
    if (len > 0) {
        debug_fprintf(
            stderr,
            "DeviceCommunicationControl: "
            "timeout=%u state=%u password=%s\n",
            (unsigned)timeDuration, (unsigned)state,
            characterstring_value(&password));
    }
    /* bad decoding or invalid service parameter
       send an abort or reject */
    if (len < 0) {
        if (len == BACNET_STATUS_ABORT) {
            len = abort_encode_apdu(
                &Handler_Transmit_Buffer[pdu_len], service_data->invoke_id,
                ABORT_REASON_OTHER, true);
            debug_print("DCC: Sending Abort!\n");
        } else if (len == BACNET_STATUS_REJECT) {
            len = reject_encode_apdu(
                &Handler_Transmit_Buffer[pdu_len], service_data->invoke_id,
                REJECT_REASON_PARAMETER_OUT_OF_RANGE);
            debug_print("DCC: Sending Reject!\n");
        }
        goto DCC_FAILURE;
    }
#if (BACNET_PROTOCOL_REVISION >= 20)
    if (state == COMMUNICATION_DISABLE) {
        /*  If the request is valid and the 'Enable/Disable'
            parameter is the deprecated value DISABLE, return the error
            SERVICES, SERVICE_REQUEST_DENIED */
        len = bacerror_encode_apdu(
            &Handler_Transmit_Buffer[pdu_len], service_data->invoke_id,
            SERVICE_CONFIRMED_DEVICE_COMMUNICATION_CONTROL,
            ERROR_CLASS_SERVICES, ERROR_CODE_SERVICE_REQUEST_DENIED);
        debug_print("DeviceCommunicationControl: "
                    "Sending Error - DISABLE has been deprecated.\n");
        goto DCC_FAILURE;
    }
#endif
    if (state >= MAX_BACNET_COMMUNICATION_ENABLE_DISABLE) {
        len = reject_encode_apdu(
            &Handler_Transmit_Buffer[pdu_len], service_data->invoke_id,
            REJECT_REASON_UNDEFINED_ENUMERATION);
        debug_print("DeviceCommunicationControl: "
                    "Sending Reject - undefined enumeration\n");
    } else {
#ifdef BAC_ROUTING
        /* Check to see if the current Device supports this service. */
        len = Routed_Device_Service_Approval(
            SERVICE_SUPPORTED_DEVICE_COMMUNICATION_CONTROL, (int)state,
            &Handler_Transmit_Buffer[pdu_len], service_data->invoke_id);
        if (len > 0) {
            goto DCC_FAILURE;
        }
#endif
        if ((My_Password[0] == '\0') ||
            characterstring_ansi_same(&password, My_Password)) {
            len = encode_simple_ack(
                &Handler_Transmit_Buffer[pdu_len], service_data->invoke_id,
                SERVICE_CONFIRMED_DEVICE_COMMUNICATION_CONTROL);
            debug_print("DeviceCommunicationControl: "
                        "Sending Simple Ack!\n");
            dcc_set_status_duration(state, timeDuration);
        } else {
            len = bacerror_encode_apdu(
                &Handler_Transmit_Buffer[pdu_len], service_data->invoke_id,
                SERVICE_CONFIRMED_DEVICE_COMMUNICATION_CONTROL,
                ERROR_CLASS_SECURITY, ERROR_CODE_PASSWORD_FAILURE);
            debug_print("DeviceCommunicationControl: "
                        "Sending Error - password failure.\n");
        }
    }
DCC_FAILURE:
    pdu_len += len;
    len = datalink_send_pdu(
        src, &npdu_data, &Handler_Transmit_Buffer[0], pdu_len);
    if (len <= 0) {
        debug_perror("DeviceCommunicationControl: Failed to send PDU");
    }

    return;
}
