/**************************************************************************
*
* Copyright (C) 2005 Steve Karg <skarg@users.sourceforge.net>
*
* Permission is hereby granted, free of charge, to any person obtaining
* a copy of this software and associated documentation files (the
* "Software"), to deal in the Software without restriction, including
* without limitation the rights to use, copy, modify, merge, publish,
* distribute, sublicense, and/or sell copies of the Software, and to
* permit persons to whom the Software is furnished to do so, subject to
* the following conditions:
*
* The above copyright notice and this permission notice shall be included
* in all copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*
*********************************************************************/
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include "config.h"
#include "bacdef.h"
#include "bacdcode.h"
#include "npdu.h"
#include "apdu.h"
#include "device.h"
#include "ai.h"
#include "rp.h"
#include "wp.h"
#include "iam.h"
#include "whois.h"
#include "reject.h"
#include "abort.h"
#include "bacerror.h"

// Example handlers of services

#ifdef BACDL_ETHERNET
#include "ethernet.h"
#define bacdl_get_broadcast_address ethernet_get_broadcast_address
#define bacdl_get_my_address ethernet_get_my_address
#define bacdl_send_pdu ethernet_send_pdu
#endif

#ifdef BACDL_MSTP
#include "mstp.h"
#define bacdl_get_broadcast_address mstp_get_broadcast_address
#define bacdl_get_my_address mstp_get_my_address
#define bacdl_send_pdu mstp_send_pdu
#define bacdl_receive mstp_receive
#endif

#ifdef BACDL_BIP
#include "bip.h"
#define bacdl_get_broadcast_address bip_get_broadcast_address
#define bacdl_get_my_address bip_get_my_address
#define bacdl_send_pdu bip_send_pdu
#define bacdl_receive bip_receive
#endif


// flag to send an I-Am
bool I_Am_Request = true;

// buffers used for transmit and receive
static uint8_t Tx_Buf[MAX_MPDU] = {0};
static uint8_t Temp_Buf[MAX_APDU] = {0};

void UnrecognizedServiceHandler(
  uint8_t *service_request,
  uint16_t service_len,
  BACNET_ADDRESS *dest,
  BACNET_CONFIRMED_SERVICE_DATA *service_data)
{
  BACNET_ADDRESS src;
  int pdu_len = 0;
  
  (void)service_request;
  (void)service_len;
  bacdl_get_my_address(&src);

  // encode the NPDU portion of the packet
  pdu_len = npdu_encode_apdu(
    &Tx_Buf[0],
    dest,
    &src,
    false,  // true for confirmed messages
    MESSAGE_PRIORITY_NORMAL);
  
  // encode the APDU portion of the packet
  pdu_len += reject_encode_apdu(
    &Tx_Buf[pdu_len], 
    service_data->invoke_id,
    REJECT_REASON_UNRECOGNIZED_SERVICE);

  (void)bacdl_send_pdu(
    dest,  // destination address
    &Tx_Buf[0],
    pdu_len); // number of bytes of data
  fprintf(stderr,"Sent Reject!\n");
}

// FIXME: if we handle multiple ports, then a port neutral version
// of this would be nice. Then it could be moved into iam.c
void Send_IAm(void)
{
  int pdu_len = 0;
  BACNET_ADDRESS dest;

  // I-Am is a global broadcast
  bacdl_get_broadcast_address(&dest);

  // encode the NPDU portion of the packet
  pdu_len = npdu_encode_apdu(
    &Tx_Buf[0],
    &dest,
    NULL,
    false,  // true for confirmed messages
    MESSAGE_PRIORITY_NORMAL);

  // encode the APDU portion of the packet
  pdu_len += iam_encode_apdu(
    &Tx_Buf[pdu_len], 
    Device_Object_Instance_Number(),
    MAX_APDU,
    SEGMENTATION_NONE,
    Device_Vendor_Identifier());

  (void)bacdl_send_pdu(
    &dest,  // destination address
    &Tx_Buf[0],
    pdu_len); // number of bytes of data
  fprintf(stderr,"Sent I-Am Request!\n");
}

void WhoIsHandler(
  uint8_t *service_request,
  uint16_t service_len,
  BACNET_ADDRESS *src)
{
  int len = 0;
  int32_t low_limit = 0;
  int32_t high_limit = 0;

  (void)src;
  fprintf(stderr,"Received Who-Is Request!\n");
  len = whois_decode_service_request(
    service_request,
    service_len,
    &low_limit,
    &high_limit);
  if (len == 0)
    I_Am_Request = true;
  else if (len != -1)
  { 
    if ((Device_Object_Instance_Number() >= (uint32_t)low_limit) && 
        (Device_Object_Instance_Number() <= (uint32_t)high_limit))
      I_Am_Request = true;
  }

  return;  
}

