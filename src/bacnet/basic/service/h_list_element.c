/**
 * @file
 * @brief AddListElement and RemoveListElement service application handlers
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date December 2022
 * @section LICENSE
 *
 * Copyright (C) 2022 Steve Karg <skarg@users.sourceforge.net>
 *
 * SPDX-License-Identifier: MIT
 */
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include "bacnet/config.h"
#include "bacnet/bacdef.h"
#include "bacnet/bacdcode.h"
#include "bacnet/bacerror.h"
#include "bacnet/apdu.h"
#include "bacnet/npdu.h"
#include "bacnet/abort.h"
#include "bacnet/list_element.h"
/* basic objects, services, TSM, and datalink */
#include "bacnet/basic/object/device.h"
#include "bacnet/basic/tsm/tsm.h"
#include "bacnet/basic/services.h"
#include "bacnet/basic/sys/debug.h"
#include "bacnet/datalink/datalink.h"

/**
 * @brief Handler for a AddListElement Service request.
 * This handler will be invoked by apdu_handler() if it has been enabled
 * via call to apdu_set_confirmed_handler().
 * This handler builds a response packet, which is
 * - an Abort if
 *   - the message is segmented
 *   - if decoding fails
 * - a SimpleACK if Device_Add_List_Element() succeeds
 * - an Error if Device_Add_List_Element() fails
 *
 * @param service_request [in] The contents of the service request.
 * @param service_len [in] The length of the service_request.
 * @param src [in] BACNET_ADDRESS of the source of the message
 * @param service_data [in] The BACNET_CONFIRMED_SERVICE_DATA information
 *                          decoded from the APDU header of this message.
 */
void handler_add_list_element(uint8_t *service_request,
    uint16_t service_len,
    BACNET_ADDRESS *src,
    BACNET_CONFIRMED_SERVICE_DATA *service_data)
{
    BACNET_LIST_ELEMENT_DATA list_element = { 0 };
    BACNET_NPDU_DATA npdu_data;
    BACNET_ADDRESS my_address;
    int len = 0;
    bool status = true;
    int pdu_len = 0;
    int bytes_sent = 0;

    /* encode the NPDU portion of the packet */
    datalink_get_my_address(&my_address);
    npdu_encode_npdu_data(&npdu_data, false, MESSAGE_PRIORITY_NORMAL);
    pdu_len = npdu_encode_pdu(
        &Handler_Transmit_Buffer[0], src, &my_address, &npdu_data);
    debug_perror("AddListElement: Received Request!\n");
    if (service_data->segmented_message) {
        len = abort_encode_apdu(&Handler_Transmit_Buffer[pdu_len],
            service_data->invoke_id, ABORT_REASON_SEGMENTATION_NOT_SUPPORTED,
            true);
        debug_perror("AddListElement: Segmented message.  Sending Abort!\n");
        status = false;
    }
    if (status) {
        /* decode the service request only */
        len = list_element_decode_service_request(
            service_request, service_len, &list_element);
        if (len > 0) {
            debug_perror("AddListElement: type=%lu instance=%lu property=%lu "
                         "index=%ld\n",
                (unsigned long)list_element.object_type,
                (unsigned long)list_element.object_instance,
                (unsigned long)list_element.object_property,
                (long)list_element.array_index);
        } else {
            debug_perror("AddListElement: Unable to decode request!\n");
        }
        /* bad decoding or something we didn't understand - send an abort */
        if (len <= 0) {
            len = abort_encode_apdu(&Handler_Transmit_Buffer[pdu_len],
                service_data->invoke_id, ABORT_REASON_OTHER, true);
            debug_perror("AddListElement: Bad Encoding. Sending Abort!\n");
            status = false;
        }
        if (status) {
            if (Device_Add_List_Element(&list_element)) {
                len = encode_simple_ack(&Handler_Transmit_Buffer[pdu_len],
                    service_data->invoke_id,
                    SERVICE_CONFIRMED_ADD_LIST_ELEMENT);
                debug_perror("AddListElement: Sending Simple Ack!\n");
            } else {
                len = bacerror_encode_apdu(&Handler_Transmit_Buffer[pdu_len],
                    service_data->invoke_id, SERVICE_CONFIRMED_ADD_LIST_ELEMENT,
                    list_element.error_class, list_element.error_code);
                debug_perror("AddListElement: Sending Error!\n");
            }
        }
    }
    /* Send PDU */
    pdu_len += len;
    bytes_sent = datalink_send_pdu(
        src, &npdu_data, &Handler_Transmit_Buffer[0], pdu_len);
    if (bytes_sent <= 0) {
        debug_perror(
            "AddListElement: Failed to send PDU (%s)!\n", strerror(errno));
    }

    return;
}

