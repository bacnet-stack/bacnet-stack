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
#include "config.h"
#include "config.h"
#include "txbuf.h"
#include "bacdef.h"
#include "bacdcode.h"
#include "address.h"
#include "tsm.h"
#include "npdu.h"
#include "apdu.h"
#include "device.h"
#include "datalink.h"
#include "bactext.h"
#include "rp.h"
/* some demo stuff needed */
#include "handlers.h"
#include "txbuf.h"

/* for debugging... */
static void PrintReadPropertyData(BACNET_READ_PROPERTY_DATA *data)
{
  if (data)
  {
    if (data->array_index == BACNET_ARRAY_ALL)
      fprintf(stderr,"%s #%u %s\n",
        bactext_object_type_name(data->object_type),
        data->object_instance,
        bactext_property_name(data->object_property));
    else
      fprintf(stderr,"%s #%u %s[%d]\n",
        bactext_object_type_name(data->object_type),
        data->object_instance,
        bactext_property_name(data->object_property),
        data->array_index);
  }
}

void handler_read_property_ack(
  uint8_t *service_request,
  uint16_t service_len,
  BACNET_ADDRESS *src,
  BACNET_CONFIRMED_SERVICE_ACK_DATA *service_data)
{
  int len = 0;
  BACNET_READ_PROPERTY_DATA data;

  (void)src;
  (void)service_data; /* we could use these... */
  len = rp_ack_decode_service_request(
    service_request,
    service_len,
    &data);
  fprintf(stderr,"Received Read-Property Ack!\n");
  if (len > 0)
    PrintReadPropertyData(&data);
}
