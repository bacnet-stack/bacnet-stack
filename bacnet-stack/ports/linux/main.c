// This example file is not copyrighted so that you can use it as you wish.
// Written by Steve Karg - 2005 - skarg@users.sourceforge.net
// Bug fixes, feature requests, and suggestions are welcome

// This is one way to use the BACnet stack under Linux

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include "config.h"
#include "bacdef.h"
#include "npdu.h"
#include "apdu.h"
#include "iam.h"
#include "whois.h"
#include "ethernet.h"

// buffers used for transmit and receive
static uint8_t Tx_Buf[MAX_MPDU] = {0};
static uint8_t Rx_Buf[MAX_MPDU] = {0};

// vendor id assigned by ASHRAE
uint16_t Vendor_Id = 42;
uint32_t Device_Id = 111;

// flag to send an I-Am
bool I_Am_Request = true;

// FIXME: if we handle multiple ports, then a port neutral version
// of this would be nice. Then it could be moved into iam.c
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

void WhoIsHandler(
  uint8_t *service_request,
  uint16_t service_len,
  BACNET_ADDRESS *src)
{
  int len = 0;
  int32_t low_limit = 0;
  int32_t high_limit = 0;

  len = whois_decode_service_request(
    service_request,
    service_len,
    &low_limit,
    &high_limit);
  if (len == 0)
    I_Am_Request = true;
  else if (len != -1)
  { 
    if ((Device_Id >= low_limit) && (Device_Id <= high_limit))
      I_Am_Request = true;
  }

  return;  
}

int main(int argc, char *argv[])
{
  BACNET_ADDRESS src = {0};  // address where message came from
  uint16_t pdu_len = 0;

  // custom handlers
  apdu_set_unconfirmed_handler(SERVICE_UNCONFIRMED_WHO_IS,WhoIsHandler);
  // init the physical layer
  if (!ethernet_init("eth0"))
    return 1;
  
  Send_IAm();
  // loop forever
  for (;;)
  {
    // input
    pdu_len = ethernet_receive(
      &src,
      &Rx_Buf[0],
      MAX_MPDU);

    // process
    if (pdu_len)
    {
      npdu_handler(
        &src,
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
