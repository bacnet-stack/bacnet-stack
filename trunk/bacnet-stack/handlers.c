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
#include <string.h>
#include <errno.h>
#include "config.h"
#include "bacdef.h"
#include "bacdcode.h"
#include "npdu.h"
#include "apdu.h"
#include "device.h"
#include "ai.h"
#include "ao.h"
#include "rp.h"
#include "wp.h"
#include "arf.h"
#include "bacfile.h"
#include "whois.h"
#include "iam.h"
#include "reject.h"
#include "abort.h"
#include "bacerror.h"
#include "address.h"
#include "tsm.h"
#include "datalink.h"

// Example handlers of services

// flag to send an I-Am
bool I_Am_Request = true;
// flag to send a global Who-Is
bool Who_Is_Request = true;

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
  int bytes_sent = 0;
  
  (void)service_request;
  (void)service_len;
  datalink_get_my_address(&src);

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

  bytes_sent = datalink_send_pdu(
    dest,  // destination address
    &Tx_Buf[0],
    pdu_len); // number of bytes of data
  if (bytes_sent > 0)
    fprintf(stderr,"Sent Reject!\n");
  else
    fprintf(stderr,"Failed to Send Reject (%s)!\n", strerror(errno));
}

void Send_IAm(void)
{
  iam_send(&Tx_Buf[0]);
}
void Send_WhoIs(void)
{
  int pdu_len = 0;
  BACNET_ADDRESS dest;
  int bytes_sent = 0;

  // Who-Is is a global broadcast
  datalink_get_broadcast_address(&dest);

  // encode the NPDU portion of the packet
  pdu_len = npdu_encode_apdu(
    &Tx_Buf[0],
    &dest,
    NULL,
    false,  // true for confirmed messages
    MESSAGE_PRIORITY_NORMAL);

  // encode the APDU portion of the packet
  pdu_len += whois_encode_apdu(
    &Tx_Buf[pdu_len], 
    -1, // send to all
    -1);// send to all

  bytes_sent = datalink_send_pdu(
    &dest,  // destination address
    &Tx_Buf[0],
    pdu_len); // number of bytes of data
  if (bytes_sent > 0)
    fprintf(stderr,"Sent Who-Is Request!\n");
  else
    fprintf(stderr,"Failed to Send Who-Is Request (%s)!\n", strerror(errno));
}

