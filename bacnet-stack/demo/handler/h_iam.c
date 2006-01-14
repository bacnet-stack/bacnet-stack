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
#include "txbuf.h"
#include "bacdef.h"
#include "bacdcode.h"
#include "iam.h"
#include "address.h"

void handler_i_am(
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
  {
    fprintf(stderr," from %u!\n",device_id);
    address_add(device_id,
      max_apdu,
      src);
  }
  else
    fprintf(stderr,"!\n");

  return;  
}
