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
#define WIN32_LEAN_AND_MEAN
#define STRICT

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <process.h>

#ifndef HOST

   #include <rttarget.h>
   #include <rtk32.h>
   #include <clock.h>
   #include <socket.h>

   #include "netcfg.h"

   int interface = SOCKET_ERROR;  // SOCKET_ERROR means no open interface

#else

   #include <winsock.h>

#endif

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include "config.h"
#include "bacdef.h"
#include "bacdcode.h"
#include "npdu.h"
#include "apdu.h"
#include "device.h"
#include "ai.h"
#include "rp.h"
#include "wp.h"
#include "iam.h"
#include "whois.h"
#include "reject.h"
#include "abort.h"
#include "bacerror.h"
#include "bip.h"

// buffers used for transmit and receive
static uint8_t Tx_Buf[MAX_MPDU] = {0};
static uint8_t Rx_Buf[MAX_MPDU] = {0};
static uint8_t Temp_Buf[MAX_APDU] = {0};

// flag to send an I-Am
bool I_Am_Request = true;

void UnrecognizedServiceHandler(
  uint8_t *service_request,
  uint16_t service_len,
  BACNET_ADDRESS *dest,
  BACNET_CONFIRMED_SERVICE_DATA *service_data)
{
  BACNET_ADDRESS src;
  int pdu_len = 0;
  
  (void)service_request;
  (void)service_len;
  bip_get_my_address(&src);

  // encode the NPDU portion of the packet
  pdu_len = npdu_encode_apdu(
    &Tx_Buf[0],
    dest,
    &src,
    false,  // true for confirmed messages
    MESSAGE_PRIORITY_NORMAL);
  
  // encode the APDU portion of the packet
  pdu_len += reject_encode_apdu(
    &Tx_Buf[pdu_len], 
    service_data->invoke_id,
    REJECT_REASON_UNRECOGNIZED_SERVICE);

  (void)bip_send_pdu(
    dest,  // destination address
    &Tx_Buf[0],
    pdu_len); // number of bytes of data
  fprintf(stderr,"Sent Reject!\n");
}

// FIXME: if we handle multiple ports, then a port neutral version
// of this would be nice. Then it could be moved into iam.c
void Send_IAm(void)
{
  int pdu_len = 0;
  BACNET_ADDRESS dest;

  // I-Am is a global broadcast
  bip_set_broadcast_address(&dest);

  // encode the NPDU portion of the packet
  pdu_len = npdu_encode_apdu(
    &Tx_Buf[0],
    &dest,
    NULL,
    false,  // true for confirmed messages
    MESSAGE_PRIORITY_NORMAL);

  // encode the APDU portion of the packet
  pdu_len += iam_encode_apdu(
    &Tx_Buf[pdu_len], 
    Device_Object_Instance_Number(),
    MAX_APDU,
    SEGMENTATION_NONE,
    Device_Vendor_Identifier());

  (void)bip_send_pdu(
    &dest,  // destination address
    &Tx_Buf[0],
    pdu_len); // number of bytes of data
  fprintf(stderr,"Sent I-Am Request!\n");
}

void WhoIsHandler(
  uint8_t *service_request,
  uint16_t service_len,
  BACNET_ADDRESS *src)
{
  int len = 0;
  int32_t low_limit = 0;
  int32_t high_limit = 0;

  (void)src;
  fprintf(stderr,"Received Who-Is Request!\n");
  len = whois_decode_service_request(
    service_request,
    service_len,
    &low_limit,
    &high_limit);
  if (len == 0)
    I_Am_Request = true;
  else if (len != -1)
  { 
    if ((Device_Object_Instance_Number() >= (uint32_t)low_limit) && 
        (Device_Object_Instance_Number() <= (uint32_t)high_limit))
      I_Am_Request = true;
  }
  else 
    fprintf(stderr,"Who-Is Not for Me!\n");

  return;  
}

void IAmHandler(
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
  if (len != -1)
    fprintf(stderr,"Received I-Am Request from %u!\n",device_id);

  return;  
}