// returns false if device is not bound
bool Send_Read_Property_Request(
  uint32_t device_id, // destination device
  BACNET_OBJECT_TYPE object_type,
  uint32_t object_instance,
  BACNET_PROPERTY_ID object_property,
  int32_t array_index)
{
  BACNET_ADDRESS dest;
  BACNET_ADDRESS my_address;
  unsigned max_apdu = 0;
  uint8_t invoke_id = 0;
  bool status = false;
  int pdu_len = 0;
  int bytes_sent = 0;
  BACNET_READ_PROPERTY_DATA data;

  status = address_get_by_device(device_id, &max_apdu, &dest);
  if (status)
  {
    datalink_get_my_address(&my_address);
    pdu_len = npdu_encode_apdu(
      &Tx_Buf[0],
      &dest,
      &my_address,
      true,  // true for confirmed messages
      MESSAGE_PRIORITY_NORMAL);

    invoke_id = tsm_next_free_invokeID();
    // load the data for the encoding
    data.object_type = object_type;
    data.object_instance = object_instance;
    data.object_property = object_property;
    data.array_index = array_index;
    pdu_len += rp_encode_apdu(
      &Tx_Buf[pdu_len],
      invoke_id,
      &data);
    if ((unsigned)pdu_len < max_apdu)
    {
      tsm_set_confirmed_unsegmented_transaction(
        invoke_id,
        &dest,
        &Tx_Buf[0],
        pdu_len);
      bytes_sent = datalink_send_pdu(
        &dest,  // destination address
        &Tx_Buf[0],
        pdu_len); // number of bytes of data
      if (bytes_sent > 0)
        fprintf(stderr,"Sent ReadProperty Request!\n");
      else
        fprintf(stderr,"Failed to Send ReadProperty Request (%s)!\n",
          strerror(errno));
    }
    else
      fprintf(stderr,"Failed to Send ReadProperty Request "
        "(exceeds destination maximum APDU)!\n");
  }

  return status;
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

void ReadPropertyAckHandler(
  uint8_t *service_request,
  uint16_t service_len,
  BACNET_ADDRESS *src,
  BACNET_CONFIRMED_SERVICE_ACK_DATA *service_data)
{
  int len = 0;
  BACNET_READ_PROPERTY_DATA data;

  (void)src;
  tsm_free_invoke_id(service_data->invoke_id);
  len = rp_ack_decode_service_request(
    service_request,
    service_len,
    &data);
  fprintf(stderr,"Received Read-Property Ack!\n");
  if (len > 0)
    fprintf(stderr,"type=%u instance=%u property=%u index=%d\n",
      data.object_type,
      data.object_instance,
      data.object_property,
      data.array_index);
}

void ReadPropertyHandler(
  uint8_t *service_request,
  uint16_t service_len,
  BACNET_ADDRESS *src,
  BACNET_CONFIRMED_SERVICE_DATA *service_data)
{
  BACNET_READ_PROPERTY_DATA data;
  int len = 0;
  int pdu_len = 0;
  BACNET_ADDRESS my_address;
  bool send = false;
  bool error = false;
  int bytes_sent = 0;
  BACNET_ERROR_CLASS error_class = ERROR_CLASS_OBJECT;
  BACNET_ERROR_CODE error_code = ERROR_CODE_UNKNOWN_OBJECT;

  len = rp_decode_service_request(
    service_request,
    service_len,
    &data);
  fprintf(stderr,"Received Read-Property Request!\n");
  if (len > 0)
    fprintf(stderr,"type=%u instance=%u property=%u index=%d\n",
      data.object_type,
      data.object_instance,
      data.object_property,
      data.array_index);
  else
    fprintf(stderr,"Unable to decode Read-Property Request!\n");
  // prepare a reply
  datalink_get_my_address(&my_address);
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
    fprintf(stderr,"Sending Abort!\n");
    send = true;
  }
  else if (service_data->segmented_message)
  {
    pdu_len += abort_encode_apdu(
      &Tx_Buf[pdu_len],
      service_data->invoke_id,
      ABORT_REASON_SEGMENTATION_NOT_SUPPORTED);
    fprintf(stderr,"Sending Abort!\n");
    send = true;
  }
  else
  {
    switch (data.object_type)
    {
      case OBJECT_DEVICE:
        // FIXME: probably need a length limitation sent with encode
        if (data.object_instance == Device_Object_Instance_Number())
        {
          len = Device_Encode_Property_APDU(
            &Temp_Buf[0],
            data.object_property,
            data.array_index,
            &error_class,
            &error_code);
          if (len > 0)
          {
            // encode the APDU portion of the packet
            data.application_data = &Temp_Buf[0];
            data.application_data_len = len;
            // FIXME: probably need a length limitation sent with encode
            pdu_len += rp_ack_encode_apdu(
              &Tx_Buf[pdu_len],
              service_data->invoke_id,
              &data);
            fprintf(stderr,"Sending Read Property Ack!\n");
            send = true;
          }
          else
            error = true;
        }
        else
          error = true;
        break;
      case OBJECT_ANALOG_INPUT:
        if (Analog_Input_Valid_Instance(data.object_instance))
        {
          len = Analog_Input_Encode_Property_APDU(
            &Temp_Buf[0],
            data.object_instance,
            data.object_property,
            data.array_index,
            &error_class,
            &error_code);
          if (len > 0)
          {
            // encode the APDU portion of the packet
            data.application_data = &Temp_Buf[0];
            data.application_data_len = len;
            // FIXME: probably need a length limitation sent with encode
            pdu_len += rp_ack_encode_apdu(
              &Tx_Buf[pdu_len],
              service_data->invoke_id,
              &data);
            fprintf(stderr,"Sending Read Property Ack!\n");
            send = true;
          }
          else
            error = true;
        }
        else
          error = true;
        break;
      case OBJECT_ANALOG_OUTPUT:
        if (Analog_Output_Valid_Instance(data.object_instance))
        {
          len = Analog_Output_Encode_Property_APDU(
            &Temp_Buf[0],
            data.object_instance,
            data.object_property,
            data.array_index,
            &error_class,
            &error_code);
          if (len > 0)
          {
            // encode the APDU portion of the packet
            data.application_data = &Temp_Buf[0];
            data.application_data_len = len;
            // FIXME: probably need a length limitation sent with encode
            pdu_len += rp_ack_encode_apdu(
              &Tx_Buf[pdu_len],
              service_data->invoke_id,
              &data);
            fprintf(stderr,"Sending Read Property Ack!\n");
            send = true;
          }
          else
            error = true;
        }
        else
          error = true;
        break;
      case OBJECT_FILE:
        if (bacfile_valid_instance(data.object_instance))
        {
          len = bacfile_encode_property_apdu(
            &Temp_Buf[0],
            data.object_instance,
            data.object_property,
            data.array_index,
            &error_class,
            &error_code);
          if (len > 0)
          {
            // encode the APDU portion of the packet
            data.application_data = &Temp_Buf[0];
            data.application_data_len = len;
            // FIXME: probably need a length limitation sent with encode
            pdu_len += rp_ack_encode_apdu(
              &Tx_Buf[pdu_len],
              service_data->invoke_id,
              &data);
            fprintf(stderr,"Sending Read Property Ack!\n");
            send = true;
          }
          else
            error = true;
        }
        else
          error = true;
        break;
      default:
        error = true;
        break;
    }
  }
  if (error)
  {
    pdu_len += bacerror_encode_apdu(
      &Tx_Buf[pdu_len],
      service_data->invoke_id,
      SERVICE_CONFIRMED_READ_PROPERTY,
      error_class,
      error_code);
    fprintf(stderr,"Sending Read Property Error!\n");
    send = true;
  }
  if (send)
  {
    bytes_sent = datalink_send_pdu(
      src,  // destination address
      &Tx_Buf[0],
      pdu_len); // number of bytes of data
    if (bytes_sent <= 0)
      fprintf(stderr,"Failed to send PDU (%s)!\n", strerror(errno));
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
  BACNET_ERROR_CLASS error_class = ERROR_CLASS_OBJECT;
  BACNET_ERROR_CODE error_code = ERROR_CODE_UNKNOWN_OBJECT;
  int bytes_sent = 0;

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
  datalink_get_my_address(&my_address);
  // encode the NPDU portion of the packet
  pdu_len = npdu_encode_apdu(
    &Tx_Buf[0],
    src,
    &my_address,
    false,  // true for confirmed messages
    MESSAGE_PRIORITY_NORMAL);
  // bad decoding or something we didn't understand - send an abort
  if (len == -1)
  {
    pdu_len += abort_encode_apdu(
      &Tx_Buf[pdu_len],
      service_data->invoke_id,
      ABORT_REASON_OTHER);
    fprintf(stderr,"Sending Abort!\n");
  }
  else if (service_data->segmented_message)
  {
    pdu_len += abort_encode_apdu(
      &Tx_Buf[pdu_len],
      service_data->invoke_id,
      ABORT_REASON_SEGMENTATION_NOT_SUPPORTED);
    fprintf(stderr,"Sending Abort!\n");
  }
  else
  {
    switch (wp_data.object_type)
    {
      case OBJECT_DEVICE:
        if (Device_Write_Property(&wp_data,&error_class,&error_code))
        {
          pdu_len += encode_simple_ack(
            &Tx_Buf[pdu_len],
            service_data->invoke_id,
            SERVICE_CONFIRMED_WRITE_PROPERTY);
          fprintf(stderr,"Sending Write Property Simple Ack!\n");
        }
        else
        {
          pdu_len += bacerror_encode_apdu(
            &Tx_Buf[pdu_len],
            service_data->invoke_id,
            SERVICE_CONFIRMED_WRITE_PROPERTY,
            error_class,
            error_code);
          fprintf(stderr,"Sending Write Property Error!\n");
        }
        break;
      case OBJECT_ANALOG_INPUT:
        error_class = ERROR_CLASS_PROPERTY;
        error_code = ERROR_CODE_WRITE_ACCESS_DENIED;
        pdu_len += bacerror_encode_apdu(
          &Tx_Buf[pdu_len],
          service_data->invoke_id,
          SERVICE_CONFIRMED_WRITE_PROPERTY,
          error_class,
          error_code);
        fprintf(stderr,"Sending Write Access Error!\n");
        break;
      case OBJECT_ANALOG_OUTPUT:
        if (Analog_Output_Write_Property(&wp_data,&error_class,&error_code))
        {
          pdu_len += encode_simple_ack(
            &Tx_Buf[pdu_len],
            service_data->invoke_id,
            SERVICE_CONFIRMED_WRITE_PROPERTY);
          fprintf(stderr,"Sending Write Property Simple Ack!\n");
        }
        else
        {
          pdu_len += bacerror_encode_apdu(
            &Tx_Buf[pdu_len],
            service_data->invoke_id,
            SERVICE_CONFIRMED_WRITE_PROPERTY,
            error_class,
            error_code);
          fprintf(stderr,"Sending Write Access Error!\n");
	}
        break;
      default:
        pdu_len += bacerror_encode_apdu(
          &Tx_Buf[pdu_len],
          service_data->invoke_id,
          SERVICE_CONFIRMED_WRITE_PROPERTY,
          error_class,
          error_code);
        fprintf(stderr,"Sending Unknown Object Error!\n");
        break;
    }
  }
  bytes_sent = datalink_send_pdu(
    src,  // destination address
    &Tx_Buf[0],
    pdu_len); // number of bytes of data
  if (bytes_sent <= 0)
    fprintf(stderr,"Failed to send PDU (%s)!\n", strerror(errno));

  return;
}

void AtomicReadFileHandler(
  uint8_t *service_request,
  uint16_t service_len,
  BACNET_ADDRESS *src,
  BACNET_CONFIRMED_SERVICE_DATA *service_data)
{
  BACNET_ATOMIC_READ_FILE_DATA data;
  int len = 0;
  int pdu_len = 0;
  BACNET_ADDRESS my_address;
  bool send = false;
  bool error = false;
  int bytes_sent = 0;
  BACNET_ERROR_CLASS error_class = ERROR_CLASS_OBJECT;
  BACNET_ERROR_CODE error_code = ERROR_CODE_UNKNOWN_OBJECT;
  char buffer[MAX_APDU - 16] = ""; // for reply data, less apdu overhead

  fprintf(stderr,"Received Atomic-Read-File Request!\n");
  len = arf_decode_service_request(
    service_request,
    service_len,
    &data);
  if (len < 0)
    fprintf(stderr,"Unable to decode Atomic-Read-File Request!\n");
  // prepare a reply
  datalink_get_my_address(&my_address);
  // encode the NPDU portion of the packet
  pdu_len = npdu_encode_apdu(
    &Tx_Buf[0],
    src,
    &my_address,
    false,  // true for confirmed messages
    MESSAGE_PRIORITY_NORMAL);
  // bad decoding - send an abort
  if (len < 0)
  {
    pdu_len += abort_encode_apdu(
      &Tx_Buf[pdu_len],
      service_data->invoke_id,
      ABORT_REASON_OTHER);
    fprintf(stderr,"Sending Abort!\n");
    send = true;
  }
  else if (service_data->segmented_message)
  {
    pdu_len += abort_encode_apdu(
      &Tx_Buf[pdu_len],
      service_data->invoke_id,
      ABORT_REASON_SEGMENTATION_NOT_SUPPORTED);
    fprintf(stderr,"Sending Abort!\n");
    send = true;
  }
  else
  {
    if (data.access == FILE_STREAM_ACCESS)
    {
      data.fileData = (uint8_t *)&buffer[0];
      data.fileDataLength = sizeof(buffer);
      if (data.type.stream.requestedOctetCount < data.fileDataLength)
      {
        if (bacfile_read_data(&data))
        {
          pdu_len += arf_ack_encode_apdu(
            &Tx_Buf[pdu_len],
            service_data->invoke_id,
            &data);
          send = true;
        }
        else
        {
          send = true;
          error = true;
        }
      }
      else
      {
        pdu_len += abort_encode_apdu(
          &Tx_Buf[pdu_len],
          service_data->invoke_id,
          ABORT_REASON_SEGMENTATION_NOT_SUPPORTED);
        fprintf(stderr,"Sending Abort!\n");
        send = true;
      }
    }
    else
    {
      error_class = ERROR_CLASS_SERVICES;
      error_code = ERROR_CODE_INVALID_FILE_ACCESS_METHOD;
      send = true;
      error = true;
    }
  }
  if (error)
  {
    pdu_len += bacerror_encode_apdu(
      &Tx_Buf[pdu_len],
      service_data->invoke_id,
      SERVICE_CONFIRMED_ATOMIC_READ_FILE,
      error_class,
      error_code);
    fprintf(stderr,"Sending Error!\n");
    send = true;
  }
  if (send)
  {
    bytes_sent = datalink_send_pdu(
      src,  // destination address
      &Tx_Buf[0],
      pdu_len); // number of bytes of data
    if (bytes_sent <= 0)
      fprintf(stderr,"Failed to send PDU (%s)!\n", strerror(errno));
  }

  return;
}

// We performed an AtomicReadFile Request,
// and here is the data from the server
// Note: it does not have to be the same file=instance
// that someone can read from us.  It is common to
// use the description as the file name.
void AtomicReadFileAckHandler(
  uint8_t *service_request,
  uint16_t service_len,
  BACNET_ADDRESS *src,
  BACNET_CONFIRMED_SERVICE_ACK_DATA *service_data)
{
  int len = 0;
  BACNET_ATOMIC_READ_FILE_DATA data;
  FILE *pFile = NULL;
  char *pFilename = NULL;
  uint32_t instance = 0;

  (void)src;
  // get the file instance from the tsm data before freeing it
  instance = bacfile_instance_from_tsm(service_data->invoke_id);
  tsm_free_invoke_id(service_data->invoke_id);
  len = arf_ack_decode_service_request(
    service_request,
    service_len,
    &data);
  fprintf(stderr,"Received Read-File Ack!\n");
  if ((len > 0) && (instance <= BACNET_MAX_INSTANCE))
  {
    // write the data received to the file specified
    if (data.access == FILE_STREAM_ACCESS)
    {
      pFilename = bacfile_name(instance);
      if (pFilename)
      {
        pFile = fopen(pFilename, "rb");
        if (pFile)
        {
          (void)fseek(pFile,
            data.type.stream.fileStartPosition,
            SEEK_SET);
          if (fwrite(data.fileData,data.fileDataLength,1,pFile) != 1)
            fprintf(stderr,"Failed to write to %s (%u)!\n",
              pFilename, instance);
          fclose(pFile);
        }
      }      
    }
    else if (data.access == FILE_RECORD_ACCESS)
    {
      // FIXME: add handling for Record Access
    }
  }
}



