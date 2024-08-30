/**
 * @file
 * @brief A basic SubscribeCOV request handler, state machine, & task
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date 2007
 * @copyright SPDX-License-Identifier: MIT
 */
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
/* BACnet Stack defines - first */
#include "bacnet/bacdef.h"
/* BACnet Stack API */
#include "bacnet/bacerror.h"
#include "bacnet/bacdcode.h"
#include "bacnet/bacaddr.h"
#include "bacnet/apdu.h"
#include "bacnet/npdu.h"
#include "bacnet/abort.h"
#include "bacnet/reject.h"
#include "bacnet/cov.h"
#include "bacnet/dcc.h"
#if PRINT_ENABLED
#include "bacnet/bactext.h"
#endif
/* basic objects, services, TSM, and datalink */
#include "bacnet/basic/tsm/tsm.h"
#include "bacnet/basic/object/device.h"
#include "bacnet/basic/services.h"
#include "bacnet/datalink/datalink.h"

#ifndef MAX_COV_PROPERTIES
#define MAX_COV_PROPERTIES 2
#endif

typedef struct BACnet_COV_Address {
    bool valid : 1;
    BACNET_ADDRESS dest;
} BACNET_COV_ADDRESS;

/* note: This COV service only monitors the properties
   of an object that have been specified in the standard.  */
typedef struct BACnet_COV_Subscription_Flags {
    bool valid : 1;
    bool issueConfirmedNotifications : 1; /* optional */
    bool send_requested : 1;
} BACNET_COV_SUBSCRIPTION_FLAGS;

typedef struct BACnet_COV_Subscription {
    BACNET_COV_SUBSCRIPTION_FLAGS flag;
    unsigned dest_index;
    uint8_t invokeID; /* for confirmed COV */
    uint32_t subscriberProcessIdentifier;
    uint32_t lifetime; /* optional */
    BACNET_OBJECT_ID monitoredObjectIdentifier;
} BACNET_COV_SUBSCRIPTION;

#ifndef MAX_COV_SUBCRIPTIONS
#define MAX_COV_SUBCRIPTIONS 128
#endif
static BACNET_COV_SUBSCRIPTION COV_Subscriptions[MAX_COV_SUBCRIPTIONS];
#ifndef MAX_COV_ADDRESSES
#define MAX_COV_ADDRESSES 16
#endif
static BACNET_COV_ADDRESS COV_Addresses[MAX_COV_ADDRESSES];

/**
 * Gets the address from the list of COV addresses
 *
 * @param  index - offset into COV address list where address is stored
 * @param  dest - address to be filled when found
 *
 * @return true if valid address, false if not valid or not found
 */
static BACNET_ADDRESS *cov_address_get(unsigned index)
{
    BACNET_ADDRESS *cov_dest = NULL;

    if (index < MAX_COV_ADDRESSES) {
        if (COV_Addresses[index].valid) {
            cov_dest = &COV_Addresses[index].dest;
        }
    }

    return cov_dest;
}

/**
 * Removes the address from the list of COV addresses, if it is not
 * used by other COV subscriptions
 */
static void cov_address_remove_unused(void)
{
    unsigned index = 0;
    unsigned cov_index = 0;
    bool found = false;

    for (cov_index = 0; cov_index < MAX_COV_ADDRESSES; cov_index++) {
        if (COV_Addresses[cov_index].valid) {
            found = false;
            for (index = 0; index < MAX_COV_SUBCRIPTIONS; index++) {
                if ((COV_Subscriptions[index].flag.valid) &&
                    (COV_Subscriptions[index].dest_index == cov_index)) {
                    found = true;
                    break;
                }
            }
            if (!found) {
                COV_Addresses[cov_index].valid = false;
            }
        }
    }
}

/**
 * Adds the address to the list of COV addresses
 *
 * @param  dest - address to be added if there is room in the list
 *
 * @return index number 0..N, or -1 if unable to add
 */
