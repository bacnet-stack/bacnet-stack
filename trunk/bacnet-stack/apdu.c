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
#include <assert.h>
#include "apdu.h"
#include "bacdef.h"
#include "bacenum.h"

// generic apdu function handler
typedef void (*apdu_function)(
  uint8_t *service_request,
  uint16_t len,
  BACNET_ADDRESS *src); 

static apdu_function Unconfirmed_I_Am_Handler = NULL;
static apdu_function Unconfirmed_Who_Is_Handler = NULL;
static apdu_function Unconfirmed_COV_Notification = NULL;
static apdu_function Unconfirmed_I_Have = NULL;
static apdu_function Unconfirmed_Event_Notification = NULL;
static apdu_function Unconfirmed_Private_Transfer = NULL;
static apdu_function Unconfirmed_Text_Message = NULL;
static apdu_function Unconfirmed_Time_Synchronization = NULL;
static apdu_function Unconfirmed_Who_Has = NULL;
static apdu_function Unconfirmed_UTC_Time_Synchronization = NULL;

void apdu_set_unconfirmed_handler(int service, apdu_function pFunction)
{
  switch (service_choice) 
  {
    case SERVICE_UNCONFIRMED_I_AM:
      Unconfirmed_I_Am_Handler = pFunction;
      break;
    case SERVICE_UNCONFIRMED_WHO_IS:
      Unconfirmed_Who_Is_Handler = pFunction;
      break;
    case SERVICE_UNCONFIRMED_COV_NOTIFICATION:
      Unconfirmed_COV_Notification = pFunction;
      break;
    case SERVICE_UNCONFIRMED_I_HAVE:
      Unconfirmed_I_Have = pFunction;
      break;
    case SERVICE_UNCONFIRMED_EVENT_NOTIFICATION:
      Unconfirmed_Event_Notification = pFunction;
      break;
    case SERVICE_UNCONFIRMED_PRIVATE_TRANSFER:
      Unconfirmed_Private_Transfer = pFunction;
      break;
    case SERVICE_UNCONFIRMED_TEXT_MESSAGE:
      Unconfirmed_Text_Message = pFunction;
      break;
    case SERVICE_UNCONFIRMED_TIME_SYNCHRONIZATION:
      Unconfirmed_Time_Synchronization = pFunction;
      break;
    case SERVICE_UNCONFIRMED_WHO_HAS:
      Unconfirmed_Who_Has = pFunction;
      break;
    case SERVICE_UNCONFIRMED_UTC_TIME_SYNCHRONIZATION:
      Unconfirmed_UTC_Time_Synchronization = pFunction;
      break;
    default:
      break;
  }
}

void apdu_handler(
  BACNET_ADDRESS *src,  // source address
  bool data_expecting_reply,
  uint8_t *apdu, // APDU data
  uint16_t pdu_len) // for confirmed messages
{
  uint8_t invoke_id = 0;
  uint8_t *service_request = NULL;
  uint16_t service_request_len = 0;
  bool segmented_message = false;
  bool more_follows = false;
  bool segmented_response_accepted = false;

  if (apdu)
  {
    // PDU Type
    switch (apdu[0] & 0xF0)
    {
      case PDU_TYPE_CONFIRMED_SERVICE_REQUEST:
        segmented_message = (apdu[0] & BIT3) ? true : false;
        more_follows = (apdu[0] & BIT2) ? true : false;
        segmented_response_accepted = (apdu[0] & BIT1) ? true : false;
        break;
      case PDU_TYPE_UNCONFIRMED_SERVICE_REQUEST:
        service_choice = apdu[1];
        service_request = &apdu[2];
        service_request_len = apdu_len - 2;
        switch (service_choice) 
        {
          case SERVICE_UNCONFIRMED_I_AM:
            if (Unconfirmed_I_Am_Handler)
              Unconfirmed_I_Am_Handler(
                service_request,
                service_request_len,
                src);
            break;
          case SERVICE_UNCONFIRMED_WHO_IS:
            Unconfirmed_Who_Is_Handler = pFunction;
            break;
          case SERVICE_UNCONFIRMED_COV_NOTIFICATION:
            Unconfirmed_COV_Notification = pFunction;
            break;
          case SERVICE_UNCONFIRMED_I_HAVE:
            Unconfirmed_I_Have = pFunction;
            break;
          case SERVICE_UNCONFIRMED_EVENT_NOTIFICATION:
            Unconfirmed_Event_Notification = pFunction;
            break;
          case SERVICE_UNCONFIRMED_PRIVATE_TRANSFER:
            Unconfirmed_Private_Transfer = pFunction;
            break;
          case SERVICE_UNCONFIRMED_TEXT_MESSAGE:
            Unconfirmed_Text_Message = pFunction;
            break;
          case SERVICE_UNCONFIRMED_TIME_SYNCHRONIZATION:
            Unconfirmed_Time_Synchronization = pFunction;
            break;
          case SERVICE_UNCONFIRMED_WHO_HAS:
            Unconfirmed_Who_Has = pFunction;
            break;
          case SERVICE_UNCONFIRMED_UTC_TIME_SYNCHRONIZATION:
            Unconfirmed_UTC_Time_Synchronization = pFunction;
            break;
          default:
            break;
        }
        break;
      case PDU_TYPE_SIMPLE_ACK:
      case PDU_TYPE_COMPLEX_ACK:
      case PDU_TYPE_SEGMENT_ACK:
      case PDU_TYPE_ERROR:
      case PDU_TYPE_REJECT:
      case PDU_TYPE_ABORT:
      default:
        break;
    }
  }
  return;
}
