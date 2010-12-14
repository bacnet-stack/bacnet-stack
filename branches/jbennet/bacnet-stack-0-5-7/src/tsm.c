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

#define DEFAULT_WINDOW_SIZE 32

#ifndef max
#define max(a,b) (((a) > (b)) ? (a) : (b))
#endif
#ifndef min
#define min(a,b) (((a) < (b)) ? (a) : (b))
#endif


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

    /* TSM transaction is activated when (tsm[id-1].InvokeID not zero) */
    if (session_object->TSM_List[index_0_N].InvokeID != 0)
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

    while (!found) {
        /* get next invokeID for this session */
        while (!(my_current_invokeID = (++(session_object->TSM_Current_InvokeID)) % (int) (MAX_TSM_TRANSACTIONS + 1))) {        /* numbers 1 to MAX_TSM_TRANSACTIONS, 0 excluded */
            /* loop once more : step over "0" */
        }

        /* test next "invokeID", ok if it is free, otherwise try next invokeId */
        index =
            tsm_allocate_first_free_index(session_object, my_current_invokeID);
        /* we allocated a free slot with the current invokeID */
        if (index != MAX_TSM_TRANSACTIONS) {
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

/** Finds (optionally creates) an existing peer data */
static BACNET_TSM_INDIRECT_DATA *tsm_get_peer_id_data(
    struct bacnet_session_object *session_object,
    BACNET_ADDRESS * src,
    int invokeID,
    bool createPeerId)
{
    int ix;
    int index;
    int free_ix_found = -1;
    BACNET_TSM_INDIRECT_DATA *item = NULL;

    bacnet_session_lock(session_object);

    /* look for an empty slot, or a matching (address,peer invoke ID) */
    for (ix = 0; ix < MAX_TSM_PEERS && !item; ix++) {
        /* not free : see if it matches */
        if (session_object->TSM_Peer_Ids[ix].InternalInvokeID != 0) {
            if (invokeID == session_object->TSM_Peer_Ids[ix].PeerInvokeID &&
                address_match(src,
                    &session_object->TSM_Peer_Ids[ix].PeerAddress)) {
                item = &session_object->TSM_Peer_Ids[ix];
            }
        } else if (free_ix_found < 0) {
            /* mark free slot found */
            free_ix_found = ix;
        }
    }
    /* create new data */
    if ((!item) && createPeerId && (free_ix_found > -1)) {
        /* memorize peer data */
        session_object->TSM_Peer_Ids[free_ix_found].PeerInvokeID = invokeID;
        session_object->TSM_Peer_Ids[free_ix_found].PeerAddress = *src;
        /* create an internal TSM slot (with internal invokeID number which is not relevant) */
        session_object->TSM_Peer_Ids[free_ix_found].InternalInvokeID =
            tsm_next_free_invokeID(session_object);

        index =
            tsm_find_invokeID_index(session_object,
            session_object->TSM_Peer_Ids[free_ix_found].InternalInvokeID);
        if (index < MAX_TSM_TRANSACTIONS) {
            /* explicitely memorize peer InvokeID */
            session_object->TSM_List[index].InvokeID = invokeID;
            session_object->TSM_List[index].dest = *src;
            item = &session_object->TSM_Peer_Ids[free_ix_found];
        } else {
            /* problem : reset slot (NULL returned) */
            session_object->TSM_Peer_Ids[free_ix_found].InternalInvokeID = 0;
        }
    }

    bacnet_session_unlock(session_object);

    return item;
}

/** Associates a Peer address and invoke ID with our TSM 
@returns A local InvokeID unique number, 0 in case of error. */
uint8_t tsm_get_peer_id(
    struct bacnet_session_object * session_object,
    BACNET_ADDRESS * src,
    int invokeID)
{
    BACNET_TSM_INDIRECT_DATA *peer_data;
    peer_data = tsm_get_peer_id_data(session_object, src, invokeID, true);
    if (peer_data)
        return peer_data->InternalInvokeID;
    return 0;
}

/** Clear TSM Peer data */
void tsm_clear_peer_id(
    struct bacnet_session_object *session_object,
    int InternalInvokeID)
{
    int ix;

    bacnet_session_lock(session_object);

    /* look for a matching internal invoke ID */
    for (ix = 0; ix < MAX_TSM_PEERS; ix++)
        /* see if it matches the internal number */
        if (session_object->TSM_Peer_Ids[ix].InternalInvokeID ==
            InternalInvokeID)
            session_object->TSM_Peer_Ids[ix].InternalInvokeID = 0;

    bacnet_session_unlock(session_object);
}

void bacnet_calc_transmittable_length(
    struct bacnet_session_object *session_object,
    BACNET_ADDRESS * dest,
    BACNET_CONFIRMED_SERVICE_DATA * confirmed_service_data,
    uint32_t * apdu_max,
    uint32_t * total_max)
{
    uint32_t deviceId = 0;
    unsigned max_apdu;
    uint8_t segmentation;
    uint32_t maxsegments;
    BACNET_ADDRESS src;

    /* either we are replying to a confirmed service, so we use prompted values ;
       either we are requesting a peer, so we use memorised informations 
       about the peer device.
     */
    if (confirmed_service_data) {
        /* use maximum available APDU */
        *total_max = *apdu_max =
            min(confirmed_service_data->max_resp, MAX_APDU);
        /* segmented : compute maximum number of packets */
        if (confirmed_service_data->segmented_response_accepted) {
            maxsegments = confirmed_service_data->max_segs;
            /* if unspecified, try the maximum available, not just 2 segments */
            if (!maxsegments || maxsegments > 64)
                maxsegments = MAX_SEGMENTS_ACCEPTED;
            /* maximum size we are able to transmit */
            *total_max = min(maxsegments, MAX_SEGMENTS_ACCEPTED) * (*apdu_max);
        }
        return;
    }

    if (address_get_device_id(session_object, dest, &deviceId)) {
        if (address_get_by_device(session_object, deviceId, &max_apdu,
                &segmentation, &maxsegments, &src)) {
            /* Best possible APDU size */
            *total_max = *apdu_max = min(max_apdu, MAX_APDU);
            /* IIf device is able to receive segments */
            if (segmentation == SEGMENTATION_BOTH ||
                segmentation == SEGMENTATION_RECEIVE) {
                /* XXX - TODO: Number of segments accepted by peer device :
                   If zero segments we should fallback to 2 segments.
                   Or Maybe we just didn't ask the device about the maximum segments supported.
                 */
                if (!maxsegments)
                    maxsegments = MAX_SEGMENTS_ACCEPTED;
                /* maximum size we are able to transmit */
                if (maxsegments)
                    *total_max =
                        min(maxsegments, MAX_SEGMENTS_ACCEPTED) * (*apdu_max);
            }
            return;
        }
    }
    *apdu_max = MAX_APDU;
    *total_max = *apdu_max * MAX_SEGMENTS_ACCEPTED;
}


/* Free allocated blob data */
void free_blob(
    BACNET_TSM_DATA * data)
{
    /* Free received data blobs */
    if (data->apdu_blob)
        free(data->apdu_blob);
    data->apdu_blob = NULL;
    data->apdu_blob_allocated = 0;
    data->apdu_blob_size = 0;
    /* Free sent data blobs */
    if (data->apdu)
        free(data->apdu);
    data->apdu = NULL;
    data->apdu_len = 0;
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

/* Copy new data to current APDU sending blob data */
void copy_apdu_blob_data(
    BACNET_TSM_DATA * data,
    uint8_t * bdata,
    uint32_t data_len)
{
    if (data->apdu)
        free(data->apdu);
    data->apdu = NULL;
    data->apdu = calloc(1, data_len);
    memcpy(data->apdu, bdata, data_len);
    data->apdu_len = data_len;
}

/* theorical size of apdu fixed header */
uint32_t get_apdu_header_typical_size(
    BACNET_APDU_FIXED_HEADER * header,
    bool segmented)
{
    switch (header->pdu_type) {
        case PDU_TYPE_COMPLEX_ACK:
            return segmented ? 5 : 3;
        case PDU_TYPE_CONFIRMED_SERVICE_REQUEST:
            return segmented ? 6 : 4;
    }
    return 3;
}

/* gets Nth packet data to send in a segmented operation, or get the only data packet in unsegmented world. */
uint8_t *get_apdu_blob_data_segment(
    BACNET_TSM_DATA * data,
    int segment_number,
    uint32_t * data_len)
{
    /* Data is splitted in N blocks of, at maximum, ( APDU_MAX - APDU_HEADER ) bytes */
    bool segmented =
        data->apdu_fixed_header.service_data.common_data.segmented_message;
    int header_size =
        get_apdu_header_typical_size(&data->apdu_fixed_header, segmented);
    int block_request_size = (data->apdu_maximum_length - header_size);
    int data_position = segment_number * block_request_size;
    int remaining_size = (int) data->apdu_len - data_position;
    *data_len = (uint32_t) max(0, min(remaining_size, block_request_size));
    return data->apdu + data_position;
}


/* calculates how many segments will be used to send data in this TSM slot 
@return 1 : No segmentation needed, >1 segmentation needed (number of segments). */
uint32_t get_apdu_max_segments(
    BACNET_TSM_DATA * data)
{
    uint32_t header_size;
    uint32_t packets;

    /* Are we unsegmented ? */
    header_size =
        get_apdu_header_typical_size(&data->apdu_fixed_header, false);
    if (header_size + data->apdu_len <= data->apdu_maximum_length)
        return 1;

    /* We are segmented : calculate how many segments to use */
    header_size = get_apdu_header_typical_size(&data->apdu_fixed_header, true);

    /* Number of packets to use formula : p = ( ( total_length - 1 ) / packet_length ) + 1; */
    packets =
        ((data->apdu_len - 1) / (data->apdu_maximum_length - header_size)) + 1;

    return packets;
}

/* send a packet to peer */
int tsm_pdu_send(
    struct bacnet_session_object *sess,
    BACNET_TSM_DATA * tsm_data,
    uint32_t segment_number)
{
    uint8_t Handler_Transmit_Buffer[MAX_PDU] = { 0 };
    BACNET_ADDRESS my_address;
    int len = 0;
    int pdu_len = 0;
    uint8_t *service_data = NULL;
    uint32_t service_len = 0;
    uint32_t total_segments = 0;

    /* Rebuild PDU */
    sess->datalink_get_my_address(sess, &my_address);
    len =
        npdu_encode_pdu(&Handler_Transmit_Buffer[pdu_len], &tsm_data->dest,
        &my_address, &tsm_data->npdu_data);
    if (len < 0)
        return -1;
    pdu_len += len;
    /* Header tweaks ! */
    total_segments = get_apdu_max_segments(tsm_data);
    /* Index out of bounds */
    if (segment_number >= total_segments)
        return -1;
    if (total_segments == 1) {
        tsm_data->apdu_fixed_header.service_data.common_data.
            segmented_message = false;
    } else {
        /* SEG */
        tsm_data->apdu_fixed_header.service_data.common_data.
            segmented_message = true;
        /* MORE */
        tsm_data->apdu_fixed_header.service_data.common_data.more_follows =
            (segment_number < total_segments - 1);
        /* Window size : do not modify */
        /* tsm_data->apdu_fixed_header.service_data.common_data.proposed_window_number = 127; */
        /* SEQ# */
        tsm_data->apdu_fixed_header.service_data.common_data.sequence_number =
            segment_number;
    }
    /* Rebuild APDU Header */
    len =
        apdu_encode_fixed_header(&Handler_Transmit_Buffer[pdu_len],
        sizeof(Handler_Transmit_Buffer) - pdu_len,
        &tsm_data->apdu_fixed_header);
    if (len < 0)
        return -1;
    pdu_len += len;
    /* Rebuild APDU service data */
    /* gets Nth packet data */
    service_data =
        get_apdu_blob_data_segment(tsm_data, segment_number, &service_len);
    if (!service_data)  /* May be zero-size ! */
        return -1;
    /* enough room ? */
    if (!check_write_apdu_space(pdu_len, sizeof(Handler_Transmit_Buffer),
            service_len))
        return -1;
    memcpy(&Handler_Transmit_Buffer[pdu_len], service_data, service_len);
    pdu_len += service_len;
    return sess->datalink_send_pdu(sess, &tsm_data->dest, &tsm_data->npdu_data,
        &Handler_Transmit_Buffer[0], pdu_len);
}

void FillWindow(
    struct bacnet_session_object *sess,
    BACNET_TSM_DATA * tsm_data,
    uint32_t sequence_number)
{
    uint32_t ix;
    uint32_t total_segments = get_apdu_max_segments(tsm_data);
    for (ix = 0;
        (ix < tsm_data->ActualWindowSize) &&
        (sequence_number + ix < total_segments); ix++) {
        tsm_pdu_send(sess, tsm_data, sequence_number + ix);
    }
    /* sent all segments ? */
    if (ix + sequence_number >= total_segments)
        tsm_data->SentAllSegments = true;
}

bool InWindow(
    BACNET_TSM_DATA * data,
    uint8_t seqA,
    uint8_t seqB)
{
    uint8_t m = seqA - seqB;
    return m < data->ActualWindowSize;
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
    uint8_t reason,
    bool server)
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
        reason, server);
    pdu_len = apdu_len + npdu_len;
    bytes_sent =
        sess->datalink_send_pdu(sess, dest, &npdu_data,
        &Handler_Transmit_Buffer[0], pdu_len);
}

/** Initiate confirmed segmented or unsegmented transaction state, and send first packet.
   @param pdu Only "service request" part of PDU packet.
   @param pdu_len Length of "service request" part of PDU packet.
   @param apdu_fixed_header APDU Header.
   @param session_object Current session context.
   @param invokeID Current TSM invokeID.
   @param dest BACnet Address of peer device.
   @param confirmed_service_data In reply of a confirmed service, the request values, null otherwise.
   @result bytes_sent to destination (complete packet or first packet only in segmented case)
*/
int tsm_set_confirmed_transaction(
    struct bacnet_session_object *session_object,
    uint8_t invokeID,
    BACNET_ADDRESS * dest,
    BACNET_NPDU_DATA * npdu_data,
    BACNET_APDU_FIXED_HEADER * apdu_fixed_header,
    uint8_t * pdu,
    uint32_t pdu_len)
{
    uint8_t index;
    uint32_t apdu_segments;
    int bytes_sent = 0;
    BACNET_TSM_DATA *tsm_data;

    if (!invokeID)
        return -1;
    index = tsm_find_invokeID_index(session_object, invokeID);
    if (index >= MAX_TSM_TRANSACTIONS)
        return -1;
    tsm_data = &session_object->TSM_List[index];

    /* Choice between a segmented or a non-segmented transaction */

    /* fill in maximum fill values */
    bacnet_calc_transmittable_length(session_object, dest, NULL,
        &tsm_data->apdu_maximum_length,
        &tsm_data->maximum_transmittable_length);
    /* copy the apdu service data */
    copy_apdu_blob_data(tsm_data, &pdu[0], pdu_len);
    /* copy npdu data */
    npdu_copy_data(&tsm_data->npdu_data, npdu_data);
    /* copy apdu header data */
    tsm_data->apdu_fixed_header = *apdu_fixed_header;
    /* destination address */
    bacnet_address_copy(&tsm_data->dest, dest);
    /* absolute "retry" count : won't be reinitialized later */
    tsm_data->RetryCount = apdu_retries(session_object);

    tsm_data->ActualWindowSize = 1;
    tsm_data->ProposedWindowSize = DEFAULT_WINDOW_SIZE;
    tsm_data->InitialSequenceNumber = 0;
    tsm_data->SentAllSegments = false;

    /* Choice between a segmented or a non-segmented transaction */
    if (1 == (apdu_segments = get_apdu_max_segments(tsm_data))) {
        /* UNSEGMENTED MODE */
        tsm_data->state = TSM_STATE_AWAIT_CONFIRMATION;
        /* start the timer */
        tsm_data->RequestTimer = apdu_timeout(session_object);
        bytes_sent = tsm_pdu_send(session_object, tsm_data, 0);
    } else {
        /* SEGMENTED-MODE */
        /* Take into account the fact that the APDU header is repeated on every segment */
        if (pdu_len +
            apdu_segments * get_apdu_header_typical_size(apdu_fixed_header,
                true) > tsm_data->maximum_transmittable_length) {
            /* Too much data : we cannot send that much, or the API cannot receive that much ! */
            bytes_sent = -2;
        } else {
            /* Window size proposal */
            tsm_data->apdu_fixed_header.service_data.common_data.
                proposed_window_number = tsm_data->ProposedWindowSize;
            /* assign the transaction */
            tsm_data->state = TSM_STATE_SEGMENTED_REQUEST_CLIENT;
            tsm_data->SegmentRetryCount = apdu_retries(session_object);
            /* start the timer */
            tsm_data->RequestTimer = 0;
            tsm_data->SegmentTimer = apdu_segment_timeout(session_object);
            /* Send first packet */
            bytes_sent = tsm_pdu_send(session_object, tsm_data, 0);
        }
    }
    /* If we cannot initiate, free transaction so we don't wait on a timeout to realize it has failed. 
       Caller don't free invoke ID : we must clear it now. */
    if (bytes_sent <= 0)
        tsm_free_invoke_id_check(session_object, tsm_data->InvokeID, dest,
            true);
    return bytes_sent;
}

/** Initiate complex-ack segmented or unsegmented transaction state, and send first packet.
   @param pdu Only "service request" part of PDU packet.
   @param pdu_len Length of "service request" part of PDU packet.
   @param apdu_fixed_header APDU Header.
   @param session_object Current session context.
   @param dest BACnet Address of peer device.
   @param confirmed_service_data In reply of a confirmed service, the service request values.
   @result bytes_sent to destination (complete packet or first packet only in segmented case)
*/
int tsm_set_complexack_transaction(
    struct bacnet_session_object *session_object,
    BACNET_ADDRESS * dest,
    BACNET_NPDU_DATA * npdu_data,
    BACNET_APDU_FIXED_HEADER * apdu_fixed_header,
    BACNET_CONFIRMED_SERVICE_DATA * confirmed_service_data,
    uint8_t * pdu,
    uint32_t pdu_len)
{
    uint8_t index;
    int bytes_sent;
    BACNET_TSM_DATA *tsm_data;
    uint32_t apdu_segments;
    uint8_t internal_service_id = tsm_get_peer_id(session_object, dest,
        confirmed_service_data->invoke_id);

    if (!internal_service_id) {
        /* failed : could not allocate enough slot for this transaction */
        abort_pdu_send(session_object, confirmed_service_data->invoke_id, dest,
            ABORT_REASON_PREEMPTED_BY_HIGHER_PRIORITY_TASK, true);
        return -1;
    }
    index = tsm_find_invokeID_index(session_object, internal_service_id);
    if (index >= MAX_TSM_TRANSACTIONS) {        /* shall not fail */
        abort_pdu_send(session_object, confirmed_service_data->invoke_id, dest,
            ABORT_REASON_OTHER, true);
        return -1;
    }
    tsm_data = &session_object->TSM_List[index];

    /* Choice between a segmented or a non-segmented transaction */

    /* fill in maximum fill values */
    bacnet_calc_transmittable_length(session_object, dest,
        confirmed_service_data, &tsm_data->apdu_maximum_length,
        &tsm_data->maximum_transmittable_length);
    /* copy the apdu service data */
    copy_apdu_blob_data(tsm_data, &pdu[0], pdu_len);
    /* copy npdu data */
    npdu_copy_data(&tsm_data->npdu_data, npdu_data);
    /* copy apdu header data */
    tsm_data->apdu_fixed_header = *apdu_fixed_header;
    /* destination address */
    bacnet_address_copy(&tsm_data->dest, dest);
    /* absolute "retry" count : won't be reinitialized later */
    tsm_data->RetryCount = apdu_retries(session_object);

    tsm_data->ActualWindowSize = 1;
    tsm_data->ProposedWindowSize = DEFAULT_WINDOW_SIZE;
    tsm_data->InitialSequenceNumber = 0;
    tsm_data->SentAllSegments = false;

    /* Choice between a segmented or a non-segmented transaction */
    if (1 == (apdu_segments = get_apdu_max_segments(tsm_data))) {
        /* UNSEGMENTED MODE : Free transaction afterwards */
        bytes_sent = tsm_pdu_send(session_object, tsm_data, 0);
        if (bytes_sent > 0)
            tsm_free_invoke_id_check(session_object, internal_service_id, dest,
                true);
    } else {
        /* SEGMENTED-MODE */
        /* Take into account the fact that the APDU header is repeated on every segment */
        if (pdu_len +
            apdu_segments * get_apdu_header_typical_size(apdu_fixed_header,
                true) > tsm_data->maximum_transmittable_length) {
            /* Too much data : we cannot send that much, or the API cannot receive that much ! */
            bytes_sent = -2;
        } else {
            /* Window size proposal */
            tsm_data->apdu_fixed_header.service_data.common_data.
                proposed_window_number = tsm_data->ProposedWindowSize;
            /* assign the transaction */
            tsm_data->state = TSM_STATE_SEGMENTED_RESPONSE;
            tsm_data->SegmentRetryCount = apdu_retries(session_object);
            /* start the timer */
            tsm_data->RequestTimer = 0;
            tsm_data->SegmentTimer = apdu_segment_timeout(session_object);
            /* Send first packet */
            bytes_sent = tsm_pdu_send(session_object, tsm_data, 0);
        }
    }
    /* If we cannot initiate, free transaction so we don't wait on a timeout to realize it has failed. 
       Caller don't free invoke ID : we must clear it now. */
    if (bytes_sent <= 0)
        tsm_free_invoke_id_check(session_object, internal_service_id, dest,
            true);
    return bytes_sent;
}

/* error pdu received */
void tsm_error_received(
    struct bacnet_session_object *session_object,
    uint8_t invoke_id,
    BACNET_ADDRESS * src)
{
    uint8_t index;
    index = tsm_find_invokeID_index(session_object, invoke_id);
    if (index < MAX_TSM_TRANSACTIONS) {
        /* ASHRAE 135-2008 5.4.4.3 ErrorPDU_Received */
        if (session_object->TSM_List[index].state ==
            TSM_STATE_SEGMENTED_REQUEST_CLIENT) {
            /* Abort */
            if (!session_object->TSM_List[index].SentAllSegments)
                abort_pdu_send(session_object, invoke_id, src,
                    ABORT_REASON_INVALID_APDU_IN_THIS_STATE, false);
        }
    }
}

/* reject pdu received */
void tsm_reject_received(
    struct bacnet_session_object *session_object,
    uint8_t invoke_id,
    BACNET_ADDRESS * src)
{
    uint8_t index;
    index = tsm_find_invokeID_index(session_object, invoke_id);
    if (index < MAX_TSM_TRANSACTIONS) {
        /* ASHRAE 135-2008 5.4.4.3 RejectPDU_Received */
        if (session_object->TSM_List[index].state ==
            TSM_STATE_SEGMENTED_REQUEST_CLIENT) {
            /* Abort */
            if (!session_object->TSM_List[index].SentAllSegments)
                abort_pdu_send(session_object, invoke_id, src,
                    ABORT_REASON_INVALID_APDU_IN_THIS_STATE, false);
        }
    }
}

void tsm_segmentack_received(
    struct bacnet_session_object *session_object,
    uint8_t invoke_id,
    uint8_t sequence_number,
    uint8_t actual_window_size,
    bool nak,
    bool server,
    BACNET_ADDRESS * src)
{
    uint8_t index;
    uint32_t big_segment_number;
    uint8_t window;
    bool some_segment_remains;
    BACNET_TSM_INDIRECT_DATA *peer_data;

    /* bad invoke number from server peer (we never use 0) */
    if (server && !invoke_id)
        return;
    /* Peer invoke id number : translate to our internal numbers */
    if (!server) {
        peer_data =
            tsm_get_peer_id_data(session_object, src, invoke_id, false);
        if (!peer_data) {
            /* failed : unknown message */
            return;
        }
        /* now we use our internal number */
        invoke_id = peer_data->InternalInvokeID;
    }
    /* Find an active TSM slot that matches the Segment-Ack */
    index = tsm_find_invokeID_index(session_object, invoke_id);
    if (index >= MAX_TSM_TRANSACTIONS)
        return;

    /* Almost the same code for segment handling between segmented requests and responses */
    if ((server &&
            session_object->TSM_List[index].state ==
            TSM_STATE_SEGMENTED_REQUEST_CLIENT)
        || (!server &&
            session_object->TSM_List[index].state ==
            TSM_STATE_SEGMENTED_RESPONSE)) {
        /* DuplicateAck_Received */
        if (!InWindow(&session_object->TSM_List[index], sequence_number,
                session_object->TSM_List[index].InitialSequenceNumber)) {
            /* Restart timer */
            session_object->TSM_List[index].SegmentTimer =
                apdu_segment_timeout(session_object);
        } else {
            /* total segment number (not modulo 256) */
            window =
                sequence_number -
                (uint8_t) session_object->TSM_List[index].
                InitialSequenceNumber;
            big_segment_number =
                session_object->TSM_List[index].InitialSequenceNumber + window;

            /* 1..N segment number < number of segments ? */
            some_segment_remains =
                (big_segment_number + 1) <
                get_apdu_max_segments(&session_object->TSM_List[index]);
            if (some_segment_remains) {
                /* NewAck_Received : do we have a segment remaining to send */
                session_object->TSM_List[index].InitialSequenceNumber =
                    big_segment_number + 1;
                session_object->TSM_List[index].ActualWindowSize =
                    actual_window_size;
                session_object->TSM_List[index].SegmentRetryCount =
                    apdu_retries(session_object);
                session_object->TSM_List[index].SegmentTimer =
                    apdu_segment_timeout(session_object);
                FillWindow(session_object, &session_object->TSM_List[index],
                    session_object->TSM_List[index].InitialSequenceNumber);
                session_object->TSM_List[index].SegmentTimer =
                    apdu_segment_timeout(session_object);
            } else {
                /* FinalAck_Received */
                session_object->TSM_List[index].SegmentTimer = 0;
                if (session_object->TSM_List[index].state ==
                    TSM_STATE_SEGMENTED_RESPONSE) {
                    /* Response : end communications */
                    /* Release data */
                    free_blob(&session_object->TSM_List[index]);
                    session_object->TSM_List[index].state = TSM_STATE_IDLE;
                    /* Completely free data */
                    tsm_free_invoke_id_check(session_object, invoke_id, NULL,
                        true);
                } else {
                    /* Request : Wait confirmation */
                    session_object->TSM_List[index].RequestTimer =
                        apdu_timeout(session_object);
                    session_object->TSM_List[index].state =
                        TSM_STATE_AWAIT_CONFIRMATION;
                }
            }
        }
    }
}

bool tsm_set_simpleack_received(
    struct bacnet_session_object *session_object,
    uint8_t invoke_id,
    BACNET_ADDRESS * src)
{
    /* Reject a ComplexACK unsegmented if TSM state is incorrect */
    uint8_t index;
    bool result = false;

    if (invoke_id) {
        index = tsm_find_invokeID_index(session_object, invoke_id);
        if (index < MAX_TSM_TRANSACTIONS)
            /* We may receive unsegmented complex-ack in only two states */
            result =
                (session_object->TSM_List[index].state ==
                TSM_STATE_AWAIT_CONFIRMATION)
                || (session_object->TSM_List[index].state ==
                TSM_STATE_SEGMENTED_REQUEST_CLIENT &&
                session_object->TSM_List[index].SentAllSegments);
    }
    /* In case of error: send abort */
    if (!result) {
        /* ERROR: Abort */
        abort_pdu_send(session_object, invoke_id, src,
            ABORT_REASON_INVALID_APDU_IN_THIS_STATE, false);
    }
    return result;
}

bool tsm_set_complexack_received(
    struct bacnet_session_object * session_object,
    BACNET_ADDRESS * src,
    BACNET_CONFIRMED_SERVICE_ACK_DATA * service_data)
{
    /* Reject a ComplexACK unsegmented if TSM state is incorrect */
    uint8_t index;
    bool result = false;

    if (service_data->invoke_id) {
        index =
            tsm_find_invokeID_index(session_object, service_data->invoke_id);
        if (index < MAX_TSM_TRANSACTIONS)
            /* We may receive unsegmented complex-ack in only two states */
            result =
                (session_object->TSM_List[index].state ==
                TSM_STATE_AWAIT_CONFIRMATION)
                || (session_object->TSM_List[index].state ==
                TSM_STATE_SEGMENTED_REQUEST_CLIENT &&
                session_object->TSM_List[index].SentAllSegments);
    }
    /* In case of error: send abort */
    if (!result) {
        /* ERROR: Abort */
        abort_pdu_send(session_object, service_data->invoke_id, src,
            ABORT_REASON_INVALID_APDU_IN_THIS_STATE, false);
    }
    return result;
}

/** We received a segment of a ConfirmedService packet, check TSM state and reassemble the full packet */
bool tsm_set_segmented_confirmed_service_received(
    struct bacnet_session_object * session_object,
    BACNET_ADDRESS * src,
    BACNET_CONFIRMED_SERVICE_DATA * service_data,
    uint8_t * internal_invoke_id,
    uint8_t ** pservice_request,        /* IN/OUT */
    uint32_t * pservice_request_len     /* IN/OUT */
    )
{
    uint8_t index;
    uint8_t *service_request = *pservice_request;
    uint32_t service_request_len = *pservice_request_len;
    bool result = false;
    bool ack_needed = false;

    uint8_t internal_service_id =
        tsm_get_peer_id(session_object, src, service_data->invoke_id);
    *internal_invoke_id = internal_service_id;
    if (!internal_service_id) {
        /* failed : could not allocate enough slot for this transaction */
        abort_pdu_send(session_object, service_data->invoke_id, src,
            ABORT_REASON_PREEMPTED_BY_HIGHER_PRIORITY_TASK, true);
        return false;
    }
    index = tsm_find_invokeID_index(session_object, internal_service_id);
    if (index >= MAX_TSM_TRANSACTIONS) {        /* shall not fail */
        abort_pdu_send(session_object, service_data->invoke_id, src,
            ABORT_REASON_OTHER, true);
        return false;
    }

    /* check states */
    switch (session_object->TSM_List[index].state) {
            /* Initial state: ConfirmedSegmentReceived */
        case TSM_STATE_IDLE:
            /* We never stay in IDLE state */
            session_object->TSM_List[index].state =
                TSM_STATE_SEGMENTED_REQUEST_SERVER;
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
            session_object->TSM_List[index].RequestTimer = 0;   /* unused */
            /* start the segmented timer */
            session_object->TSM_List[index].SegmentTimer =
                apdu_segment_timeout(session_object) * 4;
            /* reset memorized data */
            reset_blob(&session_object->TSM_List[index]);
            /* Test : sequence number MUST be 0 */
            /* UnexpectedPDU_Received */
            if (service_data->sequence_number != 0) {
                /* Release data */
                free_blob(&session_object->TSM_List[index]);
                /* Abort */
                abort_pdu_send(session_object, service_data->invoke_id, src,
                    ABORT_REASON_INVALID_APDU_IN_THIS_STATE, true);
                /* We must free invoke_id ! */
                tsm_free_invoke_id_check(session_object, internal_service_id,
                    NULL, true);
            } else {
                /* Okay : memorize data */
                add_blob_data(&session_object->TSM_List[index],
                    service_request, service_request_len);
                /* We ACK the first segment of the segmented message */
                segmentack_pdu_send(session_object, src, false, true,
                    service_data->invoke_id,
                    session_object->TSM_List[index].LastSequenceNumber,
                    session_object->TSM_List[index].ActualWindowSize);
            }
            break;

            /* New segments  */
        case TSM_STATE_SEGMENTED_REQUEST_SERVER:
            /* reset the segment timer */
            session_object->TSM_List[index].RequestTimer = 0;   /* unused */
            /* ANSI/ASHRAE 135-2008 5.4.5.2 SEGMENTED_REQUEST / Timeout */
            /* If SegmentTimer becomes greater than Tseg times four, [...] enter IDLE state */
            session_object->TSM_List[index].SegmentTimer =
                apdu_segment_timeout(session_object) * 4;

            /* Sequence number MUST be (LastSequenceNumber+1 modulo 256) */
            if (service_data->sequence_number !=
                (uint8_t) (session_object->TSM_List[index].LastSequenceNumber +
                    1)) {
                /* Recoverable Error: SegmentReceivedOutOfOrder */
                /* ACK of last segment correctly received. */
                segmentack_pdu_send(session_object, src, true, true,
                    service_data->invoke_id,
                    session_object->TSM_List[index].LastSequenceNumber,
                    session_object->TSM_List[index].ActualWindowSize);
            } else {
                /* Count maximum segments */
                if (++session_object->TSM_List[index].ReceivedSegmentsCount >
                    MAX_SEGMENTS_ACCEPTED) {
                    /* ABORT: SegmentReceivedOutOfSpace */
                    abort_pdu_send(session_object, service_data->invoke_id,
                        src, ABORT_REASON_BUFFER_OVERFLOW, true);
                    /* Release data */
                    free_blob(&session_object->TSM_List[index]);
                    /* Enter IDLE state */
                    session_object->TSM_List[index].state = TSM_STATE_IDLE;
                    /* We must free invoke_id ! */
                    tsm_free_invoke_id_check(session_object,
                        internal_service_id, NULL, true);
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
                        session_object->TSM_List[index].InitialSequenceNumber =
                            service_data->sequence_number;
                    }
                    /* LastSegmentOfComplexACK_Received */
                    if (!service_data->more_follows) {
                        /* Resulting segment data */
                        *pservice_request =
                            get_blob_data(&session_object->TSM_List[index],
                            pservice_request_len);
                        result = true;  /* Returns true on final segment received */
                        ack_needed = true;
                    }
                    /* LastSegmentOfComplexACK_Received or LastSegmentOfGroupReceived */
                    if (ack_needed)
                        /* ACK received segment */
                        segmentack_pdu_send(session_object, src, false, true,
                            service_data->invoke_id,
                            session_object->TSM_List[index].LastSequenceNumber,
                            session_object->TSM_List[index].ActualWindowSize);
                }
            }
            break;
    }
    return result;
}