void ReadPropertyHandler(
  uint8_t *service_request,
  uint16_t service_len,
  BACNET_ADDRESS *src,
  BACNET_CONFIRMED_SERVICE_DATA *service_data)
{
  BACNET_READ_PROPERTY_DATA rp_data;
  int len = 0;
  int pdu_len = 0;
  BACNET_OBJECT_TYPE object_type;
  uint32_t object_instance;
  BACNET_PROPERTY_ID object_property;
  int32_t array_index;
  BACNET_ADDRESS my_address;
  bool send = false;

  len = rp_decode_service_request(
    service_request,
    service_len,
    &object_type,
    &object_instance,
    &object_property,
    &array_index);
  fprintf(stderr,"Received Read-Property Request!\n");
  if (len > 0)
    fprintf(stderr,"type=%u instance=%u property=%u index=%d\n",
      object_type,
      object_instance,
      object_property,
      array_index);
  else
    fprintf(stderr,"Unable to decode Read-Property Request!\n");
  // prepare a reply
  bip_get_my_address(&my_address);
  // encode the NPDU portion of the packet
  pdu_len = npdu_encode_apdu(
    &Tx_Buf[0],
    src,
    &my_address,
    false,  // true for confirmed messages
    MESSAGE_PRIORITY_NORMAL);
  // bad decoding - send an abort
  if (len == -1)
  {
    pdu_len += abort_encode_apdu(
      &Tx_Buf[pdu_len],
      service_data->invoke_id,
      ABORT_REASON_OTHER);
    fprintf(stderr,"Sent Abort!\n");
    send = true;
  }
  else if (service_data->segmented_message)
  {
    pdu_len += abort_encode_apdu(
      &Tx_Buf[pdu_len],
      service_data->invoke_id,
      ABORT_REASON_SEGMENTATION_NOT_SUPPORTED);
    fprintf(stderr,"Sent Abort!\n");
    send = true;
  }
  else
  {
    switch (object_type)
    {
      case OBJECT_DEVICE:
        // FIXME: probably need a length limitation sent with encode
        // FIXME: might need to return error codes
        if (object_instance == Device_Object_Instance_Number())
        {
          len = Device_Encode_Property_APDU(
            &Temp_Buf[0],
            object_property,
            array_index);
          if (len > 0)
          {
            // encode the APDU portion of the packet
            rp_data.object_type = object_type;
            rp_data.object_instance = object_instance;
            rp_data.object_property = object_property;
            rp_data.array_index = array_index;
            rp_data.application_data = &Temp_Buf[0];
            rp_data.application_data_len = len;
            // FIXME: probably need a length limitation sent with encode
            pdu_len += rp_ack_encode_apdu(
              &Tx_Buf[pdu_len],
              service_data->invoke_id,
              &rp_data);
            fprintf(stderr,"Sent Read Property Ack!\n");
            send = true;
          }
          else
          {
            pdu_len += bacerror_encode_apdu(
              &Tx_Buf[pdu_len],
              service_data->invoke_id,
              SERVICE_CONFIRMED_READ_PROPERTY,
              ERROR_CLASS_PROPERTY,
              ERROR_CODE_UNKNOWN_PROPERTY);
            fprintf(stderr,"Sent Unknown Property Error!\n");
            send = true;
          }
        }
        else
        {
          pdu_len += bacerror_encode_apdu(
            &Tx_Buf[pdu_len],
            service_data->invoke_id,
            SERVICE_CONFIRMED_READ_PROPERTY,
            ERROR_CLASS_OBJECT,
            ERROR_CODE_UNKNOWN_OBJECT);
          fprintf(stderr,"Sent Unknown Object Error!\n");
          send = true;
        }
        break;
      case OBJECT_ANALOG_INPUT:
        if (Analog_Input_Valid_Instance(object_instance))
        {
          len = Analog_Input_Encode_Property_APDU(
            &Temp_Buf[0],
            object_instance,
            object_property,
            array_index);
          if (len > 0)
          {
            // encode the APDU portion of the packet
            rp_data.object_type = object_type;
            rp_data.object_instance = object_instance;
            rp_data.object_property = object_property;
            rp_data.array_index = array_index;
            rp_data.application_data = &Temp_Buf[0];
            rp_data.application_data_len = len;
            // FIXME: probably need a length limitation sent with encode
            pdu_len += rp_ack_encode_apdu(
              &Tx_Buf[pdu_len],
              service_data->invoke_id,
              &rp_data);
            fprintf(stderr,"Sent Read Property Ack!\n");
            send = true;
          }
          else
          {
            pdu_len += bacerror_encode_apdu(
              &Tx_Buf[pdu_len],
              service_data->invoke_id,
              SERVICE_CONFIRMED_READ_PROPERTY,
              ERROR_CLASS_PROPERTY,
              ERROR_CODE_UNKNOWN_PROPERTY);
            fprintf(stderr,"Sent Unknown Property Error!\n");
            send = true;
          }
        }
        else
        {
          pdu_len += bacerror_encode_apdu(
            &Tx_Buf[pdu_len],
            service_data->invoke_id,
            SERVICE_CONFIRMED_READ_PROPERTY,
            ERROR_CLASS_OBJECT,
            ERROR_CODE_UNKNOWN_OBJECT);
          fprintf(stderr,"Sent Unknown Object Error!\n");
          send = true;
        }
        break;
      default:
        pdu_len += bacerror_encode_apdu(
          &Tx_Buf[pdu_len],
          service_data->invoke_id,
          SERVICE_CONFIRMED_READ_PROPERTY,
          ERROR_CLASS_OBJECT,
          ERROR_CODE_UNKNOWN_OBJECT);
        fprintf(stderr,"Sent Unknown Object Error!\n");
        send = true;
        break;
    }
  }
  if (send)
  {
    (void)bip_send_pdu(
      src,  // destination address
      &Tx_Buf[0],
      pdu_len); // number of bytes of data
  }

  return;
}

