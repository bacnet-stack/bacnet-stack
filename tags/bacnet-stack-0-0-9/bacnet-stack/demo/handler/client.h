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
#ifndef CLIENT_H
#define CLIENT_H

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include "bacdef.h"
#include "apdu.h"
#include "bacapp.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* FIXME: probably should return the invoke ID for confirmed request */
bool Send_Read_Property_Request(
  uint32_t device_id, /* destination device */
  BACNET_OBJECT_TYPE object_type,
  uint32_t object_instance,
  BACNET_PROPERTY_ID object_property,
  int32_t array_index);

void Send_WhoIs(
  int32_t low_limit,
  int32_t high_limit);

/* FIXME: probably should return the invoke ID for confirmed request */
bool Send_Write_Property_Request(
    uint32_t device_id, // destination device
  BACNET_OBJECT_TYPE object_type,
  uint32_t object_instance,
  BACNET_PROPERTY_ID object_property,
  BACNET_APPLICATION_DATA_VALUE object_value,
  uint8_t priority,
  int32_t array_index);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
