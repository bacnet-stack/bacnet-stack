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
#ifndef APDU_H
#define APDU_H

#include <stdbool.h>
#include <stdint.h>
#include "bacdef.h"
#include "bacenum.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

typedef struct _confirmed_service_data 
{
  bool segmented_message;
  bool more_follows;
  bool segmented_response_accepted;
  int max_segs;
  int max_resp;
  uint8_t invoke_id;
  uint8_t sequence_number;
  uint8_t proposed_window_number;
} BACNET_CONFIRMED_SERVICE_DATA;

typedef struct _confirmed_service_ack_data 
{
  bool segmented_message;
  bool more_follows;
  uint8_t invoke_id;
  uint8_t sequence_number;
  uint8_t proposed_window_number;
} BACNET_CONFIRMED_SERVICE_ACK_DATA;

// generic unconfirmed function handler
// Suitable to handle the following services:
// I_Am, Who_Is, Unconfirmed_COV_Notification, I_Have,
// Unconfirmed_Event_Notification, Unconfirmed_Private_Transfer,
// Unconfirmed_Text_Message, Time_Synchronization, Who_Has,
// UTC_Time_Synchronization
typedef void (*unconfirmed_function)(
  uint8_t *service_request,
  uint16_t len,
  BACNET_ADDRESS *src); 

// generic confirmed function handler
// Suitable to handle the following services:
// Acknowledge_Alarm, Confirmed_COV_Notification,
// Confirmed_Event_Notification, Get_Alarm_Summary,
// Get_Enrollment_Summary_Handler, Get_Event_Information,
// Subscribe_COV_Handler, Subscribe_COV_Property,
// Life_Safety_Operation, Atomic_Read_File,
// Confirmed_Atomic_Write_File, Add_List_Element,
// Remove_List_Element, Create_Object_Handler,
// Delete_Object_Handler, Read_Property,
// Read_Property_Conditional, Read_Property_Multiple, Read_Range,
// Write_Property, Write_Property_Multiple,
// Device_Communication_Control, Confirmed_Private_Transfer,
// Confirmed_Text_Message, Reinitialize_Device,
// VT_Open, VT_Close, VT_Data_Handler,
// Authenticate, Request_Key
typedef void (*confirmed_function)(
  uint8_t *service_request,
  uint16_t service_len,
  BACNET_ADDRESS *src,
  BACNET_CONFIRMED_SERVICE_DATA *service_data); 

// generic confirmed simple ack function handler
typedef void (*confirmed_simple_ack_function)(
  BACNET_ADDRESS *src,
  uint8_t invoke_id);

// generic confirmed ack function handler
typedef void (*confirmed_ack_function)(
  uint8_t *service_request,
  uint16_t service_len,
  BACNET_ADDRESS *src,
  BACNET_CONFIRMED_SERVICE_ACK_DATA *service_data); 

void apdu_set_confirmed_ack_handler(
  BACNET_CONFIRMED_SERVICE service_choice,
  confirmed_ack_function pFunction);

void apdu_set_confirmed_simple_ack_handler(
  BACNET_CONFIRMED_SERVICE service_choice,
  confirmed_simple_ack_function pFunction);

// configure reject for confirmed services that are not supported
void apdu_set_unrecognized_service_handler_handler(
  confirmed_function pFunction);

void apdu_set_confirmed_handler(
  BACNET_CONFIRMED_SERVICE service_choice,
  confirmed_function pFunction);

void apdu_set_unconfirmed_handler(
  BACNET_UNCONFIRMED_SERVICE service_choice,
  unconfirmed_function pFunction);

uint16_t apdu_decode_confirmed_service_request(
  uint8_t *apdu, // APDU data
  uint16_t apdu_len,
  BACNET_CONFIRMED_SERVICE_DATA *service_data,
  uint8_t *service_choice,
  uint8_t **service_request,
  uint16_t *service_request_len);

void apdu_handler(
  BACNET_ADDRESS *src,  // source address
  bool data_expecting_reply,
  uint8_t *apdu, // APDU data
  uint16_t pdu_len); // for confirmed messages

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
