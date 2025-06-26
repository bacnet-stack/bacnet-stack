/**************************************************************************
 *
 * Copyright (C) 2005 Steve Karg
 * Corrections by Ferran Arumi, 2007, Barcelona, Spain
 *
 * SPDX-License-Identifier: GPL-2.0-or-later WITH GCC-exception-2.0
 *
 *********************************************************************/
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
/* BACnet Stack defines - first */
#include "bacnet/bacdef.h"
/* BACnet Stack API */
#include "bacnet/apdu.h"
#include "bacnet/bacaddr.h"
#include "bacnet/bacdcode.h"
#include "bacnet/basic/tsm/tsm.h"
#include "bacnet/datalink/datalink.h"
#include "bacnet/basic/services.h"
#include "bacnet/basic/binding/address.h"

/** @file tsm.c  BACnet Transaction State Machine operations  */
/* FIXME: modify basic service handlers to use TSM rather than this buffer! */
uint8_t Handler_Transmit_Buffer[MAX_PDU];

#if (MAX_TSM_TRANSACTIONS)
/* Really only needed for segmented messages */
/* and a little for sending confirmed messages */
/* If we are only a server and only initiate broadcasts, */
/* then we don't need a TSM layer. */

/* FIXME: not coded for segmentation */

/* declare space for the TSM transactions, and set it up in the init. */
/* table rules: an Invoke ID = 0 is an unused spot in the table */
static BACNET_TSM_DATA TSM_List[MAX_TSM_TRANSACTIONS];

/* invoke ID for incrementing between subsequent calls. */
static uint8_t Current_Invoke_ID = 1;

static tsm_timeout_function Timeout_Function;

void tsm_set_timeout_handler(tsm_timeout_function pFunction)
{
    Timeout_Function = pFunction;
}

/** Find the given Invoke-Id in the list and
 *  return the index.
 *
 * @param invokeID  Invoke Id
 *
 * @return Index of the id or MAX_TSM_TRANSACTIONS
 *         if not found
 */
static uint8_t tsm_find_invokeID_index(uint8_t invokeID)
{
    unsigned i = 0; /* counter */
    uint8_t index = MAX_TSM_TRANSACTIONS; /* return value */

    const BACNET_TSM_DATA *plist = TSM_List;

    for (i = 0; i < MAX_TSM_TRANSACTIONS; i++, plist++) {
        if (plist->InvokeID == invokeID) {
            index = (uint8_t)i;
            break;
        }
    }

    return index;
}

/** Find the first free index in the TSM table.
 *
 * @return Index of the id or MAX_TSM_TRANSACTIONS
 *         if no entry is free.
 */
static uint8_t tsm_find_first_free_index(void)
{
    unsigned i = 0; /* counter */
    uint8_t index = MAX_TSM_TRANSACTIONS; /* return value */

    const BACNET_TSM_DATA *plist = TSM_List;

    for (i = 0; i < MAX_TSM_TRANSACTIONS; i++, plist++) {
        if (plist->InvokeID == 0) {
            index = (uint8_t)i;
            break;
        }
    }

    return index;
}

/** Check if space for transactions is available.
 *
 * @return true/false
 */
bool tsm_transaction_available(void)
{
    bool status = false; /* return value */
    unsigned i = 0; /* counter */

    const BACNET_TSM_DATA *plist = TSM_List;

    for (i = 0; i < MAX_TSM_TRANSACTIONS; i++, plist++) {
        if (plist->InvokeID == 0) {
            /* one is available! */
            status = true;
            break;
        }
    }

    return status;
}

/** Return the count of idle transaction.
 *
 * @return Count of idle transaction.
 */
uint8_t tsm_transaction_idle_count(void)
{
    uint8_t count = 0; /* return value */
    unsigned i = 0; /* counter */

    const BACNET_TSM_DATA *plist = TSM_List;

    for (i = 0; i < MAX_TSM_TRANSACTIONS; i++, plist++) {
        if ((plist->InvokeID == 0) && (plist->state == TSM_STATE_IDLE)) {
            /* one is available! */
            count++;
        }
    }

    return count;
}

