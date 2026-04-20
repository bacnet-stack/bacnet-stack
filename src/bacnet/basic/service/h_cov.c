/**
 * @file
 * @brief A basic SubscribeCOV request handler, state machine, & task
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date 2007
 * @copyright SPDX-License-Identifier: MIT
 */
#include <stddef.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
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
#include "bacnet/basic/sys/keylist.h"
#include "bacnet/basic/sys/debug.h"
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
    bool issueConfirmedNotifications : 1; /* optional */
    bool send_requested : 1;
} BACNET_COV_SUBSCRIPTION_FLAGS;

typedef struct My_BACnet_COV_Subscription {
    BACNET_COV_SUBSCRIPTION_FLAGS flag;
    unsigned dest_index;
    uint8_t invokeID; /* for confirmed COV */
    uint32_t subscriberProcessIdentifier;
    uint32_t lifetime; /* optional */
    BACNET_OBJECT_ID monitoredObjectIdentifier;
} MY_BACNET_COV_SUBSCRIPTION;

#ifndef MAX_COV_SUBSCRIPTIONS
#define MAX_COV_SUBSCRIPTIONS 128
#endif

static OS_Keylist COV_Subscriptions_List[MAX_NUM_DEVICES];
#ifdef BAC_ROUTING
#define COV_Subscriptions (COV_Subscriptions_List[Routed_Device_Object_Index()])
#else
#define COV_Subscriptions (COV_Subscriptions_List[0])
#endif
#ifndef MAX_COV_ADDRESSES
#define MAX_COV_ADDRESSES 16
#endif
static BACNET_COV_ADDRESS COV_Addresses[MAX_COV_ADDRESSES];

/**
 * @brief Deletes a subscription
 * @param list_idx - keylist index
 * @return true if the subscription was deleted
 */
static bool cov_subscription_delete(uint32_t list_idx)
{
    bool status = false;
    BACNET_COV_SUBSCRIPTION *subscription = NULL;

    subscription = Keylist_Data_Delete_By_Index(COV_Subscriptions, list_idx);
    if (subscription) {
        free(subscription);
        status = true;
    }

    return status;
}

/**
 * @brief Creates a subscription
 * @param list_key - keylist key
 * @return the subscription that was created, or NULL
 */
static BACNET_COV_SUBSCRIPTION *cov_subscription_create(uint32_t list_key)
{
    BACNET_COV_SUBSCRIPTION *subscription = NULL;
    int index = 0;
    if (list_key >= MAX_COV_SUBSCRIPTIONS) {
        return NULL;
    }
    subscription = Keylist_Data(COV_Subscriptions, list_key);
    if (!subscription) {
        subscription = calloc(1, sizeof(BACNET_COV_SUBSCRIPTION));
        if (subscription) {
            index = Keylist_Data_Add(COV_Subscriptions, list_key, subscription);
            if (index < 0) {
                free(subscription);
                return NULL;
            }
        } else {
            return NULL;
        }
    }
    return subscription;
}

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
    BACNET_COV_SUBSCRIPTION *subscription = NULL;

#ifdef BAC_ROUTING
    uint16_t current_dev_id = Routed_Device_Object_Index();
    uint16_t dev_id = 0;
    for (cov_index = 0; cov_index < MAX_COV_ADDRESSES; cov_index++) {
        if (COV_Addresses[cov_index].valid) {
            found = false;
            for (dev_id = 0; dev_id < Get_Num_Managed_Devices(); dev_id++) {
                Set_Routed_Device_Object_Index(dev_id);
                for (index = 0; index < Keylist_Count(COV_Subscriptions);
                     index++) {
                    subscription = Keylist_Data_Index(COV_Subscriptions, index);
                    if (subscription &&
                        (subscription->dest_index == cov_index)) {
                        found = true;
                        break;
                    }
                }
                if (found) {
                    break;
                }
            }
            if (!found) {
                COV_Addresses[cov_index].valid = false;
            }
        }
    }
    Set_Routed_Device_Object_Index(current_dev_id);
