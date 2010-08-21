/*####COPYRIGHTBEGIN####
 -------------------------------------------
 Copyright (C) 2005 Steve Karg
 Corrections by Ferran Arumi, 2007, Barcelona, Spain

 This program is free software; you can redistribute it and/or
 modify it under the terms of the GNU General Public License
 as published by the Free Software Foundation; either version 2
 of the License, or (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program; if not, write to:
 The Free Software Foundation, Inc.
 59 Temple Place - Suite 330
 Boston, MA  02111-1307, USA.

 As a special exception, if other files instantiate templates or
 use macros or inline functions from this file, or you compile
 this file and link it with other works to produce a work based
 on this file, this file does not by itself cause the resulting
 work to be covered by the GNU General Public License. However
 the source code for this file must still be made available in
 accordance with section (3) of the GNU General Public License.

 This exception does not invalidate any other reasons why a work
 based on this file might be covered by the GNU General Public
 License.
 -------------------------------------------
####COPYRIGHTEND####*/
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include "bits.h"
#include "apdu.h"
#include "bacdef.h"
#include "bacdcode.h"
#include "bacenum.h"
#include "tsm.h"
#include "config.h"
#include "datalink.h"
#include "handlers.h"
#include "address.h"
#include "bacaddr.h"
#include "abort.h"
#include "session.h"
#include "segmentack.h"

#if (MAX_TSM_TRANSACTIONS)
/* Transaction State Machine */
/* Really only needed for segmented messages */
/* and a little for sending confirmed messages */
/* If we are only a server and only initiate broadcasts, */
/* then we don't need a TSM layer. */

/* returns MAX_TSM_TRANSACTIONS if not found */
static uint8_t tsm_find_invokeID_index(
    struct bacnet_session_object *session_object,
    uint8_t invokeID)
{
    int index_0_N =
        ((int) MAX_TSM_TRANSACTIONS + (int) invokeID -
        1) % (int) MAX_TSM_TRANSACTIONS;
    uint8_t index = MAX_TSM_TRANSACTIONS;

    /* TSM transaction is activated when (tsm[id-1].InvokeID == id) */
    /* we could as well use a boolean flag */
    if (session_object->TSM_List[index_0_N].InvokeID == invokeID)
        index = (uint8_t) index_0_N;
    return index;
}

static uint8_t tsm_allocate_first_free_index(
    struct bacnet_session_object *session_object,
    uint8_t invokeIdHint        /* id hint : 1..MAX_TSM_TRANSACTIONS */
    )
{
    uint8_t i = 0;      /* counter */
    uint8_t index = MAX_TSM_TRANSACTIONS;       /* return value */

    bacnet_session_lock(session_object);
    for (i = invokeIdHint - 1; i < MAX_TSM_TRANSACTIONS; i++) {
        if (session_object->TSM_List[i].InvokeID == 0) {
            session_object->TSM_List[i].InvokeID = i + 1;       /* Fixed symbol */
            session_object->TSM_List[i].state = TSM_STATE_ALLOCATED;
            index = i;
            bacnet_session_log(session_object, 90,
                "SEARCH: Allocated invokeID", NULL, i + 1);
            break;
        }
    }
    /* Continue if not found */
    if (index == MAX_TSM_TRANSACTIONS)
        for (i = 0; i < invokeIdHint - 1; i++) {
            if (session_object->TSM_List[i].InvokeID == 0) {
                session_object->TSM_List[i].InvokeID = i + 1;   /* Fixed symbol */
                session_object->TSM_List[i].state = TSM_STATE_ALLOCATED;
                index = i;
                bacnet_session_log(session_object, 90,
                    "SEARCH: Allocated invokeID", NULL, i + 1);
                break;
            }
        }
    bacnet_session_unlock(session_object);

    return index;
}


uint8_t tsm_transaction_idle_count(
    struct bacnet_session_object * session_object)
{
    uint8_t count = 0;  /* return value */
    unsigned i = 0;     /* counter */

    for (i = 0; i < MAX_TSM_TRANSACTIONS; i++) {
        if ((session_object->TSM_List[i].InvokeID == 0) &&
            (session_object->TSM_List[i].state == TSM_STATE_IDLE)) {
            /* one is available! */
            count++;
        }
    }

    return count;
}

/* changes invokeID */
void tsm_invokeID_set(
    struct bacnet_session_object *session_object,
    uint8_t invokeID)
{
    session_object->TSM_Current_InvokeID = invokeID--;
}

