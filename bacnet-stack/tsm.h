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

typedef enum
{
  TSM_STATE_IDLE,
  TSM_STATE_AWAIT_CONFIRMATION,
  TSM_STATE_AWAIT_RESPONSE,
  TSM_STATE_SEGMENTED_REQUEST,
  TSM_STATE_SEGMENTED_CONFIRMATION,
} BACNET_TSM_STATE;

// 5.4.1 Variables And Parameters
// The following variables are defined for each instance of 
// Transaction State Machine:
typedef struct BACnet_TSM_Data
{
  // used to count APDU retries
  uint8_t RetryCount; 
  // used to count segment retries
  //uint8_t SegmentRetryCount; 
  // used to control APDU retries and the acceptance of server replies
  //bool SentAllSegments; 
  // stores the sequence number of the last segment received in order
  //uint8_t LastSequenceNumber;
  // stores the sequence number of the first segment of
  // a sequence of segments that fill a window
  //uint8_t InitialSequenceNumber;
  // stores the current window size
  //uint8_t ActualWindowSize;
  // stores the window size proposed by the segment sender
  //uint8_t ProposedWindowSize; 
  //  used to perform timeout on PDU segments
  //uint8_t SegmentTimer;
  // used to perform timeout on Confirmed Requests
  // in milliseconds
  uint16_t RequestTimer; 
  // unique id
  uint8_t InvokeID;
  // state that the TSM is in
  BACNET_TSM_STATE state;
  // the address we sent it to
  BACNET_ADDRESS dest;
  // copy of the PDU, should we need to send it again
  uint8_t pdu[MAX_PDU];
  unsigned pdu_len;
} BACNET_TSM_DATA;

bool tsm_transaction_available(void);
uint8_t tsm_transaction_idle_count(void);
void tsm_timer_milliseconds(uint16_t milliseconds);
// free the invoke ID when the reply comes back
void tsm_free_invoke_id(uint8_t invokeID);
// use these in tandem
uint8_t tsm_next_free_invokeID(void);
// returns the same invoke ID that was given
void tsm_set_confirmed_unsegmented_transaction(
  uint8_t invokeID,
  BACNET_ADDRESS *dest,
  uint8_t *pdu,
  uint16_t pdu_len);
// returns true if transaction is found
bool tsm_get_transaction_pdu(
  uint8_t invokeID,
  BACNET_ADDRESS *dest,
  uint8_t *pdu,
  uint16_t *pdu_len);

#endif