#else
    for (cov_index = 0; cov_index < MAX_COV_ADDRESSES; cov_index++) {
        if (COV_Addresses[cov_index].valid) {
            found = false;
            for (index = 0; index < Keylist_Count(COV_Subscriptions); index++) {
                subscription = Keylist_Data_Index(COV_Subscriptions, index);
                if (subscription && (subscription->dest_index == cov_index)) {
                    found = true;
                    break;
                }
            }
            if (!found) {
                COV_Addresses[cov_index].valid = false;
            }
        }
    }
#endif
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

/** Handle a request to list all the COV subscriptions.
 * @ingroup DSCOV
 *  Invoked by a request to read the Device object's
 * PROP_ACTIVE_COV_SUBSCRIPTIONS. Loops through the list of COV Subscriptions,
 * and, for each valid one, adds its description to the APDU.
 *  @param apdu [out] Buffer in which the APDU contents are built.
 *  @param apdu_size [in] Max length of the APDU buffer.
 *  @return How many bytes were encoded in the buffer, or
 *   #BACNET_STATUS_ABORT if the response would not fit within the buffer.
 */
int handler_cov_encode_subscriptions(uint8_t *apdu, int apdu_size)
{
    if (apdu) {
        uint8_t cov_sub[MAX_COV_SUB_SIZE] = {
            0,
        };
        unsigned index = 0;
        int apdu_len = 0;
        BACNET_COV_SUBSCRIPTION *subscription = NULL;

        for (index = 0; index < Keylist_Count(COV_Subscriptions); index++) {
            subscription = Keylist_Data_Index(COV_Subscriptions, index);
            if (subscription) {
                /* Lets encode a COV subscription into an intermediate buffer
                 * that can hold it */
                int len = cov_encode_subscription(
                    &cov_sub[0], max_apdu - apdu_len, subscription);

                if ((apdu_len + len) > max_apdu) {
                    return -2;
                }

                /* Lets copy if and only if it fits in the buffer */
                memcpy(&apdu[apdu_len], cov_sub, len);
                apdu_len += len;
            }
        }
        dest = cov_address_get(COV_Subscriptions[index].dest_index);
        if (!dest) {
            continue;
        }
        data.cov_increment_present = false;
        data.cov_increment = 1.0f; /* dummy value */
        data.issue_confirmed_notifications =
            COV_Subscriptions[index].flag.issueConfirmedNotifications;
        data.monitored_property_reference.object_identifier.type =
            COV_Subscriptions[index].monitoredObjectIdentifier.type;
        data.monitored_property_reference.object_identifier.instance =
            COV_Subscriptions[index].monitoredObjectIdentifier.instance;
        data.monitored_property_reference.property_identifier =
            PROP_PRESENT_VALUE;
        data.monitored_property_reference.property_array_index =
            BACNET_ARRAY_ALL;
        data.recipient.process_identifier =
            COV_Subscriptions[index].subscriberProcessIdentifier;
        data.recipient.recipient.tag = BACNET_RECIPIENT_TAG_ADDRESS;
        bacnet_address_copy(&data.recipient.recipient.type.address, dest);
        len = bacnet_cov_subscription_encode(apdu, apdu_size - apdu_len, &data);
        if (len <= 0) {
            return BACNET_STATUS_ABORT;
        }
        apdu_len += len;
    }

    return apdu_len;
}

/** Handler to initialize the COV list, clearing and disabling each entry.
 * @ingroup DSCOV
 */