/**
 * Sets the current invokeID.
 *
 * @param invokeID  Invoke ID
 */
void tsm_invokeID_set(uint8_t invokeID)
{
    if (invokeID == 0) {
        invokeID = 1;
    }
    Current_Invoke_ID = invokeID;
}

/** Gets the next free invokeID,
 * and reserves a spot in the table
 * returns 0 if none are available.
 *
 * @return free invoke ID
 */
uint8_t tsm_next_free_invokeID(void)
{
    uint8_t index = 0;
    uint8_t invokeID = 0;
    bool found = false;
    BACNET_TSM_DATA *plist = NULL;

    /* Is there even space available? */
    if (tsm_transaction_available()) {
        while (!found) {
            index = tsm_find_invokeID_index(Current_Invoke_ID);
            if (index == MAX_TSM_TRANSACTIONS) {
                /* Not found, so this invokeID is not used */
                found = true;
                /* set this id into the table */
                index = tsm_find_first_free_index();
                if (index != MAX_TSM_TRANSACTIONS) {
                    plist = &TSM_List[index];
                    plist->InvokeID = invokeID = Current_Invoke_ID;
                    plist->state = TSM_STATE_IDLE;
                    plist->RequestTimer = apdu_timeout();
                    /* update for the next call or check */
                    Current_Invoke_ID++;
                    /* skip zero - we treat that internally as invalid or no
                     * free */
                    if (Current_Invoke_ID == 0) {
                        Current_Invoke_ID = 1;
                    }
                }
            } else {
                /* found! This invokeID is already used */
                /* try next one */
                Current_Invoke_ID++;
                /* skip zero - we treat that internally as invalid or no free */
                if (Current_Invoke_ID == 0) {
                    Current_Invoke_ID = 1;
                }
            }
        }
    }

    return invokeID;
}

/** Set for an unsegmented transaction
 *  the state to await confirmation.
 *
 * @param invokeID  Invoke-ID
 * @param dest  Pointer to the BACnet destination address.
 * @param ndpu_data  Pointer to the NPDU structure.
 * @param apdu  Pointer to the received message.
 * @param apdu_len  Bytes valid in the received message.
 */
void tsm_set_confirmed_unsegmented_transaction(
    uint8_t invokeID,
    const BACNET_ADDRESS *dest,
    const BACNET_NPDU_DATA *ndpu_data,
    const uint8_t *apdu,
    uint16_t apdu_len)
{
    uint16_t j = 0;
    uint8_t index;
    BACNET_TSM_DATA *plist;

    if (invokeID && ndpu_data && apdu && (apdu_len > 0)) {
        index = tsm_find_invokeID_index(invokeID);
        if (index < MAX_TSM_TRANSACTIONS) {
            plist = &TSM_List[index];
            /* SendConfirmedUnsegmented */
            plist->state = TSM_STATE_AWAIT_CONFIRMATION;
            plist->RetryCount = 0;
            /* start the timer */
            plist->RequestTimer = apdu_timeout();
            /* copy the data */
            for (j = 0; j < apdu_len; j++) {
                plist->apdu[j] = apdu[j];
            }
            plist->apdu_len = apdu_len;
            npdu_copy_data(&plist->npdu_data, ndpu_data);
            bacnet_address_copy(&plist->dest, dest);
        }
    }

    return;
}

/** Used to retrieve the transaction payload. Used
 *  if we wanted to find out what we sent (i.e. when
 *  we get an ack).
 *
 * @param invokeID  Invoke-ID
 * @param dest  Pointer to the BACnet destination address.
 * @param ndpu_data  Pointer to the NPDU structure.
 * @param apdu  Pointer to the received message.
 * @param apdu_len  Pointer to a variable, that takes
 *                  the count of bytes valid in the
 *                  received message.
 */
