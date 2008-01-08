/**************************************************************************
*
* Copyright (C) 2007 Steve Karg <skarg@users.sourceforge.net>
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
#include "config.h"
#include "txbuf.h"
#include "bacdef.h"
#include "bacdcode.h"
#include "bacaddr.h"
#include "apdu.h"
#include "npdu.h"
#include "abort.h"
#include "cov.h"
/* demo objects */
#include "device.h"
#include "ai.h"
#include "ao.h"
#include "av.h"
#include "bi.h"
#include "bo.h"
#include "bv.h"
#include "lc.h"
#include "lsp.h"
#include "mso.h"
#if defined(BACFILE)
#include "bacfile.h"
#endif

/* note: This COV service only monitors the properties
   of an object that have been specified in the standard.  */
typedef struct BACnet_COV_Subscription {
    bool valid;
    BACNET_ADDRESS dest;
    uint32_t subscriberProcessIdentifier;
    BACNET_OBJECT_ID monitoredObjectIdentifier;
    bool issueConfirmedNotifications;   /* optional */
    uint32_t lifetime;  /* optional */
} BACNET_COV_SUBSCRIPTION;

#define MAX_COV_SUBCRIPTIONS 32
static BACNET_COV_SUBSCRIPTION COV_Subscriptions[MAX_COV_SUBCRIPTIONS];

void handler_cov_init(
    void)
{
    unsigned index = 0;

    for (index = 0; index < MAX_COV_SUBCRIPTIONS; index++) {
        COV_Subscriptions[index].valid = false;
        COV_Subscriptions[index].dest.mac_len = 0;
        COV_Subscriptions[index].subscriberProcessIdentifier = 0;
        COV_Subscriptions[index].monitoredObjectIdentifier.type =
            OBJECT_ANALOG_INPUT;
        COV_Subscriptions[index].monitoredObjectIdentifier.instance = 0;
        COV_Subscriptions[index].issueConfirmedNotifications = false;
        COV_Subscriptions[index].lifetime = 0;
    }
}

static bool cov_list_subscribe(
    BACNET_ADDRESS * src,
    BACNET_SUBSCRIBE_COV_DATA * cov_data)
{
    bool existing_entry = false;
    int index;
    int first_invalid_index = -1;
    bool found = true;

    /* existing? - match Object ID and Process ID */
    for (index = 0; index < MAX_COV_SUBCRIPTIONS; index++) {
        if ((COV_Subscriptions[index].valid) &&
            (COV_Subscriptions[index].monitoredObjectIdentifier.type ==
                cov_data->monitoredObjectIdentifier.type) &&
            (COV_Subscriptions[index].monitoredObjectIdentifier.instance ==
                cov_data->monitoredObjectIdentifier.instance) &&
            (COV_Subscriptions[index].subscriberProcessIdentifier ==
                cov_data->subscriberProcessIdentifier)) {
            existing_entry = true;
            if (cov_data->cancellationRequest) {
                COV_Subscriptions[index].valid = false;
            } else {
                bacnet_address_copy(&COV_Subscriptions[index].dest, src);
                COV_Subscriptions[index].issueConfirmedNotifications =
                    cov_data->issueConfirmedNotifications;
                COV_Subscriptions[index].lifetime = cov_data->lifetime;
            }
            break;
        } else {
            if (first_invalid_index < 0) {
                first_invalid_index = index;
            }
        }
    }
    if (!existing_entry && (first_invalid_index >= 0) &&
        (!cov_data->cancellationRequest)) {
        index = first_invalid_index;
        found = true;
        COV_Subscriptions[index].valid = true;
        bacnet_address_copy(&COV_Subscriptions[index].dest, src);
        COV_Subscriptions[index].monitoredObjectIdentifier.type =
            cov_data->monitoredObjectIdentifier.type;
        COV_Subscriptions[index].monitoredObjectIdentifier.instance =
            cov_data->monitoredObjectIdentifier.instance;
        COV_Subscriptions[index].subscriberProcessIdentifier =
            cov_data->subscriberProcessIdentifier;
        COV_Subscriptions[index].issueConfirmedNotifications =
            cov_data->issueConfirmedNotifications;
        COV_Subscriptions[index].lifetime = cov_data->lifetime;
    } else {
        found = false;
    }

    return found;
}

/* note: worst case tasking: MS/TP with the ability to send only
   one notification per task cycle */