void handler_cov_init(void)
{
    unsigned index = 0;
#ifdef BAC_ROUTING
    uint16_t current_dev_id = Routed_Device_Object_Index();
    uint16_t dev_id = 0;
    for (dev_id = 0; dev_id < MAX_NUM_DEVICES; dev_id++) {
        Set_Routed_Device_Object_Index(dev_id);
        if (!COV_Subscriptions) {
            COV_Subscriptions = Keylist_Create();
        } else {
            Keylist_Data_Free(COV_Subscriptions);
        }
    }
    Set_Routed_Device_Object_Index(current_dev_id);
#else
    if (!COV_Subscriptions) {
        COV_Subscriptions = Keylist_Create();
    } else {
        Keylist_Data_Free(COV_Subscriptions);
    }
#endif
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
    bool found = true;
    bool address_match = false;
    const BACNET_ADDRESS *dest = NULL;
    BACNET_COV_SUBSCRIPTION *subscription = NULL;

    /* unable to subscribe - resources? */
    /* unable to cancel subscription - other? */

    /* existing? - match Object ID and Process ID and address */
    for (index = 0; index < Keylist_Count(COV_Subscriptions); index++) {
        subscription = Keylist_Data_Index(COV_Subscriptions, index);
        if (subscription) {
            dest = cov_address_get(subscription->dest_index);
            if (dest) {
                address_match = bacnet_address_same(src, dest);
            } else {
                /* skip address matching - we don't have an address */
                address_match = true;
            }
            if ((subscription->monitoredObjectIdentifier.type ==
                 cov_data->monitoredObjectIdentifier.type) &&
                (subscription->monitoredObjectIdentifier.instance ==
                 cov_data->monitoredObjectIdentifier.instance) &&
                (subscription->subscriberProcessIdentifier ==
                 cov_data->subscriberProcessIdentifier) &&
                address_match) {
                existing_entry = true;
                if (subscription->invokeID) {
                    tsm_free_invoke_id(subscription->invokeID);
                    subscription->invokeID = 0;
                }
                if (cov_data->cancellationRequest) {
                    cov_subscription_delete(index);
                    cov_address_remove_unused();
                } else {
                    subscription->dest_index = cov_address_add(src);
                    subscription->flag.issueConfirmedNotifications =
                        cov_data->issueConfirmedNotifications;
                    subscription->lifetime = cov_data->lifetime;
                    subscription->flag.send_requested = true;
                }

                break;
            }
        }
    }
    if (!existing_entry &&
        (Keylist_Count(COV_Subscriptions) < MAX_COV_SUBSCRIPTIONS) &&
        (!cov_data->cancellationRequest)) {
        const int addr_add_ret = cov_address_add(src);

        if (addr_add_ret < 0) {
            *error_class = ERROR_CLASS_RESOURCES;
            *error_code = ERROR_CODE_NO_SPACE_TO_ADD_LIST_ELEMENT;
            found = false;
        } else {
            found = true;
            index = Keylist_Next_Empty_Key(COV_Subscriptions, 0);
            subscription = cov_subscription_create(index);
            if (subscription) {
                subscription->dest_index = addr_add_ret;
                subscription->monitoredObjectIdentifier.type =
                    cov_data->monitoredObjectIdentifier.type;
                subscription->monitoredObjectIdentifier.instance =
                    cov_data->monitoredObjectIdentifier.instance;
                subscription->subscriberProcessIdentifier =
                    cov_data->subscriberProcessIdentifier;
                subscription->flag.issueConfirmedNotifications =
                    cov_data->issueConfirmedNotifications;
                subscription->invokeID = 0;
                subscription->lifetime = cov_data->lifetime;
                subscription->flag.send_requested = true;
            } else {
                *error_class = ERROR_CLASS_RESOURCES;
                *error_code = ERROR_CODE_NO_SPACE_TO_ADD_LIST_ELEMENT;
                found = false;
            }
        }
    } else if (!existing_entry) {
        if (Keylist_Count(COV_Subscriptions) >= MAX_COV_SUBSCRIPTIONS) {
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
    MY_BACNET_COV_SUBSCRIPTION *cov_subscription,
    BACNET_PROPERTY_VALUE *value_list)
{
    int len = 0;
    int pdu_len = 0;
    BACNET_NPDU_DATA npdu_data = { 0 };
    BACNET_ADDRESS my_address = { 0 };
    int bytes_sent = 0;
    uint8_t invoke_id = 0;
    bool status = false; /* return value */
    BACNET_COV_DATA cov_data = { 0 };
    BACNET_ADDRESS *dest = NULL;

    if (!dcc_communication_enabled()) {
        return status;
    }
#if PRINT_ENABLED
    debug_fprintf(stderr, "COVnotification: requested\n");
#endif
    if (!cov_subscription) {
        return status;
    }
    dest = cov_address_get(cov_subscription->dest_index);
    if (!dest) {
#if PRINT_ENABLED
        debug_fprintf(stderr, "COVnotification: dest not found!\n");
#endif
        return status;
    }
    datalink_get_my_address(&my_address);
    npdu_encode_npdu_data(
        &npdu_data, cov_subscription->flag.issueConfirmedNotifications,
        MESSAGE_PRIORITY_NORMAL);
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
        debug_fprintf(stderr, "COVnotification: Sent!\n");
#endif
    }

COV_FAILED:

    return status;
}

static void cov_lifetime_expiration_handler(
    unsigned index, uint32_t elapsed_seconds, uint32_t lifetime_seconds)
{
    if (index < MAX_COV_SUBSCRIPTIONS) {
        BACNET_COV_SUBSCRIPTION *subscription =
            Keylist_Data_Index(COV_Subscriptions, index);
        /* handle lifetime expiration */
        if (lifetime_seconds >= elapsed_seconds) {
            subscription->lifetime -= elapsed_seconds;
#if 0
            fprintf(stderr, "COVtimer: subscription[%d].lifetime=%lu\n", index,
                (unsigned long) subscription->lifetime);
#endif
        } else {
            subscription->lifetime = 0;
        }
        if (subscription->lifetime == 0) {
            /* expire the subscription */
#if PRINT_ENABLED
            debug_fprintf(
                stderr, "COVtimer: PID=%u %s %u time remaining=%u seconds\n",
                subscription->subscriberProcessIdentifier,
                bactext_object_type_name(
                    subscription->monitoredObjectIdentifier.type),
                subscription->monitoredObjectIdentifier.instance,
                subscription->lifetime);
#endif
            if (subscription->flag.issueConfirmedNotifications) {
                if (subscription->invokeID) {
                    tsm_free_invoke_id(subscription->invokeID);
                    subscription->invokeID = 0;
                }
            }
            cov_subscription_delete(index);
            cov_address_remove_unused();
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
    int index = 0;
    uint32_t lifetime_seconds = 0;
    BACNET_COV_SUBSCRIPTION *subscription = NULL;
    int list_cnt = 0;
#ifdef BAC_ROUTING
    uint16_t current_dev_id = Routed_Device_Object_Index();
    uint16_t dev_id = 0;

    if (elapsed_seconds) {
        /* handle the subscription timeouts */
        for (dev_id = 0; dev_id < Get_Num_Managed_Devices(); dev_id++) {
            Set_Routed_Device_Object_Index(dev_id);
            list_cnt = Keylist_Count(COV_Subscriptions);
            /* Iterate in reverse order due to subscription deletion */
            for (index = list_cnt - 1; index >= 0; index--) {
                subscription = Keylist_Data_Index(COV_Subscriptions, index);
                if (subscription) {
                    lifetime_seconds = subscription->lifetime;
                    if (lifetime_seconds) {
                        /* only expire COV with definite lifetimes */
                        cov_lifetime_expiration_handler(
                            index, elapsed_seconds, lifetime_seconds);
                    }
                }
            }
        }
    }
    Set_Routed_Device_Object_Index(current_dev_id);
#else
    if (elapsed_seconds) {
        /* handle the subscription timeouts */
        list_cnt = Keylist_Count(COV_Subscriptions);
        /* Iterate in reverse order due to subscription deletion */
        for (index = list_cnt - 1; index >= 0; index--) {
            subscription = Keylist_Data_Index(COV_Subscriptions, index);
            if (subscription) {
                lifetime_seconds = subscription->lifetime;
                if (lifetime_seconds) {
                    /* only expire COV with definite lifetimes */
                    cov_lifetime_expiration_handler(
                        index, elapsed_seconds, lifetime_seconds);
                }
            }
        }
    }
#endif
}

bool handler_cov_fsm(void)
{
    static int indices[MAX_NUM_DEVICES] = { 0 };
#ifdef BAC_ROUTING
    const int dev_id = Routed_Device_Object_Index();
#else
    const int dev_id = 0;
#endif

    BACNET_OBJECT_TYPE object_type = MAX_BACNET_OBJECT_TYPE;
    uint32_t object_instance = 0;
    bool status = false;
    bool send = false;
    BACNET_PROPERTY_VALUE value_list[MAX_COV_PROPERTIES] = { 0 };

    /* states for transmitting */
    typedef enum cov_fsm_state {
        COV_STATE_IDLE = 0,
        COV_STATE_MARK,
        COV_STATE_CLEAR,
        COV_STATE_FREE,
        COV_STATE_SEND
    } cov_fsm_state_t;
    static cov_fsm_state_t cov_task_states[MAX_NUM_DEVICES] = {
        COV_STATE_IDLE
    };

    int index = indices[dev_id];
    cov_fsm_state_t cov_task_state = cov_task_states[dev_id];
    BACNET_COV_SUBSCRIPTION *subscription =
        Keylist_Data_Index(COV_Subscriptions, index);
    int list_cnt = Keylist_Count(COV_Subscriptions);

    switch (cov_task_state) {
        case COV_STATE_IDLE:
            if (subscription) {
                cov_task_state = COV_STATE_MARK;
            } else {
                index++;
                if (index >= list_cnt) {
                    index = 0;
                }
            }
            break;
        case COV_STATE_MARK:
            /* mark any subscriptions where the value has changed */
            if (subscription) {
                object_type = (BACNET_OBJECT_TYPE)
                                  subscription->monitoredObjectIdentifier.type;
                object_instance =
                    subscription->monitoredObjectIdentifier.instance;
                status = Device_COV(object_type, object_instance);
                if (status) {
                    subscription->flag.send_requested = true;
#if PRINT_ENABLED
                    debug_fprintf(stderr, "COVtask: Marking...\n");
#endif
                }
            }
            index++;
            if (index >= list_cnt) {
                index = 0;
                cov_task_state = COV_STATE_CLEAR;
            }
            break;
        case COV_STATE_CLEAR:
            /* clear the COV flag after checking all subscriptions */
            if (subscription && (subscription->flag.send_requested)) {
                object_type = (BACNET_OBJECT_TYPE)
                                  subscription->monitoredObjectIdentifier.type;
                object_instance =
                    subscription->monitoredObjectIdentifier.instance;
                Device_COV_Clear(object_type, object_instance);
            }
            index++;
            if (index >= list_cnt) {
                index = 0;
                cov_task_state = COV_STATE_FREE;
            }
            break;
        case COV_STATE_FREE:
            /* confirmed notification house keeping */
            if (subscription &&
                (subscription->flag.issueConfirmedNotifications) &&
                (subscription->invokeID)) {
                if (tsm_invoke_id_free(subscription->invokeID)) {
                    subscription->invokeID = 0;
                } else if (tsm_invoke_id_failed(subscription->invokeID)) {
                    tsm_free_invoke_id(subscription->invokeID);
                    subscription->invokeID = 0;
                }
            }
            index++;
            if (index >= list_cnt) {
                index = 0;
                cov_task_state = COV_STATE_SEND;
            }
            break;
        case COV_STATE_SEND:
            /* send any COVs that are requested */
            if (subscription && (subscription->flag.send_requested)) {
                send = true;
                if (subscription->flag.issueConfirmedNotifications) {
                    if (subscription->invokeID != 0) {
                        /* already sending */
                        send = false;
                    }
                    if (!tsm_transaction_available()) {
                        /* no transactions available - can't send now */
                        send = false;
                    }
                }
                if (send) {
                    object_type =
                        (BACNET_OBJECT_TYPE)
                            subscription->monitoredObjectIdentifier.type;
                    object_instance =
                        subscription->monitoredObjectIdentifier.instance;
#if PRINT_ENABLED
                    debug_fprintf(stderr, "COVtask: Sending...\n");
#endif
                    /* configure the linked list for the two properties */
                    bacapp_property_value_list_init(
                        &value_list[0], MAX_COV_PROPERTIES);
                    status = Device_Encode_Value_List(
                        object_type, object_instance, &value_list[0]);
                    if (status) {
                        status = cov_send_request(subscription, &value_list[0]);
                    }
                    if (status) {
                        subscription->flag.send_requested = false;
                    }
                }
            }
            index++;
            if (index >= list_cnt) {
                index = 0;
                cov_task_state = COV_STATE_IDLE;
            }
            break;
        default:
            index = 0;
            cov_task_state = COV_STATE_IDLE;
            break;
    }

    indices[dev_id] = index;
    cov_task_states[dev_id] = cov_task_state;

    return (cov_task_state == COV_STATE_IDLE);
}

void handler_cov_task(void)
{
#ifdef BAC_ROUTING
    uint16_t current_dev_id = Routed_Device_Object_Index();
    uint16_t dev_id = 0;
    for (dev_id = 0; dev_id < Get_Num_Managed_Devices(); dev_id++) {
        Set_Routed_Device_Object_Index(dev_id);
        handler_cov_fsm();
    }
    Set_Routed_Device_Object_Index(current_dev_id);
#else
    handler_cov_fsm();
#endif
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
    BACNET_SUBSCRIBE_COV_DATA cov_data = { 0 };
    int len = 0;
    int pdu_len = 0;
    int npdu_len = 0;
    int apdu_len = 0;
    BACNET_NPDU_DATA npdu_data = { 0 };
    bool success = false;
    int bytes_sent = 0;
    BACNET_ADDRESS my_address = { 0 };
    bool error = false;

    /* initialize a common abort code */
    cov_data.error_code = ERROR_CODE_ABORT_SEGMENTATION_NOT_SUPPORTED;
    /* encode the NPDU portion of the packet */
    datalink_get_my_address(&my_address);
    npdu_encode_npdu_data(&npdu_data, false, service_data->priority);
    npdu_len = npdu_encode_pdu(
        &Handler_Transmit_Buffer[0], src, &my_address, &npdu_data);
    if (service_len == 0) {
        len = BACNET_STATUS_REJECT;
        cov_data.error_code = ERROR_CODE_REJECT_MISSING_REQUIRED_PARAMETER;
        debug_print("CCOV: Missing Required Parameter. Sending Reject!\n");
        error = true;
    } else if (service_data->segmented_message) {
        /* we don't support segmentation - send an abort */
        len = BACNET_STATUS_ABORT;
        debug_print("SubscribeCOV: Segmented message.  Sending Abort!\n");
        error = true;
    } else {
        len = cov_subscribe_decode_service_request(
            service_request, service_len, &cov_data);
        if (len <= 0) {
            debug_print("SubscribeCOV: Unable to decode Request!\n");
        }
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
                debug_print("SubscribeCOV: Sending Simple Ack!\n");
            } else {
                len = BACNET_STATUS_ERROR;
                error = true;
                debug_print("SubscribeCOV: Sending Error!\n");
            }
        }
    }
    /* Error? */
    if (error) {
        if (len == BACNET_STATUS_ABORT) {
            apdu_len = abort_encode_apdu(
                &Handler_Transmit_Buffer[npdu_len], service_data->invoke_id,
                abort_convert_error_code(cov_data.error_code), true);
            debug_print("SubscribeCOV: Sending Abort!\n");
        } else if (len == BACNET_STATUS_ERROR) {
            apdu_len = bacerror_encode_apdu(
                &Handler_Transmit_Buffer[npdu_len], service_data->invoke_id,
                SERVICE_CONFIRMED_SUBSCRIBE_COV, cov_data.error_class,
                cov_data.error_code);
            debug_print("SubscribeCOV: Sending Error!\n");
        } else if (len == BACNET_STATUS_REJECT) {
            apdu_len = reject_encode_apdu(
                &Handler_Transmit_Buffer[npdu_len], service_data->invoke_id,
                reject_convert_error_code(cov_data.error_code));
            debug_print("SubscribeCOV: Sending Reject!\n");
        }
    }
    pdu_len = npdu_len + apdu_len;
    bytes_sent = datalink_send_pdu(
        src, &npdu_data, &Handler_Transmit_Buffer[0], pdu_len);
    if (bytes_sent <= 0) {
        debug_perror("SubscribeCOV: Failed to send PDU");
    }

    return;
}