bool tsm_get_transaction_pdu(
    uint8_t invokeID,
    BACNET_ADDRESS *dest,
    BACNET_NPDU_DATA *ndpu_data,
    uint8_t *apdu,
    uint16_t *apdu_len)
{
    uint16_t j = 0;
    uint8_t index;
    bool found = false;
    BACNET_TSM_DATA *plist;

    if (invokeID && apdu && ndpu_data && apdu_len) {
        index = tsm_find_invokeID_index(invokeID);
        /* how much checking is needed?  state?  dest match? just invokeID? */
        if (index < MAX_TSM_TRANSACTIONS) {
            /* FIXME: we may want to free the transaction so it doesn't timeout
             */
            /* retrieve the transaction */
            plist = &TSM_List[index];
            *apdu_len = (uint16_t)plist->apdu_len;
            if (*apdu_len > MAX_PDU) {
                *apdu_len = MAX_PDU;
            }
            for (j = 0; j < *apdu_len; j++) {
                apdu[j] = plist->apdu[j];
            }
            npdu_copy_data(ndpu_data, &plist->npdu_data);
            bacnet_address_copy(dest, &plist->dest);
            found = true;
        }
    }

    return found;
}

/** Called once a millisecond or slower.
 *  This function calls the handler for a
 *  timeout 'Timeout_Function', if necessary.
 *
 * @param milliseconds - Count of milliseconds passed, since the last call.
 */
void tsm_timer_milliseconds(uint16_t milliseconds)
{
    unsigned i = 0; /* counter */

    BACNET_TSM_DATA *plist = &TSM_List[0];

    for (i = 0; i < MAX_TSM_TRANSACTIONS; i++, plist++) {
        if (plist->state == TSM_STATE_AWAIT_CONFIRMATION) {
            if (plist->RequestTimer > milliseconds) {
                plist->RequestTimer -= milliseconds;
            } else {
                plist->RequestTimer = 0;
            }
            /* AWAIT_CONFIRMATION */
            if (plist->RequestTimer == 0) {
                if (plist->RetryCount < apdu_retries()) {
                    plist->RequestTimer = apdu_timeout();
                    plist->RetryCount++;
                    printf(
                        "invoke-id[%u] Retry after %ums",
                        (unsigned)plist->InvokeID,
                        (unsigned)plist->RequestTimer);
                    datalink_send_pdu(
                        &plist->dest, &plist->npdu_data, &plist->apdu[0],
                        plist->apdu_len);
                } else {
                    /* note: the invoke id has not been cleared yet
                       and this indicates a failed message:
                       IDLE and a valid invoke id */
                    plist->state = TSM_STATE_IDLE;
                    if (plist->InvokeID != 0) {
                        if (Timeout_Function) {
                            Timeout_Function(plist->InvokeID);
                        }
                    }
                }
            }
        }
    }
}

/** Frees the invokeID and sets its state to IDLE
 *
 * @param invokeID  Invoke-ID
 */
void tsm_free_invoke_id(uint8_t invokeID)
{
    uint8_t index;
    BACNET_TSM_DATA *plist;

    index = tsm_find_invokeID_index(invokeID);
    if (index < MAX_TSM_TRANSACTIONS) {
        plist = &TSM_List[index];
        plist->state = TSM_STATE_IDLE;
        plist->InvokeID = 0;
    }
}

/** Check if the invoke ID has been made free by the Transaction State Machine.
 * @param invokeID [in] The invokeID to be checked, normally of last message
 * sent.
 * @return True if it is free (done with), False if still pending in the TSM.
 */
bool tsm_invoke_id_free(uint8_t invokeID)
{
    bool status = true;
    uint8_t index;

    index = tsm_find_invokeID_index(invokeID);
    if (index < MAX_TSM_TRANSACTIONS) {
        status = false;
    }

    return status;
}

/** See if we failed get a confirmation for the message associated
 *  with this invoke ID.
 * @param invokeID [in] The invokeID to be checked, normally of last message
 * sent.
 * @return True if already failed, False if done or segmented or still waiting
 *         for a confirmation.
 */
bool tsm_invoke_id_failed(uint8_t invokeID)
{
    bool status = false;
    uint8_t index;

    index = tsm_find_invokeID_index(invokeID);
    if (index < MAX_TSM_TRANSACTIONS) {
        /* a valid invoke ID and the state is IDLE is a
           message that failed to confirm */
        if (TSM_List[index].state == TSM_STATE_IDLE) {
            status = true;
        }
    }

    return status;
}
#endif
