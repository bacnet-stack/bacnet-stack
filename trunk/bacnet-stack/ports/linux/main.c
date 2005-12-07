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
#include "bacfile.h"
#include "datalink.h"
#include "net.h"

// This is an example application using the BACnet Stack on Linux

// buffers used for receiving
static uint8_t Rx_Buf[MAX_MPDU] = {0};

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
  bool status = false;
  unsigned max_apdu = 0;
  BACNET_ADDRESS src;
  bool next_device = false;
  static unsigned index = 0;
  static unsigned property = 0;
  /* list of required (and some optional and proprietary)
  properties in the Device Object.  Note that this demo
  tests for error messages so that the device doesn't have
  to have all the properties listed here. */
  const int object_props[] =
  {
    PROP_OBJECT_IDENTIFIER,
    PROP_OBJECT_NAME,
    PROP_OBJECT_TYPE,
    PROP_SYSTEM_STATUS,
    PROP_VENDOR_NAME,
    PROP_VENDOR_IDENTIFIER,
    PROP_MODEL_NAME,
    PROP_FIRMWARE_REVISION,
    PROP_APPLICATION_SOFTWARE_VERSION,
    PROP_PROTOCOL_VERSION,
    PROP_PROTOCOL_CONFORMANCE_CLASS,
    PROP_PROTOCOL_SERVICES_SUPPORTED,
    PROP_PROTOCOL_OBJECT_TYPES_SUPPORTED,
    PROP_MAX_APDU_LENGTH_ACCEPTED,
    PROP_SEGMENTATION_SUPPORTED,
    PROP_LOCAL_TIME,
    PROP_LOCAL_DATE,
    PROP_UTC_OFFSET,
    PROP_DAYLIGHT_SAVINGS_STATUS,
    PROP_APDU_SEGMENT_TIMEOUT,
    PROP_APDU_TIMEOUT,
    PROP_NUMBER_OF_APDU_RETRIES,
    PROP_TIME_SYNCHRONIZATION_RECIPIENTS,
    PROP_MAX_MASTER,
    PROP_MAX_INFO_FRAMES,
    PROP_DEVICE_ADDRESS_BINDING,
    /* note: PROP_OBJECT_LIST is missing because
    the result can be very large.  Read index 0
    which gives us the number of objects in the list,
    and then we can read index 1, 2.. n one by one,
    rather than trying to read the entire object
    list in one message. */
    /* some proprietary properties */
    514,515,
    /* end of list */
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
        /* note: if we wanted to do this synchronously, we would get the
          invoke ID from the sending of the request, and wait until we
          got the reply with matching invoke ID or the TSM of the
          invoke ID expired.  This demo is doing things asynchronously. */
        status = Send_Read_Property_Request(
          device_id, // destination device
          OBJECT_DEVICE,
          device_id,
          object_props[property],
          BACNET_ARRAY_ALL);
        if (status)
          property++;
      }
    }
    else
      next_device = true;
    if (next_device)
    {
      next_device = false;
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
  // Set the handlers for any confirmed services that we support.
  // We must implement read property - it's required!
  apdu_set_confirmed_handler(
    SERVICE_CONFIRMED_READ_PROPERTY,
    ReadPropertyHandler);
  apdu_set_confirmed_handler(
    SERVICE_CONFIRMED_WRITE_PROPERTY,
    WritePropertyHandler);
  apdu_set_confirmed_handler(
    SERVICE_CONFIRMED_ATOMIC_READ_FILE,
    AtomicReadFileHandler);
  // handle the data coming back from confirmed requests
  apdu_set_confirmed_ack_handler(
    SERVICE_CONFIRMED_READ_PROPERTY,
    ReadPropertyAckHandler);
  apdu_set_confirmed_ack_handler(
    SERVICE_CONFIRMED_ATOMIC_READ_FILE,
    AtomicReadFileAckHandler);
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
  datalink_cleanup();
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
  bip_set_interface("eth0");
  bip_set_port(0xBAC0);
  if (!bip_init())
    return 1;
  #endif
  #ifdef BACDL_ARCNET
  if (!arcnet_init("arc0"))
    return 1;
  #endif

  // loop forever
  for (;;)
  {
    // input
    new_time = time(NULL);
    // returns 0 bytes on timeout
    pdu_len = datalink_receive(
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
        Read_Properties();
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