/* gets the next free invokeID,
   and reserves a spot in the table
   returns 0 if none are available */
uint8_t tsm_next_free_invokeID(
    struct bacnet_session_object *session_object)
{
    uint8_t my_current_invokeID;
    uint8_t index = 0;
    uint8_t invokeID = 0;
    bool found = false;

    bacnet_session_log(session_object, 90,
        "ALLOC: Entering tsm_next_free_invokeID()", NULL, 0);


    while (!found) {
        /* get next invokeID for this session */
        while (!(my_current_invokeID = (++(session_object->TSM_Current_InvokeID)) % (int) (MAX_TSM_TRANSACTIONS + 1))) {        /* numbers 1 to MAX_TSM_TRANSACTIONS, 0 excluded */
            /* loop once more : step over "0" */
        }

        bacnet_session_log(session_object, 90, "ALLOC: Search free invokeID",
            NULL, my_current_invokeID);
        /* test next "invokeID", ok if it is free, otherwise try next invokeId */
        index =
            tsm_allocate_first_free_index(session_object, my_current_invokeID);
        /* we allocated a free slot with the current invokeID */
        if (index != MAX_TSM_TRANSACTIONS) {
            bacnet_session_log(session_object, 90,
                "ALLOC: Obtained a new invokeID", NULL,
                session_object->TSM_List[index].InvokeID);
            found = true;
            invokeID = session_object->TSM_List[index].InvokeID;
            assert(invokeID > 0);
            session_object->TSM_List[index].state = TSM_STATE_IDLE;
            session_object->TSM_List[index].RequestTimer =
                apdu_timeout(session_object);
        } else {
            bacnet_session_log(session_object, 90,
                "ALLOC: Could not obtained an invokeID, sleeping.", NULL, 0);
            /* No invokeID available, wait some milliseconds, or wait until next signal, */
            /* and try again later.. */
            if (bacnet_session_can_wait(session_object))
                bacnet_session_wait(session_object, 2);
            else
                break;  /* cannot loop indefinitely : we stop without an invokeID */
        }
    }
    return invokeID;
}

void tsm_set_confirmed_unsegmented_transaction(
    struct bacnet_session_object *session_object,
    uint8_t invokeID,
    BACNET_ADDRESS * dest,
    BACNET_NPDU_DATA * ndpu_data,
    uint8_t * apdu,
    uint16_t apdu_len)
{
    uint16_t j = 0;
    uint8_t index;

    if (invokeID) {
        index = tsm_find_invokeID_index(session_object, invokeID);
        if (index < MAX_TSM_TRANSACTIONS) {
            /* assign the transaction */
            session_object->TSM_List[index].state =
                TSM_STATE_AWAIT_CONFIRMATION;
            session_object->TSM_List[index].RetryCount =
                apdu_retries(session_object);
            /* start the timer */
            session_object->TSM_List[index].RequestTimer =
                apdu_timeout(session_object);
            /* copy the data */
            for (j = 0; j < apdu_len; j++) {
                session_object->TSM_List[index].apdu[j] = apdu[j];
            }
            session_object->TSM_List[index].apdu_len = apdu_len;
            npdu_copy_data(&session_object->TSM_List[index].npdu_data,
                ndpu_data);
            bacnet_address_copy(&session_object->TSM_List[index].dest, dest);

            bacnet_session_log(session_object, 90, "MSG: marking ID USED",
                &session_object->TSM_List[index].dest,
                session_object->TSM_List[index].InvokeID);
        }
    }

    return;
}

/* Free allocated blob data */
void free_blob(
    BACNET_TSM_DATA * data)
{
    if (data->apdu_blob)
        free(data->apdu_blob);
    data->apdu_blob = NULL;
    data->apdu_blob_allocated = 0;
    data->apdu_blob_size = 0;
}

/* keeps allocated blob data, but reset data & current size */
void reset_blob(
    BACNET_TSM_DATA * data)
{
    data->apdu_blob_size = 0;
}

