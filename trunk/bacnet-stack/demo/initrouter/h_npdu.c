/*####COPYRIGHTBEGIN####
 -------------------------------------------
 Copyright (C) 2008 Steve Karg

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
#include "bacint.h"
#include "bacenum.h"
#include "bits.h"
#include "npdu.h"
#include "apdu.h"
#define DEBUG_ENABLED 0
#include "debug.h"

#if PRINT_ENABLED
#include <stdio.h>
#endif

extern void router_handler(
    BACNET_ADDRESS *src,
    BACNET_NPDU_DATA *npdu_data,
    uint8_t * npdu,      /* PDU data */
    uint16_t npdu_len);

void npdu_handler(
    BACNET_ADDRESS * src,       /* source address */
    uint8_t * pdu,      /* PDU data */
    uint16_t pdu_len)
{       /* length PDU  */
    int apdu_offset = 0;
    BACNET_ADDRESS dest = { 0 };
    BACNET_NPDU_DATA npdu_data = { 0 };

    apdu_offset = npdu_decode(&pdu[0], &dest, src, &npdu_data);
    if (npdu_data.network_layer_message) {
        router_handler(src,&npdu_data,&pdu[apdu_offset],
            (uint16_t) (pdu_len - apdu_offset));
    } else if ((apdu_offset > 0) && (apdu_offset <= pdu_len)) {
        if ((npdu_data.protocol_version == BACNET_PROTOCOL_VERSION) &&
            ((dest.net == 0) || (dest.net == BACNET_BROADCAST_NETWORK))) {
            /* only handle the version that we know how to handle */
            /* and we are not a router, so ignore messages with
               routing information cause they are not for us */
            apdu_handler(src, &pdu[apdu_offset],
                (uint16_t) (pdu_len - apdu_offset));
        } else {
            if (dest.net) {
                debug_printf("NPDU: DNET=%d.  Discarded!\n", dest.net);
            } else {
                debug_printf("NPDU: BACnet Protocol Version=%d.  Discarded!\n",
                    npdu_data.protocol_version);
            }
        }
    }

    return;
}