void handler_cov_task(
    uint32_t elapsed_milliseconds)
{
    int index;
    int lifetime_milliseconds;
    BACNET_OBJECT_ID object_id;


    /* existing? - match Object ID and Process ID */
    for (index = 0; index < MAX_COV_SUBCRIPTIONS; index++) {
        if (COV_Subscriptions[index].valid) {
            /* handle timeouts */
            lifetime_milliseconds = COV_Subscriptions[index].lifetime;
            if (lifetime_milliseconds >= elapsed_milliseconds) {
                COV_Subscriptions[index].lifetime -= elapsed_milliseconds;
            } else {
                COV_Subscriptions[index].lifetime = 0;
            }
            if (COV_Subscriptions[index].lifetime == 0) {
                COV_Subscriptions[index].valid = false;
            }
            /* handle COV notifications */
            object_id.type =
                COV_Subscriptions[index].monitoredObjectIdentifier.type;
            object_id.instance =
                COV_Subscriptions[index].monitoredObjectIdentifier.instance;
            switch (object_id.type) {
                case OBJECT_BINARY_INPUT:
                    if (Binary_Input_Change_Of_Value(object_id.instance)) {
                        /* FIXME: send confirmed or unconfirmed request */
                    }
                    break;
                default:
                    break;
            }
        }
    }
}

static bool cov_subscribe(
    BACNET_ADDRESS * src,
    BACNET_SUBSCRIBE_COV_DATA * cov_data,
    BACNET_ERROR_CLASS * error_class,
    BACNET_ERROR_CODE * error_code)
{
    bool status = false;        /* return value */

    switch (cov_data->monitoredObjectIdentifier.type) {
        case OBJECT_BINARY_INPUT:
            status = true;
            status = cov_list_subscribe(src, cov_data);
            break;
        default:
            /* FIXME: what is the ERROR? */
            break;
    }

    return status;
}

void handler_cov_subscribe(
    uint8_t * service_request,
    uint16_t service_len,
    BACNET_ADDRESS * src,
    BACNET_CONFIRMED_SERVICE_DATA * service_data)
{
    BACNET_SUBSCRIBE_COV_DATA cov_data;
    int len = 0;
    int pdu_len = 0;
    BACNET_NPDU_DATA npdu_data;
    bool success = false;
    int bytes_sent = 0;
    BACNET_ERROR_CLASS error_class = ERROR_CLASS_OBJECT;
    BACNET_ERROR_CODE error_code = ERROR_CODE_UNKNOWN_OBJECT;
    BACNET_ADDRESS my_address;

    /* encode the NPDU portion of the packet */
    datalink_get_my_address(&my_address);
    npdu_encode_npdu_data(&npdu_data, false, MESSAGE_PRIORITY_NORMAL);
    pdu_len =
        npdu_encode_pdu(&Handler_Transmit_Buffer[0], src, &my_address,
        &npdu_data);
    if (service_data->segmented_message) {
        /* we don't support segmentation - send an abort */
        len =
            abort_encode_apdu(&Handler_Transmit_Buffer[pdu_len],
            service_data->invoke_id, ABORT_REASON_SEGMENTATION_NOT_SUPPORTED,
            true);
#if PRINT_ENABLED
        fprintf(stderr, "SubscribeCOV: Segmented message.  Sending Abort!\n");
#endif
        goto COV_ABORT;
    }
    len =
        cov_subscribe_decode_service_request(service_request, service_len,
        &cov_data);
#if PRINT_ENABLED
    if (len <= 0)
        fprintf(stderr, "SubscribeCOV: Unable to decode Request!\n");
#endif
    if (len < 0) {
        /* bad decoding - send an abort */
        len =
            abort_encode_apdu(&Handler_Transmit_Buffer[pdu_len],
            service_data->invoke_id, ABORT_REASON_OTHER, true);
#if PRINT_ENABLED
        fprintf(stderr, "SubscribeCOV: Bad decoding.  Sending Abort!\n");
#endif
        goto COV_ABORT;
    }
    success = cov_subscribe(src, &cov_data, &error_class, &error_code);
    if (success) {
        len =
            encode_simple_ack(&Handler_Transmit_Buffer[pdu_len],
            service_data->invoke_id, SERVICE_CONFIRMED_SUBSCRIBE_COV);
#if PRINT_ENABLED
        fprintf(stderr, "SubscribeCOV: Sending Simple Ack!\n");
#endif
    } else {
        len =
            bacerror_encode_apdu(&Handler_Transmit_Buffer[pdu_len],
            service_data->invoke_id, SERVICE_CONFIRMED_SUBSCRIBE_COV,
            error_class, error_code);
#if PRINT_ENABLED
        fprintf(stderr, "SubscribeCOV: Sending Error!\n");
#endif
    }
  COV_ABORT:
    pdu_len += len;
    bytes_sent =
        datalink_send_pdu(src, &npdu_data, &Handler_Transmit_Buffer[0],
        pdu_len);
#if PRINT_ENABLED
    if (bytes_sent <= 0)
        fprintf(stderr, "SubscribeCOV: Failed to send PDU (%s)!\n",
            strerror(errno));
#endif

    return;
}