/* allocate new data if necessary, keeps existing bytes */
void ensure_extra_blob_size(
    BACNET_TSM_DATA * data,
    uint32_t allocation_unit)
{
    if (!allocation_unit)       /* NOP */
        return;
    /* allocation needed ? */
    if ((!data->apdu_blob)
        || (!data->apdu_blob_allocated)
        || ((allocation_unit + data->apdu_blob_size) >
            data->apdu_blob_allocated)) {
        /* stupid idiot allocation algorithm : space allocated shall augment exponentially */
        /* (nb: here there may be extra space remaining) */
        uint8_t *apdu_new_blob =
            calloc(1, data->apdu_blob_allocated + allocation_unit);
        /* recopy old data */
        if (data->apdu_blob_size)
            memcpy(apdu_new_blob, data->apdu_blob, data->apdu_blob_size);
        /* new values */
        if (data->apdu_blob)
            free(data->apdu_blob);
        data->apdu_blob = apdu_new_blob;
        data->apdu_blob_allocated =
            data->apdu_blob_allocated + allocation_unit;
    }
}

/* add new data to current blob (allocate extra space if necessary) */
void add_blob_data(
    BACNET_TSM_DATA * data,
    uint8_t * bdata,
    uint32_t data_len)
{
    ensure_extra_blob_size(data, data_len);
    memcpy(&data->apdu_blob[data->apdu_blob_size], bdata, data_len);
    data->apdu_blob_size += data_len;
}

/* gets current blob data */
uint8_t *get_blob_data(
    BACNET_TSM_DATA * data,
    uint32_t * data_len)
{
    *data_len = data->apdu_blob_size;
    return data->apdu_blob;
}

void segmentack_pdu_send(
    struct bacnet_session_object *sess,
    BACNET_ADDRESS * dest,
    bool negativeack,
    bool server,
    uint8_t invoke_id,
    uint8_t sequence_number,
    uint8_t actual_window_size)
{
    int pdu_len = 0;
    BACNET_NPDU_DATA npdu_data;
    int bytes_sent;
    BACNET_ADDRESS my_address;
    int apdu_len = 0;
    int npdu_len = 0;
    uint8_t Handler_Transmit_Buffer[MAX_PDU] = { 0 };

    sess->datalink_get_my_address(sess, &my_address);
    npdu_encode_npdu_data(&npdu_data, false, MESSAGE_PRIORITY_NORMAL);
    npdu_len =
        npdu_encode_pdu(&Handler_Transmit_Buffer[0], dest, &my_address,
        &npdu_data);

    apdu_len =
        segmentack_encode_apdu(&Handler_Transmit_Buffer[npdu_len], negativeack,
        server, invoke_id, sequence_number, actual_window_size);

    pdu_len = apdu_len + npdu_len;
    bytes_sent =
        sess->datalink_send_pdu(sess, dest, &npdu_data,
        &Handler_Transmit_Buffer[0], pdu_len);
}

/* send an Abort-PDU message because of incorrect segment/PDU received */
void abort_pdu_send(
    struct bacnet_session_object *sess,
    uint8_t invoke_id,
    BACNET_ADDRESS * dest,
    uint8_t reason)
{
    int pdu_len = 0;
    BACNET_NPDU_DATA npdu_data;
    int bytes_sent;
    BACNET_ADDRESS my_address;
    int apdu_len = 0;
    int npdu_len = 0;
    uint8_t Handler_Transmit_Buffer[MAX_PDU] = { 0 };

    sess->datalink_get_my_address(sess, &my_address);
    npdu_encode_npdu_data(&npdu_data, false, MESSAGE_PRIORITY_NORMAL);
    npdu_len =
        npdu_encode_pdu(&Handler_Transmit_Buffer[0], dest, &my_address,
        &npdu_data);
    apdu_len =
        abort_encode_apdu(&Handler_Transmit_Buffer[npdu_len], invoke_id,
        reason, false);
    pdu_len = apdu_len + npdu_len;
    bytes_sent =
        sess->datalink_send_pdu(sess, dest, &npdu_data,
        &Handler_Transmit_Buffer[0], pdu_len);
}

