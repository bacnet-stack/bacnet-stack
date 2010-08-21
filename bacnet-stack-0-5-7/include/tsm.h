/*####COPYRIGHTBEGIN####
 -------------------------------------------
 Copyright (C) 2005 Steve Karg

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
#ifndef TSM_H
#define TSM_H

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include "bacdef.h"
#include "npdu.h"
#include "apdu.h"

/* note: TSM functionality is optional - only needed if we are 
   doing client requests */
#if (!MAX_TSM_TRANSACTIONS)
#define tsm_free_invoke_id_check(x,y,z) do{(void)x;}while(0)
#else
typedef enum {
    TSM_STATE_ALLOCATED,        /* Freshly allocated state */
    TSM_STATE_IDLE,
    TSM_STATE_AWAIT_CONFIRMATION,
    TSM_STATE_AWAIT_RESPONSE,
    TSM_STATE_SEGMENTED_REQUEST,
    TSM_STATE_SEGMENTED_CONFIRMATION
} BACNET_TSM_STATE;

/* 5.4.1 Variables And Parameters */
/* The following variables are defined for each instance of  */
/* Transaction State Machine: */
typedef struct BACnet_TSM_Data {
    /* used to count APDU retries */
    uint8_t RetryCount;
    /* used to count segment retries */
    uint8_t SegmentRetryCount;
    /* used to control APDU retries and the acceptance of server replies */
    bool SentAllSegments;
    /* stores the sequence number of the last segment received in order */
    uint8_t LastSequenceNumber;
    /* stores the sequence number of the first segment of */
    /* a sequence of segments that fill a window */
    uint8_t InitialSequenceNumber;
    /* stores the current window size */
    uint8_t ActualWindowSize;
    /* stores the window size proposed by the segment sender */
    uint8_t ProposedWindowSize;
    /*  used to perform timeout on PDU segments */
    uint16_t SegmentTimer;
    /* used to perform timeout on Confirmed Requests */
    /* in milliseconds */
    uint16_t RequestTimer;
    /* unique id */
    uint8_t InvokeID;
    /* state that the TSM is in */
    BACNET_TSM_STATE state;
    /* the address we sent it to */
    BACNET_ADDRESS dest;
    /* the network layer info */
    BACNET_NPDU_DATA npdu_data;
    /* copy of the APDU, should we need to send it again */
    uint8_t apdu[MAX_PDU];
    unsigned apdu_len;
    /* Multiple APDU segments blob memorized here */
    uint8_t *apdu_blob;
    /* Size of allocated Multiple APDU segments blob */
    uint32_t apdu_blob_allocated;
    /* Size of data within the multiple APDU segments blob  */
    uint32_t apdu_blob_size;
    /* Count received segments (prevents D.O.S.) */
    uint32_t ReceivedSegmentsCount;
} BACNET_TSM_DATA;



#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

    /*/ Free allocated blob data */
    void free_blob(
        BACNET_TSM_DATA * data);
    /*/ keeps allocated blob data, but reset data & current size */
    void reset_blob(
        BACNET_TSM_DATA * data);
    /*/ add new data to current blob (allocate extra space if necessary) */
    void add_blob_data(
        BACNET_TSM_DATA * data,
        uint8_t * pdata,
        uint32_t data_len);
    /*/ gets current blob data */
    uint8_t *get_blob_data(
        BACNET_TSM_DATA * data,
        uint32_t * data_len);
    /*/ allocate new data if necessary, keeps existing bytes */
    void ensure_extra_blob_size(
        BACNET_TSM_DATA * data,
        uint32_t allocation_unit);

    uint8_t tsm_transaction_idle_count(
        struct bacnet_session_object *session_object);
    void tsm_timer_milliseconds(
        struct bacnet_session_object *session_object,
        uint16_t milliseconds);
/* free the invoke ID when the reply comes back */
    void tsm_free_invoke_id_check(
        struct bacnet_session_object *session_object,
        uint8_t invokeID,
        BACNET_ADDRESS * peer_address,
        bool cleanup_segmented_data);
    /* also cleanup segmented data */
    /*void tsm_free_invoke_id_and_cleanup( */
    /*    struct bacnet_session_object * session_object, */
    /*    uint8_t invokeID); */

/* use these in tandem */
    uint8_t tsm_next_free_invokeID(
        struct bacnet_session_object *session_object);
/* returns the same invoke ID that was given */
    void tsm_set_confirmed_unsegmented_transaction(
        struct bacnet_session_object *session_object,
        uint8_t invokeID,
        BACNET_ADDRESS * dest,
        BACNET_NPDU_DATA * ndpu_data,
        uint8_t * apdu,
        uint16_t apdu_len);
/* returns true if transaction is found */
    bool tsm_get_transaction_pdu(
        struct bacnet_session_object *session_object,
        uint8_t invokeID,
        BACNET_ADDRESS * dest,
        BACNET_NPDU_DATA * ndpu_data,
        uint8_t * apdu,
        uint16_t * apdu_len);

    bool tsm_invoke_id_free(
        struct bacnet_session_object *session_object,
        uint8_t invokeID);
    bool tsm_invoke_id_failed(
        struct bacnet_session_object *session_object,
        uint8_t invokeID);

/* changes invokeID */
    void tsm_invokeID_set(
        struct bacnet_session_object *session_object,
        uint8_t invokeID);

    bool tsm_set_segmentedcomplexack_received(
        struct bacnet_session_object *session_object,
        BACNET_ADDRESS * src,
        BACNET_CONFIRMED_SERVICE_ACK_DATA * service_data,
        uint8_t ** pservice_request,    /* IN/OUT */
        uint32_t * pservice_request_len /* IN/OUT */
        );


#ifdef __cplusplus
}
#endif /* __cplusplus */
/* define out any functions necessary for compile */
#endif
#endif
