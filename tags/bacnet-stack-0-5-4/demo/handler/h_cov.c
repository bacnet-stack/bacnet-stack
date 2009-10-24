/**************************************************************************
*
* Copyright (C) 2007-2008 Steve Karg <skarg@users.sourceforge.net>
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
#include "tsm.h"
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
    bool send_requested;
} BACNET_COV_SUBSCRIPTION;

#define MAX_COV_SUBCRIPTIONS 32
static BACNET_COV_SUBSCRIPTION COV_Subscriptions[MAX_COV_SUBCRIPTIONS];

/*
BACnetCOVSubscription ::= SEQUENCE {
Recipient [0] BACnetRecipientProcess,
    BACnetRecipient ::= CHOICE {
    device [0] BACnetObjectIdentifier,
    address [1] BACnetAddress
        BACnetAddress ::= SEQUENCE {
        network-number Unsigned16, -- A value of 0 indicates the local network
        mac-address OCTET STRING -- A string of length 0 indicates a broadcast
        }
    }
    BACnetRecipientProcess ::= SEQUENCE {
    recipient [0] BACnetRecipient,
    processIdentifier [1] Unsigned32
    }
MonitoredPropertyReference [1] BACnetObjectPropertyReference,
    BACnetObjectPropertyReference ::= SEQUENCE {
    objectIdentifier [0] BACnetObjectIdentifier,
    propertyIdentifier [1] BACnetPropertyIdentifier,
    propertyArrayIndex [2] Unsigned OPTIONAL -- used only with array datatype
    -- if omitted with an array the entire array is referenced
    }
IssueConfirmedNotifications [2] BOOLEAN,
TimeRemaining [3] Unsigned,
COVIncrement [4] REAL OPTIONAL
*/

static int cov_encode_subscription(
    uint8_t * apdu,
    int max_apdu,
    BACNET_COV_SUBSCRIPTION * cov_subscription)
{
    int len = 0;
    int apdu_len = 0;
    BACNET_OCTET_STRING octet_string;

    /* FIXME: unused parameter */
    max_apdu = max_apdu;
    /* Recipient [0] BACnetRecipientProcess - opening */
    len = encode_opening_tag(&apdu[apdu_len], 0);
    apdu_len += len;
    /*  recipient [0] BACnetRecipient - opening */
    len = encode_opening_tag(&apdu[apdu_len], 0);
    apdu_len += len;
    /* CHOICE - address [1] BACnetAddress - opening */
    len = encode_opening_tag(&apdu[apdu_len], 1);
    apdu_len += len;
    /* network-number Unsigned16, */
    /* -- A value of 0 indicates the local network */
    len =
        encode_application_unsigned(&apdu[apdu_len],
        cov_subscription->dest.net);
    apdu_len += len;
    /* mac-address OCTET STRING */
    /* -- A string of length 0 indicates a broadcast */
    if (cov_subscription->dest.net) {
        octetstring_init(&octet_string, &cov_subscription->dest.adr[0],
            cov_subscription->dest.len);
    } else {
        octetstring_init(&octet_string, &cov_subscription->dest.mac[0],
            cov_subscription->dest.mac_len);
    }
    len = encode_application_octet_string(&apdu[apdu_len], &octet_string);
    apdu_len += len;
    /* CHOICE - address [1] BACnetAddress - closing */
    len = encode_closing_tag(&apdu[apdu_len], 1);
    apdu_len += len;
    /*  recipient [0] BACnetRecipient - closing */
    len = encode_closing_tag(&apdu[apdu_len], 0);
    apdu_len += len;
    /* processIdentifier [1] Unsigned32 */
    len =
        encode_context_unsigned(&apdu[apdu_len], 1,
        cov_subscription->subscriberProcessIdentifier);
    apdu_len += len;
    /* Recipient [0] BACnetRecipientProcess - closing */
    len = encode_closing_tag(&apdu[apdu_len], 0);
    apdu_len += len;
    /*  MonitoredPropertyReference [1] BACnetObjectPropertyReference, */
    len = encode_opening_tag(&apdu[apdu_len], 1);
    apdu_len += len;
    /* objectIdentifier [0] */
    len =
        encode_context_object_id(&apdu[apdu_len], 0,
        cov_subscription->monitoredObjectIdentifier.type,
        cov_subscription->monitoredObjectIdentifier.instance);
    apdu_len += len;
    /* propertyIdentifier [1] */
    /* FIXME: we are monitoring 2 properties! How to encode? */
    len = encode_context_enumerated(&apdu[apdu_len], 1, PROP_PRESENT_VALUE);
    apdu_len += len;
    /* MonitoredPropertyReference [1] - closing */
    len = encode_closing_tag(&apdu[apdu_len], 1);
    apdu_len += len;
    /* IssueConfirmedNotifications [2] BOOLEAN, */
    len =
        encode_context_boolean(&apdu[apdu_len], 2,
        cov_subscription->issueConfirmedNotifications);
    apdu_len += len;
    /* TimeRemaining [3] Unsigned, */
    len =
        encode_context_unsigned(&apdu[apdu_len], 3,
        cov_subscription->lifetime);
    apdu_len += len;

    return apdu_len;
}