bool tsm_set_segmentedcomplexack_received(
    struct bacnet_session_object *session_object,
    BACNET_ADDRESS * src,
    BACNET_CONFIRMED_SERVICE_ACK_DATA * service_data,
    uint8_t ** pservice_request,        /* IN/OUT */
    uint32_t * pservice_request_len     /* IN/OUT */
    )
{
    uint8_t index;
    uint8_t *service_request = *pservice_request;
    uint32_t service_request_len = *pservice_request_len;
    bool result = false;
    bool ack_needed = false;

    if (service_data->invoke_id) {
        index =
            tsm_find_invokeID_index(session_object, service_data->invoke_id);
        if (index < MAX_TSM_TRANSACTIONS) {
            /* ASHRAE 135-2008 - The Application Layer, p. 29 - SegmentedComplexACK_Received */
            /* The first segment of a segmented ACK response was received : initialize specific status for segmentation */
            if (session_object->TSM_List[index].state ==
                TSM_STATE_AWAIT_CONFIRMATION) {
                /* assign the transaction */
                session_object->TSM_List[index].state =
                    TSM_STATE_SEGMENTED_CONFIRMATION;
                /* First time : Compute Actual WindowSize */
                /* we automatically accept the proposed window size */
                session_object->TSM_List[index].ActualWindowSize =
                    session_object->TSM_List[index].ProposedWindowSize =
                    service_data->proposed_window_number;
                /* Init sequence numbers */
                session_object->TSM_List[index].InitialSequenceNumber = 0;
                session_object->TSM_List[index].LastSequenceNumber = 0;
                /* resets counters */
                session_object->TSM_List[index].RetryCount = 0;
                session_object->TSM_List[index].SegmentRetryCount = 0;
                session_object->TSM_List[index].ReceivedSegmentsCount = 1;
                /* stop unsegmented timer */
                session_object->TSM_List[index].RequestTimer = 0;       /* unused */
                /* start the segmented timer */
                session_object->TSM_List[index].SegmentTimer =
                    apdu_timeout(session_object);
                /* reset memorized data */
                reset_blob(&session_object->TSM_List[index]);
                /* Test : sequence number MUST be 0 */
                /* UnexpectedPDU_Received */
                if (service_data->sequence_number != 0) {
                    /* ERROR : Abort */
                    abort_pdu_send(session_object, service_data->invoke_id,
                        src, ABORT_REASON_INVALID_APDU_IN_THIS_STATE);
                    /* Release data */
                    free_blob(&session_object->TSM_List[index]);
                    /* Stop timer, issue a NUNITDATA.request with expecting reply = FALSE */
                    /* to transmit a BACnetAbortPDU with server = FALSE ; Enter IDLE state */
                    session_object->TSM_List[index].state = TSM_STATE_IDLE;
                } else {
                    /* Okay : memorize data */
                    add_blob_data(&session_object->TSM_List[index],
                        service_request, service_request_len);
                    /* We ACK the first segment of the segmented message */
                    segmentack_pdu_send(session_object, src, false, false,
                        service_data->invoke_id,
                        session_object->TSM_List[index].LastSequenceNumber,
                        session_object->TSM_List[index].ActualWindowSize);
                }

            } else
                /* ASHRAE 135-2008 - The Application Layer, p. 30 : */
                /* NewSegmentReceived */
                /* LastSegmentOfGroupReceived */
                /* LastSegmentOfComplexACK_Received */
                /* */
                /* A segment of a segmented ACK response was received :  */
            if (session_object->TSM_List[index].state ==
                TSM_STATE_SEGMENTED_CONFIRMATION) {
                /* reset the segment timer */
                session_object->TSM_List[index].RequestTimer = 0;       /* unused */
                session_object->TSM_List[index].SegmentTimer =
                    apdu_timeout(session_object);

                /* Sequence number MUST be (LastSequenceNumber+1 modulo 256) */
                if (service_data->sequence_number !=
                    (uint8_t) (session_object->
                        TSM_List[index].LastSequenceNumber + 1)) {
                    /* Recoverable Error: SegmentReceivedOutOfOrder */
                    /* ACK of last segment correctly received. */
                    segmentack_pdu_send(session_object, src, true, false,
                        service_data->invoke_id,
                        session_object->TSM_List[index].LastSequenceNumber,
                        session_object->TSM_List[index].ActualWindowSize);
                } else {

                    /* Count maximum segments */
                    if (++session_object->
                        TSM_List[index].ReceivedSegmentsCount >
                        MAX_SEGMENTS_ACCEPTED) {
                        /* ABORT: SegmentReceivedOutOfSpace */
                        abort_pdu_send(session_object, service_data->invoke_id,
                            src, ABORT_REASON_BUFFER_OVERFLOW);
                        /* Release data */
                        free_blob(&session_object->TSM_List[index]);
                        /* Enter IDLE state */
                        session_object->TSM_List[index].state = TSM_STATE_IDLE;
                    } else {
                        /* NewSegmentReceived */
                        session_object->TSM_List[index].LastSequenceNumber =
                            service_data->sequence_number;
                        add_blob_data(&session_object->TSM_List[index],
                            service_request, service_request_len);

                        /* LastSegmentOfComplexACK_Received */
                        if (service_data->sequence_number ==
                            (uint8_t) (session_object->
                                TSM_List[index].InitialSequenceNumber +
                                session_object->
                                TSM_List[index].ActualWindowSize)) {
                            ack_needed = true;
                            session_object->
                                TSM_List[index].InitialSequenceNumber =
                                service_data->sequence_number;
                        }
                        /* LastSegmentOfComplexACK_Received */
                        if (!service_data->more_follows) {
                            /* Resulting segment data */
                            *pservice_request =
                                get_blob_data(&session_object->TSM_List[index],
                                pservice_request_len);
                            result = true;
                            ack_needed = true;
                        }
                        /* LastSegmentOfComplexACK_Received or LastSegmentOfGroupReceived */
                        if (ack_needed)
                            /* ACK received segment */
                            segmentack_pdu_send(session_object, src, false,
                                false, service_data->invoke_id,
                                session_object->
                                TSM_List[index].LastSequenceNumber,
                                session_object->
                                TSM_List[index].ActualWindowSize);
                    }
                }
            } else {
                /* ERROR: Abort */
                abort_pdu_send(session_object, service_data->invoke_id, src,
                    ABORT_REASON_INVALID_APDU_IN_THIS_STATE);
            }
        }
    }
    return result;
}


