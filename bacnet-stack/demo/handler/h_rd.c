/**************************************************************************
*
* Copyright (C) 2006 Steve Karg <skarg@users.sourceforge.net>
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
#include "txbuf.h"
#include "bacdef.h"
#include "bacdcode.h"
#include "bacerror.h"
#include "apdu.h"
#include "npdu.h"
#include "abort.h"
#include "reject.h"
#include "rd.h"

static BACNET_CHARACTER_STRING My_Password;

void handler_reinitialize_device(
  uint8_t *service_request,
  uint16_t service_len,
  BACNET_ADDRESS *src,
  BACNET_CONFIRMED_SERVICE_DATA *service_data)
{
  BACNET_REINITIALIZED_STATE state;
  BACNET_CHARACTER_STRING password;
  int len = 0;
  int pdu_len = 0;
  BACNET_ADDRESS my_address;
  int bytes_sent = 0;

  // decode the service request only
  len = rd_decode_service_request(
    service_request,
    service_len,
    &state,
    &password);
  fprintf(stderr,"ReinitializeDevice!\n");
  if (len > 0)
    fprintf(stderr,"ReinitializeDevice: state=%u password=%s\n",
            (unsigned)state,
            characterstring_value(&password));
  else
    fprintf(stderr,"ReinitializeDevice: Unable to decode request!\n");
  // prepare a reply
  datalink_get_my_address(&my_address);
  // encode the NPDU portion of the packet
  pdu_len = npdu_encode_apdu(
    &Handler_Transmit_Buffer[0],
    src,
    &my_address,
    false,  // true for confirmed messages
    MESSAGE_PRIORITY_NORMAL);
  // bad decoding or something we didn't understand - send an abort
  if (len == -1)
  {
    pdu_len += abort_encode_apdu(
        &Handler_Transmit_Buffer[pdu_len],
    service_data->invoke_id,
    ABORT_REASON_OTHER);
    fprintf(stderr,"ReinitializeDevice: Sending Abort - could not decode.\n");
  }
  else if (service_data->segmented_message)
  {
    pdu_len += abort_encode_apdu(
        &Handler_Transmit_Buffer[pdu_len],
    service_data->invoke_id,
    ABORT_REASON_SEGMENTATION_NOT_SUPPORTED);
    fprintf(stderr,"ReinitializeDevice: Sending Abort - segmented message.\n");
  }
  else if (state >= MAX_BACNET_REINITIALIZED_STATE)
  {
    pdu_len += reject_encode_apdu(
      &Handler_Transmit_Buffer[pdu_len],
      service_data->invoke_id,
      REJECT_REASON_UNDEFINED_ENUMERATION);
    fprintf(stderr,"ReinitializeDevice: Sending Reject - undefined enumeration\n");
  }
  else
  {
    characterstring_init_ansi(&My_Password,"Jesus");
    if (characterstring_same(&password,&My_Password))
    {
      pdu_len += encode_simple_ack(
        &Handler_Transmit_Buffer[pdu_len],
        service_data->invoke_id,
        SERVICE_CONFIRMED_REINITIALIZE_DEVICE);
      fprintf(stderr,"ReinitializeDevice: Sending Simple Ack!\n");
      /* FIXME: now you can reboot, restart, quit, or something clever */
      /* Note: you can use a mix of state and password to do specific stuff */
    }
    else
    {
      pdu_len += bacerror_encode_apdu(
        &Handler_Transmit_Buffer[pdu_len],
        service_data->invoke_id,
        SERVICE_CONFIRMED_REINITIALIZE_DEVICE,
        ERROR_CLASS_SERVICES,
        ERROR_CODE_PASSWORD_FAILURE);
      fprintf(stderr,"ReinitializeDevice: Sending Error - password failure.\n");
    }
  }
  bytes_sent = datalink_send_pdu(
      src,  // destination address
  &Handler_Transmit_Buffer[0],
  pdu_len); // number of bytes of data
  if (bytes_sent <= 0)
    fprintf(stderr,"ReinitializeDevice: Failed to send PDU (%s)!\n",
      strerror(errno));

  return;
}