int handler_cov_encode_subscriptions(
    uint8_t * apdu,
    int max_apdu)
{
    int len = 0;
    int apdu_len = 0;
    unsigned index = 0;

    if (apdu) {
        for (index = 0; index < MAX_COV_SUBCRIPTIONS; index++) {
            if (COV_Subscriptions[index].valid) {
                len =
                    cov_encode_subscription(&apdu[apdu_len],
                    max_apdu - apdu_len, &COV_Subscriptions[index]);
                apdu_len += len;
                if (apdu_len > max_apdu) {
                    return -2;
                }
            }
        }
    }

    return apdu_len;
}

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
        COV_Subscriptions[index].send_requested = false;
    }
}

static bool cov_list_subscribe(
    BACNET_ADDRESS * src,
    BACNET_SUBSCRIBE_COV_DATA * cov_data,
    BACNET_ERROR_CLASS * error_class,
    BACNET_ERROR_CODE * error_code)
{
    bool existing_entry = false;
    int index;
    int first_invalid_index = -1;
    bool found = true;

    /* unable to subscribe - resources? */
    /* unable to cancel subscription - other? */

    /* existing? - match Object ID and Process ID */
    for (index = 0; index < MAX_COV_SUBCRIPTIONS; index++) {
        if (COV_Subscriptions[index].valid) {
            if ((COV_Subscriptions[index].monitoredObjectIdentifier.type ==
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
                    COV_Subscriptions[index].send_requested = true;
                }
                break;
            }
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
        COV_Subscriptions[index].send_requested = true;
    } else if (!existing_entry) {
        if (first_invalid_index < 0) {
            /* Out of resources */
            *error_class = ERROR_CLASS_RESOURCES;
            *error_code = ERROR_CODE_OTHER;
        } else {
            /* Unable to cancel request - valid object not subscribed */
            *error_class = ERROR_CLASS_OBJECT;
            *error_code = ERROR_CODE_OTHER;
        }
        found = false;
    }

    return found;
}

static bool cov_send_request(
    BACNET_COV_SUBSCRIPTION * cov_subscription)
{
    int len = 0;
    int pdu_len = 0;
    BACNET_NPDU_DATA npdu_data;
    BACNET_ADDRESS my_address;
    int bytes_sent = 0;
    uint8_t invoke_id = 0;
    bool status = false;        /* return value */
    BACNET_COV_DATA cov_data;
    BACNET_PROPERTY_VALUE value_list[2];

#if PRINT_ENABLED
    fprintf(stderr, "COVnotification: requested\n");
#endif
    datalink_get_my_address(&my_address);
    npdu_encode_npdu_data(&npdu_data, false, MESSAGE_PRIORITY_NORMAL);
    pdu_len =
        npdu_encode_pdu(&Handler_Transmit_Buffer[0], &cov_subscription->dest,
        &my_address, &npdu_data);
    /* load the COV data structure for outgoing message */
    cov_data.subscriberProcessIdentifier =
        cov_subscription->subscriberProcessIdentifier;
    cov_data.initiatingDeviceIdentifier = Device_Object_Instance_Number();
    cov_data.monitoredObjectIdentifier.type =
        cov_subscription->monitoredObjectIdentifier.type;
    cov_data.monitoredObjectIdentifier.instance =
        cov_subscription->monitoredObjectIdentifier.instance;
    cov_data.timeRemaining = cov_subscription->lifetime;
    /* encode the value list */
    cov_data.listOfValues = &value_list[0];
    value_list[0].next = &value_list[1];
    value_list[1].next = NULL;
    switch (cov_subscription->monitoredObjectIdentifier.type) {
        case OBJECT_BINARY_INPUT:
            Binary_Input_Encode_Value_List
                (cov_subscription->monitoredObjectIdentifier.instance,
                &value_list[0]);
            break;
        default:
            goto COV_FAILED;
    }
    if (cov_subscription->issueConfirmedNotifications) {
        invoke_id = tsm_next_free_invokeID();
        if (invoke_id) {
            len =
                ccov_notify_encode_apdu(&Handler_Transmit_Buffer[pdu_len],
                invoke_id, &cov_data);
        } else {
            goto COV_FAILED;
        }
    } else {
        len =
            ucov_notify_encode_apdu(&Handler_Transmit_Buffer[pdu_len],
            &cov_data);
    }
    pdu_len += len;
    if (cov_subscription->issueConfirmedNotifications) {
        tsm_set_confirmed_unsegmented_transaction(invoke_id,
            &cov_subscription->dest, &npdu_data, &Handler_Transmit_Buffer[0],
            (uint16_t) pdu_len);
    }
    bytes_sent =
        datalink_send_pdu(&cov_subscription->dest, &npdu_data,
        &Handler_Transmit_Buffer[0], pdu_len);
    if (bytes_sent > 0) {
        status = true;
    }

  COV_FAILED:

    return status;
}

/* note: worst case tasking: MS/TP with the ability to send only
   one notification per task cycle */
void handler_cov_task(
    uint32_t elapsed_seconds)
{
    int index;
    uint32_t lifetime_seconds;
    BACNET_OBJECT_ID object_id;
    bool status = false;


    /* existing? - match Object ID and Process ID */
    for (index = 0; index < MAX_COV_SUBCRIPTIONS; index++) {
        if (COV_Subscriptions[index].valid) {
            /* handle timeouts */
            lifetime_seconds = COV_Subscriptions[index].lifetime;
            if (lifetime_seconds >= elapsed_seconds) {
                COV_Subscriptions[index].lifetime -= elapsed_seconds;
#if 0
                fprintf(stderr, "COVtask: subscription[%d].lifetime=%d\n",
                    index, COV_Subscriptions[index].lifetime);
#endif
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
                        COV_Subscriptions[index].send_requested = true;
                        Binary_Input_Change_Of_Value_Clear(object_id.instance);
                    }
                    break;
                default:
                    break;
            }
            if (COV_Subscriptions[index].send_requested) {
                status = cov_send_request(&COV_Subscriptions[index]);
                COV_Subscriptions[index].send_requested = false;
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
            if (Binary_Input_Valid_Instance
                (cov_data->monitoredObjectIdentifier.instance)) {
                status =
                    cov_list_subscribe(src, cov_data, error_class, error_code);
            } else {
                *error_class = ERROR_CLASS_OBJECT;
                *error_code = ERROR_CODE_UNKNOWN_OBJECT;
            }
            break;
        default:
            *error_class = ERROR_CLASS_OBJECT;
            *error_code = ERROR_CODE_UNKNOWN_OBJECT;
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
