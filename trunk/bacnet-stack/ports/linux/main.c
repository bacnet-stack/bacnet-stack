/*####COPYRIGHTBEGIN####
 -------------------------------------------
 Copyright (C) 2004 Steve Karg

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
 Boston, MA  02111-1307
 USA.

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

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include "bacdef.h"
#include "npdu.h"
#include "apdu.h"
#include "iam.h"
#include "ethernet.h"

// buffers used for transmit and receive
static uint8_t Tx_Buf[MAX_MPDU] = {0};
static uint8_t Rx_Buf[MAX_MPDU] = {0};

// vendor id assigned by ASHRAE
uint16_t Vendor_Id = 42;
uint32_t Device_Id = 111;

// flag to send an I-Am
bool I_Am_Request = true;

void Process_PDU(
  BACNET_ADDRESS *src,  // source address
  uint8_t *pdu, // PDU data
  uint16_t pdu_len) // length PDU 
{
  int apdu_offset = 0;
  BACNET_ADDRESS dest = {0};
  BACNET_NPDU_DATA npdu_data = {0};
  
  apdu_offset = npdu_decode(
    &pdu[0], // data to decode
    &dest, // destination address - get the DNET/DLEN/DADR if in there
    src,  // source address - get the SNET/SLEN/SADR if in there
    &npdu_data); // amount of data to decode
  if (npdu_data.network_layer_message)
  {
    fprintf(stderr,"main: network layer message received!");
  }
  else
  {
    apdu_handler(
      src, 
      npdu_data.data_expecting_reply,
      &pdu[apdu_offset],
      pdu_len - apdu_offset);
  }

  return;
}

void Send_IAm(void)
{
  int pdu_len = 0;
  BACNET_ADDRESS dest;
  BACNET_ADDRESS src;

  // I-Am is a global broadcast
  dest.mac_len = 0;

  ethernet_get_my_address(&src);

  // encode the NPDU portion of the packet
  pdu_len = npdu_encode_apdu(
    &Tx_Buf[0],
    &dest,
    &src,
    false,  // true for confirmed messages
    MESSAGE_PRIORITY_NORMAL);

  // encode the APDU portion of the packet
  pdu_len += iam_encode_apdu(
    &Tx_Buf[pdu_len], 
    Device_Id,
    MAX_APDU,
    SEGMENTATION_NONE,
    Vendor_Id);

  (void)ethernet_send_pdu(
    &dest,  // destination address
    &Tx_Buf[0],
    pdu_len); // number of bytes of data
}

int main(int argc, char *argv[])
{
  BACNET_ADDRESS src = {0};  // address where message came from
  uint16_t pdu_len = 0;

  if (!ethernet_init("eth0"))
    return 1;
  
  Send_IAm();
  // loop forever
  for (;;)
  {
    // input
    pdu_len = ethernet_receive(
      &src,  // source address
      &Rx_Buf[0],
      MAX_MPDU);

    // process
    if (pdu_len)
    {
      Process_PDU(
        &src,  // source address
        &Rx_Buf[0],
        pdu_len);
    }
    if (I_Am_Request)
    {
      I_Am_Request = false;
      Send_IAm();
    }
    // output
    // blink LEDs, Turn on or off outputs, etc
    
  }
  
  return 0;
}