void IAmHandler(
  uint8_t *service_request,
  uint16_t service_len,
  BACNET_ADDRESS *src)
{
  int len = 0;
  uint32_t device_id = 0;
  unsigned max_apdu = 0;
  int segmentation = 0;
  uint16_t vendor_id = 0;

  (void)src;
  (void)service_len;
  len = iam_decode_service_request(
    service_request,
    &device_id,
    &max_apdu,
    &segmentation,
    &vendor_id);
  fprintf(stderr,"Received I-Am Request");
  if (len != -1)
    fprintf(stderr," from %u!\n",device_id);
  else
    fprintf(stderr,"!\n");

  return;  
}

void ReadPropertyHandler(
  uint8_t *service_request,
  uint16_t service_len,
  BACNET_ADDRESS *src,
  BACNET_CONFIRMED_SERVICE_DATA *service_data)
{
  BACNET_READ_PROPERTY_DATA rp_data;
  int len = 0;
  int pdu_len = 0;
  BACNET_OBJECT_TYPE object_type;
  uint32_t object_instance;
  BACNET_PROPERTY_ID object_property;
  int32_t array_index;
  BACNET_ADDRESS my_address;
  bool send = false;

  len = rp_decode_service_request(
    service_request,
    service_len,
    &object_type,
    &object_instance,
    &object_property,
    &array_index);
  fprintf(stderr,"Received Read-Property Request!\n");
  if (len > 0)
    fprintf(stderr,"type=%u instance=%u property=%u index=%d\n",
      object_type,
      object_instance,
      object_property,
      array_index);
  else
    fprintf(stderr,"Unable to decode Read-Property Request!\n");
  // prepare a reply
  bacdl_get_my_address(&my_address);
  // encode the NPDU portion of the packet
  pdu_len = npdu_encode_apdu(
    &Tx_Buf[0],
    src,
    &my_address,
    false,  // true for confirmed messages
    MESSAGE_PRIORITY_NORMAL);
  // bad decoding - send an abort
  if (len == -1)
  {
    pdu_len += abort_encode_apdu(
      &Tx_Buf[pdu_len],
      service_data->invoke_id,
      ABORT_REASON_OTHER);
    fprintf(stderr,"Sent Abort!\n");
    send = true;
  }
  else if (service_data->segmented_message)
  {
    pdu_len += abort_encode_apdu(
      &Tx_Buf[pdu_len],
      service_data->invoke_id,
      ABORT_REASON_SEGMENTATION_NOT_SUPPORTED);
    fprintf(stderr,"Sent Abort!\n");
    send = true;
  }
  else
  {
    switch (object_type)
    {
      case OBJECT_DEVICE:
        // FIXME: probably need a length limitation sent with encode
        // FIXME: might need to return error codes
        if (object_instance == Device_Object_Instance_Number())
        {
          len = Device_Encode_Property_APDU(
            &Temp_Buf[0],
            object_property,
            array_index);
          if (len > 0)
          {
            // encode the APDU portion of the packet
            rp_data.object_type = object_type;
            rp_data.object_instance = object_instance;
            rp_data.object_property = object_property;
            rp_data.array_index = array_index;
            rp_data.application_data = &Temp_Buf[0];
            rp_data.application_data_len = len;
            // FIXME: probably need a length limitation sent with encode
            pdu_len += rp_ack_encode_apdu(
              &Tx_Buf[pdu_len],
              service_data->invoke_id,
              &rp_data);
            fprintf(stderr,"Sent Read Property Ack!\n");
            send = true;
          }
          else
          {
            pdu_len += bacerror_encode_apdu(
              &Tx_Buf[pdu_len],
              service_data->invoke_id,
              SERVICE_CONFIRMED_READ_PROPERTY,
              ERROR_CLASS_PROPERTY,
              ERROR_CODE_UNKNOWN_PROPERTY);
            fprintf(stderr,"Sent Unknown Property Error!\n");
            send = true;
          }
        }
        else
        {
          pdu_len += bacerror_encode_apdu(
            &Tx_Buf[pdu_len],
            service_data->invoke_id,
            SERVICE_CONFIRMED_READ_PROPERTY,
            ERROR_CLASS_OBJECT,
            ERROR_CODE_UNKNOWN_OBJECT);
          fprintf(stderr,"Sent Unknown Object Error!\n");
          send = true;
        }
        break;
      case OBJECT_ANALOG_INPUT:
        if (Analog_Input_Valid_Instance(object_instance))
        {
          len = Analog_Input_Encode_Property_APDU(
            &Temp_Buf[0],
            object_instance,
            object_property,
            array_index);
          if (len > 0)
          {
            // encode the APDU portion of the packet
            rp_data.object_type = object_type;
            rp_data.object_instance = object_instance;
            rp_data.object_property = object_property;
            rp_data.array_index = array_index;
            rp_data.application_data = &Temp_Buf[0];
            rp_data.application_data_len = len;
            // FIXME: probably need a length limitation sent with encode
            pdu_len += rp_ack_encode_apdu(
              &Tx_Buf[pdu_len],
              service_data->invoke_id,
              &rp_data);
            fprintf(stderr,"Sent Read Property Ack!\n");
            send = true;
          }
          else
          {
            pdu_len += bacerror_encode_apdu(
              &Tx_Buf[pdu_len],
              service_data->invoke_id,
              SERVICE_CONFIRMED_READ_PROPERTY,
              ERROR_CLASS_PROPERTY,
              ERROR_CODE_UNKNOWN_PROPERTY);
            fprintf(stderr,"Sent Unknown Property Error!\n");
            send = true;
          }
        }
        else
        {
          pdu_len += bacerror_encode_apdu(
            &Tx_Buf[pdu_len],
            service_data->invoke_id,
            SERVICE_CONFIRMED_READ_PROPERTY,
            ERROR_CLASS_OBJECT,
            ERROR_CODE_UNKNOWN_OBJECT);
          fprintf(stderr,"Sent Unknown Object Error!\n");
          send = true;
        }
        break;
      default:
        pdu_len += bacerror_encode_apdu(
          &Tx_Buf[pdu_len],
          service_data->invoke_id,
          SERVICE_CONFIRMED_READ_PROPERTY,
          ERROR_CLASS_OBJECT,
          ERROR_CODE_UNKNOWN_OBJECT);
        fprintf(stderr,"Sent Unknown Object Error!\n");
        send = true;
        break;
    }
  }
  if (send)
  {
    (void)bacdl_send_pdu(
      src,  // destination address
      &Tx_Buf[0],
      pdu_len); // number of bytes of data
  }

  return;
}

