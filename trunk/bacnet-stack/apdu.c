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

// generic unconfirmed function handler
typedef void (*unconfirmed_function)(
  uint8_t *service_request,
  uint16_t len,
  BACNET_ADDRESS *src); 

static unconfirmed_function Unconfirmed_I_Am_Handler = NULL;
static unconfirmed_function Unconfirmed_Who_Is_Handler = NULL;
static unconfirmed_function Unconfirmed_COV_Notification_Handler = NULL;
static unconfirmed_function Unconfirmed_I_Have_Handler = NULL;
static unconfirmed_function Unconfirmed_Event_Notification_Handler = NULL;
static unconfirmed_function Unconfirmed_Private_Transfer_Handler = NULL;
static unconfirmed_function Unconfirmed_Text_Message_Handler = NULL;
static unconfirmed_function Unconfirmed_Time_Synchronization_Handler = NULL;
static unconfirmed_function Unconfirmed_Who_Has_Handler = NULL;
static unconfirmed_function Unconfirmed_UTC_Time_Synchronization_Handler = NULL;

void apdu_set_unconfirmed_handler(
  BACNET_UNCONFIRMED_SERVICE service_choice,
  unconfirmed_function pFunction)
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
      Unconfirmed_COV_Notification_Handler = pFunction;
      break;
    case SERVICE_UNCONFIRMED_I_HAVE:
      Unconfirmed_I_Have_Handler = pFunction;
      break;
    case SERVICE_UNCONFIRMED_EVENT_NOTIFICATION:
      Unconfirmed_Event_Notification_Handler = pFunction;
      break;
    case SERVICE_UNCONFIRMED_PRIVATE_TRANSFER:
      Unconfirmed_Private_Transfer_Handler = pFunction;
      break;
    case SERVICE_UNCONFIRMED_TEXT_MESSAGE:
      Unconfirmed_Text_Message_Handler = pFunction;
      break;
    case SERVICE_UNCONFIRMED_TIME_SYNCHRONIZATION:
      Unconfirmed_Time_Synchronization_Handler = pFunction;
      break;
    case SERVICE_UNCONFIRMED_WHO_HAS:
      Unconfirmed_Who_Has_Handler = pFunction;
      break;
    case SERVICE_UNCONFIRMED_UTC_TIME_SYNCHRONIZATION:
      Unconfirmed_UTC_Time_Synchronization_Handler = pFunction;
      break;
    default:
      break;
  }
}

// generic unconfirmed function handler
typedef void (*confirmed_function)(
  uint8_t *service_request,
  uint16_t len,
  BACNET_ADDRESS *src
  uint8_t invoke_id); 

static confirmed_function Confirmed_Acknowledge_Alarm_Handler = NULL;
static confirmed_function Confirmed_COV_Notification_Handler = NULL;
static confirmed_function Confirmed_Event_Notification_Handler = NULL;
static confirmed_function Confirmed_Get_Alarm_Summary_Handler = NULL;
static confirmed_function Confirmed_Get_Enrollment_Summary_Handler = NULL;
static confirmed_function Confirmed_Get_Event_Information_Handler = NULL;
static confirmed_function Confirmed_Subscribe_COV_Handler = NULL;
static confirmed_function Confirmed_Subscribe_COV_Property_Handler = NULL;
static confirmed_function Confirmed_Life_Safety_Operation_Handler = NULL;
static confirmed_function Confirmed_Atomic_Read_File_Handler = NULL;
static confirmed_function Confirmed_Atomic_Write_File_Handler = NULL;
static confirmed_function Confirmed_Add_List_Element_Handler = NULL;
static confirmed_function Confirmed_Remove_List_Element_Handler = NULL;
static confirmed_function Confirmed_Create_Object_Handler = NULL;
static confirmed_function Confirmed_Delete_Object_Handler = NULL;
static confirmed_function Confirmed_Read_Property_Handler = NULL;
static confirmed_function Confirmed_Read_Property_Conditional_Handler = NULL;
static confirmed_function Confirmed_Read_Property_Multiple_Handler = NULL;
static confirmed_function Confirmed_Read_Range_Handler = NULL;
static confirmed_function Confirmed_Write_Property_Handler = NULL;
static confirmed_function Confirmed_Write_Property_Multiple_Handler = NULL;
static confirmed_function Confirmed_Device_Communication_Control_Handler = NULL;
static confirmed_function Confirmed_Private_Transfer_Handler = NULL;
static confirmed_function Confirmed_Text_Message_Handler = NULL;
static confirmed_function Confirmed_Reinitialize_Device_Handler = NULL;
static confirmed_function Confirmed_VT_Open_Handler = NULL;
static confirmed_function Confirmed_VT_Close_Handler = NULL;
static confirmed_function Confirmed_VT_Data_Handler = NULL;
static confirmed_function Confirmed_Authenticate_Handler = NULL;
static confirmed_function Confirmed_Request_Key_Handler = NULL;
  