void WritePropertyHandler(
  uint8_t *service_request,
  uint16_t service_len,
  BACNET_ADDRESS *src,
  BACNET_CONFIRMED_SERVICE_DATA *service_data)
{
  BACNET_WRITE_PROPERTY_DATA wp_data;
  int len = 0;
  int pdu_len = 0;
  BACNET_ADDRESS my_address;
  bool send = false;
  BACNET_ERROR_CLASS error_class;
  BACNET_ERROR_CODE error_code;

  // decode the service request only
  len = wp_decode_service_request(
    service_request,
    service_len,
    &wp_data);
  fprintf(stderr,"Received Write-Property Request!\n");
  if (len > 0)
    fprintf(stderr,"type=%u instance=%u property=%u index=%d\n",
      wp_data.object_type,
      wp_data.object_instance,
      wp_data.object_property,
      wp_data.array_index);
  else
    fprintf(stderr,"Unable to decode Write-Property Request!\n");
  // prepare a reply
  bip_get_my_address(&my_address);
  // encode the NPDU portion of the packet
  pdu_len = npdu_encode_apdu(
    &Tx_Buf[0],
    src,
    &my_address,
    false,  // true for confirmed messages
    MESSAGE_PRIORITY_NORMAL);
  // bad decoding - send an abort
  if (len == -1)
  {
    pdu_len += abort_encode_apdu(
      &Tx_Buf[pdu_len],
      service_data->invoke_id,
      ABORT_REASON_OTHER);
    fprintf(stderr,"Sent Abort!\n");
    send = true;
  }
  else if (service_data->segmented_message)
  {
    pdu_len += abort_encode_apdu(
      &Tx_Buf[pdu_len],
      service_data->invoke_id,
      ABORT_REASON_SEGMENTATION_NOT_SUPPORTED);
    fprintf(stderr,"Sent Abort!\n");
    send = true;
  }
  else
  {
    switch (wp_data.object_type)
    {
      case OBJECT_DEVICE:
        if (Device_Write_Property(&wp_data,&error_class,&error_code))
        {
          pdu_len = encode_simple_ack(
            &Tx_Buf[pdu_len],
            service_data->invoke_id,
            SERVICE_CONFIRMED_WRITE_PROPERTY);
          fprintf(stderr,"Sent Write Property Simple Ack!\n");
          send = true;
        }
        else
        {
          pdu_len += bacerror_encode_apdu(
            &Tx_Buf[pdu_len],
            service_data->invoke_id,
            SERVICE_CONFIRMED_WRITE_PROPERTY,
            error_class,
            error_code);
          fprintf(stderr,"Sent Write Property Error!\n");
          send = true;
        }
        break;
      case OBJECT_ANALOG_INPUT:
        pdu_len += bacerror_encode_apdu(
          &Tx_Buf[pdu_len],
          service_data->invoke_id,
          SERVICE_CONFIRMED_WRITE_PROPERTY,
          ERROR_CLASS_PROPERTY,
          ERROR_CODE_WRITE_ACCESS_DENIED);
        fprintf(stderr,"Sent Write Access Error!\n");
        send = true;
        break;
      default:
        pdu_len += bacerror_encode_apdu(
          &Tx_Buf[pdu_len],
          service_data->invoke_id,
          SERVICE_CONFIRMED_WRITE_PROPERTY,
          ERROR_CLASS_OBJECT,
          ERROR_CODE_UNKNOWN_OBJECT);
        fprintf(stderr,"Sent Unknown Object Error!\n");
        send = true;
        break;
    }
  }
  if (send)
  {
    (void)bip_send_pdu(
      src,  // destination address
      &Tx_Buf[0],
      pdu_len); // number of bytes of data
  }

  return;
}