void WritePropertyHandler(
  uint8_t *service_request,
  uint16_t service_len,
  BACNET_ADDRESS *src,
  BACNET_CONFIRMED_SERVICE_DATA *service_data)
{
  BACNET_WRITE_PROPERTY_DATA wp_data;
  int len = 0;
  int pdu_len = 0;
  BACNET_ADDRESS my_address;
  bool send = false;
  BACNET_ERROR_CLASS error_class;
  BACNET_ERROR_CODE error_code;

  // decode the service request only
  len = wp_decode_service_request(
    service_request,
    service_len,
    &wp_data);
  fprintf(stderr,"Received Write-Property Request!\n");
  if (len > 0)
    fprintf(stderr,"type=%u instance=%u property=%u index=%d\n",
      wp_data.object_type,
      wp_data.object_instance,
      wp_data.object_property,
      wp_data.array_index);
  else
    fprintf(stderr,"Unable to decode Write-Property Request!\n");
  // prepare a reply
  bacdl_get_my_address(&my_address);
  // encode the NPDU portion of the packet
  pdu_len = npdu_encode_apdu(
    &Tx_Buf[0],
    src,
    &my_address,
    false,  // true for confirmed messages
    MESSAGE_PRIORITY_NORMAL);
  // bad decoding - send an abort
  if (len == -1)
  {
    pdu_len += abort_encode_apdu(
      &Tx_Buf[pdu_len],
      service_data->invoke_id,
      ABORT_REASON_OTHER);
    fprintf(stderr,"Sent Abort!\n");
    send = true;
  }
  else if (service_data->segmented_message)
  {
    pdu_len += abort_encode_apdu(
      &Tx_Buf[pdu_len],
      service_data->invoke_id,
      ABORT_REASON_SEGMENTATION_NOT_SUPPORTED);
    fprintf(stderr,"Sent Abort!\n");
    send = true;
  }
  else
  {
    switch (wp_data.object_type)
    {
      case OBJECT_DEVICE:
        if (Device_Write_Property(&wp_data,&error_class,&error_code))
        {
          pdu_len = encode_simple_ack(
            &Tx_Buf[pdu_len],
            service_data->invoke_id,
            SERVICE_CONFIRMED_WRITE_PROPERTY);
          fprintf(stderr,"Sent Write Property Simple Ack!\n");
          send = true;
        }
        else
        {
          pdu_len += bacerror_encode_apdu(
            &Tx_Buf[pdu_len],
            service_data->invoke_id,
            SERVICE_CONFIRMED_WRITE_PROPERTY,
            error_class,
            error_code);
          fprintf(stderr,"Sent Write Property Error!\n");
          send = true;
        }
        break;
      case OBJECT_ANALOG_INPUT:
        pdu_len += bacerror_encode_apdu(
          &Tx_Buf[pdu_len],
          service_data->invoke_id,
          SERVICE_CONFIRMED_WRITE_PROPERTY,
          ERROR_CLASS_PROPERTY,
          ERROR_CODE_WRITE_ACCESS_DENIED);
        fprintf(stderr,"Sent Write Access Error!\n");
        send = true;
        break;
      default:
        pdu_len += bacerror_encode_apdu(
          &Tx_Buf[pdu_len],
          service_data->invoke_id,
          SERVICE_CONFIRMED_WRITE_PROPERTY,
          ERROR_CLASS_OBJECT,
          ERROR_CODE_UNKNOWN_OBJECT);
        fprintf(stderr,"Sent Unknown Object Error!\n");
        send = true;
        break;
    }
  }
  if (send)
  {
    (void)bacdl_send_pdu(
      src,  // destination address
      &Tx_Buf[0],
      pdu_len); // number of bytes of data
  }

  return;
}

