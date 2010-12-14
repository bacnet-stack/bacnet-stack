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
#include "session.h"
#include "handlers-data-core.h"
#include "bacnet-session.h"
#include "handlers.h"
#include "cov-core.h"

#if defined(BACFILE)
#include "bacfile.h"
#endif

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
    BACNET_MY_COV_SUBSCRIPTION * cov_subscription)
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

/** Handle a request to list all the COV subscriptions.
 * @ingroup DSCOV
 *  Invoked by a request to read the Device object's PROP_ACTIVE_COV_SUBSCRIPTIONS.
 *  Loops through the list of COV Subscriptions, and, for each valid one,
 *  adds its description to the APDU.
 *  @note This function needs some work to better handle buffer overruns.
 *  @param apdu [out] Buffer in which the APDU contents are built.
 *  @param max_apdu [in] Max length of the APDU buffer.
 *  @return How many bytes were encoded in the buffer, or -2 if the response
 *          would not fit within the buffer. 
 */
int handler_cov_encode_subscriptions(
    struct bacnet_session_object *sess,
    uint8_t * apdu,
    int max_apdu)
{
    int len = 0;
    int apdu_len = 0;
    unsigned index = 0;

    if (apdu) {
        for (index = 0; index < MAX_COV_SUBCRIPTIONS; index++) {
            if (get_bacnet_session_handler_data(sess)->COV_Subscriptions
                [index].valid) {
                len =
                    cov_encode_subscription(&apdu[apdu_len],
                    max_apdu - apdu_len,
                    &get_bacnet_session_handler_data(sess)->COV_Subscriptions
                    [index]);
                apdu_len += len;
                /* TODO: too late here to notice that we overran the buffer */
                if (apdu_len > max_apdu) {
                    return -2;
                }
            }
        }
    }

    return apdu_len;
}

/** Handler to initialize the COV list, clearing and disabling each entry. 
 * @ingroup DSCOV
 */
void handler_cov_init(
    struct bacnet_session_object *sess)
{
    unsigned index = 0;

    for (index = 0; index < MAX_COV_SUBCRIPTIONS; index++) {
        get_bacnet_session_handler_data(sess)->COV_Subscriptions[index].valid =
            false;
        get_bacnet_session_handler_data(sess)->COV_Subscriptions[index].
            dest.mac_len = 0;
        get_bacnet_session_handler_data(sess)->
            COV_Subscriptions[index].subscriberProcessIdentifier = 0;
        get_bacnet_session_handler_data(sess)->
            COV_Subscriptions[index].monitoredObjectIdentifier.type =
            OBJECT_ANALOG_INPUT;
        get_bacnet_session_handler_data(sess)->
            COV_Subscriptions[index].monitoredObjectIdentifier.instance = 0;
        get_bacnet_session_handler_data(sess)->
            COV_Subscriptions[index].issueConfirmedNotifications = false;
        get_bacnet_session_handler_data(sess)->
            COV_Subscriptions[index].lifetime = 0;
        get_bacnet_session_handler_data(sess)->
            COV_Subscriptions[index].send_requested = false;
    }
}