void apdu_set_confirmed_handler(
  BACNET_CONFIRMED_SERVICE service_choice,
  confirmed_function pFunction)
{
  switch (service_choice) 
  {
    case SERVICE_CONFIRMED_ACKNOWLEDGE_ALARM:
      Confirmed_Acknowledge_Alarm_Handler = pFunction;
      break;
    case SERVICE_CONFIRMED_COV_NOTATION:
      Confirmed_COV_Notification_Handler = pFunction;
      break;
    case SERVICE_CONFIRMED_EVENT_NOTATION:
      Confirmed_Event_Notification_Handler = pFunction;
      break;
    case SERVICE_CONFIRMED_GET_ALARM_SUMMARY:
      Confirmed_Get_Alarm_Summary_Handler = pFunction;
      break;
    case SERVICE_CONFIRMED_GET_ENROLLMENT_SUMMARY:
      Confirmed_Get_Enrollment_Summary_Handler = pFunction;
      break;
    case SERVICE_CONFIRMED_GET_EVENT_INFORMATION:
      Confirmed_Get_Event_Information_Handler = pFunction;
      break;
    case SERVICE_CONFIRMED_SUBSCRIBE_COV:
      Confirmed_Subscribe_COV_Handler = pFunction;
      break;
    case SERVICE_CONFIRMED_SUBSCRIBE_COV_PROPERTY:
      Confirmed_Subscribe_COV_Property_Handler = pFunction;
      break;
    case SERVICE_CONFIRMED_LSAFETY_OPERATION:
      if (Confirmed_Life_Safety_Operation_Handler = pFunction;
      break;
    // File Access Services
    case SERVICE_CONFIRMED_ATOMIC_READ_FILE:
      Confirmed_Atomic_Read_File_Handler = pFunction;
      break;
    case SERVICE_CONFIRMED_ATOMIC_WRITE_FILE:
      Confirmed_Atomic_Write_File_Handler = pFunction;
      break;
    // Object Access Services
    case SERVICE_CONFIRMED_ADD_LIST_ELEMENT:
      Confirmed_Add_List_Element_Handler = pFunction;
      break;
    case SERVICE_CONFIRMED_REMOVE_LIST_ELEMENT:
      Confirmed_Remove_List_Element_Handler = pFunction;
      break;
    case SERVICE_CONFIRMED_CREATE_OBJECT:
      Confirmed_Create_Object_Handler = pFunction;
      break;
    case SERVICE_CONFIRMED_DELETE_OBJECT:
      Confirmed_Delete_Object_Handler = pFunction;
      break;
    case SERVICE_CONFIRMED_READ_PROPERTY:
      Confirmed_Read_Property_Handler = pFunction;
      break;
    case SERVICE_CONFIRMED_READ_PROPERTY_CONDITIONAL:
      Confirmed_Read_Property_Conditional_Handler = pFunction;
      break;
    case SERVICE_CONFIRMED_READ_PROPERTY_MULTIPLE:
      Confirmed_Read_Property_Multiple_Handler = pFunction;
      break;
    case SERVICE_CONFIRMED_READ_RANGE:
      Confirmed_Read_Range_Handler = pFunction;
      break;
    case SERVICE_CONFIRMED_WRITE_PROPERTY:
      Confirmed_Write_Property_Handler = pFunction;
      break;
    case SERVICE_CONFIRMED_WRITE_PROPERTY_MULTIPLE:
      Confirmed_Write_Property_Multiple_Handler = pFunction;
      break;
    // Remote Device Management Services
    case SERVICE_CONFIRMED_DEVICE_COMMUNICATION_CONTROL:
      Confirmed_Device_Communication_Control_Handler = pFunction;
      break;
    case SERVICE_CONFIRMED_PRIVATE_TRANSFER:
      Confirmed_Private_Transfer_Handler = pFunction;
      break;
    case SERVICE_CONFIRMED_TEXT_MESSAGE:
      Confirmed_Text_Message_Handler = pFunction;
      break;
    case SERVICE_CONFIRMED_REINITIALIZE_DEVICE:
      Confirmed_Reinitialize_Device_Handler = pFunction;
      break;
    // Virtual Terminal Services
    case SERVICE_CONFIRMED_VT_OPEN:
      Confirmed_VT_Open_Handler = pFunction;
      break;
    case SERVICE_CONFIRMED_VT_CLOSE:
      Confirmed_VT_Close_Handler = pFunction;
      break;
    case SERVICE_CONFIRMED_VT_DATA:
      Confirmed_VT_Data_Handler = pFunction;
      break;
    // Security Services
    case SERVICE_CONFIRMED_AUTHENTICATE:
      Confirmed_Authenticate_Handler = pFunction;
      break;
    case SERVICE_CONFIRMED_REQUEST_KEY:
      Confirmed_Request_Key_Handler = pFunction;
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
        //FIXME: get the correct info
        service_choice = apdu[1];
        service_request = &apdu[2];
        service_request_len = apdu_len - 2;
        switch (service_choice) 
        {
          case SERVICE_CONFIRMED_ACKNOWLEDGE_ALARM:
            if (Confirmed_Acknowledge_Alarm_Handler)
              Confirmed_Acknowledge_Alarm_Handler(
                service_request,
                service_request_len,
                src
                invoke_id); 
            else
            {
              //FIXME:  reject: service not supported
            }
            break;
          case SERVICE_CONFIRMED_COV_NOTIFICATION:
            if (Confirmed_COV_Notification_Handler)
              Confirmed_COV_Notification_Handler(
                service_request,
                service_request_len,
                src
                invoke_id); 
            break;
          case SERVICE_CONFIRMED_EVENT_NOTIFICATION:
            if (Confirmed_Event_Notification_Handler)
              Confirmed_Event_Notification_Handler(
            break;
          case SERVICE_CONFIRMED_GET_ALARM_SUMMARY:
            if (Confirmed_Get_Alarm_Summary_Handler)
              Confirmed_Get_Alarm_Summary_Handler(
                service_request,
                service_request_len,
                src
                invoke_id); 
            break;
          case SERVICE_CONFIRMED_GET_ENROLLMENT_SUMMARY:
            if (Confirmed_Get_Enrollment_Summary_Handler)
              Confirmed_Get_Enrollment_Summary_Handler(
                service_request,
                service_request_len,
                src
                invoke_id); 
            break;
          case SERVICE_CONFIRMED_GET_EVENT_INFORMATION:
            if (Confirmed_Get_Event_Information_Handler)
              Confirmed_Get_Event_Information_Handler(
                service_request,
                service_request_len,
                src
                invoke_id); 
            break;
          case SERVICE_CONFIRMED_SUBSCRIBE_COV:
            if (Confirmed_Subscribe_COV_Handler)
              Confirmed_Subscribe_COV_Handler(
                service_request,
                service_request_len,
                src
                invoke_id); 
            break;
          case SERVICE_CONFIRMED_SUBSCRIBE_COV_PROPERTY:
            if (Confirmed_Subscribe_COV_Property_Handler)
              Confirmed_Subscribe_COV_Property_Handler(
                service_request,
                service_request_len,
                src
                invoke_id); 
            break;
          case SERVICE_CONFIRMED_LIFE_SAFETY_OPERATION:
            if (Confirmed_Life_Safety_Operation_Handler)
              Confirmed_Life_Safety_Operation_Handler(
                service_request,
                service_request_len,
                src
                invoke_id); 
            break;
          // File Access Services
          case SERVICE_CONFIRMED_ATOMIC_READ_FILE:
            if (Confirmed_Atomic_Read_File_Handler)
              Confirmed_Atomic_Read_File_Handler(
                service_request,
                service_request_len,
                src
                invoke_id); 
            break;
          case SERVICE_CONFIRMED_ATOMIC_WRITE_FILE:
            if (Confirmed_Atomic_Write_File_Handler)
              Confirmed_Atomic_Write_File_Handler(
                service_request,
                service_request_len,
                src
                invoke_id); 
            break;
          // Object Access Services
          case SERVICE_CONFIRMED_ADD_LIST_ELEMENT:
            if (Confirmed_Add_List_Element_Handler)
              Confirmed_Add_List_Element_Handler(
                service_request,
                service_request_len,
                src
                invoke_id); 
            break;
          case SERVICE_CONFIRMED_REMOVE_LIST_ELEMENT:
            if (Confirmed_Remove_List_Element_Handler)
              Confirmed_Remove_List_Element_Handler(
                service_request,
                service_request_len,
                src
                invoke_id); 
            break;
          case SERVICE_CONFIRMED_CREATE_OBJECT:
            if (Confirmed_Create_Object_Handler)
              Confirmed_Create_Object_Handler(
                service_request,
                service_request_len,
                src
                invoke_id); 
            break;
          case SERVICE_CONFIRMED_DELETE_OBJECT:
            if (Confirmed_Delete_Object_Handler)
              Confirmed_Delete_Object_Handler(
                service_request,
                service_request_len,
                src
                invoke_id); 
            break;
          case SERVICE_CONFIRMED_READ_PROPERTY:
            if (Confirmed_Read_Property_Handler)
              Confirmed_Read_Property_Handler(
                service_request,
                service_request_len,
                src
                invoke_id); 
            break;
          case SERVICE_CONFIRMED_READ_PROPERTY_CONDITIONAL:
            if (Confirmed_Read_Property_Conditional_Handler)
              Confirmed_Read_Property_Conditional_Handler(
                service_request,
                service_request_len,
                src
                invoke_id); 
            break;
          case SERVICE_CONFIRMED_READ_PROPERTY_MULTIPLE:
            if (Confirmed_Read_Property_Multiple_Handler)
              Confirmed_Read_Property_Multiple_Handler(
                service_request,
                service_request_len,
                src
                invoke_id); 
            break;
          case SERVICE_CONFIRMED_READ_RANGE:
            if (Confirmed_Read_Range_Handler)
              Confirmed_Read_Range_Handler(
                service_request,
                service_request_len,
                src
                invoke_id); 
            break;
          case SERVICE_CONFIRMED_WRITE_PROPERTY:
            if (Confirmed_Write_Property_Handler)
              Confirmed_Write_Property_Handler(
                service_request,
                service_request_len,
                src
                invoke_id); 
            break;
          case SERVICE_CONFIRMED_WRITE_PROPERTY_MULTIPLE:
            if (Confirmed_Write_Property_Multiple_Handler)
              Confirmed_Write_Property_Multiple_Handler(
                service_request,
                service_request_len,
                src
                invoke_id); 
            break;
          // Remote Device Management Services
          case SERVICE_CONFIRMED_DEVICE_COMMUNICATION_CONTROL:
            if (Confirmed_Device_Communication_Control_Handler)
              Confirmed_Device_Communication_Control_Handler(
                service_request,
                service_request_len,
                src
                invoke_id); 
            break;
          case SERVICE_CONFIRMED_PRIVATE_TRANSFER:
            if (Confirmed_Private_Transfer_Handler)
              Confirmed_Private_Transfer_Handler(
                service_request,
                service_request_len,
                src
                invoke_id); 
            break;
          case SERVICE_CONFIRMED_TEXT_MESSAGE:
            if (Confirmed_Text_Message_Handler)
              Confirmed_Text_Message_Handler(
                service_request,
                service_request_len,
                src
                invoke_id); 
            break;
          case SERVICE_CONFIRMED_REINITIALIZE_DEVICE:
            if (Confirmed_Reinitialize_Device_Handler)
              Confirmed_Reinitialize_Device_Handler(
                service_request,
                service_request_len,
                src
                invoke_id); 
            break;
          // Virtual Terminal Services
          case SERVICE_CONFIRMED_VT_OPEN:
            if (Confirmed_VT_Open_Handler)
              Confirmed_VT_Open_Handler(
                service_request,
                service_request_len,
                src
                invoke_id); 
            break;
          case SERVICE_CONFIRMED_VT_CLOSE:
            if (Confirmed_VT_Close_Handler)
              Confirmed_VT_Close_Handler(
                service_request,
                service_request_len,
                src
                invoke_id); 
            break;
          case SERVICE_CONFIRMED_VT_DATA:
            if (Confirmed_VT_Data_Handler)
              Confirmed_VT_Data_Handler(
                service_request,
                service_request_len,
                src
                invoke_id); 
            break;
          // Security Services
          case SERVICE_CONFIRMED_AUTHENTICATE:
            if (Confirmed_Authenticate_Handler)
              Confirmed_Authenticate_Handler(
                service_request,
                service_request_len,
                src
                invoke_id); 
            break;
          case SERVICE_CONFIRMED_REQUEST_KEY:
            if (Confirmed_Request_Key_Handler)
              Confirmed_Request_Key_Handler(
                service_request,
                service_request_len,
                src
                invoke_id); 
            break;
          default:
            break;
        }
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
            if (Unconfirmed_Who_Is_Handler)
              Unconfirmed_Who_Is_Handler(
                service_request,
                service_request_len,
                src);
            break;
          case SERVICE_UNCONFIRMED_COV_NOTIFICATION:
            if (Unconfirmed_COV_Notification_Handler)
              Unconfirmed_COV_Notification_Handler(
                service_request,
                service_request_len,
                src);
            break;
          case SERVICE_UNCONFIRMED_I_HAVE:
            if (Unconfirmed_I_Have_Handler)
              Unconfirmed_I_Have_Handler(
                service_request,
                service_request_len,
                src);
            break;
          case SERVICE_UNCONFIRMED_EVENT_NOTIFICATION:
            if (Unconfirmed_Event_Notification_Handler)
              Unconfirmed_Event_Notification_Handler(
                service_request,
                service_request_len,
                src);
            break;
          case SERVICE_UNCONFIRMED_PRIVATE_TRANSFER:
            if (Unconfirmed_Private_Transfer_Handler)
              Unconfirmed_Private_Transfer_Handler(
                service_request,
                service_request_len,
                src);
            break;
          case SERVICE_UNCONFIRMED_TEXT_MESSAGE:
            if (Unconfirmed_Text_Message_Handler)
              Unconfirmed_Text_Message_Handler(
                service_request,
                service_request_len,
                src);
            break;
          case SERVICE_UNCONFIRMED_TIME_SYNCHRONIZATION:
            if (Unconfirmed_Time_Synchronization_Handler)
              Unconfirmed_Time_Synchronization_Handler(
                service_request,
                service_request_len,
                src);
            break;
          case SERVICE_UNCONFIRMED_WHO_HAS:
            if (Unconfirmed_Who_Has_Handler)
              Unconfirmed_Who_Has_Handler(
                service_request,
                service_request_len,
                src);
            break;
          case SERVICE_UNCONFIRMED_UTC_TIME_SYNCHRONIZATION:
            if (Unconfirmed_UTC_Time_Synchronization_Handler)
              Unconfirmed_UTC_Time_Synchronization_Handler(
                service_request,
                service_request_len,
                src);
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
