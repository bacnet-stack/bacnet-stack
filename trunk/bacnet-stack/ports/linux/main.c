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
#include "iam.h"
#include "whois.h"
#include "reject.h"
#include "ethernet.h"

// buffers used for transmit and receive
static uint8_t Tx_Buf[MAX_MPDU] = {0};
static uint8_t Rx_Buf[MAX_MPDU] = {0};

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
  BACNET_ADDRESS src;

  // I-Am is a global broadcast
  ethernet_set_broadcast_address(&dest);
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
    if ((Device_Id >= low_limit) && (Device_Id <= high_limit))
      I_Am_Request = true;
  }

  return;  
}

static void Init_Device_Parameters(void)
{
  // configure my initial values
  Device_Set_Object_Identifier(111);
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
  // custom handlers
  apdu_set_unconfirmed_handler(SERVICE_UNCONFIRMED_WHO_IS,WhoIsHandler);

  // set the handler for all the services we don't implement
  // It is required to send the proper reject message...
  apdu_set_confirmed_handler(
    SERVICE_CONFIRMED_ACKNOWLEDGE_ALARM,
    UnrecognizedServiceHandler);
  apdu_set_confirmed_handler(
    SERVICE_CONFIRMED_COV_NOTIFICATION,
    UnrecognizedServiceHandler);
  apdu_set_confirmed_handler(
    SERVICE_CONFIRMED_EVENT_NOTIFICATION,
    UnrecognizedServiceHandler);
  apdu_set_confirmed_handler(
    SERVICE_CONFIRMED_GET_ALARM_SUMMARY,
    UnrecognizedServiceHandler);
  apdu_set_confirmed_handler(
    SERVICE_CONFIRMED_GET_ENROLLMENT_SUMMARY,
    UnrecognizedServiceHandler);
  apdu_set_confirmed_handler(
    SERVICE_CONFIRMED_GET_EVENT_INFORMATION,
    UnrecognizedServiceHandler);
  apdu_set_confirmed_handler(
    SERVICE_CONFIRMED_SUBSCRIBE_COV,
    UnrecognizedServiceHandler);
  apdu_set_confirmed_handler(
    SERVICE_CONFIRMED_SUBSCRIBE_COV_PROPERTY,
    UnrecognizedServiceHandler);
  apdu_set_confirmed_handler(
    SERVICE_CONFIRMED_LIFE_SAFETY_OPERATION,
    UnrecognizedServiceHandler);
  apdu_set_confirmed_handler(
    SERVICE_CONFIRMED_ATOMIC_READ_FILE,
    UnrecognizedServiceHandler);
  apdu_set_confirmed_handler(
    SERVICE_CONFIRMED_ATOMIC_WRITE_FILE,
    UnrecognizedServiceHandler);
  apdu_set_confirmed_handler(
    SERVICE_CONFIRMED_ADD_LIST_ELEMENT,
    UnrecognizedServiceHandler);
  apdu_set_confirmed_handler(
    SERVICE_CONFIRMED_REMOVE_LIST_ELEMENT,
    UnrecognizedServiceHandler);
  apdu_set_confirmed_handler(
    SERVICE_CONFIRMED_CREATE_OBJECT,
    UnrecognizedServiceHandler);
  apdu_set_confirmed_handler(
    SERVICE_CONFIRMED_DELETE_OBJECT,
    UnrecognizedServiceHandler);
  // FIXME: we must implement read property - it's required!
  apdu_set_confirmed_handler(
    SERVICE_CONFIRMED_READ_PROPERTY,
    UnrecognizedServiceHandler);
  apdu_set_confirmed_handler(
    SERVICE_CONFIRMED_READ_PROPERTY_CONDITIONAL,
    UnrecognizedServiceHandler);
  apdu_set_confirmed_handler(
    SERVICE_CONFIRMED_READ_PROPERTY_MULTIPLE,
    UnrecognizedServiceHandler);
  apdu_set_confirmed_handler(
    SERVICE_CONFIRMED_READ_RANGE,
    UnrecognizedServiceHandler);
  // FIXME: we probably want to implement write property to be useful
  apdu_set_confirmed_handler(
    SERVICE_CONFIRMED_WRITE_PROPERTY,
    UnrecognizedServiceHandler);
  apdu_set_confirmed_handler(
    SERVICE_CONFIRMED_WRITE_PROPERTY_MULTIPLE,
    UnrecognizedServiceHandler);
  apdu_set_confirmed_handler(
    SERVICE_CONFIRMED_DEVICE_COMMUNICATION_CONTROL,
    UnrecognizedServiceHandler);
  apdu_set_confirmed_handler(
    SERVICE_CONFIRMED_PRIVATE_TRANSFER,
    UnrecognizedServiceHandler);
  apdu_set_confirmed_handler(
    SERVICE_CONFIRMED_TEXT_MESSAGE,
    UnrecognizedServiceHandler);
  apdu_set_confirmed_handler(
    SERVICE_CONFIRMED_REINITIALIZE_DEVICE,
    UnrecognizedServiceHandler);
  apdu_set_confirmed_handler(
    SERVICE_CONFIRMED_VT_OPEN,
    UnrecognizedServiceHandler);
  apdu_set_confirmed_handler(
    SERVICE_CONFIRMED_VT_CLOSE,
    UnrecognizedServiceHandler);
  apdu_set_confirmed_handler(
    SERVICE_CONFIRMED_VT_DATA,
    UnrecognizedServiceHandler);
  apdu_set_confirmed_handler(
    SERVICE_CONFIRMED_AUTHENTICATE,
    UnrecognizedServiceHandler);
  apdu_set_confirmed_handler(
    SERVICE_CONFIRMED_REQUEST_KEY,
    UnrecognizedServiceHandler);
}

int main(int argc, char *argv[])
{
  BACNET_ADDRESS src = {0};  // address where message came from
  uint16_t pdu_len = 0;

  Init_Device_Parameters();
  Init_Service_Handlers();
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