static bool cov_list_subscribe(
    struct bacnet_session_object *sess,
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
        if (get_bacnet_session_handler_data(sess)->
            COV_Subscriptions[index].valid) {
            if ((get_bacnet_session_handler_data(sess)->COV_Subscriptions
                    [index].monitoredObjectIdentifier.type ==
                    cov_data->monitoredObjectIdentifier.type) &&
                (get_bacnet_session_handler_data(sess)->COV_Subscriptions
                    [index].monitoredObjectIdentifier.instance ==
                    cov_data->monitoredObjectIdentifier.instance)
                &&
                (get_bacnet_session_handler_data(sess)->COV_Subscriptions
                    [index].subscriberProcessIdentifier ==
                    cov_data->subscriberProcessIdentifier)) {
                existing_entry = true;
                if (cov_data->cancellationRequest) {
                    get_bacnet_session_handler_data(sess)->COV_Subscriptions
                        [index].valid = false;
                } else {
                    bacnet_address_copy(&get_bacnet_session_handler_data
                        (sess)->COV_Subscriptions[index].dest, src);
                    get_bacnet_session_handler_data(sess)->COV_Subscriptions
                        [index].issueConfirmedNotifications =
                        cov_data->issueConfirmedNotifications;
                    get_bacnet_session_handler_data(sess)->COV_Subscriptions
                        [index].lifetime = cov_data->lifetime;
                    get_bacnet_session_handler_data(sess)->COV_Subscriptions
                        [index].send_requested = true;
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
        get_bacnet_session_handler_data(sess)->COV_Subscriptions[index].valid =
            true;
        bacnet_address_copy(&get_bacnet_session_handler_data
            (sess)->COV_Subscriptions[index].dest, src);
        get_bacnet_session_handler_data(sess)->
            COV_Subscriptions[index].monitoredObjectIdentifier.type =
            cov_data->monitoredObjectIdentifier.type;
        get_bacnet_session_handler_data(sess)->
            COV_Subscriptions[index].monitoredObjectIdentifier.instance =
            cov_data->monitoredObjectIdentifier.instance;
        get_bacnet_session_handler_data(sess)->
            COV_Subscriptions[index].subscriberProcessIdentifier =
            cov_data->subscriberProcessIdentifier;
        get_bacnet_session_handler_data(sess)->
            COV_Subscriptions[index].issueConfirmedNotifications =
            cov_data->issueConfirmedNotifications;
        get_bacnet_session_handler_data(sess)->
            COV_Subscriptions[index].lifetime = cov_data->lifetime;
        get_bacnet_session_handler_data(sess)->
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
    struct bacnet_session_object *sess,
    BACNET_MY_COV_SUBSCRIPTION * cov_subscription)
{
    int len = 0;
    int pdu_len = 0;
    BACNET_NPDU_DATA npdu_data;
    BACNET_APDU_FIXED_HEADER apdu_fixed_header;
    BACNET_ADDRESS my_address;
    int bytes_sent = 0;
    uint8_t invoke_id = 0;
    bool status = false;        /* return value */
    BACNET_COV_DATA cov_data;
    BACNET_PROPERTY_VALUE value_list[2];
    uint8_t Handler_Transmit_Buffer[MAX_PDU_SEND] = { 0 };

#if PRINT_ENABLED
    fprintf(stderr, "COVnotification: requested\n");
#endif
    npdu_encode_npdu_data(&npdu_data, false, MESSAGE_PRIORITY_NORMAL);
    /* in confirmed mode, all these operations are automatically done at TSM level */
    if (!cov_subscription->issueConfirmedNotifications) {
        sess->datalink_get_my_address(sess, &my_address);
        pdu_len =
            npdu_encode_pdu(&Handler_Transmit_Buffer[0],
            &cov_subscription->dest, &my_address, &npdu_data);
    }
    /* load the COV data structure for outgoing message */
    cov_data.subscriberProcessIdentifier =
        cov_subscription->subscriberProcessIdentifier;
    cov_data.initiatingDeviceIdentifier = Device_Object_Instance_Number(sess);
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
            Binary_Input_Encode_Value_List(sess,
                cov_subscription->monitoredObjectIdentifier.instance,
                &value_list[0]);
            break;
        default:
            goto COV_FAILED;
    }
    if (cov_subscription->issueConfirmedNotifications) {
        invoke_id = tsm_next_free_invokeID(sess);
        if (invoke_id) {
            len =
                ccov_notify_encode_apdu(&Handler_Transmit_Buffer[pdu_len],
                MAX_PDU_SEND - pdu_len, invoke_id, &cov_data);
        } else {
            goto COV_FAILED;
        }
    } else {
        len =
            ucov_notify_encode_apdu(&Handler_Transmit_Buffer[pdu_len],
            MAX_PDU_SEND - pdu_len, &cov_data);
    }
    pdu_len += len;
    if (cov_subscription->issueConfirmedNotifications) {
        apdu_init_fixed_header(&apdu_fixed_header,
            PDU_TYPE_CONFIRMED_SERVICE_REQUEST, invoke_id,
            SERVICE_CONFIRMED_COV_NOTIFICATION, MAX_APDU);
        /* Send data to the peer device, respecting APDU sizes, destination size,
           and segmented or unsegmented data sending possibilities */
        bytes_sent =
            tsm_set_confirmed_transaction(sess, invoke_id,
            &cov_subscription->dest, &npdu_data, &apdu_fixed_header,
            &Handler_Transmit_Buffer[0], pdu_len);
    } else {
        bytes_sent =
            sess->datalink_send_pdu(sess, &cov_subscription->dest, &npdu_data,
            &Handler_Transmit_Buffer[0], pdu_len);
    }
    status = (bytes_sent > 0);

  COV_FAILED:

    return status;
}

/** Handler to check the list of subscribed objects for any that have changed 
 *  and so need to have notifications sent.
 * @ingroup DSCOV
 * This handler will be invoked by the main program every second or so.
 * This example only handles Binary Inputs, but can be easily extended to
 * support other types.
 * For each subscribed object,
 *  - See if the subscription has timed out
 *    - Remove it if it has timed out.
 *  - See if the subscribed object instance has changed
 *    (eg, check with Binary_Input_Change_Of_Value() )
 *  - If changed,
 *    - Clear the COV (eg, Binary_Input_Change_Of_Value_Clear() )
 *    - Send the notice with cov_send_request() 
 *      - Will be confirmed or unconfirmed, as per the subscription.  
 *      
 * @note worst case tasking: MS/TP with the ability to send only
 *        one notification per task cycle.
 *        
 * @param elapsed_seconds [in] How many seconds have elapsed since last called.
 */
void handler_cov_task(
    struct bacnet_session_object *sess,
    uint32_t elapsed_seconds)
{
    int index;
    uint32_t lifetime_seconds;
    BACNET_OBJECT_ID object_id;
    bool status = false;


    /* existing? - match Object ID and Process ID */
    for (index = 0; index < MAX_COV_SUBCRIPTIONS; index++) {
        if (get_bacnet_session_handler_data(sess)->
            COV_Subscriptions[index].valid) {
            /* handle timeouts */
            lifetime_seconds =
                get_bacnet_session_handler_data(sess)->COV_Subscriptions
                [index].lifetime;
            if (lifetime_seconds >= elapsed_seconds) {
                get_bacnet_session_handler_data(sess)->COV_Subscriptions
                    [index].lifetime -= elapsed_seconds;
#if 0
                fprintf(stderr, "COVtask: subscription[%d].lifetime=%d\n",
                    index,
                    get_bacnet_session_handler_data(sess)->COV_Subscriptions
                    [index].lifetime);
#endif
            } else {
                get_bacnet_session_handler_data(sess)->COV_Subscriptions
                    [index].lifetime = 0;
            }
            if (get_bacnet_session_handler_data(sess)->COV_Subscriptions
                [index].lifetime == 0) {
                get_bacnet_session_handler_data(sess)->COV_Subscriptions
                    [index].valid = false;
            }
            /* handle COV notifications */
            object_id.type =
                get_bacnet_session_handler_data(sess)->COV_Subscriptions
                [index].monitoredObjectIdentifier.type;
            object_id.instance =
                get_bacnet_session_handler_data(sess)->COV_Subscriptions
                [index].monitoredObjectIdentifier.instance;
            switch (object_id.type) {
                case OBJECT_BINARY_INPUT:
                    if (Binary_Input_Change_Of_Value(sess, object_id.instance)) {
                        get_bacnet_session_handler_data
                            (sess)->COV_Subscriptions[index].send_requested =
                            true;
                        Binary_Input_Change_Of_Value_Clear(sess,
                            object_id.instance);
                    }
                    break;
                default:
                    break;
            }
            if (get_bacnet_session_handler_data(sess)->COV_Subscriptions
                [index].send_requested) {
                status =
                    cov_send_request(sess,
                    &get_bacnet_session_handler_data(sess)->COV_Subscriptions
                    [index]);
                get_bacnet_session_handler_data(sess)->COV_Subscriptions
                    [index].send_requested = false;
            }
        }
    }
}

static bool cov_subscribe(
    struct bacnet_session_object *sess,
    BACNET_ADDRESS * src,
    BACNET_SUBSCRIBE_COV_DATA * cov_data,
    BACNET_ERROR_CLASS * error_class,
    BACNET_ERROR_CODE * error_code)
{
    bool status = false;        /* return value */

    switch (cov_data->monitoredObjectIdentifier.type) {
        case OBJECT_BINARY_INPUT:
            if (Binary_Input_Valid_Instance(sess,
                    cov_data->monitoredObjectIdentifier.instance)) {
                status =
                    cov_list_subscribe(sess, src, cov_data, error_class,
                    error_code);
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

/** Handler for a COV Subscribe Service request.
 * @ingroup DSCOV
 * This handler will be invoked by apdu_handler() if it has been enabled
 * by a call to apdu_set_confirmed_handler().
 * This handler builds a response packet, which is
 * - an Abort if
 *   - the message is segmented
 *   - if decoding fails
 * - an ACK, if cov_subscribe() succeeds
 * - an Error if cov_subscribe() fails 
 * 
 * @param service_request [in] The contents of the service request.
 * @param service_len [in] The length of the service_request.
 * @param src [in] BACNET_ADDRESS of the source of the message
 * @param service_data [in] The BACNET_CONFIRMED_SERVICE_DATA information 
 *                          decoded from the APDU header of this message. 
 */
void handler_cov_subscribe(
    struct bacnet_session_object *sess,
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
    uint8_t Handler_Transmit_Buffer[MAX_PDU] = { 0 };

    /* encode the NPDU portion of the packet */
    sess->datalink_get_my_address(sess, &my_address);
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
    success = cov_subscribe(sess, src, &cov_data, &error_class, &error_code);
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
        sess->datalink_send_pdu(sess, src, &npdu_data,
        &Handler_Transmit_Buffer[0], pdu_len);
#if PRINT_ENABLED
    if (bytes_sent <= 0)
        fprintf(stderr, "SubscribeCOV: Failed to send PDU (%s)!\n",
            strerror(errno));
#endif

    return;
}