/* used to retrieve the transaction payload */
/* if we wanted to find out what we sent (i.e. when we get an ack) */
bool tsm_get_transaction_pdu(
    struct bacnet_session_object * session_object,
    uint8_t invokeID,
    BACNET_ADDRESS * dest,
    BACNET_NPDU_DATA * ndpu_data,
    uint8_t * apdu,
    uint16_t * apdu_len)
{
    uint16_t j = 0;
    uint8_t index;
    bool found = false;

    if (invokeID) {
        index = tsm_find_invokeID_index(session_object, invokeID);
        /* how much checking is needed?  state?  dest match? just invokeID? */
        if (index < MAX_TSM_TRANSACTIONS) {
            /* FIXME: we may want to free the transaction so it doesn't timeout */
            /* retrieve the transaction */
            /* FIXME: bounds check the pdu_len? */
            *apdu_len = session_object->TSM_List[index].apdu_len;
            for (j = 0; j < *apdu_len; j++) {
                apdu[j] = session_object->TSM_List[index].apdu[j];
            }
            npdu_copy_data(ndpu_data,
                &session_object->TSM_List[index].npdu_data);
            bacnet_address_copy(dest, &session_object->TSM_List[index].dest);
            found = true;
        }
    }

    return found;
}

/* called once a millisecond or slower */
void tsm_timer_milliseconds(
    struct bacnet_session_object *session_object,
    uint16_t milliseconds)
{
    unsigned i = 0;     /* counter */
    for (i = 0; i < MAX_TSM_TRANSACTIONS; i++) {
        if (session_object->TSM_List[i].state == TSM_STATE_AWAIT_CONFIRMATION) {
            if (session_object->TSM_List[i].RequestTimer > milliseconds)
                session_object->TSM_List[i].RequestTimer -= milliseconds;
            else
                session_object->TSM_List[i].RequestTimer = 0;
            /* timeout.  retry? */
            if (session_object->TSM_List[i].RequestTimer == 0) {
                session_object->TSM_List[i].RetryCount--;
                session_object->TSM_List[i].RequestTimer =
                    apdu_timeout(session_object);
                if (session_object->TSM_List[i].RetryCount) {
                    session_object->datalink_send_pdu(session_object,
                        &session_object->TSM_List[i].dest,
                        &session_object->TSM_List[i].npdu_data,
                        &session_object->TSM_List[i].apdu[0],
                        session_object->TSM_List[i].apdu_len);
                } else {
                    /* note: the invoke id has not been cleared yet
                       and this indicates a failed message:
                       IDLE and a valid invoke id */
                    session_object->TSM_List[i].state = TSM_STATE_IDLE;
                    bacnet_session_log(session_object, 90,
                        "TIMER: marking ID IDLE (out of time)",
                        &session_object->TSM_List[i].dest,
                        session_object->TSM_List[i].InvokeID);
                }
            }
        }
        if (session_object->TSM_List[i].state ==
            TSM_STATE_SEGMENTED_CONFIRMATION) {
            /* RequestTimer stopped in this state */
            if (session_object->TSM_List[i].SegmentTimer > milliseconds)
                session_object->TSM_List[i].SegmentTimer -= milliseconds;
            else
                session_object->TSM_List[i].SegmentTimer = 0;
            /* timeout.  retry? */
            if (session_object->TSM_List[i].SegmentTimer == 0) {
                /* note: the invoke id has not been cleared yet
                   and this indicates a failed message:
                   IDLE and a valid invoke id */
                session_object->TSM_List[i].state = TSM_STATE_IDLE;
                /* Release segmented data on error */
                free_blob(&session_object->TSM_List[i]);
            }
        }
    }
}