static int cov_address_add(const BACNET_ADDRESS *dest)
{
    int index = -1;
    unsigned i = 0;
    bool found = false;
    bool valid = false;
    BACNET_ADDRESS *cov_dest = NULL;

    if (dest) {
        for (i = 0; i < MAX_COV_ADDRESSES; i++) {
            valid = COV_Addresses[i].valid;
            if (valid) {
                cov_dest = &COV_Addresses[i].dest;
                found = bacnet_address_same(dest, cov_dest);
                if (found) {
                    index = i;
                    break;
                }
            }
        }
        if (!found) {
            /* find a free place to add a new address */
            for (i = 0; i < MAX_COV_ADDRESSES; i++) {
                valid = COV_Addresses[i].valid;
                if (!valid) {
                    index = i;
                    cov_dest = &COV_Addresses[i].dest;
                    bacnet_address_copy(cov_dest, dest);
                    COV_Addresses[i].valid = true;
                    break;
                }
            }
        }
    }

    return index;
}

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
    uint8_t *apdu,
    int max_apdu,
    const BACNET_COV_SUBSCRIPTION *cov_subscription)
{
    int len = 0;
    int apdu_len = 0;
    BACNET_OCTET_STRING octet_string;
    BACNET_ADDRESS *dest = NULL;

    (void)max_apdu;
    if (!cov_subscription) {
        return 0;
    }
    dest = cov_address_get(cov_subscription->dest_index);
    if (!dest) {
        return 0;
    }
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
    len = encode_application_unsigned(&apdu[apdu_len], dest->net);
    apdu_len += len;
    /* mac-address OCTET STRING */
    /* -- A string of length 0 indicates a broadcast */
    if (dest->net) {
        octetstring_init(&octet_string, &dest->adr[0], dest->len);
    } else {
        octetstring_init(&octet_string, &dest->mac[0], dest->mac_len);
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
    len = encode_context_unsigned(
        &apdu[apdu_len], 1, cov_subscription->subscriberProcessIdentifier);
    apdu_len += len;
    /* Recipient [0] BACnetRecipientProcess - closing */
    len = encode_closing_tag(&apdu[apdu_len], 0);
    apdu_len += len;
    /*  MonitoredPropertyReference [1] BACnetObjectPropertyReference, */
    len = encode_opening_tag(&apdu[apdu_len], 1);
    apdu_len += len;
    /* objectIdentifier [0] */
    len = encode_context_object_id(
        &apdu[apdu_len], 0, cov_subscription->monitoredObjectIdentifier.type,
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
    len = encode_context_boolean(
        &apdu[apdu_len], 2, cov_subscription->flag.issueConfirmedNotifications);
    apdu_len += len;
    /* TimeRemaining [3] Unsigned, */
    len =
        encode_context_unsigned(&apdu[apdu_len], 3, cov_subscription->lifetime);
    apdu_len += len;

    return apdu_len;
}

/** Handle a request to list all the COV subscriptions.
 * @ingroup DSCOV
 *  Invoked by a request to read the Device object's
 * PROP_ACTIVE_COV_SUBSCRIPTIONS. Loops through the list of COV Subscriptions,
 * and, for each valid one, adds its description to the APDU.
 *  @note This function needs some work to better handle buffer overruns.
 *  @param apdu [out] Buffer in which the APDU contents are built.
 *  @param max_apdu [in] Max length of the APDU buffer.
 *  @return How many bytes were encoded in the buffer, or -2 if the response
 *          would not fit within the buffer.
 */
int handler_cov_encode_subscriptions(uint8_t *apdu, int max_apdu)
{
    int len = 0;
    int apdu_len = 0;
    unsigned index = 0;

    if (apdu) {
        for (index = 0; index < MAX_COV_SUBCRIPTIONS; index++) {
            if (COV_Subscriptions[index].flag.valid) {
                len = cov_encode_subscription(
                    &apdu[apdu_len], max_apdu - apdu_len,
                    &COV_Subscriptions[index]);
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
void handler_cov_init(void)
{
    unsigned index = 0;

    for (index = 0; index < MAX_COV_SUBCRIPTIONS; index++) {
        /* initialize with invalid COV address */
        COV_Subscriptions[index].flag.valid = false;
        COV_Subscriptions[index].dest_index = MAX_COV_ADDRESSES;
        COV_Subscriptions[index].subscriberProcessIdentifier = 0;
        COV_Subscriptions[index].monitoredObjectIdentifier.type =
            OBJECT_ANALOG_INPUT;
        COV_Subscriptions[index].monitoredObjectIdentifier.instance = 0;
        COV_Subscriptions[index].flag.issueConfirmedNotifications = false;
        COV_Subscriptions[index].invokeID = 0;
        COV_Subscriptions[index].lifetime = 0;
        COV_Subscriptions[index].flag.send_requested = false;
    }
    for (index = 0; index < MAX_COV_ADDRESSES; index++) {
        COV_Addresses[index].valid = false;
    }
}

static bool cov_list_subscribe(
    const BACNET_ADDRESS *src,
    const BACNET_SUBSCRIBE_COV_DATA *cov_data,
    BACNET_ERROR_CLASS *error_class,
    BACNET_ERROR_CODE *error_code)
{
    bool existing_entry = false;
    int index;
    int first_invalid_index = -1;
    bool found = true;
    bool address_match = false;
    const BACNET_ADDRESS *dest = NULL;

    /* unable to subscribe - resources? */
    /* unable to cancel subscription - other? */

    /* existing? - match Object ID and Process ID and address */
    for (index = 0; index < MAX_COV_SUBCRIPTIONS; index++) {
        if (COV_Subscriptions[index].flag.valid) {
            dest = cov_address_get(COV_Subscriptions[index].dest_index);
            if (dest) {
                address_match = bacnet_address_same(src, dest);
            } else {
                /* skip address matching - we don't have an address */
                address_match = true;
            }
            if ((COV_Subscriptions[index].monitoredObjectIdentifier.type ==
                 cov_data->monitoredObjectIdentifier.type) &&
                (COV_Subscriptions[index].monitoredObjectIdentifier.instance ==
                 cov_data->monitoredObjectIdentifier.instance) &&
                (COV_Subscriptions[index].subscriberProcessIdentifier ==
                 cov_data->subscriberProcessIdentifier) &&
                address_match) {
                existing_entry = true;
                if (cov_data->cancellationRequest) {
                    /* initialize with invalid COV address */
                    COV_Subscriptions[index].flag.valid = false;
                    COV_Subscriptions[index].dest_index = MAX_COV_ADDRESSES;
                    cov_address_remove_unused();
                } else {
                    COV_Subscriptions[index].dest_index = cov_address_add(src);
                    COV_Subscriptions[index].flag.issueConfirmedNotifications =
                        cov_data->issueConfirmedNotifications;
                    COV_Subscriptions[index].lifetime = cov_data->lifetime;
                    COV_Subscriptions[index].flag.send_requested = true;
                }
                if (COV_Subscriptions[index].invokeID) {
                    tsm_free_invoke_id(COV_Subscriptions[index].invokeID);
                    COV_Subscriptions[index].invokeID = 0;
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
        const int addr_add_ret = cov_address_add(src);

        if (addr_add_ret < 0) {
            *error_class = ERROR_CLASS_RESOURCES;
            *error_code = ERROR_CODE_NO_SPACE_TO_ADD_LIST_ELEMENT;
            found = false;
        } else {
            COV_Subscriptions[index].dest_index = addr_add_ret;
            index = first_invalid_index;
            found = true;
            COV_Subscriptions[index].flag.valid = true;
            COV_Subscriptions[index].monitoredObjectIdentifier.type =
                cov_data->monitoredObjectIdentifier.type;
            COV_Subscriptions[index].monitoredObjectIdentifier.instance =
                cov_data->monitoredObjectIdentifier.instance;
            COV_Subscriptions[index].subscriberProcessIdentifier =
                cov_data->subscriberProcessIdentifier;
            COV_Subscriptions[index].flag.issueConfirmedNotifications =
                cov_data->issueConfirmedNotifications;
            COV_Subscriptions[index].invokeID = 0;
            COV_Subscriptions[index].lifetime = cov_data->lifetime;
            COV_Subscriptions[index].flag.send_requested = true;
        }
    } else if (!existing_entry) {
        if (first_invalid_index < 0) {
            /* Out of resources */
            *error_class = ERROR_CLASS_RESOURCES;
            *error_code = ERROR_CODE_NO_SPACE_TO_ADD_LIST_ELEMENT;
            found = false;
        } else {
            /* cancellationRequest - valid object not subscribed */
            /* From BACnet Standard 135-2010-13.14.2
               ...Cancellations that are issued for which no matching COV
               context can be found shall succeed as if a context had
               existed, returning 'Result(+)'. */
            found = true;
        }
    }

    return found;
}

static bool cov_send_request(
    BACNET_COV_SUBSCRIPTION *cov_subscription,
    BACNET_PROPERTY_VALUE *value_list)
{
    int len = 0;
    int pdu_len = 0;
    BACNET_NPDU_DATA npdu_data;
    BACNET_ADDRESS my_address;
    int bytes_sent = 0;
    uint8_t invoke_id = 0;
    bool status = false; /* return value */
    BACNET_COV_DATA cov_data;
    BACNET_ADDRESS *dest = NULL;

    if (!dcc_communication_enabled()) {
        return status;
    }
#if PRINT_ENABLED
    fprintf(stderr, "COVnotification: requested\n");
#endif
    if (!cov_subscription) {
        return status;
    }
    dest = cov_address_get(cov_subscription->dest_index);
    if (!dest) {
#if PRINT_ENABLED
        fprintf(stderr, "COVnotification: dest not found!\n");
#endif
        return status;
    }
    datalink_get_my_address(&my_address);
    npdu_encode_npdu_data(&npdu_data, false, MESSAGE_PRIORITY_NORMAL);
    pdu_len = npdu_encode_pdu(
        &Handler_Transmit_Buffer[0], dest, &my_address, &npdu_data);
    /* load the COV data structure for outgoing message */
    cov_data.subscriberProcessIdentifier =
        cov_subscription->subscriberProcessIdentifier;
    cov_data.initiatingDeviceIdentifier = Device_Object_Instance_Number();
    cov_data.monitoredObjectIdentifier.type =
        cov_subscription->monitoredObjectIdentifier.type;
    cov_data.monitoredObjectIdentifier.instance =
        cov_subscription->monitoredObjectIdentifier.instance;
    cov_data.timeRemaining = cov_subscription->lifetime;
    cov_data.listOfValues = value_list;
    if (cov_subscription->flag.issueConfirmedNotifications) {
        npdu_data.data_expecting_reply = true;
        invoke_id = tsm_next_free_invokeID();
        if (invoke_id) {
            cov_subscription->invokeID = invoke_id;
            len = ccov_notify_encode_apdu(
                &Handler_Transmit_Buffer[pdu_len],
                sizeof(Handler_Transmit_Buffer) - pdu_len, invoke_id,
                &cov_data);
        } else {
            goto COV_FAILED;
        }
    } else {
        len = ucov_notify_encode_apdu(
            &Handler_Transmit_Buffer[pdu_len],
            sizeof(Handler_Transmit_Buffer) - pdu_len, &cov_data);
    }
    pdu_len += len;
    if (cov_subscription->flag.issueConfirmedNotifications) {
        tsm_set_confirmed_unsegmented_transaction(
            invoke_id, dest, &npdu_data, &Handler_Transmit_Buffer[0],
            (uint16_t)pdu_len);
    }
    bytes_sent = datalink_send_pdu(
        dest, &npdu_data, &Handler_Transmit_Buffer[0], pdu_len);
    if (bytes_sent > 0) {
        status = true;
#if PRINT_ENABLED
        fprintf(stderr, "COVnotification: Sent!\n");
#endif
    }

COV_FAILED:

    return status;
}

static void cov_lifetime_expiration_handler(
    unsigned index, uint32_t elapsed_seconds, uint32_t lifetime_seconds)
{
    if (index < MAX_COV_SUBCRIPTIONS) {
        /* handle lifetime expiration */
        if (lifetime_seconds >= elapsed_seconds) {
            COV_Subscriptions[index].lifetime -= elapsed_seconds;
#if 0
            fprintf(stderr, "COVtimer: subscription[%d].lifetime=%lu\n", index,
                (unsigned long) COV_Subscriptions[index].lifetime);
#endif
        } else {
            COV_Subscriptions[index].lifetime = 0;
        }
        if (COV_Subscriptions[index].lifetime == 0) {
            /* expire the subscription */
#if PRINT_ENABLED
            fprintf(
                stderr, "COVtimer: PID=%u ",
                COV_Subscriptions[index].subscriberProcessIdentifier);
            fprintf(
                stderr, "%s %u ",
                bactext_object_type_name(
                    COV_Subscriptions[index].monitoredObjectIdentifier.type),
                COV_Subscriptions[index].monitoredObjectIdentifier.instance);
            fprintf(
                stderr, "time remaining=%u seconds ",
                COV_Subscriptions[index].lifetime);
            fprintf(stderr, "\n");
#endif
            /* initialize with invalid COV address */
            COV_Subscriptions[index].flag.valid = false;
            COV_Subscriptions[index].dest_index = MAX_COV_ADDRESSES;
            cov_address_remove_unused();
            if (COV_Subscriptions[index].flag.issueConfirmedNotifications) {
                if (COV_Subscriptions[index].invokeID) {
                    tsm_free_invoke_id(COV_Subscriptions[index].invokeID);
                    COV_Subscriptions[index].invokeID = 0;
                }
            }
        }
    }
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
void handler_cov_timer_seconds(uint32_t elapsed_seconds)
{
    unsigned index = 0;
    uint32_t lifetime_seconds = 0;

    if (elapsed_seconds) {
        /* handle the subscription timeouts */
        for (index = 0; index < MAX_COV_SUBCRIPTIONS; index++) {
            if (COV_Subscriptions[index].flag.valid) {
                lifetime_seconds = COV_Subscriptions[index].lifetime;
                if (lifetime_seconds) {
                    /* only expire COV with definite lifetimes */
                    cov_lifetime_expiration_handler(
                        index, elapsed_seconds, lifetime_seconds);
                }
            }
        }
    }
}

bool handler_cov_fsm(void)
{
    static int index = 0;
    BACNET_OBJECT_TYPE object_type = MAX_BACNET_OBJECT_TYPE;
    uint32_t object_instance = 0;
    bool status = false;
    bool send = false;
    BACNET_PROPERTY_VALUE value_list[MAX_COV_PROPERTIES];
    /* states for transmitting */
    static enum {
        COV_STATE_IDLE = 0,
        COV_STATE_MARK,
        COV_STATE_CLEAR,
        COV_STATE_FREE,
        COV_STATE_SEND
    } cov_task_state = COV_STATE_IDLE;

    switch (cov_task_state) {
        case COV_STATE_IDLE:
            index = 0;
            cov_task_state = COV_STATE_MARK;
            break;
        case COV_STATE_MARK:
            /* mark any subscriptions where the value has changed */
            if (COV_Subscriptions[index].flag.valid) {
                object_type = (BACNET_OBJECT_TYPE)COV_Subscriptions[index]
                                  .monitoredObjectIdentifier.type;
                object_instance =
                    COV_Subscriptions[index].monitoredObjectIdentifier.instance;
                status = Device_COV(object_type, object_instance);
                if (status) {
                    COV_Subscriptions[index].flag.send_requested = true;
#if PRINT_ENABLED
                    fprintf(stderr, "COVtask: Marking...\n");
#endif
                }
            }
            index++;
            if (index >= MAX_COV_SUBCRIPTIONS) {
                index = 0;
                cov_task_state = COV_STATE_CLEAR;
            }
            break;
        case COV_STATE_CLEAR:
            /* clear the COV flag after checking all subscriptions */
            if ((COV_Subscriptions[index].flag.valid) &&
                (COV_Subscriptions[index].flag.send_requested)) {
                object_type = (BACNET_OBJECT_TYPE)COV_Subscriptions[index]
                                  .monitoredObjectIdentifier.type;
                object_instance =
                    COV_Subscriptions[index].monitoredObjectIdentifier.instance;
                Device_COV_Clear(object_type, object_instance);
            }
            index++;
            if (index >= MAX_COV_SUBCRIPTIONS) {
                index = 0;
                cov_task_state = COV_STATE_FREE;
            }
            break;
        case COV_STATE_FREE:
            /* confirmed notification house keeping */
            if ((COV_Subscriptions[index].flag.valid) &&
                (COV_Subscriptions[index].flag.issueConfirmedNotifications) &&
                (COV_Subscriptions[index].invokeID)) {
                if (tsm_invoke_id_free(COV_Subscriptions[index].invokeID)) {
                    COV_Subscriptions[index].invokeID = 0;
                } else if (tsm_invoke_id_failed(
                               COV_Subscriptions[index].invokeID)) {
                    tsm_free_invoke_id(COV_Subscriptions[index].invokeID);
                    COV_Subscriptions[index].invokeID = 0;
                }
            }
            index++;
            if (index >= MAX_COV_SUBCRIPTIONS) {
                index = 0;
                cov_task_state = COV_STATE_SEND;
            }
            break;
        case COV_STATE_SEND:
            /* send any COVs that are requested */
            if ((COV_Subscriptions[index].flag.valid) &&
                (COV_Subscriptions[index].flag.send_requested)) {
                send = true;
                if (COV_Subscriptions[index].flag.issueConfirmedNotifications) {
                    if (COV_Subscriptions[index].invokeID != 0) {
                        /* already sending */
                        send = false;
                    }
                    if (!tsm_transaction_available()) {
                        /* no transactions available - can't send now */
                        send = false;
                    }
                }
                if (send) {
                    object_type = (BACNET_OBJECT_TYPE)COV_Subscriptions[index]
                                      .monitoredObjectIdentifier.type;
                    object_instance = COV_Subscriptions[index]
                                          .monitoredObjectIdentifier.instance;
#if PRINT_ENABLED
                    fprintf(stderr, "COVtask: Sending...\n");
#endif
                    /* configure the linked list for the two properties */
                    bacapp_property_value_list_init(
                        &value_list[0], MAX_COV_PROPERTIES);
                    status = Device_Encode_Value_List(
                        object_type, object_instance, &value_list[0]);
                    if (status) {
                        status = cov_send_request(
                            &COV_Subscriptions[index], &value_list[0]);
                    }
                    if (status) {
                        COV_Subscriptions[index].flag.send_requested = false;
                    }
                }
            }
            index++;
            if (index >= MAX_COV_SUBCRIPTIONS) {
                index = 0;
                cov_task_state = COV_STATE_IDLE;
            }
            break;
        default:
            index = 0;
            cov_task_state = COV_STATE_IDLE;
            break;
    }
    return (cov_task_state == COV_STATE_IDLE);
}

void handler_cov_task(void)
{
    handler_cov_fsm();
}

static bool cov_subscribe(
    const BACNET_ADDRESS *src,
    const BACNET_SUBSCRIBE_COV_DATA *cov_data,
    BACNET_ERROR_CLASS *error_class,
    BACNET_ERROR_CODE *error_code)
{
    bool status = false; /* return value */
    BACNET_OBJECT_TYPE object_type = MAX_BACNET_OBJECT_TYPE;
    uint32_t object_instance = 0;

    object_type = (BACNET_OBJECT_TYPE)cov_data->monitoredObjectIdentifier.type;
    object_instance = cov_data->monitoredObjectIdentifier.instance;
    status = Device_Valid_Object_Id(object_type, object_instance);
    if (status) {
        status = Device_Value_List_Supported(object_type);
        if (status) {
            status = cov_list_subscribe(src, cov_data, error_class, error_code);
        } else if (cov_data->cancellationRequest) {
            /* From BACnet Standard 135-2010-13.14.2
               ...Cancellations that are issued for which no matching COV
               context can be found shall succeed as if a context had
               existed, returning 'Result(+)'. */
            status = true;
        } else {
            *error_class = ERROR_CLASS_OBJECT;
            *error_code = ERROR_CODE_OPTIONAL_FUNCTIONALITY_NOT_SUPPORTED;
        }
    } else if (cov_data->cancellationRequest) {
        /* From BACnet Standard 135-2010-13.14.2
            ...Cancellations that are issued for which no matching COV
            context can be found shall succeed as if a context had
            existed, returning 'Result(+)'. */
        status = true;
    } else {
        *error_class = ERROR_CLASS_OBJECT;
        *error_code = ERROR_CODE_UNKNOWN_OBJECT;
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
    uint8_t *service_request,
    uint16_t service_len,
    BACNET_ADDRESS *src,
    BACNET_CONFIRMED_SERVICE_DATA *service_data)
{
    BACNET_SUBSCRIBE_COV_DATA cov_data;
    int len = 0;
    int pdu_len = 0;
    int npdu_len = 0;
    int apdu_len = 0;
    BACNET_NPDU_DATA npdu_data;
    bool success = false;
    int bytes_sent = 0;
    BACNET_ADDRESS my_address;
    bool error = false;

    /* initialize a common abort code */
    cov_data.error_code = ERROR_CODE_ABORT_SEGMENTATION_NOT_SUPPORTED;
    /* encode the NPDU portion of the packet */
    datalink_get_my_address(&my_address);
    npdu_encode_npdu_data(&npdu_data, false, MESSAGE_PRIORITY_NORMAL);
    npdu_len = npdu_encode_pdu(
        &Handler_Transmit_Buffer[0], src, &my_address, &npdu_data);
    if (service_data->segmented_message) {
        /* we don't support segmentation - send an abort */
        len = BACNET_STATUS_ABORT;
#if PRINT_ENABLED
        fprintf(stderr, "SubscribeCOV: Segmented message.  Sending Abort!\n");
#endif
        error = true;
    } else {
        len = cov_subscribe_decode_service_request(
            service_request, service_len, &cov_data);
#if PRINT_ENABLED
        if (len <= 0) {
            fprintf(stderr, "SubscribeCOV: Unable to decode Request!\n");
        }
#endif
        if (len < 0) {
            error = true;
        } else {
            cov_data.error_class = ERROR_CLASS_OBJECT;
            cov_data.error_code = ERROR_CODE_UNKNOWN_OBJECT;
            success = cov_subscribe(
                src, &cov_data, &cov_data.error_class, &cov_data.error_code);
            if (success) {
                apdu_len = encode_simple_ack(
                    &Handler_Transmit_Buffer[npdu_len], service_data->invoke_id,
                    SERVICE_CONFIRMED_SUBSCRIBE_COV);
#if PRINT_ENABLED
                fprintf(stderr, "SubscribeCOV: Sending Simple Ack!\n");
#endif
            } else {
                len = BACNET_STATUS_ERROR;
                error = true;
#if PRINT_ENABLED
                fprintf(stderr, "SubscribeCOV: Sending Error!\n");
#endif
            }
        }
    }

    /* Error? */
    if (error) {
        if (len == BACNET_STATUS_ABORT) {
            apdu_len = abort_encode_apdu(
                &Handler_Transmit_Buffer[npdu_len], service_data->invoke_id,
                abort_convert_error_code(cov_data.error_code), true);
#if PRINT_ENABLED
            fprintf(stderr, "SubscribeCOV: Sending Abort!\n");
#endif
        } else if (len == BACNET_STATUS_ERROR) {
            apdu_len = bacerror_encode_apdu(
                &Handler_Transmit_Buffer[npdu_len], service_data->invoke_id,
                SERVICE_CONFIRMED_SUBSCRIBE_COV, cov_data.error_class,
                cov_data.error_code);
#if PRINT_ENABLED
            fprintf(stderr, "SubscribeCOV: Sending Error!\n");
#endif
        } else if (len == BACNET_STATUS_REJECT) {
            apdu_len = reject_encode_apdu(
                &Handler_Transmit_Buffer[npdu_len], service_data->invoke_id,
                reject_convert_error_code(cov_data.error_code));
#if PRINT_ENABLED
            fprintf(stderr, "SubscribeCOV: Sending Reject!\n");
#endif
        }
    }
    pdu_len = npdu_len + apdu_len;
    bytes_sent = datalink_send_pdu(
        src, &npdu_data, &Handler_Transmit_Buffer[0], pdu_len);
    if (bytes_sent <= 0) {
#if PRINT_ENABLED
        fprintf(
            stderr, "SubscribeCOV: Failed to send PDU (%s)!\n",
            strerror(errno));
#endif
    }

    return;
}
