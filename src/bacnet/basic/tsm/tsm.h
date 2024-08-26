/**************************************************************************
*
* Copyright (C) 2012 Steve Karg <skarg@users.sourceforge.net>
*
* SPDX-License-Identifier: MIT
*
*********************************************************************/
#ifndef BACNET_BASIC_TSM_TSM_H
#define BACNET_BASIC_TSM_TSM_H

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
/* BACnet Stack defines - first */
#include "bacnet/bacdef.h"
/* BACnet Stack API */
#include "bacnet/npdu.h"

/* note: TSM functionality is optional - only needed if we are
   doing client requests */

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

    /* FIXME: modify basic service handlers to use TSM rather than this buffer! */
    BACNET_STACK_EXPORT extern
    uint8_t Handler_Transmit_Buffer[MAX_PDU];

#ifdef __cplusplus
}
#endif /* __cplusplus */


#if (!MAX_TSM_TRANSACTIONS)
#define tsm_free_invoke_id(x) (void)x;
#else
typedef enum {
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
    /*uint8_t SegmentRetryCount;  */
    /* used to control APDU retries and the acceptance of server replies */
    /*bool SentAllSegments;  */
    /* stores the sequence number of the last segment received in order */
    /*uint8_t LastSequenceNumber; */
    /* stores the sequence number of the first segment of */
    /* a sequence of segments that fill a window */
    /*uint8_t InitialSequenceNumber; */
    /* stores the current window size */
    /*uint8_t ActualWindowSize; */
    /* stores the window size proposed by the segment sender */
    /*uint8_t ProposedWindowSize;  */
    /*  used to perform timeout on PDU segments */
    /*uint8_t SegmentTimer; */
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
} BACNET_TSM_DATA;

typedef void (
    *tsm_timeout_function) (
    uint8_t invoke_id);


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

    BACNET_STACK_EXPORT
    void tsm_set_timeout_handler(
        tsm_timeout_function pFunction);

    BACNET_STACK_EXPORT
    bool tsm_transaction_available(
        void);
    BACNET_STACK_EXPORT
    uint8_t tsm_transaction_idle_count(
        void);
    BACNET_STACK_EXPORT
    void tsm_timer_milliseconds(
        uint16_t milliseconds);
/* free the invoke ID when the reply comes back */
    BACNET_STACK_EXPORT
    void tsm_free_invoke_id(
        uint8_t invokeID);
/* use these in tandem */
    BACNET_STACK_EXPORT
    uint8_t tsm_next_free_invokeID(
        void);
    BACNET_STACK_EXPORT
    void tsm_invokeID_set(
        uint8_t invokeID);
/* returns the same invoke ID that was given */
    BACNET_STACK_EXPORT
    void tsm_set_confirmed_unsegmented_transaction(
        uint8_t invokeID,
        BACNET_ADDRESS * dest,
        BACNET_NPDU_DATA * ndpu_data,
        uint8_t * apdu,
        uint16_t apdu_len);
/* returns true if transaction is found */
    BACNET_STACK_EXPORT
    bool tsm_get_transaction_pdu(
        uint8_t invokeID,
        BACNET_ADDRESS * dest,
        BACNET_NPDU_DATA * ndpu_data,
        uint8_t * apdu,
        uint16_t * apdu_len);

    BACNET_STACK_EXPORT
    bool tsm_invoke_id_free(
        uint8_t invokeID);
    BACNET_STACK_EXPORT
    bool tsm_invoke_id_failed(
        uint8_t invokeID);

#ifdef __cplusplus
}
#endif /* __cplusplus */
/* define out any functions necessary for compile */
#endif
#endif
