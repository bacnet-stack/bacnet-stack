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
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h> // memmove()
#include "bits.h"
#include "apdu.h"
#include "bacdef.h"
#include "bacdcode.h"
#include "bacenum.h"
#include "tsm.h"
#include "config.h"
#include "device.h"
#include "datalink.h"
#include "handlers.h"
#include "address.h"

// Transaction State Machine
// Really only needed for segmented messages
// and a little for sending confirmed messages
// If we are only a server and only initiate broadcasts,
// then we don't need a TSM layer.

// FIXME: not coded for segmentation

// declare space for the TSM transactions, and set it up in the init.
static BACNET_TSM_DATA TSM_List[MAX_TSM_TRANSACTIONS] = {{0}};

// returns MAX_TSM_TRANSACTIONS if not found
uint8_t tsm_find_invokeID_index(uint8_t invokeID)
{
  unsigned i = 0; // counter
  uint8_t index = MAX_TSM_TRANSACTIONS; // return value

  for (i = 0; i < MAX_TSM_TRANSACTIONS; i++)
  {
    if ((TSM_List[i].state != TSM_STATE_IDLE) &&
        (TSM_List[i].InvokeID == invokeID))
    {
      index = i;
      break;
    }
  }

  return index;
}

bool tsm_transaction_available(void)
{
  bool status = false; // return value
  unsigned i = 0; // counter
    
  for (i = 0; i < MAX_TSM_TRANSACTIONS; i++)
  {
    if (TSM_List[i].state == TSM_STATE_IDLE)
    {
      // one is available!
      status = true;
      break;
    }
  }

  return status;
}

uint8_t tsm_transaction_idle_count(void)
{
  uint8_t count = 0; // return value
  unsigned i = 0; // counter
    
  for (i = 0; i < MAX_TSM_TRANSACTIONS; i++)
  {
    if (TSM_List[i].state == TSM_STATE_IDLE)
    {
      // one is available!
      count++;
    }
  }

  return count;
}

uint8_t tsm_next_free_invokeID(void)
{
  static uint8_t current_invokeID = 1; // incremented...
  uint8_t index = 0;
  uint8_t invokeID = 0;
  bool found = false;

  while (!found)
  {
    index = tsm_find_invokeID_index(current_invokeID);
    if (index == MAX_TSM_TRANSACTIONS)
    {
      found = true;
      invokeID = current_invokeID;
    }
    current_invokeID++;
    // skip zero - we treat that internally as invalid or no free
    if (current_invokeID == 0)
      current_invokeID++;
  }
  
  return invokeID;
}

void tsm_set_confirmed_unsegmented_transaction(
  uint8_t invokeID,
  BACNET_ADDRESS *dest,
  uint8_t *pdu,
  uint16_t pdu_len)
{
  uint16_t j = 0;
  uint8_t index;

  if (invokeID)
  {
    index = tsm_find_invokeID_index(invokeID);
    if (index < MAX_TSM_TRANSACTIONS)
    {
      // assign the transaction
      TSM_List[index].state = TSM_STATE_AWAIT_CONFIRMATION;
      TSM_List[index].RetryCount = Device_Number_Of_APDU_Retries();
      // start the timer
      TSM_List[index].RequestTimer = Device_APDU_Timeout();
      // copy the data
      for (j = 0; j < pdu_len; j++)
      {
        TSM_List[index].pdu[j] = pdu[j];
      }
      TSM_List[index].pdu_len = pdu_len;
      address_copy(&TSM_List[index].dest,dest);
    }
  }

  return;  
}

// used to retrieve the transaction payload
// if we wanted to find out what we sent (i.e. when we get an ack)
bool tsm_get_transaction_pdu(
  uint8_t invokeID,
  BACNET_ADDRESS *dest,
  uint8_t *pdu,
  uint16_t *pdu_len)
{
  uint16_t j = 0;
  uint8_t index;
  bool found = false;

  if (invokeID)
  {
    index = tsm_find_invokeID_index(invokeID);
    // how much checking is needed?  state?  dest match? just invokeID?
    if (index < MAX_TSM_TRANSACTIONS)
    {
      // FIXME: we may want to free the transaction so it doesn't timeout
      // retrieve the transaction
      // FIXME: bounds check the pdu_len?
      *pdu_len = TSM_List[index].pdu_len;
      for (j = 0; j < *pdu_len; j++)
      {
        pdu[j] = TSM_List[index].pdu[j];
      }
      address_copy(dest,&TSM_List[index].dest);
      found = true;
    }
  }

  return found;  
}

// called once a millisecond
void tsm_timer_milliseconds(uint16_t milliseconds)
{
  unsigned i = 0; // counter
  int bytes_sent = 0;
  
  for (i = 0; i < MAX_TSM_TRANSACTIONS; i++)
  {
    if (TSM_List[i].state == TSM_STATE_AWAIT_CONFIRMATION)
    {
      if (TSM_List[i].RequestTimer > milliseconds)
        TSM_List[i].RequestTimer -= milliseconds;
      else
        TSM_List[i].RequestTimer = 0;
      // timeout.  retry?
      if (TSM_List[i].RequestTimer == 0)
      {
        TSM_List[i].RetryCount--;
        TSM_List[i].RequestTimer = Device_APDU_Timeout();
        if (TSM_List[i].RetryCount)
        {
          bytes_sent = datalink_send_pdu(
            &TSM_List[i].dest,  // destination address
            &TSM_List[i].pdu[0],
            TSM_List[i].pdu_len); // number of bytes of data
        }
        else
          TSM_List[i].state = TSM_STATE_IDLE;
      }
    }
  }
}

void tsm_free_invoke_id(uint8_t invokeID)
{
  uint8_t index;
  
  index = tsm_find_invokeID_index(invokeID);
  if (index < MAX_TSM_TRANSACTIONS)
    TSM_List[index].state = TSM_STATE_IDLE;
}

#ifdef TEST
#include <assert.h>
#include <string.h>
#include "ctest.h"

void testTSM(Test * pTest)
{
  //unsigned i;
  uint8_t invokeID = 0;
  BACNET_ADDRESS dest = {0};
  uint8_t pdu[MAX_PDU] = {0};
  uint16_t pdu_len = 0;

  memset(pdu,0xa5,sizeof(pdu));
  pdu_len = sizeof(pdu);
  
  invokeID = tsm_request_confirmed_unsegmented_transaction(
    &dest,
    &pdu[0],
    pdu_len);
  ct_test(pTest, invokeID != 0);

  return;
}

#ifdef TEST_TSM
int main(void)
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
#endif                          /* TEST_TSM */
#endif                          /* TEST */

