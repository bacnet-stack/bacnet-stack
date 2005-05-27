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
#ifndef HANDLERS_H
#define HANDLERS_H

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include "bacdef.h"
#include "apdu.h"

// flag to send an I-Am
extern bool I_Am_Request;
// flag to send a global Who-Is
extern bool Who_Is_Request;

void UnrecognizedServiceHandler(
  uint8_t *service_request,
  uint16_t service_len,
  BACNET_ADDRESS *dest,
  BACNET_CONFIRMED_SERVICE_DATA *service_data);

void Send_IAm(void);
void Send_WhoIs(void);

void WhoIsHandler(
  uint8_t *service_request,
  uint16_t service_len,
  BACNET_ADDRESS *src);

void IAmHandler(
  uint8_t *service_request,
  uint16_t service_len,
  BACNET_ADDRESS *src);

void ReadPropertyAckHandler(
  uint8_t *service_request,
  uint16_t service_len,
  BACNET_ADDRESS *src,
  BACNET_CONFIRMED_SERVICE_ACK_DATA *service_data);
  
void ReadPropertyHandler(
  uint8_t *service_request,
  uint16_t service_len,
  BACNET_ADDRESS *src,
  BACNET_CONFIRMED_SERVICE_DATA *service_data);

void WritePropertyHandler(
  uint8_t *service_request,
  uint16_t service_len,
  BACNET_ADDRESS *src,
  BACNET_CONFIRMED_SERVICE_DATA *service_data);

// returns false if device is not bound
bool Send_Read_Property_Request(
  uint32_t device_id, // destination device
  BACNET_OBJECT_TYPE object_type,
  uint32_t object_instance,
  BACNET_PROPERTY_ID object_property,
  int32_t array_index);

#endif