/**
 * @brief Handler for a RemoveListElement Service request.
 * This handler will be invoked by apdu_handler() if it has been enabled
 * via call to apdu_set_confirmed_handler().
 * This handler builds a response packet, which is
 * - an Abort if
 *   - the message is segmented
 *   - if decoding fails
 * - a SimpleACK if Device_Remove_List_Element() succeeds
 * - an Error if Device_Remove_List_Element() fails
 *
 * @param service_request [in] The contents of the service request.
 * @param service_len [in] The length of the service_request.
 * @param src [in] BACNET_ADDRESS of the source of the message
 * @param service_data [in] The BACNET_CONFIRMED_SERVICE_DATA information
 *  decoded from the APDU header of this message.
 */
void handler_remove_list_element(uint8_t *service_request,
    uint16_t service_len,
    BACNET_ADDRESS *src,
    BACNET_CONFIRMED_SERVICE_DATA *service_data)
{
    BACNET_LIST_ELEMENT_DATA list_element = { 0 };
    BACNET_NPDU_DATA npdu_data;
    BACNET_ADDRESS my_address;
    int len = 0;
    bool status = true;
    int pdu_len = 0;
    int bytes_sent = 0;

    /* encode the NPDU portion of the packet */
    datalink_get_my_address(&my_address);
    npdu_encode_npdu_data(&npdu_data, false, MESSAGE_PRIORITY_NORMAL);
    pdu_len = npdu_encode_pdu(
        &Handler_Transmit_Buffer[0], src, &my_address, &npdu_data);
    debug_perror("RemoveListElement: Received Request!\n");
    if (service_data->segmented_message) {
        len = abort_encode_apdu(&Handler_Transmit_Buffer[pdu_len],
            service_data->invoke_id, ABORT_REASON_SEGMENTATION_NOT_SUPPORTED,
            true);
        debug_perror("RemoveListElement: Segmented message.  Sending Abort!\n");
        status = false;
    }
    if (status) {
        /* decode the service request only */
        len = list_element_decode_service_request(
            service_request, service_len, &list_element);
        if (len > 0) {
            debug_perror("RemoveListElement: type=%lu instance=%lu "
                "property=%lu index=%ld\n",
                (unsigned long)list_element.object_type,
                (unsigned long)list_element.object_instance,
                (unsigned long)list_element.object_property,
                (long)list_element.array_index);
        } else {
            debug_perror("RemoveListElement: Unable to decode request!\n");
        }
        /* bad decoding or something we didn't understand - send an abort */
        if (len <= 0) {
            len = abort_encode_apdu(&Handler_Transmit_Buffer[pdu_len],
                service_data->invoke_id, ABORT_REASON_OTHER, true);
            debug_perror("RemoveListElement: Bad Encoding. Sending Abort!\n");
            status = false;
        }
        if (status) {
            if (Device_Remove_List_Element(&list_element)) {
                len = encode_simple_ack(&Handler_Transmit_Buffer[pdu_len],
                    service_data->invoke_id,
                    SERVICE_CONFIRMED_REMOVE_LIST_ELEMENT);
                debug_perror("RemoveListElement: Sending Simple Ack!\n");
            } else {
                len = bacerror_encode_apdu(&Handler_Transmit_Buffer[pdu_len],
                    service_data->invoke_id,
                    SERVICE_CONFIRMED_REMOVE_LIST_ELEMENT,
                    list_element.error_class, list_element.error_code);
                debug_perror("RemoveListElement: Sending Error!\n");
            }
        }
    }
    /* Send PDU */
    pdu_len += len;
    bytes_sent = datalink_send_pdu(
        src, &npdu_data, &Handler_Transmit_Buffer[0], pdu_len);
    if (bytes_sent <= 0) {
        debug_perror(
            "RemoveListElement: Failed to send PDU (%s)!\n", strerror(errno));
    }

    return;
}