/* frees the invokeID and sets its state to IDLE */
void tsm_free_invoke_id_check(
    struct bacnet_session_object *session_object,
    uint8_t invokeID,
    BACNET_ADDRESS * peer_address,
    bool cleanup)
{
    uint8_t index;

    bacnet_session_log(session_object, 90, "FREE: Trying to free ID",
        peer_address, invokeID);

    bacnet_session_lock(session_object);

    bacnet_session_log(session_object, 90, "FREE: Freeing ID", peer_address,
        invokeID);

    index = tsm_find_invokeID_index(session_object, invokeID);
    if ((index < MAX_TSM_TRANSACTIONS) && (!peer_address ||
            address_match(peer_address,
                &session_object->TSM_List[index].dest))) {
        bacnet_session_log(session_object, 90,
            "FREE: Freeing ID (matched & active)", peer_address, invokeID);
        /* check "double-free" cases */
        assert(session_object->TSM_List[index].state != TSM_STATE_ALLOCATED);
        session_object->TSM_List[index].state = TSM_STATE_IDLE;
        session_object->TSM_List[index].InvokeID = 0;
        if (cleanup)
            /* Release segmented data */
            free_blob(&session_object->TSM_List[index]);
    } else
        /* assert on unmatched peer address */
    if ((index < MAX_TSM_TRANSACTIONS)) {
        bacnet_session_log(session_object, 90,
            "FREE: Freeing ID (active but wrong address)", peer_address,
            invokeID);
        assert(!peer_address);
    }
    bacnet_session_unlock(session_object);
    /* Signal : we just freed an invokeID ! */
    bacnet_session_signal(session_object);
}

/* check if the invoke ID has been made free */
bool tsm_invoke_id_free(
    struct bacnet_session_object *session_object,
    uint8_t invokeID)
{
    bool status = true;
    uint8_t index;

    bacnet_session_lock(session_object);
    index = tsm_find_invokeID_index(session_object, invokeID);
    if (index < MAX_TSM_TRANSACTIONS)
        status = false;
    bacnet_session_unlock(session_object);

    return status;
}

/* see if the invoke ID has failed get a confirmation */
bool tsm_invoke_id_failed(
    struct bacnet_session_object * session_object,
    uint8_t invokeID)
{
    bool status = false;
    uint8_t index;

    bacnet_session_lock(session_object);

    index = tsm_find_invokeID_index(session_object, invokeID);
    if (index < MAX_TSM_TRANSACTIONS) {
        /* a valid invoke ID and the state is IDLE is a
           message that failed to confirm */
        if (session_object->TSM_List[index].state == TSM_STATE_IDLE)
            status = true;
    }
    bacnet_session_unlock(session_object);

    return status;
}


#ifdef TEST
#include <assert.h>
#include <string.h>
#include "ctest.h"

/* flag to send an I-Am */
bool I_Am_Request = true;

/* dummy function stubs */
int datalink_send_pdu(
    BACNET_ADDRESS * dest,
    BACNET_NPDU_DATA * npdu_data,
    uint8_t * pdu,
    unsigned pdu_len)
{
    (void) dest;
    (void) npdu_data;
    (void) pdu;
    (void) pdu_len;

    return 0;
}

/* dummy function stubs */
void datalink_get_broadcast_address(
    BACNET_ADDRESS * dest)
{
    (void) dest;
}

void testTSM(
    Test * pTest)
{
    /* FIXME: add some unit testing... */
    return;
}

#ifdef TEST_TSM
int main(
    void)
{
    Test *pTest;
    bool rc;

    pTest = ct_create("BACnet TSM", NULL);
    /* individual tests */
    rc = ct_addTestFunction(pTest, testTSM);
    assert(rc);

    ct_setStream(pTest, stdout);
    ct_run(pTest);
    (void) ct_report(pTest);
    ct_destroy(pTest);

    return 0;
}
#endif /* TEST_TSM */
#endif /* TEST */
#endif /* MAX_TSM_TRANSACTIONS */
