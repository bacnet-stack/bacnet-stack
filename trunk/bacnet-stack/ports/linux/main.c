// This example file is not copyrighted so that you can use it as you wish.
// Written by Steve Karg - 2005 - skarg@users.sourceforge.net
// Bug fixes, feature requests, and suggestions are welcome

// This is one way to use the embedded BACnet stack under Linux

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include "config.h"
#include "bacdef.h"
#include "npdu.h"
#include "apdu.h"
#include "device.h"
#include "rp.h"
#include "iam.h"
#include "whois.h"
#include "reject.h"
#include "abort.h"
#include "bacerror.h"
#include "ethernet.h"

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
  
  ethernet_get_my_address(&src);

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

  (void)ethernet_send_pdu(
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
  ethernet_set_broadcast_address(&dest);

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

  (void)ethernet_send_pdu(
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
    if ((Device_Object_Instance_Number() >= low_limit) && 
        (Device_Object_Instance_Number() <= high_limit))
      I_Am_Request = true;
  }

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

  fprintf(stderr,"Received Read-Property Request!\n");
  len = rp_decode_service_request(
    service_request,
    service_len,
    &object_type,
    &object_instance,
    &object_property,
    &array_index);
  // prepare a reply
  ethernet_get_my_address(&my_address);
  // encode the NPDU portion of the packet
  pdu_len = npdu_encode_apdu(
    &Tx_Buf[0],
    src,
    &my_address,
    false,  // true for confirmed messages
    MESSAGE_PRIORITY_NORMAL);
  // bad encoding - send an abort
  if (len == -1)
  {
    pdu_len += abort_encode_apdu(
      &Tx_Buf[pdu_len],
      service_data->invoke_id,
      ABORT_REASON_OTHER);
    (void)ethernet_send_pdu(
      src,  // destination address
      &Tx_Buf[0],
      pdu_len); // number of bytes of data
    fprintf(stderr,"Sent Abort!\n");
  }
  else if (service_data->segmented_message)
  {
    pdu_len += abort_encode_apdu(
      &Tx_Buf[pdu_len],
      service_data->invoke_id,
      ABORT_REASON_SEGMENTATION_NOT_SUPPORTED);
    (void)ethernet_send_pdu(
      src,  // destination address
      &Tx_Buf[0],
      pdu_len); // number of bytes of data
  }
  else
  {
    switch (object_type)
    {
      case OBJECT_DEVICE:
        // FIXME: probably need a length limitation sent with encode
        // FIXME: might need to return error codes
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
          (void)ethernet_send_pdu(
            src,  // destination address
            &Tx_Buf[0],
            pdu_len); // number of bytes of data
          fprintf(stderr,"Sent Read Property Ack!\n");
        }
        else
        {
          pdu_len += bacerror_encode_apdu(
            &Tx_Buf[pdu_len],
            service_data->invoke_id,
            SERVICE_CONFIRMED_READ_PROPERTY,
            ERROR_CLASS_PROPERTY,
            ERROR_CODE_UNKNOWN_PROPERTY);
          (void)ethernet_send_pdu(
            src,  // destination address
            &Tx_Buf[0],
            pdu_len); // number of bytes of data
          fprintf(stderr,"Sent Unknown Property Error!\n");
        }
        break;
      default:
        pdu_len += bacerror_encode_apdu(
          &Tx_Buf[pdu_len],
          service_data->invoke_id,
          SERVICE_CONFIRMED_READ_PROPERTY,
          ERROR_CLASS_OBJECT,
          ERROR_CODE_UNKNOWN_OBJECT);
        (void)ethernet_send_pdu(
          src,  // destination address
          &Tx_Buf[0],
          pdu_len); // number of bytes of data
        fprintf(stderr,"Sent Unknown Object Error!\n");
        break;
    }
  }

  return;
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

  // set the handler for all the services we don't implement
  // It is required to send the proper reject message...
  apdu_set_unrecognized_service_handler_handler(
    UnrecognizedServiceHandler);
  // we must implement read property - it's required!
  apdu_set_confirmed_handler(
    SERVICE_CONFIRMED_READ_PROPERTY,
    ReadPropertyHandler);
}

int main(int argc, char *argv[])
{
  BACNET_ADDRESS src = {0};  // address where message came from
  uint16_t pdu_len = 0;
  unsigned timeout = 100; // milliseconds

  Init_Device_Parameters();
  Init_Service_Handlers();
  // init the physical layer
  if (!ethernet_init("eth0"))
    return 1;
  
  // loop forever
  for (;;)
  {
    // input

    // returns 0 bytes on timeout
    pdu_len = ethernet_receive(
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
  
  return 0;
}
