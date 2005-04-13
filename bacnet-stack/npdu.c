/*####COPYRIGHTBEGIN####
 -------------------------------------------
 Copyright (C) 2005 Steve Karg

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
 Boston, MA  02111-1307, USA.

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
#include <stdbool.h>
#include <stdint.h>
#include "bacdef.h"
#include "bacdcode.h"
#include "bacenum.h"
#include "bits.h"
#include "npdu.h"
#include "apdu.h"

int npdu_encode_raw(
  uint8_t *npdu,
  BACNET_ADDRESS *dest,
  BACNET_ADDRESS *src,
  BACNET_NPDU_DATA *npdu_data)
{
  int len = 0; // return value - number of octets loaded in this function
  int i = 0; // counter 

  if (npdu && npdu_data)
  {
    // Protocol Version
    npdu[0] = 1;
    // control octet
    npdu[1] = 0;
    // Bit 7: 1 indicates that the NSDU conveys a network layer message.
    //          Message Type field is present.
    //        0 indicates that the NSDU contains a BACnet APDU.
    //          Message Type field is absent.
    if (npdu_data->network_layer_message)
      npdu[1] |= BIT7;
    //Bit 6: Reserved. Shall be zero.
    //Bit 5: Destination specifier where:
    // 0 = DNET, DLEN, DADR, and Hop Count absent
    // 1 = DNET, DLEN, and Hop Count present
    // DLEN = 0 denotes broadcast MAC DADR and DADR field is absent
    // DLEN > 0 specifies length of DADR field
    if (dest && dest->net)
      npdu[1] |= BIT5;
    // Bit 4: Reserved. Shall be zero.
    // Bit 3: Source specifier where:
    // 0 =  SNET, SLEN, and SADR absent
    // 1 =  SNET, SLEN, and SADR present
    // SLEN = 0 Invalid
    // SLEN > 0 specifies length of SADR field
    if (src && src->net)
      npdu[1] |= BIT3;
    // Bit 2: The value of this bit corresponds to the data_expecting_reply
    // parameter in the N-UNITDATA primitives.
    // 1 indicates that a BACnet-Confirmed-Request-PDU,
    // a segment of a BACnet-ComplexACK-PDU,
    // or a network layer message expecting a reply is present.
    // 0 indicates that other than a BACnet-Confirmed-Request-PDU,
    // a segment of a BACnet-ComplexACK-PDU,
    // or a network layer message expecting a reply is present.
    if (npdu_data->data_expecting_reply)
      npdu[1] |= BIT2;
    // Bits 1,0: Network priority where:
    // B'11' = Life Safety message
    // B'10' = Critical Equipment message
    // B'01' = Urgent message
    // B'00' = Normal message
    npdu[1] |= (npdu_data->priority & 0x03);
    len = 2;
    if (dest && dest->net)
    {
      len += encode_unsigned16(&npdu[len], dest->net);
      npdu[len++] = dest->len;
      // DLEN = 0 denotes broadcast MAC DADR and DADR field is absent
      // DLEN > 0 specifies length of DADR field
      if (dest->len)
      {
        for (i = 0; i < dest->len; i++)
        {
          npdu[len++] = dest->adr[i];
        }
      }
    }
    if (src && src->net)
    {
      len += encode_unsigned16(&npdu[len], src->net);
      npdu[len++] = src->len;
      // SLEN = 0 denotes broadcast MAC SADR and SADR field is absent
      // SLEN > 0 specifies length of SADR field
      if (src->len)
      {
        for (i = 0; i < src->len; i++)
        {
          npdu[len++] = src->adr[i];
        }
      }
    }
    // The Hop Count field shall be present only if the message is
    // destined for a remote network, i.e., if DNET is present.
    // This is a one-octet field that is initialized to a value of 0xff.
    if (dest && dest->net)
    {
      npdu[len] = 0xFF;
      len++;
    }
    if (npdu_data->network_layer_message)
    {
      npdu[len] = npdu_data->network_message_type;
      len++;
      // Message Type field contains a value in the range 0x80 - 0xFF,
      // then a Vendor ID field shall be present
      if (npdu_data->network_message_type >= 0x80)
        len += encode_unsigned16(&npdu[len], npdu_data->vendor_id);
    }
  }

  return len;
}

// encode the NPDU portion of the packet for an APDU
// This function does not handle the network messages, just APDUs.
int npdu_encode_apdu(
  uint8_t *npdu,
  BACNET_ADDRESS *dest,
  BACNET_ADDRESS *src,
  bool data_expecting_reply,  // true for confirmed messages
  BACNET_MESSAGE_PRIORITY priority)
{
  BACNET_NPDU_DATA npdu_data = {0};

  npdu_data.data_expecting_reply = data_expecting_reply;
  npdu_data.network_layer_message = false; // false if APDU
  npdu_data.network_message_type = 0; // optional
  npdu_data.vendor_id = 0; // optional, if net message type is > 0x80
  npdu_data.priority = priority;
  // call the real function...
  return npdu_encode_raw(
    npdu,
    dest,
    src,
    &npdu_data);
}

int npdu_decode(
  uint8_t *npdu,
  BACNET_ADDRESS *dest,
  BACNET_ADDRESS *src,
  BACNET_NPDU_DATA *npdu_data)
{
  int len = 0; // return value - number of octets loaded in this function
  int i = 0; // counter 

  if (npdu && npdu_data)
  {
    // Protocol Version
    npdu_data->protocol_version = npdu[0];
    // control octet
    // Bit 7: 1 indicates that the NSDU conveys a network layer message.
    //          Message Type field is present.
    //        0 indicates that the NSDU contains a BACnet APDU.
    //          Message Type field is absent.
    npdu_data->network_layer_message = (npdu[1] & BIT7) ? true : false;
    //Bit 6: Reserved. Shall be zero.
    // Bit 4: Reserved. Shall be zero.
    // Bit 2: The value of this bit corresponds to the data_expecting_reply
    // parameter in the N-UNITDATA primitives.
    // 1 indicates that a BACnet-Confirmed-Request-PDU,
    // a segment of a BACnet-ComplexACK-PDU,
    // or a network layer message expecting a reply is present.
    // 0 indicates that other than a BACnet-Confirmed-Request-PDU,
    // a segment of a BACnet-ComplexACK-PDU,
    // or a network layer message expecting a reply is present.
    npdu_data->data_expecting_reply = (npdu[1] & BIT2) ? true : false;
    // Bits 1,0: Network priority where:
    // B'11' = Life Safety message
    // B'10' = Critical Equipment message
    // B'01' = Urgent message
    // B'00' = Normal message
    npdu_data->priority = npdu[1] & 0x03;
    // set the offset to where the optional stuff starts
    len = 2;
    //Bit 5: Destination specifier where:
    // 0 = DNET, DLEN, DADR, and Hop Count absent
    // 1 = DNET, DLEN, and Hop Count present
    // DLEN = 0 denotes broadcast MAC DADR and DADR field is absent
    // DLEN > 0 specifies length of DADR field
    if (dest && (npdu[1] & BIT5))
    {
      len += decode_unsigned16(&npdu[len], &dest->net);
      // DLEN = 0 denotes broadcast MAC DADR and DADR field is absent
      // DLEN > 0 specifies length of DADR field
      dest->len = npdu[len++];
      if (dest->len)
      {
        for (i = 0; i < dest->len; i++)
        {
          dest->adr[i] = npdu[len++];
        }
      }
    }
    // Bit 3: Source specifier where:
    // 0 =  SNET, SLEN, and SADR absent
    // 1 =  SNET, SLEN, and SADR present
    // SLEN = 0 Invalid
    // SLEN > 0 specifies length of SADR field
    if (src && (npdu[1] & BIT3))
    {
      len += decode_unsigned16(&npdu[len], &src->net);
      // SLEN = 0 denotes broadcast MAC SADR and SADR field is absent
      // SLEN > 0 specifies length of SADR field
      src->len = npdu[len++];
      if (src->len)
      {
        for (i = 0; i < src->len; i++)
        {
          src->adr[i] = npdu[len++];
        }
      }
    }
    // The Hop Count field shall be present only if the message is
    // destined for a remote network, i.e., if DNET is present.
    // This is a one-octet field that is initialized to a value of 0xff.
    if (dest && dest->net)
      npdu_data->hop_count = npdu[len++];
    // Indicates that the NSDU conveys a network layer message.
    // Message Type field is present.
    if (npdu_data->network_layer_message)
    {
      npdu_data->network_message_type = npdu[len++];
      // Message Type field contains a value in the range 0x80 - 0xFF,
      // then a Vendor ID field shall be present
      if (npdu_data->network_message_type >= 0x80)
        len += decode_unsigned16(&npdu[len], &npdu_data->vendor_id);
    }
  }

  return len;
}

void npdu_handler(
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
    //FIXME: network layer message received!  Handle it!
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


#ifdef TEST
#include <assert.h>
#include <string.h>
#include "ctest.h"

void testNPDU2(Test * pTest)
{
  uint8_t pdu[480] = {0};
  BACNET_ADDRESS dest = {0};
  BACNET_ADDRESS src = {0};
  BACNET_ADDRESS npdu_dest = {0};
  BACNET_ADDRESS npdu_src = {0};
  int len = 0;
  bool data_expecting_reply = false;  // true for confirmed messages
  BACNET_MESSAGE_PRIORITY priority = MESSAGE_PRIORITY_NORMAL;
  BACNET_NPDU_DATA npdu_data = {0};
  int i = 0; // counter
  int npdu_len = 0;
  bool network_layer_message = false; // false if APDU
  BACNET_NETWORK_MESSAGE_TYPE network_message_type = 0;// optional
  uint16_t vendor_id = 0; // optional, if net message type is > 0x80

  // mac_len = 0 if global address
  dest.mac_len = 6;
  for (i = 0; i < dest.mac_len; i++)
  {
    dest.mac[i] = i;
  }
  // DNET,DLEN,DADR
  dest.net = 1;
  dest.len = 6;
  for (i = 0; i < dest.len; i++)
  {
    dest.adr[i] = i * 10;
  }
  src.mac_len = 1;
  for (i = 0; i < src.mac_len; i++)
  {
    src.mac[i] = 0x80;
  }
  // SNET,SLEN,SADR
  src.net = 2;
  src.len = 1;
  for (i = 0; i < src.len; i++)
  {
    src.adr[i] = 0x40;
  }
  len = npdu_encode_apdu(
    &pdu[0],
    &dest,
    &src,
    data_expecting_reply,
    priority);
  ct_test(pTest, len != 0);
  // can we get the info back?
  npdu_len = npdu_decode(
    &pdu[0],
    &npdu_dest,
    &npdu_src,
    &npdu_data);
  ct_test(pTest, npdu_len != 0);
  ct_test(pTest, npdu_data.data_expecting_reply == data_expecting_reply);
  ct_test(pTest, npdu_data.network_layer_message == network_layer_message);
  ct_test(pTest, npdu_data.network_message_type == network_message_type);
  ct_test(pTest, npdu_data.vendor_id == vendor_id);
  ct_test(pTest, npdu_data.priority == priority);
  // DNET,DLEN,DADR
  ct_test(pTest, npdu_dest.net == dest.net);
  ct_test(pTest, npdu_dest.len == dest.len);
  for (i = 0; i < dest.len; i++)
  {
    ct_test(pTest, npdu_dest.adr[i] == dest.adr[i]);
  }
  // SNET,SLEN,SADR
  ct_test(pTest, npdu_src.net == src.net);
  ct_test(pTest, npdu_src.len == src.len);
  for (i = 0; i < src.len; i++)
  {
    ct_test(pTest, npdu_src.adr[i] == src.adr[i]);
  }
}

void testNPDU1(Test * pTest)
{
  uint8_t pdu[480] = {0};
  BACNET_ADDRESS dest = {0};
  BACNET_ADDRESS src = {0};
  BACNET_ADDRESS npdu_dest = {0};
  BACNET_ADDRESS npdu_src = {0};
  int len = 0;
  bool data_expecting_reply = false;  // true for confirmed messages
  BACNET_MESSAGE_PRIORITY priority = MESSAGE_PRIORITY_NORMAL;
  BACNET_NPDU_DATA npdu_data = {0};
  int i = 0; // counter
  int npdu_len = 0;
  bool network_layer_message = false; // false if APDU
  BACNET_NETWORK_MESSAGE_TYPE network_message_type = 0;// optional
  uint16_t vendor_id = 0; // optional, if net message type is > 0x80

  // mac_len = 0 if global address
  dest.mac_len = 0;
  for (i = 0; i < MAX_MAC_LEN; i++)
  {
    dest.mac[i] = 0;
  }
  // DNET,DLEN,DADR
  dest.net = 0;
  dest.len = 0;
  for (i = 0; i < MAX_MAC_LEN; i++)
  {
    dest.adr[i] = 0;
  }
  src.mac_len = 0;
  for (i = 0; i < MAX_MAC_LEN; i++)
  {
    src.mac[i] = 0;
  }
  // SNET,SLEN,SADR
  src.net = 0;
  src.len = 0;
  for (i = 0; i < MAX_MAC_LEN; i++)
  {
    src.adr[i] = 0;
  }
  len = npdu_encode_apdu(
    &pdu[0],
    &dest,
    &src,
    data_expecting_reply,
    priority);
  ct_test(pTest, len != 0);
  // can we get the info back?
  npdu_len = npdu_decode(
    &pdu[0],
    &npdu_dest,
    &npdu_src,
    &npdu_data);
  ct_test(pTest, npdu_len != 0);
  ct_test(pTest, npdu_data.data_expecting_reply == data_expecting_reply);
  ct_test(pTest, npdu_data.network_layer_message == network_layer_message);
  ct_test(pTest, npdu_data.network_message_type == network_message_type);
  ct_test(pTest, npdu_data.vendor_id == vendor_id);
  ct_test(pTest, npdu_data.priority == priority);
  ct_test(pTest, npdu_dest.mac_len == src.mac_len);
  ct_test(pTest, npdu_src.mac_len == dest.mac_len);
}

#ifdef TEST_NPDU
int main(void)
{
    Test *pTest;
    bool rc;

    pTest = ct_create("BACnet NPDU", NULL);
    /* individual tests */
    rc = ct_addTestFunction(pTest, testNPDU1);
    assert(rc);
    rc = ct_addTestFunction(pTest, testNPDU2);
    assert(rc);

    ct_setStream(pTest, stdout);
    ct_run(pTest);
    (void) ct_report(pTest);
    ct_destroy(pTest);

    return 0;
}
#endif                          /* TEST_NPDU */
#endif                          /* TEST */
