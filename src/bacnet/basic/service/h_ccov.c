/**************************************************************************
 *
 * Copyright (C) 2011 Steve Karg <skarg@users.sourceforge.net>
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 *********************************************************************/
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include "bacnet/config.h"
#include "bacnet/bacdef.h"
#include "bacnet/bacdcode.h"
#include "bacnet/apdu.h"
#include "bacnet/npdu.h"
#include "bacnet/abort.h"
#include "bacnet/cov.h"
#include "bacnet/bactext.h"
/* basic services, TSM, and datalink */
#include "bacnet/basic/services.h"
#include "bacnet/basic/tsm/tsm.h"
#include "bacnet/datalink/datalink.h"

/** @file h_ccov.c  Handles Confirmed COV Notifications. */
#if PRINT_ENABLED
#include <stdio.h>
#define PRINTF(...) fprintf(stderr, __VA_ARGS__)
#else
#define PRINTF(...)
#endif

/* max number of COV properties decoded in a COV notification */
#ifndef MAX_COV_PROPERTIES
#define MAX_COV_PROPERTIES 2
#endif

/* COV notification callbacks list */
static BACNET_COV_NOTIFICATION Confirmed_COV_Notification_Head;

/**
 * @brief call the COV notification callbacks
 * @param cov_data - data decoded from the COV notification
 */
static void handler_ccov_notification_callback(BACNET_COV_DATA *cov_data)
{
    BACNET_COV_NOTIFICATION *head;

    head = &Confirmed_COV_Notification_Head;
    do {
        if (head->callback) {
            head->callback(cov_data);
        }
        head = head->next;
    } while (head);
}

/**
 * @brief Add a Confirmed COV notification callback
 * @param cb - COV notification callback to be added
 */
void handler_ccov_notification_add(BACNET_COV_NOTIFICATION *cb)
{
    BACNET_COV_NOTIFICATION *head;

    head = &Confirmed_COV_Notification_Head;
    do {
        if (head->next == cb) {
            /* already here! */
            break;
        } else if (!head->next) {
            /* first available free node */
            head->next = cb;
            break;
        }
        head = head->next;
    } while (head);
}

/*  */
/** Handler for an Confirmed COV Notification.
 * @ingroup DSCOV
 * Decodes the received list of Properties to update,
 * and print them out with the subscription information.
 * @note Nothing is specified in BACnet about what to do with the
 *       information received from Confirmed COV Notifications.
 *
 * @param service_request [in] The contents of the service request.
 * @param service_len [in] The length of the service_request.
 * @param src [in] BACNET_ADDRESS of the source of the message
 * @param service_data [in] The BACNET_CONFIRMED_SERVICE_DATA information
 *                          decoded from the APDU header of this message.
 */
void handler_ccov_notification(uint8_t *service_request,
    uint16_t service_len,
    BACNET_ADDRESS *src,
    BACNET_CONFIRMED_SERVICE_DATA *service_data)
{
    BACNET_NPDU_DATA npdu_data;
    BACNET_COV_DATA cov_data;
    BACNET_PROPERTY_VALUE property_value[MAX_COV_PROPERTIES];
    BACNET_PROPERTY_VALUE *pProperty_value = NULL;
    int len = 0;
    int pdu_len = 0;
    int bytes_sent = 0;
    BACNET_ADDRESS my_address;

    /* create linked list to store data if more
       than one property value is expected */
    bacapp_property_value_list_init(&property_value[0], MAX_COV_PROPERTIES);
    cov_data.listOfValues = &property_value[0];
    /* encode the NPDU portion of the packet */
    datalink_get_my_address(&my_address);
    npdu_encode_npdu_data(&npdu_data, false, MESSAGE_PRIORITY_NORMAL);
    pdu_len = npdu_encode_pdu(
        &Handler_Transmit_Buffer[0], src, &my_address, &npdu_data);
    PRINTF("CCOV: Received Notification!\n");
    if (service_data->segmented_message) {
        len = abort_encode_apdu(&Handler_Transmit_Buffer[pdu_len],
            service_data->invoke_id, ABORT_REASON_SEGMENTATION_NOT_SUPPORTED,
            true);
        PRINTF("CCOV: Segmented message.  Sending Abort!\n");
        goto CCOV_ABORT;
    }
    /* decode the service request only */
    len = cov_notify_decode_service_request(
        service_request, service_len, &cov_data);
    if (len > 0) {
        handler_ccov_notification_callback(&cov_data);
        PRINTF("CCOV: PID=%u ", cov_data.subscriberProcessIdentifier);
        PRINTF("instance=%u ", cov_data.initiatingDeviceIdentifier);
        PRINTF("%s %u ",
            bactext_object_type_name(cov_data.monitoredObjectIdentifier.type),
            cov_data.monitoredObjectIdentifier.instance);
        PRINTF("time remaining=%u seconds ", cov_data.timeRemaining);
        PRINTF("\n");
        pProperty_value = &property_value[0];
        while (pProperty_value) {
            PRINTF("CCOV: ");
            if (pProperty_value->propertyIdentifier < 512) {
                PRINTF("%s ",
                    bactext_property_name(pProperty_value->propertyIdentifier));
            } else {
                PRINTF("proprietary %u ", pProperty_value->propertyIdentifier);
            }
            if (pProperty_value->propertyArrayIndex != BACNET_ARRAY_ALL) {
                PRINTF("%u ", pProperty_value->propertyArrayIndex);
            }
            PRINTF("\n");
            pProperty_value = pProperty_value->next;
        }
    }
    /* bad decoding or something we didn't understand - send an abort */
    if (len <= 0) {
        len = abort_encode_apdu(&Handler_Transmit_Buffer[pdu_len],
            service_data->invoke_id, ABORT_REASON_OTHER, true);
        PRINTF("CCOV: Bad Encoding. Sending Abort!\n");
        goto CCOV_ABORT;
    } else {
        len = encode_simple_ack(&Handler_Transmit_Buffer[pdu_len],
            service_data->invoke_id, SERVICE_CONFIRMED_COV_NOTIFICATION);
        PRINTF("CCOV: Sending Simple Ack!\n");
    }
CCOV_ABORT:
    pdu_len += len;
    bytes_sent = datalink_send_pdu(
        src, &npdu_data, &Handler_Transmit_Buffer[0], pdu_len);
    if (bytes_sent <= 0) {
        PRINTF("CCOV: Failed to send PDU (%s)!\n", strerror(errno));
    }
    (void)bytes_sent;

    return;
}
