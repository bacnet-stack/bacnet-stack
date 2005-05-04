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
#include <stdlib.h>
#include <signal.h>
#include "config.h"
#include "bacdef.h"
#include "handlers.h"
#include "bacdcode.h"
#include "npdu.h"
#include "apdu.h"
#include "device.h"
#ifdef BACDL_ETHERNET
  #include "ethernet.h"
#endif
#ifdef BACDL_BIP
  #include "bip.h"
#endif
#include "net.h"

// buffers used for receiving
static uint8_t Rx_Buf[MAX_MPDU] = {0};

static int get_local_ifr_ioctl(char *ifname, struct ifreq *ifr, int request)
{
    int fd;
    int rv;                     // return value

    strncpy(ifr->ifr_name, ifname, sizeof(ifr->ifr_name));
    fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);
    if (fd < 0)
        rv = fd;
    else
        rv = ioctl(fd, request, ifr);

    return rv;
}

static int get_local_address_ioctl(
    char *ifname, 
    struct in_addr *addr,
    int request)
{
    struct ifreq ifr = { {{0}} };
    struct sockaddr_in *tcpip_address;
    int rv;                     // return value

    rv = get_local_ifr_ioctl(ifname,&ifr,request);
    if (rv >= 0) {
        tcpip_address = (struct sockaddr_in *) &ifr.ifr_addr;
        memcpy(addr, &tcpip_address->sin_addr, sizeof(struct in_addr));
    }

    return rv;
}

static void decode_network_address(struct in_addr *net_address,
    uint8_t *octet1, uint8_t *octet2, uint8_t *octet3, uint8_t *octet4)
{
    union {
        uint8_t byte[4];
        uint32_t value;
    } long_data = {{0}};

    long_data.value = net_address->s_addr;

    *octet1 = long_data.byte[0];
    *octet2 = long_data.byte[1];
    *octet3 = long_data.byte[2];
    *octet4 = long_data.byte[3];
}

static void Init_Network(char *ifname)
{
    struct in_addr local_address;
    uint8_t octet1; 
    uint8_t octet2; 
    uint8_t octet3; 
    uint8_t octet4;

    /* setup local address */
    get_local_address_ioctl(ifname, &local_address, SIOCGIFADDR);
    decode_network_address(&local_address, &octet1, &octet2, &octet3, &octet4);
    #ifdef BACDL_BIP
    bip_set_address(octet1, octet2, octet3, octet4);
    #endif
    fprintf(stderr,"IP Address: %d.%d.%d.%d\n",
      (int)octet1, (int)octet2, (int)octet3, (int)octet4);   
}

static void Init_Device_Parameters(void)
{
  // configure my initial values
  Device_Set_Object_Instance_Number(111);
  Device_Set_Vendor_Name("Lithonia Lighting");
  Device_Set_Vendor_Identifier(42);
  Device_Set_Model_Name("Simple BACnet Server");
  Device_Set_Firmware_Revision("1.00");
  Device_Set_Application_Software_Version("none");
  Device_Set_Description("Example of a simple BACnet server");

  return;
}
  
static void Init_Service_Handlers(void)
{
  // we need to handle who-is to support dynamic device binding
  apdu_set_unconfirmed_handler(
    SERVICE_UNCONFIRMED_WHO_IS,
    WhoIsHandler);
  apdu_set_unconfirmed_handler(
    SERVICE_UNCONFIRMED_I_AM,
    IAmHandler);

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

static void sig_handler(int signo)
{
  #ifdef BACDL_ETHERNET
  ethernet_cleanup();
  #endif
  #ifdef BACDL_BIP
  bip_cleanup();
  #endif

  exit(0);
}

int main(int argc, char *argv[])
{
  BACNET_ADDRESS src = {0};  // address where message came from
  uint16_t pdu_len = 0;
  unsigned timeout = 100; // milliseconds

  // Linux specials
  signal(SIGINT, sig_handler);
  signal(SIGHUP, sig_handler);
  signal(SIGTERM, sig_handler);
  // setup this BACnet Server
  Init_Device_Parameters();
  Init_Service_Handlers();
  #ifdef BACDL_ETHERNET
  // init the physical layer
  if (!ethernet_init("eth0"))
    return 1;
  #endif
  #ifdef BACDL_BIP
  Init_Network("eth0");
  if (!bip_init())
    return 1;
  #endif

  
  // loop forever
  for (;;)
  {
    // input

    // returns 0 bytes on timeout
    #ifdef BACDL_ETHERNET
    pdu_len = ethernet_receive(
      &src,
      &Rx_Buf[0],
      MAX_MPDU,
      timeout);
    #endif
    #ifdef BACDL_BIP
    pdu_len = bip_receive(
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

    // blink LEDs, Turn on or off outputs, etc
  }
  
  return 0;
}
