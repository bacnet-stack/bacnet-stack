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

// This is one way to use the embedded BACnet stack under RTOS-32 
// compiled with Borland C++ 5.02
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include "config.h"
#include "bacdef.h"
#include "npdu.h"
#include "apdu.h"
#include "device.h"
#include "handlers.h"
#include "net.h"

// buffers used for transmit and receive
static uint8_t Rx_Buf[MAX_MPDU] = {0};
#ifdef BACDL_MSTP
volatile struct mstp_port_struct_t MSTP_Port; // port data
static uint8_t MSTP_MAC_Address = 0x05; // local MAC address
#endif

static void Init_Service_Handlers(void)
{
  // we need to handle who-is to support dynamic device binding
  apdu_set_unconfirmed_handler(
    SERVICE_UNCONFIRMED_WHO_IS,
    WhoIsHandler);
  // set the handler for all the services we don't implement
  // It is required to send the proper reject message...
  apdu_set_unrecognized_service_handler_handler(
    UnrecognizedServiceHandler);
  // we must implement read property - it's required!
  apdu_set_confirmed_handler(
    SERVICE_CONFIRMED_READ_PROPERTY,
    ReadPropertyHandler);
  apdu_set_confirmed_handler(
    SERVICE_CONFIRMED_WRITE_PROPERTY,
    WritePropertyHandler);
}

int main(int argc, char *argv[])
{
  BACNET_ADDRESS src = {0};  // address where message came from
  uint16_t pdu_len = 0;
  unsigned timeout = 100; // milliseconds

  (void)argc;
  (void)argv;
  Device_Set_Object_Instance_Number(126);
  Init_Service_Handlers();
  // init the physical layer
  #ifdef BACDL_BIP
  if (!bip_init())
    return 1;
  #endif
  #ifdef BACDL_ETHERNET
  if (!ethernet_init(NULL))
    return 1;
  #endif
  #ifdef BACDL_MSTP
  RS485_Initialize();
  MSTP_Init(&MSTP_Port,MSTP_MAC_Address);
  #endif

  
  // loop forever
  for (;;)
  {
    // input
    #ifdef BACDL_MSTP
    MSTP_Millisecond_Timer(&MSTP_Port);
    // note: also called by RS-485 Receive ISR
    RS485_Check_UART_Data(&MSTP_Port);
    MSTP_Receive_Frame_FSM(&MSTP_Port);
    #endif

    #if (defined(BACDL_ETHERNET) || defined(BACDL_BIP))
    // returns 0 bytes on timeout
    pdu_len = bacdl_receive(
      &src,
      &Rx_Buf[0],
      MAX_MPDU,
      timeout);
    #endif

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
    #ifdef BACDL_MSTP
    MSTP_Master_Node_FSM(&MSTP_Port);
    #endif

    // blink LEDs, Turn on or off outputs, etc
  }
}
