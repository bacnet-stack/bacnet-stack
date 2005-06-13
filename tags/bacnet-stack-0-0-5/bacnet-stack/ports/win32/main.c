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

// This is one way to use the embedded BACnet stack under Win32
// compiled with Borland C++ 5.02
#include <winsock2.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include "config.h"
#include "bacdef.h"
#include "npdu.h"
#include "apdu.h"
#include "device.h"
#include "handlers.h"
#include "bip.h"

// buffer used for receive
static uint8_t Rx_Buf[MAX_MPDU] = {0};

static void Init_Device_Parameters(void)
{
  // configure my initial values
  Device_Set_Object_Instance_Number(124);
  Device_Set_Vendor_Name("Lithonia Lighting");
  Device_Set_Vendor_Identifier(42);
  Device_Set_Model_Name("Simple BACnet Client");
  Device_Set_Firmware_Revision("1.00");
  Device_Set_Application_Software_Version("win32");
  Device_Set_Description("Example of a simple BACnet client/server");

  return;
}
  
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

/* To fill a need, we invent the gethostaddr() function. */
long gethostaddr(void) 
{
  struct hostent *host_ent;
  char host_name[255];
   
  if (gethostname(host_name, sizeof(host_name)) == 0) 
    return -1;
   
  if ((host_ent = gethostbyname(host_name)) == NULL)
    return -1;
   
  return *(long *)host_ent->h_addr;
}

extern void bip_set_addr(struct in_addr *net_address);

static void NetInitialize(void)
// initialize the TCP/IP stack
{
  int Result;
  int Code;
  WSADATA wd;
  struct in_addr address;


  Result = WSAStartup(MAKEWORD(2,2), &wd);

  if (Result != 0)
  {
    Code = WSAGetLastError();
    printf("TCP/IP stack initialization failed, error code: %i\n", 
      Code);
    exit(1);
  }
  address.s_addr = gethostaddr();
  bip_set_addr(&address);
}

int main(int argc, char *argv[])
{
  BACNET_ADDRESS src = {0};  // address where message came from
  uint16_t pdu_len = 0;
  unsigned timeout = 100; // milliseconds

  (void)argc;
  (void)argv;
  Init_Device_Parameters();
  Init_Service_Handlers();
  // init the data link layer
  NetInitialize();
  if (!bip_init())
    return 1;
  
  printf("BACnet stack running...\n"); 
  // loop forever
  for (;;)
  {
    // input

    // returns 0 bytes on timeout
    pdu_len = bip_receive(
      &src,
      &Rx_Buf[0],
      MAX_MPDU,
      timeout);

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
}