static void Init_Device_Parameters(void)
{
  // configure my initial values
  Device_Set_Object_Instance_Number(112);
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

/*-----------------------------------*/
static void Error(const char * Msg)
{
   int Code = WSAGetLastError();
#ifdef HOST
   printf("%s, error code: %i\n", Msg, Code);
#else
   printf("%s, error code: %s\n", Msg, xn_geterror_string(Code));
#endif
   exit(1);
}

#ifndef HOST
/*-----------------------------------*/
void InterfaceCleanup(void)
{
   if (interface != SOCKET_ERROR)
   {
      xn_interface_close(interface);
      interface = SOCKET_ERROR;
   #if DEVICE_ID == PRISM_PCMCIA_DEVICE
      RTPCShutDown();
   #endif
   }
}
#endif

static void NetInitialize(void)
// initialize the TCP/IP stack
{
   int Result;

#ifndef HOST

   RTKernelInit(0);                   // get the kernel going

   if (!RTKDebugVersion())            // switch of all diagnostics and error messages of RTIP-32
      xn_callbacks()->cb_wr_screen_string_fnc = NULL;

   CLKSetTimerIntVal(10*1000);        // 10 millisecond tick
   RTKDelay(1);
   RTCMOSSetSystemTime();             // get the right time-of-day

#ifdef RTUSB_VER
   RTURegisterCallback(USBAX172);     // ax172 and ax772 drivers
   RTURegisterCallback(USBAX772);
   RTURegisterCallback(USBKeyboard);  // support USB keyboards
   FindUSBControllers();              // install USB host controllers
   Sleep(2000);                       // give the USB stack time to enumerate devices
#endif

#ifdef DHCP
   XN_REGISTER_DHCP_CLI()             // and optionally the DHCP client
#endif

   Result = xn_rtip_init();           // Initialize the RTIP stack
   if (Result != 0)
      Error("xn_rtip_init failed");

   atexit(InterfaceCleanup);                          // make sure the driver is shut down properly
   RTCallDebugger(RT_DBG_CALLRESET, (DWORD)exit, 0);  // even if we get restarted by the debugger

   Result = BIND_DRIVER(MINOR_0);     // tell RTIP what Ethernet driver we want (see netcfg.h)
   if (Result != 0)
      Error("driver initialization failed");

#if DEVICE_ID == PRISM_PCMCIA_DEVICE
   // if this is a PCMCIA device, start the PCMCIA driver
   if (RTPCInit(-1, 0, 2, NULL) == 0)
      Error("No PCMCIA controller found");
#endif

   // Open the interface
   interface = xn_interface_open_config(DEVICE_ID, MINOR_0, ED_IO_ADD, ED_IRQ, ED_MEM_ADD);
   if (interface == SOCKET_ERROR)
      Error("xn_interface_open_config failed");
   else
   {
      struct _iface_info ii;
      xn_interface_info(interface, &ii);
      printf("Interface opened, MAC address: %02x-%02x-%02x-%02x-%02x-%02x\n",
         ii.my_ethernet_address[0], ii.my_ethernet_address[1], ii.my_ethernet_address[2],
         ii.my_ethernet_address[3], ii.my_ethernet_address[4], ii.my_ethernet_address[5]);
   }

#if DEVICE_ID == PRISM_PCMCIA_DEVICE || DEVICE_ID == PRISM_DEVICE
   xn_wlan_setup(interface,          // iface_no: value returned by xn_interface_open_config()
                 "network name",     // SSID    : network name set in the access point
                 "station name",     // Name    : name of this node
                 0,                  // Channel : 0 for access points, 1..14 for ad-hoc
                 0,                  // KeyIndex: 0 .. 3
                 "12345",            // WEP Key : key to use (5 or 13 bytes)
                 0);                 // Flags   : see manual and Wlanapi.h for details
   Sleep(1000); // wireless devices need a little time before they can be used
#endif // WLAN device

#if defined(AUTO_IP)  // use xn_autoip() to get an IP address
   Result = xn_autoip(interface, MinIP, MaxIP, NetMask, TargetIP);
   if (Result == SOCKET_ERROR)
      Error("xn_autoip failed");
   else
   {
      printf("Auto-assigned IP address %i.%i.%i.%i\n", TargetIP[0], TargetIP[1], TargetIP[2], TargetIP[3]);
      // define default gateway and DNS server
      xn_rt_add(RT_DEFAULT, ip_ffaddr, DefaultGateway, 1, interface, RT_INF);
      xn_set_server_list((DWORD*)DNSServer, 1);
   }
#elif defined(DHCP)   // use DHCP
   {
      DHCP_param   param[] = {{SUBNET_MASK, 1}, {DNS_OP, 1}, {ROUTER_OPTION, 1}};
      DHCP_session DS;
      DHCP_conf    DC;

      xn_init_dhcp_conf(&DC);                 // load default DHCP options
      DC.plist = param;                       // add MASK, DNS, and gateway options
      DC.plist_entries = sizeof(param) / sizeof(param[0]);
      printf("Contacting DHCP server, please wait...\n");
      Result = xn_dhcp(interface, &DS, &DC);  // contact DHCP server
      if (Result == SOCKET_ERROR)
         Error("xn_dhcp failed");
      memcpy(TargetIP, DS.client_ip, 4);
      printf("My IP address is: %i.%i.%i.%i\n", TargetIP[0], TargetIP[1], TargetIP[2], TargetIP[3]);
   }
#else
   // Set the IP address and interface
   printf("Using static IP address %i.%i.%i.%i\n", TargetIP[0], TargetIP[1], TargetIP[2], TargetIP[3]);
   Result = xn_set_ip(interface, TargetIP, NetMask);
   // define default gateway and DNS server
   xn_rt_add(RT_DEFAULT, ip_ffaddr, DefaultGateway, 1, interface, RT_INF);
   xn_set_server_list((DWORD*)DNSServer, 1);
#endif

#else  // HOST defined, run on Windows

   WSADATA wd;
   Result = WSAStartup(0x0101, &wd);

#endif

   if (Result != 0)
      Error("TCP/IP stack initialization failed");
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
  // init the physical layer
  NetInitialize();
  bip_set_address(TargetIP[0], TargetIP[1], TargetIP[2], TargetIP[3]);
  if (!bip_init())
    return 1;
  
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