/** We received a segment of a ComplexAck packet, check TSM state and reassemble the full packet */
bool tsm_set_segmentedcomplexack_received(
    struct bacnet_session_object * session_object,
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

    /* we don't send invokeid = 0 packets */
    if (!service_data->invoke_id)
        return false;
    index = tsm_find_invokeID_index(session_object, service_data->invoke_id);
    if (index >= MAX_TSM_TRANSACTIONS)
        return false;

    switch (session_object->TSM_List[index].state) {
            /* ASHRAE 135-2008 - The Application Layer, p. 29 - SegmentedComplexACK_Received */
            /* The first segment of a segmented ACK response was received : initialize specific status for segmentation */
        case TSM_STATE_AWAIT_CONFIRMATION:
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
            session_object->TSM_List[index].RequestTimer = 0;   /* unused */
            /* start the segmented timer */
            session_object->TSM_List[index].SegmentTimer = apdu_segment_timeout(session_object) * 4;    /* 5.4.4.4 Timeout : Tseg times four */
            /* reset memorized data */
            reset_blob(&session_object->TSM_List[index]);
            /* Test : sequence number MUST be 0 */
            /* UnexpectedPDU_Received */
            if (service_data->sequence_number != 0) {
                /* ERROR : Abort */
                abort_pdu_send(session_object, service_data->invoke_id, src,
                    ABORT_REASON_INVALID_APDU_IN_THIS_STATE, false);
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
            break;

            /* ASHRAE 135-2008 - The Application Layer, p. 30 : */
            /* NewSegmentReceived */
            /* LastSegmentOfGroupReceived */
            /* LastSegmentOfComplexACK_Received */
            /* */
            /* A segment of a segmented ACK response was received :  */
        case TSM_STATE_SEGMENTED_CONFIRMATION:
            /* reset the segment timer */
            session_object->TSM_List[index].RequestTimer = 0;   /* unused */
            session_object->TSM_List[index].SegmentTimer = apdu_segment_timeout(session_object) * 4;    /* 5.4.4.4 Timeout : Tseg times four */

            /* Sequence number MUST be (LastSequenceNumber+1 modulo 256) */
            if (service_data->sequence_number !=
                (uint8_t) (session_object->TSM_List[index].LastSequenceNumber +
                    1)) {
                /* Recoverable Error: SegmentReceivedOutOfOrder */
                /* ACK of last segment correctly received. */
                segmentack_pdu_send(session_object, src, true, false,
                    service_data->invoke_id,
                    session_object->TSM_List[index].LastSequenceNumber,
                    session_object->TSM_List[index].ActualWindowSize);
            } else {
                /* Count maximum segments */
                if (++session_object->TSM_List[index].ReceivedSegmentsCount >
                    MAX_SEGMENTS_ACCEPTED) {
                    /* ABORT: SegmentReceivedOutOfSpace */
                    abort_pdu_send(session_object, service_data->invoke_id,
                        src, ABORT_REASON_BUFFER_OVERFLOW, false);
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
                        session_object->TSM_List[index].InitialSequenceNumber =
                            service_data->sequence_number;
                    }
                    /* LastSegmentOfComplexACK_Received */
                    if (!service_data->more_follows) {
                        /* Resulting segment data */
                        *pservice_request =
                            get_blob_data(&session_object->TSM_List[index],
                            pservice_request_len);
                        result = true;  /* Returns true on final segment received */
                        ack_needed = true;
                    }
                    /* LastSegmentOfComplexACK_Received or LastSegmentOfGroupReceived */
                    if (ack_needed)
                        /* ACK received segment */
                        segmentack_pdu_send(session_object, src, false, false,
                            service_data->invoke_id,
                            session_object->TSM_List[index].LastSequenceNumber,
                            session_object->TSM_List[index].ActualWindowSize);
                }
            }
            break;

            /* ASHRAE 135-2008 - The Application Layer, p. 28 : */
            /* SegmentedComplexACK_Received */
        case TSM_STATE_SEGMENTED_REQUEST_CLIENT:
            if (session_object->TSM_List[index].SentAllSegments) {
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
                session_object->TSM_List[index].SegmentTimer = apdu_segment_timeout(session_object) * 4;        /* 5.4.4.4 Timeout : Tseg times four */
                /* reset memorized data */
                reset_blob(&session_object->TSM_List[index]);
                /* Test : sequence number MUST be 0 */
                /* UnexpectedPDU_Received */
                if (service_data->sequence_number != 0) {
                    /* ERROR : Abort */
                    abort_pdu_send(session_object, service_data->invoke_id,
                        src, ABORT_REASON_INVALID_APDU_IN_THIS_STATE, false);
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
            } else {
                /* Abort (! SentAllSegments) */
                abort_pdu_send(session_object, service_data->invoke_id, src,
                    ABORT_REASON_INVALID_APDU_IN_THIS_STATE, false);
            }
            break;
            /* Unexpected packet */
        default:
            abort_pdu_send(session_object, service_data->invoke_id, src,
                ABORT_REASON_INVALID_APDU_IN_THIS_STATE, false);
            break;
    }   /*end switch    */
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
    uint32_t * apdu_len)
{
    uint16_t j = 0;
    uint8_t index;
    bool found = false;
    uint8_t *apdu_source;

    if (invokeID) {
        index = tsm_find_invokeID_index(session_object, invokeID);
        /* how much checking is needed?  state?  dest match? just invokeID? */
        if (index < MAX_TSM_TRANSACTIONS) {
            /* FIXME: we may want to free the transaction so it doesn't timeout */
            /* retrieve the transaction */
            /* FIXME: bounds check the pdu_len? */
            /* gets current APDU blob data to send */
            apdu_source =
                get_apdu_blob_data_segment(&session_object->TSM_List[index], 0,
                apdu_len);
            for (j = 0; j < *apdu_len; j++) {
                apdu[j] = apdu_source[j];
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
                    /* Unsegmented : stay in state TSM_STATE_AWAIT_CONFIRMATION 
                       Segmented : come back in state TSM_STATE_SEGMENTED_REQUEST: Re-send all packets */
                    if (get_apdu_max_segments(&session_object->TSM_List[i]) >
                        1) {
                        /* assign the transaction */
                        session_object->TSM_List[i].state =
                            TSM_STATE_SEGMENTED_REQUEST_CLIENT;
                        session_object->TSM_List[i].SegmentRetryCount =
                            apdu_retries(session_object);
                        /* start the timer */
                        session_object->TSM_List[i].RequestTimer = 0;
                        session_object->TSM_List[i].SegmentTimer =
                            apdu_segment_timeout(session_object);
                    }
                    /* Re-send PDU data */
                    tsm_pdu_send(session_object, &session_object->TSM_List[i],
                        0);
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
        if (session_object->TSM_List[i].state == TSM_STATE_SEGMENTED_RESPONSE) {
            /* RequestTimer stopped in this state */
            if (session_object->TSM_List[i].SegmentTimer > milliseconds)
                session_object->TSM_List[i].SegmentTimer -= milliseconds;
            else
                session_object->TSM_List[i].SegmentTimer = 0;
            /* timeout.  retry? */
            if (session_object->TSM_List[i].SegmentTimer == 0) {

                session_object->TSM_List[i].SegmentRetryCount--;
                session_object->TSM_List[i].SegmentTimer =
                    apdu_segment_timeout(session_object);
                if (session_object->TSM_List[i].SegmentRetryCount) {
                    /* Re-send PDU data */
                    FillWindow(session_object, &session_object->TSM_List[i],
                        session_object->TSM_List[i].InitialSequenceNumber);
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
            TSM_STATE_SEGMENTED_REQUEST_SERVER) {
            /* RequestTimer stopped in this state */
            if (session_object->TSM_List[i].SegmentTimer > milliseconds)
                session_object->TSM_List[i].SegmentTimer -= milliseconds;
            else
                session_object->TSM_List[i].SegmentTimer = 0;
            /* timeout : free memory */
            if (session_object->TSM_List[i].SegmentTimer == 0) {
                /* Clear Peer data, if any. Lookup with our internal ID status. */
                tsm_clear_peer_id(session_object,
                    session_object->TSM_List[i].InvokeID);
                /* Release segmented data */
                free_blob(&session_object->TSM_List[i]);

                bacnet_session_lock(session_object);
                /* flag slot as "unused" */
                session_object->TSM_List[i].InvokeID = 0;
                /* free all memory associated */
                session_object->TSM_List[i].state = TSM_STATE_IDLE;
                bacnet_session_unlock(session_object);
            }
        }

        if (session_object->TSM_List[i].state ==
            TSM_STATE_SEGMENTED_REQUEST_CLIENT) {
            /* RequestTimer stopped in this state */
            if (session_object->TSM_List[i].SegmentTimer > milliseconds)
                session_object->TSM_List[i].SegmentTimer -= milliseconds;
            else
                session_object->TSM_List[i].SegmentTimer = 0;
            /* timeout.  retry? */
            if (session_object->TSM_List[i].SegmentTimer == 0) {

                session_object->TSM_List[i].SegmentRetryCount--;
                session_object->TSM_List[i].SegmentTimer =
                    apdu_segment_timeout(session_object);
                if (session_object->TSM_List[i].SegmentRetryCount) {
                    /* Re-send PDU data */
                    FillWindow(session_object, &session_object->TSM_List[i],
                        session_object->TSM_List[i].InitialSequenceNumber);
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
        /* Clear Peer data, if any. Lookup with our internal ID status. */
        tsm_clear_peer_id(session_object, invokeID);
        /*tsm_clear_peer_id( session_object, & session_object->TSM_List[index].dest, session_object->TSM_List[index].InvokeID ); */
        /* flag slot as "unused" */
        session_object->TSM_List[index].InvokeID = 0;

        if (cleanup)
            /* Release segmented data */
            free_blob(&session_object->TSM_List[index]);
    } else
        /* unmatched peer address : could be an attack, packet injection, or data to ignore */
    if ((index < MAX_TSM_TRANSACTIONS)) {
        bacnet_session_log(session_object, 90,
            "FREE: Not releasing transaction: wrong address.", peer_address,
            invokeID);
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

/* get actual timings for current invoke id */
int tsm_invoke_id_timing(
    struct bacnet_session_object *session_object,
    uint8_t invokeID)
{
    int timing = 0;
    uint8_t index;

    bacnet_session_lock(session_object);

    index = tsm_find_invokeID_index(session_object, invokeID);
    if (index < MAX_TSM_TRANSACTIONS) {
        if (session_object->TSM_List[index].state ==
            TSM_STATE_SEGMENTED_RESPONSE)
            timing = session_object->TSM_List[index].SegmentTimer;
        if (session_object->TSM_List[index].state ==
            TSM_STATE_SEGMENTED_REQUEST_CLIENT)
            timing = session_object->TSM_List[index].SegmentTimer;
        if (session_object->TSM_List[index].state ==
            TSM_STATE_AWAIT_CONFIRMATION)
            timing = session_object->TSM_List[index].RequestTimer;
        if (session_object->TSM_List[index].state ==
            TSM_STATE_SEGMENTED_CONFIRMATION)
            timing = session_object->TSM_List[index].SegmentTimer;
    }
    bacnet_session_unlock(session_object);

    return timing;
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
