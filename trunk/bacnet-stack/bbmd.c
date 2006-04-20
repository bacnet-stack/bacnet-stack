/*####COPYRIGHTBEGIN####
 -------------------------------------------
 Copyright (C) 2006 Steve Karg

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

#include <stdint.h>             /* for standard integer types uint8_t etc. */
#include <stdbool.h>            /* for the standard bool type. */
#include "bacdcode.h"
#include "bip.h"
#include "net.h"                /* custom per port */

/* Handle the BACnet Broadcast Management Device,
   Broadcast Distribution Table, and Foreign Device Registration */

int bbmd_encode_bvlc_result(
    uint8_t *pdu,
    BACNET_BVLC_RESULT result_code)
{
    if (pdu) {
        pdu[0] = BVLL_TYPE_BACNET_IP;
        pdu[1] = BVLC_RESULT;
        /* The 2-octet BVLC Length field is the length, in octets, 
           of the entire BVLL message, including the two octets of the 
           length field itself, most significant octet first. */
        encode_unsigned16(&pdu[2], 6);
        encode_unsigned16(&pdu[4], result_code);
    }

    return 6;
}

int bbmd_encode_write_bdt_init(
    uint8_t *pdu,
    unsigned entries)
{
    int len = 0;

    if (pdu) {
        pdu[0] = BVLL_TYPE_BACNET_IP;
        pdu[1] = BVLC_WRITE_BROADCAST_DISTRIBUTION_TABLE;
        /* The 2-octet BVLC Length field is the length, in octets, 
           of the entire BVLL message, including the two octets of the 
           length field itself, most significant octet first. */
        encode_unsigned16(&pdu[2], 4 + entries * 10);
        len = 4;
    }

    return len;
}

int bbmd_encode_write_bdt_entry(
    uint8_t *pdu,
    struct in_addr *address,
    uint16_t port,
    struct in_addr *mask)
{
    int len = 0;
    
    if (pdu) {
        encode_unsigned32(&pdu[0], address->s_addr);
        encode_unsigned16(&pdu[4], port);
        encode_unsigned32(&pdu[6], mask->s_addr);
        len = 10;
    }

    return len;
}

int bbmd_encode_read_bdt(
    uint8_t *pdu)
)
{
    int len = 0;
    
    if (pdu) {
        pdu[0] = BVLL_TYPE_BACNET_IP;
        pdu[1] = BVLC_READ_BROADCAST_DISTRIBUTION_TABLE;
        /* The 2-octet BVLC Length field is the length, in octets, 
           of the entire BVLL message, including the two octets of the 
           length field itself, most significant octet first. */
        encode_unsigned16(&pdu[2], 4);
        len = 4;
    }

    return len;
}
   
void bbmd_handler(uint8_t *buf, int len, struct sockaddr_in *sin)
{
  int function_type = 0;
  
    if (buf[0] != BVLL_TYPE_BACNET_IP)
      return;
    function_type = buf[1];
    switch (function_type)
    {
      case BVLC_RESULT:
        break;
      case BVLC_WRITE_BROADCAST_DISTRIBUTION_TABLE:
        break;
      case BVLC_READ_BROADCAST_DISTRIBUTION_TABLE:
        break;
      case BVLC_READ_BROADCAST_DISTRIBUTION_TABLE_ACK:
        break;
      case BVLC_FORWARDED_NPDU:
        break;
      case BVLC_REGISTER_FOREIGN_DEVICE:
        break;
      case BVLC_READ_FOREIGN_DEVICE_TABLE:
        break;
      case BVLC_READ_FOREIGN_DEVICE_TABLE_ACK:
        break;
      case BVLC_DELETE_FOREIGN_DEVICE_TABLE_ENTRY:
        break;
      case BVLC_DISTRIBUTE_BROADCAST_TO_NETWORK:
        break;
      case BVLC_ORIGINAL_BROADCAST_NPDU:
        break;
      default:
        break;
    }
    
    return;
}
