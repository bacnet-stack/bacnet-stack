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
#include <time.h>
#include "config.h"
#include "address.h"
#include "bacdef.h"
#include "handlers.h"
#include "bacdcode.h"
#include "npdu.h"
#include "apdu.h"
#include "iam.h"
#include "tsm.h"
#include "device.h"
#ifdef BACDL_ETHERNET
  #include "ethernet.h"
#endif
#ifdef BACDL_BIP
  #include "bip.h"
#endif
#include "net.h"

// This is an example application using the BACnet Stack on Linux

// buffers used for receiving
static uint8_t Rx_Buf[MAX_MPDU] = {0};

#ifdef BACDL_BIP
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
    struct in_addr broadcast_address;
    uint8_t octet1; 
    uint8_t octet2; 
    uint8_t octet3; 
    uint8_t octet4;

    /* setup local address */
    get_local_address_ioctl(ifname, &local_address, SIOCGIFADDR);
    decode_network_address(&local_address, &octet1, &octet2, &octet3, &octet4);
    bip_set_address(octet1, octet2, octet3, octet4);
    fprintf(stderr,"IP Address: %d.%d.%d.%d\n",
      (int)octet1, (int)octet2, (int)octet3, (int)octet4);   
    /* setup local broadcast address */
    get_local_address_ioctl(ifname, &broadcast_address, SIOCGIFBRDADDR);
    decode_network_address(&broadcast_address, &octet1, &octet2, &octet3, &octet4);
    bip_set_broadcast_address(octet1, octet2, octet3, octet4);
    fprintf(stderr,"Broadcast Address: %d.%d.%d.%d\n",
      (int)octet1, (int)octet2, (int)octet3, (int)octet4);   
}
#endif

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

static void LocalIAmHandler(
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

static void Read_Properties(void)
{
  uint32_t device_id = 0;
  unsigned max_apdu = 0;
  BACNET_ADDRESS src;
  bool next_device = false;
  static unsigned index = 0;
  static unsigned property = 0;
  // list of required (and some optional) properties in the
  // Device Object
  const int object_props[] =
  {
    75,77,79,112,121,120,70,44,12,98,95,97,96,
    62,107,57,56,119,24,10,11,73,116,64,63,30,
    514,515,
    // note: 76 is missing cause we get it special below
    -1
  };

  if (address_count())
  {
    if (address_get_by_index(index, &device_id, &max_apdu, &src))
    {
      if (object_props[property] < 0)
        next_device = true;
      else
      {
        (void)Send_Read_Property_Request(
          device_id, // destination device
          OBJECT_DEVICE,
          device_id,
          object_props[property],
          BACNET_ARRAY_ALL);
        property++;
      }
    }
    else
      next_device = true;
    if (next_device)
    {
      index++;
      if (index >= MAX_ADDRESS_CACHE)
        index = 0;
      property = 0;
    }
  }

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
    LocalIAmHandler);

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
  // handle the data coming back from confirmed requests
  apdu_set_confirmed_ack_handler(
    SERVICE_CONFIRMED_READ_PROPERTY,
    ReadPropertyAckHandler);
}

static void print_address_cache(void)
{
  unsigned i,j;
  BACNET_ADDRESS address;
  uint32_t device_id = 0;
  unsigned max_apdu = 0;
  
  fprintf(stderr,"Device\tMAC\tMaxAPDU\tNet\n");
  for (i = 0; i < MAX_ADDRESS_CACHE; i++)
  {
    if (address_get_by_index(i,&device_id, &max_apdu, &address))
    {
      fprintf(stderr,"%u\t",device_id);
      for (j = 0; j < address.mac_len; j++)
      {
        fprintf(stderr,"%02X",address.mac[j]);
      }
      fprintf(stderr,"\t");
      fprintf(stderr,"%hu\t",max_apdu);
      fprintf(stderr,"%hu\n",address.net);
    }
  }
}

static void sig_handler(int signo)
{
  #ifdef BACDL_ETHERNET
  ethernet_cleanup();
  #endif
  #ifdef BACDL_BIP
  bip_cleanup();
  #endif

  print_address_cache();
 
  exit(0);
}

int main(int argc, char *argv[])
{
  BACNET_ADDRESS src = {0};  // address where message came from
  uint16_t pdu_len = 0;
  unsigned timeout = 100; // milliseconds
  unsigned count = 0; // milliseconds
  time_t start_time;
  time_t new_time = 0;
  
  start_time = time(NULL);             /* get current time */
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
    new_time = time(NULL);
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
    if (new_time > start_time)
    {
      tsm_timer_milliseconds(new_time - start_time * 1000);
      start_time = new_time;
    }
    if (I_Am_Request)
    {
      I_Am_Request = false;
      Send_IAm();
    } else if (Who_Is_Request)
    {
      Who_Is_Request = false;
      Send_WhoIs();
    }
    // output
    // some round robin task switching
    count++;
    switch (count)
    {
      case 1:
        // used for testing, but kind of noisy on the network
        //Read_Properties();
        break;
      case 2:
        break;
      default:
        count = 0;
        break;
    }

    // blink LEDs, Turn on or off outputs, etc
  }
  
  return 0;
}
